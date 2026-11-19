#include "ylang/vm.hpp"
#include "ylang/objects.hpp"

Interpreter::Interpreter() {
		enableColors();
		env = std::make_shared<Env>();
		env->set("None", Value::None(), true);
		registerStdLib();
	}

LValue Interpreter::resolveLValue(Expr *e) {
		if (ValueExpr *ve = dynamic_cast<ValueExpr *>(e)) {
			if (ve->sourcePtr)
				return LValue(ve->sourcePtr, true);
			return LValue(&ve->val, false);
		}
		if (auto v = dynamic_cast<VarExpr *>(e)) {
			if (!env->exists(v->name))
				throw NameError("Undefined variable '" + v->name + "'", e->line,
					e->col);
			Var &var = env->lookup(v->name);
			if (var.alias)
				return LValue(var.alias, true, var.isConst, var.isLocked);
			return LValue(&var.value, false, var.isConst, var.isLocked);
		}
		if (auto idx = dynamic_cast<IndexExpr *>(e)) {
			LValue base = resolveLValue(idx->base);
			Value &container = *base.ref;
			int i = eval(idx->index).asInt();
			if (container.type == ValueType::LIST) {
				auto *listObj = static_cast<ListObject *>(container.ref.get());
				if (i < 0 || i > (int)listObj->elements.size())
					throw IndexError("List index out of range", e->line, e->col);
				if (i == (int)listObj->elements.size())
					listObj->elements.push_back(Value::None());

				return LValue(&listObj->elements[i], true,
					base.isConstView || container.isConst);
			}
			if (container.type == ValueType::STRING)
				return LValue(&container, true);

			throw TypeError("Type is not indexable or mutable", e->line, e->col);
		}
		throw TypeError("Expression is not assignable", e->line, e->col);
	}

bool Interpreter::matchesError(const LangError &e, const string &typeName) {
		// --- ROOT ---
		if (typeName == "Error")
			return true;
		// --- LEVEL 1: BASE CATEGORIES ---
		if (typeName == "InternalError")
			return dynamic_cast<const InternalError *>(&e);
		if (typeName == "ControlFlowError")
			return dynamic_cast<const ControlFlowError *>(&e);
		if (typeName == "ParseError")
			return dynamic_cast<const ParseError *>(&e);
		if (typeName == "RuntimeError")
			return dynamic_cast<const RuntimeError *>(&e);
		if (typeName == "Warning")
			return dynamic_cast<const Warning *>(&e);
		// --- LEVEL 2: RUNTIME ERROR SUBTREE ---
		if (typeName == "NameError")
			return dynamic_cast<const NameError *>(&e);
		if (typeName == "AttributeError")
			return dynamic_cast<const AttributeError *>(&e);
		if (typeName == "TypeError")
			return dynamic_cast<const TypeError *>(&e);
		if (typeName == "ArgumentError")
			return dynamic_cast<const ArgumentError *>(&e);
		if (typeName == "ValueError")
			return dynamic_cast<const ValueError *>(&e);
		if (typeName == "ConstError")
			return dynamic_cast<const ConstError *>(&e);
		if (typeName == "OwnershipError")
			return dynamic_cast<const OwnershipError *>(&e);
		if (typeName == "IndexError")
			return dynamic_cast<const IndexError *>(&e);
		if (typeName == "KeyError")
			return dynamic_cast<const KeyError *>(&e);
		if (typeName == "RangeError")
			return dynamic_cast<const RangeError *>(&e);
		if (typeName == "AssertionError")
			return dynamic_cast<const AssertionError *>(&e);
		if (typeName == "RecursionError")
			return dynamic_cast<const RecursionError *>(&e);
		if (typeName == "ImportError")
			return dynamic_cast<const ImportError *>(&e);
		if (typeName == "IOError")
			return dynamic_cast<const IOError *>(&e);
		if (typeName == "MathError")
			return dynamic_cast<const MathError *>(&e);
		if (typeName == "CastError")
			return dynamic_cast<const CastError *>(&e);
		if (typeName == "IteratorError")
			return dynamic_cast<const IteratorError *>(&e);
		if (typeName == "TimeoutError")
			return dynamic_cast<const TimeoutError *>(&e);
		if (typeName == "MemoryError")
			return dynamic_cast<const MemoryError *>(&e);
		if (typeName == "SystemError")
			return dynamic_cast<const SystemError *>(&e);
		// Children of ValueError
		if (typeName == "EmptyContainerError")
			return dynamic_cast<const EmptyContainerError *>(&e);
		// Children of ConstError
		if (typeName == "MutationError")
			return dynamic_cast<const MutationError *>(&e);
		// Children of ImportError
		if (typeName == "ModuleNotFoundError")
			return dynamic_cast<const ModuleNotFoundError *>(&e);
		if (typeName == "CircularImportError")
			return dynamic_cast<const CircularImportError *>(&e);
		if (typeName == "InvalidImportError")
			return dynamic_cast<const InvalidImportError *>(&e);
		// Children of IOError
		if (typeName == "FileNotFoundError")
			return dynamic_cast<const FileNotFoundError *>(&e);
		if (typeName == "PermissionError")
			return dynamic_cast<const PermissionError *>(&e);
		if (typeName == "EOFError")
			return dynamic_cast<const EOFError *>(&e);
		if (typeName == "FileClosedError")
			return dynamic_cast<const FileClosedError *>(&e);
		// Children of MathError
		if (typeName == "DivisionByZeroError")
			return dynamic_cast<const DivisionByZeroError *>(&e);
		if (typeName == "OverflowError")
			return dynamic_cast<const OverflowError *>(&e);
		if (typeName == "UnderflowError")
			return dynamic_cast<const UnderflowError *>(&e);
		if (typeName == "DomainError")
			return dynamic_cast<const DomainError *>(&e);
		// Children of SystemError
		if (typeName == "OSError")
			return dynamic_cast<const OSError *>(&e);
		if (typeName == "EnvironmentError")
			return dynamic_cast<const EnvironmentError *>(&e);
		if (typeName == "SignalError")
			return dynamic_cast<const SignalError *>(&e);
		// Children of ControlFlowError
		if (typeName == "ReturnSignal")
			return dynamic_cast<const ReturnSignal *>(&e);
		if (typeName == "BreakSignal")
			return dynamic_cast<const BreakSignal *>(&e);
		if (typeName == "ContinueSignal")
			return dynamic_cast<const ContinueSignal *>(&e);
		// Children of ParseError
		if (typeName == "SyntaxError")
			return dynamic_cast<const SyntaxError *>(&e);
		if (typeName == "IndentationError")
			return dynamic_cast<const IndentationError *>(&e);
		if (typeName == "UnexpectedTokenError")
			return dynamic_cast<const UnexpectedTokenError *>(&e);
		if (typeName == "UnterminatedLiteralError")
			return dynamic_cast<const UnterminatedLiteralError *>(&e);
		// Children of Warning
		if (typeName == "DeprecationWarning")
			return dynamic_cast<const DeprecationWarning *>(&e);
		if (typeName == "RuntimeWarning")
			return dynamic_cast<const RuntimeWarning *>(&e);
		if (typeName == "ImportWarning")
			return dynamic_cast<const ImportWarning *>(&e);
		// Fallback: Direct String Match (For exact matches or user-defined types)
		return e.type == typeName;
	}

