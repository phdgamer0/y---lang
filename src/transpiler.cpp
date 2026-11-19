#include "ylang/transpiler.hpp"
#include "lexer.hpp"
#include <functional>
#include <iostream>

using namespace std;

static std::string serializeValue(const Value &val) {
	switch (val.type) {
	case ValueType::INT:
		return "Value::Int(" + std::to_string(val.asInt()) + "LL)";
	case ValueType::FLOAT:
		return "Value::Float(" + std::to_string(val.fVal) + ")";
	case ValueType::BOOL:
		return "Value::Bool(" + std::string(val.isTruthy() ? "true" : "false") + ")";
	case ValueType::STRING:
		return "Value::String(\"" + val.asString() + "\")";
	case ValueType::NONE:
		return "Value::None()";
	case ValueType::LIST: {
		auto *l = static_cast<ListObject *>(val.ref.get());
		std::string s = "Value::List({";
		for (size_t i = 0; i < l->elements.size(); i++) {
			s += serializeValue(l->elements[i]);
			if (i < l->elements.size() - 1)
				s += ", ";
		}
		return s + "})";
	}
	case ValueType::TUPLE: {
		auto *l = static_cast<TupleObject *>(val.ref.get());
		std::string s = "Value::Tuple({";
		for (size_t i = 0; i < l->elements.size(); i++) {
			s += serializeValue(l->elements[i]);
			if (i < l->elements.size() - 1)
				s += ", ";
		}
		return s + "})";
	}
	case ValueType::SET: {
		auto *l = static_cast<SetObject *>(val.ref.get());
		std::string s = "Value::Set(std::unordered_set<Value, ValueHash, ValueEqual>{";
		size_t i = 0;
		for (const auto &el : l->elements) {
			s += serializeValue(el);
			if (i < l->elements.size() - 1)
				s += ", ";
			i++;
		}
		return s + "})";
	}
	case ValueType::DICT: {
		auto *l = static_cast<DictObject *>(val.ref.get());
		std::string s = "Value::Dict(std::unordered_map<Value, Value, ValueHash, ValueEqual>{";
		size_t i = 0;
		for (const auto &pair : l->items) {
			s += "{" + serializeValue(pair.first) + ", " + serializeValue(pair.second) + "}";
			if (i < l->items.size() - 1)
				s += ", ";
			i++;
		}
		return s + "})";
	}
	case ValueType::VECTOR: {
		auto *l = static_cast<VectorObject *>(val.ref.get());
		std::string s = "Value::Vector(std::vector<Value>{";
		for (size_t i = 0; i < l->elements.size(); i++) {
			s += serializeValue(l->elements[i]);
			if (i < l->elements.size() - 1)
				s += ", ";
		}
		return s + "})";
	}
	case ValueType::FUNCTION:
	case ValueType::NATIVE_FUNCTION:
		return "Value::NoType() /* Dynamic Function not serializable to C++ inline yet */";
	default:
		return "Value::NoType() /* Unhandled transpiler const type */";
	}
}

// `let x = y` stores a shallow copy, exactly when the VM's compiler emits OP_SHALLOW_COPY
static std::string letValue(Expr *value, const std::string &code) {
	switch (value->type) {
	case ExprType::OWNERSHIP:
	case ExprType::LIST:
	case ExprType::DICT:
	case ExprType::SET:
	case ExprType::TUPLE:
	case ExprType::VECTOR:
	case ExprType::LAMBDA:
	case ExprType::NUMBER:
	case ExprType::STRING:
	case ExprType::BOOL:
		return code;
	default:
		return "shallowCopy(__y_val(" + code + "))";
	}
}

// ---------------------------------------------------------------------------
// Local type inference.
// Every param and `let` in a function becomes a LocalDecl. A decl that nothing reaches by name
// (no @x, no lambda call through it, ...) is stored as a C++ local instead of in current_env:
//   NATIVE  every value it ever holds has one static type (long long / double / bool)
//   VALUE   anything else: a boxed Value, still without the hash lookups
// Untyped ints only get NATIVE when no store can overflow (the VM would switch to BigInt);
// `:=` ints may, since the VM rejects that type change anyway (checked ops throw the same ConstError).
// ---------------------------------------------------------------------------
static bool isNativeType(const std::string &t) { return t == "long long" || t == "double" || t == "bool"; }

static std::string baseCppType(std::string t) {
	if (t.rfind("const ", 0) == 0)
		t = t.substr(6);
	if (!t.empty() && t.back() == '&')
		t.pop_back();
	return t;
}

static std::string joinType(const std::string &a, const std::string &b) {
	if (a == "?")
		return b;
	if (b == "?" || a == b)
		return a;
	return "Value";
}

static TokenType compoundOp(TokenType op) {
	switch (op) {
	case TokenType::PLUS_EQ: return TokenType::PLUS;
	case TokenType::MINUS_EQ: return TokenType::MINUS;
	case TokenType::STAR_EQ: return TokenType::STAR;
	case TokenType::DIV_EQ: return TokenType::SLASH;
	default: return TokenType::ASSIGN;
	}
}

static bool intLiteral(NumberExpr *n) {
	if (n->isFloat)
		return false;
	if (n->raw.empty())
		return true;
	try {
		std::stoll(n->raw);
		return true;
	} catch (...) {
		return false;
	}
}

std::string CppTranspiler::exprType(Expr *e, bool checked) {
	if (!e)
		return "";
	switch (e->type) {
	case ExprType::NUMBER: {
		auto n = static_cast<NumberExpr *>(e);
		return n->isFloat ? "double" : intLiteral(n) ? "long long" : "";
	}
	case ExprType::BOOL:
		return "bool";
	case ExprType::VAR: {
		auto v = static_cast<VarExpr *>(e);
		auto it = varDecl.find(e);
		if (it != varDecl.end()) {
			const LocalDecl &d = decls[it->second];
			if (d.escapes)
				return "";
			return d.type == "Value" ? "" : d.type;
		}
		if (auto *nv = lookupNativeVar(v->name)) {
			std::string t = baseCppType(nv->cppType);
			return isNativeType(t) ? t : "";
		}
		if (auto *cs = lookupConst(v->name))
			return cs->type == ConstType::INT ? "long long" : cs->type == ConstType::FLOAT ? "double" : cs->type == ConstType::BOOL ? "bool" : "";
		return "";
	}
	case ExprType::CALL: {
		auto it = userFunctions.find(static_cast<CallExpr *>(e)->name);
		if (it == userFunctions.end() || it->second.returnsRef)
			return "";
		return isNativeType(it->second.returnTypeStr) ? it->second.returnTypeStr : "";
	}
	case ExprType::INC_DEC: {
		auto id = static_cast<IncDecExpr *>(e);
		if (id->expr->type != ExprType::VAR)
			return "";
		std::string t = exprType(id->expr, checked);
		return (t == "long long" || t == "double" || t == "?") ? t : "";
	}
	case ExprType::BINARY: {
		auto b = static_cast<BinExpr *>(e);
		std::string r = exprType(b->right, checked);
		if (!b->left) {
			if (b->op != TokenType::NOT || r.empty())
				return "";
			return r == "?" ? "?" : "bool";
		}
		std::string l = exprType(b->left, checked);
		if (l.empty() || r.empty())
			return "";
		bool num = (l == "long long" || l == "double" || l == "?") && (r == "long long" || r == "double" || r == "?");
		bool ints = l == "long long" && r == "long long";
		bool unknown = l == "?" || r == "?";
		switch (b->op) {
		case TokenType::PLUS:
		case TokenType::MINUS:
		case TokenType::STAR:
			if (!num)
				return "";
			if (unknown)
				return "?";
			if (ints)
				return checked ? "long long" : ""; // may overflow into a BigInt
			return "double";
		case TokenType::SLASH:
			return !num ? "" : unknown ? "?" : "double";
		case TokenType::FLOOR_DIV:
			return !num ? "" : unknown ? "?" : "long long";
		case TokenType::MOD:
			return !num ? "" : unknown ? "?" : ints ? "long long" : "double";
		case TokenType::BITWISE_AND:
		case TokenType::BITWISE_OR:
		case TokenType::BITWISE_XOR:
		case TokenType::BITWISE_SHR:
			if (unknown && num)
				return "?";
			return ints ? "long long" : "";
		case TokenType::LT:
		case TokenType::LTE:
		case TokenType::GT:
		case TokenType::GTE:
			return !num ? "" : unknown ? "?" : "bool";
		case TokenType::EQ:
		case TokenType::NEQ:
			if (unknown)
				return "?";
			return (num || (l == "bool" && r == "bool")) ? "bool" : "";
		case TokenType::AND:
		case TokenType::OR:
			return unknown ? "?" : "bool"; // both sides native: truthiness is static
		default:
			return "";
		}
	}
	default:
		return "";
	}
}

static std::string asDouble(const std::string &code, const std::string &type) {
	return type == "double" ? code : "(double)(" + code + ")";
}

// C++ for an expression whose exprType is native
std::string CppTranspiler::emitNative(Expr *e, bool checked) {
	switch (e->type) {
	case ExprType::NUMBER: {
		auto n = static_cast<NumberExpr *>(e);
		if (!n->isFloat)
			return (n->raw.empty() ? std::to_string((long long)n->val) : n->raw) + "LL";
		std::string raw = n->raw.empty() ? std::to_string(n->val) : n->raw;
		return raw.find_first_of(".eE") == std::string::npos ? raw + ".0" : raw;
	}
	case ExprType::BOOL:
		return static_cast<BoolExpr *>(e)->value ? "true" : "false";
	case ExprType::BINARY: {
		auto b = static_cast<BinExpr *>(e);
		if (!b->left)
			return "(!__y_is_truthy(" + emitNative(b->right, checked) + "))";
		std::string lt = exprType(b->left, checked), rt = exprType(b->right, checked);
		std::string L = emitNative(b->left, checked), R = emitNative(b->right, checked);
		bool ints = lt == "long long" && rt == "long long";
		switch (b->op) {
		case TokenType::PLUS:
			return ints ? "__y_ck_add(" + L + ", " + R + ")" : "(" + asDouble(L, lt) + " + " + asDouble(R, rt) + ")";
		case TokenType::MINUS:
			return ints ? "__y_ck_sub(" + L + ", " + R + ")" : "(" + asDouble(L, lt) + " - " + asDouble(R, rt) + ")";
		case TokenType::STAR:
			return ints ? "__y_ck_mul(" + L + ", " + R + ")" : "(" + asDouble(L, lt) + " * " + asDouble(R, rt) + ")";
		case TokenType::SLASH:
			return "__y_fdiv(" + asDouble(L, lt) + ", " + asDouble(R, rt) + ")";
		case TokenType::FLOOR_DIV:
			return ints ? "__y_ifloordiv(" + L + ", " + R + ")" : "__y_ffloordiv(" + asDouble(L, lt) + ", " + asDouble(R, rt) + ")";
		case TokenType::MOD:
			return ints ? "__y_imod(" + L + ", " + R + ")" : "__y_fmod(" + asDouble(L, lt) + ", " + asDouble(R, rt) + ")";
		case TokenType::BITWISE_AND: return "(" + L + " & " + R + ")";
		case TokenType::BITWISE_OR: return "(" + L + " | " + R + ")";
		case TokenType::BITWISE_XOR: return "(" + L + " ^ " + R + ")";
		case TokenType::BITWISE_SHR: return "__y_shr(" + L + ", " + R + ")";
		case TokenType::LT: return "(" + L + " < " + R + ")";
		case TokenType::LTE: return "(" + L + " <= " + R + ")";
		case TokenType::GT: return "(" + L + " > " + R + ")";
		case TokenType::GTE: return "(" + L + " >= " + R + ")";
		case TokenType::EQ: return "(__y_is_truthy(" + L + ") == __y_is_truthy(" + R + "))"; // truthiness, like OP_EQ
		case TokenType::NEQ: return "(__y_is_truthy(" + L + ") != __y_is_truthy(" + R + "))"; // truthiness, like OP_NEQ
		case TokenType::AND: return "(__y_is_truthy(" + L + ") && __y_is_truthy(" + R + "))";
		case TokenType::OR: return "(__y_is_truthy(" + L + ") || __y_is_truthy(" + R + "))";
		default: break;
		}
		return transpileExpr(e);
	}
	default:
		return transpileExpr(e); // VAR / CALL / INC_DEC already produce the native value
	}
}

// operand of a dynamic helper: bool/double natives are boxed so overload resolution cannot truncate them
std::string CppTranspiler::operand(Expr *e) {
	std::string code = transpileExpr(e);
	std::string t = exprType(e, false);
	return (t == "bool" || t == "double") ? "__y_val(" + code + ")" : code;
}

