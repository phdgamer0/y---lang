#pragma once
#ifndef YLANG_COMPILER_HPP
#define YLANG_COMPILER_HPP
#include "objects.hpp"
struct ByteCodeCompiler {
#define DEBUG_TRACE_EXECUTION

	// ByteCodeCompiler logic
	bool isCTLE = false;
	bool tryEvaluateCompileTimeLoop(Stmt *s);
	Chunk *chunk;
	vector<LoopContext> loopStack;
	vector<Local> locals;
	int scopeDepth = 0;
	bool isReplMode = false;
	Interpreter* interp = nullptr;
	bool isCompileOnly = false;
	std::map<string, Value> knownGlobals;

	ValueType tryExtractType(Expr *e) {
		if (!e) return ValueType::NOTYPE;
		if (e->type == ExprType::NUMBER) return static_cast<NumberExpr*>(e)->isFloat ? ValueType::FLOAT : ValueType::INT;
		if (e->type == ExprType::STRING) return ValueType::STRING;
		if (e->type == ExprType::BOOL) return ValueType::BOOL;
		if (e->type == ExprType::VAR) {
			auto v = static_cast<VarExpr *>(e);
			int arg = resolveLocal(v->name);
			if (arg != -1 && locals[arg].hasKnownType) return locals[arg].knownType;
			else if (knownGlobals.count(v->name)) return knownGlobals[v->name].type;
		}
		return ValueType::NOTYPE;
	}
	ByteCodeCompiler(Chunk *c) : chunk(c) {}
	int resolveLocal(string name) {
		for (int i = (int)locals.size() - 1; i >= 0; i--)
			if (locals[i].name == name)
				return i;
		return -1;
	}
	void addLocal(string name) { locals.push_back({name, scopeDepth}); }
	void beginScope() { scopeDepth++; }
	void endScope(int line, int col) {
		scopeDepth--;
		while (!locals.empty() && locals.back().depth > scopeDepth) {
			emitByte(OpCode::OP_POP, line, col);
			locals.pop_back();
		}
	}
	void compile(Expr *e) {
		if (!e)
			return;
		switch (e->type) {
		case ExprType::GET: {
			auto g = static_cast<GetExpr *>(e);
			compile(g->object);
			emitIdentifier(OpCode::OP_GET_PROPERTY, g->name, g->line, g->col);
			break;
		}
		case ExprType::COMPREHENSION: {
			auto comp = static_cast<CompExpr *>(e);
			beginScope();
			bool isDict = (comp->typeToken == TokenType::LBRACE &&
								comp->valueExpr != nullptr);
			bool isSet = (comp->typeToken == TokenType::LBRACE && !isDict);
			bool isTuple = (comp->typeToken == TokenType::LPAREN);
			bool isVector = (comp->typeToken == TokenType::LT);
			if (isDict) {
				emitByte(OpCode::OP_BUILD_DICT, comp->line, comp->col);
				chunk->write(0, comp->line, comp->col);
			} else if (isSet) {
				emitByte(OpCode::OP_BUILD_SET, comp->line, comp->col);
				chunk->write(0, comp->line, comp->col);
			} else {
				emitByte(OpCode::OP_BUILD_LIST, comp->line, comp->col);
				chunk->write(0, comp->line, comp->col);
			}
			int containerSlot = (int)locals.size();
			addLocal("");
			compile(comp->iterable);
			emitByte(OpCode::OP_TO_STREAM, comp->line, comp->col);
			addLocal("");
			emitConstant(Value::Int(0), comp->line, comp->col);
			addLocal("");
			int startAddr = (int)chunk->code.size();
			emitByte(OpCode::OP_FOR_ITER, comp->line, comp->col);
			int exitJump = (int)chunk->code.size();
			chunk->write(0xff, comp->line, comp->col);
			chunk->write(0xff, comp->line, comp->col);
			chunk->write(1, comp->line, comp->col);
			beginScope();
			addLocal(comp->varName);
			int filterJump = -1;
			if (comp->filter) {
				compile(comp->filter);
				filterJump =
					emitJump(OpCode::OP_JUMP_IF_FALSE, comp->line, comp->col);
				emitByte(OpCode::OP_POP, comp->line, comp->col);
			}
			if (isDict) {
				compile(comp->expression);
				compile(comp->valueExpr);
				emitByte(OpCode::OP_DICT_SET, comp->line, comp->col);
				chunk->write((uint8_t)containerSlot, comp->line, comp->col);
			} else if (isSet) {
				compile(comp->expression);
				emitByte(OpCode::OP_SET_ADD, comp->line, comp->col);
				chunk->write((uint8_t)containerSlot, comp->line, comp->col);
			} else {
				compile(comp->expression);
				emitByte(OpCode::OP_LIST_APPEND, comp->line, comp->col);
				chunk->write((uint8_t)containerSlot, comp->line, comp->col);
			}
			if (filterJump != -1) {
				int skipAppend =
					emitJump(OpCode::OP_JUMP, comp->line, comp->col);
				patchJump(filterJump);
				emitByte(OpCode::OP_POP, comp->line, comp->col);
				patchJump(skipAppend);
			}
			endScope(comp->line, comp->col);
			emitLoop(startAddr, comp->line, comp->col);
			patchJump(exitJump);
			emitByte(OpCode::OP_POP, comp->line, comp->col);
			emitByte(OpCode::OP_POP, comp->line, comp->col);
			// emitByte(OpCode::OP_GET_LOCAL, comp->line, comp->col);
			// chunk->write((uint8_t)containerSlot, comp->line, comp->col);
			if (isTuple)
				emitByte(OpCode::OP_LIST_TO_TUPLE, comp->line, comp->col);
			else if (isVector)
				emitByte(OpCode::OP_LIST_TO_VECTOR, comp->line, comp->col);
			locals.pop_back();
			locals.pop_back();
			locals.pop_back();
			scopeDepth--;
			break;
		}
		case ExprType::OMIT_MARKER_EXPR: {
			auto o = static_cast<OmitExpr *>(e);
			Value val = Value::Omit();
			emitConstant(val, o->line, o->col);
			break;
		}
		case ExprType::LAMBDA: {
			auto lam = static_cast<LambdaExpr *>(e);
			Chunk *funcChunk = new Chunk();
			ByteCodeCompiler subCompiler(funcChunk);
			subCompiler.isCompileOnly = this->isCompileOnly;
			subCompiler.interp = this->interp;
			subCompiler.beginScope();
			for (auto &param : lam->params) {
				if (param.defaultValue != nullptr) {
					param.defaultChunk = new Chunk();
					ByteCodeCompiler defCompiler(param.defaultChunk);
					defCompiler.isCompileOnly = this->isCompileOnly;
					defCompiler.interp = this->interp;
					defCompiler.compile(param.defaultValue);
					defCompiler.emitByte(OpCode::OP_RETURN, lam->line, 0);
				}
			}
			for (const auto &param : lam->params)
				subCompiler.addLocal(param.name);
			for (auto *stmt : lam->body)
				subCompiler.compileStmt(stmt);
			subCompiler.emitByte(OpCode::OP_NOTYPE, lam->line, 0);
			subCompiler.emitByte(OpCode::OP_RETURN, lam->line, 0);
			auto *funcObj = new FunctionObject(
				lam->params, lam->returnType, lam->defaultRetArgs,
				lam->returnsConst, lam->body, nullptr, lam->isCached,
				funcChunk);
			for (auto *retArg : lam->defaultRetArgs) {
				Chunk *retChunk = new Chunk();
				ByteCodeCompiler retCompiler(retChunk);
				retCompiler.isCompileOnly = this->isCompileOnly;
				retCompiler.interp = this->interp;
				retCompiler.compile(retArg);
				retCompiler.emitByte(OpCode::OP_RETURN, lam->line, 0);
				funcObj->defaultRetChunks.push_back(retChunk);
			}
			Value funcVal;
			funcVal.type = ValueType::FUNCTION;
			funcVal.ref = std::shared_ptr<HeapObject>(funcObj);
			emitConstant(funcVal, lam->line, 0);
			break;
		}
		case ExprType::NUMBER: {
			auto n = static_cast<NumberExpr *>(e);
			Value val;
			if (n->isFloat) {
				val = Value::Float(n->val);
			} else {
				if (n->raw.empty()) {
					val = Value::Int((long long)n->val);
				} else if (n->raw.length() >= 19) {
					val = Value::BigInt(n->raw);
				} else {
					try {
						val = Value::Int(std::stoll(n->raw));
					} catch (...) {
						val = Value::BigInt(n->raw);
					}
				}
			}
			emitConstant(val, n->line, n->col);
			break;
		}
		case ExprType::BOOL: {
			auto b = static_cast<BoolExpr *>(e);
			emitByte(b->value ? OpCode::OP_TRUE : OpCode::OP_FALSE, b->line,
				b->col);
			break;
		}
		case ExprType::STRING: {
			auto s = static_cast<StringExpr *>(e);
			emitConstant(Value::String(s->val), s->line, s->col);
			break;
		}
		case ExprType::FSTRING: {
			auto fs = static_cast<FStringExpr *>(e);
			for (auto *part : fs->parts)
				compile(part);
			if (fs->parts.size() > 255)
				throw SyntaxError("F-String has too many parts (limit 255)",
					fs->line, fs->col);
			emitByte(OpCode::OP_BUILD_FSTRING, fs->line, fs->col);
			chunk->write((uint8_t)fs->parts.size(), fs->line, fs->col);
			break;
		}
		case ExprType::BINARY: {
			auto b = static_cast<BinExpr *>(e);
			if (b->op == TokenType::AND || b->op == TokenType::OR || b->op == TokenType::NAND || b->op == TokenType::NOR)
				compileLogical(b);
			else
				compileBinary(b);
			break;
		}
		case ExprType::VAR: {
			auto v = static_cast<VarExpr *>(e);
			if (v->name == "None") {
				emitByte(OpCode::OP_NONE, v->line, v->col);
				break;
			}
			int arg = resolveLocal(v->name);
			if (arg != -1) {

				emitByte(OpCode::OP_GET_LOCAL, v->line, v->col);
				chunk->write((uint8_t)arg, v->line, v->col);
			} else {
				if (isCompileOnly && interp) {
					if (!interp->env->exists(v->name) && knownGlobals.find(v->name) == knownGlobals.end()) {
						throw NameError("Undefined variable '" + v->name + "'", v->line, v->col);
					}
				}
				emitIdentifier(OpCode::OP_GET_VAR, v->name, v->line, v->col);
			}
			break;
		}
		case ExprType::OWNERSHIP: {
			auto o = static_cast<OwnershipExpr *>(e);
			if (o->mode == CopyMode::DEEP) {
				compile(o->expr);
				emitByte(OpCode::OP_DEEP_COPY, o->line, o->col);
			} else if (o->mode == CopyMode::REF) {
				if (auto v = dynamic_cast<VarExpr *>(o->expr)) {
					int local = resolveLocal(v->name);
					if (local != -1) {
						emitByte(OpCode::OP_REF_LOCAL, o->line,
							o->col); // New OpCode
						chunk->write((uint8_t)local, o->line, o->col);
					} else {
						emitIdentifier(OpCode::OP_REF_VAR, v->name, o->line,
							o->col); // New usage
					}
				} else if (auto idx = dynamic_cast<IndexExpr *>(o->expr)) {
					compile(idx->base);
					compile(idx->index);
					emitByte(OpCode::OP_REF_INDEX, o->line, o->col);
				} else if (auto get = dynamic_cast<GetExpr *>(o->expr)) {
					compile(get->object);
					emitIdentifier(OpCode::OP_REF_PROPERTY, get->name, o->line,
						o->col);
				} else {
					throw OwnershipError(
						"Cannot take reference of this expression", o->line,
						o->col);
				}
			}
			break;
		}
		case ExprType::INDEX: {
			auto idx = static_cast<IndexExpr *>(e);
			compile(idx->base);
			compile(idx->index);
			emitByte(OpCode::OP_GET_INDEX, idx->line, idx->col);
			break;
		}
		case ExprType::SLICE: {
			auto s = static_cast<SliceExpr *>(e);
			if (s->start)
				compile(s->start);
			else
				emitByte(OpCode::OP_NONE, s->line, s->col);
			if (s->end)
				compile(s->end);
			else
				emitByte(OpCode::OP_NONE, s->line, s->col);
			if (s->step)
				compile(s->step);
			else
				emitByte(OpCode::OP_NONE, s->line, s->col);
			emitByte(OpCode::OP_BUILD_SLICE, s->line, s->col);
			break;
		}
		case ExprType::CALL: {
			Value foldedResult;
			if (tryExtractConstant(e, foldedResult)) {
				emitConstant(foldedResult, e->line, e->col);
				break;
			}
			auto c = static_cast<CallExpr *>(e);
			if (isCTLE) throw CTLEAbort(); // No function calls allowed in compile-time loops yet!
			if (c->name == "super") {
				if (!c->args.empty())
					throw ArgumentError("super() takes no arguments", c->line,
						c->col);
				emitByte(OpCode::OP_SUPER, c->line, c->col);
				break;
			}
			// for (auto arg : c->args) compile(arg);
			for (size_t i = 0; i < c->args.size(); i++) {
				Expr *arg = c->args[i];
				CopyMode mode =
					(i < c->modes.size()) ? c->modes[i] : CopyMode::SHALLOW;

				if (mode == CopyMode::DEEP) {
					compile(arg);
					emitByte(OpCode::OP_DEEP_COPY, c->line, c->col);
				} else if (mode == CopyMode::REF) {
					if (auto v = dynamic_cast<VarExpr *>(arg)) {
						int local = resolveLocal(v->name);
						if (local != -1) {
							emitByte(OpCode::OP_REF_LOCAL, c->line, c->col);
							chunk->write((uint8_t)local, c->line, c->col);
						} else {
							emitIdentifier(OpCode::OP_REF_VAR, v->name, c->line,
								c->col);
						}
					} else if (auto idx = dynamic_cast<IndexExpr *>(arg)) {
						compile(idx->base);
						compile(idx->index);
						emitByte(OpCode::OP_REF_INDEX, c->line, c->col);
					} else if (auto get = dynamic_cast<GetExpr *>(arg)) {
						compile(get->object);
						emitIdentifier(OpCode::OP_REF_PROPERTY, get->name, c->line,
							c->col);
					} else {
						throw OwnershipError(
							"Cannot take reference of this expression in call",
							c->line, c->col);
					}
				} else {
					compile(arg);
				}
			}
			int arg = resolveLocal(c->name);
			if (arg != -1) {
				emitByte(OpCode::OP_GET_LOCAL, c->line, c->col);
				chunk->write((uint8_t)arg, c->line, c->col);
			} else
				emitIdentifier(OpCode::OP_GET_VAR, c->name, c->line, c->col);
			emitByte(OpCode::OP_CALL, c->line, c->col);
			chunk->write(static_cast<uint8_t>(c->args.size()), c->line, c->col);
			break;
		}
		case ExprType::TERNARY: {
			auto t = static_cast<TernaryExpr *>(e);
			compile(t->condition);
			int elseJump = emitJump(OpCode::OP_JUMP_IF_FALSE, t->line, t->col);
			emitByte(OpCode::OP_POP, t->line, t->col);
			compile(t->trueBranch);
			int endJump = emitJump(OpCode::OP_JUMP, t->line, t->col);
			patchJump(elseJump);
			emitByte(OpCode::OP_POP, t->line, t->col);
			compile(t->falseBranch);
			patchJump(endJump);
			break;
		}
		case ExprType::LIST: {
			auto l = static_cast<ListExpr *>(e);
			for (auto item : l->elements)
				compile(item);
			emitByte(OpCode::OP_BUILD_LIST, l->line, l->col);
			chunk->write((uint8_t)l->elements.size(), l->line, l->col);
			break;
		}
		case ExprType::SET: {
			auto s = static_cast<SetExpr *>(e);
			for (auto item : s->elements)
				compile(item);
			emitByte(OpCode::OP_BUILD_SET, s->line, s->col);
			chunk->write((uint8_t)s->elements.size(), s->line, s->col);
			break;
		}
		case ExprType::TUPLE: {
			auto t = static_cast<TupleExpr *>(e);
			for (auto item : t->elements)
				compile(item);
			emitByte(OpCode::OP_BUILD_TUPLE, t->line, t->col);
			chunk->write((uint8_t)t->elements.size(), t->line, t->col);
			break;
		}
		case ExprType::DICT: {
			auto d = static_cast<DictExpr *>(e);
			for (auto &item : d->items) {
				compile(item.first);
				compile(item.second);
				emitByte(OpCode::OP_COLON, d->line, d->col);
			}
			emitByte(OpCode::OP_BUILD_DICT, d->line, d->col);
			chunk->write((uint8_t)d->items.size(), d->line, d->col);
			break;
		}
		case ExprType::RANGE: {
			auto r = static_cast<RangeExpr *>(e);
			compile(r->start);
			compile(r->end);
			if (r->step)
				compile(r->step);
			else
				emitConstant(Value::Int(1), r->line, r->col);
			emitByte(OpCode::OP_BUILD_RANGE, r->line, r->col);
			uint8_t flags = 0;
			if (r->startInclusive)
				flags |= 0x01;
			if (r->endInclusive)
				flags |= 0x02;
			chunk->write(flags, r->line, r->col);
			break;
		}
		case ExprType::INC_DEC: {
			auto incdec = static_cast<IncDecExpr *>(e);
			if (auto v = dynamic_cast<VarExpr*>(incdec->expr)) {
				int arg = resolveLocal(v->name);
				if (arg != -1) {
					emitByte(OpCode::OP_GET_LOCAL, e->line, e->col);
					chunk->write((uint8_t)arg, e->line, e->col);
				} else {
					emitIdentifier(OpCode::OP_GET_VAR, v->name, e->line, e->col);
				}
				if (!incdec->isPrefix) emitByte(OpCode::OP_DUP, e->line, e->col);
				emitConstant(Value::Int(1), e->line, e->col);
				emitByte(incdec->isIncrement ? OpCode::OP_ADD : OpCode::OP_SUB, e->line, e->col);
				if (arg != -1) {
					emitByte(OpCode::OP_SET_LOCAL, e->line, e->col);
					chunk->write((uint8_t)arg, e->line, e->col);
				} else {
					emitIdentifier(OpCode::OP_SET_VAR, v->name, e->line, e->col);
				}
				if (!incdec->isPrefix) emitByte(OpCode::OP_POP, e->line, e->col);
			} else if (auto get = dynamic_cast<GetExpr*>(incdec->expr)) {
				compile(get->object);
				emitIdentifier(OpCode::OP_INC_PROPERTY, get->name, e->line, e->col);
				chunk->write(incdec->isIncrement ? 1 : 0, e->line, e->col);
				chunk->write(incdec->isPrefix ? 1 : 0, e->line, e->col);
			} else if (auto idx = dynamic_cast<IndexExpr*>(incdec->expr)) {
				compile(idx->base);
				compile(idx->index);
				emitByte(OpCode::OP_INC_INDEX, e->line, e->col);
				chunk->write(incdec->isIncrement ? 1 : 0, e->line, e->col);
				chunk->write(incdec->isPrefix ? 1 : 0, e->line, e->col);
			} else {
				throw SyntaxError("Increment/Decrement only supported on assignable expressions", e->line, e->col);
			}
			break;
		}
		case ExprType::VECTOR: {
			auto ve = static_cast<VectorExpr *>(e);
			for (auto *el : ve->elements)
				compile(el);
			emitByte(OpCode::OP_BUILD_VECTOR, ve->line, ve->col);
			chunk->write((uint8_t)ve->elements.size(), ve->line, ve->col);
			break;
		}
		case ExprType::METHOD_CALL: {
			auto m = static_cast<MethodCallExpr *>(e);
			compile(m->object);
			for (auto arg : m->args) {
				compile(arg);
			}
			emitByte(OpCode::OP_INVOKE, m->line, m->col);
			int nameIdx = chunk->addConstant(Value::String(m->method));
			chunk->write((uint8_t)nameIdx, m->line, m->col);
			chunk->write((uint8_t)m->args.size(), m->line, m->col);
			break;
		}
		// will add more cases
		default:
			break;
		}
	}
	void compileBinary(BinExpr *b) {
		if (b->op == TokenType::NOT) {
			compile(b->right);
			emitByte(OpCode::OP_NOT, b->line, b->col);
			return; // Exit early!
		}
		if (b->op == TokenType::BITWISE_NOT) {
			compile(b->right);
			emitByte(OpCode::OP_BITWISE_NOT, b->line, b->col);
			return;
		}
		// constant folding (experimental):
		Value foldedResult;
		if (tryExtractConstant(b, foldedResult)) {
			emitConstant(foldedResult, b->line, b->col);
			return;
		}
		ValueType lt = tryExtractType(b->left);
		ValueType rt = tryExtractType(b->right);
		bool isFastInt = (lt == ValueType::INT && rt == ValueType::INT);
		bool isFastFloat = (lt == ValueType::FLOAT && rt == ValueType::FLOAT);
		bool isFastStr = (lt == ValueType::STRING && rt == ValueType::STRING);
		compile(b->left);
		compile(b->right);
		switch (b->op) {
		case TokenType::PLUS:
			emitByte(isFastInt ? OpCode::OP_FAST_IADD : (isFastFloat ? OpCode::OP_FAST_FADD : (isFastStr ? OpCode::OP_FAST_SADD : OpCode::OP_ADD)), b->line, b->col);
			break;
		case TokenType::MINUS:
			emitByte(isFastInt ? OpCode::OP_FAST_ISUB : (isFastFloat ? OpCode::OP_FAST_FSUB : OpCode::OP_SUB), b->line, b->col);
			break;
		case TokenType::STAR:
			emitByte(isFastInt ? OpCode::OP_FAST_IMUL : (isFastFloat ? OpCode::OP_FAST_FMUL : OpCode::OP_MUL), b->line, b->col);
			break;
		case TokenType::SLASH:
			emitByte(isFastInt ? OpCode::OP_FAST_IDIV : (isFastFloat ? OpCode::OP_FAST_FDIV : OpCode::OP_DIV), b->line, b->col);
			break;
		case TokenType::FLOOR_DIV:
			emitByte(OpCode::OP_FLOOR_DIV, b->line, b->col);
			break;
		case TokenType::MOD:
			emitByte(OpCode::OP_MOD, b->line, b->col);
			break;
		case TokenType::POW:
			emitByte(OpCode::OP_POW, b->line, b->col);
			break;
		case TokenType::BITWISE_AND:
			emitByte(OpCode::OP_BITWISE_AND, b->line, b->col);
			break;
		case TokenType::BITWISE_OR:
			emitByte(OpCode::OP_BITWISE_OR, b->line, b->col);
			break;
		case TokenType::BITWISE_XOR:
			emitByte(OpCode::OP_BITWISE_XOR, b->line, b->col);
			break;
		case TokenType::BITWISE_SHL:
			emitByte(OpCode::OP_BITWISE_SHL, b->line, b->col);
			break;
		case TokenType::BITWISE_SHR:
			emitByte(OpCode::OP_BITWISE_SHR, b->line, b->col);
			break;
			// Logic
		case TokenType::GT:
			emitByte(OpCode::OP_GT, b->line, b->col);
			break;
		case TokenType::GTE:
			emitByte(OpCode::OP_GTE, b->line, b->col);
			break;
		case TokenType::EQ:
			emitByte(OpCode::OP_EQ, b->line, b->col);
			break;
		case TokenType::NEQ:
			emitByte(OpCode::OP_NEQ, b->line, b->col);
			break;
		case TokenType::STRICT_EQ:
			emitByte(OpCode::OP_STRICT_EQ, b->line, b->col);
			break;
		case TokenType::STRICT_NEQ:
			emitByte(OpCode::OP_STRICT_NEQ, b->line, b->col);
			break;
		case TokenType::LT:
			emitByte(OpCode::OP_LT, b->line, b->col);
			break;
		case TokenType::LTE:
			emitByte(OpCode::OP_LTE, b->line, b->col);
			break;
			// Identity & Membership
		case TokenType::IS:
			emitByte(OpCode::OP_IS, b->line, b->col);
			break;
		case TokenType::IS_NOT:
			emitByte(OpCode::OP_IS_NOT, b->line, b->col);
			break;
		case TokenType::IS_IN:
			emitByte(OpCode::OP_IS_IN, b->line, b->col);
			break;
		case TokenType::IS_NOT_IN:
			emitByte(OpCode::OP_IS_NOT_IN, b->line, b->col);
			break;
			// Logical (non-short-circuiting)
		case TokenType::XOR:
			emitByte(OpCode::OP_XOR, b->line, b->col);
			break;
		case TokenType::NXOR:
			emitByte(OpCode::OP_NXOR, b->line, b->col);
			break;
			//"pairing" op
		case TokenType::COLON:
			emitByte(OpCode::OP_COLON, b->line, b->col);
			break;
		default:
			break;
		}
	}
	void compileLogical(BinExpr *l) {
		if (l->op == TokenType::AND) {
			compile(l->left);
			int endJump = emitJump(OpCode::OP_JUMP_IF_FALSE, l->line, l->col);
			emitByte(OpCode::OP_POP, l->line, l->col); // Pop left if it was true
			compile(l->right);
			patchJump(endJump);
		} else if (l->op == TokenType::OR) {
			compile(l->left);
			int elseJump = emitJump(OpCode::OP_JUMP_IF_FALSE, l->line, l->col);
			int endJump = emitJump(OpCode::OP_JUMP, l->line, l->col);
			patchJump(elseJump);
			emitByte(OpCode::OP_POP, l->line, l->col); // Pop left if it was false
			compile(l->right);
			patchJump(endJump);
		} else if (l->op == TokenType::NAND) {
			compile(l->left);
			int shortCircuitJump = emitJump(OpCode::OP_JUMP_IF_FALSE, l->line, l->col);
			emitByte(OpCode::OP_POP, l->line, l->col);
			compile(l->right);
			emitByte(OpCode::OP_NOT, l->line, l->col);
			int endJump = emitJump(OpCode::OP_JUMP, l->line, l->col);
			patchJump(shortCircuitJump);
			emitByte(OpCode::OP_POP, l->line, l->col);
			emitByte(OpCode::OP_TRUE, l->line, l->col);
			patchJump(endJump);
		} else if (l->op == TokenType::NOR) {
			compile(l->left);
			int evalRightJump = emitJump(OpCode::OP_JUMP_IF_FALSE, l->line, l->col);
			emitByte(OpCode::OP_POP, l->line, l->col);
			emitByte(OpCode::OP_FALSE, l->line, l->col);
			int endJump = emitJump(OpCode::OP_JUMP, l->line, l->col);
			patchJump(evalRightJump);
			emitByte(OpCode::OP_POP, l->line, l->col);
			compile(l->right);
			emitByte(OpCode::OP_NOT, l->line, l->col);

			patchJump(endJump);
		}
	}
	void emitByte(OpCode op, int line, int col) { chunk->write(op, line, col); }
	void emitConstant(Value v, int line, int col) {
		int index = chunk->addConstant(v);
		if (index > 255) {
			if (index > 65535)
				throw MemoryError("Too many constants in one chunk (limit 65535)",
					line, col);
			emitByte(OpCode::OP_CONSTANT_LONG, line, col);
			chunk->write((index >> 8) & 0xff, line, col);
			chunk->write(index & 0xff, line, col);
		} else {
			emitByte(OpCode::OP_CONSTANT, line, col);
			chunk->write(static_cast<uint8_t>(index), line, col);
		}
	}
	int emitJump(OpCode instruction, int line, int col) {
		emitByte(instruction, line, col);
		chunk->write(0xff, line, col);
		chunk->write(0xff, line, col);
		return (int)chunk->code.size() - 2;
	}
	void patchJump(int offset) {
		int jump = (int)chunk->code.size() - offset - 2;
		if (jump > 65535)
			throw RangeError("Too much code to jump over!", 0, 0);
		chunk->code[offset] = (jump >> 8) & 0xff;
		chunk->code[offset + 1] = jump & 0xff;
	}
	void compileWithMode(Expr *expr, int line, int col) {
		if (auto o = dynamic_cast<OwnershipExpr *>(expr)) {
			if (o->mode == CopyMode::DEEP) {
				compile(o->expr);
				emitByte(OpCode::OP_DEEP_COPY, line, col);
			} else if (o->mode == CopyMode::REF)
				compile(expr);
		} else {
			compile(expr);
			if (expr->type != ExprType::LIST && expr->type != ExprType::DICT &&
				 expr->type != ExprType::SET && expr->type != ExprType::VECTOR &&
				 expr->type != ExprType::LAMBDA && expr->type != ExprType::NUMBER &&
				 expr->type != ExprType::STRING && expr->type != ExprType::BOOL)
				emitByte(OpCode::OP_SHALLOW_COPY, line, col);
		}
	}
	void compileStmt(Stmt *s);
	void emitIdentifier(OpCode op, const string &name, int line, int col) {
		int index = chunk->addConstant(Value::String(name));
		emitByte(op, line, col);
		chunk->write(static_cast<uint8_t>(index), line, col);
	}
	void emitLoop(int loopStart, int line, int col) {
		emitByte(OpCode::OP_LOOP, line, col);
		int offset = (int)chunk->code.size() - loopStart + 2;
		if (offset > 65535)
			throw RangeError("Loop body too large", line, col);
		chunk->write((offset >> 8) & 0xff, line, col);
		chunk->write(offset & 0xff, line, col);
	}
	void emitFunction(FuncStmt *f, bool isMethod = false, const vector<Stmt *> &fieldInits = {}) {
		Chunk *funcChunk = new Chunk();
		ByteCodeCompiler subCompiler(funcChunk);
		subCompiler.isCompileOnly = this->isCompileOnly;
		subCompiler.interp = this->interp;
		subCompiler.beginScope();
		vector<ParamSpec> actualParams;
		if (isMethod) {
			actualParams.push_back({"self", CopyMode::SHALLOW, ValueType::NOTYPE,
				nullptr, nullptr, false, false, false});
			actualParams.push_back({"obj", CopyMode::SHALLOW, ValueType::NOTYPE,
				nullptr, nullptr, false, false, false});
		}
		for (const auto &param : f->params)
			actualParams.push_back(param);
		for (auto &param : actualParams) {
			if (param.defaultValue != nullptr) {
				param.defaultChunk = new Chunk();
				ByteCodeCompiler subCompiler(param.defaultChunk);
				subCompiler.isCompileOnly = this->isCompileOnly;
				subCompiler.interp = this->interp;
				subCompiler.compile(param.defaultValue);
				subCompiler.emitByte(OpCode::OP_RETURN, f->line, f->col);
			}
		}
		for (const auto &param : actualParams)
			subCompiler.addLocal(param.name);
		for (auto *init : fieldInits)
			subCompiler.compileStmt(init);
		for (auto bodyStmt : f->body)
			subCompiler.compileStmt(bodyStmt);
		subCompiler.emitByte(OpCode::OP_NOTYPE, f->line, f->col);
		subCompiler.emitByte(OpCode::OP_RETURN, f->line, f->col);
		auto *funcObj = new FunctionObject(
			actualParams, f->returnType, f->defaultRetArgs, f->returnsConst,
			f->body, nullptr, f->isCached, funcChunk);
		for (auto *retArg : f->defaultRetArgs) {
			Chunk *retChunk = new Chunk();
			ByteCodeCompiler subCompiler(retChunk);
			subCompiler.isCompileOnly = this->isCompileOnly;
			subCompiler.interp = this->interp;
			subCompiler.compile(retArg);
			subCompiler.emitByte(OpCode::OP_RETURN, f->line, f->col);
			funcObj->defaultRetChunks.push_back(retChunk);
		}
		funcObj->name = f->name;
		Value funcVal;
		funcVal.type = ValueType::FUNCTION;
#ifdef VM_DEBUG_MODE
		if (DEBUGGER_MODE_IS_ENABLED)
			funcVal.__DEBUGGING__NAME__ = funcObj->name;
#endif
		funcVal.ref = std::shared_ptr<HeapObject>(funcObj);
		emitConstant(funcVal, f->line, f->col);
		if (!isMethod) knownGlobals[f->name] = funcVal;
	}
	bool tryExtractConstant(Expr *e, Value &outVal);
};
// 1. Detect the Compiler
#if defined(__GNUC__) || defined(__clang__)
#define USE_COMPUTED_GOTOS
#endif
#ifdef VM_DEBUG_MODE
#define RUN_DEBUGGER()                                                                  \
	do {                                                                                 \
		Chunk *currentChunk = frame->function ? frame->function->chunk : &chunk;          \
		int currentOffset = (int)(ip - currentChunk->code.data());                        \
		std::cout << "LINE: " << std::left << std::setw(4) << line << " | ";              \
		std::cout << std::left << std::setw(18) << OpCodeToString((OpCode) * ip) << " | " \
					 << PrintStackForDebug(stack) << "\n";                                   \
		std::cout << "Stack Size: " << stack.size() << "\n";                              \
	} while (false)