Value Interpreter::eval(Expr *e) {
		if (!e)
			return Value::None();
		if (ValueExpr *ve = dynamic_cast<ValueExpr *>(e))
			return ve->val;
		switch (e->type) {
		case ExprType::FSTRING: {
			auto fs = static_cast<FStringExpr *>(e);
			string res = "";
			for (auto *part : fs->parts) {
				Value v = eval(part);
				if (v.type == ValueType::STRING)
					res += v.asString();
				else
					res += valueToString(v);
			}
			return Value::String(res);
		}
		case ExprType::OWNERSHIP: {
			auto o = static_cast<OwnershipExpr *>(e);
			if (o->mode == CopyMode::REF) {
				LValue lv = resolveLValue(o->expr);
				return *lv.ref;
			}
			Value v = eval(o->expr);
			return applyCopy(v, o->mode);
		}
		case ExprType::BOOL: {
			return Value::Bool(static_cast<BoolExpr *>(e)->value);
		}
		case ExprType::NUMBER: {
			auto n = static_cast<NumberExpr *>(e);
			return n->isFloat ? Value::Float(n->val)
									: Value::Int((long long)n->val);
		}
		case ExprType::STRING: {
			return Value::String(static_cast<StringExpr *>(e)->val);
		}
		case ExprType::VAR: {
			auto *v = static_cast<VarExpr *>(e);
			if (v->cachedGlobal)
				return *v->cachedGlobal;
			if (!env->exists(v->name))
				throw NameError("Undefined variable '" + v->name + "'", e->line,
					e->col);
			Var &var = env->lookup(v->name);
			if (env->parent == nullptr)
				v->cachedGlobal = &var.value;
			if (var.alias)
				return *var.alias;
			return var.value;
		}
		case ExprType::LIST: {
			auto l = static_cast<ListExpr *>(e);
			vector<Value> vals;
			for (auto el : l->elements)
				vals.push_back(eval(el));
			return Value::List(vals);
		}
		case ExprType::BINARY: {
			auto b = static_cast<BinExpr *>(e);
			if (b->left && b->right && b->left->type == ExprType::NUMBER &&
				 b->right->type == ExprType::NUMBER) {
				double l = static_cast<NumberExpr *>(b->left)->val;
				double r = static_cast<NumberExpr *>(b->right)->val;
				switch (b->op) {
				case TokenType::PLUS:
					return Value::Float(l + r);
				case TokenType::MINUS:
					return Value::Float(l - r);
				case TokenType::STAR:
					return Value::Float(l * r);
				case TokenType::SLASH:
					return r != 0 ? Value::Float(l / r) : Value::None();
				case TokenType::POW:
					return Value::Float(pow(l, r));
				}
			}
			if (b->op == TokenType::AND) {
				Value l = eval(b->left);
				if (!l.isTruthy())
					return Value::Bool(false);
				return Value::Bool(eval(b->right).isTruthy());
			}
			if (b->op == TokenType::OR) {
				Value l = eval(b->left);
				if (l.isTruthy())
					return Value::Bool(true);
				return Value::Bool(eval(b->right).isTruthy());
			}
			Value l = b->left ? eval(b->left) : Value::None();
			Value r = eval(b->right);
			switch (b->op) {
			case TokenType::PLUS:
				if (l.type == ValueType::STRING ||
					 r.type == ValueType::STRING)
					return Value::String(
						(l.type == ValueType::STRING
								? l.asString()
								: (l.type == ValueType::FLOAT
										  ? std::to_string(l.asFloat())
										  : std::to_string(l.asInt()))) +
						(r.type == ValueType::STRING
								? r.asString()
								: (r.type == ValueType::FLOAT
										  ? std::to_string(r.asFloat())
										  : std::to_string(r.asInt()))));
				if (l.type == ValueType::INT && r.type == ValueType::INT) {
					long long a = l.asInt();
					long long b = r.asInt();
					if ((b > 0 && a > LLONG_MAX - b) ||
						 (b < 0 && a < LLONG_MIN - b))
						return Value::Float((double)a + (double)b);
					return Value::Int(a + b);
				}
				if (l.isNumber() && r.isNumber()) {
					return Value::Float(l.asFloat() + r.asFloat());
				}
				if (l.type == ValueType::VECTOR &&
					 r.type == ValueType::VECTOR) {
					auto *v1 = static_cast<VectorObject *>(l.ref.get());
					auto *v2 = static_cast<VectorObject *>(r.ref.get());
					if (v1->elements.size() != v2->elements.size())
						throw ValueError(
							"Vector dimension mismatch in addition", e->line,
							e->col);
					vector<Value> res;
					for (size_t i = 0; i < v1->elements.size(); i++)
						res.push_back(Value::Float(v1->elements[i].asFloat() +
															v2->elements[i].asFloat()));
					return Value::Vector(res);
				}
				throw TypeError("Invalid operands for +", e->line, e->col);
			case TokenType::FLOOR_DIV:
				if (r.asFloat() == 0)
					throw DivisionByZeroError("Division by zero", e->line,
						e->col);
				if (l.isNumber() && r.isNumber())
					return Value::Int(
						(long long)std::floor(l.asFloat() / r.asFloat()));
				break;
			case TokenType::MOD:
				if (r.asInt() == 0)
					throw DivisionByZeroError("Modulo by zero", e->line,
						e->col);
				if (l.type == ValueType::INT && r.type == ValueType::INT)
					return Value::Int(l.asInt() % r.asInt());
				if (l.isNumber() && r.isNumber()) {
					return Value::Float(std::fmod(l.asFloat(), r.asFloat()));
				}
				break;
			case TokenType::POW:
				if (l.isNumber() && r.isNumber())
					return Value::Float(std::pow(l.asFloat(), r.asFloat()));
				break;
			case TokenType::MINUS:
				if (l.isNumber() && r.isNumber()) {
					if (l.type == ValueType::FLOAT ||
						 r.type == ValueType::FLOAT)
						return Value::Float(l.asFloat() - r.asFloat());
					return Value::Int(l.asInt() - r.asInt());
				}
				if (l.type == ValueType::VECTOR &&
					 r.type == ValueType::VECTOR) {
					auto *v1 = static_cast<VectorObject *>(l.ref.get());
					auto *v2 = static_cast<VectorObject *>(r.ref.get());
					if (v1->elements.size() != v2->elements.size())
						throw ValueError(
							"Vector dimension mismatch in subtraction", e->line,
							e->col);
					vector<Value> res;
					for (size_t i = 0; i < v1->elements.size(); i++)
						res.push_back(Value::Float(v1->elements[i].asFloat() -
															v2->elements[i].asFloat()));
					return Value::Vector(res);
				}
				break;
			case TokenType::STAR:
				if (l.type == ValueType::INT && r.type == ValueType::INT) {
					long long a = l.asInt();
					long long b = r.asInt();
					if (a != 0 &&
						 (b > LLONG_MAX / abs(a) || b < LLONG_MIN / abs(a)))
						return Value::Float((double)a * (double)b);
					return Value::Int(a * b);
				}
				if (l.type == ValueType::STRING && r.type == ValueType::INT) {
					string res = "";
					string base = l.asString();
					long long count = r.asInt();
					if (count < 0)
						count = 0;
					if (count > 1000000)
						throw MemoryError("String repetition too large",
							e->line, e->col);
					for (long long i = 0; i < count; i++)
						res += base;
					return Value::String(res);
				}
				if (l.type == ValueType::LIST && r.type == ValueType::INT) {
					auto *listObj = static_cast<ListObject *>(l.ref.get());
					long long count = r.asInt();
					vector<Value> res;
					if (count > 0) {
						if (listObj->elements.size() * count > 1000000)
							throw MemoryError("List repetition too large",
								e->line, e->col);
						res.reserve(listObj->elements.size() * count);
						for (int i = 0; i < count; i++)
							for (const auto &elem : listObj->elements)
								res.push_back(deepCopy(elem));
					}
					return Value::List(res);
				}
				if (l.isNumber() && r.isNumber()) {
					return Value::Float(l.asFloat() * r.asFloat());
				}
				if (l.type == ValueType::VECTOR && r.isNumber()) {
					auto *v = static_cast<VectorObject *>(l.ref.get());
					double s = r.asFloat();
					vector<Value> res;
					for (auto val : v->elements)
						res.push_back(Value::Float(val.asFloat() * s));
					return Value::Vector(res);
				}
				if (l.isNumber() && r.type == ValueType::VECTOR) {
					auto *v = static_cast<VectorObject *>(r.ref.get());
					double s = l.asFloat();
					vector<Value> res;
					for (auto val : v->elements)
						res.push_back(Value::Float(val.asFloat() * s));
					return Value::Vector(res);
				}
				if (l.type == ValueType::VECTOR &&
					 r.type == ValueType::VECTOR) {
					auto *v1 = static_cast<VectorObject *>(l.ref.get());
					auto *v2 = static_cast<VectorObject *>(r.ref.get());
					if (v1->elements.size() != v2->elements.size())
						throw ValueError(
							"Vector dimension mismatch in multiplication",
							e->line, e->col);
					vector<Value> res;
					for (size_t i = 0; i < v1->elements.size(); i++)
						res.push_back(Value::Float(v1->elements[i].asFloat() *
															v2->elements[i].asFloat()));
					return Value::Vector(res);
				}
				break;
			case TokenType::SLASH:
				if (r.asFloat() == 0)
					throw DivisionByZeroError("Division by zero", e->line,
						e->col);
				if (l.isNumber() && r.isNumber())
					return Value::Float(l.asFloat() / r.asFloat());
				if (l.type == ValueType::VECTOR ||
					 r.type == ValueType::VECTOR)
					throw TypeError("Vector division is not supported",
						e->line, e->col);
				break;
			case TokenType::GT:
				return Value::Bool(l.asFloat() > r.asFloat());
			case TokenType::LT:
				return Value::Bool(l.asFloat() < r.asFloat());
			case TokenType::GTE:
				return Value::Bool(l.asFloat() >= r.asFloat());
			case TokenType::LTE:
				return Value::Bool(l.asFloat() <= r.asFloat());
			case TokenType::EQ:
				return Value::Bool(l.looseEquals(r));
			case TokenType::STRICT_EQ:
				return Value::Bool(l.strictEquals(r));
			case TokenType::STRICT_NEQ:
				return Value::Bool(!l.strictEquals(r));
			case TokenType::NEQ:
				return Value::Bool(!l.looseEquals(r));
			case TokenType::AND:
				if (l.type == ValueType::SET && r.type == ValueType::SET)
					break;
				return Value::Bool(l.isTruthy() && r.isTruthy());
			case TokenType::OR:
				if (l.type == ValueType::SET && r.type == ValueType::SET)
					break;
				return Value::Bool(l.isTruthy() || r.isTruthy());
			case TokenType::XOR:
				if (l.type == ValueType::SET && r.type == ValueType::SET)
					break;
				return Value::Bool(l.isTruthy() != r.isTruthy());
			case TokenType::NAND:
				return Value::Bool(!(l.isTruthy() && r.isTruthy()));
			case TokenType::NOR:
				return Value::Bool(!(l.isTruthy() || r.isTruthy()));
			case TokenType::NXOR:
				return Value::Bool(l.isTruthy() == r.isTruthy());
			case TokenType::NOT:
				return Value::Bool(!r.isTruthy());
			case TokenType::IS: {
				if (l.ref && r.ref)
					return Value::Bool(l.ref.get() == r.ref.get());
				if (l.type == ValueType::NONE && r.type == ValueType::NONE)
					return Value::Bool(true);
				return Value::Bool(false);
			}
			case TokenType::IS_IN: {
				if (r.type == ValueType::STRING) {
					if (l.type != ValueType::STRING) {
						return Value::Bool(false);
					}
					return Value::Bool(r.asString().find(l.asString()) !=
											 string::npos);
				}
				if (r.type == ValueType::LIST) {
					auto *listObj = static_cast<ListObject *>(r.ref.get());
					for (const auto &item : listObj->elements) {
						if (item.strictEquals(l))
							return Value::Bool(true);
					}
					return Value::Bool(false);
				}
				if (r.type == ValueType::DICT) {
					auto *d = static_cast<DictObject *>(r.ref.get());
					return Value::Bool(d->items.count(l));
				}
				if (r.type == ValueType::SET) {
					auto *s = static_cast<SetObject *>(r.ref.get());
					for (const auto &item : s->elements) {
						if (item.strictEquals(l))
							return Value::Bool(true);
					}
					return Value::Bool(false);
				}
				if (r.type == ValueType::RANGE) {
					auto *rng = static_cast<RangeObject *>(r.ref.get());
					if (!l.isNumber())
						return Value::Bool(false);
					double val = l.asFloat();
					if (!rng->isValid)
						return Value::Bool(false);
					bool inBounds =
						(rng->step > 0)
							? (val >= rng->start &&
								  (rng->endInclusive ? val <= rng->end
															: val < rng->end))
							: (val <= rng->start &&
								  (rng->endInclusive ? val >= rng->end
															: val > rng->end));
					if (!inBounds)
						return Value::Bool(false);
					if (!rng->isFloat && l.type == ValueType::INT) {
						long long diff = (long long)(val - rng->start);
						if (diff % (long long)rng->step != 0)
							return Value::Bool(false);
					}
					return Value::Bool(true);
				}
				throw TypeError("RHS of 'in' must be iterable", e->line,
					e->col);
			}
			case TokenType::IS_NOT: {
				if (l.ref && r.ref)
					return Value::Bool(l.ref.get() != r.ref.get());
				if (l.type == ValueType::NONE && r.type == ValueType::NONE)
					return Value::Bool(false);
				return Value::Bool(true);
			}
			case TokenType::IS_NOT_IN: {
				if (r.type == ValueType::STRING) {
					if (l.type != ValueType::STRING)
						throw TypeError(
							"LHS of 'is not in' string must be a string",
							e->line, e->col);
					return Value::Bool(r.asString().find(l.asString()) ==
											 string::npos);
				} else if (r.type == ValueType::LIST) {
					auto *listObj = static_cast<ListObject *>(r.ref.get());
					for (const auto &item : listObj->elements)
						if (item.looseEquals(l))
							return Value::Bool(false);
					return Value::Bool(true);
				} else if (r.type == ValueType::RANGE) {
					auto *rng = static_cast<RangeObject *>(r.ref.get());
					if (!l.isNumber())
						return Value::Bool(true);
					double val = l.asFloat();
					if (!rng->isValid)
						return Value::Bool(true);
					bool inBounds =
						(rng->step > 0)
							? (val >= rng->start &&
								  (rng->endInclusive ? val <= rng->end
															: val < rng->end))
							: (val <= rng->start &&
								  (rng->endInclusive ? val >= rng->end
															: val > rng->end));
					if (!inBounds)
						return Value::Bool(true);
					if (!rng->isFloat && l.type == ValueType::INT) {
						long long diff = (long long)(val - rng->start);
						if (diff % (long long)rng->step != 0)
							return Value::Bool(true);
					}
					return Value::Bool(false);
				} else if (r.type == ValueType::SET) {
					auto *s = static_cast<SetObject *>(r.ref.get());
					for (const auto &el : s->elements)
						if (el.strictEquals(l))
							return Value::Bool(false);
					return Value::Bool(true);
				} else if (r.type == ValueType::TUPLE) {
					auto *t = static_cast<TupleObject *>(r.ref.get());
					for (const auto &item : t->elements)
						if (item.looseEquals(l))
							return Value::Bool(false);
					return Value::Bool(true);
				}
				throw TypeError("RHS of 'is not in' must be iterable",
					e->line, e->col);
			}
			case TokenType::COLON: {
				Value k = l;
				Value v = r;
				vector<std::pair<Value, Value>> p;
				p.push_back({k, v});
				return Value::Paired(p);
			}
			default:
				return Value::None();
			}
			if (l.type == ValueType::SET && r.type == ValueType::SET) {
				auto *s1 = static_cast<SetObject *>(l.ref.get());
				auto *s2 = static_cast<SetObject *>(r.ref.get());
				std::unordered_set<Value, ValueHash, ValueEqual> result;
				if (b->op == TokenType::OR) {
					result = s1->elements;
					for (auto &v : s2->elements)
						setAdd(result, v);
					return Value::Set(result);
				}
				if (b->op == TokenType::AND) {
					for (auto &v2 : s2->elements) {
						if (s1->elements.count(v2))
							setAdd(result, v2);
					}
					return Value::Set(result);
				}
				if (b->op == TokenType::MINUS || b->op == TokenType::SLASH) {
					for (auto &v1 : s1->elements) {
						if (s2->elements.count(v1))
							continue;
						setAdd(result, v1);
					}
					return Value::Set(result);
				}
				if (b->op == TokenType::XOR) {
					for (auto &v1 : s1->elements) {
						if (s2->elements.count(v1))
							continue;
						setAdd(result, v1);
					}
					for (auto &v2 : s2->elements) {
						if (s1->elements.count(v2))
							continue;
						setAdd(result, v2);
					}
					return Value::Set(result);
				}
			}
		}
		case ExprType::OMIT_MARKER_EXPR: {
			return Value::Omit();
		}
		case ExprType::CALL: {
			auto c = static_cast<CallExpr *>(e);
			Value callee;
			if (c->cachedFunc) {
				callee = *c->cachedFunc;
			} else {
				if (!env->exists(c->name))
					throw NameError("Undefined function '" + c->name + "'",
						c->line, c->col);
				Var &v = env->lookup(c->name);
				callee = v.value;
				if (env->parent == nullptr)
					c->cachedFunc = &v.value;
			}
			vector<CallArg> callArgs;
			for (size_t i = 0; i < c->args.size(); i++) {
				CallArg ca;
				if (c->modes[i] == CopyMode::REF) {
					LValue lv = resolveLValue(c->args[i]);
					ca.lvalue = lv;
					ca.hasLValue = true;
					ca.value = *lv.ref;
				} else
					ca.value = eval(c->args[i]);
				callArgs.push_back(ca);
			}
			return call(callee, callArgs, c->modes, c->line, c->col);
		}
		case ExprType::INDEX: {
			auto idx = static_cast<IndexExpr *>(e);
			Value base = eval(idx->base);
			Value index = eval(idx->index);
			auto normalize = [&](long long i, size_t len) {
				if (i < 0)
					i += len;
				if (i < 0 || i >= (long long)len)
					throw IndexError("Index out of bounds", idx->line, idx->col);
				return i;
			};
			auto getSliceIndices = [&](size_t len) -> vector<long long> {
				auto *s = static_cast<SliceObject *>(index.ref.get());
				long long step =
					(s->step.type == ValueType::INT) ? s->step.asInt() : 1;
				if (step == 0)
					throw ValueError("Slice step cannot be zero", idx->line,
						idx->col);
				long long start, end;
				if (step > 0) {
					start =
						(s->start.type == ValueType::INT) ? s->start.asInt() : 0;
					end = (s->end.type == ValueType::INT) ? s->end.asInt()
																	  : (long long)len;
				} else {
					start = (s->start.type == ValueType::INT)
								  ? s->start.asInt()
								  : (long long)len - 1;
					end = (s->end.type == ValueType::INT) ? s->end.asInt()
																	  : -1LL - 1LL;
				}
				auto normalizeSlice = [&](long long val) {
					return (val < 0) ? val + (long long)len : val;
				};
				if (s->start.type == ValueType::INT)
					start = normalizeSlice(start);
				if (s->end.type == ValueType::INT)
					end = normalizeSlice(end);
				auto clamp = [&](long long val) {
					if (val < 0)
						return (step > 0) ? 0LL : -1LL;
					if (val > (long long)len)
						return (long long)len;
					return val;
				};
				if (step > 0) {
					if (start < 0)
						start = 0;
					if (end < 0)
						end = 0;
					if (start > (long long)len)
						start = len;
					if (end > (long long)len)
						end = len;
				} else {
					if (start < -1)
						start = -1;
					if (end < -1)
						end = -1;
					if (start > (long long)len - 1)
						start = len - 1;
					if (end > (long long)len - 1)
						end = len - 1;
				}
				vector<long long> result;
				if (step > 0) {
					for (long long i = start; i < end; i += step) {
						if (i >= 0 && i < (long long)len)
							result.push_back(i);
					}
				} else {
					for (long long i = start; i > end; i += step) {
						if (i >= 0 && i < (long long)len)
							result.push_back(i);
					}
				}
				return result;
			};
			if (base.type == ValueType::DICT) {
				auto *dictObj = static_cast<DictObject *>(base.ref.get());
				if (dictObj->items.find(index) == dictObj->items.end()) {
					throw KeyError("Key not found in dictionary", idx->line,
						idx->col);
				}
				return dictObj->items.at(index);
			}
			if (base.type == ValueType::LIST) {
				auto *list = static_cast<ListObject *>(base.ref.get());
				size_t len = list->elements.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					Value el = list->elements[i];
					if (base.isConst)
						el.isConst = true;
					return el;
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					auto *newList = new ListObject();
					for (long long i : indices) {
						newList->elements.push_back(list->elements[i]);
					}
					Value ret;
					ret.type = ValueType::LIST;
					ret.ref = shared_ptr<HeapObject>(newList);
					ret.isConst = base.isConst;
					return ret;
				}
				throw TypeError("List index must be int or slice", idx->line,
					idx->col);
			}
			if (base.type == ValueType::STRING) {
				const string &s = base.asString();
				size_t len = s.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					return Value::String(string(1, s[i]));
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					string newStr = "";
					for (long long i : indices)
						newStr += s[i];
					return Value::String(newStr);
				}
				throw TypeError("String index must be int or slice", idx->line,
					idx->col);
			}
			if (base.type == ValueType::TUPLE) {
				auto *t = static_cast<TupleObject *>(base.ref.get());
				size_t len = t->elements.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					return t->elements[i];
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					auto *newTuple = new TupleObject();
					for (long long i : indices)
						newTuple->elements.push_back(t->elements[i]);
					Value ret;
					ret.type = ValueType::TUPLE;
					ret.ref = shared_ptr<HeapObject>(newTuple);
					ret.isConst = base.isConst;
					return ret;
				}
				throw TypeError("Tuple index must be int or slice", idx->line,
					idx->col);
			}
			if (base.type == ValueType::RANGE) {
				if (index.type != ValueType::INT)
					throw TypeError("slice index must be int", idx->line,
						idx->col);
				auto *r = static_cast<RangeObject *>(base.ref.get());
				long long i = index.asInt();
				double val = r->start;
				if (!r->startInclusive)
					val += r->step;
				val += (r->step * i);
				bool outOfBounds = false;
				if (r->step > 0) {
					if (r->endInclusive) {
						if (val > r->end)
							outOfBounds = true;
					} else {
						if (val >= r->end)
							outOfBounds = true;
					}
				} else {
					if (r->endInclusive) {
						if (val < r->end)
							outOfBounds = true;
					} else {
						if (val <= r->end)
							outOfBounds = true;
					}
				}
				if (outOfBounds || i < 0)
					throw IndexError("slice index out of bounds", idx->line,
						idx->col);
				return r->isFloat ? Value::Float(val)
										: Value::Int((long long)val);
			}
			if (base.type == ValueType::SET) {
				auto *setObj = static_cast<SetObject *>(base.ref.get());
				size_t len = setObj->elements.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					auto it = setObj->elements.begin();
					std::advance(it, i);
					Value val = *it;
					if (base.isConst)
						val.isConst = true;
					return val;
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					auto *newSet = new SetObject();
					for (long long idx_val : indices) {
						auto it = setObj->elements.begin();
						std::advance(it, idx_val);
						setAdd(newSet->elements, *it);
					}
					Value ret;
					ret.type = ValueType::SET;
					ret.ref = shared_ptr<HeapObject>(newSet);
					ret.isConst = base.isConst;
					return ret;
				}
				throw TypeError("Set index must be int or slice", idx->line,
					idx->col);
			}
			if (base.type == ValueType::VECTOR) {
				auto *vec = static_cast<VectorObject *>(base.ref.get());
				size_t len = vec->elements.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					return Value::Float(vec->elements[i].asFloat());
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					vector<Value> newElems;
					for (long long i : indices) {
						newElems.push_back(vec->elements[i]);
					}
					return Value::Vector(newElems);
				}
				throw TypeError("Vector index must be int or slice", idx->line,
					idx->col);
			}
			throw TypeError("Indexing non-indexable type", idx->line, idx->col);
		}
		case ExprType::METHOD_CALL: {
			return Resolve_methods(static_cast<MethodCallExpr *>(e));
		}
		case ExprType::LAMBDA: {
			auto lam = static_cast<LambdaExpr *>(e);
			auto *funcObj = new FunctionObject(
				lam->params, lam->returnType, lam->defaultRetArgs,
				lam->returnsConst, lam->body, env, lam->isCached, nullptr, lam->returnsRef);
			Value v;
			v.type = ValueType::FUNCTION;
			v.ref = std::shared_ptr<HeapObject>(funcObj);
			return v;
		}
		case ExprType::RANGE: {
			auto r = static_cast<RangeExpr *>(e);
			Value s = eval(r->start);
			Value en = eval(r->end);
			if (!s.isNumber() || !en.isNumber())
				throw TypeError("Range start and end must be numbers", r->line,
					r->col);
			double startVal = s.asFloat();
			double endVal = en.asFloat();
			double stepVal = 1.0;
			bool isFloat =
				(s.type == ValueType::FLOAT || en.type == ValueType::FLOAT);
			if (r->step == nullptr) {
				if (startVal > endVal)
					stepVal = -1.0;
				else
					stepVal = 1.0;
			} else {
				Value st = eval(r->step);
				if (!st.isNumber())
					throw TypeError("Range step must be a number", r->line,
						r->col);
				stepVal = st.asFloat();
				if (st.type == ValueType::FLOAT)
					isFloat = true;
			}
			if (stepVal == 0)
				throw ValueError("Range step cannot be zero", r->line, r->col);
			return Value::Range(startVal, endVal, stepVal, r->startInclusive,
				r->endInclusive, isFloat);
		}
		case ExprType::SET: {
			auto s = static_cast<SetExpr *>(e);
			std::unordered_set<Value, ValueHash, ValueEqual> vals;
			for (auto el : s->elements)
				setAdd(vals, eval(el));
			return Value::Set(vals);
		}
		case ExprType::DICT: {
			auto d = static_cast<DictExpr *>(e);
			std::unordered_map<Value, Value, ValueHash, ValueEqual> map;
			for (auto &item : d->items) {
				Value key = eval(item.first);
				Value val = eval(item.second);
				if (key.type == ValueType::LIST || key.type == ValueType::SET) {
					key = deepCopy(key);
					key.isConst = true;
				}
				if (key.type == ValueType::DICT)
					throw TypeError("Dictionary cannot be used as a key", e->line,
						e->col);
				map[key] = val;
			}
			return Value::Dict(map);
		}
		case ExprType::TUPLE: {
			auto t = static_cast<TupleExpr *>(e);
			vector<Value> vals;
			for (auto el : t->elements)
				vals.push_back(eval(el));
			return Value::Tuple(vals);
		}
		case ExprType::TERNARY: {
			auto t = static_cast<TernaryExpr *>(e);
			Value cond = eval(t->condition);
			if (cond.isTruthy())
				return eval(t->trueBranch);
			else
				return eval(t->falseBranch);
		}
		case ExprType::COMPREHENSION: {
			auto comp = static_cast<CompExpr *>(e);
			Value collection = eval(comp->iterable);
			vector<Value> items;
			bool isSet = false;
			if (collection.type == ValueType::LIST)
				items = static_cast<ListObject *>(collection.ref.get())->elements;
			else if (collection.type == ValueType::SET) {
				auto *s = static_cast<SetObject *>(collection.ref.get());
				items.assign(s->elements.begin(), s->elements.end());
			} else if (collection.type == ValueType::TUPLE)
				items =
					static_cast<TupleObject *>(collection.ref.get())->elements;
			else if (collection.type == ValueType::STRING) {
				for (char c : collection.asString())
					items.push_back(Value::String(string(1, c)));
			} else if (collection.type == ValueType::RANGE) {
				auto *r = static_cast<RangeObject *>(collection.ref.get());
				double cur = r->start;
				if (!r->startInclusive)
					cur += r->step;
				while (true) {
					bool cond =
						(r->step > 0)
							? (r->endInclusive ? cur <= r->end : cur < r->end)
							: (r->endInclusive ? cur >= r->end : cur > r->end);
					if (!cond)
						break;
					items.push_back(r->isFloat ? Value::Float(cur)
														: Value::Int((long long)cur));
					cur += r->step;
				}
			} else if (collection.type == ValueType::DICT) {
				auto *d = static_cast<DictObject *>(collection.ref.get());
				for (auto &pair : d->items)
					items.push_back(pair.first);
			} else if (collection.type == ValueType::VECTOR) {
				auto *v = static_cast<VectorObject *>(collection.ref.get());
				for (auto d : v->elements)
					items.push_back(d);
			} else
				throw TypeError("Comprehension 'in' target must be iterable", comp->line, comp->col);
			vector<Value> listResults;
			std::unordered_set<Value, ValueHash, ValueEqual> setResults;
			std::unordered_map<Value, Value, ValueHash, ValueEqual> dictResults;
			bool isDict = (comp->typeToken == TokenType::LBRACE && comp->valueExpr != nullptr);
			bool isSetComp = (comp->typeToken == TokenType::LBRACE && comp->valueExpr == nullptr);
			std::shared_ptr<Env> prevEnv = env;
			for (const auto &item : items) {
				env = std::make_shared<Env>();
				env->parent = prevEnv;
				env->set(comp->varName, item, false, false);
				if (comp->filter) {
					Value cond = eval(comp->filter);
					if (!cond.isTruthy()) {
						continue;
					}
				}
				if (isDict) {
					Value k = eval(comp->expression);
					if (k.type == ValueType::LIST || k.type == ValueType::SET || k.type == ValueType::DICT) {
						k = deepCopy(k);
						k.isConst = true;
					}
					Value v = eval(comp->valueExpr);
					dictResults[k] = v;
				} else {
					Value v = eval(comp->expression);
					if (isSetComp) {
						if (v.type == ValueType::LIST || v.type == ValueType::SET || v.type == ValueType::DICT) {
							v = deepCopy(v);
							v.isConst = true;
						}
						setResults.insert(v);
					} else {
						listResults.push_back(v);
					}
				}
			}
			env = prevEnv;
			if (comp->typeToken == TokenType::LBRACKET)
				return Value::List(listResults);
			if (comp->typeToken == TokenType::LPAREN)
				return Value::Tuple(listResults);
			if (comp->typeToken == TokenType::LBRACE) {
				if (isDict)
					return Value::Dict(dictResults);
				else
					return Value::Set(setResults);
			}
			return Value::None();
		}
		case ExprType::SLICE: {
			auto sl = static_cast<SliceExpr *>(e);
			Value s = sl->start ? eval(sl->start) : Value::None();
			Value en = sl->end ? eval(sl->end) : Value::None();
			Value st = sl->step ? eval(sl->step) : Value::None();
			return Value::Slice(s, en, st);
		}
		case ExprType::VECTOR: {
			auto ve = static_cast<VectorExpr *>(e);
			if (!vectorEnabled)
				throw RuntimeError(
					"Vector syntax <...> requires 'import Vector'", e->line,
					e->col);
			vector<Value> elems;
			for (auto *el : ve->elements) {
				Value v = eval(el);
				if (!v.isNumber())
					throw TypeError("Vector elements must be numbers", e->line,
						e->col);
				elems.push_back(v);
			}
			return Value::Vector(elems);
		}
		default: {
			return Value::None();
		}
		}
	}