std::string CppTranspiler::beginScope(const std::string &restorerVar) {
	constScopes.push_back({});
	nativeScopes.push_back({});
	if (restorerVar.empty())
		return "";
	int id = (int)envNeeded.size();
	envNeeded.push_back(0);
	envScopeStack.push_back(id);
	return "        EnvRestorer " + restorerVar + "(current_env); //@envr" + std::to_string(id) + "\n";
}

void CppTranspiler::endScope(bool hadRestorer) {
	constScopes.pop_back();
	nativeScopes.pop_back();
	if (hadRestorer)
		envScopeStack.pop_back();
}

void CppTranspiler::analyzeFunction(FuncStmt *func) {
	decls.clear();
	letDecl.clear();
	varDecl.clear();
	paramDecl.clear();
	std::vector<std::unordered_map<std::string, int>> sc(1);
	auto resolve = [&](const std::string &name) -> int {
		for (auto it = sc.rbegin(); it != sc.rend(); ++it) {
			auto f = it->find(name);
			if (f != it->end())
				return f->second;
		}
		return -1;
	};
	auto escape = [&](const std::string &name) {
		int id = resolve(name);
		if (id >= 0)
			decls[id].escapes = true;
	};

	const auto &info = userFunctions[func->name];
	for (size_t i = 0; i < func->params.size(); i++) {
		const auto &p = func->params[i];
		LocalDecl d;
		d.fixed = true;
		std::string t = baseCppType(info.paramTypeStrs[i]);
		if (isNativeType(t) || t == "std::string") {
			d.kind = LocalDecl::NATIVE; // already a typed C++ param
			d.type = isNativeType(t) ? t : "Value";
		} else if (!p.isVariadic && !p.isKwargs && p.mode != CopyMode::REF && p.type == ValueType::NOTYPE && !p.isConst) {
			d.kind = LocalDecl::VALUE;
			d.type = "Value";
		} else {
			d.kind = LocalDecl::ENV;
			d.type = "Value";
		}
		paramDecl.push_back((int)decls.size());
		sc.back()[p.name] = (int)decls.size();
		decls.push_back(d);
	}

	std::function<void(Expr *)> walkE;
	std::function<void(Stmt *)> walkS;
	auto walkBlock = [&](const std::vector<Stmt *> &b) {
		sc.push_back({});
		for (auto st : b)
			walkS(st);
		sc.pop_back();
	};
	walkE = [&](Expr *e) {
		if (!e)
			return;
		switch (e->type) {
		case ExprType::VAR: {
			int id = resolve(static_cast<VarExpr *>(e)->name);
			if (id >= 0)
				varDecl[e] = id;
			break;
		}
		case ExprType::BINARY: {
			auto b = static_cast<BinExpr *>(e);
			walkE(b->left);
			walkE(b->right);
			break;
		}
		case ExprType::INC_DEC: {
			auto id = static_cast<IncDecExpr *>(e);
			walkE(id->expr);
			if (id->expr->type == ExprType::VAR) {
				int d = resolve(static_cast<VarExpr *>(id->expr)->name);
				if (d >= 0)
					decls[d].incDec = true;
			}
			break;
		}
		case ExprType::CALL: {
			auto c = static_cast<CallExpr *>(e);
			escape(c->name); // a local called by name (a lambda) is looked up in current_env
			for (size_t i = 0; i < c->args.size(); i++) {
				walkE(c->args[i]);
				CopyMode m = i < c->modes.size() ? c->modes[i] : CopyMode::SHALLOW;
				if (m == CopyMode::REF && c->args[i] && c->args[i]->type == ExprType::VAR)
					escape(static_cast<VarExpr *>(c->args[i])->name);
			}
			break;
		}
		case ExprType::OWNERSHIP: {
			auto o = static_cast<OwnershipExpr *>(e);
			walkE(o->expr);
			if (o->mode == CopyMode::REF && o->expr && o->expr->type == ExprType::VAR)
				escape(static_cast<VarExpr *>(o->expr)->name);
			break;
		}
		case ExprType::TERNARY: {
			auto t = static_cast<TernaryExpr *>(e);
			walkE(t->condition);
			walkE(t->trueBranch);
			walkE(t->falseBranch);
			break;
		}
		case ExprType::FSTRING:
			for (auto p : static_cast<FStringExpr *>(e)->parts)
				walkE(p);
			break;
		case ExprType::COMPREHENSION: {
			auto c = static_cast<CompExpr *>(e);
			escape(c->varName);
			walkE(c->expression);
			walkE(c->valueExpr);
			walkE(c->iterable);
			walkE(c->filter);
			break;
		}
		case ExprType::LIST:
			for (auto x : static_cast<ListExpr *>(e)->elements)
				walkE(x);
			break;
		case ExprType::SET:
			for (auto x : static_cast<SetExpr *>(e)->elements)
				walkE(x);
			break;
		case ExprType::TUPLE:
			for (auto x : static_cast<TupleExpr *>(e)->elements)
				walkE(x);
			break;
		case ExprType::VECTOR:
			for (auto x : static_cast<VectorExpr *>(e)->elements)
				walkE(x);
			break;
		case ExprType::DICT:
			for (auto &kv : static_cast<DictExpr *>(e)->items) {
				walkE(kv.first);
				walkE(kv.second);
			}
			break;
		case ExprType::INDEX: {
			auto i = static_cast<IndexExpr *>(e);
			walkE(i->base);
			walkE(i->index);
			break;
		}
		case ExprType::METHOD_CALL: {
			auto m = static_cast<MethodCallExpr *>(e);
			walkE(m->object);
			for (auto a : m->args)
				walkE(a);
			break;
		}
		case ExprType::RANGE: {
			auto r = static_cast<RangeExpr *>(e);
			walkE(r->start);
			walkE(r->end);
			walkE(r->step);
			break;
		}
		case ExprType::SLICE: {
			auto s = static_cast<SliceExpr *>(e);
			walkE(s->start);
			walkE(s->end);
			walkE(s->step);
			break;
		}
		case ExprType::GET:
			walkE(static_cast<GetExpr *>(e)->object);
			break;
		default: // LAMBDA: its body cannot see our locals, so it is not walked
			break;
		}
	};
	walkS = [&](Stmt *s) {
		if (!s)
			return;
		switch (s->type) {
		case StmtType::LET: {
			auto let = static_cast<LetStmt *>(s);
			walkE(let->value);
			LocalDecl d;
			d.init = let->value;
			d.locked = let->isLocked;
			if (let->isConst) {
				d.kind = LocalDecl::CONST;
				d.fixed = true;
				d.type = (let->value && (let->value->type == ExprType::NUMBER || let->value->type == ExprType::BOOL)) ? exprType(let->value, false) : "Value";
				if (d.type.empty())
					d.type = "Value";
			} else if (let->value && let->value->type == ExprType::CALL) {
				auto it = userFunctions.find(static_cast<CallExpr *>(let->value)->name);
				if (it != userFunctions.end() && it->second.returnsRef)
					d.escapes = true; // aliased with setAlias
			}
			letDecl[s] = (int)decls.size();
			sc.back()[let->name] = (int)decls.size();
			decls.push_back(d);
			break;
		}
		case StmtType::MULTI_LET: {
			auto ml = static_cast<MultiLetStmt *>(s);
			for (auto v : ml->values)
				walkE(v);
			for (auto &n : ml->names)
				sc.back()[n] = -1;
			break;
		}
		case StmtType::ASSIGN: {
			auto a = static_cast<AssignStmt *>(s);
			walkE(a->value);
			walkE(a->target);
			if (a->target->type == ExprType::VAR) {
				int id = resolve(static_cast<VarExpr *>(a->target)->name);
				if (id >= 0)
					decls[id].stores.push_back(a);
			}
			break;
		}
		case StmtType::MULTI_ASSIGN: {
			auto ma = static_cast<MultiAssignStmt *>(s);
			for (auto v : ma->values)
				walkE(v);
			for (auto t : ma->targets) {
				walkE(t);
				if (t && t->type == ExprType::VAR)
					escape(static_cast<VarExpr *>(t)->name);
			}
			break;
		}
		case StmtType::DELETE: {
			auto d = static_cast<DeleteStmt *>(s);
			walkE(d->target);
			if (d->target && d->target->type == ExprType::VAR)
				escape(static_cast<VarExpr *>(d->target)->name);
			break;
		}
		case StmtType::IF: {
			auto i = static_cast<IfStmt *>(s);
			walkE(i->condition);
			walkBlock(i->body);
			for (auto &el : i->elifs) {
				walkE(el.first);
				walkBlock(el.second);
			}
			walkBlock(i->elseBody);
			break;
		}
		case StmtType::WHILE: {
			auto w = static_cast<WhileStmt *>(s);
			sc.push_back({});
			walkE(w->condition);
			walkBlock(w->body);
			sc.pop_back();
			break;
		}
		case StmtType::DO_WHILE: {
			auto w = static_cast<DoWhileStmt *>(s);
			sc.push_back({});
			walkBlock(w->body);
			walkE(w->condition);
			sc.pop_back();
			break;
		}
		case StmtType::FOR: {
			auto f = static_cast<ForStmt *>(s);
			sc.push_back({});
			for (auto st : f->inits)
				walkS(st);
			walkE(f->condition);
			for (auto st : f->steps)
				walkS(st);
			walkBlock(f->body);
			sc.pop_back();
			break;
		}
		case StmtType::FOR_EACH: {
			auto f = static_cast<ForEachStmt *>(s);
			sc.push_back({});
			for (auto &v : f->loopVars)
				sc.back()[v] = -1;
			for (auto c : f->collections)
				walkE(c);
			walkBlock(f->body);
			sc.pop_back();
			break;
		}
		case StmtType::SWITCH: {
			auto sw = static_cast<SwitchStmt *>(s);
			walkE(sw->target);
			for (auto &c : sw->cases) {
				walkE(c.value);
				walkBlock(c.body);
			}
			walkBlock(sw->defaultBody);
			break;
		}
		case StmtType::TRY: {
			auto t = static_cast<TryStmt *>(s);
			walkBlock(t->tryBlock);
			for (auto &c : t->catches) {
				sc.push_back({{"e", -1}});
				walkBlock(c.body);
				sc.pop_back();
			}
			walkBlock(t->finallyBlock); // elseBlock is not transpiled
			break;
		}
		case StmtType::RETURN: {
			auto r = static_cast<ReturnStmt *>(s);
			walkE(r->value);
			if (func->returnsRef && r->value && r->value->type == ExprType::VAR)
				escape(static_cast<VarExpr *>(r->value)->name);
			break;
		}
		case StmtType::EXPR:
			walkE(static_cast<ExprStmt *>(s)->expr);
			break;
		case StmtType::ASSERT: {
			auto a = static_cast<AssertStmt *>(s);
			walkE(a->condition);
			walkE(a->message);
			break;
		}
		case StmtType::THROW:
			walkE(static_cast<ThrowStmt *>(s)->message);
			break;
		case StmtType::SKIP:
			walkE(static_cast<SkipStmt *>(s)->count);
			break;
		default:
			break;
		}
	};
	for (auto st : func->body)
		walkS(st);

	// optimistic fixpoint: types only move "?" -> native -> "Value"
	for (auto &d : decls)
		if (d.escapes) {
			d.type = "Value";
			d.fixed = true;
		}
	for (bool changed = true; changed;) {
		changed = false;
		for (auto &d : decls) {
			if (d.fixed)
				continue;
			std::string t = d.init ? exprType(d.init, false) : "Value";
			if (t.empty())
				t = "Value";
			for (AssignStmt *a : d.stores) {
				std::string st;
				if (a->op == TokenType::ASSIGN) {
					st = exprType(a->value, d.locked);
				} else if (compoundOp(a->op) != TokenType::ASSIGN) {
					BinExpr bin(a->target, a->value, compoundOp(a->op));
					st = exprType(&bin, d.locked);
					bin.left = bin.right = nullptr;
				}
				t = joinType(t, st.empty() ? "Value" : st);
			}
			// shortcut: x++ on an untyped int is treated as non-overflowing (2^63 steps), so it stays native
			if (d.incDec && t != "?" && t != "long long" && t != "double")
				t = "Value";
			if (t != d.type) {
				d.type = t;
				changed = true;
			}
		}
	}
	for (auto &d : decls) {
		if (d.escapes) {
			if (d.kind != LocalDecl::NATIVE && d.kind != LocalDecl::CONST)
				d.kind = LocalDecl::ENV;
			continue;
		}
		if (d.fixed)
			continue;
		if (isNativeType(d.type))
			d.kind = LocalDecl::NATIVE;
		else
			d.kind = d.locked ? LocalDecl::ENV : LocalDecl::VALUE; // a locked Value keeps Env's type-lock check
	}
}

