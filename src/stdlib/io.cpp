#include "ylang/vm.hpp"
#include "objects.hpp"
#include "types.hpp"

#ifdef _WIN32
#include <shellapi.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif

void Interpreter::registerIoModules() {
	modules["FileStream"] = [](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("FileStream");
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
		define("Open", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Open() expects exactly 1 argument (path)", l,
					c);
			string path = valueToString(args[0]);
			auto *fObj = new FileObject(path);
			if (!fObj->isOpen) {
				delete fObj;
				throw FileNotFoundError("Cannot find or open file: " + path, l, c);
			}
			Value v;
			v.type = ValueType::FILE;
			v.ref = std::shared_ptr<HeapObject>(fObj);
			return v;
		});
		define("SafeOpen", [](const vector<Value> &args, int l, int c) {
			if (args.size() < 1 || args.size() > 2)
				throw ArgumentError(
					"SafeOpen() expects path and optional failure value", l, c);
			string path = valueToString(args[0]);
			Value failVal = (args.size() == 2) ? args[1] : Value::None();
			auto *fObj = new FileObject(path);
			if (!fObj->isOpen) {
				delete fObj;
				return failVal;
			}
			Value v;
			v.type = ValueType::FILE;
			v.ref = std::shared_ptr<HeapObject>(fObj);
			return v;
		});
		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("FileStream", modVal, false, false);
	};
	modules["Os"] = [](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Os");
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
		define("Make", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("Make() expects (path, name)", l, c);
			string loc = valueToString(args[0]);
			string name = valueToString(args[1]);
			string fullPath = loc;
			if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\')
				fullPath += "/";
			fullPath += name;
			std::ofstream outfile(fullPath);
			if (!outfile)
				throw PermissionError("Cannot create file at: " + fullPath, l, c);
			outfile.close();
			return Value::None();
		});
		define("Remove", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Remove() expects 1 argument (path)", l, c);
			string path = valueToString(args[0]);
			if (std::remove(path.c_str()) != 0)
				throw PermissionError("Cannot remove: " + path, l, c);
			return Value::None();
		});
		define("Exists", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("Exists() expects 2 arguments (path, name)", l, c);
			string loc = valueToString(args[0]);
			string name = valueToString(args[1]);
			string fullPath = loc;
			if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\')
				fullPath += "/";
			fullPath += name;
			std::ifstream f(fullPath);
			return Value::Bool(f.good());
		});
		define("ListNames", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ListNames() expects 1 argument (path)", l, c);
			string pathStr = valueToString(args[0]);
			fs::path p(pathStr);
			if (!fs::exists(p))
				throw FileNotFoundError("Path not found: " + pathStr, l, c);
			vector<Value> results;
			if (fs::is_directory(p)) {
				for (const auto &entry : fs::directory_iterator(p)) {
					results.push_back(
						Value::String(entry.path().filename().string()));
				}
			} else if (fs::is_regular_file(p)) {
				std::ifstream file(p);
				string line;
				while (std::getline(file, line)) {
					if (!line.empty() && line.back() == '\r')
						line.pop_back();
					results.push_back(Value::String(line));
				}
			} else
				throw FileNotFoundError(
					"Path is not a valid file or directory: " + pathStr, l, c);
			return Value::List(results);
		});
		define("MkDir", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("MkDir() expects 1 argument (path)", l, c);
			string path = valueToString(args[0]);
			if (!fs::create_directories(path)) {
			}
			return Value::None();
		});
		define("RmDir", [](const vector<Value> &args, int l, int c) {
			if (args.size() < 1 || args.size() > 2)
				throw ArgumentError(
					"RmDir() expects 2 arguments (path, recursive?)", l, c);
			string path = valueToString(args[0]);
			bool recursive = (args.size() == 2) ? args[1].asBool() : false;
			if (recursive)
				fs::remove_all(path);
			else
				fs::remove(path);
			return Value::None();
		});
		define("Cwd", [](const vector<Value> &args, int l, int c) {
			return Value::String(fs::current_path().string());
		});
		define("Cd", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Cd() expects 1 argument (path)", l, c);
			string path = valueToString(args[0]);
			try {
				fs::current_path(path);
			} catch (const fs::filesystem_error &e) {
				throw FileNotFoundError("Cannot change directory to: " + path, l,
					c);
			}
			return Value::None();
		});
		define("Rename", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("Rename() expects 2 arguments (old, new)", l,
					c);
			string oldP = valueToString(args[0]);
			string newP = valueToString(args[1]);
			try {
				fs::rename(oldP, newP);
			} catch (...) {
				throw PermissionError("Rename failed", l, c);
			}
			return Value::None();
		});
		define("Env", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Env() expects 1 argument (name)", l, c);
			const char *val = std::getenv(valueToString(args[0]).c_str());
			return val ? Value::String(val) : Value::None();
		});
		define("SetEnv", [](const vector<Value> &args, int l, int c) {
			string name = args[0].asString();
			string val = args[1].asString();
#ifdef _WIN32
			_putenv_s(name.c_str(), val.c_str());
#else
			setenv(name.c_str(), val.c_str(), 1);
#endif
			return Value::None();
		});
		define("Copy", [](const vector<Value> &args, int l, int c) {
			fs::copy(args[0].asString(), args[1].asString(), fs::copy_options::overwrite_existing);
			return Value::None();
		});
		define("FileSize", [](const vector<Value> &args, int l, int c) {
			return Value::Int(fs::file_size(args[0].asString()));
		});
		define("IsDir", [](const vector<Value> &args, int l, int c) {
			return Value::Bool(fs::is_directory(args[0].asString()));
		});
		define("IsFile", [](const vector<Value> &args, int l, int c) {
			return Value::Bool(fs::is_regular_file(args[0].asString()));
		});
		define("Direct", [](const vector<Value> &args, int l, int c) {
			string cmd = args[0].asString();
			string file = args[1].asString();
			string full_cmd = cmd + " > " + file + " 2>&1";
			return Value::Int(std::system(full_cmd.c_str()));
		});
		define("Console", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Console() expects 1 argument (command)", l, c);
			string cmd = valueToString(args[0]);
			int result = std::system(cmd.c_str());
			return Value::Int(result);
		});
		define("Exec", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Exec() expects 1 argument (command)", l, c);
			string cmd = valueToString(args[0]);
			string result = "";
			char buffer[128];
#ifdef _WIN32
			FILE *pipe = _popen(cmd.c_str(), "r");
#else
         FILE* pipe = popen(cmd.c_str(), "r");
#endif
			if (!pipe)
				throw RuntimeError("popen() failed!", l, c);

			while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
				result += buffer;
			}

