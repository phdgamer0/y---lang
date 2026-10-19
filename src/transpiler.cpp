#include "ylang/transpiler.hpp"
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

		bool hasReturn = false;
		for (Stmt *st : func->body) {
			if (st && st->type == StmtType::RETURN) {
				hasReturn = true;
				break;
			}
		}

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
			std::string pType = getCppType(p.type, p.mode, p.isConst, false, p.mode == CopyMode::REF);
			info.paramTypeStrs.push_back(pType);
			paramsStr += pType + " " + p.name;
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
			return v->name;
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
			std::string argsStr = "";
			for (size_t i = 0; i < call->args.size(); i++) {
				std::string paramType = (i < info.paramTypeStrs.size()) ? info.paramTypeStrs[i] : "Value";
				CopyMode mode = (i < call->modes.size()) ? call->modes[i] : CopyMode::SHALLOW;
				Expr *argExpr = call->args[i];

				if (paramType == "long long&" || paramType == "const long long&") {
					if (auto v = dynamic_cast<VarExpr *>(argExpr)) {
						if (lookupNativeVar(v->name)) {
							argsStr += v->name;
						} else {
							argsStr += "__y_env_int_ref(current_env, \"" + v->name + "\")";
						}
					} else {
						argsStr += "__y_to_int(" + transpileExpr(argExpr) + ")";
					}
				} else if (paramType == "double&" || paramType == "const double&") {
					if (auto v = dynamic_cast<VarExpr *>(argExpr)) {
						if (lookupNativeVar(v->name)) {
							argsStr += v->name;
						} else {
							argsStr += "__y_env_float_ref(current_env, \"" + v->name + "\")";
						}
					} else {
						argsStr += "__y_to_float(" + transpileExpr(argExpr) + ")";
					}
				} else if (paramType == "bool&" || paramType == "const bool&") {
					if (auto v = dynamic_cast<VarExpr *>(argExpr)) {
						if (lookupNativeVar(v->name)) {
							argsStr += v->name;
						} else {
							argsStr += "__y_env_bool_ref(current_env, \"" + v->name + "\")";
						}
					} else {
						argsStr += "__y_to_bool(" + transpileExpr(argExpr) + ")";
					}
				} else if (paramType == "Value&" || paramType == "const Value&") {
					if (auto v = dynamic_cast<VarExpr *>(argExpr)) {
						if (lookupNativeVar(v->name)) {
							argsStr += v->name;
						} else {
							argsStr += "__y_env_val_ref(current_env, \"" + v->name + "\")";
						}
					} else {
						argsStr += "__y_val(" + transpileExpr(argExpr) + ")";
					}
				} else if (paramType == "long long" || paramType == "const long long") {
					argsStr += "__y_to_int(" + transpileExpr(argExpr) + ")";
				} else if (paramType == "double" || paramType == "const double") {
					argsStr += "__y_to_float(" + transpileExpr(argExpr) + ")";
				} else if (paramType == "bool" || paramType == "const bool") {
					argsStr += "__y_to_bool(" + transpileExpr(argExpr) + ")";
				} else if (paramType == "std::string" || paramType == "const std::string&") {
					argsStr += "__y_to_str(" + transpileExpr(argExpr) + ")";
				} else {
					if (mode == CopyMode::REF) {
						if (auto v = dynamic_cast<VarExpr *>(argExpr)) {
							argsStr += "__y_make_ref(current_env, \"" + v->name + "\")";
						} else {
							argsStr += "__y_val(" + transpileExpr(argExpr) + ")";
						}
					} else if (mode == CopyMode::DEEP) {
						argsStr += "deepCopy(__y_val(" + transpileExpr(call->args[i]) + "))";
					} else {
						argsStr += "__y_val(" + transpileExpr(call->args[i]) + ")";
					}
				}
				if (i < call->args.size() - 1)
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
					argsStr += "Value::FromExisting(std::make_shared<ListRefObject>(std::static_pointer_cast<ListObject>(" + transpileExpr(idx->base) + ".ref), " + transpileExpr(idx->index) + ".asInt()), ValueType::REFERENCE)";
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
				lambdaStr += "        for (size_t _i = " + std::to_string(pos_idx) + "; _i < _pos_count; _i++) {\n";
				lambdaStr += "            _rest_list.push_back(_args[_i]);\n";
				lambdaStr += "        }\n";
				lambdaStr += "        current_env->set(\"" + p.name + "\", Value::Tuple(_rest_list), false, true);\n";
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
		currentFuncReturnsConst = lambda->returnsConst;
		currentFuncReturnsRef = lambda->returnsRef;

		// Body
		string saved_output = output;
		output = ""; // Temporary redirect

		for (auto stmt : lambda->body) {
			transpileStmt(stmt);
		}
		output += "        return Value::None();\n";

		string bodyOutput = output;
		output = saved_output; // Restore

		constScopes.pop_back();
		currentFuncReturnsConst = oldReturnsConst;
		currentFuncReturnsRef = oldReturnsRef;

		lambdaStr += bodyOutput;
		lambdaStr += "    })";
		return lambdaStr;
	}
	case ExprType::METHOD_CALL: {
		auto mc = static_cast<MethodCallExpr *>(e);
		string objStr = transpileExpr(mc->object);
		string argsStr = "";
		for (size_t i = 0; i < mc->args.size(); i++) {
			argsStr += transpileExpr(mc->args[i]);
			if (i < mc->args.size() - 1)
				argsStr += ", ";
		}
		return "__y_method_call(\"" + mc->method + "\", " + objStr + ", {" + argsStr + "})";
	}
	case ExprType::INDEX: {
		auto idx = static_cast<IndexExpr *>(e);
		string baseStr = transpileExpr(idx->base);
		string idxStr = transpileExpr(idx->index);
		return "__y_get_index(" + baseStr + ", " + idxStr + ")";
	}
	case ExprType::LIST: {
		auto lst = static_cast<ListExpr *>(e);
		string s = "Value::List(std::vector<Value>{";
		for (size_t i = 0; i < lst->elements.size(); i++) {
			s += transpileExpr(lst->elements[i]);
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
			if (lookupNativeVar(varExpr->name)) {
				if (incDec->isPrefix) {
					return "([&]() { " + std::string(incDec->isIncrement ? "++" : "--") + varExpr->name + "; return " + varExpr->name + "; })()";
				} else {
					return "([&]() { return " + varExpr->name + std::string(incDec->isIncrement ? "++" : "--") + "; })()";
				}
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
			s += "{" + transpileExpr(dict->items[i].first) + ", " + transpileExpr(dict->items[i].second) + "}";
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
				return "Value::FromExisting(std::make_shared<ListRefObject>(std::static_pointer_cast<ListObject>(" + transpileExpr(idx->base) + ".ref), " + transpileExpr(idx->index) + ".asInt()), ValueType::REFERENCE)";
			}
		} else if (o->mode == CopyMode::DEEP) {
			return "deepCopy(__y_val(" + transpileExpr(o->expr) + "))";
		}
		return transpileExpr(o->expr);
	}
	case ExprType::BINARY: {
		auto bin = static_cast<BinExpr *>(e);
		if (!bin->left && bin->op == TokenType::NOT) {
			return "Value::Bool(!__y_is_truthy(" + transpileExpr(bin->right) + "))";
		}
		if (!bin->left && bin->op == TokenType::BITWISE_NOT) {
			return "EvaluateConstBinary(TokenType::BITWISE_NOT, Value::Int(0), " + transpileExpr(bin->right) + ")";
		}
		string left = transpileExpr(bin->left);
		string right = transpileExpr(bin->right);
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
			return "Value::Bool((" + left + ").strictEquals(" + right + "))";
		case TokenType::STRICT_NEQ:
			return "Value::Bool(!(" + left + ").strictEquals(" + right + "))";
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
			return "EvaluateConstBinary(TokenType::FLOOR_DIV, " + left + ", " + right + ")";
		case TokenType::MOD:
			return "EvaluateConstBinary(TokenType::MOD, " + left + ", " + right + ")";
		case TokenType::POW:
			return "EvaluateConstBinary(TokenType::POW, " + left + ", " + right + ")";
		case TokenType::BITWISE_AND:
			return "EvaluateConstBinary(TokenType::BITWISE_AND, " + left + ", " + right + ")";
		case TokenType::BITWISE_OR:
			return "EvaluateConstBinary(TokenType::BITWISE_OR, " + left + ", " + right + ")";
		case TokenType::BITWISE_XOR:
			return "EvaluateConstBinary(TokenType::BITWISE_XOR, " + left + ", " + right + ")";
		case TokenType::BITWISE_SHL:
			return "EvaluateConstBinary(TokenType::BITWISE_SHL, " + left + ", " + right + ")";
		case TokenType::BITWISE_SHR:
			return "EvaluateConstBinary(TokenType::BITWISE_SHR, " + left + ", " + right + ")";
		default:
			break;
		}
		return "EvaluateConstBinary(TokenType::PLUS, " + left + ", " + right + ")";
	}
	case ExprType::TUPLE: {
		auto tup = static_cast<TupleExpr *>(e);
		string s = "Value::Tuple(std::vector<Value>{";
		for (size_t i = 0; i < tup->elements.size(); i++) {
			s += transpileExpr(tup->elements[i]);
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
			s += "_res += valueToString(" + transpileExpr(part) + "); ";
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

	if (e->type == ExprType::BINARY) {
		auto bin = static_cast<BinExpr *>(e);
		string left = transpileExpr(bin->left);
		string right = transpileExpr(bin->right);
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
			return "(" + left + ").strictEquals(" + right + ")";
		case TokenType::STRICT_NEQ:
			return "!(" + left + ").strictEquals(" + right + ")";
		case TokenType::AND:
			return "(" + transpileCond(bin->left) + " && " + transpileCond(bin->right) + ")";
		case TokenType::OR:
			return "(" + transpileCond(bin->left) + " || " + transpileCond(bin->right) + ")";
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
		if (let->isConst && let->value) {
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
				std::string valStr = transpileExpr(let->value);
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
					break;
				}
			}
			string valStr = let->value ? transpileExpr(let->value) : "Value::NoType()";
			output += "    current_env->set(\"" + let->name + "\", __y_val(" + valStr + "), " +
						 (let->isLocked ? "true" : "false") + ", " +
						 (let->isConst ? "true" : "false") + ");\n";
		}
		break;
	}
	case StmtType::MULTI_LET: {
		auto ml = static_cast<MultiLetStmt *>(s);
		for (size_t i = 0; i < ml->names.size(); i++) {
			std::string name = ml->names[i];
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
					output += "    " + v->name + " = " + valStr + ";\n";
				} else if (assign->op == TokenType::PLUS_EQ) {
					output += "    " + v->name + " += " + valStr + ";\n";
				} else if (assign->op == TokenType::MINUS_EQ) {
					output += "    " + v->name + " -= " + valStr + ";\n";
				} else if (assign->op == TokenType::STAR_EQ) {
					output += "    " + v->name + " *= " + valStr + ";\n";
				} else if (assign->op == TokenType::DIV_EQ) {
					output += "    " + v->name + " /= " + valStr + ";\n";
				} else {
					output += "    " + v->name + " = " + valStr + ";\n";
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
			string idxStr = transpileExpr(idx->index);
			string valStr = transpileExpr(assign->value);
			output += "    __y_set_index(" + baseStr + ", " + idxStr + ", __y_val(" + valStr + "));\n";
		}
		break;
	}
	case StmtType::IF: {
		auto ifStmt = static_cast<IfStmt *>(s);
		output += "    if (" + transpileCond(ifStmt->condition) + ") {\n";
		output += "        EnvRestorer env_restorer(current_env);\n";
		constScopes.push_back({});
		for (auto stmt : ifStmt->body)
			transpileStmt(stmt);
		constScopes.pop_back();
		output += "    }\n";
		for (const auto &elif : ifStmt->elifs) {
			output += "    else if (" + transpileCond(elif.first) + ") {\n";
			output += "        EnvRestorer env_restorer(current_env);\n";
			constScopes.push_back({});
			for (auto stmt : elif.second)
				transpileStmt(stmt);
			constScopes.pop_back();
			output += "    }\n";
		}
		if (!ifStmt->elseBody.empty()) {
			output += "    else {\n";
			output += "        EnvRestorer env_restorer(current_env);\n";
			constScopes.push_back({});
			for (auto stmt : ifStmt->elseBody)
				transpileStmt(stmt);
			constScopes.pop_back();
			output += "    }\n";
		}
		break;
	}
	case StmtType::WHILE: {
		auto wStmt = static_cast<WhileStmt *>(s);
		int loopId = loopIdCounter++;
		loopStack.push_back({loopId, false});
		output += "    {\n";
		output += "        EnvRestorer env_restorer_loop(current_env);\n";
		output += "        while (" + transpileCond(wStmt->condition) + ") {\n";
		constScopes.push_back({});
		for (auto stmt : wStmt->body)
			transpileStmt(stmt);
		constScopes.pop_back();
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
		output += "        EnvRestorer env_restorer_loop(current_env);\n";
		output += "        do {\n";
		constScopes.push_back({});
		for (auto stmt : dw->body)
			transpileStmt(stmt);
		constScopes.pop_back();
		output += "        } while (" + transpileCond(dw->condition) + ");\n";
		output += "    }\n";
		loopStack.pop_back();
		break;
	}
	case StmtType::FOR: {
		auto fStmt = static_cast<ForStmt *>(s);
		int loopId = loopIdCounter++;
		loopStack.push_back({loopId, false});
		output += "    {\n";
		output += "        EnvRestorer env_restorer_outer(current_env);\n";
		constScopes.push_back({});
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
		constScopes.push_back({});
		for (auto stmt : fStmt->body)
			transpileStmt(stmt);
		constScopes.pop_back();
		output += "        }\n";
		constScopes.pop_back();
		output += "    }\n";
		loopStack.pop_back();
		break;
	}
	case StmtType::FOR_EACH: {
		auto fe = static_cast<ForEachStmt *>(s);
		int loopId = loopIdCounter++;
		loopStack.push_back({loopId, true});
		output += "    {\n";
		output += "        EnvRestorer env_restorer_outer(current_env);\n";
		output += "        long long _iter_idx_" + std::to_string(loopId) + " = 0;\n";
		output += "        std::vector<std::vector<Value>> _streams;\n";
		bool isDictUnpack = (fe->collections.size() == 1 && fe->loopVars.size() == 2);
		if (isDictUnpack) {
			output += "        Value _dict_val = " + transpileExpr(fe->collections[0]) + ";\n";
			output += "        std::vector<Value> _keys; std::vector<Value> _vals;\n";
			output += "        if (_dict_val.type == ValueType::DICT) {\n";
			output += "            auto* _d = static_cast<DictObject*>(_dict_val.ref.get());\n";
			output += "            for (auto& _p : _d->items) { _keys.push_back(_p.first); _vals.push_back(_p.second); }\n";
			output += "        }\n";
			output += "        _streams.push_back(_keys);\n";
			output += "        _streams.push_back(_vals);\n";
		} else {
			for (auto col : fe->collections) {
				output += "        _streams.push_back(__y_to_vector(" + transpileExpr(col) + "));\n";
			}
		}
		output += "        size_t _min_len_" + std::to_string(loopId) + " = _streams.empty() ? 0 : _streams[0].size();\n";
		output += "        for (size_t _si = 1; _si < _streams.size(); _si++) { if (_streams[_si].size() < _min_len_" + std::to_string(loopId) + ") _min_len_" + std::to_string(loopId) + " = _streams[_si].size(); }\n";
		output += "        for (; (size_t)_iter_idx_" + std::to_string(loopId) + " < _min_len_" + std::to_string(loopId) + "; _iter_idx_" + std::to_string(loopId) + "++) {\n";
		constScopes.push_back({});
		for (size_t i = 0; i < fe->loopVars.size(); i++) {
			output += "            current_env->set(\"" + fe->loopVars[i] + "\", _streams[" + std::to_string(i) + "][_iter_idx_" + std::to_string(loopId) + "], false, false);\n";
		}
		for (auto stmt : fe->body)
			transpileStmt(stmt);
		constScopes.pop_back();
		output += "        }\n";
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
		output += "    _iter_idx_" + std::to_string(loopId) + " += (" + transpileExpr(sk->count) + ").asInt();\n";
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

		string valStr;
		if (currentFuncReturnsRef && ret->value) {
			if (auto v = dynamic_cast<VarExpr *>(ret->value)) {
				if (lookupNativeVar(v->name)) {
					valStr = v->name;
				} else {
					valStr = "__y_make_ref(current_env, \"" + v->name + "\")";
				}
			} else if (auto idx = dynamic_cast<IndexExpr *>(ret->value)) {
				valStr = "Value::FromExisting(std::make_shared<ListRefObject>(std::static_pointer_cast<ListObject>(" + transpileExpr(idx->base) + ".ref), " + transpileExpr(idx->index) + ".asInt()), ValueType::REFERENCE)";
			} else {
				valStr = transpileExpr(ret->value);
			}
		} else {
			valStr = ret->value ? transpileExpr(ret->value) : (currentFuncNativeReturnType == "Value" ? "Value::None()" : "");
		}

		if (currentFuncNativeReturnType == "long long") {
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
				output += "    { Value _retVal = " + valStr + "; _retVal.isConst = true; return _retVal; }\n";
			} else {
				output += "    return " + valStr + ";\n";
			}
		}
		break;
	}
	case StmtType::SWITCH: {
		auto sw = static_cast<SwitchStmt *>(s);
		string targetStr = transpileExpr(sw->target);
		output += "    switch(0) { case 0: {\n";
		output += "        Value __switch_target = " + targetStr + ";\n";
		bool firstCase = true;
		for (const auto &c : sw->cases) {
			string caseValStr = transpileExpr(c.value);
			if (firstCase) {
				output += "        if (__switch_target.strictEquals(" + caseValStr + ")) {\n";
				firstCase = false;
			} else {
				output += "        else if (__switch_target.strictEquals(" + caseValStr + ")) {\n";
			}
			output += "            EnvRestorer env_restorer(current_env);\n";
			for (auto stmt : c.body)
				transpileStmt(stmt);
			output += "        }\n";
		}
		if (!sw->defaultBody.empty()) {
			if (firstCase) {
				output += "        {\n";
			} else {
				output += "        else {\n";
			}
			output += "            EnvRestorer env_restorer(current_env);\n";
			for (auto stmt : sw->defaultBody)
				transpileStmt(stmt);
			output += "        }\n";
		}
		output += "    } break; }\n";
		break;
	}
	case StmtType::ASSERT: {
		auto astmt = static_cast<AssertStmt *>(s);
		output += "    if (!" + transpileCond(astmt->condition) + ") {\n";
		if (astmt->message) {
			output += "        throw AssertionError(" + transpileExpr(astmt->message) + ".asString(), 0, 0);\n";
		} else {
			output += "        throw AssertionError(\"Assertion failed\", 0, 0);\n";
		}
		output += "    }\n";
		break;
	}
	case StmtType::THROW: {
		auto tstmt = static_cast<ThrowStmt *>(s);
		output += "    throw " + tstmt->errorType + "(" + transpileExpr(tstmt->message) + ".asString(), 0, 0);\n";
		break;
	}
	case StmtType::TRY: {
		auto tstmt = static_cast<TryStmt *>(s);
		output += "    try {\n";
		output += "        EnvRestorer env_restorer(current_env);\n";
		for (auto stmt : tstmt->tryBlock)
			transpileStmt(stmt);
		output += "    }\n";
		for (const auto &catchBlock : tstmt->catches) {
			// YLang exceptions are C++ exceptions (e.g. RuntimeError)
			// Catch block types are string names of exceptions
			// We will catch std::exception for simplicity and check type, or generate specific catch blocks
			// For simplicity in C++, we can catch the base class `YError` or standard exceptions
			output += "    catch (std::exception& e) {\n";
			output += "        EnvRestorer env_restorer(current_env);\n";
			output += "        current_env->set(\"e\", Value::String(e.what()), false, false);\n";
			for (auto stmt : catchBlock.body)
				transpileStmt(stmt);
			output += "    }\n";
		}
		if (!tstmt->finallyBlock.empty()) {
			output += "    // NOTE: C++ doesn't have finally block natively for try/catch, using RAII or scope exit in future.\n";
			output += "    {\n";
			output += "        EnvRestorer env_restorer(current_env);\n";
			for (auto stmt : tstmt->finallyBlock)
				transpileStmt(stmt);
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
		if (it == userFunctions.end()) break;
		const auto& info = it->second;

		std::string funcDef = "";
		std::string paramsStr = "";
		for (size_t i = 0; i < info.params.size(); i++) {
			paramsStr += info.paramTypeStrs[i] + " " + info.params[i].name;
			if (i < info.params.size() - 1) paramsStr += ", ";
		}

		funcDef += "inline auto " + info.cppName + "(" + paramsStr + ") -> " + info.returnTypeStr + " {\n";
		funcDef += "    auto current_env = std::make_shared<Env>();\n";
		funcDef += "    current_env->parent = y_transpile_globals;\n";

		nativeScopes.push_back({});
		constScopes.push_back({});

		for (size_t i = 0; i < info.params.size(); i++) {
			const auto& p = info.params[i];
			const std::string& pType = info.paramTypeStrs[i];

			if (pType == "long long" || pType == "const long long" ||
			    pType == "long long&" || pType == "const long long&" ||
			    pType == "double" || pType == "const double" ||
			    pType == "double&" || pType == "const double&" ||
			    pType == "bool" || pType == "const bool" ||
			    pType == "bool&" || pType == "const bool&" ||
			    pType == "std::string" || pType == "const std::string&" ||
			    pType == "std::string&") {
				nativeScopes.back()[p.name] = { pType, (p.mode == CopyMode::REF), p.isConst };
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
		currentFuncReturnsConst = func->returnsConst;
		currentFuncReturnsRef = func->returnsRef;
		currentFuncNativeReturnType = info.returnTypeStr;

		std::string saved_output = output;
		output = "";
		for (auto stmt : func->body) {
			transpileStmt(stmt);
		}
		if (info.returnTypeStr == "void") {
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

		funcDef += output;
		funcDef += "}\n\n";

		userFunctionDefs += funcDef;

		output = saved_output;
		nativeScopes.pop_back();
		constScopes.pop_back();
		currentFuncReturnsConst = oldReturnsConst;
		currentFuncReturnsRef = oldReturnsRef;
		currentFuncNativeReturnType = oldNativeReturnType;

		// Generate reflection wrapper for globals in main()
		if (func->name != "main") {
			std::string wrapper = "    y_transpile_globals->set(\"" + func->name + "\", Value::Native([](const std::vector<Value>& _args, int _pos_count, int _kw_count) mutable -> Value {\n";
			std::string callArgs = "";
			for (size_t i = 0; i < info.params.size(); i++) {
				const auto& pType = info.paramTypeStrs[i];
				std::string idx = std::to_string(i);
				if (pType == "long long" || pType == "const long long") {
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
				if (i < info.params.size() - 1) callArgs += ", ";
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
	ss << "#include \"ylang/objects.hpp\"\n"
			"#include \"ylang/vm.hpp\"\n"
			"#include \"ylang/ast.hpp\"\n"
			"#include <cmath>\n"
			"\n";

	ss << "inline Value __y_val(const Value& v) { return v; }\n"
			"inline Value __y_val(long long v) { return Value::Int(v); }\n"
			"inline Value __y_val(int v) { return Value::Int((long long)v); }\n"
			"inline Value __y_val(double v) { return Value::Float(v); }\n"
			"inline Value __y_val(bool v) { return Value::Bool(v); }\n"
			"inline Value __y_val(const std::string& v) { return Value::String(v); }\n"
			"inline Value __y_val(const char* v) { return Value::String(std::string(v)); }\n\n"
			"inline Value __y_make_ref(const std::shared_ptr<Env>& env, const std::string& name) {\n"
			"    auto& var = env->lookup(name);\n"
			"    if (var.alias) return Value::Reference(var.alias, nullptr);\n"
			"    if (var.value.type == ValueType::REFERENCE && var.value.get_ptr_safe()) return var.value;\n"
			"    return Value::FromExisting(std::make_shared<VarRefObject>(env, name), ValueType::REFERENCE);\n"
			"}\n\n"
			"inline long long __y_to_int(long long v) { return v; }\n"
			"inline long long __y_to_int(double v) { return (long long)v; }\n"
			"inline long long __y_to_int(bool v) { return v ? 1LL : 0LL; }\n"
			"inline long long __y_to_int(const Value& v) { return v.asInt(); }\n\n"
			"inline double __y_to_float(double v) { return v; }\n"
			"inline double __y_to_float(long long v) { return (double)v; }\n"
			"inline double __y_to_float(const Value& v) { return v.asFloat(); }\n\n"
			"inline bool __y_to_bool(bool v) { return v; }\n"
			"inline bool __y_to_bool(long long v) { return v != 0; }\n"
			"inline bool __y_to_bool(const Value& v) { return v.isTruthy(); }\n\n"
			"inline std::string __y_to_str(const std::string& v) { return v; }\n"
			"inline std::string __y_to_str(const char* v) { return std::string(v); }\n"
			"inline std::string __y_to_str(const Value& v) { return v.asString(); }\n\n"
			"inline long long& __y_env_int_ref(const std::shared_ptr<Env>& env, const std::string& name) {\n"
			"    auto& var = env->lookup(name);\n"
			"    if (var.alias) return var.alias->iVal;\n"
			"    if (var.value.type == ValueType::REFERENCE && var.value.get_ptr_safe()) return var.value.get_ptr_safe()->iVal;\n"
			"    return var.value.iVal;\n"
			"}\n"
			"inline double& __y_env_float_ref(const std::shared_ptr<Env>& env, const std::string& name) {\n"
			"    auto& var = env->lookup(name);\n"
			"    if (var.alias) return var.alias->fVal;\n"
			"    if (var.value.type == ValueType::REFERENCE && var.value.get_ptr_safe()) return var.value.get_ptr_safe()->fVal;\n"
			"    return var.value.fVal;\n"
			"}\n"
			"inline bool& __y_env_bool_ref(const std::shared_ptr<Env>& env, const std::string& name) {\n"
			"    auto& var = env->lookup(name);\n"
			"    if (var.alias) return var.alias->bVal;\n"
			"    if (var.value.type == ValueType::REFERENCE && var.value.get_ptr_safe()) return var.value.get_ptr_safe()->bVal;\n"
			"    return var.value.bVal;\n"
			"}\n"
			"inline Value& __y_env_val_ref(const std::shared_ptr<Env>& env, const std::string& name) {\n"
			"    auto& var = env->lookup(name);\n"
			"    if (var.alias) return *var.alias;\n"
			"    if (var.value.type == ValueType::REFERENCE && var.value.get_ptr_safe()) return *var.value.get_ptr_safe();\n"
			"    return var.value;\n"
			"}\n\n"
			"inline Value* __y_env_ptr(const std::shared_ptr<Env>& env, const std::string& name) {\n"
			"    auto& var = env->lookup(name);\n"
			"    if (var.alias) return var.alias;\n"
			"    if (var.value.type == ValueType::REFERENCE && var.value.get_ptr_safe()) return var.value.get_ptr_safe();\n"
			"    return &var.value;\n"
			"}\n\n"
			"inline bool __y_is_truthy(bool b) { return b; }\n"
			"inline bool __y_is_truthy(long long n) { return n != 0; }\n"
			"inline bool __y_is_truthy(double d) { return d != 0.0; }\n"
			"inline bool __y_is_truthy(const std::string& s) { return !s.empty(); }\n"
			"inline bool __y_is_truthy(const Value& v) { return v.isTruthy(); }\n\n"

			"inline bool __y_lt(const Value& a, const Value& b) {\n"
			"    if (a.type == ValueType::INT && b.type == ValueType::INT) return a.iVal < b.iVal;\n"
			"    if (a.type == ValueType::FLOAT && b.type == ValueType::FLOAT) return a.fVal < b.fVal;\n"
			"    if (a.isNumber() && b.isNumber()) return a.asFloat() < b.asFloat();\n"
			"    return EvaluateConstBinary(TokenType::LT, a, b).isTruthy();\n"
			"}\n"
			"inline bool __y_lt(long long a, long long b) { return a < b; }\n"
			"inline bool __y_lt(double a, double b) { return a < b; }\n"
			"inline bool __y_lt(const Value& a, long long b) {\n"
			"    if (a.type == ValueType::INT) return a.iVal < b;\n"
			"    if (a.isNumber()) return a.asFloat() < (double)b;\n"
			"    return EvaluateConstBinary(TokenType::LT, a, Value::Int(b)).isTruthy();\n"
			"}\n"
			"inline bool __y_lt(long long a, const Value& b) {\n"
			"    if (b.type == ValueType::INT) return a < b.iVal;\n"
			"    if (b.isNumber()) return (double)a < b.asFloat();\n"
			"    return EvaluateConstBinary(TokenType::LT, Value::Int(a), b).isTruthy();\n"
			"}\n\n"

			"inline bool __y_lte(const Value& a, const Value& b) {\n"
			"    if (a.type == ValueType::INT && b.type == ValueType::INT) return a.iVal <= b.iVal;\n"
			"    if (a.type == ValueType::FLOAT && b.type == ValueType::FLOAT) return a.fVal <= b.fVal;\n"
			"    if (a.isNumber() && b.isNumber()) return a.asFloat() <= b.asFloat();\n"
			"    return EvaluateConstBinary(TokenType::LTE, a, b).isTruthy();\n"
			"}\n"
			"inline bool __y_lte(long long a, long long b) { return a <= b; }\n"
			"inline bool __y_lte(double a, double b) { return a <= b; }\n"
			"inline bool __y_lte(const Value& a, long long b) {\n"
			"    if (a.type == ValueType::INT) return a.iVal <= b;\n"
			"    if (a.isNumber()) return a.asFloat() <= (double)b;\n"
			"    return EvaluateConstBinary(TokenType::LTE, a, Value::Int(b)).isTruthy();\n"
			"}\n"
			"inline bool __y_lte(long long a, const Value& b) {\n"
			"    if (b.type == ValueType::INT) return a <= b.iVal;\n"
			"    if (b.isNumber()) return (double)a <= b.asFloat();\n"
			"    return EvaluateConstBinary(TokenType::LTE, Value::Int(a), b).isTruthy();\n"
			"}\n\n"

			"inline bool __y_gt(const Value& a, const Value& b) {\n"
			"    if (a.type == ValueType::INT && b.type == ValueType::INT) return a.iVal > b.iVal;\n"
			"    if (a.type == ValueType::FLOAT && b.type == ValueType::FLOAT) return a.fVal > b.fVal;\n"
			"    if (a.isNumber() && b.isNumber()) return a.asFloat() > b.asFloat();\n"
			"    return EvaluateConstBinary(TokenType::GT, a, b).isTruthy();\n"
			"}\n"
			"inline bool __y_gt(long long a, long long b) { return a > b; }\n"
			"inline bool __y_gt(double a, double b) { return a > b; }\n"
			"inline bool __y_gt(const Value& a, long long b) {\n"
			"    if (a.type == ValueType::INT) return a.iVal > b;\n"
			"    if (a.isNumber()) return a.asFloat() > (double)b;\n"
			"    return EvaluateConstBinary(TokenType::GT, a, Value::Int(b)).isTruthy();\n"
			"}\n"
			"inline bool __y_gt(long long a, const Value& b) {\n"
			"    if (b.type == ValueType::INT) return a > b.iVal;\n"
			"    if (b.isNumber()) return (double)a > b.asFloat();\n"
			"    return EvaluateConstBinary(TokenType::GT, Value::Int(a), b).isTruthy();\n"
			"}\n\n"

			"inline bool __y_gte(const Value& a, const Value& b) {\n"
			"    if (a.type == ValueType::INT && b.type == ValueType::INT) return a.iVal >= b.iVal;\n"
			"    if (a.type == ValueType::FLOAT && b.type == ValueType::FLOAT) return a.fVal >= b.fVal;\n"
			"    if (a.isNumber() && b.isNumber()) return a.asFloat() >= b.asFloat();\n"
			"    return EvaluateConstBinary(TokenType::GTE, a, b).isTruthy();\n"
			"}\n"
			"inline bool __y_gte(long long a, long long b) { return a >= b; }\n"
			"inline bool __y_gte(double a, double b) { return a >= b; }\n"
			"inline bool __y_gte(const Value& a, long long b) {\n"
			"    if (a.type == ValueType::INT) return a.iVal >= b;\n"
			"    if (a.isNumber()) return a.asFloat() >= (double)b;\n"
			"    return EvaluateConstBinary(TokenType::GTE, a, Value::Int(b)).isTruthy();\n"
			"}\n"
			"inline bool __y_gte(long long a, const Value& b) {\n"
			"    if (b.type == ValueType::INT) return a >= b.iVal;\n"
			"    if (b.isNumber()) return (double)a >= b.asFloat();\n"
			"    return EvaluateConstBinary(TokenType::GTE, Value::Int(a), b).isTruthy();\n"
			"}\n\n"

			"inline bool __y_eq(const Value& a, const Value& b) {\n"
			"    if (a.type == b.type) return a.strictEquals(b);\n"
			"    if (a.isNumber() && b.isNumber()) return a.asFloat() == b.asFloat();\n"
			"    return a.looseEquals(b);\n"
			"}\n"
			"inline bool __y_eq(long long a, long long b) { return a == b; }\n"
			"inline bool __y_eq(double a, double b) { return a == b; }\n"
			"inline bool __y_eq(const Value& a, long long b) {\n"
			"    if (a.type == ValueType::INT) return a.iVal == b;\n"
			"    if (a.isNumber()) return a.asFloat() == (double)b;\n"
			"    return false;\n"
			"}\n"
			"inline bool __y_eq(long long a, const Value& b) {\n"
			"    return __y_eq(b, a);\n"
			"}\n"
			"inline bool __y_neq(const Value& a, const Value& b) {\n"
			"    return !__y_eq(a, b);\n"
			"}\n"
			"inline bool __y_neq(long long a, long long b) { return a != b; }\n"
			"inline bool __y_neq(double a, double b) { return a != b; }\n"
			"inline Value __y_add(const Value& a, const Value& b);\n"
			"inline Value __y_sub(const Value& a, const Value& b);\n"
			"inline Value __y_mul(const Value& a, const Value& b);\n"
			"inline Value __y_div(const Value& a, const Value& b);\n\n"
			"inline Value __y_add(long long a, long long b) {\n"
			"    long long res;\n"
			"    if (!__builtin_add_overflow(a, b, &res)) return Value::Int(res);\n"
			"    return BigIntObject::add(Value::BigInt(a), Value::BigInt(b));\n"
			"}\n"
			"inline Value __y_add(long long a, const Value& b) {\n"
			"    if (b.type == ValueType::INT) {\n"
			"        long long res;\n"
			"        if (!__builtin_add_overflow(a, b.iVal, &res)) return Value::Int(res);\n"
			"        return BigIntObject::add(Value::BigInt(a), Value::BigInt(b.iVal));\n"
			"    }\n"
			"    return __y_add(Value::Int(a), b);\n"
			"}\n"
			"inline Value __y_add(const Value& a, long long b) {\n"
			"    if (a.type == ValueType::INT) {\n"
			"        long long res;\n"
			"        if (!__builtin_add_overflow(a.iVal, b, &res)) return Value::Int(res);\n"
			"        return BigIntObject::add(Value::BigInt(a.iVal), Value::BigInt(b));\n"
			"    }\n"
			"    return __y_add(a, Value::Int(b));\n"
			"}\n"
			"inline Value __y_add(double a, double b) { return Value::Float(a + b); }\n"
			"inline Value __y_add(const Value& a, double b) { return __y_add(a, Value::Float(b)); }\n"
			"inline Value __y_add(double a, const Value& b) { return __y_add(Value::Float(a), b); }\n\n"
			"inline Value __y_add(const Value& a, const Value& b) {\n"
			"    if (a.type == ValueType::INT && b.type == ValueType::INT) {\n"
			"        long long res;\n"
			"        if (!__builtin_add_overflow(a.iVal, b.iVal, &res)) return Value::Int(res);\n"
			"        return BigIntObject::add(Value::BigInt(a.iVal), Value::BigInt(b.iVal));\n"
			"    }\n"
			"    if (a.type == ValueType::FLOAT && b.type == ValueType::FLOAT) {\n"
			"        return Value::Float(a.fVal + b.fVal);\n"
			"    }\n"
			"    return EvaluateConstBinary(TokenType::PLUS, a, b);\n"
			"}\n\n"

			"inline Value __y_sub(long long a, long long b) {\n"
			"    long long res;\n"
			"    if (!__builtin_sub_overflow(a, b, &res)) return Value::Int(res);\n"
			"    return BigIntObject::sub(Value::BigInt(a), Value::BigInt(b));\n"
			"}\n"
			"inline Value __y_sub(long long a, const Value& b) {\n"
			"    if (b.type == ValueType::INT) {\n"
			"        long long res;\n"
			"        if (!__builtin_sub_overflow(a, b.iVal, &res)) return Value::Int(res);\n"
			"        return BigIntObject::sub(Value::BigInt(a), Value::BigInt(b.iVal));\n"
			"    }\n"
			"    return __y_sub(Value::Int(a), b);\n"
			"}\n"
			"inline Value __y_sub(const Value& a, long long b) {\n"
			"    if (a.type == ValueType::INT) {\n"
			"        long long res;\n"
			"        if (!__builtin_sub_overflow(a.iVal, b, &res)) return Value::Int(res);\n"
			"        return BigIntObject::sub(Value::BigInt(a.iVal), Value::BigInt(b));\n"
			"    }\n"
			"    return __y_sub(a, Value::Int(b));\n"
			"}\n"
			"inline Value __y_sub(double a, double b) { return Value::Float(a - b); }\n"
			"inline Value __y_sub(const Value& a, double b) { return __y_sub(a, Value::Float(b)); }\n"
			"inline Value __y_sub(double a, const Value& b) { return __y_sub(Value::Float(a), b); }\n\n"
			"inline Value __y_sub(const Value& a, const Value& b) {\n"
			"    if (a.type == ValueType::INT && b.type == ValueType::INT) {\n"
			"        long long res;\n"
			"        if (!__builtin_sub_overflow(a.iVal, b.iVal, &res)) return Value::Int(res);\n"
			"        return BigIntObject::sub(Value::BigInt(a.iVal), Value::BigInt(b.iVal));\n"
			"    }\n"
			"    if (a.type == ValueType::FLOAT && b.type == ValueType::FLOAT) {\n"
			"        return Value::Float(a.fVal - b.fVal);\n"
			"    }\n"
			"    return EvaluateConstBinary(TokenType::MINUS, a, b);\n"
			"}\n\n"

			"inline Value __y_mul(long long a, long long b) {\n"
			"    long long res;\n"
			"    if (!__builtin_mul_overflow(a, b, &res)) return Value::Int(res);\n"
			"    return BigIntObject::mul(Value::BigInt(a), Value::BigInt(b));\n"
			"}\n"
			"inline Value __y_mul(long long a, const Value& b) {\n"
			"    if (b.type == ValueType::INT) {\n"
			"        long long res;\n"
			"        if (!__builtin_mul_overflow(a, b.iVal, &res)) return Value::Int(res);\n"
			"        return BigIntObject::mul(Value::BigInt(a), Value::BigInt(b.iVal));\n"
			"    }\n"
			"    return __y_mul(Value::Int(a), b);\n"
			"}\n"
			"inline Value __y_mul(const Value& a, long long b) {\n"
			"    if (a.type == ValueType::INT) {\n"
			"        long long res;\n"
			"        if (!__builtin_mul_overflow(a.iVal, b, &res)) return Value::Int(res);\n"
			"        return BigIntObject::mul(Value::BigInt(a.iVal), Value::BigInt(b));\n"
			"    }\n"
			"    return __y_mul(a, Value::Int(b));\n"
			"}\n"
			"inline Value __y_mul(double a, double b) { return Value::Float(a * b); }\n"
			"inline Value __y_mul(const Value& a, double b) { return __y_mul(a, Value::Float(b)); }\n"
			"inline Value __y_mul(double a, const Value& b) { return __y_mul(Value::Float(a), b); }\n\n"
			"inline Value __y_mul(const Value& a, const Value& b) {\n"
			"    if (a.type == ValueType::INT && b.type == ValueType::INT) {\n"
			"        long long res;\n"
			"        if (!__builtin_mul_overflow(a.iVal, b.iVal, &res)) return Value::Int(res);\n"
			"        return BigIntObject::mul(Value::BigInt(a.iVal), Value::BigInt(b.iVal));\n"
			"    }\n"
			"    if (a.type == ValueType::FLOAT && b.type == ValueType::FLOAT) {\n"
			"        return Value::Float(a.fVal * b.fVal);\n"
			"    }\n"
			"    return EvaluateConstBinary(TokenType::STAR, a, b);\n"
			"}\n\n"

			"inline Value __y_div(long long a, long long b) {\n"
			"    if (b == 0) throw std::runtime_error(\"Division by zero\");\n"
			"    return Value::Float((double)a / (double)b);\n"
			"}\n"
			"inline Value __y_div(long long a, const Value& b) {\n"
			"    return __y_div(Value::Int(a), b);\n"
			"}\n"
			"inline Value __y_div(const Value& a, long long b) {\n"
			"    if (b == 0) throw std::runtime_error(\"Division by zero\");\n"
			"    return __y_div(a, Value::Float((double)b));\n"
			"}\n"
			"inline Value __y_div(double a, double b) {\n"
			"    if (b == 0.0) throw std::runtime_error(\"Division by zero\");\n"
			"    return Value::Float(a / b);\n"
			"}\n"
			"inline Value __y_div(const Value& a, double b) {\n"
			"    if (b == 0.0) throw std::runtime_error(\"Division by zero\");\n"
			"    return __y_div(a, Value::Float(b));\n"
			"}\n"
			"inline Value __y_div(double a, const Value& b) {\n"
			"    return __y_div(Value::Float(a), b);\n"
			"}\n\n"
			"inline Value __y_div(const Value& a, const Value& b) {\n"
			"    if (a.isNumber() && b.isNumber()) {\n"
			"        double db = b.asFloat();\n"
			"        if (db == 0.0) throw std::runtime_error(\"Division by zero\");\n"
			"        return Value::Float(a.asFloat() / db);\n"
			"    }\n"
			"    return EvaluateConstBinary(TokenType::SLASH, a, b);\n"
			"}\n\n"

			"inline std::vector<Value> __y_to_vector(const Value& val) {\n"
			"    if (val.type == ValueType::LIST) {\n"
			"        return static_cast<ListObject*>(val.ref.get())->elements;\n"
			"    } else if (val.type == ValueType::TUPLE) {\n"
			"        return static_cast<TupleObject*>(val.ref.get())->elements;\n"
			"    } else if (val.type == ValueType::VECTOR) {\n"
			"        return static_cast<VectorObject*>(val.ref.get())->elements;\n"
			"    } else if (val.type == ValueType::STRING) {\n"
			"        std::string s = val.asString();\n"
			"        std::vector<Value> res;\n"
			"        res.reserve(s.size());\n"
			"        for (char c : s) res.push_back(Value::String(std::string(1, c)));\n"
			"        return res;\n"
			"    } else if (val.type == ValueType::SET) {\n"
			"        auto* s = static_cast<SetObject*>(val.ref.get());\n"
			"        std::vector<Value> res;\n"
			"        res.reserve(s->elements.size());\n"
			"        for (const auto& elem : s->elements) res.push_back(elem);\n"
			"        return res;\n"
			"    }\n"
			"    return {};\n"
			"}\n\n";

	ss << "Value __y_call(Value callee, const std::vector<Value>& args) {\n"
			"    if (callee.type == ValueType::NATIVE_FUNCTION) {\n"
			"        auto* nf = static_cast<NativeFunctionObject*>(callee.ref.get());\n"
			"        return nf->func(args, args.size(), 0);\n"
			"    } else if (callee.type == ValueType::CLASS) {\n"
			"        auto* cls = static_cast<ClassObject*>(callee.ref.get());\n"
			"        Value instance = Value::Instance(callee);\n"
			"        if (cls->methods.count(\"__construct__\")) {\n"
			"            Value initMethod = cls->methods[\"__construct__\"].func;\n"
			"            if (initMethod.type == ValueType::NATIVE_FUNCTION) {\n"
			"                static_cast<NativeFunctionObject*>(initMethod.ref.get())->func(args, args.size(), 0);\n"
			"            }\n"
			"        }\n"
			"        return instance;\n"
			"    }\n"
			"    return Value::None();\n"
			"}\n\n"
			"Value __y_get_index(Value base, Value index) {\n"
			"    if (base.type == ValueType::LIST) {\n"
			"        auto* list = static_cast<ListObject*>(base.ref.get());\n"
			"        long long idx = index.asInt();\n"
			"        if (idx < 0) idx += list->elements.size();\n"
			"        if (idx >= 0 && idx < (long long)list->elements.size()) return list->elements[idx];\n"
			"    } else if (base.type == ValueType::TUPLE) {\n"
			"        auto* tup = static_cast<TupleObject*>(base.ref.get());\n"
			"        long long idx = index.asInt();\n"
			"        if (idx < 0) idx += tup->elements.size();\n"
			"        if (idx >= 0 && idx < (long long)tup->elements.size()) return tup->elements[idx];\n"
			"    } else if (base.type == ValueType::DICT) {\n"
			"        auto* dict = static_cast<DictObject*>(base.ref.get());\n"
			"        if (dict->items.count(index)) return dict->items[index];\n"
			"    } else if (base.type == ValueType::STRING) {\n"
			"        std::string s = base.asString();\n"
			"        long long idx = index.asInt();\n"
			"        if (idx < 0) idx += s.size();\n"
			"        if (idx >= 0 && idx < (long long)s.size()) return Value::String(std::string(1, s[idx]));\n"
			"    }\n"
			"    return Value::None();\n"
			"}\n\n"
			"void __y_set_index(Value base, Value index, Value val) {\n"
			"    if (base.type == ValueType::LIST) {\n"
			"        auto* list = static_cast<ListObject*>(base.ref.get());\n"
			"        long long idx = index.asInt();\n"
			"        if (idx < 0) idx += list->elements.size();\n"
			"        if (idx >= 0 && idx < (long long)list->elements.size()) list->elements[idx] = val;\n"
			"    } else if (base.type == ValueType::VECTOR) {\n"
			"        auto* vec = static_cast<VectorObject*>(base.ref.get());\n"
			"        long long idx = index.asInt();\n"
			"        if (idx < 0) idx += vec->elements.size();\n"
			"        if (idx >= 0 && idx < (long long)vec->elements.size()) vec->elements[idx] = val;\n"
			"    } else if (base.type == ValueType::DICT) {\n"
			"        auto* dict = static_cast<DictObject*>(base.ref.get());\n"
			"        dict->items[index] = val;\n"
			"    }\n"
			"}\n\n";

	ss << "struct EnvRestorer {\n";
	ss << "    std::shared_ptr<Env>& current_env;\n";
	ss << "    std::shared_ptr<Env> previous_env;\n";
	ss << "    EnvRestorer(std::shared_ptr<Env>& ce) : current_env(ce), previous_env(ce) {\n";
	ss << "        current_env = std::make_shared<Env>();\n";
	ss << "        current_env->parent = previous_env;\n";
	ss << "    }\n";
	ss << "    ~EnvRestorer() { current_env = previous_env; }\n";
	ss << "};\n\n";

	// Globals environment
	ss << "std::shared_ptr<Env> y_transpile_globals = std::make_shared<Env>();\n";
	ss << "VM transpiled_vm;\n";
	ss << "Value __y_method_call(const std::string& method, Value obj, std::vector<Value> args) {\n";
	ss << "    if (obj.type == ValueType::REFERENCE && obj.get_ptr_safe()) obj = *obj.get_ptr_safe();\n";
	ss << "    std::vector<ValueExpr*> dummyArgs;\n";
	ss << "    for (auto& arg : args) dummyArgs.push_back(new ValueExpr(arg));\n";
	ss << "    ValueExpr* dummyObject = new ValueExpr(obj);\n";
	ss << "    std::vector<Expr*> exprArgs(dummyArgs.begin(), dummyArgs.end());\n";
	ss << "    MethodCallExpr mockAST(dummyObject, method, exprArgs);\n";
	ss << "    Value result = transpiled_vm.methodResolver(&mockAST);\n";
	ss << "    delete dummyObject;\n";
	ss << "    for (auto* a : dummyArgs) delete a;\n";
	ss << "    return result;\n";
	ss << "}\n\n";

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
		ss << "    y_user_main();\n";
	}

	ss << "\n    return 0;\n";
	ss << "}\n";
	return ss.str();
}