Value Interpreter::exec(Stmt *s) {
		switch (s->type) {
		case StmtType::ASSIGN: {
			auto as = static_cast<AssignStmt *>(s);
			if (as->target->type == ExprType::INDEX) {
				auto idx = static_cast<IndexExpr *>(as->target);
				Value base = eval(idx->base);
				if (base.type == ValueType::STRING) {
					Value rhs = eval(as->value);
					if (rhs.type != ValueType::STRING)
						throw TypeError("Can only insert string into string",
							s->line, s->col);
					int i = eval(idx->index).asInt();
					auto *strObj = static_cast<StringObject *>(base.ref.get());
					string &str = strObj->value;
					if (i < 0 || i > (int)str.size())
						throw IndexError("String index out of bounds", s->line,
							s->col);
					if (i == (int)str.size())
						str += rhs.asString();
					else {
						const string &r = rhs.asString();
						int eraseLen =
							std::min((int)r.size(), (int)str.size() - i);
						str.erase(i, eraseLen);
						str.insert(i, r);
					}
					return Value::None();
				}
				if (base.type == ValueType::DICT) {
					if (base.isConst)
						throw ConstError("Cannot modify const dictionary", s->line,
							s->col);
					Value key = eval(idx->index);
					Value val = eval(as->value);
					if (key.type == ValueType::LIST ||
						 key.type == ValueType::SET) {
						key = deepCopy(key);
						key.isConst = true;
					}
					if (key.type == ValueType::DICT)
						throw TypeError("Dictionary cannot be used as a key",
							s->line, s->col);
					auto *dictObj = static_cast<DictObject *>(base.ref.get());
					dictObj->items[key] = val;
					return Value::None();
				}
			}
			LValue lv = resolveLValue(as->target);
			Value &cur = *lv.ref;
			if (cur.isConst || lv.isConstView)
				throw ConstError("Cannot assign to or modify const variable",
					s->line, s->col);
			Value rhs = eval(as->value);
			if (lv.isLocked && cur.type != rhs.type) {
				throw TypeError("Cannot change type of locked variable", s->line,
					s->col);
			}
			if (as->op == TokenType::ASSIGN) {
				if (lv.isRefTarget) {
					*lv.ref = rhs;
					return Value::None();
				}
				if (cur.ref && cur.ref->typeLocked && cur.type != rhs.type) {
					throw TypeError("Cannot change type of locked variable",
						s->line, s->col);
				}
				if (cur.ref && cur.type == rhs.type) {
					if (cur.type == ValueType::STRING)
						static_cast<StringObject *>(cur.ref.get())->value =
							rhs.asString();
					else
						cur = rhs;
				} else
					cur = rhs;
				return Value::None();
			}
			if (cur.type == ValueType::STRING) {
				if (rhs.type == ValueType::STRING &&
					 as->op == TokenType::PLUS_EQ) {
					static_cast<StringObject *>(cur.ref.get())->value +=
						rhs.asString();
					return Value::None();
				}
				if (rhs.type == ValueType::INT && as->op == TokenType::STAR_EQ) {
					string base =
						static_cast<StringObject *>(cur.ref.get())->value;
					long long count = rhs.asInt();
					if (count < 0)
						count = 0;
					string res = "";
					if (base.length() * count > 10000000)
						throw MemoryError("Resulting string too large", s->line,
							s->col);
					for (int k = 0; k < count; k++)
						res += base;
					static_cast<StringObject *>(cur.ref.get())->value = res;
					return Value::None();
				}
			}
			if (cur.type == ValueType::LIST && rhs.type == ValueType::INT &&
				 as->op == TokenType::STAR_EQ) {
				auto *listObj = static_cast<ListObject *>(cur.ref.get());
				int count = rhs.asInt();
				if (count <= 0)
					listObj->elements.clear();
				else if (count > 1) {
					vector<Value> orig = listObj->elements;
					if (orig.size() * count > 1000000)
						throw MemoryError("Resulting list too large", s->line,
							s->col);
					listObj->elements.reserve(orig.size() * count);
					for (int k = 1; k < count; k++)
						listObj->elements.insert(listObj->elements.end(),
							orig.begin(), orig.end());
				}
				return Value::None();
			}
			if (cur.type == ValueType::FLOAT && rhs.type == ValueType::FLOAT) {
				switch (as->op) {
				case TokenType::PLUS_EQ:
					cur.fVal += rhs.asFloat();
					break;
				case TokenType::MINUS_EQ:
					cur.fVal -= rhs.asFloat();
					break;
				case TokenType::STAR_EQ:
					cur.fVal *= rhs.asFloat();
					break;
				case TokenType::DIV_EQ:
					if (rhs.asFloat() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.fVal /= rhs.asFloat();
					break;
				case TokenType::POW_EQ:
					cur.fVal = std::pow(cur.fVal, rhs.asFloat());
					break;
				default:
					throw TypeError("Invalid float operation", s->line,
						s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::FLOAT && rhs.type == ValueType::INT) {
				switch (as->op) {
				case TokenType::PLUS_EQ:
					cur.fVal += rhs.asFloat();
					break;
				case TokenType::MINUS_EQ:
					cur.fVal -= rhs.asFloat();
					break;
				case TokenType::STAR_EQ:
					cur.fVal *= rhs.asFloat();
					break;
				case TokenType::DIV_EQ:
					if (rhs.asFloat() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.fVal /= rhs.asFloat();
					break;
				case TokenType::POW_EQ:
					cur.fVal = std::pow(cur.fVal, rhs.asFloat());
					break;
				default:
					throw TypeError("Invalid float operation", s->line,
						s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::INT && rhs.type == ValueType::INT) {
				switch (as->op) {
				case TokenType::PLUS_EQ:
					cur.iVal += rhs.asInt();
					break;
				case TokenType::MINUS_EQ:
					cur.iVal -= rhs.asInt();
					break;
				case TokenType::STAR_EQ:
					cur.iVal *= rhs.asInt();
					break;
				case TokenType::DIV_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.iVal /= rhs.asInt();
					break;
				case TokenType::MOD_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Modulo by zero", s->line,
							s->col);
					cur.iVal %= rhs.asInt();
					break;
				case TokenType::AND_EQ:
					cur.iVal &= rhs.asInt();
					break;
				case TokenType::OR_EQ:
					cur.iVal |= rhs.asInt();
					break;
				case TokenType::XOR_EQ:
					cur.iVal ^= rhs.asInt();
					break;
				case TokenType::FLOOR_DIV_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.iVal =
						(long long)std::floor((double)cur.iVal / rhs.asInt());
					break;
				case TokenType::POW_EQ:
					cur.iVal = (long long)std::pow(cur.iVal, rhs.asInt());
					break;
				default:
					throw TypeError("Invalid int operation", s->line, s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::INT && rhs.type == ValueType::FLOAT) {
				switch (as->op) {
				case TokenType::PLUS_EQ:
					cur.iVal += rhs.asInt();
					break;
				case TokenType::MINUS_EQ:
					cur.iVal -= rhs.asInt();
					break;
				case TokenType::STAR_EQ:
					cur.iVal *= rhs.asInt();
					break;
				case TokenType::DIV_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.iVal /= rhs.asInt();
					break;
				case TokenType::MOD_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Modulo by zero", s->line,
							s->col);
					cur.iVal %= rhs.asInt();
					break;
				case TokenType::AND_EQ:
					cur.iVal &= rhs.asInt();
					break;
				case TokenType::OR_EQ:
					cur.iVal |= rhs.asInt();
					break;
				case TokenType::XOR_EQ:
					cur.iVal ^= rhs.asInt();
					break;
				case TokenType::FLOOR_DIV_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.iVal =
						(long long)std::floor((double)cur.iVal / rhs.asInt());
					break;
				case TokenType::POW_EQ:
					cur.iVal = (long long)std::pow(cur.iVal, rhs.asInt());
					break;
				default:
					throw TypeError("Invalid int operation", s->line, s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::BOOL && rhs.type == ValueType::BOOL) {
				switch (as->op) {
				case TokenType::AND_EQ:
					cur.bVal = cur.bVal & rhs.asBool();
					break;
				case TokenType::OR_EQ:
					cur.bVal = cur.bVal | rhs.asBool();
					break;
				case TokenType::XOR_EQ:
					cur.bVal = cur.bVal ^ rhs.asBool();
					break;
				default:
					throw TypeError("Invalid bool operation", s->line, s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::SET && rhs.type == ValueType::SET) {
				auto *s1 = static_cast<SetObject *>(cur.ref.get());
				auto *s2 = static_cast<SetObject *>(rhs.ref.get());
				if (as->op == TokenType::OR_EQ) {
					s1->elements.insert(s2->elements.begin(), s2->elements.end());
					return Value::None();
				}
				if (as->op == TokenType::AND_EQ) {
					std::unordered_set<Value, ValueHash, ValueEqual> keep;
					for (const auto &v1 : s1->elements) {
						if (s2->elements.count(v1) > 0) {
							keep.insert(v1);
						}
					}
					s1->elements = std::move(keep);
					return Value::None();
				}
				if (as->op == TokenType::MINUS_EQ || as->op == TokenType::DIV_EQ) {
					for (const auto &v2 : s2->elements) {
						s1->elements.erase(v2);
					}
					return Value::None();
				}
				if (as->op == TokenType::XOR_EQ) {
					for (const auto &v2 : s2->elements) {
						if (s1->elements.count(v2) > 0) {
							s1->elements.erase(v2);
						} else {
							s1->elements.insert(v2);
						}
					}
					return Value::None();
				}
			}
			if (cur.type == ValueType::VECTOR) {
				auto *v = static_cast<VectorObject *>(cur.ref.get());
				if (as->op == TokenType::PLUS_EQ) {
					if (rhs.type != ValueType::VECTOR)
						throw TypeError("+= requires a vector", s->line, s->col);
					auto *v2 = static_cast<VectorObject *>(rhs.ref.get());
					if (v->elements.size() != v2->elements.size())
						throw ValueError("Dimension mismatch", s->line, s->col);
					for (size_t i = 0; i < v->elements.size();
						i++)
						; // v->elements[i].asFloat() +=
						  // v2->elements[i].asFloat();
					return Value::None();
				}
				if (as->op == TokenType::MINUS_EQ) {
					if (rhs.type != ValueType::VECTOR)
						throw TypeError("-= requires a vector", s->line, s->col);
					auto *v2 = static_cast<VectorObject *>(rhs.ref.get());
					if (v->elements.size() != v2->elements.size())
						throw ValueError("Dimension mismatch", s->line, s->col);
					for (size_t i = 0; i < v->elements.size();
						i++)
						; // v->elements[i].asFloat() -=
						  // v2->elements[i].asFloat();
					return Value::None();
				}
				if (as->op == TokenType::STAR_EQ) {
					if (rhs.isNumber()) {
						double scalar = rhs.asFloat();
						for (auto &val : v->elements)
							; // val.asFloat() *= scalar;
					} else if (rhs.type == ValueType::VECTOR) {
						auto *v2 = static_cast<VectorObject *>(rhs.ref.get());
						if (v->elements.size() != v2->elements.size())
							throw ValueError("Dimension mismatch", s->line, s->col);
						for (size_t i = 0; i < v->elements.size();
							i++)
							; // v->elements[i].asFloat() *=
							  // v2->elements[i].asFloat();
					} else
						throw TypeError("*= requires number or vector", s->line,
							s->col);
					return Value::None();
				}
				if (as->op == TokenType::DIV_EQ)
					throw TypeError("/= not supported for vectors", s->line,
						s->col);
			}
			throw TypeError("Invalid operands for compound assignment", s->line,
				s->col);
		}
		case StmtType::LET: {
			auto let = static_cast<LetStmt *>(s);
			if (env->existsLocal(let->name)) {
				throw RuntimeError("Variable '" + let->name +
											 "' is already defined in this scope.",
					s->line, s->col);
			}
			if (env->exists(let->name)) {
				Var &existing = env->lookup(let->name);
				if (existing.isConst) {
					throw ConstError(
						"Cannot shadow constant '" + let->name + "'.", s->line,
						s->col);
				}
			}
			if (!let->value) {
				env->set(let->name, Value::NoType(), false);
				return Value::None();
			}
			if (let->value->type == ExprType::OWNERSHIP) {
				auto o = static_cast<OwnershipExpr *>(let->value);
				if (o->mode == CopyMode::REF) {
					LValue lv = resolveLValue(o->expr);
					Var varStruct;
					varStruct.alias = lv.ref;
					varStruct.isConst = let->isConst;
					env->vars[let->name] = varStruct;
					return Value::None();
				}
			}
			Value v = eval(let->value);
			if (let->isConst)
				v.isConst = true;
			bool lock = false;
			if (auto call = dynamic_cast<CallExpr *>(let->value)) {
				if (call->name == "int" || call->name == "float" ||
					 call->name == "bool" || call->name == "tuple" ||
					 call->name == "string" || call->name == "list" ||
					 call->name == "range" || call->name == "set") {
					lock = true;
				}
			}
			env->set(let->name, v, let->isLocked, let->isConst);
			return Value::None();
		}
		case StmtType::RETURN: {
			auto ret = static_cast<ReturnStmt *>(s);
			returning = true;
			returnValue =
				(ret->value != nullptr) ? eval(ret->value) : Value::NoType();
			return returnValue;
		}
		case StmtType::FUNC: {
			auto f = static_cast<FuncStmt *>(s);
			auto *func =
				new FunctionObject(f->params, f->returnType, f->defaultRetArgs,
					f->returnsConst, f->body, env, f->isCached, nullptr, f->returnsRef);
			Value funcVal;
			funcVal.type = ValueType::FUNCTION;
			funcVal.ref = std::shared_ptr<HeapObject>(func);
			if (env->existsLocal(f->name)) {
				Var &existing = env->lookup(f->name);
				if (existing.value.type == ValueType::FUNCTION) {
					Value group = Value::Overload(existing.value);
					static_cast<OverloadObject *>(group.ref.get())
						->overloads.push_back(funcVal);
					existing.value = group;
				} else if (existing.value.type == ValueType::OVERLOAD) {
					static_cast<OverloadObject *>(existing.value.ref.get())
						->overloads.push_back(funcVal);
				} else {
					Var v;
					v.value = funcVal;
					v.isConst = f->returnsConst;
					env->vars[f->name] = v;
				}
			} else {
				Var v;
				v.value = funcVal;
				v.isConst = f->returnsConst;
				env->vars[f->name] = v;
			}
			return Value::None();
		}
		case StmtType::IF: {
			auto ifs = static_cast<IfStmt *>(s);
			if (eval(ifs->condition).isTruthy()) {
				for (auto st : ifs->body) {
					exec(st);
					if (returning || breaking || continuing)
						break;
				}
				return returnValue;
			}
			for (auto &[cond, block] : ifs->elifs) {
				if (eval(cond).isTruthy()) {
					for (auto st : block) {
						exec(st);
						if (returning || breaking || continuing)
							break;
					}
					return returnValue;
				}
			}
			for (auto st : ifs->elseBody) {
				exec(st);
				if (returning || breaking || continuing)
					break;
			}
			return returnValue;
		}
		case StmtType::EXPR: {
			auto es = static_cast<ExprStmt *>(s);
			eval(es->expr);
			return Value::None();
		}
		case StmtType::BREAK: {
			breaking = true;
			return Value::None();
		}
		case StmtType::CONTINUE: {
			continuing = true;
			return Value::None();
		}
		case StmtType::SKIP: {
			auto sk = static_cast<SkipStmt *>(s);
			skipCount = eval(sk->count).asInt();
			return Value::None();
		}
		case StmtType::WHILE: {
			auto w = static_cast<WhileStmt *>(s);
			std::shared_ptr<Env> loopEnv = std::make_shared<Env>();
			loopEnv->parent = env;
			std::shared_ptr<Env> previousGlobal = env;
			if (canOptimizeScope(w->body)) {
				env = loopEnv;
				while (eval(w->condition).isTruthy()) {
					for (auto st : w->body) {
						exec(st);
						if (breaking || continuing || returning)
							break;
					}
					if (breaking) {
						breaking = false;
						break;
					}
					if (continuing) {
						continuing = false;
					}
					if (returning)
						break;
				}
				env = previousGlobal;
			} else {
				while (eval(w->condition).isTruthy()) {
					env = loopEnv;
					for (auto st : w->body) {
						exec(st);
						if (breaking || continuing || returning)
							break;
					}
					env = previousGlobal;
					if (breaking) {
						breaking = false;
						break;
					}
					if (continuing) {
						continuing = false;
					}
					if (returning)
						break;
					loopEnv = std::make_shared<Env>();
					loopEnv->parent = previousGlobal;
				}
			}
			return Value::None();
		}
		case StmtType::DO_WHILE: {
			auto d = static_cast<DoWhileStmt *>(s);
			do {
				std::shared_ptr<Env> prevEnv = env;
				env = std::make_shared<Env>();
				env->parent = prevEnv;
				for (auto st : d->body) {
					exec(st);
					if (breaking || continuing || returning)
						break;
				}
				env = prevEnv;
				if (breaking) {
					breaking = false;
					return Value::None();
				}
				if (continuing) {
					continuing = false;
				}
				if (returning)
					return returnValue;
			} while (eval(d->condition).isTruthy());
			return Value::None();
		}
		case StmtType::FOR: {
			auto f = static_cast<ForStmt *>(s);
			std::shared_ptr<Env> loopEnv = std::make_shared<Env>();
			loopEnv->parent = env;
			std::shared_ptr<Env> previousGlobal = env;
			env = loopEnv;
			for (auto *st : f->inits)
				exec(st);
			bool staticScope = true;
			for (auto *st : f->body) {
				if (st->type == StmtType::LET ||
					 st->type == StmtType::MULTI_LET ||
					 st->type == StmtType::FUNC || st->type == StmtType::IMPORT) {
					staticScope = false;
					break;
				}
			}
			Value *fastCounter = nullptr;
			if (f->steps.size() == 1 && f->steps[0]->type == StmtType::ASSIGN) {
				auto as = static_cast<AssignStmt *>(f->steps[0]);
				try {
					LValue lv = resolveLValue(as->target);
					if (lv.ref->type == ValueType::INT ||
						 lv.ref->type == ValueType::FLOAT)
						fastCounter = lv.ref;
				} catch (...) {
				}
			}
			double fastLimit = 0;
			TokenType fastOp = TokenType::LET;
			bool useFastCondition = false;
			if (fastCounter && f->condition->type == ExprType::BINARY) {
				auto be = static_cast<BinExpr *>(f->condition);
				if (be->left->type == ExprType::VAR &&
					 be->right->type == ExprType::NUMBER) {
					auto ve = static_cast<VarExpr *>(be->left);
					if (static_cast<AssignStmt *>(f->steps[0])->target->type ==
						 ExprType::VAR) {
						auto stepVar = static_cast<VarExpr *>(
							static_cast<AssignStmt *>(f->steps[0])->target);
						if (stepVar->name == ve->name) {
							Value limitVal = eval(be->right);
							fastLimit = limitVal.asFloat();
							fastOp = be->op;
							useFastCondition = true;
						}
					}
				}
			}
			Value *fastBodyTarget = nullptr;
			Value *fastBodyOperand = nullptr;
			bool fastBodyIsNumber = false;
			double fastBodyLiteral = 0;
			TokenType fastBodyOp = TokenType::LET;
			bool useFastBody = false;
			if (staticScope && f->body.size() == 1 &&
				 f->body[0]->type == StmtType::ASSIGN) {
				auto as = static_cast<AssignStmt *>(f->body[0]);
				if (as->op == TokenType::PLUS_EQ ||
					 as->op == TokenType::MINUS_EQ ||
					 as->op == TokenType::STAR_EQ ||
					 as->op == TokenType::DIV_EQ) {
					try {
						LValue lv = resolveLValue(as->target);
						if (lv.ref->type == ValueType::INT ||
							 lv.ref->type == ValueType::FLOAT) {
							fastBodyTarget = lv.ref;
							fastBodyOp = as->op;
							if (as->value->type == ExprType::VAR) {
								LValue opLv = resolveLValue(as->value);
								if (opLv.ref->type == ValueType::INT ||
									 opLv.ref->type == ValueType::FLOAT) {
									fastBodyOperand = opLv.ref;
									if (fastBodyTarget->type ==
										 fastBodyOperand->type) {
										useFastBody = true;
									}
								}
							} else if (as->value->type == ExprType::NUMBER) {
								fastBodyLiteral =
									static_cast<NumberExpr *>(as->value)->val;
								fastBodyIsNumber = true;
								useFastBody = true;
							}
						}
					} catch (...) {
					}
				}
			}
			if (staticScope) {
				while (true) {
					if (useFastCondition) {
						double current = (fastCounter->type == ValueType::INT)
												  ? (double)fastCounter->iVal
												  : fastCounter->fVal;
						bool cond = false;
						switch (fastOp) {
						case TokenType::LT:
							cond = current < fastLimit;
							break;
						case TokenType::LTE:
							cond = current <= fastLimit;
							break;
						case TokenType::GT:
							cond = current > fastLimit;
							break;
						case TokenType::GTE:
							cond = current >= fastLimit;
							break;
						default:
							cond = eval(f->condition).isTruthy();
							break;
						}
						if (!cond)
							break;
					} else {
						if (!eval(f->condition).isTruthy())
							break;
					}
					if (useFastBody) {
						double operand = 0;
						if (fastBodyIsNumber)
							operand = fastBodyLiteral;
						else
							operand = (fastBodyOperand->type == ValueType::INT)
											 ? (double)fastBodyOperand->iVal
											 : fastBodyOperand->fVal;
						if (fastBodyTarget->type == ValueType::INT) {
							long long opInt = (long long)operand;
							switch (fastBodyOp) {
							case TokenType::PLUS_EQ:
								fastBodyTarget->iVal += opInt;
								break;
							case TokenType::MINUS_EQ:
								fastBodyTarget->iVal -= opInt;
								break;
							case TokenType::STAR_EQ:
								fastBodyTarget->iVal *= opInt;
								break;
							case TokenType::DIV_EQ:
								if (opInt != 0)
									fastBodyTarget->iVal /= opInt;
								break;
							}
						} else {
							switch (fastBodyOp) {
							case TokenType::PLUS_EQ:
								fastBodyTarget->fVal += operand;
								break;
							case TokenType::MINUS_EQ:
								fastBodyTarget->fVal -= operand;
								break;
							case TokenType::STAR_EQ:
								fastBodyTarget->fVal *= operand;
								break;
							case TokenType::DIV_EQ:
								if (operand != 0)
									fastBodyTarget->fVal /= operand;
								break;
							}
						}
					} else {
						for (auto st : f->body) {
							exec(st);
							if (breaking || continuing || returning)
								break;
						}
						if (breaking) {
							breaking = false;
							break;
						}
						if (continuing) {
							continuing = false;
						}
						if (returning)
							break;
					}
					if (fastCounter) {
						if (fastCounter->type == ValueType::INT)
							fastCounter->iVal += 1;
						else
							fastCounter->fVal += 1.0;
					} else {
						for (auto *st : f->steps)
							exec(st);
					}
				}
			} else {
				env = previousGlobal;
				while (eval(f->condition).isTruthy()) {
					env = loopEnv;
					for (auto st : f->body)
						exec(st);
					env = previousGlobal;
					if (breaking) {
						breaking = false;
						break;
					}
					if (continuing) {
						continuing = false;
					}
					if (returning)
						break;
					env = loopEnv;
					for (auto *st : f->steps)
						exec(st);
					env = previousGlobal;
				}
			}
			env = previousGlobal;
			if (returning)
				return returnValue;
			return Value::None();
		}
		case StmtType::FOR_EACH: {
			auto fe = static_cast<ForEachStmt *>(s);
			vector<vector<Value>> allStreams;
			size_t minLen = (size_t)-1;
			bool dictUnpack =
				fe->collections.size() == 1 && fe->loopVars.size() == 2;
			for (size_t i = 0; i < fe->collections.size(); i++) {
				Value collection = eval(fe->collections[i]);
				vector<Value> currentStream;
				vector<Value> secondaryStream;
				if (collection.type == ValueType::DICT) {
					auto *dict = static_cast<DictObject *>(collection.ref.get());
					if (dictUnpack) {
						for (auto &pair : dict->items) {
							currentStream.push_back(pair.first);
							secondaryStream.push_back(pair.second);
						}
					} else {
						for (auto &pair : dict->items)
							currentStream.push_back(pair.first);
					}
				} else if (collection.type == ValueType::LIST) {
					auto *list = static_cast<ListObject *>(collection.ref.get());
					for (auto &v : list->elements)
						currentStream.push_back(v);
				} else if (collection.type == ValueType::SET) {
					auto *set = static_cast<SetObject *>(collection.ref.get());
					for (auto &v : set->elements)
						currentStream.push_back(v);
				} else if (collection.type == ValueType::TUPLE) {
					auto *tuple = static_cast<TupleObject *>(collection.ref.get());
					for (auto &v : tuple->elements)
						currentStream.push_back(v);
				} else if (collection.type == ValueType::STRING) {
					string str = collection.asString();
					for (char c : str)
						currentStream.push_back(Value::String(string(1, c)));
				} else if (collection.type == ValueType::RANGE) {
					auto *r = static_cast<RangeObject *>(collection.ref.get());
					double cur = r->start;
					if (!r->startInclusive)
						cur += r->step;
					while (true) {
						bool keep =
							(r->step > 0)
								? (r->endInclusive ? cur <= r->end : cur < r->end)
								: (r->endInclusive ? cur >= r->end : cur > r->end);
						if (!keep)
							break;
						currentStream.push_back(r->isFloat
															? Value::Float(cur)
															: Value::Int((long long)cur));
						cur += r->step;
					}
				} else if (collection.type == ValueType::VECTOR) {
					auto *v = static_cast<VectorObject *>(collection.ref.get());
					for (auto d : v->elements)
						currentStream.push_back(d);
				} else {
					throw TypeError("Object is not iterable", s->line, s->col);
				}
				if (currentStream.size() < minLen)
					minLen = currentStream.size();
				allStreams.push_back(currentStream);
				if (dictUnpack && !secondaryStream.empty()) {
					if (secondaryStream.size() < minLen)
						minLen = secondaryStream.size();
					allStreams.push_back(secondaryStream);
				}
			}
			if (minLen == (size_t)-1)
				minLen = 0;
			if (allStreams.size() != fe->loopVars.size()) {
				throw RuntimeError(
					"Mismatch between loop variables and collections", s->line,
					s->col);
			}
			std::shared_ptr<Env> prevEnv = env;
			for (size_t i = 0; i < minLen; i++) {
				if (skipCount > 0) {
					skipCount--;
					continue;
				}
				env = std::make_shared<Env>();
				env->parent = prevEnv;
				for (size_t v = 0; v < fe->loopVars.size(); v++) {
					if (i < allStreams[v].size()) {
						env->set(fe->loopVars[v], allStreams[v][i], false, false);
					}
				}
				for (auto stmt : fe->body) {
					exec(stmt);
					if (breaking || returning || continuing)
						break;
				}
				env = prevEnv;
				if (breaking) {
					breaking = false;
					break;
				}
				if (returning)
					break;
				if (continuing)
					continuing = false;
			}
			if (returning)
				return returnValue;
			return Value::None();
		}
		case StmtType::TRY: {
			auto t = static_cast<TryStmt *>(s);
			bool errorCaught = false;
			try {
				for (auto st : t->tryBlock) {
					exec(st);
					if (returning || breaking || continuing)
						break;
				}
			} catch (const LangError &e) {
				if (dynamic_cast<const ControlFlowError *>(&e)) {
					if (!t->finallyBlock.empty()) {
						bool r = returning;
						Value rv = returnValue;
						returning = false;
						try {
							for (auto st : t->finallyBlock)
								exec(st);
						} catch (...) {
							throw;
						}
						if (!returning && !breaking && !continuing) {
							returning = r;
							returnValue = rv;
						}
					}
					throw;
				}
				errorCaught = true;
				bool handled = false;
				for (auto &catchBlock : t->catches) {
					for (auto &typeName : catchBlock.types) {
						if (matchesError(e, typeName)) {
							try {
								for (auto st : catchBlock.body) {
									exec(st);
									if (returning || breaking || continuing)
										break;
								}
							} catch (...) {
								if (!t->finallyBlock.empty())
									for (auto st : t->finallyBlock)
										exec(st);
								throw;
							}
							handled = true;
							goto after_catches;
						}
					}
				}
			after_catches:
				if (!handled) {
					if (!t->finallyBlock.empty())
						for (auto st : t->finallyBlock)
							exec(st);
					throw;
				}
			}
			if (!errorCaught && !returning && !breaking && !continuing &&
				 !t->elseBlock.empty()) {
				for (auto st : t->elseBlock) {
					exec(st);
					if (returning || breaking || continuing)
						break;
				}
			}
			if (!t->finallyBlock.empty()) {
				bool oldRet = returning;
				Value oldVal = returnValue;
				bool oldBrk = breaking;
				bool oldCont = continuing;
				returning = false;
				breaking = false;
				continuing = false;
				try {
					for (auto st : t->finallyBlock) {
						exec(st);
						if (returning || breaking || continuing)
							break;
					}
				} catch (...) {
					throw;
				}
				if (!returning && !breaking && !continuing) {
					returning = oldRet;
					returnValue = oldVal;
					breaking = oldBrk;
					continuing = oldCont;
				}
			}
			return Value::None();
		}
		case StmtType::THROW: {
			auto ts = static_cast<ThrowStmt *>(s);
			Value msgVal = eval(ts->message);
			throw LangError(ts->errorType, valueToString(msgVal), -1000000,
				s->line, s->col);
		}
		case StmtType::ASSERT: {
			auto a = static_cast<AssertStmt *>(s);
			Value cond = eval(a->condition);
			if (!cond.isTruthy()) {
				string msg = "Assertion failed";
				if (a->message) {
					Value m = eval(a->message);
					msg += ": " + valueToString(m);
				}
				throw RuntimeError(msg, s->line, s->col);
			}
			return Value::None();
		}
		case StmtType::SWITCH: {
			auto sw = dynamic_cast<SwitchStmt *>(s);
			while (true) {
				Value targetVal = eval(sw->target);
				bool matchFound = false;
				for (auto &c : sw->cases) {
					Value caseVal = eval(c.value);
					if (targetVal.strictEquals(caseVal)) {
						matchFound = true;
						for (auto st : c.body) {
							exec(st);
							if (returning)
								return returnValue;
							if (continuing)
								goto exit_switch;
							if (breaking) {
								breaking = false;
								goto exit_switch;
							}
						}
						goto restart_loop;
					}
				}
				if (!matchFound) {
					for (auto st : sw->defaultBody) {
						exec(st);
						if (returning)
							return returnValue;
						if (continuing)
							goto exit_switch;
						if (breaking) {
							breaking = false;
							goto exit_switch;
						}
					}
					goto restart_loop;
				}
			restart_loop:;
			}
		exit_switch:
			return Value::None();
		}
		case StmtType::IMPORT: {
			auto imp = static_cast<ImportStmt *>(s);
			if (imp->libName.size() > 4 &&
				 imp->libName.substr(imp->libName.size() - 4) == ".ymm") {
				fs::path p(imp->libName);
				if (!fs::exists(p)) {
					throw InvalidImportError(
						"Module file not found: " + imp->libName, s->line,
						s->col);
				}
				std::error_code ec;
				string absPath = fs::absolute(p, ec).string();
				if (importStack.count(absPath)) {
					throw CircularImportError(
						"Circular import detected involving: " + imp->libName,
						s->line, s->col);
				}
				std::ifstream file(absPath);
				if (!file)
					throw InvalidImportError(
						"Unable to read module: " + imp->libName, s->line,
						s->col);
				std::stringstream buffer;
				buffer << file.rdbuf();
				string code = buffer.str();
				std::shared_ptr<Env> previousEnv = this->env;
				std::shared_ptr<Env> moduleEnv = std::make_shared<Env>();
				moduleEnv->set("None", Value::None(), true);
				importStack.insert(absPath);
				this->env = moduleEnv;
				try {
					auto tokens = tokenize(code);
					Parser parser(tokens);
					vector<Stmt *> moduleStmts;
					while (!parser.isAtEnd())
						moduleStmts.push_back(parser.parseStmt());
					for (Stmt *stmt : moduleStmts) {
						if (!isGlobalAllowed(stmt))
							throw RuntimeError(
								"Module code must only contain definitions (let, "
								"func, "
								"import).",
								stmt->line, stmt->col);
					}
					for (Stmt *ms : moduleStmts)
						exec(ms);
				} catch (...) {
					this->env = previousEnv;
					importStack.erase(absPath);
					throw;
				}
				this->env = previousEnv;
				importStack.erase(absPath);
				if (imp->symbols.empty()) {
					for (auto &[name, var] : moduleEnv->vars) {
						if (name == "None")
							continue;
						this->env->vars[name] = var;
					}
				} else {
					for (const string &sym : imp->symbols) {
						if (moduleEnv->vars.count(sym)) {
							this->env->vars[sym] = moduleEnv->vars[sym];
						} else {
							throw AttributeError("Module '" + imp->libName +
															"' does not export '" + sym +
															"'",
								s->line, s->col);
						}
					}
				}
				return Value::None();
			}
			if (modules.find(imp->libName) == modules.end()) {
				if (imp->libName.find('.') != string::npos) {
					throw InvalidImportError("Import filenames must end in .ymm",
						s->line, s->col);
				}
				throw ImportError("Unknown module '" + imp->libName + "'",
					s->line, s->col);
			}
			modules[imp->libName](this->env, imp->symbols);
			return Value::None();
		}
		case StmtType::MULTI_LET: {
			auto ml = static_cast<MultiLetStmt *>(s);
			vector<Value> results;
			for (auto *expr : ml->values)
				results.push_back(expr ? eval(expr) : Value::NoType());
			for (size_t i = 0; i < ml->names.size(); i++) {
				if (env->existsLocal(ml->names[i])) {
					throw RuntimeError("Variable '" + ml->names[i] +
												 "' is already defined in this scope.",
						s->line, s->col);
				}
				Value v = results[i];
				if (ml->isConsts[i])
					v.isConst = true;
				bool lock = false;
				env->set(ml->names[i], v, ml->isLocked, ml->isConsts[i]);
			}
			return Value::None();
		}
		case StmtType::MULTI_ASSIGN: {
			auto ma = static_cast<MultiAssignStmt *>(s);
			vector<LValue> lvals;
			for (auto *t : ma->targets)
				lvals.push_back(resolveLValue(t));
			vector<Value> rvals;
			for (auto *v : ma->values)
				rvals.push_back(eval(v));
			for (size_t i = 0; i < lvals.size(); i++) {
				Value &cur = *lvals[i].ref;
				if (cur.isConst || lvals[i].isConstView)
					throw ConstError("Cannot assign to const variable", s->line,
						s->col);
				if (cur.ref && cur.ref->typeLocked &&
					 cur.type != rvals[i].type) {
					throw TypeError("Cannot change type of locked variable",
						s->line, s->col);
				}
				if (lvals[i].isRefTarget)
					*lvals[i].ref = rvals[i];
				else if (cur.ref && cur.type == rvals[i].type) {
					if (cur.type == ValueType::STRING)
						static_cast<StringObject *>(cur.ref.get())->value =
							rvals[i].asString();
					else
						cur = rvals[i];
				} else
					cur = rvals[i];
			}
			return Value::None();
		}
		default: {
			return Value::None();
		}
		}
		return Value::None();
	}

Value Interpreter::call(Value callee, const vector<CallArg> &args,
		const vector<CopyMode> &modes, int line, int col) {
		auto error = [&](const string &msg, const string &type = "RuntimeError") {
			if (type == "TypeError")
				throw TypeError(msg, line, col);
			if (type == "ValueError")
				throw ValueError(msg, line, col);
			if (type == "ArgumentError")
				throw ArgumentError(msg, line, col);
			if (type == "NameError")
				throw NameError(msg, line, col);
			throw RuntimeError(msg, line, col);
		};
		// ------------------ RESOLVE CALLEE OBJECT ------------------
		FunctionObject *lambdaObj = nullptr;
		NativeFunctionObject *nativeObj = nullptr;
		if (callee.type == ValueType::FUNCTION) {
			lambdaObj = static_cast<FunctionObject *>(callee.ref.get());
		} else if (callee.type == ValueType::NATIVE_FUNCTION) {
			nativeObj = static_cast<NativeFunctionObject *>(callee.ref.get());
		} else if (callee.type == ValueType::OVERLOAD) {
			auto *ov = static_cast<OverloadObject *>(callee.ref.get());
			bool found = false;
			for (int i = ov->overloads.size() - 1; i >= 0; i--) {
				Value candVal = ov->overloads[i];
				if (candVal.type != ValueType::FUNCTION)
					continue;
				auto *candidate = static_cast<FunctionObject *>(candVal.ref.get());
				size_t minArgs = 0;
				bool isVariadic = false;
				for (const auto &p : candidate->params) {
					if (p.isVariadic || p.isKwargs)
						isVariadic = true;
					else if (p.defaultValue == nullptr)
						minArgs++;
				}
				if (args.size() < minArgs && !isVariadic)
					continue;
				if (!isVariadic && args.size() > candidate->params.size())
					continue;
				bool typesMatch = true;
				size_t paramIdx = 0;
				for (size_t argIdx = 0; argIdx < args.size(); argIdx++) {
					if (paramIdx >= candidate->params.size()) {
						if (!isVariadic) {
							typesMatch = false;
							break;
						}
						break;
					}
					const ParamSpec &p = candidate->params[paramIdx];
					if (p.isVariadic || p.isKwargs)
						break;
					Value argVal = args[argIdx].value;
					if (p.type != ValueType::NOTYPE && argVal.type != p.type) {
						if (!(p.type == ValueType::FLOAT &&
								 argVal.type == ValueType::INT)) {
							typesMatch = false;
							break;
						}
					}
					paramIdx++;
				}
				if (typesMatch) {
					lambdaObj = candidate;
					found = true;
					break;
				}
			}
			if (!found)
				error("No matching overload found with provided arguments",
					"TypeError");
		} else {
			error("Object is not callable", "TypeError");
		}
		if (nativeObj) {
			vector<Value> argValues;
			for (const auto &ca : args)
				argValues.push_back(ca.value);
			return nativeObj->func(argValues, line, col);
		}
		if (!lambdaObj)
			error("Critical: Lambda Object is null", "RuntimeError");
		vector<Value> cacheKey;
		if (lambdaObj->isCached) {
			for (const auto &arg : args)
				cacheKey.push_back(arg.value);
			if (lambdaObj->cache.count(cacheKey))
				return lambdaObj->cache[cacheKey];
		}
		std::shared_ptr<Env> previous = env;
		std::shared_ptr<Env> newEnv = std::make_shared<Env>();
		newEnv->parent = lambdaObj->closure ? lambdaObj->closure : previous;
		env = newEnv;
		size_t argIndex = 0;
		for (size_t i = 0; i < lambdaObj->params.size(); i++) {
			const ParamSpec &p = lambdaObj->params[i];
			Value argVal = Value::None();
			if (p.isKwargs) {
				std::unordered_map<Value, Value, ValueHash, ValueEqual> dictItems;
				while (argIndex < args.size()) {
					Value v = args[argIndex].value;
					if (v.type != ValueType::PAIRED)
						throw RuntimeError(
							"Positional argument found after keyword arguments.",
							line, col);
					auto *pairObj = static_cast<PairedObject *>(v.ref.get());
					for (const auto &entry : pairObj->pairs) {
						Value key = entry.first;
						Value val = entry.second;
						if (p.mode == CopyMode::DEEP) {
							if (key.type == ValueType::LIST ||
								 key.type == ValueType::DICT)
								key = deepCopy(key);
							val = deepCopy(val);
						}
						dictItems[key] = val;
					}
					argIndex++;
				}
				Value dictVal = Value::Dict(dictItems, p.isConst);
				env->set(p.name, dictVal, false, p.isConst);
			} else if (p.isVariadic) {
				vector<Value> tupleItems;
				while (argIndex < args.size()) {
					if (args[argIndex].value.type == ValueType::PAIRED)
						break;
					Value v = args[argIndex].value;
					if (p.mode == CopyMode::DEEP)
						v = deepCopy(v);
					tupleItems.push_back(v);
					argIndex++;
				}
				env->set(p.name, Value::Tuple(tupleItems), false, true);
			} else {
				bool foundValue = false;
				if (argIndex < args.size() &&
					 args[argIndex].value.type != ValueType::PAIRED) {
					if (args[argIndex].value.type == ValueType::OMIT_MARKER) {
						if (p.defaultChunk != nullptr) {
							argVal = eval(p.defaultValue);
							foundValue = true;
						} else {
							env = previous;
							error("Argument '" + p.name +
										"' cannot be omitted (no default value).",
								"ArgumentError");
						}
					} else {
						argVal = args[argIndex].value;
						if (p.mode == CopyMode::DEEP)
							argVal = deepCopy(argVal);
						foundValue = true;
					}
					argIndex++;
				}
				if (!foundValue) {
					if (p.defaultChunk != nullptr)
						argVal = eval(p.defaultValue);
					else {
						env = previous;
						error("Missing required argument '" + p.name + "'",
							"ArgumentError");
					}
				}
				if (p.type != ValueType::NOTYPE && argVal.type != p.type) {
					if (p.type == ValueType::FLOAT && argVal.type == ValueType::INT)
						argVal = Value::Float((double)argVal.asInt());
					else {
						env = previous;
						error("Type mismatch for '" + p.name + "'", "TypeError");
					}
				}
				env->set(p.name, argVal, false, p.isConst);
			}
		}
		bool prevReturning = returning;
		Value prevReturnValue = returnValue;
		returning = false;
		for (auto stmt : lambdaObj->body) {
			exec(stmt);
			if (returning)
				break;
		}
		bool didReturn = returning;
		Value ret = returnValue;
		returning = prevReturning;
		returnValue = prevReturnValue;
		env = previous;
		auto generateDefault = [&]() -> Value {
			vector<CallArg> defArgs;
			vector<CopyMode> defModes;
			for (auto *e : lambdaObj->defaultRetArgs) {
				defArgs.push_back({eval(e)});
				defModes.push_back(CopyMode::SHALLOW);
			}
			string typeName = "";
			switch (lambdaObj->returnType) {
			case ValueType::INT:
				typeName = "int";
				break;
			case ValueType::FLOAT:
				typeName = "float";
				break;
			case ValueType::STRING:
				typeName = "string";
				break;
			case ValueType::BOOL:
				typeName = "bool";
				break;
			case ValueType::LIST:
				typeName = "list";
				break;
			case ValueType::DICT:
				typeName = "dict";
				break;
			case ValueType::SET:
				typeName = "set";
				break;
			case ValueType::TUPLE:
				typeName = "tuple";
				break;
			case ValueType::RANGE:
				typeName = "range";
				break;
			case ValueType::VECTOR:
				typeName = "vector";
				break;
			default:
				break;
			}
			if (!typeName.empty()) {
				if (env->exists(typeName)) {
					return call(env->get(typeName), defArgs, defModes, line, col);
				}
			}
			return Value::None();
		};
		Value finalResult;
		if (didReturn) {
			if (ret.type == ValueType::NOTYPE) {
				if (!lambdaObj->defaultRetArgs.empty())
					finalResult = generateDefault();
				else if (lambdaObj->returnType != ValueType::NOTYPE)
					finalResult = defaultOf(lambdaObj->returnType);
				else
					finalResult = Value::NoType();
			} else if (lambdaObj->returnType != ValueType::NOTYPE &&
						  ret.type != lambdaObj->returnType) {
				if (lambdaObj->returnType == ValueType::FLOAT &&
					 ret.type == ValueType::INT)
					finalResult = Value::Float((double)ret.asInt());
				else if (lambdaObj->returnType == ValueType::INT &&
							ret.type == ValueType::FLOAT)
					finalResult = Value::Int((long long)ret.fVal);
				else
					error("Return type mismatch", "TypeError");
			} else
				finalResult = ret;
		} else {
			if (!lambdaObj->defaultRetArgs.empty())
				finalResult = generateDefault();
			else
				finalResult = (lambdaObj->returnType != ValueType::NOTYPE)
									  ? defaultOf(lambdaObj->returnType)
									  : Value::NoType();
		}
		if (lambdaObj->returnsConst)
			finalResult.isConst = true;
		if (lambdaObj->isCached)
			lambdaObj->cache[cacheKey] = finalResult;
		return finalResult;
	}

void Interpreter::interpret(const vector<Stmt *> &statements) {
		try {
			for (Stmt *stmt : statements) {
				if (!isGlobalAllowed(stmt))
					throw RuntimeError(
						"Executable code must be inside a 'main' function.",
						stmt->line, stmt->col);
			}

			for (Stmt *stmt : statements)
				exec(stmt);
			if (env->exists("main")) {
				Value mainVal = env->get("main");
				if (mainVal.type != ValueType::FUNCTION)
					throw RuntimeError("'main' is defined but is not a function.", 0,
						0);
				auto *func = static_cast<FunctionObject *>(mainVal.ref.get());
				if (func->params.size() > 0) {
					throw RuntimeError("'main' function cannot accept arguments.", 0,
						0);
				}
				std::shared_ptr<Env> globalEnv = this->env;
				this->env = std::make_shared<Env>();
				this->env->parent = func->closure;
				try {
					// Execute Main Body
					for (Stmt *bodyStmt : func->body) {
						exec(bodyStmt);
						if (returning) {
							returning = false;
							break;
						}
					}
				} catch (...) {
					this->env = globalEnv;
					throw;
				}
				this->env = globalEnv;
			} else
				throw RuntimeError(
					"No entry point found. Define a 'main' function.", 0, 0);
		} catch (...) {
			throw;
		}
	}

bool Interpreter::canOptimizeScope(const vector<Stmt *> &body) {
		for (auto *st : body) {
			if (st->type == StmtType::LET || st->type == StmtType::MULTI_LET ||
				 st->type == StmtType::FUNC || st->type == StmtType::IMPORT ||
				 st->type == StmtType::MULTI_ASSIGN) {
				return false;
			}
		}
		return true;
	}

bool Interpreter::isGlobalAllowed(Stmt *s) {
		if (dynamic_cast<LetStmt *>(s))
			return true;
		if (dynamic_cast<FuncStmt *>(s))
			return true;
		if (dynamic_cast<ImportStmt *>(s))
			return true;
		if (dynamic_cast<MultiLetStmt *>(s))
			return true;
		if (dynamic_cast<MultiAssignStmt *>(s))
			return true;
		return false;
	}

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

void Interpreter::resolveImport(std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
	if (this->modules.count(lib)) {
		this->modules[lib](targetEnv, sym);
		return;
	}

#ifdef _WIN32
	const std::string prefix = "y-", suffix = ".dll";
#elif defined(__APPLE__)
	const std::string prefix = "liby-", suffix = ".dylib";
#else
	const std::string prefix = "liby-", suffix = ".so";
#endif
	// `import Raylib` must find liby-raylib.so: library files are installed lowercase
	std::string lower = lib;
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) { return std::tolower(ch); });
	const char *home = getenv("HOME") ? getenv("HOME") : getenv("USERPROFILE");
	std::vector<std::string> searchPaths;
	for (const std::string &name : {lib, lower}) {
		std::string file = prefix + name + suffix;
		searchPaths.push_back("./" + file);
		searchPaths.push_back("build/" + file);
		if (home)
			searchPaths.push_back(std::string(home) + "/.yenv/lib/" + file);
	}

	void *handle = nullptr;
	for (const auto &path : searchPaths) {
#ifdef _WIN32
		handle = (void *)LoadLibraryA(path.c_str());
#else
		handle = dlopen(path.c_str(), RTLD_LAZY);
#endif
		if (handle)
			break;
	}

	if (!handle) {
		throw ImportError("Unknown module '" + lib + "'", 0, 0);
	}

	typedef void (*LoadModuleFunc)(std::shared_ptr<Env>, const std::vector<std::string> &);
#ifdef _WIN32
	LoadModuleFunc loadFunc = (LoadModuleFunc)GetProcAddress((HMODULE)handle, "YlangLoadModule");
#else
	LoadModuleFunc loadFunc = (LoadModuleFunc)dlsym(handle, "YlangLoadModule");
#endif

	if (!loadFunc) {
		throw ImportError("Invalid module library (missing YlangLoadModule entry point): " + lib, 0, 0);
	}

	loadFunc(targetEnv, sym);
}

Value Interpreter::nativePrint(const vector<Value> &args, int l, int c) {
	auto runMagic = [&](const Value &v,
							 const std::string &method) -> std::string {
		if (v.type != ValueType::INSTANCE)
			return "";
		auto *instance = static_cast<InstanceObject *>(v.ref.get());
		auto *cls = instance->klass;
		ClassObject *methodOwner = nullptr;
		for (auto *ancestor : cls->mro) {
			if (ancestor->methods.count(method)) {
				methodOwner = ancestor;
				break;
			}
		}
		if (methodOwner) {
			VM tempVM;
			tempVM.globals = this->env;
			tempVM.methodResolver = [this](MethodCallExpr *m) {
				return this->Resolve_methods(m);
			};
			tempVM.importResolver = [this](std::string lib,
												std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
				this->resolveImport(lib, sym, targetEnv);
			};
			Chunk tempChunk;
			int selfIdx = tempChunk.addConstant(v);
			tempChunk.write(OpCode::OP_CONSTANT, l, c);
			tempChunk.write((uint8_t)selfIdx, l, c);
			Value objVal;
			objVal.type = ValueType::CLASS;
			objVal.ref =
				std::shared_ptr<HeapObject>(methodOwner, [](HeapObject *) {});
			int objIdx = tempChunk.addConstant(objVal);
			tempChunk.write(OpCode::OP_CONSTANT, l, c);
			tempChunk.write((uint8_t)objIdx, l, c);
			int methIdx = tempChunk.addConstant(methodOwner->methods[method].func);
			tempChunk.write(OpCode::OP_CONSTANT, l, c);
			tempChunk.write((uint8_t)methIdx, l, c);
			tempChunk.write(OpCode::OP_CALL, l, c);
			tempChunk.write((uint8_t)2, l, c);
			tempChunk.write(OpCode::OP_RETURN, l, c);
			try {
				tempVM.run(tempChunk);
				if (!tempVM.stack.empty()) {
					Value res = tempVM.stack.back();
					return (res.type == ValueType::STRING ? res.asString()
																	  : valueToString(res));
				}
			} catch (...) {
				return "";
			}
		}
		return "";
	};
	Inspector containerInspector = [&](const Value &v) -> std::string {
		std::string res = runMagic(v, "__inspect__");
		if (res.empty())
			res = runMagic(v, "__display__");
		return res;
	};
	for (size_t i = 0; i < args.size(); i++) {
		std::unordered_set<const HeapObject *> seen;
		std::string output = runMagic(args[i], "__display__");
		if (!output.empty())
			std::cout << output;
		else
			printValue(args[i], seen, false, &containerInspector);
		if (i < args.size() - 1)
			std::cout << " ";
	}
	std::cout << "\n";
	return Value::None();
}