void CppTranspiler::generateConstants() {
	stringstream ss;
	ss << "// --- Constant Pool ---\n";
	if (chunk) {
		for (size_t i = 0; i < chunk->constants.size(); i++) {
			Value &val = chunk->constants[i];
			ss << "static Value const_" << i << " = ";

			ss << serializeValue(val) << ";\n";
		}
	}
	ss << "// ---------------------\n\n";
	constantsBlock = ss.str();
}

std::string CppTranspiler::getCppType(ValueType type, CopyMode mode, bool isConst, bool isReturn, bool isRef) {
	if (isReturn) {
		if (isRef) {
			if (type == ValueType::INT)
				return isConst ? "const long long&" : "long long&";
			if (type == ValueType::FLOAT)
				return isConst ? "const double&" : "double&";
			if (type == ValueType::BOOL)
				return isConst ? "const bool&" : "bool&";
			if (type == ValueType::STRING)
				return isConst ? "const std::string&" : "std::string&";
			return "Value";
		}
		if (type == ValueType::INT)
			return "long long";
		if (type == ValueType::FLOAT)
			return "double";
		if (type == ValueType::BOOL)
			return "bool";
		if (type == ValueType::STRING)
			return "std::string";
		if (type == ValueType::NONE)
			return "void";
		if (type == ValueType::NOTYPE)
			return "Value";
		return "Value";
	}

	if (mode == CopyMode::REF) {
		if (type == ValueType::INT)
			return isConst ? "const long long&" : "long long&";
		if (type == ValueType::FLOAT)
			return isConst ? "const double&" : "double&";
		if (type == ValueType::BOOL)
			return isConst ? "const bool&" : "bool&";
		if (type == ValueType::STRING)
			return isConst ? "const std::string&" : "std::string&";
		return isConst ? "const Value&" : "Value&";
	}

	if (type == ValueType::INT)
		return isConst ? "const long long" : "long long";
	if (type == ValueType::FLOAT)
		return isConst ? "const double" : "double";
	if (type == ValueType::BOOL)
		return isConst ? "const bool" : "bool";
	if (type == ValueType::STRING)
		return isConst ? "const std::string&" : "std::string";
	return isConst ? "const Value&" : "Value";
}

static bool containsReturn(const std::vector<Stmt *> &stmts) {
	for (Stmt *st : stmts) {
		if (!st)
			continue;
		if (st->type == StmtType::RETURN)
			return true;
		if (st->type == StmtType::IF) {
			auto ifStmt = static_cast<IfStmt *>(st);
			if (containsReturn(ifStmt->body))
				return true;
			for (const auto &elif : ifStmt->elifs) {
				if (containsReturn(elif.second))
					return true;
			}
			if (containsReturn(ifStmt->elseBody))
				return true;
		} else if (st->type == StmtType::WHILE) {
			auto wStmt = static_cast<WhileStmt *>(st);
			if (containsReturn(wStmt->body))
				return true;
		} else if (st->type == StmtType::DO_WHILE) {
			auto dwStmt = static_cast<DoWhileStmt *>(st);
			if (containsReturn(dwStmt->body))
				return true;
		} else if (st->type == StmtType::FOR) {
			auto fStmt = static_cast<ForStmt *>(st);
			if (containsReturn(fStmt->body))
				return true;
		} else if (st->type == StmtType::FOR_EACH) {
			auto feStmt = static_cast<ForEachStmt *>(st);
			if (containsReturn(feStmt->body))
				return true;
		} else if (st->type == StmtType::TRY) {
			auto tStmt = static_cast<TryStmt *>(st);
			if (containsReturn(tStmt->tryBlock))
				return true;
			for (const auto &c : tStmt->catches) {
				if (containsReturn(c.body))
					return true;
			}
			if (containsReturn(tStmt->elseBlock))
				return true;
			if (containsReturn(tStmt->finallyBlock))
				return true;
		} else if (st->type == StmtType::SWITCH) {
			auto swStmt = static_cast<SwitchStmt *>(st);
			for (const auto &c : swStmt->cases) {
				if (containsReturn(c.body))
					return true;
			}
			if (containsReturn(swStmt->defaultBody))
				return true;
		}
	}
	return false;
}

void CppTranspiler::prePass(const std::vector<Stmt *> &program) {
	for (Stmt *s : program) {
		if (!s || s->type != StmtType::FUNC)
			continue;
		auto func = static_cast<FuncStmt *>(s);
		UserFunctionInfo info;
		info.name = func->name;
		info.cppName = (func->name == "main") ? "y_user_main" : func->name;
		info.returnType = func->returnType;
		info.returnsRef = func->returnsRef;
		info.returnsConst = func->returnsConst;
		info.params = func->params;
		info.variadicIdx = -1;
		info.kwargsIdx = -1;
		for (size_t i = 0; i < func->params.size(); i++) {
			if (func->params[i].isVariadic)
				info.variadicIdx = (int)i;
			if (func->params[i].isKwargs)
				info.kwargsIdx = (int)i;
		}

		bool hasReturn = containsReturn(func->body);

		if (func->returnType == ValueType::NOTYPE && !hasReturn) {
			info.returnTypeStr = "void";
		} else {
			info.returnTypeStr = getCppType(func->returnType, CopyMode::SHALLOW, func->returnsConst, true, func->returnsRef);
		}

		if (func->name == "main") {
			hasUserMain = true;
		}

		std::string paramsStr = "";
		for (size_t i = 0; i < func->params.size(); i++) {
			const auto &p = func->params[i];
			std::string pType;
			if (p.isVariadic || p.isKwargs) {
				pType = "Value";
			} else {
				pType = getCppType(p.type, p.mode, p.isConst, false, p.mode == CopyMode::REF);
			}
			info.paramTypeStrs.push_back(pType);
			paramsStr += pType + " " + p.name;
			if (p.isVariadic) {
				paramsStr += " = Value::Tuple({})";
			} else if (p.isKwargs) {
				paramsStr += " = Value::Dict({})";
			}
			if (i < func->params.size() - 1)
				paramsStr += ", ";
		}

		userFunctions[func->name] = info;

		userFunctionDecls += "inline auto " + info.cppName + "(" + paramsStr + ") -> " + info.returnTypeStr + ";\n";
	}
}

