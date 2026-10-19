#include "ylang/transpiler.hpp"
#include <iostream>

using namespace std;


static std::string serializeValue(const Value& val) {
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
        auto* l = static_cast<ListObject*>(val.ref.get());
        std::string s = "Value::List({";
        for (size_t i = 0; i < l->elements.size(); i++) {
            s += serializeValue(l->elements[i]);
            if (i < l->elements.size() - 1) s += ", ";
        }
        return s + "})";
    }
    case ValueType::TUPLE: {
        auto* l = static_cast<TupleObject*>(val.ref.get());
        std::string s = "Value::Tuple({";
        for (size_t i = 0; i < l->elements.size(); i++) {
            s += serializeValue(l->elements[i]);
            if (i < l->elements.size() - 1) s += ", ";
        }
        return s + "})";
    }
    case ValueType::SET: {
        auto* l = static_cast<SetObject*>(val.ref.get());
        std::string s = "Value::Set(std::unordered_set<Value, ValueHash, ValueEqual>{";
        size_t i = 0;
        for (const auto& el : l->elements) {
            s += serializeValue(el);
            if (i < l->elements.size() - 1) s += ", ";
            i++;
        }
        return s + "})";
    }
    case ValueType::DICT: {
        auto* l = static_cast<DictObject*>(val.ref.get());
        std::string s = "Value::Dict(std::unordered_map<Value, Value, ValueHash, ValueEqual>{";
        size_t i = 0;
        for (const auto& pair : l->items) {
            s += "{" + serializeValue(pair.first) + ", " + serializeValue(pair.second) + "}";
            if (i < l->items.size() - 1) s += ", ";
            i++;
        }
        return s + "})";
    }
    case ValueType::VECTOR: {
        auto* l = static_cast<VectorObject*>(val.ref.get());
        std::string s = "Value::Vector(std::vector<Value>{";
        for (size_t i = 0; i < l->elements.size(); i++) {
            s += serializeValue(l->elements[i]);
            if (i < l->elements.size() - 1) s += ", ";
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

std::string CppTranspiler::transpileExpr(Expr *e) {
	if (!e)
		return "Value::None()";

	switch (e->type) {
	case ExprType::NUMBER: {
		auto num = static_cast<NumberExpr *>(e);
		if (num->isFloat) {
			return "Value::Float(" + to_string(num->val) + ")";
		} else {
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
		// Dynamic variable lookup
		return "current_env->get(\"" + v->name + "\")";
	}
	case ExprType::CALL: {
		auto call = static_cast<CallExpr *>(e);
		string calleeStr = "current_env->get(\"" + call->name + "\")";
		string argsStr = "";
		for (size_t i = 0; i < call->args.size(); i++) {
			argsStr += transpileExpr(call->args[i]);
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
		
		size_t pos_idx = 0;
		for (const auto& p : lambda->params) {
			if (p.isVariadic) {
				lambdaStr += "        std::vector<Value> _rest_list;\n";
				lambdaStr += "        for (size_t _i = " + std::to_string(pos_idx) + "; _i < _pos_count; _i++) {\n";
				lambdaStr += "            _rest_list.push_back(_args[_i]);\n";
				lambdaStr += "        }\n";
				lambdaStr += "        current_env->set(\"" + p.name + "\", Value::Tuple(_rest_list), false, false);\n";
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
				lambdaStr += "        current_env->set(\"" + p.name + "\", _arg_" + p.name + ", false, false);\n";
				pos_idx++;
			}
		}

		// Body
		string saved_output = output;
		output = ""; // Temporary redirect
		
		for (auto stmt : lambda->body) {
			transpileStmt(stmt);
		}
		output += "        return Value::None();\n";
		
		string bodyOutput = output;
		output = saved_output; // Restore
		
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
			string varName = "\"" + varExpr->name + "\"";
			string op = incDec->isIncrement ? "TokenType::PLUS" : "TokenType::MINUS";
			if (incDec->isPrefix) {
				return "[&]() { Value tmp = EvaluateConstBinary(" + op + ", current_env->get(" + varName + "), Value::Int(1LL)); current_env->assign(" + varName + ", tmp); return tmp; }()";
			} else {
				return "[&]() { Value tmp = current_env->get(" + varName + "); current_env->assign(" + varName + ", EvaluateConstBinary(" + op + ", tmp, Value::Int(1LL))); return tmp; }()";
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
	case ExprType::BINARY: {
		auto bin = static_cast<BinExpr *>(e);
		if (!bin->left && bin->op == TokenType::NOT) {
			return "Value::Bool(!(" + transpileExpr(bin->right) + ").isTruthy())";
		}
		if (!bin->left && bin->op == TokenType::BITWISE_NOT) {
			return "EvaluateConstBinary(TokenType::BITWISE_NOT, Value::Int(0), " + transpileExpr(bin->right) + ")";
		}
		string left = transpileExpr(bin->left);
		string right = transpileExpr(bin->right);
		// Map token type to C++ enum
		string opEnum = "TokenType::PLUS"; // Default fallback
		switch (bin->op) {
		case TokenType::PLUS:
			opEnum = "TokenType::PLUS";
			break;
		case TokenType::MINUS:
			opEnum = "TokenType::MINUS";
			break;
		case TokenType::STAR:
			opEnum = "TokenType::STAR";
			break;
		case TokenType::SLASH:
			opEnum = "TokenType::SLASH";
			break;
		case TokenType::EQ:
			opEnum = "TokenType::EQ";
			break;
		case TokenType::STRICT_EQ:
			opEnum = "TokenType::STRICT_EQ";
			break;
		case TokenType::AND:
			opEnum = "TokenType::AND";
			break;
		case TokenType::OR:
			opEnum = "TokenType::OR";
			break;
		case TokenType::NEQ:
			opEnum = "TokenType::NEQ";
			break;
		case TokenType::STRICT_NEQ:
			opEnum = "TokenType::STRICT_NEQ";
			break;
		case TokenType::LT:
			opEnum = "TokenType::LT";
			break;
		case TokenType::LTE:
			opEnum = "TokenType::LTE";
			break;
		case TokenType::GT:
			opEnum = "TokenType::GT";
			break;
		case TokenType::GTE:
			opEnum = "TokenType::GTE";
			break;
		default:
			break; // Handle more as needed
		}
		return "EvaluateConstBinary(" + opEnum + ", " + left + ", " + right + ")";
	}
	default:
		return "Value::None() /* Unimplemented Expr */";
	}
}

void CppTranspiler::transpileStmt(Stmt *s) {
	if (!s)
		return;

	switch (s->type) {
	case StmtType::LET: {
		auto let = static_cast<LetStmt *>(s);
		string valStr = transpileExpr(let->value);
		output += "    current_env->set(\"" + let->name + "\", " + valStr + ", " +
					 (let->isLocked ? "true" : "false") + ", " +
					 (let->isConst ? "true" : "false") + ");\n";
		break;
	}
	case StmtType::ASSIGN: {
		auto assign = static_cast<AssignStmt *>(s);
		if (assign->target->type == ExprType::VAR) {
			auto v = static_cast<VarExpr *>(assign->target);
			string valStr = transpileExpr(assign->value);
			output += "    current_env->assign(\"" + v->name + "\", " + valStr + ");\n";
		}
		break;
	}
	case StmtType::IF: {
		auto ifStmt = static_cast<IfStmt *>(s);
		output += "    if ((" + transpileExpr(ifStmt->condition) + ").isTruthy()) {\n";
		output += "        EnvRestorer env_restorer(current_env);\n";
		for (auto stmt : ifStmt->body)
			transpileStmt(stmt);
		output += "    }\n";
		// TODO: handle elifs here if needed
		if (!ifStmt->elseBody.empty()) {
			output += "    else {\n";
			output += "        EnvRestorer env_restorer(current_env);\n";
			for (auto stmt : ifStmt->elseBody)
				transpileStmt(stmt);
			output += "    }\n";
		}
		break;
	}
	case StmtType::WHILE: {
		auto wStmt = static_cast<WhileStmt *>(s);
		int loopId = loopIdCounter++;
		loopStack.push_back({loopId, false});
		output += "    while ((" + transpileExpr(wStmt->condition) + ").isTruthy()) {\n";
		output += "        EnvRestorer env_restorer(current_env);\n";
		for (auto stmt : wStmt->body)
			transpileStmt(stmt);
		output += "    }\n";
		loopStack.pop_back();
		break;
	}
	case StmtType::FOR: {
		auto fStmt = static_cast<ForStmt *>(s);
		int loopId = loopIdCounter++;
		loopStack.push_back({loopId, false});
		output += "    {\n"; // Outer block for initializer
		output += "        EnvRestorer env_restorer_outer(current_env);\n";
		for (auto stmt : fStmt->inits)
			transpileStmt(stmt);
		output += "        while (";
		if (fStmt->condition)
			output += "(" + transpileExpr(fStmt->condition) + ").isTruthy()";
		else
			output += "true";
		output += ") {\n";
		output += "            EnvRestorer env_restorer_inner(current_env);\n";
		for (auto stmt : fStmt->body)
			transpileStmt(stmt);
		for (auto stmt : fStmt->steps)
			transpileStmt(stmt);
		output += "        }\n";
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
		output += "        std::vector<Value> _streams;\n";
		// Wait! y-- unpacks dicts as 2 streams (keys and values).
		bool isDictUnpack = (fe->collections.size() == 1 && fe->loopVars.size() == 2);
		if (isDictUnpack) {
			output += "        Value _dict_val = " + transpileExpr(fe->collections[0]) + ";\n";
			output += "        std::vector<Value> _keys; std::vector<Value> _vals;\n";
			output += "        if (_dict_val.type == ValueType::DICT) {\n";
			output += "            auto* _d = static_cast<DictObject*>(_dict_val.ref.get());\n";
			output += "            for (auto& _p : _d->elements) { _keys.push_back(_p.first); _vals.push_back(_p.second); }\n";
			output += "        }\n";
			output += "        _streams.push_back(Value::List(_keys));\n";
			output += "        _streams.push_back(Value::List(_vals));\n";
		} else {
			for (auto col : fe->collections) {
				output += "        _streams.push_back(" + transpileExpr(col) + ");\n";
			}
		}
		output += "        while (true) {\n";
		output += "            bool _valid = true;\n";
		output += "            std::vector<Value> _next_vals;\n";
		output += "            for (auto& _stream : _streams) {\n";
		output += "                Value _item;\n";
		output += "                if (_stream.type == ValueType::LIST) {\n";
		output += "                    auto* _l = static_cast<ListObject*>(_stream.ref.get());\n";
		output += "                    if (_iter_idx_" + std::to_string(loopId) + " >= (long long)_l->elements.size()) { _valid = false; break; }\n";
		output += "                    _item = _l->elements[_iter_idx_" + std::to_string(loopId) + "];\n";
		output += "                } else if (_stream.type == ValueType::TUPLE) {\n";
		output += "                    auto* _t = static_cast<TupleObject*>(_stream.ref.get());\n";
		output += "                    if (_iter_idx_" + std::to_string(loopId) + " >= (long long)_t->elements.size()) { _valid = false; break; }\n";
		output += "                    _item = _t->elements[_iter_idx_" + std::to_string(loopId) + "];\n";
		output += "                } else if (_stream.type == ValueType::STRING) {\n";
		output += "                    std::string _str = _stream.asString();\n";
		output += "                    if (_iter_idx_" + std::to_string(loopId) + " >= (long long)_str.length()) { _valid = false; break; }\n";
		output += "                    _item = Value::String(std::string(1, _str[_iter_idx_" + std::to_string(loopId) + "]));\n";
		output += "                } else if (_stream.type == ValueType::SET) {\n";
		output += "                    auto* _s = static_cast<SetObject*>(_stream.ref.get());\n";
		output += "                    if (_iter_idx_" + std::to_string(loopId) + " >= (long long)_s->elements.size()) { _valid = false; break; }\n";
		output += "                    auto it = _s->elements.begin(); std::advance(it, _iter_idx_" + std::to_string(loopId) + ");\n";
		output += "                    _item = *it;\n";
		output += "                } else if (_stream.type == ValueType::VECTOR) {\n";
		output += "                    auto* _v = static_cast<VectorObject*>(_stream.ref.get());\n";
		output += "                    if (_iter_idx_" + std::to_string(loopId) + " >= (long long)_v->elements.size()) { _valid = false; break; }\n";
		output += "                    _item = _v->elements[_iter_idx_" + std::to_string(loopId) + "];\n";
		output += "                } else {\n";
		output += "                    _valid = false; break;\n";
		output += "                }\n";
		output += "                _next_vals.push_back(_item);\n";
		output += "            }\n";
		output += "            if (!_valid) break;\n";
		output += "            EnvRestorer env_restorer_inner(current_env);\n";
		for (size_t i = 0; i < fe->loopVars.size(); i++) {
			output += "            current_env->set(\"" + fe->loopVars[i] + "\", _next_vals[" + std::to_string(i) + "], false, false);\n";
		}
		for (auto stmt : fe->body)
			transpileStmt(stmt);
		output += "            _iter_idx_" + std::to_string(loopId) + "++;\n";
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
		output += "    return " + transpileExpr(ret->value) + ";\n";
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
				output += "        if (EvaluateConstBinary(TokenType::STRICT_EQ, __switch_target, " + caseValStr + ").isTruthy()) {\n";
				firstCase = false;
			} else {
				output += "        else if (EvaluateConstBinary(TokenType::STRICT_EQ, __switch_target, " + caseValStr + ").isTruthy()) {\n";
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
		output += "    if (!(" + transpileExpr(astmt->condition) + ").isTruthy()) {\n";
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
		output += "    current_env->set(\"" + func->name + "\", Value::Native([current_env](const std::vector<Value>& _args, int _pos_count, int _kw_count) mutable -> Value {\n";
		output += "        EnvRestorer env_restorer(current_env);\n";
		
		size_t pos_idx = 0;
		for (const auto& p : func->params) {
			if (p.isVariadic) {
				output += "        std::vector<Value> _rest_list;\n";
				output += "        for (size_t _i = " + std::to_string(pos_idx) + "; _i < _pos_count; _i++) {\n";
				output += "            _rest_list.push_back(_args[_i]);\n";
				output += "        }\n";
				output += "        current_env->set(\"" + p.name + "\", Value::Tuple(_rest_list), false, false);\n";
			} else {
				output += "        Value _arg_" + p.name + ";\n";
				output += "        if (_pos_count > " + std::to_string(pos_idx) + ") {\n";
				output += "            _arg_" + p.name + " = _args[" + std::to_string(pos_idx) + "];\n";
				output += "        } else {\n";
				if (p.defaultValue) {
					output += "            _arg_" + p.name + " = " + transpileExpr(p.defaultValue) + ";\n";
				} else {
					output += "            _arg_" + p.name + " = Value::NoType();\n";
				}
				output += "        }\n";
				output += "        current_env->set(\"" + p.name + "\", _arg_" + p.name + ", false, false);\n";
				pos_idx++;
			}
		}

		string saved_output = output;
		output = "";
		for (auto stmt : func->body) {
			transpileStmt(stmt);
		}
		output += "        return Value::None();\n";
		
		string bodyOutput = output;
		output = saved_output + bodyOutput;
		
		output += "    }), false, false);\n";

		if (func->name == "main") {
			output += "    // Auto-calling main() for PoC\n";
			output += "    if (current_env->get(\"main\").type == ValueType::NATIVE_FUNCTION) {\n";
			output += "        auto* nf = static_cast<NativeFunctionObject*>(current_env->get(\"main\").ref.get());\n";
			output += "        nf->func({}, 0, 0);\n";
			output += "    }\n";
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
			"\n";
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

	ss << constantsBlock;

	ss << "int main(int argc, char* argv[]) {\n";
	ss << "    Interpreter transpiled_interp;\n";
	ss << "    y_transpile_globals = transpiled_interp.env;\n";
	ss << "    transpiled_vm.globals = y_transpile_globals;\n";
	ss << "    transpiled_vm.methodResolver = [&](MethodCallExpr *m) { return transpiled_interp.Resolve_methods(m); };\n";
	ss << "    std::shared_ptr<Env> current_env = y_transpile_globals;\n\n";

	ss << output;

	ss << "\n    return 0;\n";
	ss << "}\n";
	return ss.str();
}
