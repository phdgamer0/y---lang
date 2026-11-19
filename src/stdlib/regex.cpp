#include "ylang/vm.hpp"
#include "objects.hpp"
#include "types.hpp"

#ifdef _WIN32
#include <shellapi.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif

void Interpreter::registerRegexModule() {

#ifndef _WIN32

#endif

	modules["Regex"] = [this](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Regex");
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

		auto getSubject = [&](const Value &arg, int l, int c) -> string {
			if (arg.type == ValueType::STRING) {
				return arg.asString();
			} else if (arg.type == ValueType::FILE) {
				auto *fileObj = static_cast<FileObject *>(arg.ref.get());
				if (!fileObj->isOpen)
					throw RuntimeError("File is closed", l, c);
				fileObj->stream.clear();
				fileObj->stream.seekg(0, std::ios::beg);
				std::stringstream buffer;
				buffer << fileObj->stream.rdbuf();
				return buffer.str();
			}
			throw ArgumentError("Regex subject must be a String or a File", l, c);
		};

		define("match", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("Regex.match(pattern, string/file)", l, c);
			string pattern = args[0].asString();
			string subject = getSubject(args[1], l, c);
			try {
				std::regex re(pattern);
				return Value::Bool(std::regex_match(subject, re));
			} catch (const std::regex_error &e) {
				throw RuntimeError(string("Regex error: ") + e.what(), l, c);
			}
		});

		define("search", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("Regex.search(pattern, string/file)", l, c);
			string pattern = args[0].asString();
			string subject = getSubject(args[1], l, c);
			try {
				std::regex re(pattern);
				std::smatch m;
				if (std::regex_search(subject, m, re)) {
					std::vector<Value> results;
					for (auto &match : m) {
						results.push_back(Value::String(match.str()));
					}
					return Value::List(results);
				}
				return Value::List({});
			} catch (const std::regex_error &e) {
				throw RuntimeError(string("Regex error: ") + e.what(), l, c);
			}
		});

		define("find_all", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("Regex.find_all(pattern, string/file)", l, c);
			string pattern = args[0].asString();
			string subject = getSubject(args[1], l, c);
			try {
				std::regex re(pattern);
				std::sregex_iterator next(subject.begin(), subject.end(), re);
				std::sregex_iterator end;
				std::vector<Value> results;
				while (next != end) {
					std::smatch match = *next;
					results.push_back(Value::String(match.str()));
					next++;
				}
				return Value::List(results);
			} catch (const std::regex_error &e) {
				throw RuntimeError(string("Regex error: ") + e.what(), l, c);
			}
		});

		define("replace", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("Regex.replace(pattern, string/file, replacement)", l, c);
			string pattern = args[0].asString();
			string subject = getSubject(args[1], l, c);
			string repl = args[2].asString();
			try {
				std::regex re(pattern);
				string result = std::regex_replace(subject, re, repl);
				return Value::String(result);
			} catch (const std::regex_error &e) {
				throw RuntimeError(string("Regex error: ") + e.what(), l, c);
			}
		});

		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Regex", modVal, false, false);
	};
}