std::string CppTranspiler::transpileExpr(Expr *e) {
	if (!e)
		return "Value::None()";

	switch (e->type) {
	case ExprType::NUMBER: {
		auto num = static_cast<NumberExpr *>(e);
		if (num->isFloat) {
			if (!num->raw.empty())
				return "Value::Float(" + num->raw + ")";
			return "Value::Float(" + to_string(num->val) + ")";
		} else {
			if (!num->raw.empty()) {
				try {
					std::stoll(num->raw);
					return "Value::Int(" + num->raw + "LL)";
				} catch (...) {
					return "Value::BigInt(\"" + num->raw + "\")";
				}
			}
			return "Value::Int(" + to_string((long long)num->val) + "LL)";
		}
	}
	case ExprType::STRING: {
		auto str = static_cast<StringExpr *>(e);
		return "Value::String(\"" + str->val + "\")";
	}
	case ExprType::BOOL: {
		auto b = static_cast<BoolExpr *>(e);
		return string("Value::Bool(") + (b->value ? "true" : "false") + ")";
	}
	case ExprType::VAR: {
		auto v = static_cast<VarExpr *>(e);
		if (auto *nv = lookupNativeVar(v->name)) {
			const std::string &cpp = nv->cppName.empty() ? v->name : nv->cppName;
			return nv->cppType == "Value" ? "__y_rd(" + cpp + ")" : cpp;
		}
		auto *cs = lookupConst(v->name);
		if (cs) {
			if (cs->type == ConstType::STRING) {
				return "Value::String(" + cs->cppName + ")";
			}
			return cs->cppName;
		}
		// Dynamic variable lookup
		return "current_env->get(\"" + v->name + "\")";
	}
	case ExprType::CALL: {
		auto call = static_cast<CallExpr *>(e);
		auto it = userFunctions.find(call->name);
		if (it != userFunctions.end()) {
			const auto &info = it->second;
			std::string targetFunc = info.cppName;

			// Separate positional and keyword arguments
			std::vector<Expr *> posArgs;
			std::vector<CopyMode> posModes;
			std::vector<std::pair<std::string, Expr *>> kwArgs;
			for (size_t i = 0; i < call->args.size(); i++) {
				Expr *arg = call->args[i];
				CopyMode mode = (i < call->modes.size()) ? call->modes[i] : CopyMode::SHALLOW;
				std::string kwName = "";
				Expr *kwVal = nullptr;
				if (arg && arg->type == ExprType::BINARY) {
					auto bin = static_cast<BinExpr *>(arg);
					if (bin->op == TokenType::COLON) {
						if (bin->left && bin->left->type == ExprType::STRING) {
							kwName = static_cast<StringExpr *>(bin->left)->val;
							kwVal = bin->right;
						} else if (bin->left && bin->left->type == ExprType::VAR) {
							kwName = static_cast<VarExpr *>(bin->left)->name;
							kwVal = bin->right;
						}
					}
				}
				if (!kwName.empty()) {
					kwArgs.push_back({kwName, kwVal});
				} else {
					posArgs.push_back(arg);
					posModes.push_back(mode);
				}
			}

			size_t normalParamCount = 0;
			for (const auto &p : info.params) {
				if (!p.isVariadic && !p.isKwargs)
					normalParamCount++;
			}

			auto formatArg = [&](const std::string &paramType, CopyMode mode, Expr *argExpr) -> std::string {
				// `f(x)` to an @ param passes a copy (only `f(@x)` aliases); bind a temporary that lives for the call
				if (mode != CopyMode::REF && paramType.back() == '&' && paramType.rfind("const ", 0) != 0) {
					std::string conv = paramType == "long long&" ? "__y_to_int" : paramType == "double&" ? "__y_to_float" : paramType == "bool&" ? "__y_to_bool" : paramType == "std::string&" ? "__y_to_str" : "__y_val";
					return "__y_tmp_lvalue(" + conv + "(" + transpileExpr(argExpr) + "))";
				}
				if (paramType == "long long&" || paramType == "const long long&") {
					if (auto v = dynamic_cast<VarExpr *>(argExpr)) {
						if (auto *nv = lookupNativeVar(v->name)) {
							if (baseCppType(nv->cppType) == "long long")
								return transpileExpr(argExpr);
							return "__y_to_int(" + transpileExpr(argExpr) + ")";
						} else {
							return "__y_env_int_ref(current_env, \"" + v->name + "\")";
						}
					} else {
						return "__y_to_int(" + transpileExpr(argExpr) + ")";
					}
				} else if (paramType == "double&" || paramType == "const double&") {
					if (auto v = dynamic_cast<VarExpr *>(argExpr)) {
						if (auto *nv = lookupNativeVar(v->name)) {
							if (baseCppType(nv->cppType) == "double")
								return transpileExpr(argExpr);
							return "__y_to_float(" + transpileExpr(argExpr) + ")";
						} else {
							return "__y_env_float_ref(current_env, \"" + v->name + "\")";
						}
					} else {
						return "__y_to_float(" + transpileExpr(argExpr) + ")";
					}
				} else if (paramType == "bool&" || paramType == "const bool&") {
					if (auto v = dynamic_cast<VarExpr *>(argExpr)) {
						if (auto *nv = lookupNativeVar(v->name)) {
							if (baseCppType(nv->cppType) == "bool")
								return transpileExpr(argExpr);
							return "__y_to_bool(" + transpileExpr(argExpr) + ")";
						} else {
							return "__y_env_bool_ref(current_env, \"" + v->name + "\")";
						}
					} else {
						return "__y_to_bool(" + transpileExpr(argExpr) + ")";
					}
				} else if (paramType == "Value&" || paramType == "const Value&") {
					if (auto v = dynamic_cast<VarExpr *>(argExpr)) {
						if (auto *nv = lookupNativeVar(v->name)) {
							if (baseCppType(nv->cppType) == "Value")
								return transpileExpr(argExpr);
							return "__y_val(" + transpileExpr(argExpr) + ")";
						} else {
							return "__y_env_val_ref(current_env, \"" + v->name + "\")";
						}
					} else {
						return "__y_val(" + transpileExpr(argExpr) + ")";
					}
				} else if (exprType(argExpr, true) == baseCppType(paramType)) {
					return emitNative(argExpr, true); // typed param: an overflow is a type error, as in the VM
				} else if (paramType == "long long" || paramType == "const long long") {
					return "__y_to_int(" + transpileExpr(argExpr) + ")";
				} else if (paramType == "double" || paramType == "const double") {
					return "__y_to_float(" + transpileExpr(argExpr) + ")";
				} else if (paramType == "bool" || paramType == "const bool") {
					return "__y_to_bool(" + transpileExpr(argExpr) + ")";
				} else if (paramType == "std::string" || paramType == "const std::string&") {
					return "__y_to_str(" + transpileExpr(argExpr) + ")";
				} else {
					if (mode == CopyMode::REF) {
						if (auto v = dynamic_cast<VarExpr *>(argExpr)) {
							return "__y_make_ref(current_env, \"" + v->name + "\")";
						} else {
							return "__y_val(" + transpileExpr(argExpr) + ")";
						}
					} else if (mode == CopyMode::DEEP) {
						return "deepCopy(__y_val(" + transpileExpr(argExpr) + "))";
					} else {
						return "__y_val(" + transpileExpr(argExpr) + ")";
					}
				}
			};

			std::string argsStr = "";
			std::vector<bool> kwConsumed(kwArgs.size(), false);

			for (size_t i = 0; i < info.params.size(); i++) {
				const auto &p = info.params[i];
				const std::string &paramType = info.paramTypeStrs[i];

				if (p.isVariadic) {
					if (posArgs.size() > normalParamCount) {
						std::string tupleElems = "";
						for (size_t j = normalParamCount; j < posArgs.size(); j++) {
							tupleElems += "__y_val(" + transpileExpr(posArgs[j]) + ")";
							if (j < posArgs.size() - 1)
								tupleElems += ", ";
						}
						argsStr += "Value::Tuple({" + tupleElems + "})";
					} else {
						argsStr += "Value::Tuple({})";
					}
				} else if (p.isKwargs) {
					std::string dictElems = "";
					bool first = true;
					for (size_t j = 0; j < kwArgs.size(); j++) {
						if (!kwConsumed[j]) {
							if (!first)
								dictElems += ", ";
							dictElems += "{ Value::String(\"" + kwArgs[j].first + "\"), __y_val(" + transpileExpr(kwArgs[j].second) + ") }";
							first = false;
						}
					}
					argsStr += "Value::Dict({" + dictElems + "})";
				} else {
					if (i < posArgs.size()) {
						argsStr += formatArg(paramType, posModes[i], posArgs[i]);
					} else {
						bool foundKw = false;
						for (size_t j = 0; j < kwArgs.size(); j++) {
							if (!kwConsumed[j] && kwArgs[j].first == p.name) {
								argsStr += formatArg(paramType, CopyMode::SHALLOW, kwArgs[j].second);
								kwConsumed[j] = true;
								foundKw = true;
								break;
							}
						}
						if (!foundKw) {
							if (p.defaultValue) {
								argsStr += formatArg(paramType, CopyMode::SHALLOW, p.defaultValue);
							} else if (paramType == "long long" || paramType == "const long long") {
								argsStr += "0LL";
							} else if (paramType == "double" || paramType == "const double") {
								argsStr += "0.0";
							} else if (paramType == "bool" || paramType == "const bool") {
								argsStr += "false";
							} else if (paramType == "std::string" || paramType == "const std::string&") {
								argsStr += "\"\"";
							} else {
								argsStr += "Value::NoType()";
							}
						}
					}
				}
				if (i < info.params.size() - 1)
					argsStr += ", ";
			}
			return targetFunc + "(" + argsStr + ")";
		}

		string calleeStr = "current_env->get(\"" + call->name + "\")";
		string argsStr = "";
		for (size_t i = 0; i < call->args.size(); i++) {
			CopyMode mode = (i < call->modes.size()) ? call->modes[i] : CopyMode::SHALLOW;
			if (mode == CopyMode::REF) {
				if (auto v = dynamic_cast<VarExpr *>(call->args[i])) {
					argsStr += "__y_make_ref(current_env, \"" + v->name + "\")";
				} else if (auto idx = dynamic_cast<IndexExpr *>(call->args[i])) {
					argsStr += "Value::FromExisting(std::make_shared<ListRefObject>(std::static_pointer_cast<ListObject>(" + transpileExpr(idx->base) + ".ref), " + "__y_to_int(" + transpileExpr(idx->index) + ")), ValueType::REFERENCE)";
				} else {
					argsStr += "__y_val(" + transpileExpr(call->args[i]) + ")";
				}
			} else if (mode == CopyMode::DEEP) {
				argsStr += "deepCopy(__y_val(" + transpileExpr(call->args[i]) + "))";
			} else {
				argsStr += "__y_val(" + transpileExpr(call->args[i]) + ")";
			}
			if (i < call->args.size() - 1)
				argsStr += ", ";
		}
		// For standard function calls
		return "__y_call(" + calleeStr + ", {" + argsStr + "})";
	}
	case ExprType::LAMBDA: {
		auto lambda = static_cast<LambdaExpr *>(e);
		string lambdaStr = "Value::Native([current_env](const std::vector<Value>& _args, int _pos_count, int _kw_count) mutable -> Value {\n";
		lambdaStr += "        EnvRestorer env_restorer(current_env);\n";

		constScopes.push_back({});

		size_t pos_idx = 0;
		for (const auto &p : lambda->params) {
			bool isLocked = (p.type != ValueType::NOTYPE);
			bool isConst = p.isConst;
			if (p.isVariadic) {
				lambdaStr += "        std::vector<Value> _rest_list;\n";
				lambdaStr += "        for (size_t _i = " + std::to_string(pos_idx) + "; _i < (size_t)_pos_count; _i++) {\n";
				lambdaStr += "            _rest_list.push_back(_args[_i]);\n";
				lambdaStr += "        }\n";
				lambdaStr += "        current_env->set(\"" + p.name + "\", Value::Tuple(_rest_list), false, true);\n";
			} else if (p.isKwargs) {
				lambdaStr += "        std::unordered_map<Value, Value, ValueHash, ValueEqual> _kw_items;\n";
				lambdaStr += "        for (size_t _i = (size_t)_pos_count; _i < _args.size(); _i++) {\n";
				lambdaStr += "            if (_args[_i].type == ValueType::PAIRED) {\n";
				lambdaStr += "                auto* _pairObj = static_cast<PairedObject*>(_args[_i].ref.get());\n";
				lambdaStr += "                for (const auto& entry : _pairObj->pairs) _kw_items[entry.first] = entry.second;\n";
				lambdaStr += "            }\n";
				lambdaStr += "        }\n";
				lambdaStr += "        current_env->set(\"" + p.name + "\", Value::Dict(_kw_items), false, " + (p.isConst ? "true" : "false") + ");\n";
			} else {
				lambdaStr += "        Value _arg_" + p.name + ";\n";
				lambdaStr += "        if (_pos_count > " + std::to_string(pos_idx) + ") {\n";
				lambdaStr += "            _arg_" + p.name + " = _args[" + std::to_string(pos_idx) + "];\n";
				lambdaStr += "        } else {\n";
				if (p.defaultValue) {
					lambdaStr += "            _arg_" + p.name + " = " + transpileExpr(p.defaultValue) + ";\n";
				} else {
					lambdaStr += "            _arg_" + p.name + " = Value::NoType();\n";
				}
				lambdaStr += "        }\n";

				if (p.mode == CopyMode::REF) {
					lambdaStr += "        if (_arg_" + p.name + ".type == ValueType::REFERENCE && _arg_" + p.name + ".get_ptr_safe()) {\n";
					lambdaStr += "            current_env->setAlias(\"" + p.name + "\", _arg_" + p.name + ".get_ptr_safe(), " + (isLocked ? "true" : "false") + ", " + (isConst ? "true" : "false") + ");\n";
					lambdaStr += "        } else {\n";
					lambdaStr += "            current_env->set(\"" + p.name + "\", _arg_" + p.name + ", " + (isLocked ? "true" : "false") + ", " + (isConst ? "true" : "false") + ");\n";
					lambdaStr += "        }\n";
				} else {
					if (p.mode == CopyMode::DEEP) {
						lambdaStr += "        _arg_" + p.name + " = deepCopy(_arg_" + p.name + ");\n";
					}
					lambdaStr += "        current_env->set(\"" + p.name + "\", _arg_" + p.name + ", " + (isLocked ? "true" : "false") + ", " + (isConst ? "true" : "false") + ");\n";
				}
				pos_idx++;
			}
		}

		bool oldReturnsConst = currentFuncReturnsConst;
		bool oldReturnsRef = currentFuncReturnsRef;
		std::string oldNativeReturnType = currentFuncNativeReturnType;
		Expr *oldDefaultRet = currentFuncDefaultRet;
		auto oldNativeScopes = nativeScopes;
		auto oldEnvScopes = envScopeStack; // the lambda has its own Env
		envScopeStack.clear();
		currentFuncReturnsConst = lambda->returnsConst;
		currentFuncReturnsRef = lambda->returnsRef;
		currentFuncNativeReturnType = "Value"; // the generated C++ lambda always returns Value
		currentFuncDefaultRet = lambda->defaultRetArgs.empty() ? nullptr : lambda->defaultRetArgs[0];
		nativeScopes = {{}};

		// Body
		string saved_output = output;
		output = ""; // Temporary redirect

		for (auto stmt : lambda->body) {
			transpileStmt(stmt);
		}
		ReturnStmt fallOff(nullptr); // None, or the declared default
		transpileStmt(&fallOff);

		string bodyOutput = output;
		output = saved_output; // Restore

		constScopes.pop_back();
		currentFuncReturnsConst = oldReturnsConst;
		currentFuncReturnsRef = oldReturnsRef;
		currentFuncNativeReturnType = oldNativeReturnType;
		currentFuncDefaultRet = oldDefaultRet;
		nativeScopes = oldNativeScopes;
		envScopeStack = oldEnvScopes;

		lambdaStr += bodyOutput;
		lambdaStr += "    })";
		return lambdaStr;
	}
	case ExprType::METHOD_CALL: {
		auto mc = static_cast<MethodCallExpr *>(e);
		string objStr = "__y_val(" + transpileExpr(mc->object) + ")";
		string argsStr = "";
		for (size_t i = 0; i < mc->args.size(); i++) {
			argsStr += "__y_val(" + transpileExpr(mc->args[i]) + ")";
			if (i < mc->args.size() - 1)
				argsStr += ", ";
		}
		return "__y_method_call(\"" + mc->method + "\", " + objStr + ", {" + argsStr + "})";
	}
	case ExprType::INDEX: {
		auto idx = static_cast<IndexExpr *>(e);
		string baseStr = "__y_val(" + transpileExpr(idx->base) + ")";
		string idxStr = operand(idx->index);
		return "__y_get_index(" + baseStr + ", " + idxStr + ")";
	}
	case ExprType::LIST: {
		auto lst = static_cast<ListExpr *>(e);
		string s = "Value::List(std::vector<Value>{";
		for (size_t i = 0; i < lst->elements.size(); i++) {
			s += "__y_val(" + transpileExpr(lst->elements[i]) + ")";
			if (i < lst->elements.size() - 1)
				s += ", ";
		}
		s += "})";
		return s;
	}
	case ExprType::INC_DEC: {
		auto incDec = static_cast<IncDecExpr *>(e);
		if (incDec->expr->type == ExprType::VAR) {
			auto varExpr = static_cast<VarExpr *>(incDec->expr);
			if (auto *nv = lookupNativeVar(varExpr->name)) {
				std::string cpp = nv->cppName.empty() ? varExpr->name : nv->cppName;
				std::string op = incDec->isIncrement ? "++" : "--";
				if (nv->cppType == "Value") {
					std::string helper = incDec->isIncrement ? "__y_add" : "__y_sub";
					if (incDec->isPrefix)
						return "[&]() { Value tmp = " + helper + "(__y_rd(" + cpp + "), Value::Int(1LL)); __y_assign_local(" + cpp + ", tmp); return tmp; }()";
					return "[&]() { Value tmp = __y_rd(" + cpp + "); __y_assign_local(" + cpp + ", " + helper + "(tmp, Value::Int(1LL))); return tmp; }()";
				}
				return incDec->isPrefix ? "(" + op + cpp + ")" : "(" + cpp + op + ")";
			}
			string varName = "\"" + varExpr->name + "\"";
			string helper = incDec->isIncrement ? "__y_add" : "__y_sub";
			if (incDec->isPrefix) {
				return "[&]() { Value tmp = " + helper + "(current_env->get(" + varName + "), Value::Int(1LL)); current_env->assign(" + varName + ", tmp); return tmp; }()";
			} else {
				return "[&]() { Value tmp = current_env->get(" + varName + "); current_env->assign(" + varName + ", " + helper + "(tmp, Value::Int(1LL))); return tmp; }()";
			}
		}
		return "Value::None() /* Unsupported IncDec expr */";
	}
	case ExprType::DICT: {
		auto dict = static_cast<DictExpr *>(e);
		string s = "Value::Dict(std::unordered_map<Value, Value, ValueHash, ValueEqual>{";
		for (size_t i = 0; i < dict->items.size(); i++) {
			s += "{__y_val(" + transpileExpr(dict->items[i].first) + "), __y_val(" + transpileExpr(dict->items[i].second) + ")}";
			if (i < dict->items.size() - 1)
				s += ", ";
		}
		s += "})";
		return s;
	}
	case ExprType::OWNERSHIP: {
		auto o = static_cast<OwnershipExpr *>(e);
		if (o->mode == CopyMode::REF) {
			if (auto v = dynamic_cast<VarExpr *>(o->expr)) {
				return "__y_make_ref(current_env, \"" + v->name + "\")";
			} else if (auto idx = dynamic_cast<IndexExpr *>(o->expr)) {
				return "Value::FromExisting(std::make_shared<ListRefObject>(std::static_pointer_cast<ListObject>(" + transpileExpr(idx->base) + ".ref), " + "__y_to_int(" + transpileExpr(idx->index) + ")), ValueType::REFERENCE)";
			}
		} else if (o->mode == CopyMode::DEEP) {
			return "deepCopy(__y_val(" + transpileExpr(o->expr) + "))";
		}
		return transpileExpr(o->expr);
	}
	case ExprType::BINARY: {
		auto bin = static_cast<BinExpr *>(e);
		if (isNativeType(exprType(e, false)))
			return emitNative(e, false);
		if (!bin->left && bin->op == TokenType::NOT) {
			return "Value::Bool(!__y_is_truthy(" + transpileExpr(bin->right) + "))";
		}
		if (!bin->left && bin->op == TokenType::BITWISE_NOT) {
			return "EvaluateConstBinary(TokenType::BITWISE_NOT, Value::Int(0), __y_val(" + transpileExpr(bin->right) + "))";
		}
		string left = operand(bin->left);
		string right = operand(bin->right);
		switch (bin->op) {
		case TokenType::PLUS:
			return "__y_add(" + left + ", " + right + ")";
		case TokenType::MINUS:
			return "__y_sub(" + left + ", " + right + ")";
		case TokenType::STAR:
			return "__y_mul(" + left + ", " + right + ")";
		case TokenType::SLASH:
			return "__y_div(" + left + ", " + right + ")";
		case TokenType::EQ:
			return "Value::Bool(__y_eq(" + left + ", " + right + "))";
		case TokenType::NEQ:
			return "Value::Bool(__y_neq(" + left + ", " + right + "))";
		case TokenType::STRICT_EQ:
			return "Value::Bool(__y_strict_eq(" + left + ", " + right + "))";
		case TokenType::STRICT_NEQ:
			return "Value::Bool(__y_strict_neq(" + left + ", " + right + "))";
		case TokenType::LT:
			return "Value::Bool(__y_lt(" + left + ", " + right + "))";
		case TokenType::LTE:
			return "Value::Bool(__y_lte(" + left + ", " + right + "))";
		case TokenType::GT:
			return "Value::Bool(__y_gt(" + left + ", " + right + "))";
		case TokenType::GTE:
			return "Value::Bool(__y_gte(" + left + ", " + right + "))";
		case TokenType::AND:
			return "Value::Bool(__y_is_truthy(" + left + ") && __y_is_truthy(" + right + "))";
		case TokenType::OR:
			return "Value::Bool(__y_is_truthy(" + left + ") || __y_is_truthy(" + right + "))";
		case TokenType::FLOOR_DIV:
			return "EvaluateConstBinary(TokenType::FLOOR_DIV, __y_val(" + left + "), __y_val(" + right + "))";
		case TokenType::MOD:
			return "EvaluateConstBinary(TokenType::MOD, __y_val(" + left + "), __y_val(" + right + "))";
		case TokenType::POW:
			return "EvaluateConstBinary(TokenType::POW, __y_val(" + left + "), __y_val(" + right + "))";
		case TokenType::BITWISE_AND:
			return "EvaluateConstBinary(TokenType::BITWISE_AND, __y_val(" + left + "), __y_val(" + right + "))";
		case TokenType::BITWISE_OR:
			return "EvaluateConstBinary(TokenType::BITWISE_OR, __y_val(" + left + "), __y_val(" + right + "))";
		case TokenType::BITWISE_XOR:
			return "EvaluateConstBinary(TokenType::BITWISE_XOR, __y_val(" + left + "), __y_val(" + right + "))";
		case TokenType::BITWISE_SHL:
			return "EvaluateConstBinary(TokenType::BITWISE_SHL, __y_val(" + left + "), __y_val(" + right + "))";
		case TokenType::BITWISE_SHR:
			return "EvaluateConstBinary(TokenType::BITWISE_SHR, __y_val(" + left + "), __y_val(" + right + "))";
		case TokenType::IS_IN:
			return "Value::Bool(__y_is_in(__y_val(" + left + "), __y_val(" + right + ")))";
		case TokenType::IS_NOT_IN:
			return "Value::Bool(__y_is_not_in(__y_val(" + left + "), __y_val(" + right + ")))";
		case TokenType::IS:
			return "EvaluateConstBinary(TokenType::IS, __y_val(" + left + "), __y_val(" + right + "))";
		case TokenType::IS_NOT:
			return "EvaluateConstBinary(TokenType::IS_NOT, __y_val(" + left + "), __y_val(" + right + "))";
		case TokenType::COLON:
			return "Value::Paired({ {__y_val(" + left + "), __y_val(" + right + ")} })";
		default:
			break;
		}
		return "EvaluateConstBinary(TokenType::PLUS, __y_val(" + left + "), __y_val(" + right + "))";
	}
	case ExprType::TUPLE: {
		auto tup = static_cast<TupleExpr *>(e);
		string s = "Value::Tuple(std::vector<Value>{";
		for (size_t i = 0; i < tup->elements.size(); i++) {
			s += "__y_val(" + transpileExpr(tup->elements[i]) + ")";
			if (i < tup->elements.size() - 1)
				s += ", ";
		}
		s += "})";
		return s;
	}
	case ExprType::FSTRING: {
		auto fs = static_cast<FStringExpr *>(e);
		string s = "([&]() -> Value { std::string _res = \"\"; ";
		for (auto part : fs->parts) {
			s += "_res += valueToString(__y_val(" + transpileExpr(part) + ")); ";
		}
		s += "return Value::String(_res); })()";
		return s;
	}
	default:
		return "Value::None() /* Unimplemented Expr */";
	}
}