#else
#define RUN_DEBUGGER() \
	do {                \
	} while (false)
#endif
// 2. Define the cross-platform OpCode label
#ifdef USE_COMPUTED_GOTOS
#define OP(name) TARGET_##name
#else
#define OP(name) case OpCode::name
#endif

// 3. Define the jump mechanism!
#ifdef USE_COMPUTED_GOTOS
#define DISPATCH()                                                        \
	do {                                                                  \
		if (gc_alloc_count > 50000) { gc_collect(); gc_alloc_count = 0; } \
		RUN_DEBUGGER();                                                   \
		currentChunk = frame->function ? frame->function->chunk : &chunk; \
		instruction = static_cast<OpCode>(*ip++);                         \
		int offset = (int)(ip - currentChunk->code.data());               \
		line = currentChunk->lines[offset - 1];                           \
		col = currentChunk->columns[offset - 1];                          \
		goto *dispatch_table[static_cast<size_t>(instruction)];           \
	} while (false)
#else
#define DISPATCH() goto loop_start
#endif
// Helper: Attempts to synchronously execute a native dunder method.
// Returns a pair: <bool found, Value result>
static inline std::pair<bool, Value> tryCastDunder(Value v, const std::string &dunderName, int l, int c, std::shared_ptr<Env> globals, std::function<Value(MethodCallExpr *)> methodResolver, std::function<void(std::string, std::vector<std::string>, std::shared_ptr<Env>)> importResolver);

#endif // YLANG_COMPILER_HPP
