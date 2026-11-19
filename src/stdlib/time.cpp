#include "ylang/vm.hpp"
#include "objects.hpp"
#include "types.hpp"

#ifdef _WIN32
#include <shellapi.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif

void Interpreter::registerTimeModule() {
	modules["Time"] = [](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Time");
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
		define("Sleep", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Sleep(milliseconds)", l, c);
			long long ms = args[0].asInt();
			std::this_thread::sleep_for(std::chrono::milliseconds(ms));
			return Value::None();
		});
		define("Now", [](const vector<Value> &args, int l, int c) {
			auto now = std::chrono::system_clock::now();
			auto duration = now.time_since_epoch();
			double seconds = std::chrono::duration<double>(duration).count();
			return Value::Float(seconds);
		});
		define("Clock", [](const vector<Value> &args, int l, int c) {
			auto now = std::chrono::high_resolution_clock::now();
			double ms =
				std::chrono::duration<double, std::milli>(now.time_since_epoch())
					.count();
			return Value::Float(ms);
		});
		define("Format", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("Format(timestamp, formatStr)", l, c);
			time_t rawTime = (time_t)args[0].asFloat();
			string fmt = valueToString(args[1]);
			struct tm *timeInfo = std::localtime(&rawTime);
			char buffer[80];
			std::strftime(buffer, 80, fmt.c_str(), timeInfo);
			return Value::String(string(buffer));
		});
		define("Local", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Local(timestamp)", l, c);
			time_t rawTime = (time_t)args[0].asFloat();
			struct tm *t = std::localtime(&rawTime);
			unordered_map<Value, Value, ValueHash, ValueEqual> parts;
			parts[Value::String("year")] = Value::Int(t->tm_year + 1900);
			parts[Value::String("month")] = Value::Int(t->tm_mon + 1);
			parts[Value::String("weekday")] = Value::Int(t->tm_wday);
			parts[Value::String("day")] = Value::Int(t->tm_mday);
			parts[Value::String("hour")] = Value::Int(t->tm_hour);
			parts[Value::String("min")] = Value::Int(t->tm_min);
			parts[Value::String("sec")] = Value::Int(t->tm_sec);
			return Value::Dict(parts);
		});
		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Time", modVal, false, false);
	};
}