std::string CppTranspiler::transpileCond(Expr *e) {
	if (!e)
		return "true";

	if (exprType(e, false) == "bool")
		return emitNative(e, false);
	if (e->type == ExprType::BINARY) {
		auto bin = static_cast<BinExpr *>(e);
		string left = operand(bin->left);
		string right = operand(bin->right);
		switch (bin->op) {
		case TokenType::LT:
			return "__y_lt(" + left + ", " + right + ")";
		case TokenType::LTE:
			return "__y_lte(" + left + ", " + right + ")";
		case TokenType::GT:
			return "__y_gt(" + left + ", " + right + ")";
		case TokenType::GTE:
			return "__y_gte(" + left + ", " + right + ")";
		case TokenType::EQ:
			return "__y_eq(" + left + ", " + right + ")";
		case TokenType::NEQ:
			return "__y_neq(" + left + ", " + right + ")";
		case TokenType::STRICT_EQ:
			return "__y_strict_eq(" + left + ", " + right + ")";
		case TokenType::STRICT_NEQ:
			return "__y_strict_neq(" + left + ", " + right + ")";
		case TokenType::AND:
			return "(" + transpileCond(bin->left) + " && " + transpileCond(bin->right) + ")";
		case TokenType::OR:
			return "(" + transpileCond(bin->left) + " || " + transpileCond(bin->right) + ")";
		case TokenType::IS_IN:
			return "__y_is_in(__y_val(" + left + "), __y_val(" + right + "))";
		case TokenType::IS_NOT_IN:
			return "__y_is_not_in(__y_val(" + left + "), __y_val(" + right + "))";
		case TokenType::IS:
			return "EvaluateConstBinary(TokenType::IS, __y_val(" + left + "), __y_val(" + right + ")).isTruthy()";
		case TokenType::IS_NOT:
			return "EvaluateConstBinary(TokenType::IS_NOT, __y_val(" + left + "), __y_val(" + right + ")).isTruthy()";
		default:
			break;
		}
	}
	if (e->type == ExprType::BOOL) {
		auto b = static_cast<BoolExpr *>(e);
		return b->value ? "true" : "false";
	}
	return "__y_is_truthy(" + transpileExpr(e) + ")";
}

