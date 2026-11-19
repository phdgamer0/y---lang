#include "ylang/vm.hpp"
#include "objects.hpp"
#include "types.hpp"

#ifdef _WIN32
#include <shellapi.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif

void Interpreter::registerBuiltins() {

	// ========= CASTING ==========
	env->set("exit", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.size() > 1) {
			throw ArgumentError("exit() takes 1 or 0 argumnet, (exit code)", l, c);
		}
		exit(args.empty() ? 0 : args[0].asInt());
		return Value::None();
	}),
		false);
	env->set("int", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.empty())
			return Value::Int(0);
		auto mRes = [this](MethodCallExpr *m) {
			return this->Resolve_methods(m);
		};
		auto iRes = [this](std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
			this->resolveImport(lib, sym, targetEnv);
		};
		auto dunder = tryCastDunder(args[0], "__to_int__", l, c, this->env, mRes, iRes);
		if (dunder.first)
			return dunder.second;
		Value v = dunder.second;
		if (v.type == ValueType::INT)
			return v;
		if (v.type == ValueType::BIGINT)
			return v;
		if (v.type == ValueType::BOOL)
			return Value::Int(v.asBool() ? 1 : 0);
		if (v.type == ValueType::FLOAT) {
			double d = v.asFloat();
			if (d >= (double)LLONG_MIN && d <= (double)LLONG_MAX)
				return Value::Int((long long)d);
			std::ostringstream ss;
			ss << std::fixed << std::setprecision(0) << std::abs(d);
			std::string s = ss.str();
			BigIntObject res(0);
			BigIntObject ten(10);
			for (char ch : s)
				if (ch >= '0' && ch <= '9')
					res = (res * ten) + BigIntObject(ch - '0');
			res.isNegative = (d < 0);
			return Value::BigInt(std::make_shared<BigIntObject>(res));
		}
		if (v.type == ValueType::STRING) {
			string s = v.asString();
			if (s == "inf") {
				return Value::Float(std::numeric_limits<double>::infinity());
			}
			try {
				return Value::Int(std::stoll(s));
			} catch (...) {
				bool neg = false;
				if (!s.empty() && s[0] == '-') {
					neg = true;
					s = s.substr(1);
				}
				BigIntObject res(0);
				BigIntObject ten(10);
				for (char ch : s) {
					if (ch >= '0' && ch <= '9')
						res = (res * ten) + BigIntObject(ch - '0');
					else
						throw ValueError(
							"Invalid literal for int(): " + v.asString(), l, c);
				}
				res.isNegative = neg;
				return Value::BigInt(std::make_shared<BigIntObject>(res));
			}
		}
		throw TypeError("Cannot cast '" + valueToString(v) + "' to int", l,
			c);
	}),
		false);
	env->set("float", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.empty())
			return Value::Float(0.0, false);
		auto mRes = [this](MethodCallExpr *m) {
			return this->Resolve_methods(m);
		};
		auto iRes = [this](std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
			this->resolveImport(lib, sym, targetEnv);
		};
		auto dunder = tryCastDunder(args[0], "__to_float__", l, c, this->env, mRes, iRes);
		if (dunder.first)
			return dunder.second;
		Value v = dunder.second;
		if (v.type == ValueType::FLOAT)
			return Value::Float(v.asFloat());
		if (v.type == ValueType::INT)
			return Value::Float(v.asInt());
		if (v.type == ValueType::BOOL)
			return Value::Float(v.asBool() ? 1.0 : 0.0);
		if (v.type == ValueType::STRING && isdecimal_str(v.asString())) {
			try {
				return Value::Int(std::stof(v.asString()));
			} catch (...) {
				throw ValueError("ValueError: String too large for float",
					l, c);
			}
		}
		throw TypeError(
			"Cannot cast '" + valueToString(v) + "' to float", l, c);
	}),
		false);
	env->set("bool", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.empty())
			return Value::Bool(false);
		auto mRes = [this](MethodCallExpr *m) {
			return this->Resolve_methods(m);
		};
		auto iRes = [this](std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
			this->resolveImport(lib, sym, targetEnv);
		};
		auto dunder = tryCastDunder(args[0], "__to_bool__", l, c, this->env, mRes, iRes);
		if (dunder.first)
			return dunder.second;
		Value v = dunder.second;
		return Value::Bool(v.isTruthy());
	}),
		false);
	env->set("string", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.empty())
			return Value::String("");
		auto mRes = [this](MethodCallExpr *m) {
			return this->Resolve_methods(m);
		};
		auto iRes = [this](std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
			this->resolveImport(lib, sym, targetEnv);
		};
		auto dunder = tryCastDunder(args[0], "__to_string__", l, c, this->env, mRes, iRes);
		if (dunder.first)
			return dunder.second; // The dunder handled it!
		Value v = dunder.second;
		switch (v.type) {
		case ValueType::INT:
			return Value::String(v.adress ? ptr_to_string(v.adress) : std::to_string(v.asInt()));
		case ValueType::FLOAT:
			return Value::String(std::to_string(v.asFloat()));
		case ValueType::BOOL:
			return Value::String(v.asBool() ? "true" : "false");
		case ValueType::STRING:
			return Value::String(v.asString());
		case ValueType::NONE:
			return Value::String("None", true);
		case ValueType::RANGE:
			return Value::String(valueToString(v));
		case ValueType::SET:
			return Value::String(valueToString(v));
		case ValueType::TUPLE:
			return Value::String(valueToString(v));
		case ValueType::LIST:
			return Value::String(valueToString(v));
		case ValueType::BIGINT:
			return Value::String(valueToString(v));
		default:
			return Value::String("");
		}
	}),
		false);
	// ======== CONSTRUCTOR ========
	env->set("range", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.empty())
			return Value::Range(0, 0, 1, false, false, false);
		std::vector<Value> safeArgs = args;
		if (args.size() == 1) {
			auto mRes = [this](MethodCallExpr *m) {
				return this->Resolve_methods(m);
			};
			auto iRes = [this](std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
				this->resolveImport(lib, sym, targetEnv);
			};
			auto dunder = tryCastDunder(args[0], "__to_range__", l, c, this->env, mRes, iRes);
			if (dunder.first)
				return dunder.second;
			safeArgs[0] = dunder.second;
			if (safeArgs[0].type == ValueType::RANGE)
				return safeArgs[0];
		}
		double start = 0, end = 0, step = 1;
		bool isFloat = false;
		auto checkFloat = [&](const Value &v) {
			if (v.type == ValueType::FLOAT)
				isFloat = true;
		};
		if (safeArgs.size() == 1) {
			checkFloat(safeArgs[0]);
			end = safeArgs[0].asFloat();
		} else if (safeArgs.size() >= 2) {
			checkFloat(safeArgs[0]);
			checkFloat(safeArgs[1]);
			start = safeArgs[0].asFloat();
			end = safeArgs[1].asFloat();
		}
		if (safeArgs.size() == 3) {
			checkFloat(safeArgs[2]);
			step = safeArgs[2].asFloat();
		}
		return Value::Range(start, end, step, true, false, isFloat);
	}),
		false);
	env->set("list", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.empty())
			return Value::List({});
		if (args.size() == 1) {
			auto mRes = [this](MethodCallExpr *m) {
				return this->Resolve_methods(m);
			};
			auto iRes = [this](std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
				this->resolveImport(lib, sym, targetEnv);
			};
			auto dunder = tryCastDunder(args[0], "__to_list__", l, c, this->env, mRes, iRes);
			if (dunder.first)
				return dunder.second;
			Value src = dunder.second;
			if (src.type == ValueType::RANGE) {
				auto *r = static_cast<RangeObject *>(src.ref.get());
				vector<Value> elems;
				double current = r->start;
				if (!r->startInclusive)
					current += r->step;
				if ((r->step > 0 && r->start > r->end) || (r->step < 0 && r->start < r->end) || r->step == 0)
					return Value::List({});
				double diff = std::abs(r->end - r->start);
				double steps = diff / std::abs(r->step);
				if (steps > 1000000)
					throw MemoryError("MemoryError: Range too large to convert to list", l, c);
				while (true) {
					bool cond = (r->step > 0) ? (r->endInclusive ? (current <= r->end) : (current < r->end))
													  : (r->endInclusive ? (current >= r->end) : (current > r->end));
					if (!cond)
						break;
					elems.push_back(r->isFloat ? Value::Float(current) : Value::Int((long long)current));
					current += r->step;
				}
				return Value::List(elems);
			}
			if (src.type == ValueType::SET) {
				auto *s = static_cast<SetObject *>(src.ref.get());
				std::vector<Value> res(s->elements.begin(), s->elements.end());
				return Value::List(res);
			}
			if (src.type == ValueType::TUPLE)
				return Value::List(static_cast<TupleObject *>(src.ref.get())->elements);
			if (src.type == ValueType::VECTOR)
				return Value::List(static_cast<VectorObject *>(src.ref.get())->elements);
			return Value::List({src});
		}
		vector<Value> vals;
		for (auto &v : args)
			vals.push_back(v);
		return Value::List(vals);
	}),
		false);
	env->set("set", Value::Native([this](const vector<Value> &args, int l, int c) {
		std::unordered_set<Value, ValueHash, ValueEqual> elems;
		if (args.empty())
			return Value::Set({});
		if (args.size() == 1) {
			auto mRes = [this](MethodCallExpr *m) {
				return this->Resolve_methods(m);
			};
			auto iRes = [this](std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
				this->resolveImport(lib, sym, targetEnv);
			};
			auto dunder = tryCastDunder(args[0], "__to_set__", l, c, this->env, mRes, iRes);
			if (dunder.first)
				return dunder.second;
			Value src = dunder.second;
			if (src.type == ValueType::LIST) {
				auto *listObj = static_cast<ListObject *>(src.ref.get());
				for (auto &e : listObj->elements)
					setAdd(elems, e);
			} else if (src.type == ValueType::RANGE) {
				auto *r = static_cast<RangeObject *>(src.ref.get());
				double current = r->start;
				if (!r->startInclusive)
					current += r->step;
				while (true) {
					bool cond = (r->step > 0) ? (r->endInclusive ? (current <= r->end) : (current < r->end))
													  : (r->endInclusive ? (current >= r->end) : (current > r->end));
					if (!cond)
						break;
					setAdd(elems, r->isFloat ? Value::Float(current) : Value::Int((long long)current));
					current += r->step;
				}
			} else if (src.type == ValueType::VECTOR) {
				for (auto d : static_cast<VectorObject *>(src.ref.get())->elements)
					setAdd(elems, d);
			} else if (src.type == ValueType::STRING) {
				for (char c : src.asString())
					setAdd(elems, Value::String(string(1, c)));
			} else if (src.type == ValueType::SET) {
				for (auto &e : static_cast<SetObject *>(src.ref.get())->elements)
					setAdd(elems, e);
			} else if (src.type == ValueType::TUPLE) {
				for (auto &e : static_cast<TupleObject *>(src.ref.get())->elements)
					setAdd(elems, e);
			} else {
				setAdd(elems, src);
			}
			return Value::Set(elems);
		}
		for (auto &arg : args)
			setAdd(elems, arg);
		return Value::Set(elems);
	}),
		false);
	env->set("tuple", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.empty())
			return Value::Tuple({});
		if (args.size() == 1) {
			auto mRes = [this](MethodCallExpr *m) {
				return this->Resolve_methods(m);
			};
			auto iRes = [this](std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
				this->resolveImport(lib, sym, targetEnv);
			};
			auto dunder = tryCastDunder(args[0], "__to_tuple__", l, c, this->env, mRes, iRes);
			if (dunder.first)
				return dunder.second;
			Value src = dunder.second;
			if (src.type == ValueType::LIST)
				return Value::Tuple(static_cast<ListObject *>(src.ref.get())->elements);
			if (src.type == ValueType::SET) {
				auto s = static_cast<SetObject *>(src.ref.get())->elements;
				std::vector<Value> v(s.begin(), s.end());
				return Value::Tuple(v);
			}
			if (src.type == ValueType::RANGE) {
				auto *r = static_cast<RangeObject *>(src.ref.get());
				vector<Value> elems;
				double current = r->start;
				if (!r->startInclusive)
					current += r->step;
				while (true) {
					bool cond = (r->step > 0) ? (r->endInclusive ? current <= r->end : current < r->end)
													  : (r->endInclusive ? current >= r->end : current > r->end);
					if (!cond)
						break;
					elems.push_back(r->isFloat ? Value::Float(current) : Value::Int((long long)current));
					current += r->step;
				}
				return Value::Tuple(elems);
			}
			if (src.type == ValueType::TUPLE)
				return src;
			if (src.type == ValueType::VECTOR)
				return Value::Tuple(static_cast<VectorObject *>(src.ref.get())->elements);
			return Value::Tuple({src});
		}
		vector<Value> elems;
		for (auto &arg : args)
			elems.push_back(arg);
		return Value::Tuple(elems);
	}),
		false);
	env->set("dict", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.empty())
			return Value::Dict({});
		if (args.size() == 1) {
			auto mRes = [this](MethodCallExpr *m) {
				return this->Resolve_methods(m);
			};
			auto iRes = [this](std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
				this->resolveImport(lib, sym, targetEnv);
			};
			auto dunder = tryCastDunder(args[0], "__to_dict__", l, c, this->env, mRes, iRes);
			if (dunder.first)
				return dunder.second;
		}
		std::unordered_map<Value, Value, ValueHash, ValueEqual> map;
		for (size_t i = 0; i < args.size(); i++) {
			Value v = args[i];
			while (v.type == ValueType::REFERENCE) {
				if (!v.get_ptr_safe())
					throw RuntimeError("Null reference in dict()", l, c);
				v = *(v.get_ptr_safe());
			}
			if (v.type != ValueType::PAIRED) {
				throw TypeError("dict() requires 'key : value' arguments or 'pair()' objects", l, c);
			}
			auto *pObj = static_cast<PairedObject *>(v.ref.get());
			for (const auto &pair : pObj->pairs) {
				Value key = pair.first;
				Value val = pair.second;
				if (key.type == ValueType::LIST || key.type == ValueType::SET || key.type == ValueType::DICT) {
					if (key.type == ValueType::DICT)
						throw TypeError("Dictionary cannot be used as a key (unhashable)", l, c);
					key = deepCopy(key);
					key.isConst = true;
				}
				map[key] = val;
			}
		}
		return Value::Dict(map);
	}),
		false);
	// ======= INTROSPECTION =======
	env->set("typeof", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.size() != 1)
			throw ArgumentError("typeof() takes exactly one argument", l, c);
		switch (args[0].type) {
		case ValueType::NOTYPE:
			return Value::String("NoType");
		case ValueType::INT:
			return Value::String("integer");
		case ValueType::FLOAT:
			return Value::String("float");
		case ValueType::BOOL:
			return Value::String("boolean");
		case ValueType::STRING:
			return Value::String("string");
		case ValueType::LIST:
			return Value::String("list");
		case ValueType::RANGE:
			return Value::String("range");
		case ValueType::SET:
			return Value::String("set");
		case ValueType::TUPLE:
			return Value::String("tuple");
		case ValueType::DICT:
			return Value::String("dictionary");
		case ValueType::FUNCTION:
			return Value::String("function");
		case ValueType::VECTOR:
			return Value::String("vector");
		case ValueType::NATIVE_FUNCTION:
			return Value::String("native function");
		case ValueType::FILE:
			return Value::String("file");
		case ValueType::PAIRED:
			return Value::String("pair");
		case ValueType::BIGINT:
			return Value::String("integer");
		case ValueType::CLASS:
			return Value::String("class");
		case ValueType::INSTANCE:
			return Value::String("instance");
		case ValueType::SUPER:
			return Value::String("function");
		case ValueType::ERROR:
			return Value::String("error");
		case ValueType::NONE:
			return Value::String("None");
		case ValueType::REFERENCE:
			return Value::String("Reference");
		default:
			return Value::String("Unknown type: " + to_string(static_cast<int>(args[0].type)));
		}
	}),
		false);
	env->set("isLocked", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.size() != 1)
			throw ArgumentError("isLocked() takes exactly one argument",
				l, c);
		return Value::Bool(args[0].isLocked);
	}),
		false);
	env->set("isConst", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.size() != 1)
			throw ArgumentError("isConst() takes exactly one argument", l,
				c);
		return Value::Bool(args[0].isConst);
	}),
		false);
	env->set("length", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.empty())
			return Value::Int(0);
		if (args.size() > 1)
			throw ArgumentError("length() takes exactly one argument", l, c);
		auto mRes = [this](MethodCallExpr *m) { return this->Resolve_methods(m); };
		auto iRes = [this](std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
			this->resolveImport(lib, sym, targetEnv);
		};
		auto dunder = tryCastDunder(args[0], "__count__", l, c, this->env, mRes, iRes);
		if (dunder.first) {
			return dunder.second;
		}
		Value v = dunder.second;
		if (v.type == ValueType::LIST) {
			auto *list = static_cast<ListObject *>(v.ref.get());
			return Value::Int(list->elements.size());
		} else if (v.type == ValueType::STRING) {
			return Value::Int(v.asString().size());
		} else if (v.type == ValueType::SET) {
			auto *s = static_cast<SetObject *>(v.ref.get());
			return Value::Int(s->elements.size());
		} else if (v.type == ValueType::TUPLE) {
			auto *t = static_cast<TupleObject *>(v.ref.get());
			return Value::Int(t->elements.size());
		} else if (v.type == ValueType::DICT) {
			auto *d = static_cast<DictObject *>(v.ref.get());
			return Value::Int(d->items.size());
		} else if (v.type == ValueType::RANGE) {
			auto *r = static_cast<RangeObject *>(v.ref.get());
			if (r->step == 0)
				return Value::Int(0);
			double s = r->start;
			if (!r->startInclusive)
				s += r->step;
			double e = r->end;
			if (r->endInclusive)
				e += (r->step > 0 ? 1 : -1) * (r->step * 0.000000001);
			long long count = 0;
			if (r->step > 0 && r->end > s) {
				if (r->endInclusive)
					count = (long long)floor((r->end - s) / r->step) + 1;
				else
					count = (long long)ceil((r->end - s) / r->step);
			} else if (r->step < 0 && r->end < s) {
				if (r->endInclusive)
					count = (long long)floor((s - r->end) / -r->step) + 1;
				else
					count = (long long)ceil((s - r->end) / -r->step);
			}
			return Value::Int(count < 0 ? 0 : count);
		} else {
			throw TypeError("Object of type " + valueToString(v) + " has no length", l, c);
		}
	}),
		false);
	env->set("sum", Value::Native([this](const vector<Value> &args, int l, int c) {
		long double total = 0;
		bool isFloat = false;
		vector<Value> worklist;
		worklist.reserve(args.size() * 2);
		for (const auto &arg : args)
			worklist.push_back(arg);
		while (!worklist.empty()) {
			Value v = worklist.back();
			worklist.pop_back();
			switch (v.type) {
			case ValueType::INT:
				total += v.asInt();
				break;
			case ValueType::FLOAT:
				total += v.asFloat();
				isFloat = true;
				break;
			case ValueType::LIST: {
				auto *list = static_cast<ListObject *>(v.ref.get());
				worklist.insert(worklist.end(), list->elements.begin(),
					list->elements.end());
				break;
			}
			case ValueType::TUPLE: {
				auto *t = static_cast<TupleObject *>(v.ref.get());
				worklist.insert(worklist.end(), t->elements.begin(),
					t->elements.end());
				break;
			}
			case ValueType::SET: {
				auto *s = static_cast<SetObject *>(v.ref.get());
				worklist.insert(worklist.end(), s->elements.begin(),
					s->elements.end());
				break;
			}
			case ValueType::RANGE: {
				auto *r = static_cast<RangeObject *>(v.ref.get());
				if (r->isFloat)
					isFloat = true;
				double current = r->start;
				if (!r->startInclusive)
					current += r->step;
				while (true) {
					bool cond = (r->step > 0)
										? (r->endInclusive ? current <= r->end
																 : current < r->end)
										: (r->endInclusive ? current >= r->end
																 : current > r->end);
					if (!cond)
						break;
					total += current;
					current += r->step;
				}
				break;
			}
			case ValueType::BOOL:
				total += v.asBool() ? 1 : 0;
				break;
			default:
				throw TypeError("sum() encountered non-numeric type: " +
										 valueToString(v),
					l, c);
			}
		}
		return isFloat ? Value::Float(total) : Value::Int((long long)total);
	}),
		false);
	env->set("pair", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.size() != 2)
			throw ArgumentError("pair() takes exactly two arguments (keys, values)", l, c);
		auto extract = [&](Value v) -> vector<Value> {
			if (v.type == ValueType::LIST)
				return static_cast<ListObject *>(v.ref.get())->elements;
			if (v.type == ValueType::SET) {
				auto st = static_cast<SetObject *>(v.ref.get())->elements;
				std::vector<Value> s;
				s.reserve(st.size());
				s.assign(st.begin(), st.end());
				return s;
			}
			if (v.type == ValueType::TUPLE)
				return static_cast<TupleObject *>(v.ref.get())->elements;
			if (v.type == ValueType::RANGE) {
				auto *r = static_cast<RangeObject *>(v.ref.get());
				vector<Value> rvals;
				double current = r->start;
				if (!r->startInclusive)
					current += r->step;
				while (true) {
					bool cond = (r->step > 0)
										? (r->endInclusive ? current <= r->end
																 : current < r->end)
										: (r->endInclusive ? current >= r->end
																 : current > r->end);
					if (!cond)
						break;
					rvals.push_back(r->isFloat ? Value::Float(current)
														: Value::Int((long long)current));
					current += r->step;
				}
				return rvals;
			}
			throw TypeError(
				"pair() arguments must be containers (list, set, tuple, "
				"range)",
				l, c);
			return {};
		};
		vector<Value> rawKeys = extract(args[0]);
		vector<Value> vals = extract(args[1]);
		vector<Value> uniqueKeys;
		for (const auto &k : rawKeys) {
			bool seen = false;
			for (const auto &u : uniqueKeys)
				if (k.strictEquals(u)) {
					seen = true;
					break;
				}
			if (!seen)
				uniqueKeys.push_back(k);
		}
		size_t count = std::min(uniqueKeys.size(), vals.size());
		std::vector<std::pair<Value, Value>> finalPairs;
		for (size_t i = 0; i < count; i++) {
			finalPairs.push_back({uniqueKeys[i], vals[i]});
		}
		return Value::Paired(finalPairs);
	}),
		false);
	env->set("swap", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.size() != 2)
			throw ArgumentError("swap() takes exactly two arguments", l, c);
		if (args[0].type != ValueType::REFERENCE ||
			 args[1].type != ValueType::REFERENCE) {
			throw TypeError("swap requires refrences. use swap(@a, @b)", l, c);
		}
		auto *val1 = args[0].get_ptr_safe();
		auto *val2 = args[1].get_ptr_safe();
		if (!val1 || !val2)
			throw RuntimeError("cannot swap null refrences", l, c);
		if (val1->isConst || val2->isConst)
			throw ConstError("cannot swap constants", l, c);
		if ((val1->isLocked || val2->isLocked) && !val1->sameType(*val2))
			throw ConstError("cannot change the type of the locked variables", l, c);
		std::swap(*val1, *val2);
		return Value::None();
	}),
		false);
	env->set("max", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.size() < 2)
			throw ArgumentError("max(), takes at least 2 arguments", l, c);
		auto maxVal = Value::NoType();
		auto mRes = [this](MethodCallExpr *m) {
			return this->Resolve_methods(m);
		};

		auto iRes = [this](std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
			this->resolveImport(lib, sym, targetEnv);
		};
		for (int i = 1; i < args.size(); i++) {
			maxVal = lessValue(args[i - 1], args[i], this->env, mRes, iRes) ? args[i] : args[i - 1];
		}
		return maxVal;
	}),
		false);
	env->set("min", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.size() < 2)
			throw ArgumentError("min(), takes at least 2 arguments", l, c);
		auto minVal = Value::NoType();
		auto mRes = [this](MethodCallExpr *m) {
			return this->Resolve_methods(m);
		};

		auto iRes = [this](std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv) {
			this->resolveImport(lib, sym, targetEnv);
		};
		for (int i = 1; i < args.size(); i++) {
			minVal = lessValue(args[i - 1], args[i], this->env, mRes, iRes) ? args[i - 1] : args[i];
		}
		return minVal;
	}),
		false);
	// ============ I/O ============
	env->set("print", Value::Native([this](const vector<Value> &args, int l, int c) {
		return this->nativePrint(args, l, c);
	}),
		false);
	env->set("input", Value::Native([this](const vector<Value> &args, int l, int c) {
		string prompt = "";
		if (!args.empty())
			prompt = valueToString(args[0]);
		std::cout << prompt;
		if (std::cin.fail())
			std::cin.clear();
		if (std::cin.peek() == '\n')
			std::cin.ignore();
		string line;
		if (!std::getline(std::cin, line))
			return Value::None();
		if (args.size() > 1) {
			ValueType targetType = args[1].type;
			try {
				if (targetType == ValueType::INT)
					return Value::Int(std::stoll(line));
				else if (targetType == ValueType::FLOAT)
					return Value::Float(std::stod(line));
				else if (targetType == ValueType::BOOL)
					return Value::Bool(line == "true");
				else if (targetType == ValueType::LIST || targetType == ValueType::SET || targetType == ValueType::RANGE) {
					throw TypeError("Complex type input not fully supported yet", l, c);
				}
			} catch (const LangError &) {
				throw;
			} catch (...) {
				throw ValueError("ValueError: Could not convert input '" + line + "' to target type", l, c);
			}
		}
		return Value::String(line);
	}),
		false);
}

void Interpreter::registerStdLib() {
	registerIoModules();
	registerTimeModule();
	registerSerialModule();
	registerEventsModule();
	registerMathModules();
	registerRegexModule();
	registerBuiltins();
	registerFfi();
}
