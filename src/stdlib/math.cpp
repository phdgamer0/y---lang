#include "ylang/vm.hpp"
#include "objects.hpp"
#include "types.hpp"

#ifdef _WIN32
#include <shellapi.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif

void Interpreter::registerMathModules() {
	modules["Math"] = [](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Math");
		moduleNamespace->mro.push_back(moduleNamespace.get());
		auto define = [&](string name, NativeFunc f) {
			moduleNamespace->staticFields[name] = Value::Native(f);
			if (symbols.size() == 1 && symbols[0] == "*") {
				env->set(name, Value::Native(f), true);
				return;
			}
			for (const auto &s : symbols)
				if (s == name) {
					env->set(name, Value::Native(f), true);
					break;
				}
		};
		auto valueToFloat = [&](const Value &v, int l, int c) -> double {
			if (v.type == ValueType::FLOAT)
				return v.asFloat();
			if (v.type == ValueType::INT)
				return (double)v.iVal;
			if (v.type == ValueType::BOOL)
				return v.asBool() ? 1.0 : 0.0;
			if (v.type == ValueType::BIGINT) {
				try {
					return std::stod(valueToString(v));
				} catch (...) {
					return INFINITY;
				}
			}
			throw TypeError("Expected a number", l, c);
		};
		auto defineValue = [&](string name, Value v) {
			moduleNamespace->staticFields[name] = v;
			if (symbols.size() == 1 && symbols[0] == "*") {
				env->set(name, v, true, true);
				return;
			}
			for (const auto &s : symbols) {
				if (s == name) {
					env->set(name, v, true, true);
					break;
				}
			}
		};
		// Constants
		defineValue("PI", Value::Float(3.141592653589793));
		defineValue("E", Value::Float(2.718281828459045));
		defineValue("PHI", Value::Float(1.618033988749894));
		defineValue("G", Value::Float(6.6743e-11));
		defineValue("G_EARTH", Value::Float(9.80665));
		defineValue("EPSILON_0", Value::Float(8.8541878128e-12));
		defineValue("PLANCK_H", Value::Float(6.62607015e-34));
		// Basic Functions
		define("Abs", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Abs() expects 1 argument (num)", l, c);
			const Value &v = args[0];
			if (v.type == ValueType::INT)
				return Value::Int(std::abs(v.asInt()));
			if (v.type == ValueType::FLOAT)
				return Value::Float(std::abs(v.asFloat()));
			if (v.type == ValueType::BIGINT) {
				auto *big = static_cast<BigIntObject *>(v.ref.get());
				if (!big->isNegative)
					return v;
				auto copy = std::make_shared<BigIntObject>(*big);
				copy->isNegative = false;
				return Value::BigInt(copy);
			}
			return Value::Float(std::abs(valueToFloat(v, l, c)));
		});
		define("Floor", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Floor() expects 1 argument (num)", l, c);
			if (args[0].type == ValueType::INT ||
				 args[0].type == ValueType::BIGINT)
				return args[0];
			return Value::Int((long long)std::floor(valueToFloat(args[0], l, c)));
		});
		define("Ceil", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Ceil() expects 1 argument (num)", l, c);
			if (args[0].type == ValueType::INT ||
				 args[0].type == ValueType::BIGINT)
				return args[0];
			return Value::Int((long long)std::ceil(valueToFloat(args[0], l, c)));
		});
		define("Sqrt", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Sqrt() expects 1 argument (num)", l, c);
			double val = valueToFloat(args[0], l, c);
			if (val < 0.0)
				throw ValueError("argument cannot be negative", l, c);
			return Value::Float(std::sqrt(val));
		});
		define("Cbrt", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Cbrt() expects 1 argument (num)", l, c);
			return Value::Float(std::cbrt(valueToFloat(args[0], l, c)));
		});
		define("Sgn", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Sgn() expects 1 argument (num)", l, c);
			double val = valueToFloat(args[0], l, c);
			if (val > 0.0)
				return Value::Int(1);
			else if (val < 0.0)
				return Value::Int(-1);
			else
				return Value::Int(0);
		});
		define("RadToDeg", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("RadToDeg() expects 1 argument (num)", l, c);
			return Value::Float(valueToFloat(args[0], l, c) * (long double)180 / 3.141592653589793);
		});
		define("DegToRad", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("DegToRad() expects 1 argument (num)", l, c);
			return Value::Float(valueToFloat(args[0], l, c) * 3.141592653589793 /
									  (long double)180);
		});
		define("MapRange", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("MapRange() expects 5 arguments (num, num_range_min_inclusive, num_range_max_inclusive, new_range_min_inclusive, new_range_max_inclusive)", l, c);
			double num = valueToFloat(args[0], l, c);
			double in_min = valueToFloat(args[1], l, c);
			double in_max = valueToFloat(args[2], l, c);
			double out_min = valueToFloat(args[3], l, c);
			double out_max = valueToFloat(args[4], l, c);
			double in_range = in_max - in_min;
			if (in_range == 0.0) {
				return Value::Float(out_min);
			}
			double result = out_min + ((num - in_min) * (out_max - out_min)) / in_range;
			return Value::Float(result);
		});
		define("RoundToPower", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("RoundToPower() expects exactly 2 arguments (number, power)", l, c);
			double num = valueToFloat(args[0], l, c);
			double power = valueToFloat(args[1], l, c);
			double scale = std::pow(10.0, power);
			double result = std::round(num / scale) * scale;
			if (power >= 0.0 && (args[0].type == ValueType::INT || args[0].type == ValueType::BIGINT)) {
				return Value::Int((long long)result);
			}
			return Value::Float(result);
		});
		define("ConvertToBase", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("ConvertToBase() expects exactly 3 arguments (string_num, curr_base, target_base)", l, c);
			auto CharToVal = [](char c) -> int {
				if (c >= '0' && c <= '9')
					return c - '0';
				if (c >= 'A' && c <= 'Z')
					return c - 'A' + 10;
				if (c >= 'a' && c <= 'z')
					return c - 'a' + 10;
			};
			auto ValToChar = [](long long v) -> char {
				if (v >= 0 && v <= 9)
					return v + '0';
				return v - 10 + 'A';
			};
			std::string num = args[0].asString();
			long long n = args[1].asInt();
			long long m = args[2].asInt();
			if (n < 2 || n > 36 || m < 2 || m > 36)
				throw ValueError("Invalid Base", l, c);
			long long decimalVal = 0;
			long long power = 1;
			for (int i = num.length() - 1; i >= 0; i--) {
				int val = CharToVal(num[i]);
				if (val < 0 || val >= n)
					throw ValueError("Invalid Input for Base", l, c);
				decimalVal += val * power;
				power *= n;
			}
			if (decimalVal == 0)
				return Value::String("0");
			std::string result = "";
			while (decimalVal > 0) {
				result += ValToChar(decimalVal % m);
				decimalVal /= m;
			}
			std::reverse(result.begin(), result.end());
			return Value::String(result);
		});
		// Trig
		define("Sin", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Sin() expects 1 argument (num)", l, c);
			return Value::Float(std::sin(valueToFloat(args[0], l, c)));
		});
		define("Cos", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Cos() expects 1 argument (num)", l, c);
			return Value::Float(std::cos(valueToFloat(args[0], l, c)));
		});
		define("Tan", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Tan() expects 1 argument (num)", l, c);
			return Value::Float(std::tan(valueToFloat(args[0], l, c)));
		});
		define("Sinh", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Sinh() expects 1 argument (num)", l, c);
			return Value::Float(std::sinh(valueToFloat(args[0], l, c)));
		});
		define("Cosh", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Cosh() expects 1 argument (num)", l, c);
			return Value::Float(std::cosh(valueToFloat(args[0], l, c)));
		});
		define("Tanh", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Tanh() expects 1 argument (num)", l, c);
			return Value::Float(std::tanh(valueToFloat(args[0], l, c)));
		});
		// Arc
		define("Arcsin", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Arcsin() expects 1 argument (num)", l, c);
			return Value::Float(std::asin(valueToFloat(args[0], l, c)));
		});
		define("Arccos", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Arccos() expects 1 argument (num)", l, c);
			return Value::Float(std::acos(valueToFloat(args[0], l, c)));
		});
		define("Arctan", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Arctan() expects 1 argument (num)", l, c);
			return Value::Float(std::atan(valueToFloat(args[0], l, c)));
		});
		define("Atan2", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("Atan2() expects 2 arguments (num1, num2)", l, c);
			return Value::Float(std::atan2(valueToFloat(args[0], l, c), valueToFloat(args[1], l, c)));
		});
		define("Arcsinh", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Arcsinh() expects 1 argument (num)", l, c);
			return Value::Float(std::asinh(valueToFloat(args[0], l, c)));
		});
		define("Arccosh", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Arccosh() expects 1 argument (num)", l, c);
			return Value::Float(std::acosh(valueToFloat(args[0], l, c)));
		});
		define("Arctanh", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Arctanh() expects 1 argument (num)", l, c);
			return Value::Float(std::atanh(valueToFloat(args[0], l, c)));
		});
		// Log
		define("Log", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Log() expects 1 argument (num)", l, c);
			double val = valueToFloat(args[0], l, c);
			if (val < 0.0)
				throw ValueError("argument cannot be negative", l, c);
			return Value::Float(std::log(val));
		});
		define("Log2", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Log2() expects 1 argument (num)", l, c);
			double val = valueToFloat(args[0], l, c);
			if (val < 0.0)
				throw ValueError("argument cannot be negative", l, c);
			return Value::Float(std::log2(val));
		});
		define("Log10", [&](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Log10() expects 1 argument (num)", l, c);
			double val = valueToFloat(args[0], l, c);
			if (val < 0.0)
				throw ValueError("argument cannot be negative", l, c);
			return Value::Float(std::log10(val));
		});
		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Math", modVal, false, false);
	};

	modules["Random"] = [](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Random");
		moduleNamespace->mro.push_back(moduleNamespace.get());
		auto define = [&](string name, NativeFunc f) {
			moduleNamespace->staticFields[name] = Value::Native(f);
			if (symbols.size() == 1 && symbols[0] == "*") {
				env->set(name, Value::Native(f), true);
				return;
			}
			for (const auto &s : symbols)
				if (s == name) {
					env->set(name, Value::Native(f), true);
					break;
				}
		};
		auto getGen = []() -> std::mt19937 & {
			static std::random_device rd;
			static std::mt19937 gen(rd());
			return gen;
		};
		define("RandFloat", [=](const vector<Value> &args, int l, int c) {
			std::uniform_real_distribution<> dis(0.0, 1.0);
			return Value::Float(dis(getGen()));
		});
		define("RandChoice", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("RandChoice() expects 1 argument", l, c);
			Value v = args[0];
			auto pickIndex = [&](size_t size) {
				if (size == 0)
					throw EmptyContainerError("Cannot choose from empty container",
						l, c);
				std::uniform_int_distribution<size_t> dis(0, size - 1);
				return dis(getGen());
			};
			if (v.type == ValueType::LIST) {
				auto *list = static_cast<ListObject *>(v.ref.get());
				return list->elements[pickIndex(list->elements.size())];
			}
			if (v.type == ValueType::TUPLE) {
				auto *tuple = static_cast<TupleObject *>(v.ref.get());
				return tuple->elements[pickIndex(tuple->elements.size())];
			}
			if (v.type == ValueType::STRING) {
				const string &str = v.asString();
				char c_char = str[pickIndex(str.size())];
				return Value::String(string(1, c_char));
			}
			if (v.type == ValueType::SET) {
				auto *set = static_cast<SetObject *>(v.ref.get());
				size_t idx = pickIndex(set->elements.size());
				auto it = set->elements.begin();
				std::advance(it, idx);
				return *it;
			}
			if (v.type == ValueType::RANGE) {
				auto *r = static_cast<RangeObject *>(v.ref.get());
				if (!r->isValid)
					throw EmptyContainerError("Cannot choose from invalid range", l,
						c);
				double diff = std::abs(r->end - r->start);
				double steps = diff / std::abs(r->step);
				long long count = 0;
				if (r->step > 0 && r->end > r->start)
					count = (long long)(r->endInclusive ? floor(steps) + 1
																	: ceil(steps));
				else if (r->step < 0 && r->end < r->start)
					count = (long long)(r->endInclusive ? floor(steps) + 1
																	: ceil(steps));
				if (count <= 0)
					throw EmptyContainerError("Cannot choose from empty range", l,
						c);
				std::uniform_int_distribution<long long> dis(0, count - 1);
				long long offset = dis(getGen());
				double val = r->start + (offset * r->step);
				return r->isFloat ? Value::Float(val) : Value::Int((long long)val);
			}
			throw TypeError(
				"RandChoice requires a container (list, set, tuple, string, "
				"range)",
				l, c);
		});
		define("Shuffle", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Shuffle() expects 1 argument", l, c);
			Value v = args[0];
			if (v.type == ValueType::LIST) {
				Value newVal = deepCopy(v);
				auto *list = static_cast<ListObject *>(newVal.ref.get());
				std::shuffle(list->elements.begin(), list->elements.end(),
					getGen());
				return newVal;
			}
			if (v.type == ValueType::TUPLE) {
				Value newVal = deepCopy(v);
				auto *t = static_cast<TupleObject *>(newVal.ref.get());
				std::shuffle(t->elements.begin(), t->elements.end(), getGen());
				return newVal;
			}
			if (v.type == ValueType::STRING) {
				string s = v.asString();
				std::shuffle(s.begin(), s.end(), getGen());
				return Value::String(s);
			}
			if (v.type == ValueType::SET) {
				throw TypeError("Sets are unordered collections and cannot be shuffled. Cast to a list first using list(my_set).", l, c);
			}
			if (v.type == ValueType::RANGE) {
				Value listVer = Value::List({});
				throw TypeError(
					"Cannot shuffle a Range (result would not be a range). Use "
					"Sample(range, len) to get a shuffled list.",
					l, c);
			}
			throw TypeError("Shuffle requires a mutable sequence or string", l, c);
		});
		define("Sample", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError(
					"Sample() expects 2 arguments (container, count)", l, c);
			Value v = args[0];
			long long k = args[1].asInt();
			if (k < 0)
				throw ValueError("Sample count cannot be negative", l, c);
			vector<Value> pool;
			bool isString = (v.type == ValueType::STRING);
			if (v.type == ValueType::LIST)
				pool = static_cast<ListObject *>(v.ref.get())->elements;
			else if (v.type == ValueType::TUPLE)
				pool = static_cast<TupleObject *>(v.ref.get())->elements;
			else if (v.type == ValueType::SET) {
				auto s = static_cast<SetObject *>(v.ref.get())->elements;
				pool.assign(s.begin(), s.end());
			} else if (v.type == ValueType::RANGE) {
				auto *r = static_cast<RangeObject *>(v.ref.get());
				long long rangeSize = 0;
				if (r->step > 0) {
					rangeSize = std::ceil((r->end - r->start) / r->step);
					if (r->endInclusive && r->start + rangeSize * r->step == r->end)
						rangeSize++;
				} else if (r->step < 0) {
					rangeSize = std::ceil((r->start - r->end) / -r->step);
					if (r->endInclusive && r->start + rangeSize * r->step == r->end)
						rangeSize++;
				}
				if (k > rangeSize)
					throw ValueError("Sample larger than population", l, c);
				std::unordered_set<long long> chosenIndices;
				while (chosenIndices.size() < k) {
					long long randIdx = std::uniform_int_distribution<long long>(0, rangeSize - 1)(getGen());
					chosenIndices.insert(randIdx);
				}
				vector<Value> deepResult;
				for (long long idx : chosenIndices) {
					double val = r->start + (idx * r->step);
					deepResult.push_back(r->isFloat ? Value::Float(val) : Value::Int((long long)val));
				}
				return Value::List(deepResult);
			} else if (isString) {
				string s = v.asString();
				for (char ch : s)
					pool.push_back(Value::String(string(1, ch)));
			} else
				throw TypeError("Sample requires a container", l, c);
			if (k > (long long)pool.size())
				throw ValueError("Sample larger than population", l, c);
			vector<Value> result;
			result.reserve(k);
			std::sample(pool.begin(), pool.end(), std::back_inserter(result), k,
				getGen());
			std::shuffle(result.begin(), result.end(), getGen());
			vector<Value> deepResult;
			for (auto &val : result)
				deepResult.push_back(deepCopy(val));
			if (v.type == ValueType::LIST || v.type == ValueType::RANGE)
				return Value::List(deepResult);
			if (v.type == ValueType::TUPLE)
				return Value::Tuple(deepResult);
			if (v.type == ValueType::SET) {
				std::unordered_set<Value, ValueHash, ValueEqual> res(deepResult.begin(), deepResult.end());
				return Value::Set(res);
			}
			if (isString) {
				string s = "";
				for (const auto &val : deepResult)
					s += val.asString();
				return Value::String(s);
			}
			return Value::None();
		});
		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Random", modVal, true, true);
	};
	modules["Vector"] = [&](std::shared_ptr<Env> env, const vector<string> &symbols) {
		this->vectorEnabled = true;
		auto vecConstructor = [](const vector<Value> &args, int l, int c) {
			vector<Value> elems;
			if (args.empty())
				elems = {Value::Float(0.0), Value::Float(0.0), Value::Float(0.0)};
			else {
				for (const auto &arg : args) {
					if (!arg.isNumber())
						throw TypeError("Vector arguments must be numbers", l, c);
					elems.push_back(arg);
				}
			}
			return Value::Vector(elems);
		};
		if (symbols.empty())
			env->set("vector", Value::Native(vecConstructor), true);
		else
			for (const auto &s : symbols)
				if (s == "vector")
					env->set("vector", Value::Native(vecConstructor), true);
	};
}