void CppTranspiler::transpileStmt(Stmt *s) {
	if (!s)
		return;

	switch (s->type) {
	case StmtType::LET: {
		auto let = static_cast<LetStmt *>(s);
		auto declIt = letDecl.find(s);
		if (!let->isConst && declIt != letDecl.end() && !nativeScopes.empty() &&
			 (decls[declIt->second].kind == LocalDecl::NATIVE || decls[declIt->second].kind == LocalDecl::VALUE)) {
			const LocalDecl &d = decls[declIt->second];
			std::string cpp = getUniqueVar("y_" + let->name); // a fresh name per `let`: a redeclaration may change type
			if (d.kind == LocalDecl::NATIVE) {
				std::string conv = d.type == "long long" ? "__y_to_int" : d.type == "double" ? "__y_to_float" : "__y_to_bool";
				std::string val = exprType(let->value, false) == d.type ? emitNative(let->value, false) : conv + "(" + transpileExpr(let->value) + ")";
				output += "    " + d.type + " " + cpp + " = " + val + ";\n";
			} else {
				std::string val = let->value ? letValue(let->value, transpileExpr(let->value)) : "Value::NoType()";
				output += "    Value " + cpp + " = __y_val(" + val + ");\n";
			}
			nativeScopes.back()[let->name] = {d.kind == LocalDecl::NATIVE ? d.type : "Value", false, false, cpp};
			break;
		}
		if (let->isConst && let->value) {
			shadowWithEnv(let->name);
			std::string cppVar = getUniqueVar("c_" + let->name);
			if (let->value->type == ExprType::NUMBER) {
				auto num = static_cast<NumberExpr *>(let->value);
				if (num->isFloat) {
					output += "    constexpr double " + cppVar + " = " + to_string(num->val) + ";\n";
					output += "    current_env->set(\"" + let->name + "\", Value::Float(" + cppVar + "), true, true);\n";
					constScopes.back()[let->name] = {cppVar, ConstType::FLOAT, true};
				} else {
					std::string rawVal = !num->raw.empty() ? num->raw : to_string((long long)num->val);
					bool isBig = false;
					try {
						stoll(rawVal);
					} catch (...) {
						isBig = true;
					}
					if (!isBig) {
						output += "    constexpr long long " + cppVar + " = " + rawVal + "LL;\n";
						output += "    current_env->set(\"" + let->name + "\", Value::Int(" + cppVar + "), true, true);\n";
						constScopes.back()[let->name] = {cppVar, ConstType::INT, true};
					} else {
						output += "    const Value " + cppVar + " = Value::BigInt(\"" + rawVal + "\");\n";
						output += "    current_env->set(\"" + let->name + "\", " + cppVar + ", true, true);\n";
						constScopes.back()[let->name] = {cppVar, ConstType::VALUE, false};
					}
				}
			} else if (let->value->type == ExprType::BOOL) {
				auto b = static_cast<BoolExpr *>(let->value);
				std::string bStr = b->value ? "true" : "false";
				output += "    constexpr bool " + cppVar + " = " + bStr + ";\n";
				output += "    current_env->set(\"" + let->name + "\", Value::Bool(" + cppVar + "), true, true);\n";
				constScopes.back()[let->name] = {cppVar, ConstType::BOOL, true};
			} else if (let->value->type == ExprType::STRING) {
				auto str = static_cast<StringExpr *>(let->value);
				output += "    const std::string " + cppVar + " = \"" + str->val + "\";\n";
				output += "    current_env->set(\"" + let->name + "\", Value::String(" + cppVar + "), true, true);\n";
				constScopes.back()[let->name] = {cppVar, ConstType::STRING, false};
			} else {
				std::string valStr = letValue(let->value, transpileExpr(let->value));
				output += "    const Value " + cppVar + " = " + valStr + ";\n";
				output += "    current_env->set(\"" + let->name + "\", " + cppVar + ", true, true);\n";
				constScopes.back()[let->name] = {cppVar, ConstType::VALUE, false};
			}
		} else {
			if (let->value && let->value->type == ExprType::CALL) {
				auto call = static_cast<CallExpr *>(let->value);
				auto it = userFunctions.find(call->name);
				if (it != userFunctions.end() && it->second.returnsRef) {
					std::string refVarName = "";
					for (size_t i = 0; i < call->args.size(); i++) {
						CopyMode m = (i < call->modes.size()) ? call->modes[i] : CopyMode::SHALLOW;
						if (m == CopyMode::REF) {
							if (auto v = dynamic_cast<VarExpr *>(call->args[i])) {
								refVarName = v->name;
								break;
							}
						}
					}
					string callStr = transpileExpr(let->value);
					output += "    " + callStr + ";\n";
					if (!refVarName.empty()) {
						output += "    current_env->setAlias(\"" + let->name + "\", __y_env_ptr(current_env, \"" + refVarName + "\"), " + (let->isLocked ? "true" : "false") + ", " + (let->isConst ? "true" : "false") + ");\n";
					}
					shadowWithEnv(let->name);
					break;
				}
			}
			string valStr = let->value ? letValue(let->value, transpileExpr(let->value)) : "Value::NoType()";
			output += "    current_env->set(\"" + let->name + "\", __y_val(" + valStr + "), " +
						 (let->isLocked ? "true" : "false") + ", " +
						 (let->isConst ? "true" : "false") + ");\n";
			shadowWithEnv(let->name);
		}
		break;
	}
	case StmtType::MULTI_LET: {
		auto ml = static_cast<MultiLetStmt *>(s);
		for (size_t i = 0; i < ml->names.size(); i++) {
			std::string name = ml->names[i];
			shadowWithEnv(name);
			Expr *val = (i < ml->values.size()) ? ml->values[i] : nullptr;
			bool isConst = (i < ml->isConsts.size()) ? ml->isConsts[i] : false;
			bool isLocked = ml->isLocked;
			if (isConst && val) {
				std::string cppVar = getUniqueVar("c_" + name);
				if (val->type == ExprType::NUMBER) {
					auto num = static_cast<NumberExpr *>(val);
					if (num->isFloat) {
						output += "    constexpr double " + cppVar + " = " + to_string(num->val) + ";\n";
						output += "    current_env->set(\"" + name + "\", Value::Float(" + cppVar + "), true, true);\n";
						constScopes.back()[name] = {cppVar, ConstType::FLOAT, true};
					} else {
						std::string rawVal = !num->raw.empty() ? num->raw : to_string((long long)num->val);
						bool isBig = false;
						try {
							stoll(rawVal);
						} catch (...) {
							isBig = true;
						}
						if (!isBig) {
							output += "    constexpr long long " + cppVar + " = " + rawVal + "LL;\n";
							output += "    current_env->set(\"" + name + "\", Value::Int(" + cppVar + "), true, true);\n";
							constScopes.back()[name] = {cppVar, ConstType::INT, true};
						} else {
							output += "    const Value " + cppVar + " = Value::BigInt(\"" + rawVal + "\");\n";
							output += "    current_env->set(\"" + name + "\", " + cppVar + ", true, true);\n";
							constScopes.back()[name] = {cppVar, ConstType::VALUE, false};
						}
					}
				} else if (val->type == ExprType::BOOL) {
					auto b = static_cast<BoolExpr *>(val);
					std::string bStr = b->value ? "true" : "false";
					output += "    constexpr bool " + cppVar + " = " + bStr + ";\n";
					output += "    current_env->set(\"" + name + "\", Value::Bool(" + cppVar + "), true, true);\n";
					constScopes.back()[name] = {cppVar, ConstType::BOOL, true};
				} else if (val->type == ExprType::STRING) {
					auto str = static_cast<StringExpr *>(val);
					output += "    const std::string " + cppVar + " = \"" + str->val + "\";\n";
					output += "    current_env->set(\"" + name + "\", Value::String(" + cppVar + "), true, true);\n";
					constScopes.back()[name] = {cppVar, ConstType::STRING, false};
				} else {
					std::string valStr = transpileExpr(val);
					output += "    const Value " + cppVar + " = " + valStr + ";\n";
					output += "    current_env->set(\"" + name + "\", " + cppVar + ", true, true);\n";
					constScopes.back()[name] = {cppVar, ConstType::VALUE, false};
				}
			} else {
				std::string valStr = val ? transpileExpr(val) : "Value::NoType()";
				output += "    current_env->set(\"" + name + "\", " + valStr + ", " +
							 (isLocked ? "true" : "false") + ", " +
							 (isConst ? "true" : "false") + ");\n";
			}
		}
		break;
	}
	case StmtType::ASSIGN: {
		auto assign = static_cast<AssignStmt *>(s);
		if (assign->target->type == ExprType::VAR) {
			auto v = static_cast<VarExpr *>(assign->target);
			if (lookupConst(v->name)) {
				output += "    throw RuntimeError(\"Cannot reassign a constant variable '" + v->name + "'\", 0, 0);\n";
				break;
			}
			if (auto *nv = lookupNativeVar(v->name)) {
				if (nv->isConst) {
					output += "    throw RuntimeError(\"Cannot reassign a constant variable '" + v->name + "'\", 0, 0);\n";
					break;
				}
				std::string cpp = nv->cppName.empty() ? v->name : nv->cppName;
				if (nv->cppType == "Value") {
					std::string val = "__y_val(" + transpileExpr(assign->value) + ")";
					const char *helper = assign->op == TokenType::PLUS_EQ ? "__y_add" : assign->op == TokenType::MINUS_EQ ? "__y_sub" : assign->op == TokenType::STAR_EQ ? "__y_mul" : assign->op == TokenType::DIV_EQ ? "__y_div" : nullptr;
					if (helper)
						val = std::string(helper) + "(__y_rd(" + cpp + "), " + val + ")";
					output += "    __y_assign_local(" + cpp + ", " + val + ");\n";
					break;
				}
				std::string bt = baseCppType(nv->cppType);
				if (isNativeType(bt) && (assign->op == TokenType::ASSIGN || compoundOp(assign->op) != TokenType::ASSIGN)) {
					BinExpr bin(assign->target, assign->value, compoundOp(assign->op));
					Expr *rhs = assign->op == TokenType::ASSIGN ? assign->value : &bin;
					if (exprType(rhs, true) == bt) { // stores into typed ints are overflow-checked, like the VM's type lock
						output += "    " + cpp + " = " + emitNative(rhs, true) + ";\n";
						break;
					}
				}
				string valStr = transpileExpr(assign->value);
				if (nv->cppType == "long long" || nv->cppType == "long long&") {
					valStr = "__y_to_int(" + valStr + ")";
				} else if (nv->cppType == "double" || nv->cppType == "double&") {
					valStr = "__y_to_float(" + valStr + ")";
				} else if (nv->cppType == "bool" || nv->cppType == "bool&") {
					valStr = "__y_to_bool(" + valStr + ")";
				} else if (nv->cppType == "std::string" || nv->cppType == "std::string&") {
					valStr = "__y_to_str(" + valStr + ")";
				}
				if (assign->op == TokenType::ASSIGN) {
					output += "    " + cpp + " = " + valStr + ";\n";
				} else if (assign->op == TokenType::PLUS_EQ) {
					output += "    " + cpp + " += " + valStr + ";\n";
				} else if (assign->op == TokenType::MINUS_EQ) {
					output += "    " + cpp + " -= " + valStr + ";\n";
				} else if (assign->op == TokenType::STAR_EQ) {
					output += "    " + cpp + " *= " + valStr + ";\n";
				} else if (assign->op == TokenType::DIV_EQ) {
					output += "    " + cpp + " /= " + valStr + ";\n";
				} else {
					output += "    " + cpp + " = " + valStr + ";\n";
				}
				break;
			}
			string valStr = transpileExpr(assign->value);
			string varName = "\"" + v->name + "\"";
			if (assign->op == TokenType::ASSIGN) {
				output += "    current_env->assign(" + varName + ", __y_val(" + valStr + "));\n";
			} else if (assign->op == TokenType::PLUS_EQ) {
				output += "    current_env->assign(" + varName + ", __y_add(current_env->get(" + varName + "), __y_val(" + valStr + ")));\n";
			} else if (assign->op == TokenType::MINUS_EQ) {
				output += "    current_env->assign(" + varName + ", __y_sub(current_env->get(" + varName + "), __y_val(" + valStr + ")));\n";
			} else if (assign->op == TokenType::STAR_EQ) {
				output += "    current_env->assign(" + varName + ", __y_mul(current_env->get(" + varName + "), __y_val(" + valStr + ")));\n";
			} else if (assign->op == TokenType::DIV_EQ) {
				output += "    current_env->assign(" + varName + ", __y_div(current_env->get(" + varName + "), __y_val(" + valStr + ")));\n";
			} else {
				output += "    current_env->assign(" + varName + ", __y_val(" + valStr + "));\n";
			}
		} else if (assign->target->type == ExprType::INDEX) {
			auto idx = static_cast<IndexExpr *>(assign->target);
			string baseStr = transpileExpr(idx->base);
			string idxStr = operand(idx->index);
			string valStr = "__y_val(" + transpileExpr(assign->value) + ")";
			const char *helper = assign->op == TokenType::PLUS_EQ ? "__y_add" : assign->op == TokenType::MINUS_EQ ? "__y_sub" : assign->op == TokenType::STAR_EQ ? "__y_mul" : assign->op == TokenType::DIV_EQ ? "__y_div" : nullptr;
			if (helper)
				valStr = std::string(helper) + "(__y_get_index(" + baseStr + ", " + idxStr + "), " + valStr + ")";
			output += "    __y_set_index(" + baseStr + ", " + idxStr + ", " + valStr + ");\n";
		}
		break;
	}
	case StmtType::IF: {
		auto ifStmt = static_cast<IfStmt *>(s);
		output += "    if (" + transpileCond(ifStmt->condition) + ") {\n";
		output += beginScope("env_restorer");
		for (auto stmt : ifStmt->body)
			transpileStmt(stmt);
		endScope(true);
		output += "    }\n";
		for (const auto &elif : ifStmt->elifs) {
			output += "    else if (" + transpileCond(elif.first) + ") {\n";
			output += beginScope("env_restorer");
			for (auto stmt : elif.second)
				transpileStmt(stmt);
			endScope(true);
			output += "    }\n";
		}
		if (!ifStmt->elseBody.empty()) {
			output += "    else {\n";
			output += beginScope("env_restorer");
			for (auto stmt : ifStmt->elseBody)
				transpileStmt(stmt);
			endScope(true);
			output += "    }\n";
		}
		break;
	}
	case StmtType::WHILE: {
		auto wStmt = static_cast<WhileStmt *>(s);
		int loopId = loopIdCounter++;
		loopStack.push_back({loopId, false});
		output += "    {\n";
		output += beginScope("env_restorer_loop");
		output += "        while (" + transpileCond(wStmt->condition) + ") {\n";
		beginScope("");
		for (auto stmt : wStmt->body)
			transpileStmt(stmt);
		endScope(false);
		endScope(true);
		output += "        }\n";
		output += "    }\n";
		loopStack.pop_back();
		break;
	}
	case StmtType::DO_WHILE: {
		auto dw = static_cast<DoWhileStmt *>(s);
		int loopId = loopIdCounter++;
		loopStack.push_back({loopId, false});
		output += "    {\n";
		output += beginScope("env_restorer_loop");
		output += "        do {\n";
		beginScope("");
		for (auto stmt : dw->body)
			transpileStmt(stmt);
		endScope(false);
		output += "        } while (" + transpileCond(dw->condition) + ");\n";
		endScope(true);
		output += "    }\n";
		loopStack.pop_back();
		break;
	}
	case StmtType::FOR: {
		auto fStmt = static_cast<ForStmt *>(s);
		int loopId = loopIdCounter++;
		loopStack.push_back({loopId, false});
		output += "    {\n";
		output += beginScope("env_restorer_outer");
		for (auto stmt : fStmt->inits)
			transpileStmt(stmt);
		output += "        for (; " + transpileCond(fStmt->condition) + "; ";
		if (!fStmt->steps.empty()) {
			output += "[&]() {\n";
			for (auto stmt : fStmt->steps)
				transpileStmt(stmt);
			output += "        }()";
		}
		output += ") {\n";
		beginScope("");
		for (auto stmt : fStmt->body)
			transpileStmt(stmt);
		endScope(false);
		output += "        }\n";
		endScope(true);
		output += "    }\n";
		loopStack.pop_back();
		break;
	}
	case StmtType::FOR_EACH: {
		auto fe = static_cast<ForEachStmt *>(s);
		int loopId = loopIdCounter++;
		loopStack.push_back({loopId, true});
		output += "    {\n";
		output += beginScope("env_restorer_outer");
		for (const auto &lv : fe->loopVars)
			shadowWithEnv(lv);
		output += "        long long _iter_idx_" + std::to_string(loopId) + " = 0;\n";
		bool isDictUnpack = (fe->collections.size() == 1 && fe->loopVars.size() == 2);
		if (fe->collections.size() == 1 && fe->loopVars.size() == 1) {
			std::string colExpr = transpileExpr(fe->collections[0]);
			std::string loopVar = fe->loopVars[0];
			std::string colVar = "_col_" + std::to_string(loopId);
			std::string vecVar = "_vec_" + std::to_string(loopId);
			std::string idxVar = "_iter_idx_" + std::to_string(loopId);

			std::string saved_output = output;
			output = "";
			beginScope("");
			for (auto stmt : fe->body)
				transpileStmt(stmt);
			endScope(false);
			std::string bodyOutput = output;
			output = saved_output;

			output += "        Value " + colVar + " = " + colExpr + ";\n";
			output += "        if (const auto* " + vecVar + " = __y_get_vector_ptr(" + colVar + ")) {\n";
			output += "            size_t _len_" + std::to_string(loopId) + " = " + vecVar + "->size();\n";
			output += "            for (; (size_t)" + idxVar + " < _len_" + std::to_string(loopId) + "; " + idxVar + "++) {\n";
			output += "                current_env->set(\"" + loopVar + "\", (*" + vecVar + ")[" + idxVar + "], false, false);\n";
			output += bodyOutput;
			output += "            }\n";
			output += "        } else {\n";
			output += "            auto _items_" + std::to_string(loopId) + " = __y_to_vector(" + colVar + ");\n";
			output += "            size_t _len_" + std::to_string(loopId) + " = _items_" + std::to_string(loopId) + ".size();\n";
			output += "            for (; (size_t)" + idxVar + " < _len_" + std::to_string(loopId) + "; " + idxVar + "++) {\n";
			output += "                current_env->set(\"" + loopVar + "\", _items_" + std::to_string(loopId) + "[" + idxVar + "], false, false);\n";
			output += bodyOutput;
			output += "            }\n";
			output += "        }\n";
		} else if (isDictUnpack) {
			std::string colExpr = transpileExpr(fe->collections[0]);
			std::string kVar = fe->loopVars[0];
			std::string vVar = fe->loopVars[1];
			std::string colVar = "_dict_val_" + std::to_string(loopId);
			std::string dictVar = "_dict_ptr_" + std::to_string(loopId);

			std::string saved_output = output;
			output = "";
			beginScope("");
			for (auto stmt : fe->body)
				transpileStmt(stmt);
			endScope(false);
			std::string bodyOutput = output;
			output = saved_output;

			output += "        Value " + colVar + " = " + colExpr + ";\n";
			output += "        if (const auto* " + dictVar + " = __y_get_dict_ptr(" + colVar + ")) {\n";
			output += "            for (const auto& [_k, _v] : *" + dictVar + ") {\n";
			output += "                current_env->set(\"" + kVar + "\", _k, false, false);\n";
			output += "                current_env->set(\"" + vVar + "\", _v, false, false);\n";
			output += bodyOutput;
			output += "            }\n";
			output += "        } else {\n";
			output += "            auto _items_" + std::to_string(loopId) + " = __y_to_vector(" + colVar + ");\n";
			output += "            for (const auto& _pair : _items_" + std::to_string(loopId) + ") {\n";
			output += "                current_env->set(\"" + kVar + "\", __y_get_index(_pair, 0LL), false, false);\n";
			output += "                current_env->set(\"" + vVar + "\", __y_get_index(_pair, 1LL), false, false);\n";
			output += bodyOutput;
			output += "            }\n";
			output += "        }\n";
		} else {
			output += "        std::vector<std::vector<Value>> _streams;\n";
			for (auto col : fe->collections) {
				output += "        _streams.push_back(__y_to_vector(" + transpileExpr(col) + "));\n";
			}
			output += "        size_t _min_len_" + std::to_string(loopId) + " = _streams.empty() ? 0 : _streams[0].size();\n";
			output += "        for (size_t _si = 1; _si < _streams.size(); _si++) { if (_streams[_si].size() < _min_len_" + std::to_string(loopId) + ") _min_len_" + std::to_string(loopId) + " = _streams[_si].size(); }\n";
			output += "        for (; (size_t)_iter_idx_" + std::to_string(loopId) + " < _min_len_" + std::to_string(loopId) + "; _iter_idx_" + std::to_string(loopId) + "++) {\n";
			beginScope("");
			for (size_t i = 0; i < fe->loopVars.size(); i++) {
				output += "            current_env->set(\"" + fe->loopVars[i] + "\", _streams[" + std::to_string(i) + "][_iter_idx_" + std::to_string(loopId) + "], false, false);\n";
			}
			for (auto stmt : fe->body)
				transpileStmt(stmt);
			endScope(false);
			output += "        }\n";
		}
		endScope(true);
		output += "    }\n";
		loopStack.pop_back();
		break;
	}
	case StmtType::SKIP: {
		auto sk = static_cast<SkipStmt *>(s);
		if (loopStack.empty() || !loopStack.back().second) {
			output += "    /* ERROR: skip statement outside of for-each loop */\n";
			break;
		}
		int loopId = loopStack.back().first;
		output += "    _iter_idx_" + std::to_string(loopId) + " += __y_to_int(" + transpileExpr(sk->count) + ");\n";
		break;
	}

	case StmtType::BREAK: {
		output += "    break;\n";
		break;
	}
	case StmtType::CONTINUE: {
		output += "    continue;\n";
		break;
	}
	case StmtType::RETURN: {
		auto ret = static_cast<ReturnStmt *>(s);
		if (currentFuncNativeReturnType == "void") {
			if (ret->value) {
				output += "    " + transpileExpr(ret->value) + "; return;\n";
			} else {
				output += "    return;\n";
			}
			break;
		}

		Expr *retExpr = ret->value ? ret->value : currentFuncDefaultRet;
		string valStr;
		if (currentFuncReturnsRef && ret->value) {
			if (auto v = dynamic_cast<VarExpr *>(ret->value)) {
				if (lookupNativeVar(v->name)) {
					valStr = v->name;
				} else {
					valStr = "__y_make_ref(current_env, \"" + v->name + "\")";
				}
			} else if (auto idx = dynamic_cast<IndexExpr *>(ret->value)) {
				valStr = "Value::FromExisting(std::make_shared<ListRefObject>(std::static_pointer_cast<ListObject>(" + transpileExpr(idx->base) + ".ref), " + "__y_to_int(" + transpileExpr(idx->index) + ")), ValueType::REFERENCE)";
			} else {
				valStr = transpileExpr(ret->value);
			}
		} else {
			valStr = retExpr ? transpileExpr(retExpr) : "Value::None()";
		}

		if (retExpr && !(currentFuncReturnsRef && ret->value) && isNativeType(currentFuncNativeReturnType) &&
			 exprType(retExpr, true) == currentFuncNativeReturnType) {
			output += "    return " + emitNative(retExpr, true) + ";\n";
		} else if (currentFuncNativeReturnType == "long long") {
			output += "    return __y_to_int(" + valStr + ");\n";
		} else if (currentFuncNativeReturnType == "double") {
			output += "    return __y_to_float(" + valStr + ");\n";
		} else if (currentFuncNativeReturnType == "bool") {
			output += "    return __y_to_bool(" + valStr + ");\n";
		} else if (currentFuncNativeReturnType == "std::string") {
			output += "    return __y_to_str(" + valStr + ");\n";
		} else if (currentFuncNativeReturnType == "long long&" ||
					  currentFuncNativeReturnType == "const long long&" ||
					  currentFuncNativeReturnType == "double&" ||
					  currentFuncNativeReturnType == "const double&" ||
					  currentFuncNativeReturnType == "bool&" ||
					  currentFuncNativeReturnType == "const bool&" ||
					  currentFuncNativeReturnType == "std::string&" ||
					  currentFuncNativeReturnType == "const std::string&" ||
					  currentFuncNativeReturnType == "Value&" ||
					  currentFuncNativeReturnType == "const Value&") {
			output += "    return " + valStr + ";\n";
		} else {
			if (currentFuncReturnsConst) {
				output += "    { Value _retVal = __y_val(" + valStr + "); _retVal.isConst = true; return _retVal; }\n";
			} else {
				output += "    return __y_val(" + valStr + ");\n";
			}
		}
		break;
	}
	case StmtType::SWITCH: {
		auto sw = static_cast<SwitchStmt *>(s);
		string targetStr = transpileExpr(sw->target);
		output += "    switch(0) { case 0: {\n";
		output += "        Value __switch_target = __y_val(" + targetStr + ");\n";
		bool firstCase = true;
		for (const auto &c : sw->cases) {
			string caseValStr = transpileExpr(c.value);
			if (firstCase) {
				output += "        if (__y_strict_eq(__switch_target, " + caseValStr + ")) {\n";
				firstCase = false;
			} else {
				output += "        else if (__y_strict_eq(__switch_target, " + caseValStr + ")) {\n";
			}
			output += beginScope("env_restorer");
			for (auto stmt : c.body)
				transpileStmt(stmt);
			endScope(true);
			output += "        }\n";
		}
		if (!sw->defaultBody.empty()) {
			if (firstCase) {
				output += "        {\n";
			} else {
				output += "        else {\n";
			}
			output += beginScope("env_restorer");
			for (auto stmt : sw->defaultBody)
				transpileStmt(stmt);
			endScope(true);
			output += "        }\n";
		}
		output += "    } break; }\n";
		break;
	}
	case StmtType::ASSERT: {
		auto astmt = static_cast<AssertStmt *>(s);
		output += "    if (!" + transpileCond(astmt->condition) + ") {\n";
		if (astmt->message) {
			output += "        throw AssertionError(__y_val(" + transpileExpr(astmt->message) + ").asString(), 0, 0);\n";
		} else {
			output += "        throw AssertionError(\"Assertion failed\", 0, 0);\n";
		}
		output += "    }\n";
		break;
	}
	case StmtType::THROW: {
		auto tstmt = static_cast<ThrowStmt *>(s);
		output += "    throw " + tstmt->errorType + "(__y_val(" + transpileExpr(tstmt->message) + ").asString(), 0, 0);\n";
		break;
	}
	case StmtType::TRY: {
		auto tstmt = static_cast<TryStmt *>(s);
		output += "    try {\n";
		output += beginScope("env_restorer");
		for (auto stmt : tstmt->tryBlock)
			transpileStmt(stmt);
		endScope(true);
		output += "    }\n";
		for (const auto &catchBlock : tstmt->catches) {
			// YLang exceptions are C++ exceptions (e.g. RuntimeError)
			// Catch block types are string names of exceptions
			// We will catch std::exception for simplicity and check type, or generate specific catch blocks
			// For simplicity in C++, we can catch the base class `YError` or standard exceptions
			output += "    catch (std::exception& e) {\n";
			output += beginScope("env_restorer");
			shadowWithEnv("e");
			output += "        current_env->set(\"e\", Value::String(e.what()), false, false);\n";
			for (auto stmt : catchBlock.body)
				transpileStmt(stmt);
			endScope(true);
			output += "    }\n";
		}
		if (!tstmt->finallyBlock.empty()) {
			output += "    // NOTE: C++ doesn't have finally block natively for try/catch, using RAII or scope exit in future.\n";
			output += "    {\n";
			output += beginScope("env_restorer");
			for (auto stmt : tstmt->finallyBlock)
				transpileStmt(stmt);
			endScope(true);
			output += "    }\n";
		}
		break;
	}
	case StmtType::EXPR: {
		auto exprStmt = reinterpret_cast<struct ExprStmt *>(s);
		output += "    " + transpileExpr(exprStmt->expr) + ";\n";
		break;
	}
	case StmtType::FUNC: {
		auto func = reinterpret_cast<struct FuncStmt *>(s);
		auto it = userFunctions.find(func->name);
		if (it == userFunctions.end())
			break;
		const auto &info = it->second;

		std::string funcDef = "";
		std::string paramsStr = "";
		for (size_t i = 0; i < info.params.size(); i++) {
			paramsStr += info.paramTypeStrs[i] + " " + info.params[i].name;
			if (i < info.params.size() - 1)
				paramsStr += ", ";
		}

		funcDef += "inline auto " + info.cppName + "(" + paramsStr + ") -> " + info.returnTypeStr + " {\n";
		funcDef += "    auto current_env = std::make_shared<Env>();\n";
		funcDef += "    current_env->parent = y_transpile_globals;\n";

		analyzeFunction(func);
		nativeScopes.push_back({});
		constScopes.push_back({});

		for (size_t i = 0; i < info.params.size(); i++) {
			const auto &p = info.params[i];
			const std::string &pType = info.paramTypeStrs[i];

			if (p.isVariadic) {
				funcDef += "    current_env->set(\"" + p.name + "\", " + p.name + ", false, true);\n";
			} else if (p.isKwargs) {
				funcDef += "    current_env->set(\"" + p.name + "\", " + p.name + ", false, " + (p.isConst ? "true" : "false") + ");\n";
			} else if (pType == "long long" || pType == "const long long" ||
						  pType == "long long&" || pType == "const long long&" ||
						  pType == "double" || pType == "const double" ||
						  pType == "double&" || pType == "const double&" ||
						  pType == "bool" || pType == "const bool" ||
						  pType == "bool&" || pType == "const bool&" ||
						  pType == "std::string" || pType == "const std::string&" ||
						  pType == "std::string&") {
				nativeScopes.back()[p.name] = {pType, (p.mode == CopyMode::REF), p.isConst};
			} else if (decls[paramDecl[i]].kind == LocalDecl::VALUE) {
				nativeScopes.back()[p.name] = {"Value", false, false, p.name}; // the C++ param itself, no Env slot
			} else {
				bool isLocked = (p.type != ValueType::NOTYPE);
				if (p.mode == CopyMode::REF) {
					funcDef += "    if (" + p.name + ".type == ValueType::REFERENCE && " + p.name + ".get_ptr_safe()) {\n";
					funcDef += "        current_env->setAlias(\"" + p.name + "\", " + p.name + ".get_ptr_safe(), " + (isLocked ? "true" : "false") + ", " + (p.isConst ? "true" : "false") + ");\n";
					funcDef += "    } else {\n";
					funcDef += "        current_env->setAlias(\"" + p.name + "\", const_cast<Value*>(&" + p.name + "), " + (isLocked ? "true" : "false") + ", " + (p.isConst ? "true" : "false") + ");\n";
					funcDef += "    }\n";
				} else {
					funcDef += "    current_env->set(\"" + p.name + "\", " + p.name + ", " + (isLocked ? "true" : "false") + ", " + (p.isConst ? "true" : "false") + ");\n";
				}
			}
		}

		bool oldReturnsConst = currentFuncReturnsConst;
		bool oldReturnsRef = currentFuncReturnsRef;
		std::string oldNativeReturnType = currentFuncNativeReturnType;
		Expr *oldDefaultRet = currentFuncDefaultRet;
		currentFuncReturnsConst = func->returnsConst;
		currentFuncReturnsRef = func->returnsRef;
		currentFuncNativeReturnType = info.returnTypeStr;
		currentFuncDefaultRet = func->defaultRetArgs.empty() ? nullptr : func->defaultRetArgs[0];

		std::string saved_output = output;
		output = "";
		for (auto stmt : func->body) {
			transpileStmt(stmt);
		}
		if (currentFuncDefaultRet && info.returnTypeStr != "void") {
			ReturnStmt fallOff(nullptr); // emits the declared default
			transpileStmt(&fallOff);
		} else if (info.returnTypeStr == "void") {
			output += "    return;\n";
		} else if (info.returnTypeStr == "Value") {
			output += "    return Value::None();\n";
		} else if (info.returnTypeStr == "long long") {
			output += "    return 0LL;\n";
		} else if (info.returnTypeStr == "double") {
			output += "    return 0.0;\n";
		} else if (info.returnTypeStr == "bool") {
			output += "    return false;\n";
		} else if (info.returnTypeStr == "std::string") {
			output += "    return \"\";\n";
		}

		// a body that never touches current_env (all locals native) needs no Env: skip the per-call and per-block allocations
		std::string body;
		for (size_t pos = 0; pos < output.size();) {
			size_t end = output.find('\n', pos);
			end = (end == std::string::npos) ? output.size() : end + 1;
			std::string_view line(output.data() + pos, end - pos);
			if (line.find("EnvRestorer ") == std::string_view::npos)
				body += line;
			pos = end;
		}
		if (funcDef.find("current_env->set") == std::string::npos && body.find("current_env") == std::string::npos) {
			funcDef.erase(funcDef.find("    auto current_env"));
			funcDef += body;
		} else {
			funcDef += output;
		}
		funcDef += "}\n\n";

		userFunctionDefs += funcDef;

		output = saved_output;
		nativeScopes.pop_back();
		constScopes.pop_back();
		currentFuncReturnsConst = oldReturnsConst;
		currentFuncReturnsRef = oldReturnsRef;
		currentFuncNativeReturnType = oldNativeReturnType;
		currentFuncDefaultRet = oldDefaultRet;

		// Generate reflection wrapper for globals in main()
		if (func->name != "main") {
			std::string wrapper = "    y_transpile_globals->set(\"" + func->name + "\", Value::Native([](const std::vector<Value>& _args, int _pos_count, int _kw_count) mutable -> Value {\n";

			size_t normalParamCount = 0;
			for (const auto &p : info.params) {
				if (!p.isVariadic && !p.isKwargs)
					normalParamCount++;
			}

			if (info.variadicIdx != -1) {
				wrapper += "        std::vector<Value> _rest_tuple;\n";
				wrapper += "        for (size_t _i = " + std::to_string(normalParamCount) + "; _i < (size_t)_pos_count; _i++) {\n";
				wrapper += "            _rest_tuple.push_back(_args[_i]);\n";
				wrapper += "        }\n";
				wrapper += "        Value _variadic_val = Value::Tuple(_rest_tuple);\n";
			}

			if (info.kwargsIdx != -1) {
				wrapper += "        std::unordered_map<Value, Value, ValueHash, ValueEqual> _kw_items;\n";
				wrapper += "        for (size_t _i = (size_t)_pos_count; _i < _args.size(); _i++) {\n";
				wrapper += "            if (_args[_i].type == ValueType::PAIRED) {\n";
				wrapper += "                auto* _pairObj = static_cast<PairedObject*>(_args[_i].ref.get());\n";
				wrapper += "                for (const auto& entry : _pairObj->pairs) _kw_items[entry.first] = entry.second;\n";
				wrapper += "            }\n";
				wrapper += "        }\n";
				wrapper += "        Value _kwargs_val = Value::Dict(_kw_items);\n";
			}

			std::string callArgs = "";
			for (size_t i = 0; i < info.params.size(); i++) {
				const auto &p = info.params[i];
				const auto &pType = info.paramTypeStrs[i];
				std::string idx = std::to_string(i);

				if (p.isVariadic) {
					callArgs += "_variadic_val";
				} else if (p.isKwargs) {
					callArgs += "_kwargs_val";
				} else if (pType == "long long" || pType == "const long long") {
					callArgs += "(_pos_count > " + idx + " ? _args[" + idx + "].asInt() : 0LL)";
				} else if (pType == "long long&" || pType == "const long long&") {
					callArgs += "(_pos_count > " + idx + " ? (_args[" + idx + "].get_ptr_safe() ? _args[" + idx + "].get_ptr_safe()->iVal : const_cast<Value&>(_args[" + idx + "]).iVal) : const_cast<Value&>(_args[" + idx + "]).iVal)";
				} else if (pType == "double" || pType == "const double") {
					callArgs += "(_pos_count > " + idx + " ? _args[" + idx + "].asFloat() : 0.0)";
				} else if (pType == "double&" || pType == "const double&") {
					callArgs += "(_pos_count > " + idx + " ? (_args[" + idx + "].get_ptr_safe() ? _args[" + idx + "].get_ptr_safe()->fVal : const_cast<Value&>(_args[" + idx + "]).fVal) : const_cast<Value&>(_args[" + idx + "]).fVal)";
				} else if (pType == "bool" || pType == "const bool") {
					callArgs += "(_pos_count > " + idx + " ? _args[" + idx + "].isTruthy() : false)";
				} else if (pType == "bool&" || pType == "const bool&") {
					callArgs += "(_pos_count > " + idx + " ? (_args[" + idx + "].get_ptr_safe() ? _args[" + idx + "].get_ptr_safe()->bVal : const_cast<Value&>(_args[" + idx + "]).bVal) : const_cast<Value&>(_args[" + idx + "]).bVal)";
				} else if (pType == "std::string" || pType == "const std::string&") {
					callArgs += "(_pos_count > " + idx + " ? _args[" + idx + "].asString() : \"\")";
				} else if (pType == "Value&" || pType == "const Value&") {
					callArgs += "(_pos_count > " + idx + " ? (_args[" + idx + "].get_ptr_safe() ? *_args[" + idx + "].get_ptr_safe() : const_cast<Value&>(_args[" + idx + "])) : const_cast<Value&>(_args[" + idx + "]))";
				} else {
					callArgs += "(_pos_count > " + idx + " ? _args[" + idx + "] : Value::NoType())";
				}
				if (i < info.params.size() - 1)
					callArgs += ", ";
			}
			if (info.returnTypeStr == "void") {
				wrapper += "        " + info.cppName + "(" + callArgs + ");\n";
				wrapper += "        return Value::None();\n";
			} else {
				wrapper += "        return __y_val(" + info.cppName + "(" + callArgs + "));\n";
			}
			wrapper += "    }), false, false);\n";
			userFunctionRegistrations += wrapper;
		}

		break;
	}
	default:
		output += "    // Unimplemented Stmt type: " + to_string((int)s->type) + "\n";
		break;
	}
}

