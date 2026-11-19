#include "ylang/vm.hpp"
#include "objects.hpp"
#include "types.hpp"

#ifdef _WIN32
#include <shellapi.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif

Value Interpreter::Resolve_methods(MethodCallExpr *m) {
	auto error = [&](const string &msg, const string &type = "RuntimeError") {
		if (type == "TypeError")
			throw TypeError(msg, m->line, m->col);
		if (type == "ValueError")
			throw ValueError(msg, m->line, m->col);
		if (type == "IndexError")
			throw IndexError(msg, m->line, m->col);
		if (type == "ConstError")
			throw ConstError(msg, m->line, m->col);
		if (type == "KeyError")
			throw KeyError(msg, m->line, m->col);
		if (type == "ArgumentError")
			throw ArgumentError(msg, m->line, m->col);
		if (type == "EmptyContainerError")
			throw EmptyContainerError(msg, m->line, m->col);
		if (type == "AttributeError")
			throw AttributeError(msg, m->line, m->col);
		throw RuntimeError(msg, m->line, m->col);
	};
	Value *targetPtr = nullptr;
	Value tempVal;
	bool isConstView = false;
	auto mRes = [this](MethodCallExpr *m) {
		return this->Resolve_methods(m);
	};

	auto iRes = [this](std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
		this->resolveImport(lib, sym, targetEnv);
	};
	if (dynamic_cast<VarExpr *>(m->object) ||
		 dynamic_cast<IndexExpr *>(m->object)) {
		LValue lv = resolveLValue(m->object);
		targetPtr = lv.ref;
		isConstView = lv.isConstView;
	} else {
		tempVal = eval(m->object);
		targetPtr = &tempVal;
		isConstView = tempVal.isConst;
	}
	Value &target = *targetPtr;
	while (target.type == ValueType::REFERENCE) {
		if (!target.get_ptr_safe())
			throw RuntimeError("Dead-end reference", m->line, m->col);
		target = *(target.get_ptr_safe());
	}
	auto checkConst = [&]() {
		if (target.isConst || isConstView) {
			error(
				"Cannot call mutating method '" + m->method + "' on const object",
				"ConstError");
		}
	};
	// if (m->method == "swap") {
	// 	if (m->args.size() != 1) error("swap() takes one argument",
	// "ArgumentError"); 	auto* base = &eval(m->args[0]);
	// 	//std::swap(base, target);
	// 	auto* temp = base;
	// 	base = targetPtr;
	// 	targetPtr = temp;
	// 	return Value::None();
	// }
	if (m->method == "adress") {
		if (!m->args.empty())
			error("adress() takes no arguments", "ArgumentError");
		if (target.ref) {
			return Value::pInt(target.ref.get());
		}
		if (dynamic_cast<VarExpr *>(m->object) ||
			 dynamic_cast<IndexExpr *>(m->object)) {
			return Value::pInt(targetPtr);
		}
		return Value::pInt(nullptr);
	}
	if (m->method == "base") {
		if (m->args.size() != 1)
			error("base() takes exactly one argument", "ArgumentError");
		int base = eval(m->args[0]).asInt();
		if (base < 2 || base > 36)
			error("base() target must be between 2 and 36", "ValueError");
		if (target.type == ValueType::INT || target.type == ValueType::FLOAT) {
			string res = "";
			bool isNeg = false;
			double val = target.asFloat();
			if (val < 0) {
				isNeg = true;
				val = -val;
			}
			long long intPart = (long long)val;
			double fracPart = val - intPart;
			if (intPart == 0)
				res = "0";
			else {
				while (intPart > 0) {
					int rem = intPart % base;
					res += (rem < 10 ? '0' + rem : 'A' + (rem - 10));
					intPart /= base;
				}
			}
			if (isNeg)
				res += '-';
			std::reverse(res.begin(), res.end());
			if (fracPart > 0) {
				res += '.';
				int precision = 8;
				while (fracPart > 0 && precision-- > 0) {
					fracPart *= base;
					int digit = (int)fracPart;
					res += (digit < 10 ? '0' + digit : 'A' + (digit - 10));
					fracPart -= digit;
					if (fracPart < 1e-9)
						break;
				}
			}
			return Value::String(res);
		}
		error("base() requires an int or float", "TypeError");
	}
	// ---------------- REVERSE ---------------
	if (m->method == "reverse") {
		if (m->args.size() > 2)
			error("reverse() accepts 2 optional arguments (modify_original:bool(true), custom_lambda = None)", "ArgumentError");
		bool modifyOriginal = true;
		if (m->args.size() >= 1) {
			modifyOriginal = eval(m->args[0]).asBool();
		}
		Value customLambda = Value::None();
		if (m->args.size() == 2) {
			customLambda = eval(m->args[1]);
			if (customLambda.type != ValueType::FUNCTION && customLambda.type != ValueType::NONE) {
				error("reverse() second argument must be a function/lambda or None", "TypeError");
			}
		}
		if (modifyOriginal) {
			checkConst();
		}
		VM tempVM;
		if (customLambda.type == ValueType::FUNCTION) {
			tempVM.globals = this->env;
			tempVM.methodResolver = [&](MethodCallExpr *expr) {
				return this->Resolve_methods(expr);
			};
		}
		auto applyLambda = [&](Value &elem) {
			Chunk tempChunk;
			int lambdaIdx = tempChunk.addConstant(customLambda);
			int argIdx = tempChunk.addConstant(elem);
			tempChunk.write(OpCode::OP_CONSTANT, m->line, m->col);
			tempChunk.write((uint8_t)argIdx, m->line, m->col);
			tempChunk.write(OpCode::OP_CONSTANT, m->line, m->col);
			tempChunk.write((uint8_t)lambdaIdx, m->line, m->col);
			tempChunk.write(OpCode::OP_CALL, m->line, m->col);
			tempChunk.write((uint8_t)1, m->line, m->col);
			tempChunk.write(OpCode::OP_RETURN, m->line, m->col);
			tempVM.stack.clear();
			tempVM.run(tempChunk);
			elem = tempVM.stack.empty() ? Value::None() : tempVM.stack.back();
		};
		if (target.type == ValueType::STRING) {
			auto *strObj = static_cast<StringObject *>(target.ref.get());
			string s = strObj->value;
			std::reverse(s.begin(), s.end());
			if (customLambda.type == ValueType::FUNCTION) {
				string mappedStr = "";
				for (char c : s) {
					Value charVal = Value::String(string(1, c));
					applyLambda(charVal);
					mappedStr += charVal.asString();
				}
				s = mappedStr;
			}
			if (modifyOriginal) {
				strObj->value = s;
				return target;
			}
			return Value::String(s);
		}
		if (target.type == ValueType::LIST) {
			auto *listObj = static_cast<ListObject *>(target.ref.get());
			vector<Value> elems = listObj->elements;
			std::reverse(elems.begin(), elems.end());
			if (customLambda.type == ValueType::FUNCTION) {
				for (auto &elem : elems) {
					applyLambda(elem);
				}
			}
			if (modifyOriginal) {
				listObj->elements = std::move(elems);
				return target;
			}
			return Value::List(elems);
		}
		if (target.type == ValueType::RANGE) {
			if (customLambda.type == ValueType::FUNCTION) {
				error("Cannot apply a lambda to a reversed Range directly. Cast to a list first.", "TypeError");
			}
			auto *rang = static_cast<RangeObject *>(target.ref.get());
			double newStart = rang->end;
			double newEnd = rang->start;
			bool newStartInc = rang->endInclusive;
			bool newEndInc = rang->startInclusive;
			double newStep = -rang->step;
			if (modifyOriginal) {
				rang->start = newStart;
				rang->end = newEnd;
				rang->startInclusive = newStartInc;
				rang->endInclusive = newEndInc;
				rang->step = newStep;
				return target;
			} else {
				auto newRange = std::make_shared<RangeObject>(*rang);
				newRange->start = newStart;
				newRange->end = newEnd;
				newRange->step = newStep;
				newRange->startInclusive = newStartInc;
				newRange->endInclusive = newEndInc;
				return Value::Range(newRange->start, newRange->end, newRange->step, newRange->startInclusive, newRange->endInclusive, newRange->isFloat);
			}
		}
		error("reverse() not supported on this type", "TypeError");
	}
	//----------------- RESERVE ---------------
	if (m->method == "reserve") {
		checkConst();
		if (m->args.size() != 1)
			error("reserve() takes exactly one argument (size)", "ArgumentError");
		Value v = eval(m->args[0]);
		switch (target.type) {
		case ValueType::LIST: {
			auto *val = static_cast<ListObject *>(target.ref.get());
			if (v.asInt() <= val->elements.capacity())
				throw RuntimeWarning("Redundant reserve() call: the requested capacity is already allocated.", m->line, m->col);
			val->elements.reserve(v.asInt());
			break;
		}
		case ValueType::SET: {
			auto *val = static_cast<SetObject *>(target.ref.get());
			if (v.asInt() <= static_cast<long>(val->elements.bucket_count() * val->elements.max_load_factor()))
				throw RuntimeWarning("calling reserve() on a map that already has sufficient bucket capacity is redundant", m->line, m->col);
			val->elements.reserve(v.asInt());
			break;
		}
		case ValueType::STRING: {
			auto *val = static_cast<StringObject *>(target.ref.get());
			if (v.asInt() <= val->value.capacity())
				throw RuntimeWarning("Redundant reserve() call: the requested capacity is already allocated.", m->line, m->col);
			val->value.reserve(v.asInt());
			break;
		}
		case ValueType::DICT: {
			auto *val = static_cast<DictObject *>(target.ref.get());
			if (v.asInt() <= static_cast<long>(val->items.bucket_count() * val->items.max_load_factor()))
				throw RuntimeWarning("calling reserve() on a map that already has sufficient bucket capacity is redundant", m->line, m->col);
			val->items.reserve(v.asInt());
			break;
		}
		default:
			error("reserve() can only bee used on Lists, Sets, Dictionaries and Strings", "TypeError");
			break;
		}
		return Value::None();
	}
	//----------------- RESIZE ----------------
	if (m->method == "resize") {
		checkConst();
		if (m->args.size() != 1)
			error("resize() takes exactly one argument (size)", "ArgumentError");
		Value v = eval(m->args[0]);
		switch (target.type) {
		case ValueType::LIST: {
			auto *val = static_cast<ListObject *>(target.ref.get());
			if (v.asInt() < val->elements.size())
				throw DeprecationWarning("Using a value smaller than the length of the conainer might cause loss of data...", m->line, m->col);
			val->elements.resize(v.asInt());
			break;
		}
		case ValueType::STRING: {
			auto *val = static_cast<StringObject *>(target.ref.get());
			if (v.asInt() < val->value.size())
				throw DeprecationWarning("Using a value smaller than the length of the conainer might cause loss of data...", m->line, m->col);
			val->value.resize(v.asInt());
			break;
		}
		default: {
			error("resize() can only bee used on Lists and Strings", "TypeError");
			break;
		}
		}
		return Value::None();
	}
	// ---------------- APPEND ----------------
	if (m->method == "append") {
		checkConst();
		if (m->args.size() != 1)
			error("append() takes exactly one argument", "ArgumentError");
		Value v = eval(m->args[0]);
		if (target.type == ValueType::LIST) {
			auto *list = static_cast<ListObject *>(target.ref.get());
			list->elements.push_back(v);
			return target;
		}
		if (target.type == ValueType::STRING) {
			auto *str = static_cast<StringObject *>(target.ref.get());
			str->value += valueToString(v);
			return target;
		}
		error("append() not supported on this type", "TypeError");
	}
	// ---------------- SORT ----------------
	if (m->method == "sort") {
		if (m->args.size() > 3)
			error("sort() accepts up to 3 arguments (reverse_sort:bool(false), modify_original:bool(true), custom_lambda = None)", "ArgumentError");
		bool reverseSort = false;
		if (m->args.size() >= 1)
			reverseSort = eval(m->args[0]).asBool();
		bool modifyOriginal = true;
		if (m->args.size() >= 2)
			modifyOriginal = eval(m->args[1]).asBool();
		Value customLambda = Value::None();
		if (m->args.size() == 3) {
			customLambda = eval(m->args[2]);
			if (customLambda.type != ValueType::FUNCTION && customLambda.type != ValueType::NONE)
				error("sort() third argument must be a function/lambda or None", "TypeError");
		}
		if (modifyOriginal) {
			checkConst();
		}
		if (target.type == ValueType::STRING) {
			auto *str = static_cast<StringObject *>(target.ref.get());
			string s = str->value;
			if (s.size() > 1) {
				std::sort(s.begin(), s.end());
				if (reverseSort)
					std::reverse(s.begin(), s.end());
			}
			if (modifyOriginal) {
				str->value = s;
				return target;
			}
			return Value::String(s);
		}
		if (target.type == ValueType::LIST) {
			auto *listObj = static_cast<ListObject *>(target.ref.get());
			vector<Value> elems = listObj->elements;
			if (elems.size() > 1) {
				VM tempVM;
				Chunk callChunk;
				if (customLambda.type == ValueType::FUNCTION) {
					tempVM.globals = this->env;
					tempVM.methodResolver = [&](MethodCallExpr *expr) {
						return this->Resolve_methods(expr);
					};
					callChunk.write(OpCode::OP_CALL, m->line, m->col);
					callChunk.write((uint8_t)2, m->line, m->col);
					callChunk.write(OpCode::OP_RETURN, m->line, m->col);
				}
				auto cmp = [&](const Value &a, const Value &b) {
					if (customLambda.type == ValueType::FUNCTION) {
						tempVM.stack.clear();
						tempVM.stack.push_back(a);
						tempVM.stack.push_back(b);
						tempVM.stack.push_back(customLambda);
						tempVM.run(callChunk);
						Value ret = tempVM.stack.empty() ? Value::None() : tempVM.stack.back();
						return ret.isTruthy();
					} else {
						return lessValue(a, b, this->env, mRes, iRes);
					}
				};
				std::sort(elems.begin(), elems.end(), cmp);
				if (reverseSort)
					std::reverse(elems.begin(), elems.end());
			}
			if (modifyOriginal) {
				listObj->elements = std::move(elems);
				return target;
			}
			return Value::List(elems);
		}
		error("sort() only works on mutable types", "TypeError");
	}
	// ---------------- FUNCTIONAL METHODS ----------------
	if (m->method == "All_Of" || m->method == "Any_Of" || m->method == "None_Of" || m->method == "One_Of" || m->method == "find" || m->method == "select" || m->method == "reject" || m->method == "partition" || m->method == "map") {
		if (m->args.size() != 1)
			error(
				m->method + "() expects exactly one argument (a lambda function)",
				"ArgumentError");
		Value lambda = eval(m->args[0]);
		if (lambda.type != ValueType::FUNCTION)
			error(m->method + "() argument must be a function or lambda",
				"TypeError");
		vector<Value> elements;
		if (target.type == ValueType::LIST)
			elements = static_cast<ListObject *>(target.ref.get())->elements;
		else if (target.type == ValueType::SET) {
			auto *s = static_cast<SetObject *>(target.ref.get());
			elements.assign(s->elements.begin(), s->elements.end());
		} else if (target.type == ValueType::TUPLE)
			elements = static_cast<TupleObject *>(target.ref.get())->elements;
		else if (target.type == ValueType::VECTOR) {
			for (auto d : static_cast<VectorObject *>(target.ref.get())->elements)
				elements.push_back(d);
		} else if (target.type == ValueType::DICT) {
			auto *d = static_cast<DictObject *>(target.ref.get());
			for (auto &pair : d->items)
				elements.push_back(pair.first); // Iterate keys
		} else if (target.type == ValueType::STRING) {
			string &s = static_cast<StringObject *>(target.ref.get())->value;
			for (char c : s)
				elements.push_back(Value::String(string(1, c)));
		} else
			error("Method '" + m->method + "' requires a container (List, Set, Tuple, Vector, Dict, or " + "String)", "TypeError");
		vector<Value> resultsTrue;
		vector<Value> resultsFalse;
		vector<Value> resultsMap;
		int matchCount = 0;
		for (const auto &elem : elements) {
			Chunk tempChunk;
			int lambdaIdx = tempChunk.addConstant(lambda);
			int argIdx = tempChunk.addConstant(elem);
			tempChunk.write(OpCode::OP_CONSTANT, m->line, m->col);
			tempChunk.write((uint8_t)argIdx, m->line, m->col);
			tempChunk.write(OpCode::OP_CONSTANT, m->line, m->col);
			tempChunk.write((uint8_t)lambdaIdx, m->line, m->col);
			tempChunk.write(OpCode::OP_CALL, m->line, m->col);
			tempChunk.write((uint8_t)1, m->line, m->col);
			tempChunk.write(OpCode::OP_RETURN, m->line, m->col);
			VM tempVM;
			tempVM.globals = this->env;
			tempVM.methodResolver = [&](MethodCallExpr *m) {
				return this->Resolve_methods(m);
			};
			tempVM.run(tempChunk);
			Value ret = tempVM.stack.empty() ? Value::None() : tempVM.stack.back();
			bool matches = ret.isTruthy();
			if (m->method == "All_Of") {
				if (!matches)
					return Value::Bool(false);
			} else if (m->method == "Any_Of") {
				if (matches)
					return Value::Bool(true);
			} else if (m->method == "None_Of") {
				if (matches)
					return Value::Bool(false);
			} else if (m->method == "One_Of") {
				if (matches) {
					matchCount++;
					if (matchCount > 1)
						return Value::Bool(false);
				}
			} else if (m->method == "find") {
				if (matches)
					return elem;
			} else if (m->method == "select") {
				if (matches)
					resultsTrue.push_back(elem);
			} else if (m->method == "reject") {
				if (!matches)
					resultsTrue.push_back(elem);
			} else if (m->method == "partition") {
				if (matches)
					resultsTrue.push_back(elem);
				else
					resultsFalse.push_back(elem);
			} else if (m->method == "map") {
				resultsMap.push_back(ret);
			}
		}
		if (m->method == "All_Of")
			return Value::Bool(true);
		if (m->method == "Any_Of")
			return Value::Bool(false);
		if (m->method == "None_Of")
			return Value::Bool(true);
		if (m->method == "One_Of")
			return Value::Bool(matchCount == 1);
		if (m->method == "find")
			return Value::None();
		auto reconstruct = [&](const vector<Value> &src) -> Value {
			if (target.type == ValueType::LIST || target.type == ValueType::DICT)
				return Value::List(src);
			if (target.type == ValueType::TUPLE)
				return Value::Tuple(src);
			if (target.type == ValueType::SET)
				return Value::Set([&src]() -> std::unordered_set<Value, ValueHash, ValueEqual> {
					std::unordered_set<Value, ValueHash, ValueEqual> s;
					for (auto &v : src)
						s.insert(v);
					return s;
				}());
			if (target.type == ValueType::VECTOR) {
				vector<Value> nums;
				bool allNums = true;
				for (auto &v : src) {
					if (!v.isNumber()) {
						allNums = false;
						break;
					}
					nums.push_back(v);
				}
				if (allNums)
					return Value::Vector(nums);
				return Value::List(src);
			}
			if (target.type == ValueType::STRING) {
				string s = "";
				bool allStr = true;
				for (auto &v : src) {
					if (v.type != ValueType::STRING) {
						allStr = false;
						break;
					}
					s += v.asString();
				}
				if (allStr)
					return Value::String(s);
				return Value::List(src);
			}
			return Value::List(src);
		};
		if (m->method == "select" || m->method == "reject")
			return reconstruct(resultsTrue);
		if (m->method == "partition")
			return Value::List(
				{reconstruct(resultsTrue), reconstruct(resultsFalse)});
		if (m->method == "map") {
			if (target.type == ValueType::DICT)
				return Value::List(resultsMap);
			return reconstruct(resultsMap);
		}
	}
	// ------- STRING TRANSFORMATIONS -------
	if (target.type == ValueType::STRING) {
		if (m->method == "ascii") {
			if (!m->args.empty())
				error("ascii() does not accept arguments", "ArgumentError");
			auto *str = static_cast<StringObject *>(target.ref.get());
			if (str->value.empty())
				return target;
			if (str->value.size() != 1)
				error("string size cannot exceed 1", "ArgumentError");
			return Value::Int(static_cast<int>(str->value[0]));
		}
		if (m->method == "capitalize") {
			if (m->args.size() > 1)
				error("capitalize() accepts at most 1 argument (modify_original:bool(true))", "ArgumentError");
			bool modifyOriginal = true;
			if (m->args.size() == 1) {
				modifyOriginal = eval(m->args[0]).asBool();
			}
			if (modifyOriginal) {
				checkConst();
			}
			auto *strObj = static_cast<StringObject *>(target.ref.get());
			string result = capitalize(strObj->value);
			if (modifyOriginal) {
				strObj->value = result;
				return target;
			}
			return Value::String(result);
		}
		if (m->method == "chars") {
			auto *str = static_cast<StringObject *>(target.ref.get());
			int start = 0;
			int end = -1;
			if (m->args.size() >= 1)
				start = eval(m->args[0]).asInt();
			if (m->args.size() >= 2)
				end = eval(m->args[1]).asInt();
			std::vector<std::string> parts = chars(str->value, start, end);
			std::vector<Value> valList;
			valList.reserve(parts.size());
			for (const auto &p : parts)
				valList.push_back(Value::String(p));
			return Value::List(valList);
		}
		if (m->method == "casefold") {
			if (m->args.size() > 1)
				error("casefold() accepts at most 1 argument (modify_original:bool(true))", "ArgumentError");
			bool modifyOriginal = true;
			if (m->args.size() == 1) {
				modifyOriginal = eval(m->args[0]).asBool();
			}
			if (modifyOriginal) {
				checkConst();
			}
			auto *strObj = static_cast<StringObject *>(target.ref.get());
			string result = casefold(strObj->value);
			if (modifyOriginal) {
				strObj->value = result;
				return target;
			}
			return Value::String(result);
		}
		if (m->method == "center") {
			if (m->args.empty() || m->args.size() > 3)
				error("center() needs 1 to 3 arguments (width, fillchar:string(' '), modify_original:bool(true))", "ArgumentError");
			int width = eval(m->args[0]).asInt();
			bool useFillChar = false;
			char fillChar = ' ';
			bool modifyOriginal = true;
			if (m->args.size() >= 2) {
				Value arg1 = eval(m->args[1]);
				if (arg1.type == ValueType::BOOL) {
					modifyOriginal = arg1.asBool();
				} else {
					string a = arg1.asString();
					if (a.empty() || a.size() > 1)
						error("padding can only be one character", "ValueError");
					fillChar = a[0];
					useFillChar = true;
				}
			}
			if (m->args.size() == 3) {
				modifyOriginal = eval(m->args[2]).asBool();
			}
			if (modifyOriginal) {
				checkConst();
			}
			auto *strObj = static_cast<StringObject *>(target.ref.get());
			string result;
			if (useFillChar) {
				result = center(strObj->value, width, fillChar);
			} else {
				result = center(strObj->value, width);
			}
			if (modifyOriginal) {
				strObj->value = result;
				return target;
			}
			return Value::String(result);
		}
		if (m->method == "count") {
			if (m->args.empty() || m->args.size() > 3)
				error("count() needs 1 to 3 arguments", "ArgumentError");
			auto *str = static_cast<StringObject *>(target.ref.get());
			switch (m->args.size()) {
			case 1:
				return Value::Int(
					::count(str->value, eval(m->args[0]).asString()));
			case 2:
				if (eval(m->args[1]).asInt() < 0)
					error("starting position cannot be negative", "ValueError");
				return Value::Int(::count(str->value,
					eval(m->args[0]).asString(),
					eval(m->args[1]).asInt()));
			case 3:
				if (eval(m->args[1]).asInt() < 0)
					error("starting position cannot be negative", "ValueError");
				return Value::Int(
					::count(str->value, eval(m->args[0]).asString(),
						eval(m->args[1]).asInt(), eval(m->args[2]).asInt()));
			}
		}
		if (m->method == "endswith") {
			if (m->args.empty() || m->args.size() > 1)
				error("endswith() needs one argument", "ArgumentError");
			auto *str = static_cast<StringObject *>(target.ref.get());
			return Value::Bool(endswith(str->value, eval(m->args[0]).asString()));
		}
		if (m->method == "index") {
			if (m->args.empty() || m->args.size() > 1)
				error("index() needs one argument", "ArgumentError");
			auto *str = static_cast<StringObject *>(target.ref.get());
			return Value::Int(index(str->value, eval(m->args[0]).asString()));
		}
		if (m->method == "isalnum") {
			if (!m->args.empty())
				error("isalnum() does not accept arguments", "ArgumentError");
			auto *str = static_cast<StringObject *>(target.ref.get());
			return Value::Bool(isalnum_str(str->value));
		}
		if (m->method == "isalpha") {
			if (!m->args.empty())
				error("isalpha() does not accept arguments", "ArgumentError");
			auto *str = static_cast<StringObject *>(target.ref.get());
			return Value::Bool(isalpha_str(str->value));
		}
		if (m->method == "isdecimal") {
			if (!m->args.empty())
				error("isdecimal() does not accept arguments", "ArgumentError");
			auto *str = static_cast<StringObject *>(target.ref.get());
			return Value::Bool(isdecimal_str(str->value));
		}
		if (m->method == "islower") {
			if (!m->args.empty())
				error("islower() does not accept arguments", "ArgumentError");
			auto *str = static_cast<StringObject *>(target.ref.get());
			return Value::Bool(islower_str(str->value));
		}
		if (m->method == "isupper") {
			if (!m->args.empty())
				error("isupper() does not accept arguments", "ArgumentError");
			auto *str = static_cast<StringObject *>(target.ref.get());
			return Value::Bool(isupper_str(str->value));
		}
		if (m->method == "ljust") {
			if (m->args.empty() || m->args.size() > 3)
				error("ljust() needs 1 to 3 arguments (width, fillchar, modify_original)", "ArgumentError");
			int width = eval(m->args[0]).asInt();
			bool useFillChar = false;
			char fillChar = ' ';
			bool modifyOriginal = true;
			if (m->args.size() >= 2) {
				Value arg1 = eval(m->args[1]);
				if (arg1.type == ValueType::BOOL) {
					modifyOriginal = arg1.asBool();
				} else {
					string p = arg1.asString();
					if (p.size() != 1)
						error("padding can only be one character", "ValueError");
					fillChar = p[0];
					useFillChar = true;
				}
			}
			if (m->args.size() == 3) {
				modifyOriginal = eval(m->args[2]).asBool();
			}
			if (modifyOriginal) {
				checkConst();
			}
			auto *strObj = static_cast<StringObject *>(target.ref.get());
			string result;
			if (useFillChar) {
				result = ljust(strObj->value, width, fillChar);
			} else {
				result = ljust(strObj->value, width);
			}
			if (modifyOriginal) {
				strObj->value = result;
				return target;
			}
			return Value::String(result);
		}
		if (m->method == "lower") {
			if (m->args.size() > 1)
				error("lower() accepts at most 1 argument (modify_original:bool)", "ArgumentError");
			bool modifyOriginal = true;
			if (m->args.size() == 1) {
				modifyOriginal = eval(m->args[0]).asBool();
			}
			if (modifyOriginal) {
				checkConst();
			}
			auto *strObj = static_cast<StringObject *>(target.ref.get());
			string result = lower(strObj->value);
			if (modifyOriginal) {
				strObj->value = result;
				return target;
			}
			return Value::String(result);
		}
		if (m->method == "lstrip") {
			if (m->args.size() > 2)
				error("lstrip() takes at most 2 arguments (chars, modify_original)", "ArgumentError");
			string chars = " \t\n\r\v\f";
			bool modifyOriginal = true;
			if (m->args.size() >= 1) {
				Value arg0 = eval(m->args[0]);
				if (arg0.type == ValueType::BOOL) {
					modifyOriginal = arg0.asBool();
				} else {
					chars = arg0.asString();
				}
			}
			if (m->args.size() == 2) {
				modifyOriginal = eval(m->args[1]).asBool();
			}
			if (modifyOriginal) {
				checkConst();
			}
			auto *strObj = static_cast<StringObject *>(target.ref.get());
			string result = lstrip(strObj->value, chars);
			if (modifyOriginal) {
				strObj->value = result;
				return target;
			}
			return Value::String(result);
		}
		if (m->method == "rstrip") {
			if (m->args.size() > 2)
				error("rstrip() takes at most 2 arguments (chars, modify_original)", "ArgumentError");
			string chars = " \t\n\r\v\f";
			bool modifyOriginal = true;
			if (m->args.size() >= 1) {
				Value arg0 = eval(m->args[0]);
				if (arg0.type == ValueType::BOOL) {
					modifyOriginal = arg0.asBool();
				} else {
					chars = arg0.asString();
				}
			}
			if (m->args.size() == 2) {
				modifyOriginal = eval(m->args[1]).asBool();
			}
			if (modifyOriginal) {
				checkConst();
			}
			auto *strObj = static_cast<StringObject *>(target.ref.get());
			string result = rstrip(strObj->value, chars);
			if (modifyOriginal) {
				strObj->value = result;
				return target;
			}
			return Value::String(result);
		}
		if (m->method == "rjust") {
			if (m->args.empty() || m->args.size() > 3)
				error("rjust() needs 1 to 3 arguments (width, fillchar, modify_original)", "ArgumentError");
			int width = eval(m->args[0]).asInt();
			bool useFillChar = false;
			char fillChar = ' ';
			bool modifyOriginal = true;
			if (m->args.size() >= 2) {
				Value arg1 = eval(m->args[1]);
				if (arg1.type == ValueType::BOOL) {
					modifyOriginal = arg1.asBool();
				} else {
					string p = arg1.asString();
					if (p.size() != 1)
						error("padding can only be one character", "ValueError");
					fillChar = p[0];
					useFillChar = true;
				}
			}
			if (m->args.size() == 3) {
				modifyOriginal = eval(m->args[2]).asBool();
			}
			if (modifyOriginal) {
				checkConst(); // Added this since it was missing in your original code!
			}
			auto *strObj = static_cast<StringObject *>(target.ref.get());
			string result;
			if (useFillChar) {
				result = rjust(strObj->value, width, fillChar);
			} else {
				result = rjust(strObj->value, width, ' ');
			}
			if (modifyOriginal) {
				strObj->value = result;
				return target;
			}
			return Value::String(result);
		}
		if (m->method == "strip") {
			if (m->args.size() > 2)
				error("strip() takes at most 2 arguments (chars, modify_original)", "ArgumentError");
			string chars = " \t\n\r\v\f";
			bool modifyOriginal = true;
			if (m->args.size() >= 1) {
				Value arg0 = eval(m->args[0]);
				if (arg0.type == ValueType::BOOL) {
					modifyOriginal = arg0.asBool();
				} else {
					chars = arg0.asString();
				}
			}
			if (m->args.size() == 2) {
				modifyOriginal = eval(m->args[1]).asBool();
			}
			if (modifyOriginal) {
				checkConst();
			}
			auto *strObj = static_cast<StringObject *>(target.ref.get());
			string result = strip(strObj->value, chars);
			if (modifyOriginal) {
				strObj->value = result;
				return target;
			}
			return Value::String(result);
		}
		if (m->method == "split") {
			string delimiter = " ";
			if (!m->args.empty()) {
				if (m->args.size() > 1)
					error("split() expects at most 1 argument", "ArgumentError");
				Value delimVal = eval(m->args[0]);
				if (delimVal.type != ValueType::STRING)
					error("split() delimiter must be a string", "TypeError");
				delimiter = delimVal.asString();
			}
			vector<string> parts = split(target.asString(), delimiter);
			vector<Value> resultList;
			resultList.reserve(parts.size());
			for (const auto &part : parts)
				resultList.push_back(Value::String(part));
			return Value::List(resultList);
		}
		if (m->method == "upper") {
			if (m->args.size() > 1)
				error("upper() accepts at most 1 argument (modify_original:bool)", "ArgumentError");
			bool modifyOriginal = true;
			if (m->args.size() == 1) {
				modifyOriginal = eval(m->args[0]).asBool();
			}
			if (modifyOriginal) {
				checkConst();
			}
			auto *strObj = static_cast<StringObject *>(target.ref.get());
			string result = upper(strObj->value);
			if (modifyOriginal) {
				strObj->value = result;
				return target;
			}
			return Value::String(result);
		}
		error("Object '" + m->method + "' is not a string method", "AttributeError");
	}
	// ---------- RANGE METHODS ----------
	if (target.type == ValueType::RANGE) {
		if (m->method == "min") {
			if (!m->args.empty())
				error("min() does not accept arguments", "ArgumentError");
			auto *rang = static_cast<RangeObject *>(target.ref.get());
			if (!rang->isValid)
				error("min() arg is an empty range", "ValueError");
			return Value::Int(
				rang->step < 0
					? (rang->endInclusive ? rang->end : rang->end - rang->step)
					: (rang->startInclusive ? rang->start
													: rang->start + rang->step));
		}
		if (m->method == "max") {
			if (!m->args.empty())
				error("max() does not accept arguments", "ArgumentError");
			auto *rang = static_cast<RangeObject *>(target.ref.get());
			if (!rang->isValid)
				error("max() arg is an empty range", "ValueError");

			return Value::Int(
				rang->step < 0
					? (rang->startInclusive ? rang->start
													: rang->start + rang->step)
					: (rang->endInclusive ? rang->end : rang->end - rang->step));
		}
		if (m->method == "step") {
			if (!m->args.empty())
				error("step() does not accept arguments", "ArgumentError");
			auto *rang = static_cast<RangeObject *>(target.ref.get());
			if (!rang->isValid)
				error("step() arg is an empty range", "ValueError");
			return Value::Int(rang->step);
		}
		error("Object '" + m->method + "' is not a range method",
			"AttributeError");
	}
	// ---------------- LIST METHODS ----------------
	if (target.type == ValueType::LIST) {
		auto *listObj = static_cast<ListObject *>(target.ref.get());
		auto &elems = listObj->elements;
		if (m->method == "get") {
			if (m->args.size() != 1 && m->args.size() != 2)
				error(
					"list.get() takes one mandatory argument index, and an "
					"optional "
					"argument default_return",
					"ArgumentError");
			auto idx = eval(m->args[0]).asInt();
			if (idx < 0 && elems.size() > elems.size() + idx)
				return elems[idx + elems.size()];
			else if (idx >= 0 and elems.size() > idx)
				return elems[idx];
			if (m->args.size() == 2)
				return eval(m->args[1]);
			else
				return Value::None();
		}
		if (m->method == "count") {
			if (m->args.size() != 1)
				error("list.count() takes exactly one argument", "ArgumentError");
			Value needle = eval(m->args[0]);
			long long c = 0;
			for (const auto &el : elems) {
				if (el.strictEquals(needle))
					c++;
			}
			return Value::Int(c);
		}
		if (m->method == "index") {
			if (m->args.size() != 1)
				error("list.index() takes exactly one argument", "ArgumentError");
			Value needle = eval(m->args[0]);
			for (size_t i = 0; i < elems.size(); i++) {
				if (elems[i].strictEquals(needle))
					return Value::Int((long long)i);
			}
			return Value::Int(-1);
		}
		if (m->method == "insert") {
			checkConst();
			if (m->args.size() != 2)
				error("insert() takes exactly two arguments (index, value)",
					"ArgumentError");
			int idx = eval(m->args[0]).asInt();
			Value val = eval(m->args[1]);
			if (idx < 0 || idx > (int)elems.size())
				error("Index out of bounds", "IndexError");
			elems.insert(elems.begin() + idx, val);
			return target;
		}
		if (m->method == "pop") {
			checkConst();
			if (m->args.size() > 1)
				error("pop() takes at most one argument", "ArgumentError");
			if (elems.empty())
				error("pop from empty list", "EmptyContainerError");
			int idx = elems.size() - 1;
			if (!m->args.empty())
				idx = eval(m->args[0]).asInt();
			if (idx < 0 || idx >= (int)elems.size())
				error("pop index out of bounds", "IndexError");
			Value val = elems[idx];
			elems.erase(elems.begin() + idx);
			return val;
		}
		if (m->method == "remove") {
			checkConst();
			if (m->args.size() != 1)
				error("remove() takes exactly one argument", "ArgumentError");
			Value val = eval(m->args[0]);
			for (auto it = elems.begin(); it != elems.end(); ++it) {
				if (it->strictEquals(val)) {
					elems.erase(it);
					return Value::None();
				}
			}
			error("list.remove(x): x not in list", "ValueError");
		}
		if (m->method == "clear") {
			checkConst();
			elems.clear();
			return target;
		}
		if (m->method == "extend") {
			checkConst();
			if (m->args.size() != 1)
				error("extend() takes exactly one argument", "ArgumentError");
			Value other = eval(m->args[0]);
			if (other.type == ValueType::LIST) {
				auto *o = static_cast<ListObject *>(other.ref.get());
				elems.insert(elems.end(), o->elements.begin(), o->elements.end());
			} else if (other.type == ValueType::SET) {
				auto *o = static_cast<SetObject *>(other.ref.get());
				elems.insert(elems.end(), o->elements.begin(), o->elements.end());
			} else if (other.type == ValueType::RANGE) {
				auto *r = static_cast<RangeObject *>(other.ref.get());
				double current = r->start;
				if (!r->startInclusive)
					current += r->step;
				while (true) {
					bool cond = (r->step > 0) ? (r->endInclusive ? current <= r->end : current < r->end) : (r->endInclusive ? current >= r->end : current > r->end);
					if (!cond)
						break;
					elems.push_back(r->isFloat ? Value::Float(current) : Value::Int((long long)current));
					current += r->step;
				}
			} else
				error("extend() requires an iterable (list, set, or range)", "TypeError");
			return target;
		}
		if (m->method == "sum") {
			double total = 0;
			bool isFloat = false;
			for (const auto &el : elems) {
				if (el.type == ValueType::INT)
					total += el.asInt();
				else if (el.type == ValueType::FLOAT) {
					total += el.asFloat();
					isFloat = true;
				} else
					error("sum() requires numeric values", "TypeError");
			}
			return isFloat ? Value::Float(total) : Value::Int((long long)total);
		}
		if (m->method == "min") {
			if (elems.empty())
				error("min() on empty list", "ValueError");
			Value minVal = elems[0];
			for (size_t i = 1; i < elems.size(); i++) {
				bool smaller = false;
				if (elems[i].type == ValueType::INT &&
					 minVal.type == ValueType::INT)
					smaller = elems[i].asInt() < minVal.asInt();
				else if (elems[i].type == ValueType::FLOAT ||
							minVal.type == ValueType::FLOAT)
					smaller = elems[i].asFloat() < minVal.asFloat();
				else if (elems[i].type == ValueType::STRING &&
							minVal.type == ValueType::STRING)
					smaller = elems[i].asString() < minVal.asString();
				if (smaller)
					minVal = elems[i];
			}
			return minVal;
		}
		if (m->method == "max") {
			if (elems.empty())
				error("max() on empty list", "ValueError");
			Value maxVal = elems[0];
			for (size_t i = 1; i < elems.size(); i++) {
				bool larger = false;
				if (elems[i].type == ValueType::INT &&
					 maxVal.type == ValueType::INT)
					larger = elems[i].asInt() > maxVal.asInt();
				else if (elems[i].type == ValueType::FLOAT ||
							maxVal.type == ValueType::FLOAT)
					larger = elems[i].asFloat() > maxVal.asFloat();
				else if (elems[i].type == ValueType::STRING &&
							maxVal.type == ValueType::STRING)
					larger = elems[i].asString() > maxVal.asString();

				if (larger)
					maxVal = elems[i];
			}
			return maxVal;
		}
		if (m->method == "average") {
			if (elems.empty())
				return Value::Float(0);
			double total = 0;
			for (const auto &el : elems)
				total += el.asFloat();
			return Value::Float(total / elems.size());
		}
		if (m->method == "shuffle") {
			checkConst();
			if (elems.empty())
				return Value::None();
			static std::random_device rd;
			static std::mt19937 gen(rd());
			std::shuffle(elems.begin(), elems.end(), gen);
			return target;
		}
		if (m->method == "sample") {
			int k = eval(m->args[0]).asInt();
			if (k > (int)elems.size())
				error("Sample larger than population", "ValueError");
			static std::random_device rd;
			static std::mt19937 gen(rd());
			vector<Value> result = elems;
			for (int i = 0; i < k; i++) {
				std::uniform_int_distribution<> dis(i, (int)result.size() - 1);
				int j = dis(gen);
				std::swap(result[i], result[j]);
			}
			result.resize(k);
			return Value::List(result);
		}
		if (m->method == "flatten") {
			checkConst();
			vector<Value> flatResult;
			std::function<void(const vector<Value> &)> recursiveFlatten;
			recursiveFlatten = [&](const vector<Value> &currentElems) {
				for (const auto &el : currentElems) {
					if (el.type == ValueType::LIST) {
						auto *sub = static_cast<ListObject *>(el.ref.get());
						recursiveFlatten(sub->elements);
					} else
						flatResult.push_back(el);
				}
			};
			recursiveFlatten(elems);
			elems = flatResult;
			return target;
		}
		if (m->method == "chunk") {
			int size = eval(m->args[0]).asInt();
			if (size <= 0)
				error("Chunk size must be > 0", "ValueError");
			vector<Value> chunks;
			vector<Value> current;
			for (const auto &el : elems) {
				current.push_back(el);
				if (current.size() == size) {
					chunks.push_back(Value::List(current));
					current.clear();
				}
			}
			if (!current.empty())
				chunks.push_back(Value::List(current));
			return Value::List(chunks);
		}
		if (m->method == "rotate") {
			checkConst();
			if (elems.empty())
				return Value::None();
			int n = eval(m->args[0]).asInt();
			n %= (int)elems.size();
			if (n < 0)
				n += elems.size();
			std::rotate(elems.rbegin(), elems.rbegin() + n, elems.rend());
			return target;
		}
		if (m->method == "unique") {
			checkConst();
			vector<Value> unique;
			for (const auto &item : elems) {
				bool exists = false;
				for (const auto &u : unique)
					if (u.strictEquals(item)) {
						exists = true;
						break;
					}
				if (!exists)
					unique.push_back(item);
			}
			elems = unique;
			return target;
		}
		if (m->method == "join") {
			string sep = "";
			if (!m->args.empty())
				sep = eval(m->args[0]).asString();
			string res = "";
			for (size_t i = 0; i < elems.size(); i++) {
				if (elems[i].type == ValueType::STRING)
					res += elems[i].asString();
				else
					res += valueToString(elems[i]);
				if (i + 1 < elems.size())
					res += sep;
			}
			return Value::String(res);
		}
		if (m->method == "fill") {
			checkConst();
			Value val = eval(m->args[0]);
			if (m->args.size() > 1) {
				int count = eval(m->args[1]).asInt();
				if (count < 0)
					count = 0;
				elems.clear();
				elems.reserve(count);
				for (int i = 0; i < count; i++)
					elems.push_back(deepCopy(val));
			} else
				for (auto &el : elems)
					el = deepCopy(val);
			return target;
		}
		if (m->method == "MaxHeapify") {
			checkConst();
			if (!m->args.empty())
				error("MaxHeapify() takes no arguments", "ArgumentError");
			std::make_heap(elems.begin(), elems.end(),
				[&](const Value &a, const Value &b) { return lessValue(a, b, this->env, mRes, iRes); });
			return target;
		}
		if (m->method == "MinHeapify") {
			checkConst();
			if (!m->args.empty())
				error("MinHeapify() takes no arguments", "ArgumentError");
			std::make_heap(elems.begin(), elems.end(),
				[&](const Value &a, const Value &b) { return lessValue(b, a, this->env, mRes, iRes); });
			return target;
		}
		if (m->method == "MaxHeapPop") {
			checkConst();
			if (!m->args.empty())
				throw ArgumentError("MaxHeapPop() takes no arguments", m->line, m->col);
			if (elems.empty())
				throw EmptyContainerError("pop from empty heap", m->line, m->col);
			std::pop_heap(elems.begin(), elems.end(),
				[&](const Value &a, const Value &b) { return lessValue(a, b, this->env, mRes, iRes); });
			auto result = elems.back();
			elems.pop_back();
			return result;
		}
		if (m->method == "MinHeapPop") {
			checkConst();
			if (!m->args.empty())
				throw ArgumentError("MinHeapPop() takes no arguments", m->line, m->col);
			if (elems.empty())
				throw EmptyContainerError("pop from empty heap", m->line, m->col);
			std::pop_heap(elems.begin(), elems.end(),
				[&](const Value &a, const Value &b) { return lessValue(b, a, this->env, mRes, iRes); });
			auto result = elems.back();
			elems.pop_back();
			return result;
		}
		if (m->method == "MaxHeapPush") {
			checkConst();
			if (m->args.size() != 1)
				throw ArgumentError("MaxHeapPush() takes one argument", m->line, m->col);
			elems.push_back(eval(m->args[0]));
			std::push_heap(elems.begin(), elems.end(),
				[&](const Value &a, const Value &b) { return lessValue(a, b, this->env, mRes, iRes); });
			return target;
		}
		if (m->method == "MinHeapPush") {
			checkConst();
			if (m->args.size() != 1)
				throw ArgumentError("MinHeapPush() takes one argument", m->line, m->col);
			elems.push_back(eval(m->args[0]));
			std::push_heap(elems.begin(), elems.end(),
				[&](const Value &a, const Value &b) { return lessValue(b, a, this->env, mRes, iRes); });
			return target;
		}
		error("Object '" + m->method + "' is not a list method", "AttributeError");
	}
	//--------- SET METHODS ----------
	if (target.type == ValueType::SET) {
		auto *setObj = static_cast<SetObject *>(target.ref.get());
		//--- MODIFIERS ---
		if (m->method == "add") {
			checkConst();
			if (m->args.size() != 1)
				error("add() takes exactly one argument", "ArgumentError");
			setAdd(setObj->elements, eval(m->args[0]));
			return target;
		}
		if (m->method == "remove") {
			checkConst();
			if (m->args.size() != 1)
				error("remove() takes exactly one argument", "ArgumentError");
			Value val = eval(m->args[0]);
			auto &elems = setObj->elements;
			for (auto it = elems.begin(); it != elems.end(); ++it) {
				if (it->strictEquals(val)) {
					elems.erase(it);
					return target;
				}
			}
			error("KeyError: element not found in set", "KeyError");
		}
		if (m->method == "discard") {
			checkConst();
			if (m->args.size() != 1)
				error("discard() takes exactly one argument", "ArgumentError");
			Value val = eval(m->args[0]);
			auto &elems = setObj->elements;
			for (auto it = elems.begin(); it != elems.end(); ++it) {
				if (it->strictEquals(val)) {
					elems.erase(it);
					return target;
				}
			}
			return target;
		}
		if (m->method == "pop") {
			checkConst();
			if (!m->args.empty())
				error("pop() takes no arguments", "ArgumentError");
			if (setObj->elements.empty())
				error("pop from empty set", "EmptyContainerError");
			auto it = setObj->elements.begin();
			Value val = *it;
			setObj->elements.erase(it);
			return val;
		}
		if (m->method == "clear") {
			checkConst();
			if (!m->args.empty())
				error("clear() takes no arguments", "ArgumentError");
			setObj->elements.clear();
			return target;
		}
		//--- OPERATIONS ---
		if (m->method == "union") {
			if (m->args.size() != 1)
				error("union() takes one argument", "ArgumentError");
			Value other = eval(m->args[0]);
			if (other.type != ValueType::SET)
				error("union() requires a set", "TypeError");
			std::unordered_set<Value, ValueHash, ValueEqual> result = setObj->elements;
			auto *otherSet = static_cast<SetObject *>(other.ref.get());
			result.insert(otherSet->elements.begin(), otherSet->elements.end());
			return Value::Set(result);
		}
		if (m->method == "intersection") {
			if (m->args.size() != 1)
				error("intersection() takes one argument", "ArgumentError");
			Value other = eval(m->args[0]);
			if (other.type != ValueType::SET)
				error("intersection() requires a set", "TypeError");
			std::unordered_set<Value, ValueHash, ValueEqual> result;
			auto *otherSet = static_cast<SetObject *>(other.ref.get());
			for (const auto &v : setObj->elements) {
				if (otherSet->elements.count(v) > 0) {
					result.insert(v);
				}
			}
			return Value::Set(result);
		}
		if (m->method == "difference") {
			if (m->args.size() != 1)
				error("difference() takes one argument", "ArgumentError");
			Value other = eval(m->args[0]);
			if (other.type != ValueType::SET)
				error("difference() requires a set", "TypeError");
			std::unordered_set<Value, ValueHash, ValueEqual> result;
			auto *otherSet = static_cast<SetObject *>(other.ref.get());
			for (const auto &v : setObj->elements) {
				if (otherSet->elements.count(v) == 0) {
					result.insert(v);
				}
			}
			return Value::Set(result);
		}
		if (m->method == "symmetric_difference") {
			if (m->args.size() != 1)
				error("symmetric_difference() takes one argument", "ArgumentError");
			Value other = eval(m->args[0]);
			if (other.type != ValueType::SET)
				error("symmetric_difference() requires a set", "TypeError");
			std::unordered_set<Value, ValueHash, ValueEqual> result;
			auto *s2 = static_cast<SetObject *>(other.ref.get());
			for (const auto &v1 : setObj->elements) {
				if (s2->elements.count(v1) == 0)
					result.insert(v1);
			}
			for (const auto &v2 : s2->elements) {
				if (setObj->elements.count(v2) == 0)
					result.insert(v2);
			}
			return Value::Set(result);
		}
		if (m->method == "issubset") {
			if (m->args.size() != 1)
				error("issubset() takes one argument", "ArgumentError");
			Value other = eval(m->args[0]);
			if (other.type != ValueType::SET)
				error("issubset() requires a set", "TypeError");
			auto *parent = static_cast<SetObject *>(other.ref.get());
			for (const auto &childElem : setObj->elements) {
				if (parent->elements.count(childElem) == 0) {
					return Value::Bool(false);
				}
			}
			return Value::Bool(true);
		}
		if (m->method == "issuperset") {
			if (m->args.size() != 1)
				error("issuperset() takes one argument", "ArgumentError");
			Value other = eval(m->args[0]);
			if (other.type != ValueType::SET)
				error("issuperset() requires a set", "TypeError");
			auto *child = static_cast<SetObject *>(other.ref.get());
			for (const auto &cElem : child->elements) {
				if (setObj->elements.count(cElem) == 0) {
					return Value::Bool(false);
				}
			}
			return Value::Bool(true);
		}
		if (m->method == "isdisjoint") {
			if (m->args.size() != 1)
				error("isdisjoint() takes one argument", "ArgumentError");
			Value other = eval(m->args[0]);
			if (other.type != ValueType::SET)
				error("isdisjoint() requires a set", "TypeError");
			auto *otherSet = static_cast<SetObject *>(other.ref.get());
			for (const auto &v : setObj->elements) {
				if (otherSet->elements.count(v) > 0) {
					return Value::Bool(false);
				}
			}
			return Value::Bool(true);
		}
		if (m->method == "join") {
			string sep = "";
			if (!m->args.empty())
				sep = eval(m->args[0]).asString();
			string res = "";
			size_t i = 0;
			size_t total = setObj->elements.size();
			for (const auto &elem : setObj->elements) {
				if (elem.type == ValueType::STRING)
					res += elem.asString();
				else
					res += valueToString(elem);
				if (i + 1 < total)
					res += sep;
				i++;
			}
			return Value::String(res);
		}
		error("Object '" + m->method + "' is not a set method", "AttributeError");
	}
	//-------- TUPLE METHODS ---------
	if (target.type == ValueType::TUPLE) {
		auto *tObj = static_cast<TupleObject *>(target.ref.get());
		if (m->method == "count") {
			if (m->args.size() != 1)
				error("tuple.count() takes 1 arg", "ArgumentError");
			Value needle = eval(m->args[0]);
			long long c = 0;
			for (const auto &el : tObj->elements)
				if (el.strictEquals(needle))
					c++;
			return Value::Int(c);
		}
		if (m->method == "index") {
			if (m->args.size() != 1)
				error("tuple.index() takes 1 arg", "ArgumentError");
			Value needle = eval(m->args[0]);
			for (size_t i = 0; i < tObj->elements.size(); i++) {
				if (tObj->elements[i].strictEquals(needle))
					return Value::Int((long long)i);
			}
			return Value::Int(-1);
		}
		if (m->method == "join") {
			string sep = "";
			if (!m->args.empty())
				sep = eval(m->args[0]).asString();
			string res = "";
			auto &elems = tObj->elements;
			for (size_t i = 0; i < elems.size(); i++) {
				if (elems[i].type == ValueType::STRING)
					res += elems[i].asString();
				else
					res += valueToString(elems[i]);
				if (i + 1 < elems.size())
					res += sep;
			}
			return Value::String(res);
		}
		auto elems = tObj->elements;
		if (m->method == "sum") {
			double total = 0;
			bool isFloat = false;
			for (const auto &el : elems) {
				if (el.type == ValueType::INT)
					total += el.asInt();
				else if (el.type == ValueType::FLOAT) {
					total += el.asFloat();
					isFloat = true;
				} else
					error("sum() requires numeric values", "TypeError");
			}
			return isFloat ? Value::Float(total) : Value::Int((long long)total);
		}
		if (m->method == "min") {
			if (elems.empty())
				error("min() on empty tuple", "ValueError");
			Value minVal = elems[0];
			for (size_t i = 1; i < elems.size(); i++) {
				bool smaller = false;
				if (elems[i].type == ValueType::INT &&
					 minVal.type == ValueType::INT)
					smaller = elems[i].asInt() < minVal.asInt();
				else if (elems[i].type == ValueType::FLOAT ||
							minVal.type == ValueType::FLOAT)
					smaller = elems[i].asFloat() < minVal.asFloat();
				else if (elems[i].type == ValueType::STRING &&
							minVal.type == ValueType::STRING)
					smaller = elems[i].asString() < minVal.asString();
				if (smaller)
					minVal = elems[i];
			}
			return minVal;
		}
		if (m->method == "max") {
			if (elems.empty())
				error("max() on empty tuple", "ValueError");
			Value maxVal = elems[0];
			for (size_t i = 1; i < elems.size(); i++) {
				bool larger = false;
				if (elems[i].type == ValueType::INT &&
					 maxVal.type == ValueType::INT)
					larger = elems[i].asInt() > maxVal.asInt();
				else if (elems[i].type == ValueType::FLOAT ||
							maxVal.type == ValueType::FLOAT)
					larger = elems[i].asFloat() > maxVal.asFloat();
				else if (elems[i].type == ValueType::STRING &&
							maxVal.type == ValueType::STRING)
					larger = elems[i].asString() > maxVal.asString();

				if (larger)
					maxVal = elems[i];
			}
			return maxVal;
		}
		if (m->method == "average") {
			if (elems.empty())
				return Value::Float(0);
			double total = 0;
			for (const auto &el : elems)
				total += el.asFloat();
			return Value::Float(total / elems.size());
		}
		if (m->method == "sample") {
			int k = eval(m->args[0]).asInt();
			if (k > (int)elems.size())
				error("Sample larger than population", "ValueError");
			static std::random_device rd;
			static std::mt19937 gen(rd());
			vector<Value> result = elems;
			for (int i = 0; i < k; i++) {
				std::uniform_int_distribution<> dis(i, (int)result.size() - 1);
				int j = dis(gen);
				std::swap(result[i], result[j]);
			}
			result.resize(k);
			return Value::Tuple(result);
		}
		error("Object '" + m->method + "' is not a tuple method",
			"AttributeError");
	}
	// -------- DICTIONARY METHODS ---------
	if (target.type == ValueType::DICT) {
		auto *d = static_cast<DictObject *>(target.ref.get());
		if (m->method == "get") {
			if (m->args.size() < 1 || m->args.size() > 2)
				error("get() takes 1 or 2 arguments", "ArgumentError");
			Value key = eval(m->args[0]);
			Value defVal =
				(m->args.size() == 2) ? eval(m->args[1]) : Value::None();

			if (d->items.count(key))
				return d->items.at(key);
			return defVal;
		}
		if (m->method == "get_default") {
			checkConst();
			if (m->args.size() < 1 || m->args.size() > 2)
				error("get_default() takes 1 or 2 arguments", "ArgumentError");
			Value key = eval(m->args[0]);
			if (d->items.count(key))
				return d->items.at(key);
			Value defVal =
				(m->args.size() == 2) ? eval(m->args[1]) : Value::None();
			Value insertKey = key;
			if (insertKey.type == ValueType::LIST ||
				 insertKey.type == ValueType::SET) {
				insertKey = deepCopy(insertKey);
				insertKey.isConst = true;
			}
			if (insertKey.type == ValueType::DICT)
				error("Dictionary cannot be used as a key", "TypeError");
			d->items[insertKey] = defVal;
			return defVal;
		}
		if (m->method == "clear") {
			checkConst();
			d->items.clear();
			return Value::None();
		}
		if (m->method == "update") {
			checkConst();
			if (m->args.size() != 1)
				error("update() takes exactly 1 argument", "ArgumentError");
			Value other = eval(m->args[0]);
			if (other.type != ValueType::DICT)
				error("update() requires a dictionary argument", "TypeError");
			auto *otherDict = static_cast<DictObject *>(other.ref.get());
			for (const auto &[k, v] : otherDict->items) {
				d->items[k] = v;
			}
			return Value::None();
		}
		if (m->method == "pop") {
			checkConst();
			if (m->args.size() > 1)
				error("pop() takes 0 or 1 argument", "ArgumentError");
			if (m->args.empty()) {
				if (d->items.empty())
					error("pop from empty dictionary", "EmptyContainerError");
				auto it = d->items.begin();
				Value val = it->second;
				d->items.erase(it);
				return val;
			} else {
				Value key = eval(m->args[0]);
				auto it = d->items.find(key);
				if (it == d->items.end())
					error("KeyError: " + valueToString(key), "KeyError");
				Value val = it->second;
				d->items.erase(it);
				return val;
			}
		}
		if (m->method == "keys") {
			vector<Value> keys;
			keys.reserve(d->items.size());
			for (auto &[k, v] : d->items)
				keys.push_back(k);
			return Value::List(keys);
		}
		if (m->method == "values") {
			vector<Value> vals;
			vals.reserve(d->items.size());
			for (auto &[k, v] : d->items)
				vals.push_back(v);
			return Value::List(vals);
		}
		if (m->method == "items") {
			vector<Value> pairs;
			pairs.reserve(d->items.size());
			for (auto &[k, v] : d->items) {
				vector<Value> pair = {k, v};
				pairs.push_back(Value::Tuple(pair));
			}
			return Value::List(pairs);
		}
		if (d->items.count(Value::String(m->method))) {
			Value callee = d->items[Value::String(m->method)];
			vector<CallArg> callArgs;
			vector<CopyMode> modes;
			for (size_t i = 0; i < m->args.size(); i++) {
				CallArg ca;
				ca.value = eval(m->args[i]);
				callArgs.push_back(ca);
				modes.push_back(CopyMode::SHALLOW);
			}
			return this->call(callee, callArgs, modes, m->line, m->col);
		}
		error("Object '" + m->method + "' is not a dict method",
			"AttributeError");
	}
	// ------------------ FILE METHODS ------------------
	if (target.type == ValueType::FILE) {
		auto *f = static_cast<FileObject *>(target.ref.get());
		if (!f->isOpen && m->method != "IsOpen")
			throw FileClosedError("Cannot perform operation on closed file",
				m->line, m->col);
		if (m->method == "IsOpen") {
			if (!m->args.empty())
				error("IsOpen() takes no arguments", "ArgumentError");
			return Value::Bool(f->isOpen);
		}
		if (m->method == "Write") {
			if (m->args.empty() || m->args.size() > 2)
				error("write() expects message and optional replace bool",
					"ArgumentError");
			string msg = valueToString(eval(m->args[0]));
			bool replace = false;
			if (m->args.size() == 2)
				replace = eval(m->args[1]).asBool();
			f->stream.clear();
			if (replace)
				f->stream.seekp(0, std::ios::beg);
			else
				f->stream.seekp(0, std::ios::end);
			if (!(f->stream << msg))
				throw PermissionError("Failed to write to file", m->line, m->col);
			f->stream.flush();
			return Value::None();
		}
		if (m->method == "Clear") {
			if (!m->args.empty())
				error("Clear() expects no arguments", "Arguments");
			f->Reset();
			return Value::None();
		}
		if (m->method == "Read") {
			if (!m->args.empty())
				error("Read() takes no arguments", "ArgumentError");
			f->stream.clear();
			f->stream.seekg(0, std::ios::beg);
			std::stringstream buffer;
			buffer << f->stream.rdbuf();
			return Value::String(buffer.str());
		}
		if (m->method == "ReadLines") {
			bool includeN = false;
			if (m->args.size() == 1)
				includeN = eval(m->args[0]).asBool();
			else if (m->args.size() > 1)
				error("ReadLines() takes optional boolean", "ArgumentError");
			f->stream.clear();
			f->stream.seekg(0, std::ios::beg);
			vector<Value> lines;
			string line;
			while (std::getline(f->stream, line)) {
				if (!line.empty() && line.back() == '\r')
					line.pop_back();
				if (includeN)
					line += "\n";
				lines.push_back(Value::String(line));
			}
			return Value::List(lines);
		}
		if (m->method == "Close") {
			if (f->isOpen) {
				f->stream.close();
				f->isOpen = false;
			}
			return Value::None();
		}
		error("Object '" + m->method + "' is not a file method",
			"AttributeError");
	}
	//-------- VECTOR METHODS --------
	if (target.type == ValueType::VECTOR) {
		auto *v = static_cast<VectorObject *>(target.ref.get());
		if (m->method == "dimension") {
			if (!m->args.empty())
				error("dimension() takes no arguments", "ArgumentError");
			return Value::Int((long long)v->elements.size());
		}
		if (m->method == "magnitude") {
			if (!m->args.empty())
				error("magnitude() takes no arguments", "ArgumentError");
			Value sum = Value::Int(0);
			for (const auto &d : v->elements) {
				Value sq;
				if (d.type == ValueType::INT) {
					long long r;
					bool ovf = __builtin_mul_overflow(d.iVal, d.iVal, &r);
					if (ovf)
						sq = BigIntObject::mul(Value::BigInt(d.iVal),
							Value::BigInt(d.iVal));
					else
						sq = Value::Int(r);
				} else if (d.type == ValueType::BIGINT)
					sq = BigIntObject::mul(d, d);
				else
					sq = Value::Float(d.asFloat() * d.asFloat());
				if (sum.type == ValueType::INT && sq.type == ValueType::INT) {
					long long r = sum.iVal + sq.iVal;
					bool ovf = ((sum.iVal ^ r) & (sq.iVal ^ r)) < 0;
					if (ovf)
						sum = BigIntObject::add(Value::BigInt(sum.iVal),
							Value::BigInt(sq.iVal));
					else
						sum = Value::Int(r);
				} else if (sum.type == ValueType::BIGINT ||
							  sq.type == ValueType::BIGINT)
					sum = BigIntObject::add(sum, sq);
				else
					sum = Value::Float(sum.asFloat() + sq.asFloat());
			}
			return Value::Float(std::sqrt(sum.asFloat()));
		}
		if (m->method == "vector_reversal") {
			checkConst();
			std::reverse(v->elements.begin(), v->elements.end());
			return target;
		}
		if (m->method == "expand") {
			checkConst();
			if (m->args.size() != 1)
				error("expand() takes 1 argument", "ArgumentError");
			long long n = eval(m->args[0]).asInt();
			if (n < 0)
				error("Cannot expand by negative amount", "ValueError");
			for (int i = 0; i < n; i++)
				v->elements.push_back(Value::Int(0));
			return target;
		}
		if (m->method == "shrink") {
			checkConst();
			if (m->args.size() != 1)
				error("shrink() takes 1 argument", "ArgumentError");
			long long n = eval(m->args[0]).asInt();
			if (n < 0)
				error("Cannot shrink by negative amount", "ValueError");
			if ((long long)v->elements.size() - n < 1)
				error("Vector cannot be shrunk below 1 dimension", "ValueError");
			for (int i = 0; i < n; i++)
				v->elements.pop_back();
			return target;
		}
		if (m->method == "unitVec") {
			Value sum = Value::Int(0);
			for (const auto &d : v->elements) {
				Value sq;
				if (d.type == ValueType::INT) {
					long long r;
					bool ovf = __builtin_mul_overflow(d.iVal, d.iVal, &r);
					if (ovf)
						sq = BigIntObject::mul(Value::BigInt(d.iVal),
							Value::BigInt(d.iVal));
					else
						sq = Value::Int(r);
				} else if (d.type == ValueType::BIGINT)
					sq = BigIntObject::mul(d, d);
				else
					sq = Value::Float(d.asFloat() * d.asFloat());
				if (sum.type == ValueType::INT && sq.type == ValueType::INT) {
					long long r = sum.iVal + sq.iVal;
					bool ovf = ((sum.iVal ^ r) & (sq.iVal ^ r)) < 0;
					if (ovf)
						sum = BigIntObject::add(Value::BigInt(sum.iVal),
							Value::BigInt(sq.iVal));
					else
						sum = Value::Int(r);
				} else if (sum.type == ValueType::BIGINT ||
							  sq.type == ValueType::BIGINT)
					sum = BigIntObject::add(sum, sq);
				else
					sum = Value::Float(sum.asFloat() + sq.asFloat());
			}
			double mag = std::sqrt(sum.asFloat());
			if (mag == 0)
				error("Cannot get unit vector of zero vector", "MathError");
			std::vector<Value> res;
			res.reserve(v->elements.size());
			for (auto d : v->elements)
				res.push_back(Value::Float(d.asFloat() / mag));
			return Value::Vector(res);
		}
		if (m->method == "projectOnto") {
			if (m->args.size() != 1)
				error("projectOnto() takes 1 argument", "ArgumentError");
			Value other = eval(m->args[0]);
			if (other.type != ValueType::VECTOR)
				error("Argument must be a vector", "TypeError");
			auto *u = v;
			auto *v2 = static_cast<VectorObject *>(other.ref.get());
			if (u->elements.size() != v2->elements.size())
				error("Dimension mismatch", "ValueError");
			Value dot = Value::Int(0);
			Value mag2 = Value::Int(0);
			for (size_t i = 0; i < u->elements.size(); i++) {
				Value valU = u->elements[i];
				Value valV = v2->elements[i];
				Value prod;
				if (valU.type == ValueType::INT && valV.type == ValueType::INT) {
					long long r;
					bool ovf = __builtin_mul_overflow(valU.iVal, valV.iVal, &r);
					if (ovf)
						prod = BigIntObject::mul(Value::BigInt(valU.iVal),
							Value::BigInt(valV.iVal));
					else
						prod = Value::Int(r);
				} else if (valU.type == ValueType::BIGINT ||
							  valV.type == ValueType::BIGINT)
					prod = BigIntObject::mul(valU, valV);
				else
					prod = Value::Float(valU.asFloat() * valV.asFloat());
				if (dot.type == ValueType::INT && prod.type == ValueType::INT) {
					long long r = dot.iVal + prod.iVal;
					bool ovf = ((dot.iVal ^ r) & (prod.iVal ^ r)) < 0;
					if (ovf)
						dot = BigIntObject::add(Value::BigInt(dot.iVal),
							Value::BigInt(prod.iVal));
					else
						dot = Value::Int(r);
				} else if (dot.type == ValueType::BIGINT ||
							  prod.type == ValueType::BIGINT)
					dot = BigIntObject::add(dot, prod);
				else
					dot = Value::Float(dot.asFloat() + prod.asFloat());
				Value sq;
				if (valV.type == ValueType::INT) {
					long long r;
					bool ovf = __builtin_mul_overflow(valV.iVal, valV.iVal, &r);
					if (ovf)
						sq = BigIntObject::mul(Value::BigInt(valV.iVal),
							Value::BigInt(valV.iVal));
					else
						sq = Value::Int(r);
				} else if (valV.type == ValueType::BIGINT)
					sq = BigIntObject::mul(valV, valV);
				else
					sq = Value::Float(valV.asFloat() * valV.asFloat());
				if (mag2.type == ValueType::INT && sq.type == ValueType::INT) {
					long long r = mag2.iVal + sq.iVal;
					bool ovf = ((mag2.iVal ^ r) & (sq.iVal ^ r)) < 0;
					if (ovf)
						mag2 = BigIntObject::add(Value::BigInt(mag2.iVal),
							Value::BigInt(sq.iVal));
					else
						mag2 = Value::Int(r);
				} else if (mag2.type == ValueType::BIGINT ||
							  sq.type == ValueType::BIGINT)
					mag2 = BigIntObject::add(mag2, sq);
				else
					mag2 = Value::Float(mag2.asFloat() + sq.asFloat());
			}
			if (mag2.asFloat() == 0)
				error("Cannot project onto zero vector", "MathError");
			double scalar = dot.asFloat() / mag2.asFloat();
			std::vector<Value> res;
			res.reserve(v2->elements.size());
			for (auto d : v2->elements)
				res.push_back(Value::Float(d.asFloat() * scalar));
			return Value::Vector(res);
		}
		if (m->method == "dot") {
			if (m->args.size() != 1)
				error("dot() takes 1 argument", "ArgumentError");
			Value other = eval(m->args[0]);
			if (other.type != ValueType::VECTOR)
				error("Argument must be a vector", "TypeError");
			auto *v2 = static_cast<VectorObject *>(other.ref.get());
			if (v->elements.size() != v2->elements.size())
				error("Dimension mismatch", "ValueError");
			Value dot = Value::Int(0);
			for (size_t i = 0; i < v->elements.size(); i++) {
				Value x = v->elements[i];
				Value y = v2->elements[i];
				Value prod;
				if (x.type == ValueType::INT && y.type == ValueType::INT) {
					long long r;
					bool ovf = __builtin_mul_overflow(x.iVal, y.iVal, &r);
					if (ovf)
						prod = BigIntObject::mul(Value::BigInt(x.iVal),
							Value::BigInt(y.iVal));
					else
						prod = Value::Int(r);
				} else if (x.type == ValueType::BIGINT ||
							  y.type == ValueType::BIGINT)
					prod = BigIntObject::mul(x, y);
				else
					prod = Value::Float(x.asFloat() * y.asFloat());
				if (dot.type == ValueType::INT && prod.type == ValueType::INT) {
					long long r = dot.iVal + prod.iVal;
					bool ovf = ((dot.iVal ^ r) & (prod.iVal ^ r)) < 0;
					if (ovf)
						dot = BigIntObject::add(Value::BigInt(dot.iVal),
							Value::BigInt(prod.iVal));
					else
						dot = Value::Int(r);
				} else if (dot.type == ValueType::BIGINT ||
							  prod.type == ValueType::BIGINT)
					dot = BigIntObject::add(dot, prod);
				else
					dot = Value::Float(dot.asFloat() + prod.asFloat());
			}
			return dot;
		}
		if (m->method == "cross") {
			if (m->args.size() != 1)
				error("cross() takes 1 argument", "ArgumentError");
			Value other = eval(m->args[0]);
			if (other.type != ValueType::VECTOR)
				error("Argument must be a vector", "TypeError");
			auto *v2 = static_cast<VectorObject *>(other.ref.get());
			size_t dim = v->elements.size();
			if (dim != v2->elements.size())
				error("Dimension mismatch", "ValueError");
			auto safeMul = [](const Value &a, const Value &b) -> Value {
				if (a.type == ValueType::INT && b.type == ValueType::INT) {
					long long r;
					bool ovf = __builtin_mul_overflow(a.iVal, b.iVal, &r);
					if (ovf)
						return BigIntObject::mul(Value::BigInt(a.iVal),
							Value::BigInt(b.iVal));
					return Value::Int(r);
				}
				if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT)
					return BigIntObject::mul(a, b);
				return Value::Float(a.asFloat() * b.asFloat());
			};
			auto safeSub = [](const Value &a, const Value &b) -> Value {
				if (a.type == ValueType::INT && b.type == ValueType::INT) {
					long long r = a.iVal - b.iVal;
					bool ovf = ((a.iVal ^ b.iVal) & (a.iVal ^ r)) < 0;
					if (ovf)
						return BigIntObject::sub(Value::BigInt(a.iVal),
							Value::BigInt(b.iVal));
					return Value::Int(r);
				}
				if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT)
					return BigIntObject::sub(a, b);
				return Value::Float(a.asFloat() - b.asFloat());
			};
			if (dim == 1)
				return Value::Int(0);
			else if (dim == 2) {
				Value term1 = safeMul(v->elements[0], v2->elements[1]);
				Value term2 = safeMul(v->elements[1], v2->elements[0]);
				std::vector<Value> res = {safeSub(term1, term2)};
				return Value::Vector(res);
			} else if (dim == 3) {
				Value x = safeSub(safeMul(v->elements[1], v2->elements[2]),
					safeMul(v->elements[2], v2->elements[1]));
				Value y = safeSub(safeMul(v->elements[2], v2->elements[0]),
					safeMul(v->elements[0], v2->elements[2]));
				Value z = safeSub(safeMul(v->elements[0], v2->elements[1]),
					safeMul(v->elements[1], v2->elements[0]));
				std::vector<Value> res = {x, y, z};
				return Value::Vector(res);
			} else {
				error("Binary cross product is not defined for dimensions > 3",
					"ValueError");
			}
		}
		error("Object '" + m->method + "' is not a vector method",
			"AttributeError");
	}
	error("Type has no attribute '" + m->method + "'", "AttributeError");
	return Value::None();
}