#ifdef _WIN32
			_pclose(pipe);
#else
         pclose(pipe);
#endif
			return Value::String(result);
		});
		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Os", modVal, false, false);
	};
	modules["System"] = [](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("System");
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
		define("Exit", [](const vector<Value> &args, int l, int c) {
			int code = (args.size() > 0) ? (int)args[0].asInt() : 0;
			exit(code);
			return Value::None(); // Never reached
		});
		define("Args", [](const vector<Value> &args, int l, int c) {
			vector<Value> argsList;
#ifdef _WIN32
			int nArgs;
			LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
			if (szArglist != NULL) {
				for (int i = 0; i < nArgs; i++) {
					std::wstring ws(szArglist[i]);
					std::string s(ws.begin(), ws.end());
					argsList.push_back(Value::String(s));
				}
				LocalFree(szArglist);
			}
#else
			std::ifstream cmdline("/proc/self/cmdline");
			std::string arg;
			while (std::getline(cmdline, arg, '\0')) {
				argsList.push_back(Value::String(arg));
			}
#endif
			return Value::List(argsList);
		});
		define("ProcessId", [](const vector<Value> &args, int l, int c) {
#ifdef _WIN32
			return Value::Int((int)GetCurrentProcessId());
#else
			return Value::Int((int)getpid());
#endif
		});
		define("Platform", [](const vector<Value> &args, int l, int c) {
#ifdef _WIN32
			return Value::String("windows");
#elif __APPLE__
			return Value::String("macos");
#elif __linux__
			return Value::String("linux");
#else
			return Value::String("unknown");
#endif
		});
		define("Color", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("Color() expects 2 arguments (text, colorName)",
					l, c);
			string text = valueToString(args[0]);
			string color = valueToString(args[1]);
			string code = "37";
			if (color == "red")
				code = "31";
			else if (color == "green")
				code = "32";
			else if (color == "yellow")
				code = "33";
			else if (color == "blue")
				code = "34";
			else if (color == "magenta")
				code = "35";
			else if (color == "cyan")
				code = "36";
			else if (color == "white")
				code = "37";
			else if (color == "reset")
				code = "0";
			else
				throw ValueError("invalid color", l, c);
			return Value::String("\033[" + code + "m" + text + "\033[0m");
		});
		define("Beep", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError(
					"Beep() expects 2 arguments (frequency, duration)", l, c);
			if (args[0].asFloat() <= 0 || args[1].asFloat() <= 0)
				throw ValueError("Frequency or duration should be positive", l, c);
#ifdef _WIN32
			Beep(args[0].asFloat(), args[1].asFloat());
#else
			std::cout << '\a' << std::flush;
#endif
			return Value::None();
		});
		define("ReadKey", [](const vector<Value> &args, int l, int c) {
			char ch = 0;
#ifdef _WIN32
			// Windows has native getch() in <conio.h>
			ch = _getch();
#else
         struct termios oldt, newt;
         tcgetattr(STDIN_FILENO, &oldt);
         newt = oldt;
         newt.c_lflag &= ~(ICANON | ECHO);
         tcsetattr(STDIN_FILENO, TCSANOW, &newt);
         ch = getchar();
         tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
			return Value::String(string(1, ch));
		});
		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("System", modVal, false, false);
	};
}