std::string CppTranspiler::finalize(const std::string &scriptName) {
	stringstream ss;
	ss << "// Auto generated by y-- transpiler DO NOT EDIT THIS FILE!!!\n";
	ss << "#include \"ylang/transpiler_runtime.hpp\"\n\n";

	ss << "// --- User Function Declarations ---\n";
	ss << userFunctionDecls << "\n";
	ss << "// --- User Function Definitions ---\n";
	ss << userFunctionDefs << "\n";

	ss << constantsBlock;

	ss << "int main(int argc, char* argv[]) {\n";
	ss << "    Interpreter transpiled_interp;\n";
	ss << "    y_transpile_globals = transpiled_interp.env;\n";
	ss << "    transpiled_vm.globals = y_transpile_globals;\n";
	ss << "    transpiled_vm.methodResolver = [&](MethodCallExpr *m) { return transpiled_interp.Resolve_methods(m); };\n";
	ss << "    std::shared_ptr<Env> current_env = y_transpile_globals;\n\n";

	ss << userFunctionRegistrations << "\n";
	ss << output;

	if (hasUserMain) {
		// same as the VM: main() gets nothing, main(*a) a tuple, main(a, b) one string per parameter
		const auto &params = userFunctions["main"].params;
		if (params.empty()) {
			ss << "    y_user_main();\n";
		} else if (params[0].isVariadic) {
			ss << "    std::vector<Value> args_vec;\n";
			ss << "    for (int i = 0; i < argc; i++) args_vec.push_back(Value::String(argv[i]));\n";
			ss << "    y_user_main(Value::Tuple(args_vec));\n";
		} else {
			ss << "    y_user_main(";
			for (size_t i = 0; i < params.size(); i++)
				ss << (i ? ", " : "") << "(" << i << " < argc ? Value::String(argv[" << i << "]) : Value::None())";
			ss << ");\n";
		}
	}

	ss << "    return 0;\n";
	ss << "}\n";
	// drop the block EnvRestorers whose block never declared anything in current_env
	std::string code = ss.str(), out;
	out.reserve(code.size());
	for (size_t pos = 0; pos < code.size();) {
		size_t end = code.find('\n', pos);
		end = (end == std::string::npos) ? code.size() : end + 1;
		std::string line = code.substr(pos, end - pos);
		size_t m = line.find(" //@envr");
		if (m == std::string::npos)
			out += line;
		else if (envNeeded[std::stoi(line.substr(m + 8))])
			out += line.substr(0, m) + "\n";
		pos = end;
	}
	return out;
}
