#include "ylang/vm.hpp"

#ifdef _WIN32
#include <shellapi.h>
#endif

void Interpreter::registerStdLib() {
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

	modules["Serial"] = [this](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Serial");
		moduleNamespace->mro.push_back(moduleNamespace.get());
		static auto serialClass = std::make_shared<ClassObject>("SerialObj");
		auto define = [&](string name, NativeFunc f) {
			moduleNamespace->staticFields[name] = Value::Native(f);
		};
		serialClass->methods["read"] = ClassObject::MethodInfo{
			.func = Value::Native([](const vector<Value> &args, int l, int c) -> Value {
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				int n = args[2].asInt();
#ifdef _WIN32
				HANDLE hComm = (HANDLE)self->fields["ptr"].aspInt();
				char* buf = new char[n + 1];
				DWORD bytesRead = 0;
				ReadFile(hComm, buf, n, &bytesRead, NULL);
				buf[bytesRead] = '\0';
				string res(buf, bytesRead);
				delete[] buf;
				return Value::String(res);
#else
				int fd = (int)(intptr_t)self->fields["ptr"].aspInt();
				char* buf = new char[n + 1];
				int bytesRead = ::read(fd, buf, n);
				if (bytesRead < 0) bytesRead = 0;
				buf[bytesRead] = '\0';
				string res(buf, bytesRead);
				delete[] buf;
				return Value::String(res);
#endif
			}),
			.access = AccessLevel::PUBLIC
		};
		serialClass->methods["write"] = ClassObject::MethodInfo{
			.func = Value::Native([](const vector<Value> &args, int l, int c) -> Value {
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				string data = args[2].asString();
#ifdef _WIN32
				HANDLE hComm = (HANDLE)self->fields["ptr"].aspInt();
				DWORD bytesWritten = 0;
				WriteFile(hComm, data.c_str(), data.length(), &bytesWritten, NULL);
				return Value::Int(bytesWritten);
#else
				int fd = (int)(intptr_t)self->fields["ptr"].aspInt();
				int bytesWritten = ::write(fd, data.c_str(), data.length());
				return Value::Int(bytesWritten);
#endif
			}),
			.access = AccessLevel::PUBLIC
		};
		serialClass->methods["close"] = ClassObject::MethodInfo{
			.func = Value::Native([](const vector<Value> &args, int l, int c) -> Value {
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
#ifdef _WIN32
				HANDLE hComm = (HANDLE)self->fields["ptr"].aspInt();
				CloseHandle(hComm);
#else
				int fd = (int)(intptr_t)self->fields["ptr"].aspInt();
				::close(fd);
#endif
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};
		define("open", [=](const vector<Value> &args, int l, int c) -> Value {
			string port = args[0].asString();
			int baud = args[1].asInt();
			auto inst = std::make_shared<InstanceObject>(serialClass.get());
#ifdef _WIN32
			HANDLE hComm = CreateFileA(port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (hComm == INVALID_HANDLE_VALUE) throw RuntimeError("Failed to open port", l, c);
			DCB dcbSerialParams = {0};
			dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
			GetCommState(hComm, &dcbSerialParams);
			dcbSerialParams.BaudRate = baud;
			dcbSerialParams.ByteSize = 8;
			dcbSerialParams.StopBits = ONESTOPBIT;
			dcbSerialParams.Parity = NOPARITY;
			SetCommState(hComm, &dcbSerialParams);
			COMMTIMEOUTS timeouts = {0};
			timeouts.ReadIntervalTimeout = 50;
			timeouts.ReadTotalTimeoutConstant = 50;
			timeouts.ReadTotalTimeoutMultiplier = 10;
			timeouts.WriteTotalTimeoutConstant = 50;
			timeouts.WriteTotalTimeoutMultiplier = 10;
			SetCommTimeouts(hComm, &timeouts);
			inst->fields["ptr"] = Value::pInt((void*)hComm);
#else
			int fd = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
			if (fd < 0) throw RuntimeError("Failed to open port", l, c);
			struct termios tty;
			if (tcgetattr(fd, &tty) != 0) throw RuntimeError("Error from tcgetattr", l, c);
			cfsetospeed(&tty, baud);
			cfsetispeed(&tty, baud);
			tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
			tty.c_iflag &= ~IGNBRK;
			tty.c_lflag = 0;
			tty.c_oflag = 0;
			tty.c_cc[VMIN]  = 0;
			tty.c_cc[VTIME] = 5;
			tty.c_iflag &= ~(IXON | IXOFF | IXANY);
			tty.c_cflag |= (CLOCAL | CREAD);
			tty.c_cflag &= ~(PARENB | PARODD);
			tty.c_cflag &= ~CSTOPB;
			tty.c_cflag &= ~CRTSCTS;
			if (tcsetattr(fd, TCSANOW, &tty) != 0) throw RuntimeError("Error from tcsetattr", l, c);
			inst->fields["ptr"] = Value::pInt((void*)(intptr_t)fd);
#endif
			Value v; v.type = ValueType::INSTANCE; v.ref = inst;
			return v;
		});
		Value modVal; modVal.type = ValueType::CLASS; modVal.ref = moduleNamespace;
		env->set("Serial", modVal, false, false);
	};

	modules["Events"] = [this](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Events");
		moduleNamespace->mro.push_back(moduleNamespace.get());
		auto define = [&](string name, NativeFunc f) {
			moduleNamespace->staticFields[name] = Value::Native(f);
		};

#ifndef _WIN32
		// Setup global display for XTest
		static Display* x_display = XOpenDisplay(NULL);
#endif

		define("key_down", [=](const vector<Value> &args, int l, int c) {
			int key = (int)args[0].asInt();
#ifdef _WIN32
			INPUT input = {0};
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = key;
			SendInput(1, &input, sizeof(INPUT));
#else
			if (x_display) {
				KeyCode code = XKeysymToKeycode(x_display, key);
				if (code != 0) { XTestFakeKeyEvent(x_display, code, true, CurrentTime); XFlush(x_display); }
			}
#endif
			return Value::None();
		});

		define("key_up", [=](const vector<Value> &args, int l, int c) {
			int key = (int)args[0].asInt();
#ifdef _WIN32
			INPUT input = {0};
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = key;
			input.ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(1, &input, sizeof(INPUT));
#else
			if (x_display) {
				KeyCode code = XKeysymToKeycode(x_display, key);
				if (code != 0) { XTestFakeKeyEvent(x_display, code, false, CurrentTime); XFlush(x_display); }
			}
#endif
			return Value::None();
		});

		define("key_press", [=](const vector<Value> &args, int l, int c) {
			int key = (int)args[0].asInt();
#ifdef _WIN32
			INPUT inputs[2] = {0};
			inputs[0].type = INPUT_KEYBOARD;
			inputs[0].ki.wVk = key;
			inputs[1].type = INPUT_KEYBOARD;
			inputs[1].ki.wVk = key;
			inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(2, inputs, sizeof(INPUT));
#else
			if (x_display) {
				KeyCode code = XKeysymToKeycode(x_display, key);
				if (code != 0) { 
					XTestFakeKeyEvent(x_display, code, true, CurrentTime); 
					XTestFakeKeyEvent(x_display, code, false, CurrentTime); 
					XFlush(x_display); 
				}
			}
#endif
			return Value::None();
		});

		define("mouse_move", [=](const vector<Value> &args, int l, int c) {
			int x = (int)args[0].asInt();
			int y = (int)args[1].asInt();
#ifdef _WIN32
			SetCursorPos(x, y);
#else
			if (x_display) { XTestFakeMotionEvent(x_display, -1, x, y, CurrentTime); XFlush(x_display); }
#endif
			return Value::None();
		});

		define("mouse_down", [=](const vector<Value> &args, int l, int c) {
			int btn = (int)args[0].asInt();
#ifdef _WIN32
			INPUT input = {0}; input.type = INPUT_MOUSE;
			if (btn == 0) input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			else if (btn == 1) input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
			else if (btn == 2) input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
			SendInput(1, &input, sizeof(INPUT));
#else
			if (x_display) {
				int xbtn = (btn == 0) ? 1 : ((btn == 1) ? 3 : 2);
				XTestFakeButtonEvent(x_display, xbtn, true, CurrentTime); XFlush(x_display);
			}
#endif
			return Value::None();
		});

		define("mouse_up", [=](const vector<Value> &args, int l, int c) {
			int btn = (int)args[0].asInt();
#ifdef _WIN32
			INPUT input = {0}; input.type = INPUT_MOUSE;
			if (btn == 0) input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
			else if (btn == 1) input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
			else if (btn == 2) input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
			SendInput(1, &input, sizeof(INPUT));
#else
			if (x_display) {
				int xbtn = (btn == 0) ? 1 : ((btn == 1) ? 3 : 2);
				XTestFakeButtonEvent(x_display, xbtn, false, CurrentTime); XFlush(x_display);
			}
#endif
			return Value::None();
		});

		define("mouse_click", [=](const vector<Value> &args, int l, int c) {
			int btn = (int)args[0].asInt();
#ifdef _WIN32
			INPUT inputs[2] = {0};
			inputs[0].type = INPUT_MOUSE; inputs[1].type = INPUT_MOUSE;
			if (btn == 0) { inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN; inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP; }
			else if (btn == 1) { inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP; }
			else if (btn == 2) { inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN; inputs[1].mi.dwFlags = MOUSEEVENTF_MIDDLEUP; }
			SendInput(2, inputs, sizeof(INPUT));
#else
			if (x_display) {
				int xbtn = (btn == 0) ? 1 : ((btn == 1) ? 3 : 2);
				XTestFakeButtonEvent(x_display, xbtn, true, CurrentTime);
				XTestFakeButtonEvent(x_display, xbtn, false, CurrentTime);
				XFlush(x_display);
			}
#endif
			return Value::None();
		});

		define("mouse_click_every", [=](const vector<Value> &args, int l, int c) {
			int btn = (int)args[0].asInt();
			int interval = (int)args[1].asInt();
			int escape_key = (int)args[2].asInt();
			auto isKeyDown = [=](int key) -> bool {
#ifdef _WIN32
				return (GetAsyncKeyState(key) & 0x8000) != 0;
#else
				if (!x_display) return false;
				char keys_return[32];
				XQueryKeymap(x_display, keys_return);
				KeyCode kc = XKeysymToKeycode(x_display, key);
				if (kc == 0) return false;
				return (keys_return[kc / 8] & (1 << (kc % 8))) != 0;
#endif
			};
			while (!isKeyDown(escape_key)) {
#ifdef _WIN32
				INPUT inputs[2] = {0};
				inputs[0].type = INPUT_MOUSE; inputs[1].type = INPUT_MOUSE;
				if (btn == 0) { inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN; inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP; }
				else if (btn == 1) { inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP; }
				else if (btn == 2) { inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN; inputs[1].mi.dwFlags = MOUSEEVENTF_MIDDLEUP; }
				SendInput(2, inputs, sizeof(INPUT));
#else
				if (x_display) {
					int xbtn = (btn == 0) ? 1 : ((btn == 1) ? 3 : 2);
					XTestFakeButtonEvent(x_display, xbtn, true, CurrentTime);
					XTestFakeButtonEvent(x_display, xbtn, false, CurrentTime);
					XFlush(x_display);
				}
#endif
				std::this_thread::sleep_for(std::chrono::milliseconds(interval));
			}
			return Value::None();
		});

		Value modVal; modVal.type = ValueType::CLASS; modVal.ref = moduleNamespace;
		env->set("Events", modVal, false, false);
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
				for(int i=0; i<nArgs; i++) {
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
	
modules["Crypto"] = [](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Crypto");
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
		define("MD5", [](const vector<Value> &args, int l, int c) {
			if (args.empty()) return Value::String("");
			return Value::String(computeHash<HL_MD5>(args[0]));
		});
		define("SHA1", [](const vector<Value> &args, int l, int c) {
			if (args.empty()) return Value::String("");
			return Value::String(computeHash<HL_SHA1>(args[0]));
		});
		define("SHA256", [](const vector<Value> &args, int l, int c) {
			if (args.empty()) return Value::String("");
			return Value::String(computeHash<HL_SHA256>(args[0]));
		});
		define("SHA3", [](const vector<Value> &args, int l, int c) {
			if (args.empty()) return Value::String("");
			return Value::String(computeHash<HL_SHA3>(args[0]));
		});
		define("Keccak", [](const vector<Value> &args, int l, int c) {
			if (args.empty()) return Value::String("");
			return Value::String(computeHash<HL_Keccak>(args[0]));
		});
		define("DoubleSHA256", [](const vector<Value> &args, int l, int c) {
			if (args.empty()) return Value::String("");
			return Value::String(computeHash<DoubleSHA256>(args[0]));
		});
		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Crypto", modVal, false, false);
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
	modules["Raylib"] = [](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Raylib");
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
		static auto colorClass = std::make_shared<ClassObject>("Color");
		static auto imageClass = std::make_shared<ClassObject>("Image");
		static auto textureClass = std::make_shared<ClassObject>("Texture2D");
		static auto renderClass = std::make_shared<ClassObject>("RenderTexture");
		static auto fontClass = std::make_shared<ClassObject>("Font");
		static auto soundClass = std::make_shared<ClassObject>("Sound");
		static auto musicClass = std::make_shared<ClassObject>("Music");
		colorClass->methods["__display__"] = ClassObject::MethodInfo{
			.func = Value::Native([&](const std::vector<Value> &args, int line, int col) -> Value {
				if (args.empty() || args[0].type != ValueType::INSTANCE)
					return Value::String("<invalid color>");
				auto *inst = static_cast<InstanceObject *>(args[0].ref.get());
				int r = inst->fields["r"].asInt();
				int g = inst->fields["g"].asInt();
				int b = inst->fields["b"].asInt();
				int a = inst->fields["a"].asInt();
				std::string info = "RGBA(" + to_string(r) + ", " + to_string(g) + ", " + to_string(b) + ", " + to_string(a) + ") " + "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m██\033[0m";
				return Value::String(info);
			}),
			.access = AccessLevel::PUBLIC};
		auto ImageToValue = [&](const Image &img) -> Value {
			auto inst = std::make_shared<InstanceObject>(imageClass.get());
			inst->fields["data"] = Value::pInt(img.data);
			inst->fields["width"] = Value::Int(img.width);
			inst->fields["height"] = Value::Int(img.height);
			inst->fields["mipmaps"] = Value::Int(img.mipmaps);
			inst->fields["format"] = Value::Int(img.format);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};
		auto ValueToImage = [&](Value v, int l, int c) -> Image {
			if (v.type != ValueType::INSTANCE)
				throw TypeError("Expected Image instance", l, c);
			auto inst = static_cast<InstanceObject *>(v.ref.get());
			if (inst->klass->name != "Image")
				throw TypeError("Expected Image instance", l, c);
			Image img;
			img.data = inst->fields["data"].aspInt();
			img.width = (int)inst->fields["width"].asInt();
			img.height = (int)inst->fields["height"].asInt();
			img.mipmaps = (int)inst->fields["mipmaps"].asInt();
			img.format = (int)inst->fields["format"].asInt();
			return img;
		};
		auto TextureToValue = [&](const Texture2D &tex) -> Value {
			auto inst = std::make_shared<InstanceObject>(textureClass.get());
			inst->fields["id"] = Value::Int(tex.id); // Store OpenGL Texture ID
			inst->fields["width"] = Value::Int(tex.width);
			inst->fields["height"] = Value::Int(tex.height);
			inst->fields["mipmaps"] = Value::Int(tex.mipmaps);
			inst->fields["format"] = Value::Int(tex.format);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};
		auto ValueToTexture = [&](Value v, int l, int c) -> Texture2D {
			if (v.type != ValueType::INSTANCE)
				throw TypeError("Expected Texture2D instance", l, c);
			auto inst = static_cast<InstanceObject *>(v.ref.get());
			if (inst->klass->name != "Texture2D")
				throw TypeError("Expected Texture2D instance", l, c);
			Texture2D tex;
			tex.id = (unsigned int)inst->fields["id"].asInt();
			tex.width = (int)inst->fields["width"].asInt();
			tex.height = (int)inst->fields["height"].asInt();
			tex.mipmaps = (int)inst->fields["mipmaps"].asInt();
			tex.format = (int)inst->fields["format"].asInt();
			return tex;
		};
		auto ValueToRenderTexture = [&](Value v, int l, int c) -> RenderTexture2D {
			// Type Safety Check
			if (v.type != ValueType::INSTANCE)
				throw TypeError("Expected RenderTexture instance", l, c);
			auto inst = static_cast<InstanceObject *>(v.ref.get());
			if (inst->klass->name != "RenderTexture")
				throw TypeError("Expected RenderTexture instance", l, c);
			RenderTexture2D target;
			target.id = (unsigned int)inst->fields["id"].asInt();
			target.texture = ValueToTexture(inst->fields["texture"], l, c);
			target.depth = ValueToTexture(inst->fields["depth"], l, c);
			return target;
		};
		auto RenderTextureToValue = [&](const RenderTexture2D &target) -> Value {
			auto inst = std::make_shared<InstanceObject>(renderClass.get());
			inst->fields["id"] = Value::Int(target.id);
			inst->fields["texture"] = TextureToValue(target.texture);
			inst->fields["depth"] = TextureToValue(target.depth);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};
		auto MakeColor = [&](int r, int g, int b, int a) -> Value {
			auto inst = std::make_shared<InstanceObject>(colorClass.get());
			inst->fields["r"] = Value::Int(r);
			inst->fields["g"] = Value::Int(g);
			inst->fields["b"] = Value::Int(b);
			inst->fields["a"] = Value::Int(a);

			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			v.isConst = true; // Important: Make default colors constant!
			return v;
		};
		auto FontToValue = [&](const Font &font) -> Value {
			auto inst = std::make_shared<InstanceObject>(fontClass.get());
			inst->fields["baseSize"] = Value::Int(font.baseSize);
			inst->fields["glyphCount"] = Value::Int(font.glyphCount);
			inst->fields["glyphPadding"] = Value::Int(font.glyphPadding);
			inst->fields["texture"] = TextureToValue(font.texture);
			inst->fields["recs"] = Value::pInt(font.recs);
			inst->fields["glyphs"] = Value::pInt(font.glyphs);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};
		auto ValueToFont = [&](Value v, int l, int c) -> Font {
			if (v.type != ValueType::INSTANCE)
				throw TypeError("Expected Font instance", l, c);
			auto inst = static_cast<InstanceObject *>(v.ref.get());
			if (inst->klass->name != "Font")
				throw TypeError("Expected Font instance", l, c);
			Font font;
			font.baseSize = (int)inst->fields["baseSize"].asInt();
			font.glyphCount = (int)inst->fields["glyphCount"].asInt();
			font.glyphPadding = (int)inst->fields["glyphPadding"].asInt();
			font.texture = ValueToTexture(inst->fields["texture"], l, c);
			font.recs = (Rectangle *)inst->fields["recs"].aspInt();
			font.glyphs = (GlyphInfo *)inst->fields["glyphs"].aspInt();
			return font;
		};
		auto SoundToValue = [&](Sound s) -> Value {
			auto inst = std::make_shared<InstanceObject>(soundClass.get());
			inst->fields["buffer"] =
				Value::Int((long long)(uintptr_t)s.stream.buffer);
			inst->fields["processor"] =
				Value::Int((long long)(uintptr_t)s.stream.processor);
			inst->fields["sampleRate"] = Value::Int(s.stream.sampleRate);
			inst->fields["sampleSize"] = Value::Int(s.stream.sampleSize);
			inst->fields["channels"] = Value::Int(s.stream.channels);
			inst->fields["frameCount"] = Value::Int(s.frameCount);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};
		auto MusicToValue = [&](Music m) -> Value {
			auto inst = std::make_shared<InstanceObject>(musicClass.get());
			inst->fields["buffer"] =
				Value::Int((long long)(uintptr_t)m.stream.buffer);
			inst->fields["processor"] =
				Value::Int((long long)(uintptr_t)m.stream.processor);
			inst->fields["sampleRate"] = Value::Int(m.stream.sampleRate);
			inst->fields["sampleSize"] = Value::Int(m.stream.sampleSize);
			inst->fields["channels"] = Value::Int(m.stream.channels);
			inst->fields["frameCount"] = Value::Int(m.frameCount);
			inst->fields["looping"] = Value::Bool(m.looping);
			inst->fields["ctxType"] = Value::Int(m.ctxType);
			inst->fields["ctxData"] = Value::Int((long long)(uintptr_t)m.ctxData);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};
		auto ValueToSound = [&](Value v, int l, int c) -> Sound {
			if (v.type != ValueType::INSTANCE)
				throw ArgumentError("Expected Sound object", l, c);
			auto *inst = static_cast<InstanceObject *>(v.ref.get());
			Sound s;
			s.stream.buffer =
				(rAudioBuffer *)(uintptr_t)inst->fields["buffer"].asInt();
			s.stream.processor =
				(rAudioProcessor *)(uintptr_t)inst->fields["processor"].asInt();
			s.stream.sampleRate = (unsigned int)inst->fields["sampleRate"].asInt();
			s.stream.sampleSize = (unsigned int)inst->fields["sampleSize"].asInt();
			s.stream.channels = (unsigned int)inst->fields["channels"].asInt();
			s.frameCount = (unsigned int)inst->fields["frameCount"].asInt();
			return s;
		};
		auto ValueToMusic = [&](Value v, int l, int c) -> Music {
			if (v.type != ValueType::INSTANCE)
				throw ArgumentError("Expected Music object", l, c);
			auto *inst = static_cast<InstanceObject *>(v.ref.get());
			Music m;
			m.stream.buffer =
				(rAudioBuffer *)(uintptr_t)inst->fields["buffer"].asInt();
			m.stream.processor =
				(rAudioProcessor *)(uintptr_t)inst->fields["processor"].asInt();
			m.stream.sampleRate = (unsigned int)inst->fields["sampleRate"].asInt();
			m.stream.sampleSize = (unsigned int)inst->fields["sampleSize"].asInt();
			m.stream.channels = (unsigned int)inst->fields["channels"].asInt();
			m.frameCount = (unsigned int)inst->fields["frameCount"].asInt();
			m.looping = inst->fields["looping"].asBool();
			m.ctxType = (int)inst->fields["ctxType"].asInt();
			m.ctxData = (void *)(uintptr_t)inst->fields["ctxData"].asInt();
			return m;
		};
		defineValue("LIGHTGRAY", MakeColor(200, 200, 200, 255));
		defineValue("GRAY", MakeColor(130, 130, 130, 255));
		defineValue("DARKGRAY", MakeColor(80, 80, 80, 255));
		defineValue("YELLOW", MakeColor(253, 249, 0, 255));
		defineValue("GOLD", MakeColor(255, 203, 0, 255));
		defineValue("ORANGE", MakeColor(255, 161, 0, 255));
		defineValue("PINK", MakeColor(255, 109, 194, 255));
		defineValue("RED", MakeColor(230, 41, 55, 255));
		defineValue("MAROON", MakeColor(190, 33, 55, 255));
		defineValue("GREEN", MakeColor(0, 228, 48, 255));
		defineValue("LIME", MakeColor(0, 158, 47, 255));
		defineValue("DARKGREEN", MakeColor(0, 117, 44, 255));
		defineValue("SKYBLUE", MakeColor(102, 191, 255, 255));
		defineValue("BLUE", MakeColor(0, 121, 241, 255));
		defineValue("DARKBLUE", MakeColor(0, 82, 172, 255));
		defineValue("PURPLE", MakeColor(200, 122, 255, 255));
		defineValue("VIOLET", MakeColor(135, 60, 190, 255));
		defineValue("DARKPURPLE", MakeColor(112, 31, 126, 255));
		defineValue("BEIGE", MakeColor(211, 176, 131, 255));
		defineValue("BROWN", MakeColor(127, 106, 79, 255));
		defineValue("DARKBROWN", MakeColor(76, 63, 47, 255));
		defineValue("WHITE", MakeColor(255, 255, 255, 255));
		defineValue("BLACK", MakeColor(0, 0, 0, 255));
		defineValue("BLANK", MakeColor(0, 0, 0, 0));
		defineValue("MAGENTA", MakeColor(255, 0, 255, 255));
		defineValue("CYAN", MakeColor(0, 255, 255, 255));
		defineValue("RAYWHITE", MakeColor(245, 245, 245, 255));
		defineValue("TEXTURE_FILTER_POINT", Value::Int(0));
		defineValue("TEXTURE_FILTER_BILINEAR", Value::Int(1));
		defineValue("TEXTURE_FILTER_TRILINEAR", Value::Int(2));
		defineValue("TEXTURE_FILTER_ANISOTROPIC_4X", Value::Int(3));
		defineValue("TEXTURE_FILTER_ANISOTROPIC_8X", Value::Int(4));
		defineValue("TEXTURE_FILTER_ANISOTROPIC_16X", Value::Int(5));
		defineValue("TEXTURE_WRAP_REPEAT", Value::Int(0));
		defineValue("TEXTURE_WRAP_CLAMP", Value::Int(1));
		defineValue("TEXTURE_WRAP_MIRROR_REPEAT", Value::Int(2));
		defineValue("TEXTURE_WRAP_MIRROR_CLAMP", Value::Int(3));
		defineValue("FLAG_VSYNC_HINT", Value::Int(64));
		defineValue("FLAG_FULLSCREEN_MODE", Value::Int(2));
		defineValue("FLAG_WINDOW_RESIZABLE", Value::Int(4));
		defineValue("FLAG_WINDOW_UNDECORATED", Value::Int(8));
		defineValue("FLAG_WINDOW_TRANSPARENT", Value::Int(16));
		defineValue("FLAG_WINDOW_HIDDEN", Value::Int(128));
		defineValue("FLAG_WINDOW_MINIMIZED", Value::Int(512));
		defineValue("FLAG_WINDOW_MAXIMIZED", Value::Int(1024));
		defineValue("FLAG_WINDOW_UNFOCUSED", Value::Int(2048));
		defineValue("FLAG_WINDOW_TOPMOST", Value::Int(4096));
		defineValue("FLAG_WINDOW_ALWAYS_RUN", Value::Int(256));
		defineValue("FLAG_WINDOW_HIGHDPI", Value::Int(8192));
		defineValue("FLAG_WINDOW_MOUSE_PASSTHROUGH", Value::Int(16384));
		defineValue("FLAG_BORDERLESS_WINDOWED_MODE", Value::Int(32768));
		defineValue("FLAG_MSAA_4X_HINT", Value::Int(32));
		defineValue("FLAG_INTERLACED_HINT", Value::Int(65536));
		defineValue("PIXELFORMAT_UNCOMPRESSED_GRAYSCALE", Value::Int(1));
		defineValue("PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA", Value::Int(2));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R5G6B5", Value::Int(3));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R8G8B8", Value::Int(4));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R5G5B5A1", Value::Int(5));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R4G4B4A4", Value::Int(6));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R8G8B8A8", Value::Int(7));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R32", Value::Int(8));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R32G32B32", Value::Int(9));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R32G32B32A32", Value::Int(10));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R16", Value::Int(11));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R16G16B16", Value::Int(12));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R16G16B16A16", Value::Int(13));
		defineValue("PIXELFORMAT_COMPRESSED_DXT1_RGB", Value::Int(14));
		defineValue("PIXELFORMAT_COMPRESSED_DXT1_RGBA", Value::Int(15));
		defineValue("PIXELFORMAT_COMPRESSED_DXT3_RGBA", Value::Int(16));
		defineValue("PIXELFORMAT_COMPRESSED_DXT5_RGBA", Value::Int(17));
		defineValue("PIXELFORMAT_COMPRESSED_ETC1_RGB", Value::Int(18));
		defineValue("PIXELFORMAT_COMPRESSED_ETC2_RGB", Value::Int(19));
		defineValue("PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA", Value::Int(20));
		defineValue("PIXELFORMAT_COMPRESSED_PVRT_RGB", Value::Int(21));
		defineValue("PIXELFORMAT_COMPRESSED_PVRT_RGBA", Value::Int(22));
		defineValue("PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA", Value::Int(23));
		defineValue("PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA", Value::Int(24));
		defineValue("BLEND_ALPHA", Value::Int(0));
		defineValue("BLEND_ADDITIVE", Value::Int(1));
		defineValue("BLEND_MULTIPLIED", Value::Int(2));
		defineValue("BLEND_ADD_COLORS", Value::Int(3));
		defineValue("BLEND_SUBTRACT_COLORS", Value::Int(4));
		defineValue("BLEND_ALPHA_PREMULTIPLY", Value::Int(5));
		defineValue("BLEND_CUSTOM", Value::Int(6));
		defineValue("BLEND_CUSTOM_SEPARATE", Value::Int(7));
		defineValue("KEY_NULL", Value::Int(0));
		defineValue("KEY_APOSTROPHE", Value::Int(39));
		defineValue("KEY_COMMA", Value::Int(44));
		defineValue("KEY_MINUS", Value::Int(45));
		defineValue("KEY_PERIOD", Value::Int(46));
		defineValue("KEY_SLASH", Value::Int(47));
		defineValue("KEY_ZERO", Value::Int(48));
		defineValue("KEY_ONE", Value::Int(49));
		defineValue("KEY_TWO", Value::Int(50));
		defineValue("KEY_THREE", Value::Int(51));
		defineValue("KEY_FOUR", Value::Int(52));
		defineValue("KEY_FIVE", Value::Int(53));
		defineValue("KEY_SIX", Value::Int(54));
		defineValue("KEY_SEVEN", Value::Int(55));
		defineValue("KEY_EIGHT", Value::Int(56));
		defineValue("KEY_NINE", Value::Int(57));
		defineValue("KEY_SEMICOLON", Value::Int(59));
		defineValue("KEY_EQUAL", Value::Int(61));
		defineValue("KEY_A", Value::Int(65));
		defineValue("KEY_B", Value::Int(66));
		defineValue("KEY_C", Value::Int(67));
		defineValue("KEY_D", Value::Int(68));
		defineValue("KEY_E", Value::Int(69));
		defineValue("KEY_F", Value::Int(70));
		defineValue("KEY_G", Value::Int(71));
		defineValue("KEY_H", Value::Int(72));
		defineValue("KEY_I", Value::Int(73));
		defineValue("KEY_J", Value::Int(74));
		defineValue("KEY_K", Value::Int(75));
		defineValue("KEY_L", Value::Int(76));
		defineValue("KEY_M", Value::Int(77));
		defineValue("KEY_N", Value::Int(78));
		defineValue("KEY_O", Value::Int(79));
		defineValue("KEY_P", Value::Int(80));
		defineValue("KEY_Q", Value::Int(81));
		defineValue("KEY_R", Value::Int(82));
		defineValue("KEY_S", Value::Int(83));
		defineValue("KEY_T", Value::Int(84));
		defineValue("KEY_U", Value::Int(85));
		defineValue("KEY_V", Value::Int(86));
		defineValue("KEY_W", Value::Int(87));
		defineValue("KEY_X", Value::Int(88));
		defineValue("KEY_Y", Value::Int(89));
		defineValue("KEY_Z", Value::Int(90));
		defineValue("KEY_LEFT_BRACKET", Value::Int(91));
		defineValue("KEY_BACKSLASH", Value::Int(92));
		defineValue("KEY_RIGHT_BRACKET", Value::Int(93));
		defineValue("KEY_GRAVE", Value::Int(96));
		defineValue("KEY_SPACE", Value::Int(32));
		defineValue("KEY_ESCAPE", Value::Int(256));
		defineValue("KEY_ENTER", Value::Int(257));
		defineValue("KEY_TAB", Value::Int(258));
		defineValue("KEY_BACKSPACE", Value::Int(259));
		defineValue("KEY_INSERT", Value::Int(260));
		defineValue("KEY_DELETE", Value::Int(261));
		defineValue("KEY_RIGHT", Value::Int(262));
		defineValue("KEY_LEFT", Value::Int(263));
		defineValue("KEY_DOWN", Value::Int(264));
		defineValue("KEY_UP", Value::Int(265));
		defineValue("KEY_PAGE_UP", Value::Int(266));
		defineValue("KEY_PAGE_DOWN", Value::Int(267));
		defineValue("KEY_HOME", Value::Int(268));
		defineValue("KEY_END", Value::Int(269));
		defineValue("KEY_CAPS_LOCK", Value::Int(280));
		defineValue("KEY_SCROLL_LOCK", Value::Int(281));
		defineValue("KEY_NUM_LOCK", Value::Int(282));
		defineValue("KEY_PRINT_SCREEN", Value::Int(283));
		defineValue("KEY_PAUSE", Value::Int(284));
		defineValue("KEY_F1", Value::Int(290));
		defineValue("KEY_F2", Value::Int(291));
		defineValue("KEY_F3", Value::Int(292));
		defineValue("KEY_F4", Value::Int(293));
		defineValue("KEY_F5", Value::Int(294));
		defineValue("KEY_F6", Value::Int(295));
		defineValue("KEY_F7", Value::Int(296));
		defineValue("KEY_F8", Value::Int(297));
		defineValue("KEY_F9", Value::Int(298));
		defineValue("KEY_F10", Value::Int(299));
		defineValue("KEY_F11", Value::Int(300));
		defineValue("KEY_F12", Value::Int(301));
		defineValue("KEY_LEFT_SHIFT", Value::Int(340));
		defineValue("KEY_LEFT_CONTROL", Value::Int(341));
		defineValue("KEY_LEFT_ALT", Value::Int(342));
		defineValue("KEY_LEFT_SUPER", Value::Int(343));
		defineValue("KEY_RIGHT_SHIFT", Value::Int(344));
		defineValue("KEY_RIGHT_CONTROL", Value::Int(345));
		defineValue("KEY_RIGHT_ALT", Value::Int(346));
		defineValue("KEY_RIGHT_SUPER", Value::Int(347));
		defineValue("KEY_KB_MENU", Value::Int(348));
		defineValue("KEY_KP_0", Value::Int(320));
		defineValue("KEY_KP_1", Value::Int(321));
		defineValue("KEY_KP_2", Value::Int(322));
		defineValue("KEY_KP_3", Value::Int(323));
		defineValue("KEY_KP_4", Value::Int(324));
		defineValue("KEY_KP_5", Value::Int(325));
		defineValue("KEY_KP_6", Value::Int(326));
		defineValue("KEY_KP_7", Value::Int(327));
		defineValue("KEY_KP_8", Value::Int(328));
		defineValue("KEY_KP_9", Value::Int(329));
		defineValue("KEY_KP_DECIMAL", Value::Int(330));
		defineValue("KEY_KP_DIVIDE", Value::Int(331));
		defineValue("KEY_KP_MULTIPLY", Value::Int(332));
		defineValue("KEY_KP_SUBTRACT", Value::Int(333));
		defineValue("KEY_KP_ADD", Value::Int(334));
		defineValue("KEY_KP_ENTER", Value::Int(335));
		defineValue("KEY_KP_EQUAL", Value::Int(336));
		defineValue("GAMEPAD_BUTTON_UNKNOWN", Value::Int(0));
		defineValue("GAMEPAD_BUTTON_LEFT_FACE_UP", Value::Int(1));
		defineValue("GAMEPAD_BUTTON_LEFT_FACE_RIGHT", Value::Int(2));
		defineValue("GAMEPAD_BUTTON_LEFT_FACE_DOWN", Value::Int(3));
		defineValue("GAMEPAD_BUTTON_LEFT_FACE_LEFT", Value::Int(4));
		defineValue("GAMEPAD_BUTTON_RIGHT_FACE_UP", Value::Int(5));
		defineValue("GAMEPAD_BUTTON_RIGHT_FACE_RIGHT", Value::Int(6));
		defineValue("GAMEPAD_BUTTON_RIGHT_FACE_DOWN", Value::Int(7));
		defineValue("GAMEPAD_BUTTON_RIGHT_FACE_LEFT", Value::Int(8));
		defineValue("GAMEPAD_BUTTON_LEFT_TRIGGER_1", Value::Int(9));
		defineValue("GAMEPAD_BUTTON_LEFT_TRIGGER_2", Value::Int(10));
		defineValue("GAMEPAD_BUTTON_RIGHT_TRIGGER_1", Value::Int(11));
		defineValue("GAMEPAD_BUTTON_RIGHT_TRIGGER_2", Value::Int(12));
		defineValue("GAMEPAD_BUTTON_MIDDLE_LEFT", Value::Int(13));
		defineValue("GAMEPAD_BUTTON_MIDDLE", Value::Int(14));
		defineValue("GAMEPAD_BUTTON_MIDDLE_RIGHT", Value::Int(15));
		defineValue("GAMEPAD_BUTTON_LEFT_THUMB", Value::Int(16));
		defineValue("GAMEPAD_BUTTON_RIGHT_THUMB", Value::Int(17));

		define("Color", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("Color(r, g, b, a)", l, c);
			auto inst = std::make_shared<InstanceObject>(colorClass.get());
			inst->fields["r"] = args[0];
			inst->fields["g"] = args[1];
			inst->fields["b"] = args[2];
			inst->fields["a"] = args[3];
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		});
		define("Rectangle", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("Rectangle(x, y, w, h)", l, c);
			static auto rectClass = std::make_shared<ClassObject>("Rectangle");
			auto inst = std::make_shared<InstanceObject>(rectClass.get());
			inst->fields["x"] = args[0];
			inst->fields["y"] = args[1];
			inst->fields["width"] = args[2];
			inst->fields["height"] = args[3];
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		});
		define("Font", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6)
				throw ArgumentError(
					"Font(baseSize, glyphCount, glyphPadding, texture, recs_ptr, "
					"glyphs_ptr)",
					l, c);
			auto inst = std::make_shared<InstanceObject>(fontClass.get());
			inst->fields["baseSize"] = args[0];
			inst->fields["glyphCount"] = args[1];
			inst->fields["glyphPadding"] = args[2];
			inst->fields["texture"] = args[3];
			inst->fields["recs"] = args[4];
			inst->fields["glyphs"] = args[5];
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		});
		define("FadeColor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError(
					"Fade() takes two arguments, (color, fadeAmount)", l, c);
			Color newColor = Fade(ValueToColor(args[0], l, c), args[1].asFloat());
			auto v = MakeColor(newColor.r, newColor.g, newColor.b, newColor.a);
			v.isConst = false;
			return v;
		});
		define("ColorToGrayFast", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ColorToGrayFast() takes one argument, (color)",
					l, c);
			Color newColor = ValueToColor(args[0], l, c);
			uint8_t G = static_cast<uint8_t>(
				floorf((newColor.r + newColor.g + newColor.b) / 3.0F));
			auto v = MakeColor(G, G, G, newColor.a);
			v.isConst = false;
			return v;
		});
		define("ColorToGray", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ColorToGray() takes one argument, (color)", l,
					c);
			Color newColor = ValueToColor(args[0], l, c);
			float R_linear = powf(newColor.r / 255.0F, 2.2F);
			float G_linear = powf(newColor.g / 255.0F, 2.2F);
			float B_linear = powf(newColor.r / 255.0F, 2.2F);
			float GRAY_linear =
				(0.2126F * R_linear) + (0.7152F * G_linear) + (0.0722F * B_linear);
			uint8_t G = static_cast<uint8_t>(
				floorf(255.0F * powf(GRAY_linear, 1.0F / 2.2F)));
			auto v = MakeColor(G, G, G, newColor.a);
			v.isConst = false;
			return v;
		});
		define("InitWindow", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError(
					"InitWindow() takes three arguments, (width, height, text)", l,
					c);
			InitWindow((int)args[0].asInt(), (int)args[1].asInt(),
				args[2].asString().c_str());
			return Value::None();
		});
		define("ToggleFullscreen", [=](const vector<Value> &args, int l, int c) {
			if (!args.empty())
				throw ArgumentError("ToggleFullscreen() takes no arguments", l, c);
			ToggleFullscreen();
			return Value::None();
		});
		define("CloseWindow", [=](const vector<Value> &args, int l, int c) {
			if (!args.empty())
				throw ArgumentError("CloseWindow() takes no arguments", l, c);
			CloseWindow();
			return Value::None();
		});
		define("WindowShouldClose", [=](const vector<Value> &args, int l, int c) {
			if (!args.empty())
				throw ArgumentError("WindowShouldClose() takes no arguments", l, c);
			return Value::Bool((bool)WindowShouldClose());
		});
		define("BeginDrawing", [=](const vector<Value> &args, int l, int c) {
			BeginDrawing();
			return Value::None();
		});
		define("BeginBlendMode", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError(
					"BeginBlendMode() takes only one argument (mode)", l, c);
			BeginBlendMode(args[0].asInt());
			return Value::None();
		});
		define("EndBlendMode", [=](const vector<Value> &args, int l, int c) {
			EndBlendMode();
			return Value::None();
		});
		define("EndDrawing", [=](const vector<Value> &args, int l, int c) {
			EndDrawing();
			return Value::None();
		});
		define("SetConfigFlags", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() < 1) 
				throw ArgumentError("SetConfigFlags(flags...)", l, c);
			unsigned int flags = 0;
			for (const auto& f : args) flags |= static_cast<unsigned int>(f.asInt());
			SetConfigFlags(flags);
			return Value::None();
		});
		define("SetTextureFilter", [=](const std::vector<Value> &args, int l, int c) {
			if (args.size() < 2) 
				throw ArgumentError("SetTextureFilter(texture, flags...)", l, c);
			unsigned int flags = 0;
			for (auto f = args.begin() + 1; f < args.end(); f++) flags |= static_cast<unsigned int>(f->asInt());
			auto t = ValueToTexture(args[0], l, c);
			SetTextureFilter(t, flags);
			return Value::None();
		});
		define("SetWindowPosition", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 2) 
				throw ArgumentError("SetWindowPosition(x, y)", l, c);
			SetWindowPosition(args[0].asInt(), args[1].asInt());
			return Value::None();
		});
		define("SetWindowPositionV", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 1) 
				throw ArgumentError("SetWindowPositionV(VectorXY)", l, c);
			::Vector2 pos = ValueToVector2(args[0], l, c);
			SetWindowPosition(pos.x, pos.y);
			return Value::None();
		});
		define("GetMonitorWidth", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 1) 
				throw ArgumentError("GetMonitorWidth(monitorNumber)", l, c);
			return Value::Int(GetMonitorWidth(args[0].asInt()));
		});
		define("GetMonitorHeight", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 1) 
				throw ArgumentError("GetMonitorHeight(monitorNumber)", l, c);
			return Value::Int(GetMonitorHeight(args[0].asInt()));
		});
		define("GetWindowScaleV", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 0) 
				throw ArgumentError("GetWindowScaleV()", l, c);
			::Vector2 scale = GetWindowScaleDPI();
			return Vector2ToValue(scale); 
		});
		define("GetWindowScaleF", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 0) 
				throw ArgumentError("GetWindowScaleF()", l, c);
			::Vector2 scale = GetWindowScaleDPI();
			return Value::Float((double)scale.x); 
		});
		define("LoadRenderTexture", [&](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 2) 
				throw ArgumentError("LoadRenderTexture(width, height)", l, c);
			int width = (int)args[0].asInt();
			int height = (int)args[1].asInt();
			RenderTexture2D target = LoadRenderTexture(width, height);
			return RenderTextureToValue(target);
		});
		define("BeginTextureMode", [&](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 1) 
				throw ArgumentError("BeginTextureMode(target)", l, c);
			RenderTexture2D target = ValueToRenderTexture(args[0], l, c);
			BeginTextureMode(target);
			return Value::None();
		});
		define("EndTextureMode", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 0) 
				throw ArgumentError("EndTextureMode()", l, c);
			EndTextureMode();
			return Value::None();
		});
		define("UnloadRenderTexture", [&](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 1) 
				throw ArgumentError("UnloadRenderTexture(target)", l, c);
			RenderTexture2D target = ValueToRenderTexture(args[0], l, c);
			UnloadRenderTexture(target);
			return Value::None();
		});
		define("ClearBackground", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ClearBackground(Color)", l, c);
			ClearBackground(ValueToColor(args[0], l, c));
			return Value::None();
		});
		define("GetFrameTime", [=](const vector<Value> &args, int l, int c) {
			if (!args.empty())
				throw ArgumentError("GetFrameTime()", l, c);
			return Value::Float(GetFrameTime());
		});
		define("SetTargetFPS", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("SetTargetFPS(fps)", l, c);
			SetTargetFPS((int)args[0].asInt());
			return Value::None();
		});
		define("DrawFPS", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("DrawFPS(x, y)", l, c);
			DrawFPS((int)args[0].asInt(), (int)args[1].asInt());
			return Value::None();
		});
		define("GetFontDefault", [=](const vector<Value> &args, int l, int c) {
			if (!args.empty())
				throw ArgumentError("GetFontDefault() takes no arguments", l, c);
			return FontToValue(GetFontDefault());
		});
		define("LoadFont", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("LoadFont(fileName)", l, c);
			Font font = LoadFont(args[0].asString().c_str());
			return FontToValue(font);
		});
		define("LoadFontEx", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("LoadFontEx(fileName, fontSize, fontChars_ptr)",
					l, c);
			// Usually pass Value::Omit() or Value::None() for the 3rd arg to load
			// default ASCII
			int *chars =
				args[2].type == ValueType::NONE ? nullptr : (int *)args[2].aspInt();
			Font font = LoadFontEx(args[0].asString().c_str(),
				(int)args[1].asInt(), chars, 0);
			return FontToValue(font);
		});
		define("UnloadFont", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("UnloadFont(font)", l, c);
			UnloadFont(ValueToFont(args[0], l, c));
			return Value::None();
		});
		define("DrawTextEx", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6)
				throw ArgumentError(
					"DrawTextEx(font, text, positionVec, fontSize, spacing, "
					"tintColor)",
					l, c);
			DrawTextEx(ValueToFont(args[0], l, c), args[1].asString().c_str(),
				ValueToVector2(args[2], l, c), (float)args[3].asFloat(),
				(float)args[4].asFloat(), ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("DrawTextPro", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 8)
				throw ArgumentError(
					"DrawTextPro(font, text, posVec, originVec, rotation, "
					"fontSize, "
					"spacing, tintColor)",
					l, c);
			DrawTextPro(ValueToFont(args[0], l, c), args[1].asString().c_str(),
				ValueToVector2(args[2], l, c),
				ValueToVector2(args[3], l, c), (float)args[4].asFloat(),
				(float)args[5].asFloat(), (float)args[6].asFloat(),
				ValueToColor(args[7], l, c));
			return Value::None();
		});
		define("MeasureTextEx", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("MeasureTextEx(font, text, fontSize, spacing)",
					l, c);
			::Vector2 size = MeasureTextEx(
				ValueToFont(args[0], l, c), args[1].asString().c_str(),
				(float)args[2].asFloat(), (float)args[3].asFloat());
			return Vector2ToValue(size);
		});
		define("DrawText", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("DrawText(str, x, y, size, color)", l, c);
			DrawText(args[0].asString().c_str(), (int)args[1].asInt(),
				(int)args[2].asInt(), (int)args[3].asInt(),
				ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawPixel", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("DrawPixel(x, y, color)", l, c);
			DrawPixel((int)args[0].asInt(), (int)args[1].asInt(),
				ValueToColor(args[2], l, c));
			return Value::None();
		});
		define("DrawPixelV", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("DrawPixelV(vec, color)", l, c);
			DrawPixelV(ValueToVector2(args[0], l, c), ValueToColor(args[1], l, c));
			return Value::None();
		});
		define("DrawLine", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("DrawLine(x1, y1, x2, y2, color)", l, c);
			DrawLine((int)args[0].asInt(), (int)args[1].asInt(),
				(int)args[2].asInt(), (int)args[3].asInt(),
				ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawLineV", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("DrawLineV(vec1, vec2, color)", l, c);
			DrawLineV(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c),
				ValueToColor(args[2], l, c));
			return Value::None();
		});
		define("DrawLineEx", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("DrawLineEx(vec1, vec2, thick, color)", l, c);
			DrawLineEx(ValueToVector2(args[0], l, c),
				ValueToVector2(args[1], l, c), (float)args[2].asFloat(),
				ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawLineBezier", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("DrawLineBezier(vec1, vec2, thick, color)", l,
					c);
			DrawLineBezier(ValueToVector2(args[0], l, c),
				ValueToVector2(args[1], l, c), (float)args[2].asFloat(),
				ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawLineStrip", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("DrawLineStrip(pointList, color)", l, c);
			auto points = ValueToVectorList(args[0], l, c);
			DrawLineStrip(points.data(), (int)points.size(),
				ValueToColor(args[1], l, c));
			return Value::None();
		});
		define("DrawCircle", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("DrawCircle(x, y, radius, color)", l, c);
			DrawCircle((int)args[0].asInt(), (int)args[1].asInt(),
				(float)args[2].asFloat(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawCircleV", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("DrawCircleV(centerVec, radius, color)", l, c);
			DrawCircleV(ValueToVector2(args[0], l, c), (float)args[1].asFloat(),
				ValueToColor(args[2], l, c));
			return Value::None();
		});
		define("DrawCircleLines", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("DrawCircleLines(x, y, radius, color)", l, c);
			DrawCircleLines((int)args[0].asInt(), (int)args[1].asInt(),
				(float)args[2].asFloat(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawCircleGradient", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError(
					"DrawCircleGradient(x, y, radius, inner, outer)", l, c);
			DrawCircleGradient((int)args[0].asInt(), (int)args[1].asInt(),
				(float)args[2].asFloat(),
				ValueToColor(args[3], l, c),
				ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawCircleSector", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 6)
				throw ArgumentError(
					"DrawCircleSector(center, radius, start, end, segs, color)", l,
					c);
			DrawCircleSector(ValueToVector2(args[0], l, c),
				(float)args[1].asFloat(), (float)args[2].asFloat(),
				(float)args[3].asFloat(), (int)args[4].asInt(),
				ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("DrawEllipse", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("DrawEllipse(x, y, radH, radV, color)", l, c);
			DrawEllipse((int)args[0].asInt(), (int)args[1].asInt(),
				(float)args[2].asFloat(), (float)args[3].asFloat(),
				ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawRing", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 7)
				throw ArgumentError(
					"DrawRing(center, inner, outer, start, end, segs, color)", l,
					c);
			DrawRing(ValueToVector2(args[0], l, c), (float)args[1].asFloat(),
				(float)args[2].asFloat(), (float)args[3].asFloat(),
				(float)args[4].asFloat(), (int)args[5].asInt(),
				ValueToColor(args[6], l, c));
			return Value::None();
		});
		define("DrawRectangle", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("DrawRectangle(x, y, w, h, color)", l, c);
			DrawRectangle((int)args[0].asInt(), (int)args[1].asInt(),
				(int)args[2].asInt(), (int)args[3].asInt(),
				ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawRectangleV", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("DrawRectangleV(posVec, sizeVec, color)", l, c);
			DrawRectangleV(ValueToVector2(args[0], l, c),
				ValueToVector2(args[1], l, c),
				ValueToColor(args[2], l, c));
			return Value::None();
		});
		define("DrawRectangleRec", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("DrawRectangleRec(rect, color)", l, c);
			DrawRectangleRec(ValueToRect(args[0], l, c),
				ValueToColor(args[1], l, c));
			return Value::None();
		});
		define("DrawRectanglePro", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("DrawRectanglePro(rect, originVec, rot, color)",
					l, c);
			DrawRectanglePro(
				ValueToRect(args[0], l, c), ValueToVector2(args[1], l, c),
				(float)args[2].asFloat(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawRectangleGradientV", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 6)
				throw ArgumentError(
					"DrawRectangleGradientV(x, y, w, h, topCol, botCol)", l, c);
			DrawRectangleGradientV((int)args[0].asInt(), (int)args[1].asInt(),
				(int)args[2].asInt(), (int)args[3].asInt(),
				ValueToColor(args[4], l, c),
				ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("DrawRectangleRounded", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError(
					"DrawRectangleRounded(rect, roundness, segs, color)", l, c);
			DrawRectangleRounded(
				ValueToRect(args[0], l, c), (float)args[1].asFloat(),
				(int)args[2].asInt(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawTriangle", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("DrawTriangle(v1, v2, v3, color)", l, c);
			DrawTriangle(
				ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c),
				ValueToVector2(args[2], l, c), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawTriangleFan", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("DrawTriangleFan(pointList, color)", l, c);
			auto points = ValueToVectorList(args[0], l, c);
			DrawTriangleFan(points.data(), (int)points.size(),
				ValueToColor(args[1], l, c));
			return Value::None();
		});
		define("DrawPoly", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("DrawPoly(center, sides, radius, rot, color)",
					l, c);
			DrawPoly(ValueToVector2(args[0], l, c), (int)args[1].asInt(),
				(float)args[2].asFloat(), (float)args[3].asFloat(),
				ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawPolyLinesEx", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 6)
				throw ArgumentError(
					"DrawPolyLinesEx(center, sides, radius, rot, thick, color)", l,
					c);
			DrawPolyLinesEx(ValueToVector2(args[0], l, c), (int)args[1].asInt(),
				(float)args[2].asFloat(), (float)args[3].asFloat(),
				(float)args[4].asFloat(), ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("IsKeyPressed", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsKeyPressed(key)", l, c);
			return Value::Bool(IsKeyPressed((int)args[0].asInt()));
		});
		define("IsKeyPressedRepeat", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsKeyPressedRepeat(key)", l, c);
			return Value::Bool(IsKeyPressedRepeat((int)args[0].asInt()));
		});
		define("IsKeyDown", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsKeyDown(key)", l, c);
			return Value::Bool(IsKeyDown((int)args[0].asInt()));
		});
		define("IsKeyReleased", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsKeyReleased(key)", l, c);
			return Value::Bool(IsKeyReleased((int)args[0].asInt()));
		});
		define("IsKeyUp", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsKeyUp(key)", l, c);
			return Value::Bool(IsKeyUp((int)args[0].asInt()));
		});
		define("GetKeyPressed", [](const vector<Value> &args, int l, int c) {
			return Value::Int(GetKeyPressed());
		});
		define("GetCharPressed", [](const vector<Value> &args, int l, int c) {
			return Value::Int(GetCharPressed());
		});
		define("SetExitKey", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("SetExitKey(key)", l, c);
			SetExitKey((int)args[0].asInt());
			return Value::None();
		});
		define("IsMouseButtonPressed", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsMouseButtonPressed(button)", l, c);
			return Value::Bool(IsMouseButtonPressed((int)args[0].asInt()));
		});
		define("IsMouseButtonDown", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsMouseButtonDown(button)", l, c);
			return Value::Bool(IsMouseButtonDown((int)args[0].asInt()));
		});
		define("IsMouseButtonReleased", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsMouseButtonReleased(button)", l, c);
			return Value::Bool(IsMouseButtonReleased((int)args[0].asInt()));
		});
		define("IsMouseButtonUp", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsMouseButtonUp(button)", l, c);
			return Value::Bool(IsMouseButtonUp((int)args[0].asInt()));
		});
		define("GetMouseX", [](const vector<Value> &args, int l, int c) {
			return Value::Int(GetMouseX());
		});
		define("GetMouseY", [](const vector<Value> &args, int l, int c) {
			return Value::Int(GetMouseY());
		});
		define("GetMousePosition", [=](const vector<Value> &args, int l, int c) {
			return Vector2ToValue(GetMousePosition());
		});
		define("GetMouseDelta", [=](const vector<Value> &args, int l, int c) {
			return Vector2ToValue(GetMouseDelta());
		});
		define("SetMousePosition", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetMousePosition(x, y)", l, c);
			SetMousePosition((int)args[0].asInt(), (int)args[1].asInt());
			return Value::None();
		});
		define("SetMouseOffset", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetMouseOffset(x, y)", l, c);
			SetMouseOffset((int)args[0].asInt(), (int)args[1].asInt());
			return Value::None();
		});
		define("SetMouseScale", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetMouseScale(x, y)", l, c);
			SetMouseScale((float)args[0].asFloat(), (float)args[1].asFloat());
			return Value::None();
		});
		define("GetMouseWheelMove", [](const vector<Value> &args, int l, int c) {
			return Value::Float(GetMouseWheelMove());
		});
		define("GetMouseWheelMoveV", [=](const vector<Value> &args, int l, int c) {
			return Vector2ToValue(GetMouseWheelMoveV());
		});
		define("SetMouseCursor", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("SetMouseCursor(cursor)", l, c);
			SetMouseCursor((int)args[0].asInt());
			return Value::None();
		});
		define("Image", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError(
					"Image(data_ptr, width, height, mipmaps, format)", l, c);
			auto inst = std::make_shared<InstanceObject>(imageClass.get());
			inst->fields["data"] = args[0];
			inst->fields["width"] = args[1];
			inst->fields["height"] = args[2];
			inst->fields["mipmaps"] = args[3];
			inst->fields["format"] = args[4];
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		});
		define("Texture2D", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("Texture2D(id, width, height, mipmaps, format)",
					l, c);
			auto inst = std::make_shared<InstanceObject>(textureClass.get());
			inst->fields["id"] = args[0];
			inst->fields["width"] = args[1];
			inst->fields["height"] = args[2];
			inst->fields["mipmaps"] = args[3];
			inst->fields["format"] = args[4];
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		});
		define("LoadImage", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("LoadImage(fileName)", l, c);
			Image img = LoadImage(args[0].asString().c_str());
			return ImageToValue(img);
		});
		define("LoadImageColors", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("LoadImageColors(img)", l, c);
			Image img = ValueToImage(args[0], l, c);
			Color *pixels = LoadImageColors(img);
			std::vector<Value> colorPix;
			colorPix.reserve(img.width * img.height);
			for (size_t i = 0; i < img.width * img.height; i++)
				colorPix.push_back(
					MakeColor(pixels[i].r, pixels[i].g, pixels[i].b, pixels[i].a));
			return Value::List(colorPix);
		});
		define("UpdateImagePixels", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("UpdateImagePixels(img, PixelList)", l, c);
			Image img = ValueToImage(args[0], l, c);
			auto *vec = static_cast<ListObject *>(args[1].ref.get());
			Color *pixels = LoadImageColors(img);
			size_t limit;
			if (vec->elements.size() != img.width * img.height)
				throw Warning(
					"list size does not match with the image's pixels. The result "
					"may "
					"not be correct, try resizing the image: ImageResizeNN(img, "
					"New_Width, New_Height)",
					l, c);
			limit = std::min((size_t)img.width * (size_t)img.height,
				vec->elements.size());
			for (size_t i = 0; i < limit; i++)
				pixels[i] = ValueToColor(vec->elements[i], l, c);
			return Value::None();
		});
		define("LoadImageFromScreen", [=](const vector<Value> &args, int l, int c) {
			if (!args.empty())
				throw ArgumentError("LoadImageFromScreen()", l, c);
			Image img = LoadImageFromScreen();
			return ImageToValue(img);
		});
		define("ImageResizeNN", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("ImageResizeNN(image, newWidth, newHeight)", l,
					c);
			Image img = ValueToImage(args[0], l, c);
			ImageResizeNN(&img, (int)args[1].asInt(), (int)args[2].asInt());
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			inst->fields["width"] = Value::Int(img.width);
			inst->fields["height"] = Value::Int(img.height);
			return Value::None();
		});
		define("ImageColorTint", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("ImageColorTint(image, color)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageColorTint(&img, ValueToColor(args[1], l, c));
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			return Value::None();
		});
		define("ImageDither", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("ImageDither(image, rBpp, gBpp, bBpp, aBpp)", l,
					c);
			Image img = ValueToImage(args[0], l, c);
			ImageDither(&img, (int)args[1].asInt(), (int)args[2].asInt(),
				(int)args[3].asInt(), (int)args[4].asInt());
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			inst->fields["format"] = Value::Int(img.format);
			inst->fields["width"] = Value::Int(img.width);
			inst->fields["height"] = Value::Int(img.height);
			return Value::None();
		});
		define("ImageApplyPalette", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("ImageApplyPalette(image, colorList)", l, c);
			Image img = ValueToImage(args[0], l, c);
			vector<Color> palette;
			auto list = static_cast<ListObject *>(args[1].ref.get());
			for (const auto &val : list->elements)
				palette.push_back(ValueToColor(val, l, c));
			Color *pixels = LoadImageColors(img);
			for (int i = 0; i < img.width * img.height; i++) {
				Color current = pixels[i];
				Color closest = palette[0];
				int minDistance = INT_MAX;
				for (Color p : palette) {
					int rDiff = current.r - p.r;
					int gDiff = current.g - p.g;
					int bDiff = current.b - p.b;
					double dist = (rDiff * rDiff * 0.30) + (gDiff * gDiff * 0.59) +
									  (bDiff * bDiff * 0.11);
					if (dist < minDistance) {
						minDistance = dist;
						closest = p;
					}
				}
				pixels[i] = closest;
			}
			for (int y = 0; y < img.height; y++)
				for (int x = 0; x < img.width; x++)
					ImageDrawPixel(&img, x, y, pixels[y * img.width + x]);
			UnloadImageColors(pixels);
			return Value::None();
		});
		define("ImageColorGrayscale", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ImageColorGrayscale(image)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageColorGrayscale(&img);
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			inst->fields["format"] = Value::Int(img.format);
			inst->fields["width"] = Value::Int(img.width);
			inst->fields["height"] = Value::Int(img.height);
			return Value::None();
		});
		define("ImageColorInvert", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ImageColorInvert(image)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageColorInvert(&img);
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			return Value::None();
		});
		define("ImageColorContrast", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError(
					"ImageColorContrast(image, contrastFloat)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageColorContrast(&img, (float)args[1].asFloat());
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			return Value::None();
		});
		define("ImageColorBrightness", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError(
					"ImageColorBrightness(image, brightnessInt)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageColorBrightness(&img, (int)args[1].asInt());
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			return Value::None();
		});
		define("ImageBlurGaussian", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("ImageBlurGaussian(image, blur)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageBlurGaussian(&img, args[1].asInt());
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			return Value::None();
		});
		define("ImageFlipVertical", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ImageFlipVertical(image)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageFlipVertical(&img);
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			return Value::None();
		});
		define("ImageFlipHorizontal", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ImageFlipHorizontal(image)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageFlipHorizontal(&img);
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			return Value::None();
		});
		define("ExportImage", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("ExportImage(image, fileName)", l, c);
			Image img = ValueToImage(args[0], l, c);
			bool success = ExportImage(img, args[1].asString().c_str());
			return Value::Bool(success);
		});
		define("UnloadImage", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("UnloadImage(image)", l, c);
			UnloadImage(ValueToImage(args[0], l, c));
			return Value::None();
		});
		define("LoadTexture", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("LoadTexture(fileName)", l, c);
			Texture2D tex = LoadTexture(args[0].asString().c_str());
			return TextureToValue(tex);
		});
		define("LoadTextureFromImage", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("LoadTextureFromImage(image)", l, c);
			Texture2D tex = LoadTextureFromImage(ValueToImage(args[0], l, c));
			return TextureToValue(tex);
		});
		define("UnloadTexture", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("UnloadTexture(texture)", l, c);
			UnloadTexture(ValueToTexture(args[0], l, c));
			return Value::None();
		});
		define("DrawTexture", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("DrawTexture(texture, posX, posY, tintColor)",
					l, c);
			DrawTexture(ValueToTexture(args[0], l, c), (int)args[1].asInt(),
				(int)args[2].asInt(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawTextureV", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("DrawTextureV(texture, positionVec, tintColor)",
					l, c);
			DrawTextureV(ValueToTexture(args[0], l, c),
				ValueToVector2(args[1], l, c),
				ValueToColor(args[2], l, c));
			return Value::None();
		});
		define("DrawTextureEx", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError(
					"DrawTextureEx(texture, positionVec, rotation, scale, "
					"tintColor)",
					l, c);
			DrawTextureEx(ValueToTexture(args[0], l, c),
				ValueToVector2(args[1], l, c), (float)args[2].asFloat(),
				(float)args[3].asFloat(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawTextureRec", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError(
					"DrawTextureRec(texture, sourceRec, positionVec, tintColor)", l,
					c);
			DrawTextureRec(
				ValueToTexture(args[0], l, c), ValueToRect(args[1], l, c),
				ValueToVector2(args[2], l, c), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawTexturePro", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6)
				throw ArgumentError("DrawTexturePro(texture, sourceRec, destRec, originVec, rotation, tintColor)", l, c);
			DrawTexturePro(
				ValueToTexture(args[0], l, c),
				ValueToRect(args[1], l, c),
				ValueToRect(args[2], l, c),
				ValueToVector2(args[3], l, c),
				static_cast<float>(args[4].asFloat()),
				ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("InitAudioDevice", [=](const vector<Value> &args, int l, int c) {
			InitAudioDevice();
			return Value::None();
		});
		define("CloseAudioDevice", [=](const vector<Value> &args, int l, int c) {
			CloseAudioDevice();
			return Value::None();
		});
		define("LoadSound", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("LoadSound(fileName)", l, c);
			Sound s = LoadSound(args[0].asString().c_str());
			return SoundToValue(s);
		});
		define("UnloadSound", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("UnloadSound(sound)", l, c);
			UnloadSound(ValueToSound(args[0], l, c));
			return Value::None();
		});
		define("LoadMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("LoadMusicStream(fileName)", l, c);
			Music m = LoadMusicStream(args[0].asString().c_str());
			return MusicToValue(m);
		});
		define("UnloadMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("UnloadMusicStream(music)", l, c);
			UnloadMusicStream(ValueToMusic(args[0], l, c));
			return Value::None();
		});
		define("SetMasterVolume", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("SetMasterVolume(float)", l, c);
			SetMasterVolume((float)args[0].asFloat());
			return Value::None();
		});
		define("PlaySound", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("PlaySound(sound)", l, c);
			PlaySound(ValueToSound(args[0], l, c));
			return Value::None();
		});
		define("StopSound", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("StopSound(sound)", l, c);
			StopSound(ValueToSound(args[0], l, c));
			return Value::None();
		});
		define("SetSoundPitch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetSoundPitch(sound, pitchFloat)", l, c);
			// 1.0 is normal, 0.5 is slow/deep, 2.0 is fast/high
			SetSoundPitch(ValueToSound(args[0], l, c), (float)args[1].asFloat());
			return Value::None();
		});
		define("SetSoundVolume", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetSoundVolume(sound, volumeFloat)", l, c);
			SetSoundVolume(ValueToSound(args[0], l, c), (float)args[1].asFloat());
			return Value::None();
		});
		define("SetSoundPan", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetSoundPan(sound, panFloat)", l, c);
			// 0.0 is left speaker, 0.5 is middle, 1.0 is right speaker
			SetSoundPan(ValueToSound(args[0], l, c), (float)args[1].asFloat());
			return Value::None();
		});
		define("PlayMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("PlayMusicStream(music)", l, c);
			PlayMusicStream(ValueToMusic(args[0], l, c));
			return Value::None();
		});
		define("UpdateMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("UpdateMusicStream(music)", l, c);
			UpdateMusicStream(ValueToMusic(args[0], l, c));
			return Value::None();
		});
		define("SetMusicPitch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetMusicPitch(music, pitchFloat)", l, c);
			SetMusicPitch(ValueToMusic(args[0], l, c), (float)args[1].asFloat());
			return Value::None();
		});
		define("SetMusicVolume", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetMusicVolume(music, volumeFloat)", l, c);
			SetMusicVolume(ValueToMusic(args[0], l, c), (float)args[1].asFloat());
			return Value::None();
		});


		// --- AUTO-GENERATED RAYLIB BINDINGS ---
		define("IsWindowReady", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsWindowReady expects 0 arguments", l, c);
			return Value::Bool(IsWindowReady());
		});
		define("IsWindowFullscreen", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsWindowFullscreen expects 0 arguments", l, c);
			return Value::Bool(IsWindowFullscreen());
		});
		define("IsWindowHidden", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsWindowHidden expects 0 arguments", l, c);
			return Value::Bool(IsWindowHidden());
		});
		define("IsWindowMinimized", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsWindowMinimized expects 0 arguments", l, c);
			return Value::Bool(IsWindowMinimized());
		});
		define("IsWindowMaximized", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsWindowMaximized expects 0 arguments", l, c);
			return Value::Bool(IsWindowMaximized());
		});
		define("IsWindowFocused", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsWindowFocused expects 0 arguments", l, c);
			return Value::Bool(IsWindowFocused());
		});
		define("IsWindowResized", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsWindowResized expects 0 arguments", l, c);
			return Value::Bool(IsWindowResized());
		});
		define("IsWindowState", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsWindowState expects 1 arguments", l, c);
			return Value::Bool(IsWindowState((unsigned int)args[0].asInt()));
		});
		define("SetWindowState", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetWindowState expects 1 arguments", l, c);
			SetWindowState((unsigned int)args[0].asInt());
			return Value::None();
		});
		define("ClearWindowState", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("ClearWindowState expects 1 arguments", l, c);
			ClearWindowState((unsigned int)args[0].asInt());
			return Value::None();
		});
		define("ToggleBorderlessWindowed", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("ToggleBorderlessWindowed expects 0 arguments", l, c);
			ToggleBorderlessWindowed();
			return Value::None();
		});
		define("MaximizeWindow", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("MaximizeWindow expects 0 arguments", l, c);
			MaximizeWindow();
			return Value::None();
		});
		define("MinimizeWindow", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("MinimizeWindow expects 0 arguments", l, c);
			MinimizeWindow();
			return Value::None();
		});
		define("RestoreWindow", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("RestoreWindow expects 0 arguments", l, c);
			RestoreWindow();
			return Value::None();
		});
		define("SetWindowIcon", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetWindowIcon expects 1 arguments", l, c);
			SetWindowIcon(ValueToImage(args[0], l, c));
			return Value::None();
		});
		define("SetWindowMonitor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetWindowMonitor expects 1 arguments", l, c);
			SetWindowMonitor((int)args[0].asInt());
			return Value::None();
		});
		define("SetWindowMinSize", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("SetWindowMinSize expects 2 arguments", l, c);
			SetWindowMinSize((int)args[0].asInt(), (int)args[1].asInt());
			return Value::None();
		});
		define("SetWindowMaxSize", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("SetWindowMaxSize expects 2 arguments", l, c);
			SetWindowMaxSize((int)args[0].asInt(), (int)args[1].asInt());
			return Value::None();
		});
		define("SetWindowSize", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("SetWindowSize expects 2 arguments", l, c);
			SetWindowSize((int)args[0].asInt(), (int)args[1].asInt());
			return Value::None();
		});
		define("SetWindowOpacity", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetWindowOpacity expects 1 arguments", l, c);
			SetWindowOpacity((float)args[0].asFloat());
			return Value::None();
		});
		define("SetWindowFocused", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("SetWindowFocused expects 0 arguments", l, c);
			SetWindowFocused();
			return Value::None();
		});
		define("GetScreenWidth", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetScreenWidth expects 0 arguments", l, c);
			return Value::Int(GetScreenWidth());
		});
		define("GetScreenHeight", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetScreenHeight expects 0 arguments", l, c);
			return Value::Int(GetScreenHeight());
		});
		define("GetRenderWidth", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetRenderWidth expects 0 arguments", l, c);
			return Value::Int(GetRenderWidth());
		});
		define("GetRenderHeight", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetRenderHeight expects 0 arguments", l, c);
			return Value::Int(GetRenderHeight());
		});
		define("GetMonitorCount", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetMonitorCount expects 0 arguments", l, c);
			return Value::Int(GetMonitorCount());
		});
		define("GetCurrentMonitor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetCurrentMonitor expects 0 arguments", l, c);
			return Value::Int(GetCurrentMonitor());
		});
		define("GetMonitorPosition", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetMonitorPosition expects 1 arguments", l, c);
			return Vector2ToValue(GetMonitorPosition((int)args[0].asInt()));
		});
		define("GetMonitorPhysicalWidth", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetMonitorPhysicalWidth expects 1 arguments", l, c);
			return Value::Int(GetMonitorPhysicalWidth((int)args[0].asInt()));
		});
		define("GetMonitorPhysicalHeight", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetMonitorPhysicalHeight expects 1 arguments", l, c);
			return Value::Int(GetMonitorPhysicalHeight((int)args[0].asInt()));
		});
		define("GetMonitorRefreshRate", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetMonitorRefreshRate expects 1 arguments", l, c);
			return Value::Int(GetMonitorRefreshRate((int)args[0].asInt()));
		});
		define("GetWindowPosition", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetWindowPosition expects 0 arguments", l, c);
			return Vector2ToValue(GetWindowPosition());
		});
		define("GetWindowScaleDPI", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetWindowScaleDPI expects 0 arguments", l, c);
			return Vector2ToValue(GetWindowScaleDPI());
		});
		define("GetClipboardImage", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetClipboardImage expects 0 arguments", l, c);
			return ImageToValue(GetClipboardImage());
		});
		define("EnableEventWaiting", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("EnableEventWaiting expects 0 arguments", l, c);
			EnableEventWaiting();
			return Value::None();
		});
		define("DisableEventWaiting", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("DisableEventWaiting expects 0 arguments", l, c);
			DisableEventWaiting();
			return Value::None();
		});
		define("ShowCursor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("ShowCursor expects 0 arguments", l, c);
			ShowCursor();
			return Value::None();
		});
		define("HideCursor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("HideCursor expects 0 arguments", l, c);
			HideCursor();
			return Value::None();
		});
		define("IsCursorHidden", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsCursorHidden expects 0 arguments", l, c);
			return Value::Bool(IsCursorHidden());
		});
		define("EnableCursor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("EnableCursor expects 0 arguments", l, c);
			EnableCursor();
			return Value::None();
		});
		define("DisableCursor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("DisableCursor expects 0 arguments", l, c);
			DisableCursor();
			return Value::None();
		});
		define("IsCursorOnScreen", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsCursorOnScreen expects 0 arguments", l, c);
			return Value::Bool(IsCursorOnScreen());
		});
		define("EndMode2D", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("EndMode2D expects 0 arguments", l, c);
			EndMode2D();
			return Value::None();
		});
		define("BeginScissorMode", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("BeginScissorMode expects 4 arguments", l, c);
			BeginScissorMode((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt());
			return Value::None();
		});
		define("EndScissorMode", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("EndScissorMode expects 0 arguments", l, c);
			EndScissorMode();
			return Value::None();
		});
		define("EndVrStereoMode", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("EndVrStereoMode expects 0 arguments", l, c);
			EndVrStereoMode();
			return Value::None();
		});
		define("GetTime", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetTime expects 0 arguments", l, c);
			return Value::Float(GetTime());
		});
		define("GetFPS", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetFPS expects 0 arguments", l, c);
			return Value::Int(GetFPS());
		});
		define("SwapScreenBuffer", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("SwapScreenBuffer expects 0 arguments", l, c);
			SwapScreenBuffer();
			return Value::None();
		});
		define("PollInputEvents", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("PollInputEvents expects 0 arguments", l, c);
			PollInputEvents();
			return Value::None();
		});
		define("WaitTime", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("WaitTime expects 1 arguments", l, c);
			WaitTime((double)args[0].asFloat());
			return Value::None();
		});
		define("SetRandomSeed", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetRandomSeed expects 1 arguments", l, c);
			SetRandomSeed((unsigned int)args[0].asInt());
			return Value::None();
		});
		define("GetRandomValue", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("GetRandomValue expects 2 arguments", l, c);
			return Value::Int(GetRandomValue((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("SetTraceLogLevel", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetTraceLogLevel expects 1 arguments", l, c);
			SetTraceLogLevel((int)args[0].asInt());
			return Value::None();
		});
		define("IsFileDropped", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsFileDropped expects 0 arguments", l, c);
			return Value::Bool(IsFileDropped());
		});
		define("SetAutomationEventBaseFrame", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetAutomationEventBaseFrame expects 1 arguments", l, c);
			SetAutomationEventBaseFrame((int)args[0].asInt());
			return Value::None();
		});
		define("StartAutomationEventRecording", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("StartAutomationEventRecording expects 0 arguments", l, c);
			StartAutomationEventRecording();
			return Value::None();
		});
		define("StopAutomationEventRecording", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("StopAutomationEventRecording expects 0 arguments", l, c);
			StopAutomationEventRecording();
			return Value::None();
		});
		define("IsGamepadAvailable", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsGamepadAvailable expects 1 arguments", l, c);
			return Value::Bool(IsGamepadAvailable((int)args[0].asInt()));
		});
		define("IsGamepadButtonPressed", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("IsGamepadButtonPressed expects 2 arguments", l, c);
			return Value::Bool(IsGamepadButtonPressed((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("IsGamepadButtonDown", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("IsGamepadButtonDown expects 2 arguments", l, c);
			return Value::Bool(IsGamepadButtonDown((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("IsGamepadButtonReleased", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("IsGamepadButtonReleased expects 2 arguments", l, c);
			return Value::Bool(IsGamepadButtonReleased((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("IsGamepadButtonUp", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("IsGamepadButtonUp expects 2 arguments", l, c);
			return Value::Bool(IsGamepadButtonUp((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("GetGamepadButtonPressed", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetGamepadButtonPressed expects 0 arguments", l, c);
			return Value::Int(GetGamepadButtonPressed());
		});
		define("GetGamepadAxisCount", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetGamepadAxisCount expects 1 arguments", l, c);
			return Value::Int(GetGamepadAxisCount((int)args[0].asInt()));
		});
		define("GetGamepadAxisMovement", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("GetGamepadAxisMovement expects 2 arguments", l, c);
			return Value::Float(GetGamepadAxisMovement((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("SetGamepadVibration", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("SetGamepadVibration expects 4 arguments", l, c);
			SetGamepadVibration((int)args[0].asInt(), (float)args[1].asFloat(), (float)args[2].asFloat(), (float)args[3].asFloat());
			return Value::None();
		});
		define("GetTouchX", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetTouchX expects 0 arguments", l, c);
			return Value::Int(GetTouchX());
		});
		define("GetTouchY", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetTouchY expects 0 arguments", l, c);
			return Value::Int(GetTouchY());
		});
		define("GetTouchPosition", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetTouchPosition expects 1 arguments", l, c);
			return Vector2ToValue(GetTouchPosition((int)args[0].asInt()));
		});
		define("GetTouchPointId", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetTouchPointId expects 1 arguments", l, c);
			return Value::Int(GetTouchPointId((int)args[0].asInt()));
		});
		define("GetTouchPointCount", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetTouchPointCount expects 0 arguments", l, c);
			return Value::Int(GetTouchPointCount());
		});
		define("SetGesturesEnabled", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetGesturesEnabled expects 1 arguments", l, c);
			SetGesturesEnabled((unsigned int)args[0].asInt());
			return Value::None();
		});
		define("IsGestureDetected", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsGestureDetected expects 1 arguments", l, c);
			return Value::Bool(IsGestureDetected((unsigned int)args[0].asInt()));
		});
		define("GetGestureDetected", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetGestureDetected expects 0 arguments", l, c);
			return Value::Int(GetGestureDetected());
		});
		define("GetGestureHoldDuration", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetGestureHoldDuration expects 0 arguments", l, c);
			return Value::Float(GetGestureHoldDuration());
		});
		define("GetGestureDragVector", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetGestureDragVector expects 0 arguments", l, c);
			return Vector2ToValue(GetGestureDragVector());
		});
		define("GetGestureDragAngle", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetGestureDragAngle expects 0 arguments", l, c);
			return Value::Float(GetGestureDragAngle());
		});
		define("GetGesturePinchVector", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetGesturePinchVector expects 0 arguments", l, c);
			return Vector2ToValue(GetGesturePinchVector());
		});
		define("GetGesturePinchAngle", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetGesturePinchAngle expects 0 arguments", l, c);
			return Value::Float(GetGesturePinchAngle());
		});
		define("SetShapesTexture", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("SetShapesTexture expects 2 arguments", l, c);
			SetShapesTexture(ValueToTexture(args[0], l, c), ValueToRect(args[1], l, c));
			return Value::None();
		});
		define("GetShapesTexture", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetShapesTexture expects 0 arguments", l, c);
			return TextureToValue(GetShapesTexture());
		});
		define("DrawLineDashed", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawLineDashed expects 5 arguments", l, c);
			DrawLineDashed(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), (int)args[2].asInt(), (int)args[3].asInt(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawCircleSectorLines", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6) throw ArgumentError("DrawCircleSectorLines expects 6 arguments", l, c);
			DrawCircleSectorLines(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), (float)args[2].asFloat(), (float)args[3].asFloat(), (int)args[4].asInt(), ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("DrawCircleLinesV", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("DrawCircleLinesV expects 3 arguments", l, c);
			DrawCircleLinesV(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), ValueToColor(args[2], l, c));
			return Value::None();
		});
		define("DrawEllipseV", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("DrawEllipseV expects 4 arguments", l, c);
			DrawEllipseV(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), (float)args[2].asFloat(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawEllipseLines", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawEllipseLines expects 5 arguments", l, c);
			DrawEllipseLines((int)args[0].asInt(), (int)args[1].asInt(), (float)args[2].asFloat(), (float)args[3].asFloat(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawEllipseLinesV", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("DrawEllipseLinesV expects 4 arguments", l, c);
			DrawEllipseLinesV(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), (float)args[2].asFloat(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawRingLines", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 7) throw ArgumentError("DrawRingLines expects 7 arguments", l, c);
			DrawRingLines(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), (float)args[2].asFloat(), (float)args[3].asFloat(), (float)args[4].asFloat(), (int)args[5].asInt(), ValueToColor(args[6], l, c));
			return Value::None();
		});
		define("DrawRectangleGradientH", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6) throw ArgumentError("DrawRectangleGradientH expects 6 arguments", l, c);
			DrawRectangleGradientH((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt(), ValueToColor(args[4], l, c), ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("DrawRectangleGradientEx", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawRectangleGradientEx expects 5 arguments", l, c);
			DrawRectangleGradientEx(ValueToRect(args[0], l, c), ValueToColor(args[1], l, c), ValueToColor(args[2], l, c), ValueToColor(args[3], l, c), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawRectangleLines", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawRectangleLines expects 5 arguments", l, c);
			DrawRectangleLines((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawRectangleLinesEx", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("DrawRectangleLinesEx expects 3 arguments", l, c);
			DrawRectangleLinesEx(ValueToRect(args[0], l, c), (float)args[1].asFloat(), ValueToColor(args[2], l, c));
			return Value::None();
		});
		define("DrawRectangleRoundedLines", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("DrawRectangleRoundedLines expects 4 arguments", l, c);
			DrawRectangleRoundedLines(ValueToRect(args[0], l, c), (float)args[1].asFloat(), (int)args[2].asInt(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawRectangleRoundedLinesEx", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawRectangleRoundedLinesEx expects 5 arguments", l, c);
			DrawRectangleRoundedLinesEx(ValueToRect(args[0], l, c), (float)args[1].asFloat(), (int)args[2].asInt(), (float)args[3].asFloat(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawTriangleLines", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("DrawTriangleLines expects 4 arguments", l, c);
			DrawTriangleLines(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawPolyLines", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawPolyLines expects 5 arguments", l, c);
			DrawPolyLines(ValueToVector2(args[0], l, c), (int)args[1].asInt(), (float)args[2].asFloat(), (float)args[3].asFloat(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawSplineSegmentLinear", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("DrawSplineSegmentLinear expects 4 arguments", l, c);
			DrawSplineSegmentLinear(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), (float)args[2].asFloat(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawSplineSegmentBasis", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6) throw ArgumentError("DrawSplineSegmentBasis expects 6 arguments", l, c);
			DrawSplineSegmentBasis(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c), (float)args[4].asFloat(), ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("DrawSplineSegmentCatmullRom", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6) throw ArgumentError("DrawSplineSegmentCatmullRom expects 6 arguments", l, c);
			DrawSplineSegmentCatmullRom(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c), (float)args[4].asFloat(), ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("DrawSplineSegmentBezierQuadratic", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawSplineSegmentBezierQuadratic expects 5 arguments", l, c);
			DrawSplineSegmentBezierQuadratic(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), (float)args[3].asFloat(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawSplineSegmentBezierCubic", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6) throw ArgumentError("DrawSplineSegmentBezierCubic expects 6 arguments", l, c);
			DrawSplineSegmentBezierCubic(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c), (float)args[4].asFloat(), ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("GetSplinePointLinear", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("GetSplinePointLinear expects 3 arguments", l, c);
			return Vector2ToValue(GetSplinePointLinear(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), (float)args[2].asFloat()));
		});
		define("GetSplinePointBasis", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("GetSplinePointBasis expects 5 arguments", l, c);
			return Vector2ToValue(GetSplinePointBasis(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c), (float)args[4].asFloat()));
		});
		define("GetSplinePointCatmullRom", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("GetSplinePointCatmullRom expects 5 arguments", l, c);
			return Vector2ToValue(GetSplinePointCatmullRom(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c), (float)args[4].asFloat()));
		});
		define("GetSplinePointBezierQuad", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("GetSplinePointBezierQuad expects 4 arguments", l, c);
			return Vector2ToValue(GetSplinePointBezierQuad(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), (float)args[3].asFloat()));
		});
		define("GetSplinePointBezierCubic", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("GetSplinePointBezierCubic expects 5 arguments", l, c);
			return Vector2ToValue(GetSplinePointBezierCubic(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c), (float)args[4].asFloat()));
		});
		define("CheckCollisionRecs", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("CheckCollisionRecs expects 2 arguments", l, c);
			return Value::Bool(CheckCollisionRecs(ValueToRect(args[0], l, c), ValueToRect(args[1], l, c)));
		});
		define("CheckCollisionCircles", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("CheckCollisionCircles expects 4 arguments", l, c);
			return Value::Bool(CheckCollisionCircles(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), ValueToVector2(args[2], l, c), (float)args[3].asFloat()));
		});
		define("CheckCollisionCircleRec", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("CheckCollisionCircleRec expects 3 arguments", l, c);
			return Value::Bool(CheckCollisionCircleRec(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), ValueToRect(args[2], l, c)));
		});
		define("CheckCollisionCircleLine", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("CheckCollisionCircleLine expects 4 arguments", l, c);
			return Value::Bool(CheckCollisionCircleLine(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c)));
		});
		define("CheckCollisionPointRec", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("CheckCollisionPointRec expects 2 arguments", l, c);
			return Value::Bool(CheckCollisionPointRec(ValueToVector2(args[0], l, c), ValueToRect(args[1], l, c)));
		});
		define("CheckCollisionPointCircle", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("CheckCollisionPointCircle expects 3 arguments", l, c);
			return Value::Bool(CheckCollisionPointCircle(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), (float)args[2].asFloat()));
		});
		define("CheckCollisionPointTriangle", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("CheckCollisionPointTriangle expects 4 arguments", l, c);
			return Value::Bool(CheckCollisionPointTriangle(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c)));
		});
		define("CheckCollisionPointLine", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("CheckCollisionPointLine expects 4 arguments", l, c);
			return Value::Bool(CheckCollisionPointLine(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), (int)args[3].asInt()));
		});
		define("LoadImageFromTexture", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("LoadImageFromTexture expects 1 arguments", l, c);
			return ImageToValue(LoadImageFromTexture(ValueToTexture(args[0], l, c)));
		});
		define("IsImageValid", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsImageValid expects 1 arguments", l, c);
			return Value::Bool(IsImageValid(ValueToImage(args[0], l, c)));
		});
		define("GenImageColor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("GenImageColor expects 3 arguments", l, c);
			return ImageToValue(GenImageColor((int)args[0].asInt(), (int)args[1].asInt(), ValueToColor(args[2], l, c)));
		});
		define("GenImageGradientLinear", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("GenImageGradientLinear expects 5 arguments", l, c);
			return ImageToValue(GenImageGradientLinear((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), ValueToColor(args[3], l, c), ValueToColor(args[4], l, c)));
		});
		define("GenImageGradientRadial", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("GenImageGradientRadial expects 5 arguments", l, c);
			return ImageToValue(GenImageGradientRadial((int)args[0].asInt(), (int)args[1].asInt(), (float)args[2].asFloat(), ValueToColor(args[3], l, c), ValueToColor(args[4], l, c)));
		});
		define("GenImageGradientSquare", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("GenImageGradientSquare expects 5 arguments", l, c);
			return ImageToValue(GenImageGradientSquare((int)args[0].asInt(), (int)args[1].asInt(), (float)args[2].asFloat(), ValueToColor(args[3], l, c), ValueToColor(args[4], l, c)));
		});
		define("GenImageChecked", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6) throw ArgumentError("GenImageChecked expects 6 arguments", l, c);
			return ImageToValue(GenImageChecked((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt(), ValueToColor(args[4], l, c), ValueToColor(args[5], l, c)));
		});
		define("GenImageWhiteNoise", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("GenImageWhiteNoise expects 3 arguments", l, c);
			return ImageToValue(GenImageWhiteNoise((int)args[0].asInt(), (int)args[1].asInt(), (float)args[2].asFloat()));
		});
		define("GenImagePerlinNoise", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("GenImagePerlinNoise expects 5 arguments", l, c);
			return ImageToValue(GenImagePerlinNoise((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt(), (float)args[4].asFloat()));
		});
		define("GenImageCellular", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("GenImageCellular expects 3 arguments", l, c);
			return ImageToValue(GenImageCellular((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt()));
		});
		define("ImageCopy", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("ImageCopy expects 1 arguments", l, c);
			return ImageToValue(ImageCopy(ValueToImage(args[0], l, c)));
		});
		define("ImageFromImage", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("ImageFromImage expects 2 arguments", l, c);
			return ImageToValue(ImageFromImage(ValueToImage(args[0], l, c), ValueToRect(args[1], l, c)));
		});
		define("ImageFromChannel", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("ImageFromChannel expects 2 arguments", l, c);
			return ImageToValue(ImageFromChannel(ValueToImage(args[0], l, c), (int)args[1].asInt()));
		});
		define("IsTextureValid", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsTextureValid expects 1 arguments", l, c);
			return Value::Bool(IsTextureValid(ValueToTexture(args[0], l, c)));
		});
		define("IsRenderTextureValid", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsRenderTextureValid expects 1 arguments", l, c);
			return Value::Bool(IsRenderTextureValid(ValueToRenderTexture(args[0], l, c)));
		});
		define("SetTextureWrap", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("SetTextureWrap expects 2 arguments", l, c);
			SetTextureWrap(ValueToTexture(args[0], l, c), (int)args[1].asInt());
			return Value::None();
		});
		define("ColorIsEqual", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("ColorIsEqual expects 2 arguments", l, c);
			return Value::Bool(ColorIsEqual(ValueToColor(args[0], l, c), ValueToColor(args[1], l, c)));
		});
		define("ColorToInt", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("ColorToInt expects 1 arguments", l, c);
			return Value::Int(ColorToInt(ValueToColor(args[0], l, c)));
		});
		define("GetPixelDataSize", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("GetPixelDataSize expects 3 arguments", l, c);
			return Value::Int(GetPixelDataSize((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt()));
		});
		define("LoadFontFromImage", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("LoadFontFromImage expects 3 arguments", l, c);
			return FontToValue(LoadFontFromImage(ValueToImage(args[0], l, c), ValueToColor(args[1], l, c), (int)args[2].asInt()));
		});
		define("IsFontValid", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsFontValid expects 1 arguments", l, c);
			return Value::Bool(IsFontValid(ValueToFont(args[0], l, c)));
		});
		define("DrawTextCodepoint", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawTextCodepoint expects 5 arguments", l, c);
			DrawTextCodepoint(ValueToFont(args[0], l, c), (int)args[1].asInt(), ValueToVector2(args[2], l, c), (float)args[3].asFloat(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("SetTextLineSpacing", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetTextLineSpacing expects 1 arguments", l, c);
			SetTextLineSpacing((int)args[0].asInt());
			return Value::None();
		});
		define("GetGlyphIndex", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("GetGlyphIndex expects 2 arguments", l, c);
			return Value::Int(GetGlyphIndex(ValueToFont(args[0], l, c), (int)args[1].asInt()));
		});
		define("DrawGrid", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("DrawGrid expects 2 arguments", l, c);
			DrawGrid((int)args[0].asInt(), (float)args[1].asFloat());
			return Value::None();
		});
		define("IsAudioDeviceReady", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsAudioDeviceReady expects 0 arguments", l, c);
			return Value::Bool(IsAudioDeviceReady());
		});
		define("GetMasterVolume", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetMasterVolume expects 0 arguments", l, c);
			return Value::Float(GetMasterVolume());
		});
		define("LoadSoundAlias", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("LoadSoundAlias expects 1 arguments", l, c);
			return SoundToValue(LoadSoundAlias(ValueToSound(args[0], l, c)));
		});
		define("IsSoundValid", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsSoundValid expects 1 arguments", l, c);
			return Value::Bool(IsSoundValid(ValueToSound(args[0], l, c)));
		});
		define("UnloadSoundAlias", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("UnloadSoundAlias expects 1 arguments", l, c);
			UnloadSoundAlias(ValueToSound(args[0], l, c));
			return Value::None();
		});
		define("PauseSound", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("PauseSound expects 1 arguments", l, c);
			PauseSound(ValueToSound(args[0], l, c));
			return Value::None();
		});
		define("ResumeSound", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("ResumeSound expects 1 arguments", l, c);
			ResumeSound(ValueToSound(args[0], l, c));
			return Value::None();
		});
		define("IsSoundPlaying", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsSoundPlaying expects 1 arguments", l, c);
			return Value::Bool(IsSoundPlaying(ValueToSound(args[0], l, c)));
		});
		define("IsMusicValid", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsMusicValid expects 1 arguments", l, c);
			return Value::Bool(IsMusicValid(ValueToMusic(args[0], l, c)));
		});
		define("IsMusicStreamPlaying", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsMusicStreamPlaying expects 1 arguments", l, c);
			return Value::Bool(IsMusicStreamPlaying(ValueToMusic(args[0], l, c)));
		});
		define("StopMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("StopMusicStream expects 1 arguments", l, c);
			StopMusicStream(ValueToMusic(args[0], l, c));
			return Value::None();
		});
		define("PauseMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("PauseMusicStream expects 1 arguments", l, c);
			PauseMusicStream(ValueToMusic(args[0], l, c));
			return Value::None();
		});
		define("ResumeMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("ResumeMusicStream expects 1 arguments", l, c);
			ResumeMusicStream(ValueToMusic(args[0], l, c));
			return Value::None();
		});
		define("SeekMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("SeekMusicStream expects 2 arguments", l, c);
			SeekMusicStream(ValueToMusic(args[0], l, c), (float)args[1].asFloat());
			return Value::None();
		});
		define("SetMusicPan", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("SetMusicPan expects 2 arguments", l, c);
			SetMusicPan(ValueToMusic(args[0], l, c), (float)args[1].asFloat());
			return Value::None();
		});
		define("GetMusicTimeLength", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetMusicTimeLength expects 1 arguments", l, c);
			return Value::Float(GetMusicTimeLength(ValueToMusic(args[0], l, c)));
		});
		define("GetMusicTimePlayed", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetMusicTimePlayed expects 1 arguments", l, c);
			return Value::Float(GetMusicTimePlayed(ValueToMusic(args[0], l, c)));
		});
		define("SetAudioStreamBufferSizeDefault", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetAudioStreamBufferSizeDefault expects 1 arguments", l, c);
			SetAudioStreamBufferSizeDefault((int)args[0].asInt());
			return Value::None();
		});

		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Raylib", modVal, false, false);
	};
#ifndef _WIN32
	modules["Ncurses"] = [](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Ncurses");
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
		env->set("COLOR_BLACK", Value::Int(COLOR_BLACK), true, true);
		env->set("COLOR_RED", Value::Int(COLOR_RED), true, true);
		env->set("COLOR_GREEN", Value::Int(COLOR_GREEN), true, true);
		env->set("COLOR_YELLOW", Value::Int(COLOR_YELLOW), true, true);
		env->set("COLOR_BLUE", Value::Int(COLOR_BLUE), true, true);
		env->set("COLOR_MAGENTA", Value::Int(COLOR_MAGENTA), true, true);
		env->set("COLOR_CYAN", Value::Int(COLOR_CYAN), true, true);
		env->set("COLOR_WHITE", Value::Int(COLOR_WHITE), true, true);
		env->set("A_NORMAL", Value::Int(A_NORMAL), true, true);
		env->set("A_BOLD", Value::Int(A_BOLD), true, true);
		env->set("A_UNDERLINE", Value::Int(A_UNDERLINE), true, true);
		env->set("A_REVERSE", Value::Int(A_REVERSE), true, true);
		env->set("A_BLINK", Value::Int(A_BLINK), true, true);

		define("initscr", [](const vector<Value> &args, int l, int c) {
			setlocale(LC_ALL, "");
			WINDOW* win = initscr();
			cbreak();
			noecho();
			keypad(stdscr, true);
			return Value::Int((long long)win);
		});
		define("endwin", [](const vector<Value> &args, int l, int c) {
			endwin();
			return Value::None();
		});
		define("printw", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("printw(string)", l, c);
			printw("%s", args[0].asString().c_str());
			return Value::None();
		});
		define("getmaxy", [](const vector<Value> &args, int l, int c) {
			return Value::Int(getmaxy(stdscr));
		});
		define("getmaxx", [](const vector<Value> &args, int l, int c) {
			return Value::Int(getmaxx(stdscr));
		});
		define("getyx", [](const vector<Value> &args, int l, int c) {
			int y, x;
			getyx(stdscr, y, x);
			return Value::List({Value::Int(y), Value::Int(x)});
		});
		define("mvprintw", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("mvprintw(y, x, string)", l, c);
			mvprintw((int)args[0].asInt(), (int)args[1].asInt(), "%s", args[2].asString().c_str());
			return Value::None();
		});
		define("mvwprintw", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("mvwprintw(win, y, x, string)", l, c);
			mvwprintw((WINDOW*)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), "%s", args[3].asString().c_str());
			return Value::None();
		});
		define("wprintw", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("wprintw(win, string)", l, c);
			wprintw((WINDOW*)(long long)args[0].asInt(), "%s", args[1].asString().c_str());
			return Value::None();
		});

		// --- AUTO-GENERATED NCURSES BINDINGS ---
		define("addnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("addnstr expects 2 arguments", l, c);
			return Value::Int(addnstr(args[0].asString().c_str(), (int)args[1].asInt()));
		});
		define("attroff", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("attroff expects 1 arguments", l, c);
			return Value::Int(attroff((NCURSES_ATTR_T)args[0].asInt()));
		});
		define("attron", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("attron expects 1 arguments", l, c);
			return Value::Int(attron((NCURSES_ATTR_T)args[0].asInt()));
		});
		define("attrset", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("attrset expects 1 arguments", l, c);
			return Value::Int(attrset((NCURSES_ATTR_T)args[0].asInt()));
		});
		define("baudrate", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("baudrate expects 0 arguments", l, c);
			return Value::Int(baudrate());
		});
		define("beep", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("beep expects 0 arguments", l, c);
			return Value::Int(beep());
		});
		define("bkgd", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("bkgd expects 1 arguments", l, c);
			return Value::Int(bkgd((chtype)args[0].asInt()));
		});
		define("bkgdset", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("bkgdset expects 1 arguments", l, c);
			bkgdset((chtype)args[0].asInt());
			return Value::None();
		});
		define("border", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 8) throw ArgumentError("border expects 8 arguments", l, c);
			return Value::Int(border((chtype)args[0].asInt(), (chtype)args[1].asInt(), (chtype)args[2].asInt(), (chtype)args[3].asInt(), (chtype)args[4].asInt(), (chtype)args[5].asInt(), (chtype)args[6].asInt(), (chtype)args[7].asInt()));
		});
		define("box", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("box expects 3 arguments", l, c);
			return Value::Int(box((WINDOW *)(long long)args[0].asInt(), (chtype)args[1].asInt(), (chtype)args[2].asInt()));
		});
		define("can_change_color", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("can_change_color expects 0 arguments", l, c);
			return Value::Bool(can_change_color());
		});
		define("cbreak", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("cbreak expects 0 arguments", l, c);
			return Value::Int(cbreak());
		});
		define("clear", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("clear expects 0 arguments", l, c);
			return Value::Int(clear());
		});
		define("clearok", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("clearok expects 2 arguments", l, c);
			return Value::Int(clearok((WINDOW *)(long long)args[0].asInt(), args[1].asBool()));
		});
		define("clrtobot", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("clrtobot expects 0 arguments", l, c);
			return Value::Int(clrtobot());
		});
		define("clrtoeol", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("clrtoeol expects 0 arguments", l, c);
			return Value::Int(clrtoeol());
		});
		define("COLOR_PAIR", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("COLOR_PAIR expects 1 arguments", l, c);
			return Value::Int(COLOR_PAIR((int)args[0].asInt()));
		});
		define("curs_set", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("curs_set expects 1 arguments", l, c);
			return Value::Int(curs_set((int)args[0].asInt()));
		});
		define("def_prog_mode", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("def_prog_mode expects 0 arguments", l, c);
			return Value::Int(def_prog_mode());
		});
		define("def_shell_mode", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("def_shell_mode expects 0 arguments", l, c);
			return Value::Int(def_shell_mode());
		});
		define("delay_output", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("delay_output expects 1 arguments", l, c);
			return Value::Int(delay_output((int)args[0].asInt()));
		});
		define("delch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("delch expects 0 arguments", l, c);
			return Value::Int(delch());
		});
		define("delwin", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("delwin expects 1 arguments", l, c);
			return Value::Int(delwin((WINDOW *)(long long)args[0].asInt()));
		});
		define("deleteln", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("deleteln expects 0 arguments", l, c);
			return Value::Int(deleteln());
		});
		define("derwin", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("derwin expects 5 arguments", l, c);
			return Value::Int((long long)derwin((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt(), (int)args[4].asInt()));
		});
		define("doupdate", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("doupdate expects 0 arguments", l, c);
			return Value::Int(doupdate());
		});
		define("dupwin", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("dupwin expects 1 arguments", l, c);
			return Value::Int((long long)dupwin((WINDOW *)(long long)args[0].asInt()));
		});
		define("echo", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("echo expects 0 arguments", l, c);
			return Value::Int(echo());
		});
		define("erase", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("erase expects 0 arguments", l, c);
			return Value::Int(erase());
		});
		define("endwin", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("endwin expects 0 arguments", l, c);
			return Value::Int(endwin());
		});
		define("filter", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("filter expects 0 arguments", l, c);
			filter();
			return Value::None();
		});
		define("flash", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("flash expects 0 arguments", l, c);
			return Value::Int(flash());
		});
		define("flushinp", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("flushinp expects 0 arguments", l, c);
			return Value::Int(flushinp());
		});
		define("getbkgd", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("getbkgd expects 1 arguments", l, c);
			return Value::Int(getbkgd((WINDOW *)(long long)args[0].asInt()));
		});
		define("getch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("getch expects 0 arguments", l, c);
			return Value::Int(getch());
		});
		define("getnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("getnstr expects 2 arguments", l, c);
			return Value::Int(getnstr((char *)args[0].asString().c_str(), (int)args[1].asInt()));
		});
		define("getstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("getstr expects 1 arguments", l, c);
			return Value::Int(getstr((char *)args[0].asString().c_str()));
		});
		define("halfdelay", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("halfdelay expects 1 arguments", l, c);
			return Value::Int(halfdelay((int)args[0].asInt()));
		});
		define("has_colors", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("has_colors expects 0 arguments", l, c);
			return Value::Bool(has_colors());
		});
		define("has_ic", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("has_ic expects 0 arguments", l, c);
			return Value::Bool(has_ic());
		});
		define("has_il", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("has_il expects 0 arguments", l, c);
			return Value::Bool(has_il());
		});
		define("hline", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("hline expects 2 arguments", l, c);
			return Value::Int(hline((chtype)args[0].asInt(), (int)args[1].asInt()));
		});
		define("idcok", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("idcok expects 2 arguments", l, c);
			idcok((WINDOW *)(long long)args[0].asInt(), args[1].asBool());
			return Value::None();
		});
		define("idlok", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("idlok expects 2 arguments", l, c);
			return Value::Int(idlok((WINDOW *)(long long)args[0].asInt(), args[1].asBool()));
		});
		define("immedok", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("immedok expects 2 arguments", l, c);
			immedok((WINDOW *)(long long)args[0].asInt(), args[1].asBool());
			return Value::None();
		});
		define("inch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("inch expects 0 arguments", l, c);
			return Value::Int(inch());
		});
		define("initscr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("initscr expects 0 arguments", l, c);
			return Value::Int((long long)initscr());
		});
		define("init_color", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("init_color expects 4 arguments", l, c);
			return Value::Int(init_color((NCURSES_COLOR_T)args[0].asInt(), (NCURSES_COLOR_T)args[1].asInt(), (NCURSES_COLOR_T)args[2].asInt(), (NCURSES_COLOR_T)args[3].asInt()));
		});
		define("init_pair", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("init_pair expects 3 arguments", l, c);
			return Value::Int(init_pair((NCURSES_PAIRS_T)args[0].asInt(), (NCURSES_COLOR_T)args[1].asInt(), (NCURSES_COLOR_T)args[2].asInt()));
		});
		define("innstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("innstr expects 2 arguments", l, c);
			return Value::Int(innstr((char *)args[0].asString().c_str(), (int)args[1].asInt()));
		});
		define("insch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("insch expects 1 arguments", l, c);
			return Value::Int(insch((chtype)args[0].asInt()));
		});
		define("insdelln", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("insdelln expects 1 arguments", l, c);
			return Value::Int(insdelln((int)args[0].asInt()));
		});
		define("insertln", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("insertln expects 0 arguments", l, c);
			return Value::Int(insertln());
		});
		define("insnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("insnstr expects 2 arguments", l, c);
			return Value::Int(insnstr(args[0].asString().c_str(), (int)args[1].asInt()));
		});
		define("insstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("insstr expects 1 arguments", l, c);
			return Value::Int(insstr(args[0].asString().c_str()));
		});
		define("instr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("instr expects 1 arguments", l, c);
			return Value::Int(instr((char *)args[0].asString().c_str()));
		});
		define("intrflush", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("intrflush expects 2 arguments", l, c);
			return Value::Int(intrflush((WINDOW *)(long long)args[0].asInt(), args[1].asBool()));
		});
		define("isendwin", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("isendwin expects 0 arguments", l, c);
			return Value::Bool(isendwin());
		});
		define("is_linetouched", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("is_linetouched expects 2 arguments", l, c);
			return Value::Bool(is_linetouched((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt()));
		});
		define("is_wintouched", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("is_wintouched expects 1 arguments", l, c);
			return Value::Bool(is_wintouched((WINDOW *)(long long)args[0].asInt()));
		});
		define("keypad", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("keypad expects 2 arguments", l, c);
			return Value::Int(keypad((WINDOW *)(long long)args[0].asInt(), args[1].asBool()));
		});
		define("leaveok", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("leaveok expects 2 arguments", l, c);
			return Value::Int(leaveok((WINDOW *)(long long)args[0].asInt(), args[1].asBool()));
		});
		define("longname", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("longname expects 0 arguments", l, c);
			return Value::String(longname());
		});
		define("meta", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("meta expects 2 arguments", l, c);
			return Value::Int(meta((WINDOW *)(long long)args[0].asInt(), args[1].asBool()));
		});
		define("move", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("move expects 2 arguments", l, c);
			return Value::Int(move((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("mvaddnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("mvaddnstr expects 4 arguments", l, c);
			return Value::Int(mvaddnstr((int)args[0].asInt(), (int)args[1].asInt(), args[2].asString().c_str(), (int)args[3].asInt()));
		});
		define("mvaddstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("mvaddstr expects 3 arguments", l, c);
			return Value::Int(mvaddstr((int)args[0].asInt(), (int)args[1].asInt(), args[2].asString().c_str()));
		});
		define("mvcur", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("mvcur expects 4 arguments", l, c);
			return Value::Int(mvcur((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt()));
		});
		define("mvdelch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("mvdelch expects 2 arguments", l, c);
			return Value::Int(mvdelch((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("mvderwin", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("mvderwin expects 3 arguments", l, c);
			return Value::Int(mvderwin((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt()));
		});
		define("mvgetch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("mvgetch expects 2 arguments", l, c);
			return Value::Int(mvgetch((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("mvgetnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("mvgetnstr expects 4 arguments", l, c);
			return Value::Int(mvgetnstr((int)args[0].asInt(), (int)args[1].asInt(), (char *)args[2].asString().c_str(), (int)args[3].asInt()));
		});
		define("mvgetstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("mvgetstr expects 3 arguments", l, c);
			return Value::Int(mvgetstr((int)args[0].asInt(), (int)args[1].asInt(), (char *)args[2].asString().c_str()));
		});
		define("mvhline", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("mvhline expects 4 arguments", l, c);
			return Value::Int(mvhline((int)args[0].asInt(), (int)args[1].asInt(), (chtype)args[2].asInt(), (int)args[3].asInt()));
		});
		define("mvinch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("mvinch expects 2 arguments", l, c);
			return Value::Int(mvinch((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("mvinnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("mvinnstr expects 4 arguments", l, c);
			return Value::Int(mvinnstr((int)args[0].asInt(), (int)args[1].asInt(), (char *)args[2].asString().c_str(), (int)args[3].asInt()));
		});
		define("mvinsch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("mvinsch expects 3 arguments", l, c);
			return Value::Int(mvinsch((int)args[0].asInt(), (int)args[1].asInt(), (chtype)args[2].asInt()));
		});
		define("mvinsnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("mvinsnstr expects 4 arguments", l, c);
			return Value::Int(mvinsnstr((int)args[0].asInt(), (int)args[1].asInt(), args[2].asString().c_str(), (int)args[3].asInt()));
		});
		define("mvinsstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("mvinsstr expects 3 arguments", l, c);
			return Value::Int(mvinsstr((int)args[0].asInt(), (int)args[1].asInt(), args[2].asString().c_str()));
		});
		define("mvinstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("mvinstr expects 3 arguments", l, c);
			return Value::Int(mvinstr((int)args[0].asInt(), (int)args[1].asInt(), (char *)args[2].asString().c_str()));
		});
		define("mvvline", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("mvvline expects 4 arguments", l, c);
			return Value::Int(mvvline((int)args[0].asInt(), (int)args[1].asInt(), (chtype)args[2].asInt(), (int)args[3].asInt()));
		});
		define("mvwaddnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("mvwaddnstr expects 5 arguments", l, c);
			return Value::Int(mvwaddnstr((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), args[3].asString().c_str(), (int)args[4].asInt()));
		});
		define("mvwaddstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("mvwaddstr expects 4 arguments", l, c);
			return Value::Int(mvwaddstr((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), args[3].asString().c_str()));
		});
		define("mvwdelch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("mvwdelch expects 3 arguments", l, c);
			return Value::Int(mvwdelch((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt()));
		});
		define("mvwgetch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("mvwgetch expects 3 arguments", l, c);
			return Value::Int(mvwgetch((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt()));
		});
		define("mvwgetnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("mvwgetnstr expects 5 arguments", l, c);
			return Value::Int(mvwgetnstr((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (char *)args[3].asString().c_str(), (int)args[4].asInt()));
		});
		define("mvwgetstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("mvwgetstr expects 4 arguments", l, c);
			return Value::Int(mvwgetstr((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (char *)args[3].asString().c_str()));
		});
		define("mvwhline", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("mvwhline expects 5 arguments", l, c);
			return Value::Int(mvwhline((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (chtype)args[3].asInt(), (int)args[4].asInt()));
		});
		define("mvwin", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("mvwin expects 3 arguments", l, c);
			return Value::Int(mvwin((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt()));
		});
		define("mvwinch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("mvwinch expects 3 arguments", l, c);
			return Value::Int(mvwinch((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt()));
		});
		define("mvwinnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("mvwinnstr expects 5 arguments", l, c);
			return Value::Int(mvwinnstr((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (char *)args[3].asString().c_str(), (int)args[4].asInt()));
		});
		define("mvwinsch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("mvwinsch expects 4 arguments", l, c);
			return Value::Int(mvwinsch((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (chtype)args[3].asInt()));
		});
		define("mvwinsnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("mvwinsnstr expects 5 arguments", l, c);
			return Value::Int(mvwinsnstr((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), args[3].asString().c_str(), (int)args[4].asInt()));
		});
		define("mvwinsstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("mvwinsstr expects 4 arguments", l, c);
			return Value::Int(mvwinsstr((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), args[3].asString().c_str()));
		});
		define("mvwinstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("mvwinstr expects 4 arguments", l, c);
			return Value::Int(mvwinstr((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (char *)args[3].asString().c_str()));
		});
		define("mvwvline", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("mvwvline expects 5 arguments", l, c);
			return Value::Int(mvwvline((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (chtype)args[3].asInt(), (int)args[4].asInt()));
		});
		define("napms", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("napms expects 1 arguments", l, c);
			return Value::Int(napms((int)args[0].asInt()));
		});
		define("newpad", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("newpad expects 2 arguments", l, c);
			return Value::Int((long long)newpad((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("newwin", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("newwin expects 4 arguments", l, c);
			return Value::Int((long long)newwin((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt()));
		});
		define("nl", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("nl expects 0 arguments", l, c);
			return Value::Int(nl());
		});
		define("nocbreak", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("nocbreak expects 0 arguments", l, c);
			return Value::Int(nocbreak());
		});
		define("nodelay", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("nodelay expects 2 arguments", l, c);
			return Value::Int(nodelay((WINDOW *)(long long)args[0].asInt(), args[1].asBool()));
		});
		define("noecho", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("noecho expects 0 arguments", l, c);
			return Value::Int(noecho());
		});
		define("nonl", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("nonl expects 0 arguments", l, c);
			return Value::Int(nonl());
		});
		define("noqiflush", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("noqiflush expects 0 arguments", l, c);
			noqiflush();
			return Value::None();
		});
		define("noraw", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("noraw expects 0 arguments", l, c);
			return Value::Int(noraw());
		});
		define("notimeout", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("notimeout expects 2 arguments", l, c);
			return Value::Int(notimeout((WINDOW *)(long long)args[0].asInt(), args[1].asBool()));
		});
		define("PAIR_NUMBER", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("PAIR_NUMBER expects 1 arguments", l, c);
			return Value::Int(PAIR_NUMBER((int)args[0].asInt()));
		});
		define("prefresh", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 7) throw ArgumentError("prefresh expects 7 arguments", l, c);
			return Value::Int(prefresh((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt(), (int)args[4].asInt(), (int)args[5].asInt(), (int)args[6].asInt()));
		});
		define("qiflush", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("qiflush expects 0 arguments", l, c);
			qiflush();
			return Value::None();
		});
		define("raw", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("raw expects 0 arguments", l, c);
			return Value::Int(raw());
		});
		define("redrawwin", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("redrawwin expects 1 arguments", l, c);
			return Value::Int(redrawwin((WINDOW *)(long long)args[0].asInt()));
		});
		define("refresh", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("refresh expects 0 arguments", l, c);
			return Value::Int(refresh());
		});
		define("resetty", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("resetty expects 0 arguments", l, c);
			return Value::Int(resetty());
		});
		define("reset_prog_mode", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("reset_prog_mode expects 0 arguments", l, c);
			return Value::Int(reset_prog_mode());
		});
		define("reset_shell_mode", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("reset_shell_mode expects 0 arguments", l, c);
			return Value::Int(reset_shell_mode());
		});
		define("savetty", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("savetty expects 0 arguments", l, c);
			return Value::Int(savetty());
		});
		define("scr_dump", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("scr_dump expects 1 arguments", l, c);
			return Value::Int(scr_dump(args[0].asString().c_str()));
		});
		define("scr_init", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("scr_init expects 1 arguments", l, c);
			return Value::Int(scr_init(args[0].asString().c_str()));
		});
		define("scrl", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("scrl expects 1 arguments", l, c);
			return Value::Int(scrl((int)args[0].asInt()));
		});
		define("scroll", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("scroll expects 1 arguments", l, c);
			return Value::Int(scroll((WINDOW *)(long long)args[0].asInt()));
		});
		define("scrollok", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("scrollok expects 2 arguments", l, c);
			return Value::Int(scrollok((WINDOW *)(long long)args[0].asInt(), args[1].asBool()));
		});
		define("scr_restore", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("scr_restore expects 1 arguments", l, c);
			return Value::Int(scr_restore(args[0].asString().c_str()));
		});
		define("scr_set", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("scr_set expects 1 arguments", l, c);
			return Value::Int(scr_set(args[0].asString().c_str()));
		});
		define("setscrreg", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("setscrreg expects 2 arguments", l, c);
			return Value::Int(setscrreg((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("slk_clear", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("slk_clear expects 0 arguments", l, c);
			return Value::Int(slk_clear());
		});
		define("slk_color", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("slk_color expects 1 arguments", l, c);
			return Value::Int(slk_color((NCURSES_PAIRS_T)args[0].asInt()));
		});
		define("slk_init", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("slk_init expects 1 arguments", l, c);
			return Value::Int(slk_init((int)args[0].asInt()));
		});
		define("slk_label", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("slk_label expects 1 arguments", l, c);
			return Value::String(slk_label((int)args[0].asInt()));
		});
		define("slk_noutrefresh", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("slk_noutrefresh expects 0 arguments", l, c);
			return Value::Int(slk_noutrefresh());
		});
		define("slk_refresh", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("slk_refresh expects 0 arguments", l, c);
			return Value::Int(slk_refresh());
		});
		define("slk_restore", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("slk_restore expects 0 arguments", l, c);
			return Value::Int(slk_restore());
		});
		define("slk_set", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("slk_set expects 3 arguments", l, c);
			return Value::Int(slk_set((int)args[0].asInt(), args[1].asString().c_str(), (int)args[2].asInt()));
		});
		define("slk_touch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("slk_touch expects 0 arguments", l, c);
			return Value::Int(slk_touch());
		});
		define("standout", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("standout expects 0 arguments", l, c);
			return Value::Int(standout());
		});
		define("standend", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("standend expects 0 arguments", l, c);
			return Value::Int(standend());
		});
		define("start_color", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("start_color expects 0 arguments", l, c);
			return Value::Int(start_color());
		});
		define("subpad", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("subpad expects 5 arguments", l, c);
			return Value::Int((long long)subpad((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt(), (int)args[4].asInt()));
		});
		define("subwin", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("subwin expects 5 arguments", l, c);
			return Value::Int((long long)subwin((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt(), (int)args[4].asInt()));
		});
		define("syncok", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("syncok expects 2 arguments", l, c);
			return Value::Int(syncok((WINDOW *)(long long)args[0].asInt(), args[1].asBool()));
		});
		define("termattrs", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("termattrs expects 0 arguments", l, c);
			return Value::Int(termattrs());
		});
		define("termname", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("termname expects 0 arguments", l, c);
			return Value::String(termname());
		});
		define("timeout", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("timeout expects 1 arguments", l, c);
			timeout((int)args[0].asInt());
			return Value::None();
		});
		define("touchline", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("touchline expects 3 arguments", l, c);
			return Value::Int(touchline((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt()));
		});
		define("touchwin", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("touchwin expects 1 arguments", l, c);
			return Value::Int(touchwin((WINDOW *)(long long)args[0].asInt()));
		});
		define("typeahead", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("typeahead expects 1 arguments", l, c);
			return Value::Int(typeahead((int)args[0].asInt()));
		});
		define("ungetch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("ungetch expects 1 arguments", l, c);
			return Value::Int(ungetch((int)args[0].asInt()));
		});
		define("untouchwin", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("untouchwin expects 1 arguments", l, c);
			return Value::Int(untouchwin((WINDOW *)(long long)args[0].asInt()));
		});
		define("use_env", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("use_env expects 1 arguments", l, c);
			use_env(args[0].asBool());
			return Value::None();
		});
		define("use_tioctl", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("use_tioctl expects 1 arguments", l, c);
			use_tioctl(args[0].asBool());
			return Value::None();
		});
		define("vidattr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("vidattr expects 1 arguments", l, c);
			return Value::Int(vidattr((chtype)args[0].asInt()));
		});
		define("vline", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("vline expects 2 arguments", l, c);
			return Value::Int(vline((chtype)args[0].asInt(), (int)args[1].asInt()));
		});
		define("waddnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("waddnstr expects 3 arguments", l, c);
			return Value::Int(waddnstr((WINDOW *)(long long)args[0].asInt(), args[1].asString().c_str(), (int)args[2].asInt()));
		});
		define("waddstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("waddstr expects 2 arguments", l, c);
			return Value::Int(waddstr((WINDOW *)(long long)args[0].asInt(), args[1].asString().c_str()));
		});
		define("wattron", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("wattron expects 2 arguments", l, c);
			return Value::Int(wattron((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt()));
		});
		define("wattroff", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("wattroff expects 2 arguments", l, c);
			return Value::Int(wattroff((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt()));
		});
		define("wattrset", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("wattrset expects 2 arguments", l, c);
			return Value::Int(wattrset((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt()));
		});
		define("wbkgd", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("wbkgd expects 2 arguments", l, c);
			return Value::Int(wbkgd((WINDOW *)(long long)args[0].asInt(), (chtype)args[1].asInt()));
		});
		define("wbkgdset", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("wbkgdset expects 2 arguments", l, c);
			wbkgdset((WINDOW *)(long long)args[0].asInt(), (chtype)args[1].asInt());
			return Value::None();
		});
		define("wborder", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 9) throw ArgumentError("wborder expects 9 arguments", l, c);
			return Value::Int(wborder((WINDOW *)(long long)args[0].asInt(), (chtype)args[1].asInt(), (chtype)args[2].asInt(), (chtype)args[3].asInt(), (chtype)args[4].asInt(), (chtype)args[5].asInt(), (chtype)args[6].asInt(), (chtype)args[7].asInt(), (chtype)args[8].asInt()));
		});
		define("wclear", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("wclear expects 1 arguments", l, c);
			return Value::Int(wclear((WINDOW *)(long long)args[0].asInt()));
		});
		define("wclrtobot", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("wclrtobot expects 1 arguments", l, c);
			return Value::Int(wclrtobot((WINDOW *)(long long)args[0].asInt()));
		});
		define("wclrtoeol", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("wclrtoeol expects 1 arguments", l, c);
			return Value::Int(wclrtoeol((WINDOW *)(long long)args[0].asInt()));
		});
		define("wcursyncup", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("wcursyncup expects 1 arguments", l, c);
			wcursyncup((WINDOW *)(long long)args[0].asInt());
			return Value::None();
		});
		define("wdelch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("wdelch expects 1 arguments", l, c);
			return Value::Int(wdelch((WINDOW *)(long long)args[0].asInt()));
		});
		define("wdeleteln", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("wdeleteln expects 1 arguments", l, c);
			return Value::Int(wdeleteln((WINDOW *)(long long)args[0].asInt()));
		});
		define("werase", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("werase expects 1 arguments", l, c);
			return Value::Int(werase((WINDOW *)(long long)args[0].asInt()));
		});
		define("wgetch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("wgetch expects 1 arguments", l, c);
			return Value::Int(wgetch((WINDOW *)(long long)args[0].asInt()));
		});
		define("wgetnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("wgetnstr expects 3 arguments", l, c);
			return Value::Int(wgetnstr((WINDOW *)(long long)args[0].asInt(), (char *)args[1].asString().c_str(), (int)args[2].asInt()));
		});
		define("wgetstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("wgetstr expects 2 arguments", l, c);
			return Value::Int(wgetstr((WINDOW *)(long long)args[0].asInt(), (char *)args[1].asString().c_str()));
		});
		define("whline", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("whline expects 3 arguments", l, c);
			return Value::Int(whline((WINDOW *)(long long)args[0].asInt(), (chtype)args[1].asInt(), (int)args[2].asInt()));
		});
		define("winch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("winch expects 1 arguments", l, c);
			return Value::Int(winch((WINDOW *)(long long)args[0].asInt()));
		});
		define("winnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("winnstr expects 3 arguments", l, c);
			return Value::Int(winnstr((WINDOW *)(long long)args[0].asInt(), (char *)args[1].asString().c_str(), (int)args[2].asInt()));
		});
		define("winsch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("winsch expects 2 arguments", l, c);
			return Value::Int(winsch((WINDOW *)(long long)args[0].asInt(), (chtype)args[1].asInt()));
		});
		define("winsdelln", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("winsdelln expects 2 arguments", l, c);
			return Value::Int(winsdelln((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt()));
		});
		define("winsertln", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("winsertln expects 1 arguments", l, c);
			return Value::Int(winsertln((WINDOW *)(long long)args[0].asInt()));
		});
		define("winsnstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("winsnstr expects 3 arguments", l, c);
			return Value::Int(winsnstr((WINDOW *)(long long)args[0].asInt(), args[1].asString().c_str(), (int)args[2].asInt()));
		});
		define("winsstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("winsstr expects 2 arguments", l, c);
			return Value::Int(winsstr((WINDOW *)(long long)args[0].asInt(), args[1].asString().c_str()));
		});
		define("winstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("winstr expects 2 arguments", l, c);
			return Value::Int(winstr((WINDOW *)(long long)args[0].asInt(), (char *)args[1].asString().c_str()));
		});
		define("wmove", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("wmove expects 3 arguments", l, c);
			return Value::Int(wmove((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt()));
		});
		define("wnoutrefresh", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("wnoutrefresh expects 1 arguments", l, c);
			return Value::Int(wnoutrefresh((WINDOW *)(long long)args[0].asInt()));
		});
		define("wredrawln", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("wredrawln expects 3 arguments", l, c);
			return Value::Int(wredrawln((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt()));
		});
		define("wrefresh", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("wrefresh expects 1 arguments", l, c);
			return Value::Int(wrefresh((WINDOW *)(long long)args[0].asInt()));
		});
		define("wscrl", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("wscrl expects 2 arguments", l, c);
			return Value::Int(wscrl((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt()));
		});
		define("wsetscrreg", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("wsetscrreg expects 3 arguments", l, c);
			return Value::Int(wsetscrreg((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt()));
		});
		define("wstandout", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("wstandout expects 1 arguments", l, c);
			return Value::Int(wstandout((WINDOW *)(long long)args[0].asInt()));
		});
		define("wstandend", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("wstandend expects 1 arguments", l, c);
			return Value::Int(wstandend((WINDOW *)(long long)args[0].asInt()));
		});
		define("wsyncdown", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("wsyncdown expects 1 arguments", l, c);
			wsyncdown((WINDOW *)(long long)args[0].asInt());
			return Value::None();
		});
		define("wsyncup", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("wsyncup expects 1 arguments", l, c);
			wsyncup((WINDOW *)(long long)args[0].asInt());
			return Value::None();
		});
		define("wtimeout", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("wtimeout expects 2 arguments", l, c);
			wtimeout((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt());
			return Value::None();
		});
		define("wtouchln", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("wtouchln expects 4 arguments", l, c);
			return Value::Int(wtouchln((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt()));
		});
		define("wvline", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("wvline expects 3 arguments", l, c);
			return Value::Int(wvline((WINDOW *)(long long)args[0].asInt(), (chtype)args[1].asInt(), (int)args[2].asInt()));
		});
		define("tigetflag", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("tigetflag expects 1 arguments", l, c);
			return Value::Int(tigetflag(args[0].asString().c_str()));
		});
		define("tigetnum", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("tigetnum expects 1 arguments", l, c);
			return Value::Int(tigetnum(args[0].asString().c_str()));
		});
		define("tigetstr", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("tigetstr expects 1 arguments", l, c);
			return Value::String(tigetstr(args[0].asString().c_str()));
		});
		define("putp", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("putp expects 1 arguments", l, c);
			return Value::Int(putp(args[0].asString().c_str()));
		});
		define("assume_default_colors", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("assume_default_colors expects 2 arguments", l, c);
			return Value::Int(assume_default_colors((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("curses_version", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("curses_version expects 0 arguments", l, c);
			return Value::String(curses_version());
		});
		define("define_key", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("define_key expects 2 arguments", l, c);
			return Value::Int(define_key(args[0].asString().c_str(), (int)args[1].asInt()));
		});
		define("get_escdelay", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("get_escdelay expects 0 arguments", l, c);
			return Value::Int(get_escdelay());
		});
		define("is_cbreak", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("is_cbreak expects 0 arguments", l, c);
			return Value::Int(is_cbreak());
		});
		define("is_echo", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("is_echo expects 0 arguments", l, c);
			return Value::Int(is_echo());
		});
		define("is_nl", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("is_nl expects 0 arguments", l, c);
			return Value::Int(is_nl());
		});
		define("is_raw", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("is_raw expects 0 arguments", l, c);
			return Value::Int(is_raw());
		});
		define("is_term_resized", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("is_term_resized expects 2 arguments", l, c);
			return Value::Bool(is_term_resized((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("key_defined", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("key_defined expects 1 arguments", l, c);
			return Value::Int(key_defined(args[0].asString().c_str()));
		});
		define("keybound", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("keybound expects 2 arguments", l, c);
			return Value::String(keybound((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("keyok", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("keyok expects 2 arguments", l, c);
			return Value::Int(keyok((int)args[0].asInt(), args[1].asBool()));
		});
		define("nofilter", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("nofilter expects 0 arguments", l, c);
			nofilter();
			return Value::None();
		});
		define("resize_term", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("resize_term expects 2 arguments", l, c);
			return Value::Int(resize_term((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("resizeterm", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("resizeterm expects 2 arguments", l, c);
			return Value::Int(resizeterm((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("set_escdelay", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("set_escdelay expects 1 arguments", l, c);
			return Value::Int(set_escdelay((int)args[0].asInt()));
		});
		define("set_tabsize", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("set_tabsize expects 1 arguments", l, c);
			return Value::Int(set_tabsize((int)args[0].asInt()));
		});
		define("use_default_colors", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("use_default_colors expects 0 arguments", l, c);
			return Value::Int(use_default_colors());
		});
		define("use_legacy_coding", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("use_legacy_coding expects 1 arguments", l, c);
			return Value::Int(use_legacy_coding((int)args[0].asInt()));
		});
		define("wresize", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("wresize expects 3 arguments", l, c);
			return Value::Int(wresize((WINDOW *)(long long)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt()));
		});
		define("use_extended_names", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("use_extended_names expects 1 arguments", l, c);
			return Value::Int(use_extended_names(args[0].asBool()));
		});
		define("has_mouse", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("has_mouse expects 0 arguments", l, c);
			return Value::Bool(has_mouse());
		});
		define("mouseinterval", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("mouseinterval expects 1 arguments", l, c);
			return Value::Int(mouseinterval((int)args[0].asInt()));
		});
		define("mcprint", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("mcprint expects 2 arguments", l, c);
			return Value::Int(mcprint((char *)args[0].asString().c_str(), (int)args[1].asInt()));
		});
		define("has_key", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("has_key expects 1 arguments", l, c);
			return Value::Int(has_key((int)args[0].asInt()));
		});

		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Ncurses", modVal, false, false);
	};

#endif
	modules["Json"] = [](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Json");
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
		static std::function<json(Value, int, int, std::vector<void *> &)>
			ValueToJson;
		static std::function<Value(json, int, int)> JsonToValue;
		static auto jsonClass = std::make_shared<ClassObject>("JsonObject");
		ValueToJson = [&](Value v, int l, int c,
							  std::vector<void *> &visited) -> json {
			if (v.type == ValueType::NONE)
				return nullptr;
			if (v.type == ValueType::BOOL)
				return v.asBool();
			if (v.type == ValueType::INT)
				return v.asInt();
			if (v.type == ValueType::FLOAT)
				return v.asFloat();
			if (v.type == ValueType::STRING)
				return v.asString();
			if (v.type == ValueType::LIST || v.type == ValueType::INSTANCE) {
				if (!v.ref)
					return nullptr;
				void *ptr = v.ref.get();
				for (void *p : visited)
					if (p == ptr)
						return "[Cyclic Reference]";
				visited.push_back(ptr);
				if (v.type == ValueType::LIST) {
					json j = json::array();
					auto list = static_cast<ListObject *>(ptr);
					for (auto &item : list->elements)
						j.push_back(ValueToJson(item, l, c, visited));
					visited.pop_back();
					return j;
				}
				if (v.type == ValueType::INSTANCE) {
					json j = json::object();
					auto inst = static_cast<InstanceObject *>(ptr);
					for (auto &pair : inst->fields) {
						if (pair.second.type != ValueType::INT &&
							 pair.second.type != ValueType::FLOAT &&
							 pair.second.type != ValueType::STRING &&
							 pair.second.type != ValueType::BOOL &&
							 pair.second.type != ValueType::LIST &&
							 pair.second.type != ValueType::INSTANCE &&
							 pair.second.type != ValueType::NONE) {
							continue;
						}
						j[pair.first] = ValueToJson(pair.second, l, c, visited);
					}
					visited.pop_back();
					return j;
				}
			}
			return nullptr;
		};
		JsonToValue = [&](json j, int l, int c) -> Value {
			if (j.is_null())
				return Value::None();
			if (j.is_boolean())
				return Value::Bool(j.get<bool>());
			if (j.is_number_integer())
				return Value::Int(j.get<long long>());
			if (j.is_number_float())
				return Value::Float(j.get<double>());
			if (j.is_string())
				return Value::String(j.get<std::string>());
			if (j.is_array()) {
				auto list = std::make_shared<ListObject>();
				for (auto &item : j) {
					list->elements.push_back(JsonToValue(item, l, c));
				}
				Value v;
				v.type = ValueType::LIST;
				v.ref = list;
				return v;
			}
			if (j.is_object()) {
				auto inst = std::make_shared<InstanceObject>(jsonClass.get());
				for (auto &[key, value] : j.items()) {
					inst->fields[key] = JsonToValue(value, l, c);
				}
				Value v;
				v.type = ValueType::INSTANCE;
				v.ref = inst;
				return v;
			}
			return Value::None();
		};
		define("ParseJson", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Json.Parse(string)", l, c);
			try {
				json j = json::parse(args[0].asString());
				return JsonToValue(j, l, c);
			} catch (json::parse_error &e) {
				throw ParseError(
					"Invalid JSON string at byte " + std::to_string(e.byte), l, c);
			}
		});
		define("StringifyJson", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1 && args.size() != 2)
				throw ArgumentError("Json.stringify(object, indent = -1)", l, c);
			std::vector<void *> visited;
			json j = ValueToJson(args[0], l, c, visited);
			return Value::String(j.dump(args.size() == 2 ? args[1].asInt() : -1));
		});
		define("IsValidJson", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Json.isValid(string)", l, c);
			bool isValid = json::accept(args[0].asString());
			return Value::Bool(isValid);
		});
		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Json", modVal, false, false);
	};
	modules["QRgen"] = [](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("QRgen");
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
		static auto QrClass = std::make_shared<ClassObject>("QRcode");
		auto QrToValue = [=](QrCode qr, int l, int c) -> Value {
			auto inst = std::make_shared<InstanceObject>(QrClass.get());
			inst->fields["Version"] = Value::Int(qr.getVersion());
			inst->fields["Size"] = Value::Int(qr.getSize());
			std::vector<Value> arr;
			arr.reserve(qr.getSize());
			for (int x = 0; x < qr.getSize(); x++) {
				std::vector<Value> temp;
				temp.reserve(qr.getSize());
				for (int y = 0; y < qr.getSize(); y++) {
					temp.push_back(Value::Bool(qr.getModule(x, y)));
				}
				arr.push_back(Value::Tuple(temp));
			}
			inst->fields["BoolArray"] = Value::Tuple(arr);
			inst->fields["CorrectionLevel"] =
				Value::Int(static_cast<long long>(qr.getErrorCorrectionLevel()));
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};
		env->set("ECC_LEVEL_LOW",
			Value::Int(static_cast<long long>(QrCode::Ecc::LOW)), true,
			true);
		env->set("ECC_LEVEL_MEDIUM",
			Value::Int(static_cast<long long>(QrCode::Ecc::MEDIUM)), true,
			true);
		env->set("ECC_LEVEL_QUARTILE",
			Value::Int(static_cast<long long>(QrCode::Ecc::QUARTILE)), true,
			true);
		env->set("ECC_LEVEL_HIGH",
			Value::Int(static_cast<long long>(QrCode::Ecc::HIGH)), true,
			true);
		define("GenerateQRcode", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("EncodeText(text, encodeLevel)", l, c);
			QrCode QR =
				QrCode::encodeText(args[0].asString().c_str(),
					static_cast<QrCode::Ecc>(args[1].asInt()));
			return QrToValue(QR, l, c);
		});
		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Math", modVal, false, false);
	};
	
	modules["Socket"] = [](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Socket");
		moduleNamespace->mro.push_back(moduleNamespace.get());

		static auto socketClass = std::make_shared<ClassObject>("SocketObj");

		auto defineConstants = [&](string name, long val) {
			moduleNamespace->staticFields[name] = Value::Int(val);
		};
		defineConstants("AF_INET", AF_INET);
		defineConstants("AF_INET6", AF_INET6);
		defineConstants("SOCK_STREAM", SOCK_STREAM);
		defineConstants("SOCK_DGRAM", SOCK_DGRAM);
		defineConstants("SOCK_RAW", SOCK_RAW);
		defineConstants("IPPROTO_TCP", IPPROTO_TCP);
		defineConstants("IPPROTO_UDP", IPPROTO_UDP);
		defineConstants("SOL_SOCKET", SOL_SOCKET);
		defineConstants("SO_REUSEADDR", SO_REUSEADDR);
		defineConstants("SO_BROADCAST", SO_BROADCAST);

		socketClass->methods["bind"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 4) throw ArgumentError("bind(host, port)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
#ifdef _WIN32
				SOCKET sock = (SOCKET)self->fields["ptr"].aspInt();
#else
				int sock = (int)(intptr_t)self->fields["ptr"].aspInt();
#endif
				string host = args[2].asString();
				int port = (int)args[3].asInt();

				struct sockaddr_in addr;
				addr.sin_family = AF_INET;
				addr.sin_port = htons(port);
				addr.sin_addr.s_addr = inet_addr(host.c_str());

				if (::bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
					throw RuntimeError("bind() failed", l, c);
				}
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};

		socketClass->methods["listen"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 3) throw ArgumentError("listen(backlog)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
#ifdef _WIN32
				SOCKET sock = (SOCKET)self->fields["ptr"].aspInt();
#else
				int sock = (int)(intptr_t)self->fields["ptr"].aspInt();
#endif
				int backlog = (int)args[2].asInt();
				if (::listen(sock, backlog) < 0) {
					throw RuntimeError("listen() failed", l, c);
				}
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};

		socketClass->methods["accept"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
#ifdef _WIN32
				SOCKET sock = (SOCKET)self->fields["ptr"].aspInt();
				struct sockaddr_in client_addr;
				int addrlen = sizeof(client_addr);
				SOCKET client_sock = ::accept(sock, (struct sockaddr*)&client_addr, &addrlen);
				if (client_sock == INVALID_SOCKET) throw RuntimeError("accept() failed", l, c);
#else
				int sock = (int)(intptr_t)self->fields["ptr"].aspInt();
				struct sockaddr_in client_addr;
				socklen_t addrlen = sizeof(client_addr);
				int client_sock = ::accept(sock, (struct sockaddr*)&client_addr, &addrlen);
				if (client_sock < 0) throw RuntimeError("accept() failed", l, c);
#endif
				auto inst = std::make_shared<InstanceObject>(socketClass.get());
				inst->fields["ptr"] = Value::pInt((void*)(intptr_t)client_sock);
				Value v;
				v.type = ValueType::INSTANCE;
				v.ref = inst;
				return v;
			}),
			.access = AccessLevel::PUBLIC
		};

		socketClass->methods["connect"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 4) throw ArgumentError("connect(host, port)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
#ifdef _WIN32
				SOCKET sock = (SOCKET)self->fields["ptr"].aspInt();
#else
				int sock = (int)(intptr_t)self->fields["ptr"].aspInt();
#endif
				string host = args[2].asString();
				int port = (int)args[3].asInt();

				struct sockaddr_in addr;
				addr.sin_family = AF_INET;
				addr.sin_port = htons(port);
				addr.sin_addr.s_addr = inet_addr(host.c_str());

				if (::connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
					throw RuntimeError("connect() failed", l, c);
				}
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};

		socketClass->methods["send"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 3) throw ArgumentError("send(data)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
#ifdef _WIN32
				SOCKET sock = (SOCKET)self->fields["ptr"].aspInt();
#else
				int sock = (int)(intptr_t)self->fields["ptr"].aspInt();
#endif
				string data = args[2].asString();
				int sent = ::send(sock, data.c_str(), data.length(), 0);
				return Value::Int(sent);
			}),
			.access = AccessLevel::PUBLIC
		};

		socketClass->methods["recv"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 3) throw ArgumentError("recv(size)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
#ifdef _WIN32
				SOCKET sock = (SOCKET)self->fields["ptr"].aspInt();
#else
				int sock = (int)(intptr_t)self->fields["ptr"].aspInt();
#endif
				int size = (int)args[2].asInt();
				std::vector<char> buffer(size);
				int received = ::recv(sock, buffer.data(), size, 0);
				if (received <= 0) return Value::String("");
				return Value::String(string(buffer.data(), received));
			}),
			.access = AccessLevel::PUBLIC
		};

		socketClass->methods["close"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
#ifdef _WIN32
				SOCKET sock = (SOCKET)self->fields["ptr"].aspInt();
				::closesocket(sock);
#else
				int sock = (int)(intptr_t)self->fields["ptr"].aspInt();
				::close(sock);
#endif
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};
        
		
		socketClass->methods["setblocking"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 3) throw ArgumentError("setblocking(bool)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
#ifdef _WIN32
				SOCKET sock = (SOCKET)self->fields["ptr"].aspInt();
#else
				int sock = (int)(intptr_t)self->fields["ptr"].aspInt();
#endif
				bool block = args[2].asBool();
#ifdef _WIN32
				u_long mode = block ? 0 : 1;
				if (ioctlsocket(sock, FIONBIO, &mode) != NO_ERROR) {
					throw RuntimeError("ioctlsocket failed", l, c);
				}
#else
				int flags = fcntl(sock, F_GETFL, 0);
				if (flags == -1) throw RuntimeError("fcntl F_GETFL failed", l, c);
				flags = block ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
				if (fcntl(sock, F_SETFL, flags) != 0) {
					throw RuntimeError("fcntl F_SETFL failed", l, c);
				}
#endif
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};

		socketClass->methods["setsockopt"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 5) throw ArgumentError("setsockopt(level, optname, value)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
#ifdef _WIN32
				SOCKET sock = (SOCKET)self->fields["ptr"].aspInt();
#else
				int sock = (int)(intptr_t)self->fields["ptr"].aspInt();
#endif
				int level = (int)args[2].asInt();
				int optname = (int)args[3].asInt();
				int value = (int)args[4].asInt();
				
#ifdef _WIN32
                const char* val_ptr = (const char*)&value;
#else
                const void* val_ptr = &value;
#endif

				if (::setsockopt(sock, level, optname, val_ptr, sizeof(value)) < 0) {
					throw RuntimeError("setsockopt() failed", l, c);
				}
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};

		moduleNamespace->staticFields["create"] = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
			if (args.size() < 3) throw ArgumentError("Socket.create(domain, type, protocol)", l, c);
			int domain = (int)args[0].asInt();
			int type = (int)args[1].asInt();
			int protocol = (int)args[2].asInt();
			
#ifdef _WIN32
			SOCKET sock = ::socket(domain, type, protocol);
			if (sock == INVALID_SOCKET) throw RuntimeError("Failed to create socket", l, c);
#else
			int sock = ::socket(domain, type, protocol);
			if (sock < 0) throw RuntimeError("Failed to create socket", l, c);
#endif
			auto inst = std::make_shared<InstanceObject>(socketClass.get());
			inst->fields["ptr"] = Value::pInt((void*)(intptr_t)sock);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		});

		Value classVal;
		classVal.type = ValueType::CLASS;
		classVal.ref = socketClass;
		moduleNamespace->staticFields["SocketObj"] = classVal;
		
		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Socket", modVal, false, false);
	};


	modules["Http"] = [this](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Http");
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

		static auto responseClass = std::make_shared<ClassObject>("HttpResponse");
		auto ResultToValue = [=](const httplib::Result &res) -> Value {
			if (!res) {
				std::cerr << "HTTP request failed with error code: " << (int)res.error() << std::endl;
				return Value::None();
			}
			auto inst = std::make_shared<InstanceObject>(responseClass.get());
			inst->fields["status"] = Value::Int(res->status);
			inst->fields["body"] = Value::String(res->body);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};

		define("Get", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("Http.Get(url, path)", l, c);
			string url = args[0].asString();
			if (url.find("https://") == 0) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
				string host = url.substr(8);
				httplib::SSLClient cli(host);
				cli.enable_server_certificate_verification(false);
				return ResultToValue(cli.Get(args[1].asString()));
#else
				throw RuntimeError("HTTPS not supported (OpenSSL not enabled)", l, c);
#endif
			} else {
				httplib::Client cli(url);
				return ResultToValue(cli.Get(args[1].asString()));
			}
		});

		define("Post", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("Http.Post(url, path, body, contentType)", l, c);
			string url = args[0].asString();
			if (url.find("https://") == 0) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
				string host = url.substr(8);
				httplib::SSLClient cli(host);
				cli.enable_server_certificate_verification(false);
				return ResultToValue(cli.Post(args[1].asString(), args[2].asString(), args[3].asString()));
#else
				throw RuntimeError("HTTPS not supported", l, c);
#endif
			} else {
				httplib::Client cli(url);
				return ResultToValue(cli.Post(args[1].asString(), args[2].asString(), args[3].asString()));
			}
		});

		define("Put", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("Http.Put(url, path, body, contentType)", l, c);
			string url = args[0].asString();
			if (url.find("https://") == 0) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
				httplib::SSLClient cli(url);
				return ResultToValue(cli.Put(args[1].asString(), args[2].asString(), args[3].asString()));
#else
				throw RuntimeError("HTTPS not supported", l, c);
#endif
			} else {
				httplib::Client cli(url);
				return ResultToValue(cli.Put(args[1].asString(), args[2].asString(), args[3].asString()));
			}
		});

		define("Delete", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("Http.Delete(url, path)", l, c);
			string url = args[0].asString();
			if (url.find("https://") == 0) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
				httplib::SSLClient cli(url);
				return ResultToValue(cli.Delete(args[1].asString()));
#else
				throw RuntimeError("HTTPS not supported", l, c);
#endif
			} else {
				httplib::Client cli(url);
				return ResultToValue(cli.Delete(args[1].asString()));
			}
		});

		static auto httpServerClass = std::make_shared<ClassObject>("HttpServerObj");
		static auto httpsServerClass = std::make_shared<ClassObject>("HttpsServerObj");

		auto bindRouteMethod = [this, env](httplib::Server* svr, const string &method, const string &path, Value lambda, int l, int c) {
			auto handler = [=](const httplib::Request &req, httplib::Response &res) {
				Chunk tempChunk;
				int lambdaIdx = tempChunk.addConstant(lambda);
				int bodyIdx = tempChunk.addConstant(Value::String(req.body));
				tempChunk.write(OpCode::OP_CONSTANT, l, c);
				tempChunk.write((uint8_t)bodyIdx, l, c);
				tempChunk.write(OpCode::OP_CONSTANT, l, c);
				tempChunk.write((uint8_t)lambdaIdx, l, c);
				tempChunk.write(OpCode::OP_CALL, l, c);
				tempChunk.write((uint8_t)1, l, c);
				tempChunk.write(OpCode::OP_RETURN, l, c);
				VM tempVM;
				tempVM.globals = env;
				tempVM.methodResolver = [&](MethodCallExpr *m) { return this->Resolve_methods(m); };
				try {
					tempVM.run(tempChunk);
					Value ret = tempVM.stack.empty() ? Value::None() : tempVM.stack.back();
					if (ret.type == ValueType::STRING) {
						res.set_content(ret.asString(), "application/json");
					} else {
						res.set_content("Success", "text/plain");
					}
				} catch (...) {
					res.status = 500;
					res.set_content("Internal Server Error in y_lang VM", "text/plain");
				}
			};
			if (method == "GET") svr->Get(path, handler);
			else if (method == "POST") svr->Post(path, handler);
		};

		httpServerClass->methods["Get"] = ClassObject::MethodInfo{
			.func = Value::Native([=](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 4) throw ArgumentError("Server.Get(path, lambda)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				auto *svr = (httplib::Server *)self->fields["ptr"].aspInt();
				bindRouteMethod(svr, "GET", args[2].asString(), args[3], l, c);
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};
		httpServerClass->methods["Post"] = ClassObject::MethodInfo{
			.func = Value::Native([=](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 4) throw ArgumentError("Server.Post(path, lambda)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				auto *svr = (httplib::Server *)self->fields["ptr"].aspInt();
				bindRouteMethod(svr, "POST", args[2].asString(), args[3], l, c);
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};
		httpServerClass->methods["set_mount_point"] = ClassObject::MethodInfo{
			.func = Value::Native([=](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 4) throw ArgumentError("Server.set_mount_point(mount, dir)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				auto *svr = (httplib::Server *)self->fields["ptr"].aspInt();
				svr->set_mount_point(args[2].asString(), args[3].asString());
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};
		httpServerClass->methods["listen"] = ClassObject::MethodInfo{
			.func = Value::Native([=](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 4) throw ArgumentError("Server.listen(host, port)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				auto *svr = (httplib::Server *)self->fields["ptr"].aspInt();
				string host = args[2].asString();
				int port = (int)args[3].asInt();
				std::cout << "[ymm Http] Listening on " << host << ":" << port << "...\n";
				svr->listen(host.c_str(), port);
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};

		httpsServerClass->methods = httpServerClass->methods;

		moduleNamespace->staticFields["Server"] = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
			auto *svr = new httplib::Server();
			auto inst = std::make_shared<InstanceObject>(httpServerClass.get());
			inst->fields["ptr"] = Value::pInt(svr);
			Value v; v.type = ValueType::INSTANCE; v.ref = inst;
			return v;
		});

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
		moduleNamespace->staticFields["SSLServer"] = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
			if (args.size() < 2) throw ArgumentError("Http.SSLServer(cert_path, key_path)", l, c);
			auto *svr = new httplib::SSLServer(args[0].asString().c_str(), args[1].asString().c_str());
			auto inst = std::make_shared<InstanceObject>(httpsServerClass.get());
			inst->fields["ptr"] = Value::pInt(svr);
			Value v; v.type = ValueType::INSTANCE; v.ref = inst;
			return v;
		});
#endif

		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Http", modVal, false, false);
	};

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

		auto getSubject = [&](const Value& arg, int l, int c) -> string {
			if (arg.type == ValueType::STRING) {
				return arg.asString();
			} else if (arg.type == ValueType::FILE) {
				auto *fileObj = static_cast<FileObject *>(arg.ref.get());
				if (!fileObj->isOpen) throw RuntimeError("File is closed", l, c);
				fileObj->stream.clear();
				fileObj->stream.seekg(0, std::ios::beg);
				std::stringstream buffer;
				buffer << fileObj->stream.rdbuf();
				return buffer.str();
			}
			throw ArgumentError("Regex subject must be a String or a File", l, c);
		};

		define("match", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("Regex.match(pattern, string/file)", l, c);
			string pattern = args[0].asString();
			string subject = getSubject(args[1], l, c);
			try {
				std::regex re(pattern);
				return Value::Bool(std::regex_match(subject, re));
			} catch (const std::regex_error& e) {
				throw RuntimeError(string("Regex error: ") + e.what(), l, c);
			}
		});

		define("search", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("Regex.search(pattern, string/file)", l, c);
			string pattern = args[0].asString();
			string subject = getSubject(args[1], l, c);
			try {
				std::regex re(pattern);
				std::smatch m;
				if (std::regex_search(subject, m, re)) {
					std::vector<Value> results;
					for (auto& match : m) {
						results.push_back(Value::String(match.str()));
					}
					return Value::List(results);
				}
				return Value::List({});
			} catch (const std::regex_error& e) {
				throw RuntimeError(string("Regex error: ") + e.what(), l, c);
			}
		});

		define("find_all", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("Regex.find_all(pattern, string/file)", l, c);
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
			} catch (const std::regex_error& e) {
				throw RuntimeError(string("Regex error: ") + e.what(), l, c);
			}
		});

		define("replace", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("Regex.replace(pattern, string/file, replacement)", l, c);
			string pattern = args[0].asString();
			string subject = getSubject(args[1], l, c);
			string repl = args[2].asString();
			try {
				std::regex re(pattern);
				string result = std::regex_replace(subject, re, repl);
				return Value::String(result);
			} catch (const std::regex_error& e) {
				throw RuntimeError(string("Regex error: ") + e.what(), l, c);
			}
		});

		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Regex", modVal, false, false);
	};

	modules["OpenNN"] = [](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("OpenNN");
		moduleNamespace->mro.push_back(moduleNamespace.get());
		auto defineClass = [&](std::shared_ptr<ClassObject> cls) {
			Value classVal;
			classVal.type = ValueType::CLASS;
			classVal.ref = cls;
			moduleNamespace->staticFields[cls->name] = classVal;
			if (symbols.size() == 1 && symbols[0] == "*") {
				env->set(cls->name, classVal, true, true);
				return;
			}
			for (const auto &s : symbols) {
				if (s == cls->name) {
					env->set(cls->name, classVal, true, true);
					break;
				}
			}
		};
		static auto nnClass = std::make_shared<ClassObject>("NeuralNetwork");
      nnClass->staticFields["APPROXIMATION"] = Value::Int(0);
      nnClass->staticFields["CLASSIFICATION"] = Value::Int(1);
		nnClass->methods["__construct__"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 5) throw ArgumentError("NeuralNetwork(inputs, outputs, hidden, [mode])", l, c);
				long inputs = (long)args[2].asInt();
				long outputs = (long)args[3].asInt();
				long hidden = (long)args[4].asInt();
            int mode = 0;
            if (args.size() >= 6) mode = (int)args[5].asInt();
            opennn::NeuralNetwork* net = nullptr;
            if (mode == 1) net = new opennn::ClassificationNetwork({inputs}, {hidden}, {outputs});
            else net = new opennn::ApproximationNetwork({inputs}, {hidden}, {outputs});
            auto *self = static_cast<InstanceObject *>(args[0].ref.get());
            self->fields["ptr"] = Value::pInt(net);
				return args[0];
			}),
			.access = AccessLevel::PUBLIC
		};
		nnClass->methods["__destruct__"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				if (self->fields.find("ptr") != self->fields.end()) {
					auto *net = (opennn::NeuralNetwork *)self->fields["ptr"].aspInt();
					delete net;
				}
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};
		nnClass->methods["predict"] = ClassObject::MethodInfo{
         .func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
            if (args.size() < 3) throw ArgumentError("predict() requires 1 argument", l, c);
            auto *self = static_cast<InstanceObject *>(args[0].ref.get());
            auto *net = (opennn::NeuralNetwork *)self->fields["ptr"].aspInt();
            std::vector<Value>* elements_ptr = nullptr;
            if (args[2].type == ValueType::VECTOR) elements_ptr = &static_cast<VectorObject *>(args[2].ref.get())->elements;
				else if (args[2].type == ValueType::LIST) elements_ptr = &static_cast<ListObject *>(args[2].ref.get())->elements;
				else if (args[2].type == ValueType::TUPLE) elements_ptr = &static_cast<TupleObject *>(args[2].ref.get())->elements;
				else throw ArgumentError("predict() expects a vector <>, list [], or tuple ()", l, c);
            long input_size = (long)elements_ptr->size();
            MatrixR inputs(1, input_size);
            for (long i = 0; i < input_size; i++) inputs(0, i) = (*elements_ptr)[i].asFloat();
            MatrixR outputs = net->calculate_outputs(inputs);
            std::vector<Value> result;
            long output_size = outputs.cols();
            for (long i = 0; i < output_size; i++) result.push_back(Value::Float(outputs(0, i)));
				return Value::Vector(result);
         }),
         .access = AccessLevel::PUBLIC
      };
		nnClass->methods["get_weights"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() > 2) throw ArgumentError("get_weights()", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				auto *net = (opennn::NeuralNetwork *)self->fields["ptr"].aspInt();
				auto arr = net->get_parameters();
            std::vector<Value> vec(arr.size());
            for (const auto& element : arr) vec.push_back(Value::Float(element));
            Value resArr = Value::Vector(vec);
				return resArr;
			}),
			.access = AccessLevel::PUBLIC
		};
      nnClass->methods["set_weights"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() != 3) throw ArgumentError("set_weights(vector)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				auto *net = (opennn::NeuralNetwork *)self->fields["ptr"].aspInt();
				auto *weights = static_cast<VectorObject *>(args[2].ref.get());
            VectorR w(weights->elements.size());
            for (size_t i = 0; i < w.size(); i++) w[i] = weights->elements[i].asFloat();
            net->set_parameters(w);
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};
      nnClass->methods["save"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 3) throw ArgumentError("save(filepath)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				auto *net = (opennn::NeuralNetwork *)self->fields["ptr"].aspInt();
				net->save(args[2].asString());
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};
		nnClass->methods["load"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 3 || args[2].type != ValueType::STRING) throw ArgumentError("load(filepath)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				auto *net = (opennn::ApproximationNetwork *)self->fields["ptr"].aspInt();
				net->load(args[2].asString());
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};
		static auto dataClass = std::make_shared<ClassObject>("Dataset");
		dataClass->methods["__construct__"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 4 || args[2].type != ValueType::STRING) throw ArgumentError("Dataset(csv_filepath_string, has_header_bool)", l, c);
				bool has_header = args[3].asBool();
				auto *ds = new opennn::Dataset(args[2].asString(), ",", has_header, false);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				self->fields["ptr"] = Value::pInt(ds);
				return args[0];
			}),
			.access = AccessLevel::PUBLIC
		};
		dataClass->methods["__destruct__"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				if (self->fields.find("ptr") != self->fields.end()) {
					auto *ds = (opennn::Dataset *)self->fields["ptr"].aspInt();
					delete ds;
				}
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};
      dataClass->methods["split"] = ClassObject::MethodInfo{
         .func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
            if (args.size() < 5) throw ArgumentError("split(train_pct, val_pct, test_pct)", l, c);
            auto *self = static_cast<InstanceObject *>(args[0].ref.get());
            auto *ds = (opennn::Dataset *)self->fields["ptr"].aspInt();
            ds->split_samples_random(args[2].asFloat(), args[3].asFloat(), args[4].asFloat());
            return Value::None();
         }),
         .access = AccessLevel::PUBLIC
      };
      dataClass->methods["scale_inputs"] = ClassObject::MethodInfo{
         .func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
            auto *self = static_cast<InstanceObject *>(args[0].ref.get());
            auto *ds = (opennn::Dataset *)self->fields["ptr"].aspInt();
            ds->scale_features("Input");
            return Value::None();
         }),
         .access = AccessLevel::PUBLIC
      };
		dataClass->methods["set_columns"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 4) throw ArgumentError("set_columns(input_count, target_count)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				auto *ds = (opennn::Dataset *)self->fields["ptr"].aspInt();
				long inputs = (long)args[2].asInt();
				long targets = (long)args[3].asInt();
				long total_vars = (long)ds->get_variables().size();
            for (long i = 0; i < total_vars; i++) {
               if (i < inputs) ds->set_variable_role(i, "Input");
					else if (i < inputs + targets) ds->set_variable_role(i, "Target");
					else ds->set_variable_role(i, "Unused");
            }
            return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};
		static auto optClass = std::make_shared<ClassObject>("Optimizer");
		optClass->staticFields["GRADIENT_DESCENT"] = Value::String("StochasticGradientDescent");
		optClass->staticFields["QUASI_NEWTON"] = Value::String("QuasiNewtonMethod");
		optClass->staticFields["LEVENBERG_MARQUARDT"] = Value::String("LevenbergMarquardtAlgorithm");
		optClass->staticFields["ADAM"] = Value::String("AdaptiveMomentEstimation");
      optClass->staticFields["MEAN_SQUARED_ERROR"] = Value::String("MeanSquaredError");
      optClass->staticFields["NORMALIZED_SQUARED_ERROR"] = Value::String("NormalizedSquaredError");
      optClass->staticFields["CROSS_ENTROPY_ERROR"] = Value::String("CrossEntropyError");
		optClass->methods["__construct__"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 4) throw ArgumentError("Optimizer(network_instance, dataset_instance)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				auto *netInst = static_cast<InstanceObject *>(args[2].ref.get());
				auto *dataInst = static_cast<InstanceObject *>(args[3].ref.get());
				auto *net = (opennn::NeuralNetwork *)netInst->fields["ptr"].aspInt();
				auto *ds = (opennn::Dataset *)dataInst->fields["ptr"].aspInt();
				auto *strategy = new opennn::TrainingStrategy(net, ds);
				self->fields["ptr"] = Value::pInt(strategy);
				return args[0]; 
			}),
			.access = AccessLevel::PUBLIC
		};
      optClass->methods["set_learning_rate"] = ClassObject::MethodInfo{
         .func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
            if (args.size() < 3) throw ArgumentError("set_learning_rate(float)", l, c);
            auto *self = static_cast<InstanceObject *>(args[0].ref.get());
            auto *strategy = (opennn::TrainingStrategy *)self->fields["ptr"].aspInt();
            opennn::Optimizer* opt = strategy->get_optimization_algorithm();
            double lr = args[2].asFloat();
            if (auto adam = dynamic_cast<opennn::AdaptiveMomentEstimation *>(opt)) {
                  adam->set_learning_rate(lr);
            }
            else if (auto gd = dynamic_cast<opennn::StochasticGradientDescent *>(opt)) {
                  gd->set_initial_learning_rate(lr);
            }
            else {
                  throw RuntimeError("Current optimizer does not support manual learning rates!", l, c);
            }
            return Value::None();
         }),
         .access = AccessLevel::PUBLIC
      };
      optClass->methods["set_loss"] = ClassObject::MethodInfo{
         .func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
            if (args.size() < 3 || args[2].type != ValueType::STRING) throw ArgumentError("set_loss(string)", l, c);
            auto *self = static_cast<InstanceObject *>(args[0].ref.get());
            auto *strategy = (opennn::TrainingStrategy *)self->fields["ptr"].aspInt();
            strategy->set_loss(args[2].asString());
            return Value::None();
         }),
         .access = AccessLevel::PUBLIC
      };
		optClass->methods["set_epochs"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 3) throw ArgumentError("set_epochs(int)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				auto *strategy = (opennn::TrainingStrategy *)self->fields["ptr"].aspInt();
				long epochs = (long)args[2].asInt();
				OptimizerHack::set_epochs(strategy->get_optimization_algorithm(), epochs);
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};
		optClass->methods["train"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				auto *strategy = (opennn::TrainingStrategy *)self->fields["ptr"].aspInt();
				auto results = strategy->train();
				return Value::Float((double)results.get_training_error());
			}),
			.access = AccessLevel::PUBLIC
		};
      optClass->methods["train_with_history"] = ClassObject::MethodInfo{
         .func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
            auto *self = static_cast<InstanceObject *>(args[0].ref.get());
            auto *strategy = (opennn::TrainingStrategy *)self->fields["ptr"].aspInt();
            auto results = strategy->train();
            std::vector<Value> history_vec;
            long hist_size = (long)results.training_error_history.size();
            for (long i = 0; i < hist_size; i++) history_vec.push_back(Value::Float((double)results.training_error_history[i]));
            return Value::Vector(history_vec);
         }),
         .access = AccessLevel::PUBLIC
      };
		optClass->methods["set_algorithm"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				if (args.size() < 3 || args[2].type != ValueType::STRING) throw ArgumentError("set_algorithm(string)", l, c);
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				auto *strategy = (opennn::TrainingStrategy *)self->fields["ptr"].aspInt();
				strategy->set_optimization_algorithm(args[2].asString());
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};
		optClass->methods["__destruct__"] = ClassObject::MethodInfo{
			.func = Value::Native([](const std::vector<Value> &args, int l, int c) -> Value {
				auto *self = static_cast<InstanceObject *>(args[0].ref.get());
				if (self->fields.find("ptr") != self->fields.end()) {
					auto *strategy = (opennn::TrainingStrategy *)self->fields["ptr"].aspInt();
					delete strategy;
				}
				return Value::None();
			}),
			.access = AccessLevel::PUBLIC
		};
		defineClass(nnClass);
		defineClass(dataClass);
		defineClass(optClass);
	};
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
			if (this->modules.count(lib)) {
				this->modules[lib](targetEnv, sym);
			} else {
				throw ImportError("Unknown module '" + lib + "'", 0, 0);
			}
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
			if (this->modules.count(lib)) {
				this->modules[lib](targetEnv, sym);
			} else {
				throw ImportError("Unknown module '" + lib + "'", 0, 0);
			}
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
			if (this->modules.count(lib)) {
				this->modules[lib](targetEnv, sym);
			} else {
				throw ImportError("Unknown module '" + lib + "'", 0, 0);
			}
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
			if (this->modules.count(lib)) {
				this->modules[lib](targetEnv, sym);
			} else {
				throw ImportError("Unknown module '" + lib + "'", 0, 0);
			}
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
				if (this->modules.count(lib)) {
					this->modules[lib](targetEnv, sym);
				} else {
					throw ImportError("Unknown module '" + lib + "'", 0, 0);
				}
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
				if (this->modules.count(lib)) {
					this->modules[lib](targetEnv, sym);
				} else {
					throw ImportError("Unknown module '" + lib + "'", 0, 0);
				}
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
				if (this->modules.count(lib)) {
					this->modules[lib](targetEnv, sym);
				} else {
					throw ImportError("Unknown module '" + lib + "'", 0, 0);
				}
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
				if (this->modules.count(lib)) {
					this->modules[lib](targetEnv, sym);
				} else {
					throw ImportError("Unknown module '" + lib + "'", 0, 0);
				}
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
				if (this->modules.count(lib)) {
					this->modules[lib](targetEnv, sym);
				} else {
					throw ImportError("Unknown module '" + lib + "'", 0, 0);
				}
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
			if (this->modules.count(lib))
				this->modules[lib](targetEnv, sym);
			else
				throw ImportError("Unknown module '" + lib + "'", 0, 0);
		};
		auto dunder = tryCastDunder(args[0], "__count__", l, c, this->env, mRes, iRes);
		if (dunder.first) {
			return dunder.second;
		}
		Value v = dunder.second;
		if (v.type == ValueType::LIST) {
			auto *list = static_cast<ListObject *>(v.ref.get());
			return Value::Int(list->elements.size());
		} else if (v.type == ValueType::STRING)
			return Value::Int(v.asString().size());
		else if (v.type == ValueType::SET) {
			auto *s = static_cast<SetObject *>(v.ref.get());
			return Value::Int(s->elements.size());
		} else if (v.type == ValueType::TUPLE) {
			auto *t = static_cast<TupleObject *>(v.ref.get());
			return Value::Int(t->elements.size());
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
			if (this->modules.count(lib)) {
				this->modules[lib](targetEnv, sym);
			} else {
				throw ImportError("Unknown module '" + lib + "'", 0, 0);
			}
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
			if (this->modules.count(lib)) {
				this->modules[lib](targetEnv, sym);
			} else {
				throw ImportError("Unknown module '" + lib + "'", 0, 0);
			}
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
	env->set("cppCompile", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.size() < 3 || args[0].type != ValueType::STRING || args[2].type != ValueType::DICT) {
			throw TypeError("cppCompile expects (String code, Value return_type, Dict args, String path_to_cpp_compile)", l, c);
		}
		std::string raw_user_code = args[0].asString();
		std::string includes = "";
		std::string logic = "";
		std::istringstream stream(raw_user_code);
		std::string line;
		while (std::getline(stream, line)) {
			size_t start = line.find_first_not_of(" \t");
			if (start != std::string::npos && line.compare(start, 8, "#include") == 0) {
				includes += line + "\n";
			} else {
				logic += line + "\n";
			}
		}
		std::string user_code = logic;
		std::string cache_key = raw_user_code;
		auto *dict = static_cast<DictObject *>(args[2].ref.get());
		std::vector<std::string> sorted_keys;
		for (const auto &[key, val] : dict->items) {
			sorted_keys.push_back(key.asString());
		}
		std::sort(sorted_keys.begin(), sorted_keys.end());
		std::function<std::string(const Value &, int, int)> getDeepCppType = [&](const Value &v, int l, int c) -> std::string {
			if (v.type == ValueType::INT)
				return "long long";
			if (v.type == ValueType::FLOAT)
				return "double";
			if (v.type == ValueType::BOOL)
				return "bool";
			if (v.type == ValueType::STRING)
				return "std::string";
			if (v.type == ValueType::LIST || v.type == ValueType::VECTOR || v.type == ValueType::TUPLE) {
				const auto &elems = (v.type == ValueType::LIST) ? static_cast<ListObject *>(v.ref.get())->elements : (v.type == ValueType::VECTOR) ? static_cast<VectorObject *>(v.ref.get())->elements
																																															  : static_cast<TupleObject *>(v.ref.get())->elements;
				if (elems.empty())
					throw TypeError("Cannot deduce deep C++ type from empty container", l, c);
				return "std::vector<" + getDeepCppType(elems[0], l, c) + ">";
			}
			if (v.type == ValueType::DICT) {
				auto *dictObj = static_cast<DictObject *>(v.ref.get());
				if (dictObj->items.empty())
					throw TypeError("Cannot deduce deep C++ type from empty Dict", l, c);
				auto first_pair = dictObj->items.begin();
				return "std::unordered_map<" + getDeepCppType(first_pair->first, l, c) + ", " + getDeepCppType(first_pair->second, l, c) + ">";
			}
			if (v.type == ValueType::SET) {
				auto *setObj = static_cast<SetObject *>(v.ref.get());
				if (setObj->elements.empty())
					throw TypeError("Cannot deduce deep C++ type from empty Set", l, c);
				auto first_it = setObj->elements.begin();
				return "std::unordered_set<" + getDeepCppType(*first_it, l, c) + ">";
			}
			throw TypeError("Unsupported nested type for C++ FFI translation", l, c);
		};

		for (const auto &k : sorted_keys) {
			Value val = dict->items.at(Value::String(k));
			cache_key += "|" + k + ":" + getDeepCppType(val, l, c);
		}
		static std::unordered_map<std::string, void *> ffi_cache;
		typedef Value (*MacroFunc)(Value *);
		MacroFunc exec_func = nullptr;
		if (ffi_cache.find(cache_key) != ffi_cache.end()) {
			// CACHED
			exec_func = (MacroFunc)ffi_cache[cache_key];
		} else {
			// --- NOT CACHED: MUST COMPILE ---
			std::stringstream cpp;
			cpp << "#include <"<< args[3].asString() <<">\n";
			cpp << "#include <string>\\n";
			cpp << "#include <type_traits>\n\n";
			cpp << includes << "\n";
			// 1. Unpackers: y-- Value -> Raw C++ (Using Struct Specialization for safe recursion)
			cpp << "template<typename T> struct Unboxer;\n";
			cpp << "template<> struct Unboxer<long long> { static long long get(const Value& v) { return v.asInt(); } };\n";
			cpp << "template<> struct Unboxer<int> { static int get(const Value& v) { return (int)v.asInt(); } };\n";
			cpp << "template<> struct Unboxer<size_t> { static int get(const Value& v) { return (size_t)v.asInt(); } };\n";
			cpp << "template<> struct Unboxer<double> { static double get(const Value& v) { return v.asFloat(); } };\n";
			cpp << "template<> struct Unboxer<float> { static float get(const Value& v) { return (float)v.asFloat(); } };\n";
			cpp << "template<> struct Unboxer<bool> { static bool get(const Value& v) { return v.asBool(); } };\n";
			cpp << "template<> struct Unboxer<std::string> { static std::string get(const Value& v) { return v.asString(); } };\n\n";

			cpp << "template<typename T> struct Unboxer<std::vector<T>> {\n";
			cpp << "    static std::vector<T> get(const Value& v) {\n";
			cpp << "        std::vector<T> res;\n";
			cpp << "        if (v.type == ValueType::LIST) {\n";
			cpp << "            for(const auto& e : static_cast<ListObject*>(v.ref.get())->elements) res.push_back(Unboxer<T>::get(e));\n";
			cpp << "        } else if (v.type == ValueType::VECTOR) {\n";
			cpp << "            for(const auto& e : static_cast<VectorObject*>(v.ref.get())->elements) res.push_back(Unboxer<T>::get(e));\n";
			cpp << "        } else if (v.type == ValueType::TUPLE) {\n";
			cpp << "            for(const auto& e : static_cast<TupleObject*>(v.ref.get())->elements) res.push_back(Unboxer<T>::get(e));\n";
			cpp << "        }\n";
			cpp << "        return res;\n";
			cpp << "    }\n";
			cpp << "};\n";
			cpp << "template<typename T> T ValueToCpp(const Value& v) { return Unboxer<T>::get(v); }\n\n";

			cpp << "template<typename K, typename V> struct Unboxer<std::unordered_map<K, V>> {\n";
			cpp << "    static std::unordered_map<K, V> get(const Value& v) {\n";
			cpp << "        std::unordered_map<K, V> res;\n";
			cpp << "        for(const auto& [key, val] : static_cast<DictObject*>(v.ref.get())->items) {\n";
			cpp << "            res[Unboxer<K>::get(key)] = Unboxer<V>::get(val);\n";
			cpp << "        }\n";
			cpp << "        return res;\n";
			cpp << "    }\n";
			cpp << "};\n";

			cpp << "template<typename T> struct Unboxer<std::unordered_set<T>> {\n";
			cpp << "    static std::unordered_set<T> get(const Value& v) {\n";
			cpp << "        std::unordered_set<T> res;\n";
			cpp << "        for(const auto& e : static_cast<SetObject*>(v.ref.get())->elements) {\n";
			cpp << "            res.insert(Unboxer<T>::get(e));\n";
			cpp << "        }\n";
			cpp << "        return res;\n";
			cpp << "    }\n";
			cpp << "};\n";

			// Raw C++ -> y-- Value
			cpp << "template<typename T> Value CppToValue(T val);\n";
			cpp << "template<> Value CppToValue(long long v) { return Value::Int(v); }\n";
			cpp << "template<> Value CppToValue(int v) { return Value::Int(v); }\n";
			cpp << "template<> Value CppToValue(size_t v) { return Value::Int(v); }\n";
			cpp << "template<> Value CppToValue(double v) { return Value::Float(v); }\n";
			cpp << "template<> Value CppToValue(float v) { return Value::Float(v); }\n";
			cpp << "template<> Value CppToValue(bool v) { return Value::Bool(v); }\n";
			cpp << "template<> Value CppToValue(std::string v) { return Value::String(v); }\n";
			cpp << "template<> Value CppToValue(const char* v) { return Value::String(std::string(v)); }\n\n";

			cpp << "template<typename T> Value CppToValue(const std::vector<T>& vec) {\n";
			cpp << "    std::vector<Value> res;\n";
			cpp << "    res.reserve(vec.size());\n";
			cpp << "    for(const auto& e : vec) res.push_back(CppToValue(e));\n";
			cpp << "    return Value::List(res);\n";
			cpp << "}\n\n";

			cpp << "template<typename K, typename V> Value CppToValue(const std::unordered_map<K, V>& m) {\n";
			cpp << "    std::unordered_map<Value, Value, ValueHash, ValueEqual> res;\n";
			cpp << "    for(const auto& [k, v] : m) res[CppToValue(k)] = CppToValue(v);\n";
			cpp << "    return Value::Dict(res);\n";
			cpp << "}\n\n";

			cpp << "template<typename T> Value CppToValue(const std::unordered_set<T>& s) {\n";
			cpp << "    std::unordered_set<Value, ValueHash, ValueEqual> res;\n";
			cpp << "    for(const auto& e : s) setAdd(res, CppToValue(e));\n";
			cpp << "    return Value::Set(res);\n";
			cpp << "}\n\n";

			cpp << "template<typename T1, typename T2> Value CppToValue(const std::pair<T1, T2>& p) {\n";
			cpp << "    return Value::Tuple({CppToValue(p.first), CppToValue(p.second)});\n";
			cpp << "}\n\n";

			cpp << "extern \"C\" {\n";
			cpp << "#ifdef _WIN32\n__declspec(dllexport)\n#endif\n";
			cpp << "Value y_macro_exec(Value* args_array) {\n";
			// RECURSIVE Type Sniffer
			// Unpack variables based on SORTED order
			for (size_t i = 0; i < sorted_keys.size(); i++) {
				std::string var_name = sorted_keys[i];
				Value val = dict->items.at(Value::String(var_name));
				// Sniff the deep type signature
				std::string cpp_type = getDeepCppType(val, l, c);
				// Emit exactly ONE C++ line to invoke the recursive Unboxer template
				cpp << "    " << cpp_type << " " << var_name << " = ValueToCpp<" << cpp_type << ">(args_array[" << i << "]);\n";
			}
			cpp << "\n    auto user_logic = [&]() {\n";
			cpp << "        " << user_code << "\n";
			cpp << "    };\n\n";
			cpp << "    using RetType = decltype(user_logic());\n";
			cpp << "    if constexpr (std::is_same_v<RetType, void>) {\n";
			cpp << "        user_logic();\n";
			cpp << "        return Value::None();\n";
			cpp << "    } else {\n";
			cpp << "        return CppToValue(user_logic());\n";
			cpp << "    }\n";
			cpp << "}\n}\n";
			std::string current_dir = std::filesystem::current_path().string();
			std::string hash_str = std::to_string(std::hash<std::string>{}(cache_key));
			std::string debug_flag = "";
#ifdef VM_DEBUG_MODE // NEVER USE
			debug_flag = " -DVM_DEBUG_MODE ";
#endif
#ifdef _WIN32
            char exePathStr[MAX_PATH];
            GetModuleFileNameA(NULL, exePathStr, MAX_PATH);
            std::filesystem::path exePath(exePathStr);
            std::string exeDir = exePath.parent_path().string();
            std::string cpp_file = current_dir + "\\y_macro_" + hash_str + ".cpp";
            std::string lib_file = current_dir + "\\y_macro_" + hash_str + ".dll";
            std::string implib_file = exeDir + "\\y_lang.a";
            std::string cmd = "g++ -std=c++17 -shared -O3 -fopenmp " + debug_flag + 
                              "-I\"" + current_dir + "\" " + 
                              "-I/c/raylib-src/raylib-5.5/src " + 
                              "\"" + cpp_file + "\" -o \"" + lib_file + "\" " + 
                              "\"" + implib_file + "\" " + 
                              "-Wl,--allow-multiple-definition 2>&1";
#else
            std::string cpp_file = "/tmp/y_macro_" + hash_str + ".cpp";
            std::string lib_file = "/tmp/y_macro_" + hash_str + ".so";
            std::string cmd = "g++ -std=c++17 -shared -fPIC -O3 -fopenmp " + debug_flag + "-I\"" + current_dir + "\" " + cpp_file + " -o " + lib_file + " -lraylib 2>&1";
#endif
            std::ofstream out(cpp_file);
            out << cpp.str();
            out.flush();
            out.close();
            if (system(cmd.c_str()) != 0)
                throw RuntimeError("C++ FFI Compilation failed!", l, c);
#ifdef _WIN32
            SetDllDirectoryA(exeDir.c_str());
            HINSTANCE handle = LoadLibraryA(lib_file.c_str());
            SetDllDirectoryA(NULL); // Reset immediately for security
            
            if (!handle) {
                DWORD err = GetLastError();
                throw RuntimeError("Failed to load DLL. Windows Error Code: " + std::to_string(err), l, c);
            }
            exec_func = (MacroFunc)GetProcAddress(handle, "y_macro_exec");
#else
            void *handle = dlopen(lib_file.c_str(), RTLD_NOW);
            if (!handle)
                throw RuntimeError(std::string("Failed to load SO: ") + dlerror(), l, c);
            exec_func = (MacroFunc)dlsym(handle, "y_macro_exec");
#endif
			if (!exec_func)
				throw RuntimeError("Failed to find execution hook", l, c);
			ffi_cache[cache_key] = (void *)exec_func;
		}
		std::vector<Value> packed_args;
		for (const auto &k : sorted_keys) {
			packed_args.push_back(dict->items.at(Value::String(k)));
		}
		Value raw_result = exec_func(packed_args.data());
		ValueType expected = args[1].type;
		if (expected == ValueType::NONE || expected == ValueType::NOTYPE || expected == raw_result.type) {
			return raw_result;
		}
		if (expected == ValueType::VECTOR && raw_result.type == ValueType::LIST) {
			return Value::Vector(static_cast<ListObject *>(raw_result.ref.get())->elements);
		}
		if (expected == ValueType::LIST && raw_result.type == ValueType::VECTOR) {
			return Value::List(static_cast<VectorObject *>(raw_result.ref.get())->elements);
		}
		if (expected == ValueType::SET && raw_result.type == ValueType::LIST) {
			auto &elems = static_cast<ListObject *>(raw_result.ref.get())->elements;
			std::unordered_set<Value, ValueHash, ValueEqual> s(elems.begin(), elems.end());
			return Value::Set(s);
		}
		if (expected == ValueType::TUPLE && raw_result.type == ValueType::LIST) {
			return Value::Tuple(static_cast<ListObject *>(raw_result.ref.get())->elements);
		}
		return raw_result;
	}),
		false);
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
				if (this->modules.count(lib))
					this->modules[lib](targetEnv, sym);
				else
					throw ImportError("Unknown module '" + lib + "'", 0, 0);
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
	std::unordered_set<const HeapObject *> seen;
	for (size_t i = 0; i < args.size(); i++) {
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
		if (this->modules.count(lib)) {
			this->modules[lib](targetEnv, sym);
		} else {
			throw ImportError("Unknown module '" + lib + "'", 0, 0);
		}
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
					long long r = d.iVal * d.iVal;
					bool ovf = (d.iVal != 0 && r / d.iVal != d.iVal);
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
					long long r = d.iVal * d.iVal;
					bool ovf = (d.iVal != 0 && r / d.iVal != d.iVal);
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
					long long r = valU.iVal * valV.iVal;
					bool ovf = (valU.iVal != 0 && r / valU.iVal != valV.iVal);
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
					long long r = valV.iVal * valV.iVal;
					bool ovf = (valV.iVal != 0 && r / valV.iVal != valV.iVal);
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
					long long r = x.iVal * y.iVal;
					bool ovf = (x.iVal != 0 && r / x.iVal != y.iVal);
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
					long long r = a.iVal * b.iVal;
					bool ovf = (a.iVal != 0 && r / a.iVal != b.iVal);
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


	void VM::run(Chunk &chunk) {
		CallFrame mainFrame;
		mainFrame.function = nullptr;
		mainFrame.ip = chunk.code.data();
		mainFrame.basePointer = stack.size();
		frames.push_back(mainFrame);
		frame = &frames.back();
		ip = frame->ip;
		Value pendingReturn = Value::NoType();
		bool isReturning = false;
		Value pendingError = Value::NoType();
		bool isHandlingError = false;
		int line = 0;
		int col = 0;
#ifdef USE_COMPUTED_GOTOS
		static void *dispatch_table[] = {
			&&TARGET_OP_CONSTANT, &&TARGET_OP_CONSTANT_LONG, &&TARGET_OP_TRUE,
			&&TARGET_OP_FALSE, &&TARGET_OP_NONE, &&TARGET_OP_NOTYPE,

			// Variables & Scope
			&&TARGET_OP_DEFINE_VAR, &&TARGET_OP_GET_VAR, &&TARGET_OP_SET_VAR,
			&&TARGET_OP_DEEP_COPY, &&TARGET_OP_REF_LOCAL, &&TARGET_OP_DEFINE_REF,
			&&TARGET_OP_REF_VAR, &&TARGET_OP_REF_INDEX, &&TARGET_OP_SET_REF,
			&&TARGET_OP_SHALLOW_COPY, &&TARGET_OP_MULTI_SET,
			&&TARGET_OP_GET_LOCAL, &&TARGET_OP_SET_LOCAL, &&TARGET_OP_INC_LOCAL,
			&&TARGET_OP_SET_FLAGS, &&TARGET_OP_REF_PROPERTY, &&TARGET_OP_DELETE,

			// Arithmetic & Logic
			&&TARGET_OP_ADD, &&TARGET_OP_SUB, &&TARGET_OP_MUL, &&TARGET_OP_DIV, &&TARGET_OP_FAST_IADD, &&TARGET_OP_FAST_ISUB, &&TARGET_OP_FAST_IMUL, &&TARGET_OP_FAST_IDIV, &&TARGET_OP_FAST_FADD, &&TARGET_OP_FAST_FSUB, &&TARGET_OP_FAST_FMUL, &&TARGET_OP_FAST_FDIV, &&TARGET_OP_FAST_SADD, &&TARGET_OP_CTLE_TICK,
			&&TARGET_OP_FLOOR_DIV, &&TARGET_OP_MOD, &&TARGET_OP_POW,
			&&TARGET_OP_IADD, &&TARGET_OP_ISUB, &&TARGET_OP_IMUL,
			&&TARGET_OP_IDIV, &&TARGET_OP_IFLOOR_DIV, &&TARGET_OP_IMOD,
			&&TARGET_OP_IPOW, &&TARGET_OP_DUP, &&TARGET_OP_DUP_2, &&TARGET_OP_EQ,
			&&TARGET_OP_NEQ, &&TARGET_OP_LT, &&TARGET_OP_GT, &&TARGET_OP_LTE,
			&&TARGET_OP_GTE, &&TARGET_OP_COLON, &&TARGET_OP_STRICT_NEQ,
			&&TARGET_OP_NOT, &&TARGET_OP_AND, &&TARGET_OP_OR, &&TARGET_OP_XOR,
			&&TARGET_OP_IS, &&TARGET_OP_IN, &&TARGET_OP_IS_NOT,
			&&TARGET_OP_STRICT_EQ, &&TARGET_OP_IS_IN, &&TARGET_OP_IS_NOT_IN,
			&&TARGET_OP_NXOR, &&TARGET_OP_NAND, &&TARGET_OP_NOR,
			&&TARGET_OP_NEGATE, &&TARGET_OP_INCREMENT, &&TARGET_OP_DECREMENT,
			&&TARGET_OP_BITWISE_AND, &&TARGET_OP_BITWISE_OR, &&TARGET_OP_BITWISE_XOR,
			&&TARGET_OP_BITWISE_NOT, &&TARGET_OP_BITWISE_SHL, &&TARGET_OP_BITWISE_SHR,
			&&TARGET_OP_PRE_INCREMENT, &&TARGET_OP_PRE_DECREMENT,

			// Containers
			&&TARGET_OP_BUILD_LIST, &&TARGET_OP_BUILD_TUPLE,
			&&TARGET_OP_BUILD_SET, &&TARGET_OP_BUILD_DICT,
			&&TARGET_OP_UNPACK_DICT, &&TARGET_OP_BUILD_RANGE,
			&&TARGET_OP_BUILD_VECTOR, &&TARGET_OP_BUILD_FSTRING,
			&&TARGET_OP_BUILD_FILE, &&TARGET_OP_BUILD_SLICE,

			// OOP
			&&TARGET_OP_CLASS, &&TARGET_OP_METHOD, &&TARGET_OP_GET_PROPERTY,
			&&TARGET_OP_SET_PROPERTY, &&TARGET_OP_INC_PROPERTY, &&TARGET_OP_CLASS_FIELD, &&TARGET_OP_SUPER,

			// Comprehension
			&&TARGET_OP_LIST_APPEND, &&TARGET_OP_SET_ADD, &&TARGET_OP_DICT_SET,
			&&TARGET_OP_LIST_TO_TUPLE, &&TARGET_OP_LIST_TO_VECTOR,

			// Access & Calls
			&&TARGET_OP_GET_INDEX, &&TARGET_OP_SET_INDEX, &&TARGET_OP_INC_INDEX, &&TARGET_OP_INVOKE,
			&&TARGET_OP_CALL,

			// Control Flow
			&&TARGET_OP_JUMP, &&TARGET_OP_JUMP_IF_FALSE, &&TARGET_OP_LOOP,
			&&TARGET_OP_RETURN, &&TARGET_OP_TO_STREAM, &&TARGET_OP_JUMP_IF_NOT_LT,
			&&TARGET_OP_BREAK, &&TARGET_OP_CONTINUE, &&TARGET_OP_SKIP,
			&&TARGET_OP_OMIT, &&TARGET_OP_FOR_ITER, &&TARGET_OP_SKIP_ITER,
			&&TARGET_OP_SWITCH_TABLE,

			// Errors & Systems
			&&TARGET_OP_THROW, &&TARGET_OP_ASSERT, &&TARGET_OP_IMPORT,
			&&TARGET_OP_POP, &&TARGET_OP_DEBUG_NAME, &&TARGET_OP_REPL_PRINT, &&TARGET_OP_TRY_ENTER,
			&&TARGET_OP_TRY_EXIT, &&TARGET_OP_CATCH, &&TARGET_OP_RETHROW,
			&&TARGET_OP_END_FINALLY};
#endif
		auto invokeBinaryDunder = [&](Value a, Value b, const string &leftOp, const string &rightOp, int line, int col) -> bool {
			if (a.type == ValueType::INSTANCE) {
				auto *instanceA = static_cast<InstanceObject *>(a.ref.get());
				ClassObject *clsA = instanceA->klass;
				ClassObject::MethodInfo *methodA = nullptr;
				for (auto *ancestor : clsA->mro) {
					if (ancestor->methods.count(leftOp)) {
						methodA = &ancestor->methods[leftOp];
						break;
					}
				}
				if (methodA) {
					stack.push_back(a);
					Value objVal;
					objVal.type = ValueType::CLASS;
					objVal.ref =
						std::shared_ptr<HeapObject>(clsA, [](HeapObject *) {});
					stack.push_back(objVal);
					stack.push_back(b);
					callValue(methodA->func, 3, line, col);
					return true;
				}
			}
			if (b.type == ValueType::INSTANCE) {
				auto *instanceB = static_cast<InstanceObject *>(b.ref.get());
				ClassObject *clsB = instanceB->klass;
				ClassObject::MethodInfo *methodB = nullptr;
				for (auto *ancestor : clsB->mro) {
					if (ancestor->methods.count(rightOp)) {
						methodB = &ancestor->methods[rightOp];
						break;
					}
				}
				if (methodB) {
					stack.push_back(b);
					Value objVal;
					objVal.type = ValueType::CLASS;
					objVal.ref =
						std::shared_ptr<HeapObject>(clsB, [](HeapObject *) {});
					stack.push_back(objVal);
					stack.push_back(a);
					callValue(methodB->func, 3, line, col);
					return true;
				}
			}
			return false;
		};
		auto checkFieldAccess = [&](ClassObject *targetClass, const string &name, int line, int col) {
			if (targetClass->fieldAccess.count(name)) {
				AccessLevel access = targetClass->fieldAccess[name];
				if (access != AccessLevel::PUBLIC) {
					bool allowed = false;
					if (frame->function && frame->function->owner) {
						ClassObject *callerCls = frame->function->owner;
						if (access == AccessLevel::PRIVATE &&
							 callerCls == targetClass)
							allowed = true;
						else if (access == AccessLevel::PROTECTED) {
							for (auto *ancestor : callerCls->mro) {
								if (ancestor == targetClass) {
									allowed = true;
									break;
								}
							}
						}
					}
					if (!allowed)
						throw RuntimeError(
							"Cannot access private/protected field '" + name + "'",
							line, col);
				}
			}
		};
		auto invokeUnaryDunder = [&](Value a, const string &opName, int line, int col) -> bool {
			if (a.type == ValueType::INSTANCE) {
				auto *instanceA = static_cast<InstanceObject *>(a.ref.get());
				ClassObject *clsA = instanceA->klass;
				for (auto *ancestor : clsA->mro) {
					if (ancestor->methods.count(opName)) {
						stack.push_back(a);
						Value objVal;
						objVal.type = ValueType::CLASS;
						objVal.ref = std::shared_ptr<HeapObject>(clsA, [](HeapObject *) {});
						stack.push_back(objVal);
						callValue(ancestor->methods[opName].func, 2, line, col);
						return true;
					}
				}
			}
			return false;
		};
		while (true) {
			Chunk *currentChunk = frame->function ? frame->function->chunk : &chunk;
			try {
				OpCode instruction;
#ifdef USE_COMPUTED_GOTOS
				DISPATCH();
#else
			loop_start:
				if (gc_alloc_count > 50000) { gc_collect(); gc_alloc_count = 0; }
				currentChunk = frame->function ? frame->function->chunk : &chunk;
				RUN_DEBUGGER();
				instruction = static_cast<OpCode>(*ip++);
				int offset = (int)(ip - currentChunk->code.data());
				line = currentChunk->lines[offset - 1];
				col = currentChunk->columns[offset - 1];
				switch (instruction)
#endif
				{
					OP(OP_REPL_PRINT) : {
						{
							Value v = pop();
							if (printOutput /*v.type != ValueType::NOTYPE && v.type != ValueType::NONE*/) {
								std::cout << "-> " << valueToString(v) << "\n";
							}
							DISPATCH();
						}
					}
					OP(OP_CLASS) : {
						{
							uint8_t parentCount = *ip++;
							string name = pop().asString();
							Value classVal = Value::Class(name);
							auto *newClassObj =
								static_cast<ClassObject *>(classVal.ref.get());
							for (int i = 0; i < parentCount; i++) {
								Value pVal = pop();
								if (pVal.type != ValueType::CLASS)
									throw RuntimeError("Superclass must be a class", line,
										col);
								newClassObj->parents.push_back(pVal);
							}
							std::reverse(newClassObj->parents.begin(),
								newClassObj->parents.end());
							newClassObj->computeMRO();
							stack.push_back(classVal);
						}
						DISPATCH();
					}
					OP(OP_SUPER) : {
						{
							Value self = stack[frame->basePointer];
							if (self.type != ValueType::INSTANCE)
								throw RuntimeError("super() must be called on an instance",
									line, col);
							FunctionObject *currentFunc = frame->function;
							if (!currentFunc || !currentFunc->owner) {
								throw RuntimeError(
									"super() used in a function that is not a method.",
									line, col);
							}
							ClassObject *definingClass = currentFunc->owner;
							auto *instance = static_cast<InstanceObject *>(self.ref.get());
							ClassObject *trueClass = instance->klass;
							ClassObject *superTarget = nullptr;
							bool foundDefining = false;
							for (auto *ancestor : trueClass->mro) {
								if (foundDefining) {
									superTarget = ancestor;
									break;
								}
								if (ancestor == definingClass)
									foundDefining = true;
							}
							if (!superTarget)
								throw RuntimeError(
									"super(): No superclass found (reached top of MRO).",
									line, col);
							Value v;
							v.type = ValueType::SUPER;
							v.ref = make_shared<SuperObject>(self, superTarget);
							stack.push_back(v);
						}
						DISPATCH();
					}
					OP(OP_METHOD) : {
						{
							uint8_t accessByte = *ip++;
							AccessLevel access = (AccessLevel)accessByte;
							Value funcVal = pop();
							Value classVal = stack.back();
							if (classVal.type != ValueType::CLASS)
								throw RuntimeError("Cannot define method on non-class",
									line, col);
							auto *cls = static_cast<ClassObject *>(classVal.ref.get());
							auto *func = static_cast<FunctionObject *>(funcVal.ref.get());
							func->owner = cls;
							if (cls->methods.count(func->name)) {
								Value existing = cls->methods[func->name].func;
								if (existing.type == ValueType::FUNCTION || existing.type == ValueType::NATIVE_FUNCTION) {
									auto ovObj = std::make_shared<OverloadObject>(existing);
									ovObj->overloads.push_back(funcVal);
									Value ovVal;
									ovVal.type = ValueType::OVERLOAD;
									ovVal.ref = ovObj;
									cls->methods[func->name] = {ovVal, access};
								} else if (existing.type == ValueType::OVERLOAD) {
									auto ovObj = std::make_shared<OverloadObject>(existing);
									ovObj->overloads.push_back(funcVal);
								} else {
									cls->methods[func->name] = {funcVal, access};
								}
							} else {
								cls->methods[func->name] = {funcVal, access};
							}
						}
						DISPATCH();
					}
					OP(OP_GET_PROPERTY) : {
						{
							uint8_t nameIdx = *ip++;
							string name = currentChunk->constants[nameIdx].asString();
							Value obj = pop();
							if (name == "__mro__" || name == "__var_count__" || name == "__var_names__" ||
								 name == "__function_count__" || name == "__function_names__" || name == "__var_reverse_pairs__" ||
								 name == "__all_count__" || name == "__all_names__" || name == "__var_values__" || name == "__var_pairs__") {
								ClassObject *cls = nullptr;
								InstanceObject *inst = nullptr;
								if (obj.type == ValueType::INSTANCE) {
									inst = static_cast<InstanceObject *>(obj.ref.get());
									cls = inst->klass;
								} else if (obj.type == ValueType::CLASS) {
									cls = static_cast<ClassObject *>(obj.ref.get());
								}
								if (name == "__mro__") {
									auto mroList = std::make_shared<ListObject>();
									if (cls) {
										for (auto *ancestor : cls->mro) {
											mroList->elements.push_back(Value::String(ancestor->name));
										}
									}
									stack.push_back(Value::List(mroList->elements));
								} else {
									bool wantsNames = (name.find("_names__") != string::npos);
									bool wantsValues = (name.find("_values__") != string::npos);
									bool wantsPairs = (name.find("_pairs__") != string::npos);
									bool wantsReverse = (name.find("_reverse_") != string::npos);
									bool wantsVars = (name.find("__var_") != string::npos || name.find("__all_") != string::npos);
									bool wantsFuncs = (name.find("__function_") != string::npos || name.find("__all_") != string::npos);
									std::unordered_set<std::string> seen;
									std::unordered_set<Value, ValueHash, ValueEqual> seenVal;
									auto nameList = std::make_shared<SetObject>();
									auto valueList = std::make_shared<SetObject>();
									auto pairDict = std::make_shared<DictObject>();
									auto addName = [&](const std::string &n) {
										if (seen.insert(n).second && wantsNames) {
											nameList->elements.insert(Value::String(n));
										}
									};
									auto addVal = [&](const Value &n) {
										if (seenVal.insert(n).second && wantsValues) {
											valueList->elements.insert(n);
										}
									};
									auto addPair = [&](const std::string &n, const Value &v) {
										if (seen.insert(n).second && seenVal.insert(v).second && wantsPairs) {
											pairDict->items[Value::String(n)] = v;
										}
									};
									auto addReversePair = [&](const std::string &n, const Value &v) {
										if (seen.insert(n).second && seenVal.insert(v).second && wantsPairs && wantsReverse) {
											pairDict->items[v] = Value::String(n);
										}
									};
									if (cls) {
										if (wantsVars) {
											if (inst) {
												for (const auto &pair : inst->fields) {
													AccessLevel acc = cls->fieldAccess.count(pair.first) ? cls->fieldAccess[pair.first] : AccessLevel::PUBLIC;
													if (acc == AccessLevel::PUBLIC) {
														if (wantsValues) {
															addVal(pair.second);
														} else if (wantsPairs) {
															if (wantsReverse)
																addReversePair(pair.first, pair.second);
															else
																addPair(pair.first, pair.second);
														} else {
															addName(pair.first);
														}
													}
												}
											}
											for (auto *ancestor : cls->mro) {
												for (const auto &pair : ancestor->staticFields) {
													AccessLevel acc = ancestor->fieldAccess.count(pair.first) ? ancestor->fieldAccess[pair.first] : AccessLevel::PUBLIC;
													if (acc == AccessLevel::PUBLIC) {
														if (wantsValues) {
															addVal(pair.second);
														} else if (wantsPairs) {
															if (wantsReverse)
																addReversePair(pair.first, pair.second);
															else
																addPair(pair.first, pair.second);
														} else {
															addName(pair.first);
														}
													}
												}
											}
										}
										if (wantsFuncs) {
											for (auto *ancestor : cls->mro) {
												for (const auto &pair : ancestor->methods) {
													if (pair.second.access == AccessLevel::PUBLIC) {
														addName(pair.first);
													}
												}
											}
										}
									}
									if (wantsNames) {
										stack.push_back(Value::Set(nameList->elements));
									} else if (wantsValues) {
										stack.push_back(Value::Set(valueList->elements));
									} else if (wantsPairs) {
										stack.push_back(Value::Dict(pairDict->items));
									} else {
										stack.push_back(Value::Int(seen.size()));
									}
								}
							} else {
								std::shared_ptr<HeapObject> lifeline = obj.ref;
								if (obj.type == ValueType::REFERENCE) {
									if (obj.get_ptr_safe() == nullptr) {
										throw RuntimeError("Null pointer dereference.", line, col);
									}
									obj = *(obj.ptr);
									if (obj.type == ValueType::INSTANCE)
										lifeline = obj.ref;
								}
								if (obj.type == ValueType::INSTANCE) {
									auto *instance =
										static_cast<InstanceObject *>(obj.ref.get());
									checkFieldAccess(instance->klass, name, line, col);
									if (instance->fields.count(name))
										stack.push_back(instance->fields[name]);
									else {
										ClassObject *cls = instance->klass;
										bool found = false;
										for (auto *ancestor : cls->mro) {
											if (ancestor->methods.count(name)) {
												stack.push_back(ancestor->methods[name].func);
												found = true;
												break;
											}
											if (ancestor->staticFields.count(name)) {
												stack.push_back(ancestor->staticFields[name]);
												found = true;
												break;
											}
										}
										if (!found)
											throw AttributeError("Instance of '" + cls->name +
																			"' has no attribute '" +
																			name + "'",
												line, col);
									}
								} else if (obj.type == ValueType::CLASS) {
									auto *cls = static_cast<ClassObject *>(obj.ref.get());
									bool found = false;
									for (auto *ancestor : cls->mro) {
										if (ancestor->staticFields.count(name)) {
											stack.push_back(ancestor->staticFields[name]);
											found = true;
											break;
										}
										if (ancestor->methods.count(name)) {
											stack.push_back(ancestor->methods[name].func);
											found = true;
											break;
										}
									}
									if (!found)
										throw AttributeError("Class '" + cls->name +
																		"' has no attribute '" + name +
																		"'",
											line, col);
								} else {
									bool foundItem = false;
									if (obj.type == ValueType::DICT) {
										auto *dictObj = static_cast<DictObject *>(obj.ref.get());
										Value key = Value::String(name);
										if (dictObj->items.count(key)) {
											stack.push_back(dictObj->items[key]);
											foundItem = true;
										}
									}
									
									if (!foundItem) {
										Value boundObj = obj;
										std::string boundName = name;
										auto *vm = this;
										auto boundFunc = [boundObj, boundName, vm](const std::vector<Value>& args, int l, int c) -> Value {
											std::vector<ValueExpr *> dummyArgs(args.size());
											for (size_t i = 0; i < args.size(); i++) {
												dummyArgs[i] = new ValueExpr(args[i]);
											}
											ValueExpr *dummyObject = new ValueExpr(boundObj);
											std::vector<Expr *> exprArgs(dummyArgs.begin(), dummyArgs.end());
											MethodCallExpr mockAST(dummyObject, boundName, exprArgs);
											mockAST.line = l;
											mockAST.col = c;

											if (!vm->methodResolver) {
												delete dummyObject;
												for (auto *a : dummyArgs) delete a;
												throw EnvironmentError("VM methodResolver bridge not initialized.", l, c);
											}

											Value result;
											try {
												result = vm->methodResolver(&mockAST);
											} catch (...) {
												delete dummyObject;
												for (auto *a : dummyArgs) delete a;
												throw;
											}

											delete dummyObject;
											for (auto *a : dummyArgs) delete a;
											return result;
										};
										stack.push_back(Value::Native(boundFunc));
									}
								}
							}
						}
						DISPATCH();
					}
					OP(OP_REF_PROPERTY) : {
						{
							uint8_t nameIdx = *ip++;
							string name = currentChunk->constants[nameIdx].asString();
							Value obj = pop();
							std::shared_ptr<HeapObject> lifeline = obj.ref;
							if (obj.type == ValueType::REFERENCE) {
								if (obj.get_ptr_safe() == nullptr) {
									throw RuntimeError("Null pointer dereference.", line, col);
								}
								obj = *(obj.ptr);
								if (obj.type == ValueType::INSTANCE)
									lifeline = obj.ref;
							}
							if (obj.type == ValueType::INSTANCE) {
								auto *instance = static_cast<InstanceObject *>(obj.ref.get());
								checkFieldAccess(instance->klass, name, line, col);
								if (instance->fields.find(name) == instance->fields.end()) {
									instance->fields[name] = Value::None();
								}
								stack.push_back(Value::FromExisting(std::make_shared<PropRefObject>(std::static_pointer_cast<InstanceObject>(obj.ref), name), ValueType::REFERENCE));
							} else if (obj.type == ValueType::CLASS) {
								auto *cls = static_cast<ClassObject *>(obj.ref.get());
								Value *ptr = &cls->staticFields[name];
								stack.push_back(Value::Reference(ptr, obj.ref));
							} else if (obj.type == ValueType::DICT) {
								Value key = Value::String(name);
								stack.push_back(Value::FromExisting(std::make_shared<DictRefObject>(std::static_pointer_cast<DictObject>(obj.ref), key), ValueType::REFERENCE));
							} else {
								throw TypeError("Cannot take reference of property on non-object", line, col);
							}
						}
						DISPATCH();
					}
					OP(OP_SET_PROPERTY) : {
						{
							uint8_t nameIdx = *ip++;
							string name = currentChunk->constants[nameIdx].asString();
							uint8_t accessByte = *ip++;
							AccessLevel access = (AccessLevel)accessByte;
							Value val = pop();
							Value obj = pop();
							std::shared_ptr<HeapObject> lifeline = obj.ref;
							if (obj.type == ValueType::REFERENCE) {
								if (obj.get_ptr_safe() == nullptr) {
									throw RuntimeError("Null pointer dereference.", line, col);
								}
								obj = *(obj.ptr);
								if (obj.type == ValueType::INSTANCE)
									lifeline = obj.ref;
							}
							if (obj.type == ValueType::INSTANCE) {
								auto *instance = static_cast<InstanceObject *>(obj.ref.get());
								checkFieldAccess(instance->klass, name, line, col);
								instance->fields[name] = val;
								instance->klass->fieldAccess[name] = access;
							} else if (obj.type == ValueType::CLASS) {
								auto *cls = static_cast<ClassObject *>(obj.ref.get());
								cls->staticFields[name] = val;
								cls->fieldAccess[name] = access;
							} else if (obj.type == ValueType::DICT) {
								auto *dictObj = static_cast<DictObject *>(obj.ref.get());
								Value key = Value::String(name);
								dictObj->items[key] = val;
							} else
								throw AttributeError("Cannot set property on non-object",
									line, col);
							stack.push_back(val);
						}
						DISPATCH();
					}
					OP(OP_INC_PROPERTY) : {
						{
							uint8_t nameIdx = *ip++;
							string name = currentChunk->constants[nameIdx].asString();
							uint8_t isInc = *ip++;
							uint8_t isPrefix = *ip++;
							
							Value obj = pop();
							std::shared_ptr<HeapObject> lifeline = obj.ref;
							if (obj.type == ValueType::REFERENCE) {
								if (obj.get_ptr_safe() == nullptr) throw RuntimeError("Null pointer dereference.", line, col);
								obj = *(obj.ptr);
								if (obj.type == ValueType::INSTANCE) lifeline = obj.ref;
							}
							
							Value oldVal;
							if (obj.type == ValueType::INSTANCE) {
								auto *instance = static_cast<InstanceObject *>(obj.ref.get());
								if (instance->fields.find(name) != instance->fields.end()) {
									oldVal = instance->fields[name];
								} else {
									throw AttributeError("Instance has no property '" + name + "'", line, col);
								}
							} else if (obj.type == ValueType::DICT) {
								auto *dictObj = static_cast<DictObject *>(obj.ref.get());
								Value key = Value::String(name);
								if (dictObj->items.find(key) != dictObj->items.end()) {
									oldVal = dictObj->items[key];
								} else {
									throw AttributeError("Dictionary has no property '" + name + "'", line, col);
								}
							} else {
								throw AttributeError("Cannot increment property on this object type", line, col);
							}
							
							Value newVal;
							if (oldVal.type == ValueType::INT) {
								newVal = Value::Int(oldVal.iVal + (isInc ? 1 : -1));
							} else if (oldVal.type == ValueType::FLOAT) {
								newVal = Value::Float(oldVal.fVal + (isInc ? 1.0 : -1.0));
							} else {
								throw TypeError("Increment/Decrement requires numeric property", line, col);
							}
							
							if (obj.type == ValueType::INSTANCE) {
								auto *instance = static_cast<InstanceObject *>(obj.ref.get());
								instance->fields[name] = newVal;
							} else if (obj.type == ValueType::DICT) {
								auto *dictObj = static_cast<DictObject *>(obj.ref.get());
								Value key = Value::String(name);
								dictObj->items[key] = newVal;
							}
							
							stack.push_back(isPrefix ? newVal : oldVal);
						}
						DISPATCH();
					}
					OP(OP_CLASS_FIELD) : {
						{
							uint8_t accessByte = *ip++;
							AccessLevel access = (AccessLevel)accessByte;
							uint8_t nameIdx = *ip++;
							string name = currentChunk->constants[nameIdx].asString();
							Value val = pop();
							Value classVal = stack.back();
							if (classVal.type != ValueType::CLASS)
								throw RuntimeError(
									"Cannot define static field on non-class", line, col);
							auto *cls = static_cast<ClassObject *>(classVal.ref.get());
							cls->fieldAccess[name] = access;
							if (val.type != ValueType::NOTYPE)
								cls->staticFields[name] = val;
						}
						DISPATCH();
					}
					OP(OP_DUP) : {
						{
							stack.push_back(stack.back());
						}
						DISPATCH();
					}
					OP(OP_DUP_2) : {
						{
							if (stack.size() < 2)
								throw UnderflowError("Stack underflow for DUP_2", line,
									col);
							Value top = stack.back();
							Value under = stack[stack.size() - 2];
							stack.push_back(under);
							stack.push_back(top);
						}
						DISPATCH();
					}
					OP(OP_CALL) : {
						{
							if (stack.size() > 65000)
								throw RecursionError(
									"Stack Overflow Maximum call stack size exceeded.",
									line, col);
							uint8_t argCount = *ip++;
							Value callee = pop();
							if (callee.type == ValueType::NATIVE_FUNCTION) {
								vector<Value> args(stack.end() - argCount, stack.end());
								stack.erase(stack.end() - argCount, stack.end());
								auto native =
									static_cast<NativeFunctionObject *>(callee.ref.get());
								Value result = native->func(args, line, 0);
								stack.push_back(result);
							} else if (callee.type == ValueType::CLASS) {
								Value instance = Value::Instance(callee);
								auto *cls = static_cast<ClassObject *>(callee.ref.get());
								if (cls->methods.count(magic_methods_to_string(
										 Magic_Methods::__construct__))) {
									Value initMethod =
										cls->methods[magic_methods_to_string(
															 Magic_Methods::__construct__)]
											.func;
									vector<Value> args;
									for (int i = 0; i < argCount; i++)
										args.push_back(pop());
									std::reverse(args.begin(), args.end());
									stack.push_back(instance);
									stack.push_back(callee);
									for (auto &a : args)
										stack.push_back(a);
									callValue(initMethod, argCount + 2, line, col);
								} else {
									for (int i = 0; i < argCount; i++)
										pop();
									stack.push_back(instance);
								}
							} else if (callee.type == ValueType::INSTANCE) {
								auto *inst = static_cast<InstanceObject *>(callee.ref.get());
								ClassObject *cls = inst->klass;
								ClassObject::MethodInfo *callMethod = nullptr;
								ClassObject *methodOwner = nullptr;
								for (auto *ancestor : cls->mro) {
									if (ancestor->methods.count("__call__")) {
										callMethod = &ancestor->methods["__call__"];
										methodOwner = ancestor;
										break;
									}
								}
								if (callMethod) {
									Value classObjVal;
									classObjVal.type = ValueType::CLASS;
									classObjVal.ref = std::shared_ptr<HeapObject>(methodOwner, [](HeapObject *) {});
									vector<Value> args;
									for (int i = 0; i < argCount; i++)
										args.push_back(pop());
									std::reverse(args.begin(), args.end());
									stack.push_back(callee);
									stack.push_back(classObjVal);
									for (auto &a : args)
										stack.push_back(a);
									callValue(callMethod->func, argCount + 2, line, col);
								} else {
									throw TypeError("Instance object is not callable", line, col);
								}
							} else
								callValue(callee, argCount, line, col);
						}
						DISPATCH();
					}
					OP(OP_IMPORT) : {
						{
							uint8_t count = *ip++;
							std::vector<std::string> symbols;
							for (int i = 0; i < count; i++)
								symbols.push_back(pop().asString());
							std::reverse(symbols.begin(), symbols.end());
							std::string libName = pop().asString();
							Value moduleResult;
							bool isStar = (count == 1 && symbols[0] == "*");
							if (libName.length() > 4 && libName.substr(libName.length() - 4) == ".ymm") {
								namespace fs = std::filesystem;
								fs::path p(libName);
								if (!fs::exists(p))
									throw ImportError("Module file not found: " + libName, line, col);
								std::error_code ec;
								std::string absPath = fs::absolute(p, ec).string();
								
								// Check if already loaded in globals
								// We can check if a special dictionary or variable exists
								// Since this is VM::run, we need to know if the module was loaded
								if (globals->exists(absPath)) {
									moduleResult = globals->get(absPath);
								} else {
									if (importStack.count(absPath))
										throw CircularImportError("Circular import detected: " + libName, line, col);
									std::ifstream file(absPath);
									if (!file)
										throw FileNotFoundError("Unable to read module: " + libName, line, col);
									std::stringstream buffer;
									buffer << file.rdbuf();
									std::string source = buffer.str();
									importStack.insert(absPath);
								try {
									auto tokens = tokenize(source);
									Parser parser(tokens);
									Chunk moduleChunk;
									ByteCodeCompiler moduleCompiler(&moduleChunk);
									while (!parser.isAtEnd()) {
										Stmt *stmt = parser.parseStmt();
										moduleCompiler.compileStmt(stmt);
									}
									moduleCompiler.emitByte(OpCode::OP_NONE, 0, 0);
									moduleCompiler.emitByte(OpCode::OP_RETURN, 0, 0);
									VM moduleVM;
									moduleVM.globals->set("None", Value::None(), true);
									moduleVM.methodResolver = this->methodResolver;
									moduleVM.importResolver = this->importResolver;
									moduleVM.run(moduleChunk);
									auto exportDict = std::make_shared<DictObject>();
									for (const auto &[key, var] : moduleVM.globals->vars) {
										if (key == "None")
											continue;
										exportDict->items[Value::String(key)] = var.value;
										if (isStar) {
											if (!globals->exists(key)) {
												globals->set(key, var.value, var.isLocked,
													var.isConst);
											}
										}
									}
									moduleResult = Value::Dict(exportDict->items);
									// Cache the module result using the absolute path
									globals->set(absPath, moduleResult, false, false);
								} catch (...) {
									importStack.erase(absPath);
									throw;
								}
								importStack.erase(absPath);
							}
						} else {
								if (this->importResolver) {
									this->importResolver(libName, symbols, globals);
									if (count == 0) {
										if (globals->exists(libName))
											moduleResult = globals->get(libName);
										else
											moduleResult = Value::None();
									} else
										moduleResult = Value::None();
								} else
									throw EnvironmentError("Import resolver not linked!", line, col);
							}
							if (count == 0)
								stack.push_back(moduleResult);
							else {
								if (isStar) {
									stack.push_back(Value::None());
								} else {
									auto findVal = [&](std::string key) -> Value {
										if (moduleResult.type == ValueType::DICT) {
											auto *d = static_cast<DictObject *>(moduleResult.ref.get());
											Value k = Value::String(key);
											if (d->items.count(k))
												return d->items.at(k);
										}
										if (globals->exists(key))
											return globals->get(key);
										throw InvalidImportError("Module '" + libName + "' does not export '" + key + "'", line, col);
									};
									for (const auto &sym : symbols)
										stack.push_back(findVal(sym));
								}
							}
						}
						DISPATCH();
					}
					OP(OP_DEBUG_NAME) : {
						{
							uint8_t nameIndex = *ip++;
							string name = currentChunk->constants[nameIndex].asString();

#ifdef VM_DEBUG_MODE
							stack.back().__DEBUGGING__NAME__ = name;
#endif
						}
						DISPATCH();
					}
					OP(OP_SET_FLAGS) : {
						{
							uint8_t flags = *ip++;
							if (stack.empty())
								throw UnderflowError("Stack underflow", line, col);
							stack.back().isConst = (flags & 0x01);
							stack.back().isLocked = (flags & 0x02);
						}
						DISPATCH();
					}
					OP(OP_LIST_APPEND) : {
						{
							uint8_t slot = *ip++;
							Value val = pop();
							Value &listVal = stack[stack.size() - 4];
							if (listVal.type != ValueType::LIST)
								throw TypeError("Append target is not a list", line, col);
							auto *list = static_cast<ListObject *>(listVal.ref.get());
							list->elements.push_back(val);
						}
						DISPATCH();
					}
					OP(OP_SET_ADD) : {
						{
							uint8_t slot = *ip++;
							Value val = pop();
							Value &setVal = stack[stack.size() - 4];
							if (setVal.type != ValueType::SET)
								throw TypeError("Add target is not a set", line, col);
							auto *set = static_cast<SetObject *>(setVal.ref.get());
							setAdd(set->elements, val);
						}
						DISPATCH();
					}
					OP(OP_DICT_SET) : {
						{
							uint8_t slot = *ip++;
							Value val = pop();
							Value key = pop();
							Value &dictVal = stack[stack.size() - 4];
							if (dictVal.type != ValueType::DICT)
								throw TypeError("Target is not a dict", line, col);
							auto *dict = static_cast<DictObject *>(dictVal.ref.get());
							if (key.type == ValueType::LIST ||
								 key.type == ValueType::SET ||
								 key.type == ValueType::DICT) {
								if (key.type == ValueType::DICT)
									throw TypeError("Dictionary cannot be used as a key",
										line, col);
								key = deepCopy(key);
								key.isConst = true;
							}
							dict->items[key] = val;
						}
						DISPATCH();
					}
					OP(OP_LIST_TO_TUPLE) : {
						{
							Value v = pop();
							if (v.type != ValueType::LIST)
								throw TypeError("Expected list for tuple conversion", line,
									col);
							auto *list = static_cast<ListObject *>(v.ref.get());
							stack.push_back(Value::Tuple(list->elements));
						}
						DISPATCH();
					}
					OP(OP_LIST_TO_VECTOR) : {
						{
							Value v = pop();
							if (v.type != ValueType::LIST)
								throw TypeError("Expected list for vector conversion",
									line, col);
							auto *list = static_cast<ListObject *>(v.ref.get());
							for (const auto &el : list->elements)
								if (!el.isNumber())
									throw TypeError("Vector elements must be numbers", line,
										col);
							stack.push_back(Value::Vector(list->elements));
						}
						DISPATCH();
					}
					OP(OP_BUILD_FSTRING) : {
						{
							uint8_t count = *ip++;
							int startPos = stack.size() - count;
							std::string finalStr = "";
							for (int i = 0; i < count; i++) {
								Value v = stack[startPos + i];
								finalStr += v.type == ValueType::STRING ? v.asString()
																					 : valueToString(v);
							}
							for (int i = 0; i < count; i++)
								stack.pop_back();
							stack.push_back(Value::String(finalStr));
						}
						DISPATCH();
					}
					OP(OP_GET_LOCAL) : {
						{
							uint8_t slot = *ip++;
							const Value& val = stack[frame->basePointer + slot];
							if (val.type == ValueType::NOTYPE) {
								throw CTLEAbort();
							}
							if (val.type == ValueType::REFERENCE)
								stack.push_back(*(val.get_ptr_safe()));
							else
								stack.push_back(val);
						}
						DISPATCH();
					}
					OP(OP_SET_LOCAL) : {
						{
							uint8_t slot = *ip++;
							Value &slotVal = stack[frame->basePointer + slot];
							Value newVal = stack.back();
							if (newVal.type == ValueType::REFERENCE) {
								bool wasConst = slotVal.isConst;
								bool wasLocked = slotVal.isLocked;
								slotVal = newVal;
								slotVal.isConst = wasConst;
								slotVal.isLocked = wasLocked;
							} else if (slotVal.type == ValueType::REFERENCE) {
								if (slotVal.isConst)
									throw OwnershipError("Cannot assign to const reference",
										line, col);
								Value *target = slotVal.ptr;
								if (target->isConst)
									throw ConstError(
										"Cannot assign to const variable via reference",
										line, col);
								if (target->isLocked && target->type != newVal.type) {
									throw TypeError(
										"Cannot change type of locked variable via "
										"reference",
										line, col);
								}
								bool wasConst = target->isConst;
								bool wasLocked = target->isLocked;
								*target = newVal;
								target->isConst = wasConst;
								target->isLocked = wasLocked;
							} else {
								if (slotVal.isConst)
									throw ConstError("Cannot assign to const variable",
										line, col);
								if (slotVal.isLocked && slotVal.type != newVal.type) {
									throw ConstError(
										"Cannot change type of locked variable", line, col);
								}
								bool wasConst = slotVal.isConst;
								bool wasLocked = slotVal.isLocked;
								slotVal = newVal;
								slotVal.isConst = wasConst;
								slotVal.isLocked = wasLocked;
							}
						}
						DISPATCH();
					}
					OP(OP_REF_LOCAL) : {
						{
							uint8_t slot = *ip++;
							Value *ptr = &stack[frame->basePointer + slot];
							if (!invokeUnaryDunder(*ptr, "__ref__", line, col)) {
								std::shared_ptr<HeapObject> owner = nullptr;
								if (ptr->type == ValueType::REFERENCE) {
									owner = ptr->ref;
									ptr = ptr->ptr;
								}
								stack.push_back(Value::Reference(ptr, owner));
							}
						}
						DISPATCH();
					}
					OP(OP_REF_VAR) : {
						{
							uint8_t nameIndex = *ip++;
							string name = currentChunk->constants[nameIndex].asString();
							Var &v = globals->lookup(name);
							Value *ptr = v.alias ? v.alias : &v.value;
							if (!invokeUnaryDunder(*ptr, "__ref__", line, col)) {
								stack.push_back(Value::FromExisting(std::make_shared<VarRefObject>(globals, name), ValueType::REFERENCE));
							}
						}
						DISPATCH();
					}
					OP(OP_REF_INDEX) : {
						{
							Value index = pop();
							Value base = pop();
							Value *ptr = nullptr;
							std::shared_ptr<HeapObject> owner = base.ref;
							if (base.type == ValueType::LIST) {
								auto *list = static_cast<ListObject *>(base.ref.get());
								long long idx = index.asInt();
								stack.push_back(Value::FromExisting(std::make_shared<ListRefObject>(std::static_pointer_cast<ListObject>(base.ref), idx), ValueType::REFERENCE));
							} else if (base.type == ValueType::VECTOR) {
								auto *vec = static_cast<VectorObject *>(base.ref.get());
								long long idx = index.asInt();
								ptr = &vec->elements[idx];
								stack.push_back(Value::Reference(ptr, owner));
							} else {
								throw OwnershipError("Cannot take reference of this type", line, col);
							}
						}
						DISPATCH();
					}
					OP(OP_INC_LOCAL) : {
						{
							uint8_t slot = *ip++;
							int idx = frame->basePointer + slot;
							if (stack[idx].type == ValueType::NOTYPE) {
								throw CTLEAbort();
							}
							if (stack[idx].type == ValueType::INT)
								stack[idx].iVal++;
							else if (stack[idx].type == ValueType::FLOAT)
								stack[idx].fVal++;
							else
								stack[idx].fVal++;
						}
						DISPATCH();
					}
					OP(OP_JUMP_IF_NOT_LT) : {
						{
							uint8_t slot = *ip++;
							uint8_t constIdx = *ip++;
							uint16_t offset = (ip[0] << 8) | ip[1];
							ip += 2;
							int idx = frame->basePointer + slot;
							if (stack[idx].type == ValueType::NOTYPE) {
								throw CTLEAbort();
							}
							if (stack[idx].type == ValueType::INT) {
								long long localVal = stack[idx].iVal;
								long long constVal =
									currentChunk->constants[constIdx].iVal;
								if (localVal >= constVal)
									ip += offset;
							} else
								throw TypeError("Optimized loop requires integer", line,
									col);
						}
						DISPATCH();
					}
					OP(OP_SWITCH_TABLE) : {
						{
							Value val = stack.back();
							uint8_t minIdx = *ip++;
							uint8_t count = *ip++;
							if (val.type == ValueType::INT) {
								long long minVal = currentChunk->constants[minIdx].iVal;
								long long jumpIdx = val.iVal - minVal;
								if (jumpIdx >= 0 && jumpIdx < count) {
									uint8_t *tableEntry = ip + (jumpIdx * 2);
									uint16_t offset = (tableEntry[0] << 8) | tableEntry[1];
									ip = tableEntry + 2 + offset;
								} else
									ip += (count * 2);
							} else
								ip += (count * 2);
						}
						DISPATCH();
					}
					OP(OP_CONSTANT) : {
						{
							uint8_t index = *ip++;
							stack.push_back(currentChunk->constants[index]);
						}
						DISPATCH();
					}
					OP(OP_CONSTANT_LONG) : {
						{
							uint8_t hi = *ip++;
							uint8_t lo = *ip++;
							uint16_t index = (hi << 8) | lo;
							stack.push_back(currentChunk->constants[index]);
						}
						DISPATCH();
					}
					OP(OP_TRUE) : {
						{
							stack.push_back(Value::Bool(true));
						}
						DISPATCH();
					}
					OP(OP_FALSE) : {
						{
							stack.push_back(Value::Bool(false));
						}
						DISPATCH();
					}
					OP(OP_NONE) : {
						{
							stack.push_back(Value::None());
						}
						DISPATCH();
					}
					OP(OP_NOTYPE) : {
						{
							stack.push_back(Value::NoType());
						}
						DISPATCH();
					}
					OP(OP_POP) : {
						{
							if (!stack.empty())
								stack.pop_back();
						}
						DISPATCH();
					}
					OP(OP_IADD) : {
						bool goToAdd = false;
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__plus_eq__", "", line, col)) {
								stack.push_back(a);
								stack.push_back(b);
								goToAdd = true;
							}
						}
						if (goToAdd) {
							goto execute_op_add;
						}
						DISPATCH();
					}
					OP(OP_FAST_IADD) : {
						{
							Value b = pop();
							Value a = pop();
							long long res;
							if (__builtin_add_overflow(a.iVal, b.iVal, &res)) {
								stack.push_back(BigIntObject::add(Value::BigInt(a.iVal), Value::BigInt(b.iVal)));
							} else {
								stack.push_back(Value::Int(res));
							}
						}
						DISPATCH();
					}
					OP(OP_FAST_ISUB) : {
						{
							Value b = pop();
							Value a = pop();
							long long res;
							if (__builtin_sub_overflow(a.iVal, b.iVal, &res)) {
								stack.push_back(BigIntObject::sub(Value::BigInt(a.iVal), Value::BigInt(b.iVal)));
							} else {
								stack.push_back(Value::Int(res));
							}
						}
						DISPATCH();
					}
					OP(OP_FAST_IMUL) : {
						{	
							Value b = pop();
							Value a = pop();
							long long res;
							if (__builtin_mul_overflow(a.iVal, b.iVal, &res)) {
								stack.push_back(BigIntObject::mul(Value::BigInt(a.iVal), Value::BigInt(b.iVal)));
							} else {
								stack.push_back(Value::Int(res));
							}
						}
						DISPATCH();
					}
					OP(OP_FAST_IDIV) : {
						{
							Value b = pop();
							Value a = pop();
							if (b.iVal == 0) {
								throw ValueError("Division by zero", line, col);
							}
							stack.push_back(Value::Int(a.iVal / b.iVal));
						}
						DISPATCH();
					}
					OP(OP_FAST_FADD) : {
						{Value b = pop();
						Value a = pop();
						stack.push_back(Value::Float(a.fVal + b.fVal));}
						DISPATCH();
					}
					OP(OP_FAST_FSUB) : {
						{Value b = pop();
						Value a = pop();
						stack.push_back(Value::Float(a.fVal - b.fVal));}
						DISPATCH();
					}
					OP(OP_FAST_FMUL) : {
						{Value b = pop();
						Value a = pop();
						stack.push_back(Value::Float(a.fVal * b.fVal));}
						DISPATCH();
					}
					OP(OP_FAST_FDIV) : {
						{Value b = pop();
						Value a = pop();
						if (b.fVal == 0.0) {
							throw ValueError("Division by zero", line, col);
						}
						stack.push_back(Value::Float(a.fVal / b.fVal));}
						DISPATCH();
					}
					OP(OP_FAST_SADD) : {
						{Value b = pop();
						Value a = pop();
						stack.push_back(Value::String(a.asString() + b.asString()));
						}DISPATCH();
					}
					OP(OP_CTLE_TICK) : {
						{
							ctleTicks++;
							if (ctleTicks > 50000000) {
								throw CTLEAbort();
							}
						}
						DISPATCH();
					}
					OP(OP_ADD) : {
					execute_op_add: {
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__plus__", "__r_plus__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::PLUS, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
					}
						DISPATCH();
					}
					OP(OP_ISUB) : {
						bool goToSub = false;
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__minus_eq__", "", line, col)) {
								stack.push_back(a);
								stack.push_back(b);
								goToSub = true;
							}
						}
						if (goToSub) {
							goto execute_op_sub;
						}
						DISPATCH();
					}
					OP(OP_SUB) : {
					execute_op_sub: {
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__minus__", "__r_minus__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::MINUS, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
					}
						DISPATCH();
					}
					OP(OP_IDIV) : {
						bool goToDiv = false;
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__divide_eq__", "", line, col)) {
								stack.push_back(a);
								stack.push_back(b);
								goToDiv = true;
							}
						}
						if (goToDiv) {
							goto execute_op_div;
						}
						DISPATCH();
					}
					OP(OP_DIV) : {
					execute_op_div: {
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__divide__", "__r_divide__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::SLASH, a, b));
							} catch (const std::runtime_error &e) {
								std::string msg = e.what();
								if (msg.find("zero") != std::string::npos) {
									throw DivisionByZeroError(msg, line, col);
								} else if (msg.find("Vector") != std::string::npos || msg.find("divide by") != std::string::npos) {
									throw TypeError(msg, line, col);
								} else {
									throw ValueError(msg, line, col);
								}
							}
						}
					}
						DISPATCH();
					}
					OP(OP_IMUL) : {
						bool goToMul = false;
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__times_eq__", "", line, col)) {
								stack.push_back(a);
								stack.push_back(b);
								goToMul = true;
							}
						}
						if (goToMul) {
							goto execute_op_mul;
						}
						DISPATCH();
					}
					OP(OP_MUL) : {
					execute_op_mul: {
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__times__", "__r_times__", line, col)) {
							if (a.type == ValueType::LIST && b.type == ValueType::INT) {
								auto *listObj = static_cast<ListObject *>(a.ref.get());
								long long count = b.asInt();
								vector<Value> res;
								if (count > 0) {
									if (listObj->elements.size() * count > 1000000)
										throw MemoryError("List repetition too large", line, col);
									res.reserve(listObj->elements.size() * count);
									for (int i = 0; i < count; i++) {
										for (const auto &elem : listObj->elements)
											res.push_back(deepCopy(elem));
									}
								}
								stack.push_back(Value::List(res));
							} else if ((a.type == ValueType::FLOAT && b.type == ValueType::BIGINT) ||
										  (a.type == ValueType::BIGINT && b.type == ValueType::FLOAT)) {
								Value fVal = (a.type == ValueType::FLOAT) ? a : b;
								Value intVal = (a.type == ValueType::FLOAT) ? b : a;
								auto *bigObj = static_cast<BigIntObject *>(intVal.ref.get());
								std::vector<uint32_t> tempChunks = bigObj->chunks;
								double floatBase = fVal.asFloat();
								double result = 0.0;
								double powerOf10 = 1.0;
								bool overflow = false;
								if (tempChunks.size() > 40) {
									overflow = true;
								} else {
									while (!tempChunks.empty() && !(tempChunks.size() == 1 && tempChunks[0] == 0)) {
										uint32_t rem = divMod1e9(tempChunks);
										result += (floatBase * rem * powerOf10);
										powerOf10 *= 1e9;
										if (std::isinf(result) || std::isinf(powerOf10)) {
											overflow = true;
											break;
										}
									}
								}
								if (!overflow) {
									if (bigObj->isNegative)
										result = -result;
									stack.push_back(Value::Float(result));
								} else {
									std::ostringstream ss;
									ss << std::fixed << std::setprecision(6) << std::abs(floatBase);
									std::string s = ss.str();
									size_t decimalPos = s.find('.');
									long long power = 0;
									if (decimalPos != std::string::npos) {
										power = s.length() - decimalPos - 1;
										s.erase(decimalPos, 1);
									}
									BigIntObject mantissa(0);
									BigIntObject ten(10);
									for (char c : s) {
										BigIntObject digit(c - '0');
										mantissa = (mantissa * ten) + digit;
									}
									BigIntObject product = (*bigObj) * mantissa;
									if (power > 0) {
										BigIntObject divisor(1);
										for (int i = 0; i < power; i++)
											divisor = divisor * ten;
										product = product / divisor;
									}
									bool resultNeg = (bigObj->isNegative) != (floatBase < 0);
									product.isNegative = resultNeg;
									stack.push_back(Value::BigInt(std::make_shared<BigIntObject>(product)));
								}
							} else {
								try {
									stack.push_back(EvaluateConstBinary(TokenType::STAR, a, b));
								} catch (const std::runtime_error &e) {
									std::string msg = e.what();
									if (msg.find("Memory") != std::string::npos || msg.find("large") != std::string::npos) {
										throw MemoryError(msg, line, col);
									} else {
										throw ValueError(msg, line, col);
									}
								}
							}
						}
					}
						DISPATCH();
					}
					OP(OP_IFLOOR_DIV) : {
						bool goToFloorDiv = false;
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__int_divide_eq__", "", line, col)) {
								stack.push_back(a);
								stack.push_back(b);
								goToFloorDiv = true;
							}
						}
						if (goToFloorDiv) {
							goto execute_op_int_div;
						}
						DISPATCH();
					}
					OP(OP_FLOOR_DIV) : {
					execute_op_int_div: {
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__int_divide__", "__r_int_divide__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::FLOOR_DIV, a, b));
							} catch (const std::runtime_error &e) {
								std::string msg = e.what();
								if (msg.find("zero") != std::string::npos) {
									throw DivisionByZeroError(msg, line, col);
								} else if (msg.find("Vector") != std::string::npos || msg.find("floor-divide by") != std::string::npos) {
									throw TypeError(msg, line, col);
								} else {
									throw ValueError(msg, line, col);
								}
							}
						}
					}
						DISPATCH();
					}
					OP(OP_IMOD) : {
						bool goToMod = false;
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__modulo_eq__", "", line, col)) {
								stack.push_back(a);
								stack.push_back(b);
								goToMod = true;
							}
						}
						if (goToMod) {
							goto execute_op_mod;
						}
						DISPATCH();
					}
					OP(OP_MOD) : {
					execute_op_mod: {
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__modulo__", "__r_modulo__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::MOD, a, b));
							} catch (const std::runtime_error &e) {
								std::string msg = e.what();
								if (msg.find("zero") != std::string::npos) {
									throw DivisionByZeroError(msg, line, col);
								} else {
									throw ValueError(msg, line, col);
								}
							}
						}
					}
						DISPATCH();
					}
					OP(OP_IPOW) : {
						bool goToPow = false;
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__power_eq__", "", line, col)) {
								stack.push_back(a);
								stack.push_back(b);
								goToPow = true;
							}
						}
						if (goToPow) {
							goto execute_op_pow;
						}
						DISPATCH();
					}
					OP(OP_POW) : {
					execute_op_pow: {
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__power__", "__r_power__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::POW, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
					}
						DISPATCH();
					}
					OP(OP_BITWISE_AND) : {
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__bit_and__", "__r_bit_and__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::BITWISE_AND, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
						DISPATCH();
					}
					OP(OP_BITWISE_OR) : {
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__bit_or__", "__r_bit_or__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::BITWISE_OR, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
						DISPATCH();
					}
					OP(OP_BITWISE_XOR) : {
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__bit_xor__", "__r_bit_xor__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::BITWISE_XOR, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
						DISPATCH();
					}
					OP(OP_BITWISE_SHL) : {
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__bit_shl__", "__r_bit_shl__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::BITWISE_SHL, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
						DISPATCH();
					}
					OP(OP_BITWISE_SHR) : {
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__bit_shr__", "__r_bit_shr__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::BITWISE_SHR, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
						DISPATCH();
					}
					// --- Comparisons ---
					OP(OP_GT) : {
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__greater__", "__less__", line, col)) {
								bool result = lessValue(b, a, this->globals, this->methodResolver, this->importResolver);
								stack.push_back(Value::Bool(result));
							}
						}
						DISPATCH();
					}
					OP(OP_GTE) : {
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__greater_eq__", "__less_eq__", line, col)) {
								bool result = !lessValue(a, b, this->globals, this->methodResolver, this->importResolver);
								stack.push_back(Value::Bool(result));
							}
						}
						DISPATCH();
					}
					OP(OP_LT) : {
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__less__", "__greater__", line, col)) {
								bool result = lessValue(a, b, this->globals, this->methodResolver, this->importResolver);
								stack.push_back(Value::Bool(result));
							}
						}
						DISPATCH();
					}
					OP(OP_LTE) : {
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__less_eq__", "__greater_eq__", line, col)) {
								bool result = !lessValue(b, a, this->globals, this->methodResolver, this->importResolver);
								stack.push_back(Value::Bool(result));
							}
						}
						DISPATCH();
					}
					OP(OP_EQ) : {
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__equals__", "__equals__", line, col)) {
								stack.push_back(Value::Bool(a.looseEquals(b)));
							}
						}
						DISPATCH();
					}
					OP(OP_NEQ) : {
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__differs__", "__differs__", line, col)) {
								stack.push_back(Value::Bool(!(a.looseEquals(b))));
							}
						}
						DISPATCH();
					}
					OP(OP_STRICT_EQ) : {
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__identical__", "__identical__", line, col)) {
								stack.push_back(Value::Bool(a.strictEquals(b)));
							}
						}
						DISPATCH();
					}
					OP(OP_STRICT_NEQ) : {
						{
							Value b = pop();
							Value a = pop();
							if (!invokeBinaryDunder(a, b, "__distinct__", "__distinct__", line, col)) {
								stack.push_back(Value::Bool(!(a.strictEquals(b))));
							}
						}
						DISPATCH();
					}
					OP(OP_IS) : {
						{
							Value b = pop();
							Value a = pop();
							const Value &valA = (a.type == ValueType::REFERENCE && a.get_ptr_safe()) ? *(a.get_ptr_safe()) : a;
							const Value &valB = (b.type == ValueType::REFERENCE && b.get_ptr_safe()) ? *(b.get_ptr_safe()) : b;
							bool same = false;
							if (valA.type == valB.type) {
								if (valA.ref != nullptr || valB.ref != nullptr) {
									same = (valA.ref.get() == valB.ref.get());
								} else {
									same = valA.strictEquals(valB);
								}
							}
							stack.push_back(Value::Bool(same));
						}
						DISPATCH();
					}
					OP(OP_IS_NOT) : {
						{
							Value b = pop();
							Value a = pop();
							const Value &valA = (a.type == ValueType::REFERENCE && a.get_ptr_safe()) ? *(a.get_ptr_safe()) : a;
							const Value &valB = (b.type == ValueType::REFERENCE && b.get_ptr_safe()) ? *(b.get_ptr_safe()) : b;
							bool same = false;
							if (valA.type == valB.type) {
								if (valA.ref != nullptr || valB.ref != nullptr) {
									same = (valA.ref.get() == valB.ref.get());
								} else {
									same = valA.strictEquals(valB);
								}
							}
							stack.push_back(Value::Bool(!same));
						}
						DISPATCH();
					}
					OP(OP_XOR) : {
						{
							Value b = pop();
							Value a = pop();
							stack.push_back(Value::Bool(a.isTruthy() != b.isTruthy()));
						}
						DISPATCH();
					}
					OP(OP_NXOR) : {
						{
							Value b = pop();
							Value a = pop();
							stack.push_back(Value::Bool(a.isTruthy() == b.isTruthy()));
						}
						DISPATCH();
					}
					OP(OP_NAND) : {
						{
							Value b = pop();
							Value a = pop();
							stack.push_back(Value::Bool(!(a.isTruthy() && b.isTruthy())));
						}
						DISPATCH();
					}
					OP(OP_NOR) : {
						{
							Value b = pop();
							Value a = pop();
							stack.push_back(Value::Bool(!(a.isTruthy() || b.isTruthy())));
						}
						DISPATCH();
					}
					OP(OP_NOT) : {
						{
							Value v = pop();
							stack.push_back(Value::Bool(!v.isTruthy()));
						}
						DISPATCH();
					}
					OP(OP_BITWISE_NOT) : {
						{
							Value v = pop();
							if (v.type == ValueType::INT) {
								stack.push_back(Value::Int(~v.iVal));
							} else if (v.type == ValueType::BIGINT) {
								throw std::runtime_error("Bitwise NOT on BigInt is not supported");
							} else {
								throw TypeError("Bitwise NOT requires an integer", line, col);
							}
						}
						DISPATCH();
					}
					OP(OP_NEGATE) : {
						{
							Value v = pop();
							if (v.type == ValueType::INT) {
								if (v.iVal == LLONG_MIN)
									stack.push_back(BigIntObject::mul(Value::BigInt(v.iVal),
										Value::BigInt(-1)));
								else
									stack.push_back(Value::Int(-v.iVal));
							} else if (v.type == ValueType::BIGINT) {
								stack.push_back(BigIntObject::mul(v, Value::BigInt(-1)));
							} else if (v.type == ValueType::VECTOR) {
								auto *vec = static_cast<VectorObject *>(v.ref.get());
								vector<Value> res;
								res.reserve(vec->elements.size());
								for (const auto &el : vec->elements) {
									if (el.type == ValueType::INT) {
										if (el.iVal == LLONG_MIN)
											res.push_back(BigIntObject::mul(
												Value::BigInt(el.iVal), Value::BigInt(-1)));
										else
											res.push_back(Value::Int(-el.iVal));
									} else if (el.type == ValueType::BIGINT) {
										res.push_back(
											BigIntObject::mul(el, Value::BigInt(-1)));
									} else {
										res.push_back(Value::Float(-el.asFloat()));
									}
								}
								stack.push_back(Value::Vector(res));
							} else
								stack.push_back(Value::Float(-v.asFloat()));
						}
						DISPATCH();
					}
					OP(OP_IS_IN) : {
						{
							Value rhs = pop(); // The container
							Value lhs = pop(); // The item
							if (!invokeBinaryDunder(rhs, lhs, "__has__", "__missing__", line, col)) {
								bool found = false;
								if (rhs.type == ValueType::STRING) {
									if (lhs.type == ValueType::STRING) {
										found = rhs.asString().find(lhs.asString()) != string::npos;
									}
								} else if (rhs.type == ValueType::LIST) {
									auto *list = static_cast<ListObject *>(rhs.ref.get());
									for (const auto &item : list->elements) {
										if (item.strictEquals(lhs)) {
											found = true;
											break;
										}
									}
								} else if (rhs.type == ValueType::DICT) {
									auto *d = static_cast<DictObject *>(rhs.ref.get());
									found = d->items.count(lhs) > 0;
								} else if (rhs.type == ValueType::SET) {
									auto *s = static_cast<SetObject *>(rhs.ref.get());
									found = s->elements.count(lhs) > 0;
								} else if (rhs.type == ValueType::RANGE) {
									auto *rng = static_cast<RangeObject *>(rhs.ref.get());
									if (lhs.isNumber()) {
										double val = lhs.asFloat();
										bool inBounds =
											(rng->step > 0)
												? (val >= rng->start &&
													  (rng->endInclusive ? val <= rng->end : val < rng->end))
												: (val <= rng->start &&
													  (rng->endInclusive ? val >= rng->end : val > rng->end));
										if (inBounds && !rng->isFloat && lhs.type == ValueType::INT) {
											found = ((long long)(val - rng->start) % (long long)rng->step == 0);
										} else {
											found = inBounds;
										}
									}
								}
								stack.push_back(Value::Bool(found));
							}
						}
						DISPATCH();
					}
					OP(OP_IS_NOT_IN) : {
						{
							Value rhs = pop(); // The container
							Value lhs = pop(); // The item
							if (!invokeBinaryDunder(rhs, lhs, "__lacks__", "__not_missing__", line, col)) {
								bool found = false;
								if (rhs.type == ValueType::STRING) {
									if (lhs.type == ValueType::STRING) {
										found = rhs.asString().find(lhs.asString()) != string::npos;
									}
								} else if (rhs.type == ValueType::LIST) {
									auto *list = static_cast<ListObject *>(rhs.ref.get());
									for (const auto &item : list->elements) {
										if (item.strictEquals(lhs)) {
											found = true;
											break;
										}
									}
								} else if (rhs.type == ValueType::DICT) {
									auto *d = static_cast<DictObject *>(rhs.ref.get());
									found = d->items.count(lhs) > 0;
								} else if (rhs.type == ValueType::SET) {
									auto *s = static_cast<SetObject *>(rhs.ref.get());
									found = s->elements.count(lhs) > 0;
								} else if (rhs.type == ValueType::RANGE) {
									auto *rng = static_cast<RangeObject *>(rhs.ref.get());
									if (lhs.isNumber()) {
										double val = lhs.asFloat();
										bool inBounds =
											(rng->step > 0)
												? (val >= rng->start &&
													  (rng->endInclusive ? val <= rng->end : val < rng->end))
												: (val <= rng->start &&
													  (rng->endInclusive ? val >= rng->end : val > rng->end));
										if (inBounds && !rng->isFloat && lhs.type == ValueType::INT) {
											found = ((long long)(val - rng->start) % (long long)rng->step == 0);
										} else {
											found = inBounds;
										}
									}
								}
								stack.push_back(Value::Bool(!found));
							}
						}
						DISPATCH();
					}
					OP(OP_LOOP) : {
						{
							uint8_t hi = *ip++;
							uint8_t lo = *ip++;
							uint16_t offset = (hi << 8) | lo;
							ip -= offset;
						}
						DISPATCH();
					}
					OP(OP_DEFINE_VAR) : {
						{
							uint8_t nameIndex = *ip++;
							uint8_t flags = *ip++;
							string name = currentChunk->constants[nameIndex].asString();
							Value val = pop();
#ifdef VM_DEBUG_MODE
							if (DEBUGGER_MODE_IS_ENABLED)
								val.__DEBUGGING__NAME__ = name;
#endif
							bool isConst = (flags & 0x01) != 0;
							bool isLocked = (flags & 0x02) != 0;
							if (globals->exists(name)) {
								Value existing = globals->get(name);
								bool identical = false;
								if (existing.ref != nullptr && val.ref != nullptr) {
									identical = (existing.ref.get() == val.ref.get());
								} else if (existing.type == val.type) {
									if (existing.type == ValueType::FLOAT)
										identical = (existing.fVal == val.fVal);
									else if (existing.type == ValueType::INT)
										identical = (existing.iVal == val.iVal);
									else if (existing.type == ValueType::BOOL)
										identical = (existing.bVal == val.bVal);
									else if (existing.type == ValueType::NONE)
										identical = true;
								}
								if (identical) {
									goto skip_define;
								}
							}
							if (val.type == ValueType::FUNCTION || val.type == ValueType::NATIVE_FUNCTION) {
								if (globals->exists(name)) {
									Value existing = globals->get(name);
									if (existing.type == ValueType::FUNCTION || existing.type == ValueType::NATIVE_FUNCTION) {
										auto ovObj = std::make_shared<OverloadObject>(existing);
										ovObj->overloads.push_back(val);
										Value ovVal;
										ovVal.type = ValueType::OVERLOAD;
										ovVal.ref = ovObj;
										globals->set(name, ovVal, isLocked, isConst);
									} else if (existing.type == ValueType::OVERLOAD) {
										auto *ov = static_cast<OverloadObject *>(existing.ref.get());
										ov->overloads.push_back(val);
									} else {
										globals->set(name, val, isLocked, isConst);
									}
								} else {
									globals->set(name, val, isLocked, isConst);
								}
							} else {
								globals->set(name, val, isLocked, isConst);
							}
						}
					skip_define:;
						DISPATCH();
					}
					OP(OP_GET_VAR) : {
						{
							uint8_t nameIndex = *ip++;
							string name = currentChunk->constants[nameIndex].asString();
							if (!globals->exists(name))
								throw NameError("Undefined variable '" + name + "'", line,
									col);
							Var &v = globals->lookup(name);
							if (v.alias)
								stack.push_back(*v.alias);
							else
								stack.push_back(v.value);
						}
						DISPATCH();
					}
					OP(OP_JUMP_IF_FALSE) : {
						{
							uint8_t hi = *ip++;
							uint8_t lo = *ip++;
							uint16_t offset = (hi << 8) | lo;
							Value val = stack.back();
							bool truthy = val.isTruthy();
							if (val.type == ValueType::INSTANCE) {
								auto dunder = tryCastDunder(val, "__to_bool__", line, col, globals, methodResolver, importResolver);
								if (dunder.first) {
									truthy = dunder.second.isTruthy();
								}
							}
							if (!truthy) {
								ip += offset;
							}
						}
						DISPATCH();
					}
					OP(OP_JUMP) : {
						{
							uint8_t hi = *ip++;
							uint8_t lo = *ip++;
							uint16_t offset = (hi << 8) | lo;
							ip += offset;
						}
						DISPATCH();
					}
					OP(OP_UNPACK_DICT) : {
						{
							Value v = pop();
							if (v.type != ValueType::DICT) {
								throw TypeError("Cannot unpack non-dictionary", line, col);
							}
							auto dict = static_cast<DictObject *>(v.ref.get());
							auto keys = std::make_shared<ListObject>();
							auto vals = std::make_shared<ListObject>();
							for (const auto &pair : dict->items) {
								keys->elements.push_back(pair.first);
								vals->elements.push_back(pair.second);
							}
							stack.push_back(Value::List(keys->elements));
							stack.push_back(Value::List(vals->elements));
						}
						DISPATCH();
					}
					OP(OP_SET_VAR) : {
						{
							uint8_t nameIndex = *ip++;
							string name = currentChunk->constants[nameIndex].asString();
							Value val = stack.back();
#ifdef VM_DEBUG_MODE
							if (DEBUGGER_MODE_IS_ENABLED)
								val.__DEBUGGING__NAME__ = name;
#endif
							if (!globals->exists(name))
								throw NameError("Undefined variable '" + name + "'", line,
									col);
							Var &v = globals->lookup(name);
							if (val.type == ValueType::REFERENCE) {
								v.alias = val.ptr;
								v.value.ref = val.ref;
							} else if (v.alias) {
								Value *target = v.alias;
								if (target->isConst)
									throw ConstError("Cannot assign to const variable '" +
															  name + "' via reference",
										line, col);
								if (target->isLocked && target->type != val.type) {
									throw TypeError(
										"Cannot change type of locked variable '" + name +
											"' via reference",
										line, col);
								}
								bool wasConst = target->isConst;
								bool wasLocked = target->isLocked;
								*target = val;
								target->isConst = wasConst;
								target->isLocked = wasLocked;
							} else {
								if (v.isConst)
									throw ConstError(
										"Cannot assign to const variable '" + name + "'",
										line, col);
								if (v.isLocked && v.value.type != val.type)
									throw TypeError(
										"Cannot change type of locked variable '" + name +
											"'",
										line, col);
								v.value = val;
							}
						}
						DISPATCH();
					}
					OP(OP_DEFINE_REF) : {
						{
							uint8_t nameIdx = *ip++;
							string newVarName =
								currentChunk->constants[nameIdx].asString();
							OpCode subOp = static_cast<OpCode>(*ip++);
							if (subOp == OpCode::OP_REF_VAR) {
								string targetName =
									currentChunk->constants[*ip++].asString();
								Var &targetVar = globals->lookup(targetName);
								Var aliasVar;
								aliasVar.alias =
									targetVar.alias ? targetVar.alias : &targetVar.value;
								globals->vars[newVarName] = aliasVar;
							} else if (subOp == OpCode::OP_REF_INDEX) {
								Value index = pop();
								Value base = pop();
								if (base.type == ValueType::LIST) {
									auto *list = static_cast<ListObject *>(base.ref.get());
									int i = (int)index.asInt();
									Var aliasVar;
									aliasVar.alias = &list->elements[i];
									globals->vars[newVarName] = aliasVar;
								}
							}
						}
						DISPATCH();
					}
					OP(OP_SET_REF) : {
						{
							uint8_t nameIdx = *ip++;
							string varName = currentChunk->constants[nameIdx].asString();
							if (!globals->exists(varName))
								throw NameError("Undefined variable" + varName, line, col);
							OpCode subOp = static_cast<OpCode>(*ip++);
							if (subOp == OpCode::OP_REF_VAR) {
								string target = currentChunk->constants[*ip++].asString();
								Var &targetVar = globals->lookup(target);
								globals->vars[varName].alias =
									targetVar.alias ? targetVar.alias : &targetVar.value;
							}
						}
						DISPATCH();
					}
					OP(OP_COLON) : {
						{
							Value v = pop();
							Value k = pop();
							vector<std::pair<Value, Value>> p;
							p.push_back({k, v});
							stack.push_back(Value::Paired(p));
						}
						DISPATCH();
					}
					OP(OP_TO_STREAM) : {
						{
							Value v = pop();
							if (v.type == ValueType::INSTANCE) {
								auto dunder = tryCastDunder(v, "__traverse__", line, col, this->globals, this->methodResolver, this->importResolver);
								if (dunder.first) {
									stack.push_back(dunder.second);
								} else {
									stack.push_back(prepareIterable(v, line, col));
								}
							} else {
								stack.push_back(prepareIterable(v, line, col));
							}
						}
						DISPATCH();
					}
					OP(OP_FOR_ITER) : {
						{
							uint8_t *jumpOffsetAddr = ip;
							uint16_t offset = (jumpOffsetAddr[0] << 8) | jumpOffsetAddr[1];
							ip += 2;
							uint8_t count = *ip++;
							Value &indexVal = stack.back();
							long long stepCount = indexVal.asInt();
							bool valid = true;
							vector<Value> nextValues;
							for (int i = 0; i < count; i++) {
								int stackPos = stack.size() - 1 - count + i;
								Value &stream = stack[stackPos];
								std::shared_ptr<HeapObject> owner = stream.ref;
								if (stream.type == ValueType::LIST) {
									auto *list = static_cast<ListObject *>(stream.ref.get());
									if (stepCount >= (long long)list->elements.size()) {
										valid = false;
										break;
									}
									nextValues.push_back(Value::Reference(&list->elements[stepCount], owner));
								} else if (stream.type == ValueType::TUPLE) {
									auto *tuple = static_cast<TupleObject *>(stream.ref.get());
									if (stepCount >= (long long)tuple->elements.size()) {
										valid = false;
										break;
									}
									nextValues.push_back(tuple->elements[stepCount]);
								} else if (stream.type == ValueType::SET) {
									auto *s = static_cast<SetObject *>(stream.ref.get());
									std::vector<Value> snapshot(s->elements.begin(), s->elements.end());
									stream = Value::List(snapshot);
									owner = stream.ref;
									auto *list = static_cast<ListObject *>(stream.ref.get());
									if (stepCount >= (long long)list->elements.size()) {
										valid = false;
										break;
									}
									nextValues.push_back(Value::Reference(&list->elements[stepCount], owner));
								} else if (stream.type == ValueType::STRING) {
									string s = stream.asString();
									if (stepCount >= (long long)s.length()) {
										valid = false;
										break;
									}
									nextValues.push_back(Value::String(string(1, s[stepCount])));
								} else if (stream.type == ValueType::VECTOR) {
									auto *vec = static_cast<VectorObject *>(stream.ref.get());
									if (stepCount >= (long long)vec->elements.size()) {
										valid = false;
										break;
									}
									nextValues.push_back(Value::Reference(&vec->elements[stepCount], owner));
								} else if (stream.type == ValueType::RANGE) {
									auto *r = static_cast<RangeObject *>(stream.ref.get());
									double current = r->start + (r->step * stepCount);
									bool inBounds = (r->step > 0)
															 ? (r->endInclusive ? current <= r->end : current < r->end)
															 : (r->endInclusive ? current >= r->end : current > r->end);
									if (!inBounds) {
										valid = false;
										break;
									}
									if (r->isFloat)
										nextValues.push_back(Value::Float(current));
									else
										nextValues.push_back(Value::Int((long long)current));
								} else if (stream.type == ValueType::INSTANCE) {
									auto *inst = static_cast<InstanceObject *>(stream.ref.get());
									ClassObject *cls = inst->klass;
									ClassObject::MethodInfo *advMethod = nullptr;
									ClassObject *methodOwner = nullptr;
									for (auto *ancestor : cls->mro) {
										if (ancestor->methods.count("__advance__")) {
											advMethod = &ancestor->methods["__advance__"];
											methodOwner = ancestor;
											break;
										}
									}
									if (advMethod) {
										Value classObjVal;
										classObjVal.type = ValueType::CLASS;
										classObjVal.ref = std::shared_ptr<HeapObject>(methodOwner, [](HeapObject *) {});
										VM tempVM;
										tempVM.globals = this->globals;
										tempVM.methodResolver = this->methodResolver;
										tempVM.importResolver = this->importResolver;
										Chunk tempChunk;
										int selfIdx = tempChunk.addConstant(stream);
										tempChunk.write(OpCode::OP_CONSTANT, line, col);
										tempChunk.write((uint8_t)selfIdx, line, col);
										int objIdx = tempChunk.addConstant(classObjVal);
										tempChunk.write(OpCode::OP_CONSTANT, line, col);
										tempChunk.write((uint8_t)objIdx, line, col);
										int stepIdx = tempChunk.addConstant(Value::Int(stepCount));
										tempChunk.write(OpCode::OP_CONSTANT, line, col);
										tempChunk.write((uint8_t)stepIdx, line, col);
										int methIdx = tempChunk.addConstant(advMethod->func);
										tempChunk.write(OpCode::OP_CONSTANT, line, col);
										tempChunk.write((uint8_t)methIdx, line, col);
										tempChunk.write(OpCode::OP_CALL, line, col);
										tempChunk.write((uint8_t)3, line, col);
										tempChunk.write(OpCode::OP_RETURN, line, col);
										try {
											tempVM.run(tempChunk);
											if (!tempVM.stack.empty()) {
												nextValues.push_back(tempVM.stack.back());
											} else {
												nextValues.push_back(Value::None());
											}
										} catch (const LangError &e) {
											if (e.type == "IteratorError") {
												valid = false;
											} else {
												throw;
											}
										}
									} else {
										throw TypeError("Iterator object missing '__advance__' method", line, col);
									}
								} else {
									throw TypeError("Unsupported stream type in iterator", line, col);
								}
							}
							if (valid) {
								for (const auto &val : nextValues)
									stack.push_back(val);
								stack[stack.size() - 1 - count].iVal++;
							} else {
								ip = jumpOffsetAddr + 2 + offset;
							}
						}
						DISPATCH();
					}
					OP(OP_SKIP_ITER) : {
						{
							uint8_t slot = *ip++;
							Value amount = pop();
							if (!amount.isNumber())
								throw TypeError("skip amount must be a number", line, col);
							long long skipN = amount.asInt();
							int absoluteSlot = frame->basePointer + slot;
							if (absoluteSlot >= stack.size())
								throw IndexError("Skip iterator slot out of bounds", line,
									col);
							stack[absoluteSlot].iVal += skipN;
						}
						DISPATCH();
					}
					OP(OP_BUILD_SLICE) : {
						{
							Value step = pop();
							Value end = pop();
							Value start = pop();
							auto slice = std::make_shared<SliceObject>(start, end, step);
							Value v;
							v.type = ValueType::SLICE;
							v.ref = slice;
							stack.push_back(v);
						}
						DISPATCH();
					}
					// CONTAINERS
					OP(OP_BUILD_LIST) : {
						{
							uint8_t count = *ip++;
							auto list = std::make_shared<ListObject>();
							if (stack.size() < count)
								throw EmptyContainerError(
									"Stack underflow during list build", line, col);
							list->elements.resize(count);
							for (int i = count - 1; i >= 0; i--) {
								list->elements[i] = pop();
							}
							stack.push_back(Value::FromExisting(list, ValueType::LIST));
						}
						DISPATCH();
					}
					OP(OP_BUILD_DICT) : {
						{
							uint8_t count = *ip++;
							auto dict = std::make_shared<DictObject>();
							for (int i = 0; i < count; i++) {
								Value pairVal = pop();
								if (pairVal.type == ValueType::PAIRED) {
									auto *pObj =
										static_cast<PairedObject *>(pairVal.ref.get());
									for (const auto &entry : pObj->pairs) {
										Value key = entry.first;
										if (key.type == ValueType::LIST ||
											 key.type == ValueType::SET) {
											key = deepCopy(key);
											key.isConst = true;
										}
										dict->items[key] = entry.second;
									}
								}
							}
							stack.push_back(Value::Dict(dict->items));
						}
						DISPATCH();
					}
					OP(OP_BUILD_SET) : {
						{
							uint8_t count = *ip++;
							auto set = std::make_shared<SetObject>();
							for (int i = 0; i < count; i++)
								setAdd(set->elements, pop());
							stack.push_back(Value::Set(set->elements));
						}
						DISPATCH();
					}
					OP(OP_BUILD_TUPLE) : {
						{
							uint8_t count = *ip++;
							vector<Value> elems(count);
							for (int i = count - 1; i >= 0; i--)
								elems[i] = pop();
							stack.push_back(Value::Tuple(elems));
						}
						DISPATCH();
					}
					OP(OP_BUILD_VECTOR) : {
						{
							uint8_t count = *ip++;
							vector<Value> elems;
							elems.resize(count);
							for (int i = count - 1; i >= 0; i--) {
								Value v = pop();
								if (!v.isNumber())
									throw TypeError("Vector elements must be numbers", line,
										col);
								elems[i] = v;
							}
							stack.push_back(Value::Vector(elems));
						}
						DISPATCH();
					}
					OP(OP_BUILD_RANGE) : {
						{
							uint8_t flags = *ip++;
							bool si = (flags & 0x01) != 0;
							bool ei = (flags & 0x02) != 0;
							Value step = pop();
							Value end = pop();
							Value start = pop();
							bool isFloat = (start.type == ValueType::FLOAT || end.type == ValueType::FLOAT || step.type == ValueType::FLOAT);
							stack.push_back(Value::Range(start.asFloat(), end.asFloat(),
								step.asFloat(), si, ei,
								isFloat));
						}
						DISPATCH();
					}
					OP(OP_SHALLOW_COPY) : {
						{
							Value v = pop();
							if (!invokeUnaryDunder(v, "__copy__", line, col)) {
								if (v.type == ValueType::INSTANCE ||
									 v.type == ValueType::CLASS ||
									 v.type == ValueType::REFERENCE) {
									stack.push_back(v);
								} else {
									Value res = shallowCopy(v);
									res.isConst = false;
									res.isLocked = false;
									stack.push_back(res);
								}
							}
						}
						DISPATCH();
					}
					OP(OP_DEEP_COPY) : {
						{
							Value v = pop();
							if (!invokeUnaryDunder(v, "__clone__", line, col)) {
								Value res = deepCopy(v);
								res.isConst = false;
								res.isLocked = false;
								stack.push_back(res);
							}
						}
						DISPATCH();
					}
					OP(OP_GET_INDEX) : {
						{
							Value index = pop();
							Value base = pop();
							auto getSliceIndices = [&](size_t rawLen) -> std::vector<long long> {
								auto *s = static_cast<SliceObject *>(index.ref.get());
								long long len = (long long)rawLen;
								long long step = 1;
								if (s->step.type == ValueType::INT)
									step = s->step.asInt();
								if (step == 0)
									throw ValueError("Slice step cannot be zero", line,
										col);
								long long start, end;
								if (step > 0) {
									start = (s->start.type == ValueType::INT)
												  ? s->start.asInt()
												  : 0;
									end = (s->end.type == ValueType::INT) ? s->end.asInt()
																					  : len;
								} else {
									start = (s->start.type == ValueType::INT)
												  ? s->start.asInt()
												  : len - 1;
									end = (s->end.type == ValueType::INT) ? s->end.asInt()
																					  : -1;
								}
								if (s->start.type == ValueType::INT && start < 0)
									start += len;
								if (s->end.type == ValueType::INT && end < 0)
									end += len;
								if (step > 0) {
									if (start < 0)
										start = 0;
									if (end < 0)
										end = 0;
									if (start > len)
										start = len;
									if (end > len)
										end = len;
								} else {
									if (start > len - 1)
										start = len - 1;
									if (end > len - 1)
										end = len - 1;
									if (start < -1)
										start = -1;
									if (end < -1)
										end = -1;
								}
								std::vector<long long> result;
								if (step > 0) {
									for (long long i = start; i < end; i += step)
										if (i >= 0 && i < len)
											result.push_back(i);
								} else {
									for (long long i = start; i > end; i += step)
										if (i >= 0 && i < len)
											result.push_back(i);
								}
								return result;
							};
							while (base.type == ValueType::REFERENCE) {
								if (!base.get_ptr_safe())
									throw RuntimeError("Dead-end reference", line, col);
								base = *(base.get_ptr_safe());
							}
							if (!invokeBinaryDunder(base, index, "__at__", "", line, col)) {
								switch (base.type) {
								case ValueType::LIST: {
									auto *list = static_cast<ListObject *>(base.ref.get());
									if (index.type == ValueType::SLICE) {
										auto indices =
											getSliceIndices(list->elements.size());
										auto newList = std::make_shared<ListObject>();
										newList->elements.reserve(indices.size());
										for (long long i : indices)
											newList->elements.push_back(list->elements[i]);
										stack.push_back(Value::List(newList->elements));
									} else {
										if (!index.isNumber())
											throw TypeError("List index must be int or slice",
												line, col);
										long long idx = index.asInt();
										if (idx < 0)
											idx += list->elements.size();
										if (idx < 0 ||
											 idx >= (long long)list->elements.size())
											throw IndexError("List index out of range", line,
												col);
										stack.push_back(list->elements[idx]);
									}
									break;
								}
								case ValueType::SET: {
									throw TypeError("Sets are unordered collections and do not support indexing or slicing.", line, col);
								}
								case ValueType::VECTOR: {
									auto *vec = static_cast<VectorObject *>(base.ref.get());
									if (index.type == ValueType::SLICE) {
										auto indices = getSliceIndices(vec->elements.size());
										std::vector<Value> newElems;
										newElems.reserve(indices.size());
										for (long long i : indices)
											newElems.push_back(vec->elements[i]);
										stack.push_back(Value::Vector(newElems));
									} else {
										if (!index.isNumber())
											throw TypeError(
												"Vector index must be int or slice", line,
												col);
										long long idx = index.asInt();
										if (idx < 0)
											idx += vec->elements.size();
										if (idx < 0 ||
											 idx >= (long long)vec->elements.size())
											throw IndexError("Vector index out of range",
												line, col);
										stack.push_back(vec->elements[idx]);
									}
									break;
								}
								case ValueType::TUPLE: {
									auto *tuple = static_cast<TupleObject *>(base.ref.get());
									if (index.type == ValueType::SLICE) {
										auto indices =
											getSliceIndices(tuple->elements.size());
										std::vector<Value> newElems;
										newElems.reserve(indices.size());
										for (long long i : indices)
											newElems.push_back(tuple->elements[i]);
										stack.push_back(Value::Tuple(newElems));
									} else {
										if (!index.isNumber())
											throw TypeError(
												"Tuple index must be int or slice", line,
												col);
										long long idx = index.asInt();
										if (idx < 0)
											idx += tuple->elements.size();
										if (idx < 0 ||
											 idx >= (long long)tuple->elements.size())
											throw IndexError("Tuple index out of range", line,
												col);
										stack.push_back(tuple->elements[idx]);
									}
									break;
								}
								case ValueType::DICT: {
									auto *dict = static_cast<DictObject *>(base.ref.get());
									if (dict->items.count(index) == 0)
										throw KeyError(
											"Key not found: " + valueToString(index), line,
											0);
									stack.push_back(dict->items.at(index));
									break;
								}
								case ValueType::STRING: {
									string s = base.asString();
									if (index.type == ValueType::SLICE) {
										auto indices = getSliceIndices(s.length());
										string newStr = "";
										newStr.reserve(indices.size());
										for (long long i : indices)
											newStr += s[i];
										stack.push_back(Value::String(newStr));
									} else {
										if (!index.isNumber())
											throw TypeError(
												"String index must be int or slice", line,
												col);
										long long idx = index.asInt();
										if (idx < 0)
											idx += s.length();
										if (idx < 0 || idx >= (long long)s.length())
											throw IndexError("String index out of range",
												line, col);
										stack.push_back(Value::String(string(1, s[idx])));
									}
									break;
								}
								case ValueType::RANGE: {
									auto *rng = static_cast<RangeObject *>(base.ref.get());
									if (!index.isNumber())
										throw TypeError("Range index must be a number", line,
											col);
									long long idx = index.asInt();
									long long len =
										(long long)((rng->end - rng->start) / rng->step);
									if (!rng->endInclusive) {
										if ((rng->step > 0 && rng->end > rng->start) ||
											 (rng->step < 0 && rng->end < rng->start))
											len = (long long)ceil((rng->end - rng->start) /
																		 rng->step);
										else
											len = 0;
									} else
										len++;
									if (idx < 0)
										throw IndexError("Range index cannot be negative",
											line, col);
									double val = rng->start + (idx * rng->step);
									if (rng->step > 0 && val >= rng->end &&
										 !rng->endInclusive)
										throw IndexError("Range index out of range", line,
											col);
									stack.push_back(rng->isFloat
															 ? Value::Float(val)
															 : Value::Int((long long)val));
									break;
								}
								default:
									throw TypeError("Object is not subscriptable", line, col);
									break;
								}
							}
						}
						DISPATCH();
					}
					OP(OP_SET_INDEX) : {
						{
							Value val = pop();
							Value index = pop();
							Value base = pop();
							bool dunderFound = false;
							if (base.type == ValueType::INSTANCE) {
								auto *inst = static_cast<InstanceObject *>(base.ref.get());
								ClassObject *cls = inst->klass;
								ClassObject::MethodInfo *putMethod = nullptr;
								ClassObject *methodOwner = nullptr;
								for (auto *ancestor : cls->mro) {
									if (ancestor->methods.count("__put__")) {
										putMethod = &ancestor->methods["__put__"];
										methodOwner = ancestor;
										break;
									}
								}
								if (putMethod) {
									dunderFound = true;
									Value classObjVal;
									classObjVal.type = ValueType::CLASS;
									classObjVal.ref = std::shared_ptr<HeapObject>(methodOwner, [](HeapObject *) {});
									if (putMethod->func.type == ValueType::NATIVE_FUNCTION) {
										auto *nat = static_cast<NativeFunctionObject *>(putMethod->func.ref.get());
										std::vector<Value> args = {base, classObjVal, index, val};
										nat->func(args, line, col);
									} else {
										VM tempVM;
										tempVM.globals = this->globals;
										tempVM.methodResolver = this->methodResolver;
										tempVM.importResolver = this->importResolver;
										Chunk tempChunk;
										int selfIdx = tempChunk.addConstant(base);
										tempChunk.write(OpCode::OP_CONSTANT, line, col);
										tempChunk.write((uint8_t)selfIdx, line, col);
										int objIdx = tempChunk.addConstant(classObjVal);
										tempChunk.write(OpCode::OP_CONSTANT, line, col);
										tempChunk.write((uint8_t)objIdx, line, col);
										int keyIdx = tempChunk.addConstant(index);
										tempChunk.write(OpCode::OP_CONSTANT, line, col);
										tempChunk.write((uint8_t)keyIdx, line, col);
										int valIdx = tempChunk.addConstant(val);
										tempChunk.write(OpCode::OP_CONSTANT, line, col);
										tempChunk.write((uint8_t)valIdx, line, col);
										int methIdx = tempChunk.addConstant(putMethod->func);
										tempChunk.write(OpCode::OP_CONSTANT, line, col);
										tempChunk.write((uint8_t)methIdx, line, col);
										tempChunk.write(OpCode::OP_CALL, line, col);
										tempChunk.write((uint8_t)4, line, col);
										tempChunk.write(OpCode::OP_RETURN, line, col);
										try {
											tempVM.run(tempChunk);
										} catch (...) {
											throw;
										}
									}
									stack.push_back(val);
								}
							}
							if (!dunderFound) {
								if (base.type == ValueType::LIST) {
									auto *list = static_cast<ListObject *>(base.ref.get());
									if (!index.isNumber())
										throw TypeError("List index must be a number", line,
											col);
									long long idx = index.asInt();
									if (idx < 0)
										idx += list->elements.size();
									if (idx < 0 || idx >= (long long)list->elements.size())
										throw IndexError("List assignment index out of range",
											line, col);
									list->elements[idx] = val;
								} else if (base.type == ValueType::DICT) {
									auto *dict = static_cast<DictObject *>(base.ref.get());
									if (index.type == ValueType::LIST ||
										 index.type == ValueType::SET) {
										index = deepCopy(index);
										index.isConst = true;
									}
									dict->items[index] = val;
								} else if (base.type == ValueType::STRING) {
									auto *str = static_cast<StringObject *>(base.ref.get());
									if (!index.isNumber())
										throw TypeError("String index must be a number", line,
											col);
									long long idx = index.asInt();
									if (idx < 0)
										idx += str->value.size();
									if (idx < 0 || idx >= (long long)str->value.size())
										throw IndexError("String assignment index out of range",
											line, col);
									str->value[idx] = val.asString()[0];
								} else if (base.type == ValueType::VECTOR) {
									auto *vec = static_cast<VectorObject *>(base.ref.get());
									if (!index.isNumber())
										throw TypeError("Vector index must be a number", line,
											col);
									long long idx = index.asInt();
									if (idx < 0)
										idx += vec->elements.size();
									if (idx < 0 || idx >= (long long)vec->elements.size())
										throw IndexError("Vector assignment index out of range",
											line, col);
									vec->elements[idx] = val;
								} else if (base.type == ValueType::SET) {
									throw TypeError("Sets are unordered collections and do not support indexing or slicing.", line, col);
								} else if (base.type == ValueType::TUPLE)
									throw MutationError("Tuple object does not support item assignment", line, col);
								else
									throw MutationError("Object does not support item assignment", line, col);
								stack.push_back(val);
							}
						}
						DISPATCH();
					}
										OP(OP_INC_INDEX) : {
						{
							uint8_t isInc = *ip++;
							uint8_t isPrefix = *ip++;
							Value index = pop();
							Value obj = pop();
							
							if (obj.type == ValueType::REFERENCE) {
								if (obj.get_ptr_safe() == nullptr) throw RuntimeError("Null pointer dereference.", line, col);
								obj = *(obj.ptr);
							}
							
							Value oldVal;
							if (obj.type == ValueType::DICT) {
								auto *dictObj = static_cast<DictObject *>(obj.ref.get());
								if (dictObj->items.find(index) != dictObj->items.end()) {
									oldVal = dictObj->items[index];
								} else {
									throw KeyError("Key not found in dict", line, col);
								}
							} else if (obj.type == ValueType::LIST) {
								auto *listObj = static_cast<ListObject *>(obj.ref.get());
								if (index.type != ValueType::INT) throw TypeError("List indices must be integers", line, col);
								long long i = index.iVal;
								if (i < 0) i += listObj->elements.size();
								if (i < 0 || i >= listObj->elements.size()) throw IndexError("List index out of bounds", line, col);
								oldVal = listObj->elements[i];
							} else {
								throw TypeError("Cannot increment index on this object type", line, col);
							}
							
							Value newVal;
							if (oldVal.type == ValueType::INT) {
								newVal = Value::Int(oldVal.iVal + (isInc ? 1 : -1));
							} else if (oldVal.type == ValueType::FLOAT) {
								newVal = Value::Float(oldVal.fVal + (isInc ? 1.0 : -1.0));
							} else {
								throw TypeError("Increment/Decrement requires numeric property", line, col);
							}
							
							if (obj.type == ValueType::DICT) {
								auto *dictObj = static_cast<DictObject *>(obj.ref.get());
								dictObj->items[index] = newVal;
							} else if (obj.type == ValueType::LIST) {
								auto *listObj = static_cast<ListObject *>(obj.ref.get());
								long long i = index.iVal;
								if (i < 0) i += listObj->elements.size();
								listObj->elements[i] = newVal;
							}
							
							stack.push_back(isPrefix ? newVal : oldVal);
						}
						DISPATCH();
					}
OP(OP_INVOKE) : {
						{
							uint8_t nameIdx = *ip++;
							uint8_t argCount = *ip++;
							string methodName = currentChunk->constants[nameIdx].asString();
							Value receiver = stack[stack.size() - 1 - argCount];
							bool shouldDispatch = false;

							if (receiver.type == ValueType::INSTANCE ||
								 receiver.type == ValueType::CLASS ||
								 receiver.type == ValueType::SUPER) {

								Value methodToCall;
								Value selfVal = Value::None();
								Value objVal = Value::None();
								ClassObject *startClass = nullptr;

								if (receiver.type == ValueType::SUPER) {
									auto *superObj = static_cast<SuperObject *>(receiver.ref.get());
									startClass = superObj->startClass;
									selfVal = superObj->instance;
								} else if (receiver.type == ValueType::INSTANCE) {
									auto *instance = static_cast<InstanceObject *>(receiver.ref.get());
									startClass = instance->klass;
									selfVal = receiver;
								} else {
									startClass = static_cast<ClassObject *>(receiver.ref.get());
									selfVal = Value::None();
								}

								ClassObject *methodOwner = nullptr;
								ClassObject::MethodInfo *methodInfo = nullptr;

								if (receiver.type == ValueType::INSTANCE) {
									auto *inst = static_cast<InstanceObject *>(receiver.ref.get());
									if (inst->fields.count(methodName)) {
										Value dynamicMethod = inst->fields[methodName];
										vector<Value> args(argCount);
										for (int i = argCount - 1; i >= 0; i--)
											args[i] = pop();
										pop();
										for (const auto &v : args)
											stack.push_back(v);
										callValue(dynamicMethod, argCount, line, col);
										shouldDispatch = true;
									}
								}

								if (!shouldDispatch) {
									for (auto *ancestor : startClass->mro) {
										if (ancestor->methods.count(methodName)) {
											methodOwner = ancestor;
											methodInfo = &ancestor->methods[methodName];
											break;
										}
									}

									if (!methodOwner || !methodInfo) {
										bool foundStatic = false;
										for (auto *ancestor : startClass->mro) {
											if (ancestor->staticFields.count(methodName)) {
												Value staticCallable = ancestor->staticFields[methodName];
												vector<Value> tempArgs(argCount);
												for (int i = argCount - 1; i >= 0; i--)
													tempArgs[i] = pop();
												pop();
												for (const auto &v : tempArgs)
													stack.push_back(v);
												callValue(staticCallable, argCount, line, col);
												foundStatic = true;
												shouldDispatch = true;
												break;
											}
										}
										if (!foundStatic)
											throw AttributeError("'" + startClass->name + "' object has no attribute '" + methodName + "'", line, col);
									}

									if (!shouldDispatch) {
										if (methodInfo->access != AccessLevel::PUBLIC) {
											bool allowed = false;
											if (frame->basePointer + 1 < stack.size()) {
												Value potentialCaller = stack[frame->basePointer + 1];
												if (potentialCaller.type == ValueType::CLASS) {
													auto *callerPtr = static_cast<ClassObject *>(potentialCaller.ref.get());
													if (methodInfo->access == AccessLevel::PRIVATE) {
														if (callerPtr == methodOwner)
															allowed = true;
													} else if (methodInfo->access == AccessLevel::PROTECTED) {
														for (auto *ancestor : callerPtr->mro) {
															if (ancestor == methodOwner) {
																allowed = true;
																break;
															}
														}
													}
												}
											}
											if (!allowed) {
												throw RuntimeError("Cannot access method '" + methodName + "' from this context.", line, col);
											}
										}

										methodToCall = methodInfo->func;
										objVal.type = ValueType::CLASS;
										objVal.ref = std::shared_ptr<HeapObject>(methodOwner, [](HeapObject *) {});

										vector<Value> args(argCount);
										for (int i = argCount - 1; i >= 0; i--)
											args[i] = pop();
										pop();
										stack.push_back(selfVal);
										stack.push_back(objVal);
										for (const auto &v : args)
											stack.push_back(v);

										callValue(methodToCall, argCount + 2, line, col);
										shouldDispatch = true;
									}
								}
							}

							if (!shouldDispatch) {
								// Fallback: Dummy AST execution
								vector<ValueExpr *> dummyArgs(argCount);
								for (int i = argCount - 1; i >= 0; i--)
									dummyArgs[i] = new ValueExpr(pop());
								Value targetVal = pop();
								ValueExpr *dummyObject = new ValueExpr(targetVal);

								vector<Expr *> exprArgs(dummyArgs.begin(), dummyArgs.end());
								MethodCallExpr mockAST(dummyObject, methodName, exprArgs);
								mockAST.line = line;

								if (!methodResolver) {
									delete dummyObject;
									for (auto *a : dummyArgs)
										delete a;
									throw EnvironmentError("VM methodResolver bridge not initialized.", line, col);
								}

								Value result;
								try {
									result = methodResolver(&mockAST);
								} catch (...) {
									delete dummyObject;
									for (auto *a : dummyArgs)
										delete a;
									throw;
								}

								delete dummyObject;
								for (auto *a : dummyArgs)
									delete a;

								stack.push_back(result);
							}
						}
						DISPATCH();
					}
					OP(OP_TRY_ENTER) : {
						{
							uint8_t cHi = *ip++;
							uint8_t cLo = *ip++;
							uint8_t fHi = *ip++;
							uint8_t fLo = *ip++;
							int catchOffset = (cHi << 8) | cLo;
							int finallyOffset = (fHi << 8) | fLo;
							ExceptionHandler h;
							h.stackDepth = stack.size();
							int currentOffset = (int)(ip - 4 - currentChunk->code.data());
							h.catchAddress = currentOffset + catchOffset;
							h.finallyAddress =
								(finallyOffset == 0) ? -1 : currentOffset + finallyOffset;
							frame->handlerStack.push_back(h);
						}
						DISPATCH();
					}

					OP(OP_TRY_EXIT) : {
						{
							if (!frame->handlerStack.empty())
								frame->handlerStack.pop_back();
						}
						DISPATCH();
					}
					OP(OP_THROW) : {
						{
							Value typeVal = pop();
							Value msgVal = pop();
							string t = typeVal.asString();
							string m = msgVal.asString();
							static const std::unordered_map<
								std::string, std::function<void(string, int, int)>>
								errorFactory = {
									// Base Categories
									{"InternalError",
										[](string m, int l, int c) {
											throw InternalError(m, l, c);
										}},
									{"ControlFlowError",
										[](string m, int l, int c) {
											throw ControlFlowError(m, l, c);
										}},
									{"ParseError",
										[](string m, int l, int c) {
											throw ParseError(m, l, c);
										}},
									{"RuntimeError",
										[](string m, int l, int c) {
											throw RuntimeError(m, l, c);
										}},
									{"Warning", [](string m, int l,
														int c) { throw Warning(m, l, c); }},

									// Runtime Errors
									{"NameError",
										[](string m, int l, int c) {
											throw NameError(m, l, c);
										}},
									{"AttributeError",
										[](string m, int l, int c) {
											throw AttributeError(m, l, c);
										}},
									{"TypeError",
										[](string m, int l, int c) {
											throw TypeError(m, l, c);
										}},
									{"ArgumentError",
										[](string m, int l, int c) {
											throw ArgumentError(m, l, c);
										}},
									{"ValueError",
										[](string m, int l, int c) {
											throw ValueError(m, l, c);
										}},
									{"ConstError",
										[](string m, int l, int c) {
											throw ConstError(m, l, c);
										}},
									{"OwnershipError",
										[](string m, int l, int c) {
											throw OwnershipError(m, l, c);
										}},
									{"IndexError",
										[](string m, int l, int c) {
											throw IndexError(m, l, c);
										}},
									{"KeyError", [](string m, int l,
														 int c) { throw KeyError(m, l, c); }},
									{"RangeError",
										[](string m, int l, int c) {
											throw RangeError(m, l, c);
										}},
									{"AssertionError",
										[](string m, int l, int c) {
											throw AssertionError(m, l, c);
										}},
									{"RecursionError",
										[](string m, int l, int c) {
											throw RecursionError(m, l, c);
										}},
									{"ImportError",
										[](string m, int l, int c) {
											throw ImportError(m, l, c);
										}},
									{"IOError", [](string m, int l,
														int c) { throw IOError(m, l, c); }},
									{"MathError",
										[](string m, int l, int c) {
											throw MathError(m, l, c);
										}},
									{"CastError",
										[](string m, int l, int c) {
											throw CastError(m, l, c);
										}},
									{"IteratorError",
										[](string m, int l, int c) {
											throw IteratorError(m, l, c);
										}},
									{"TimeoutError",
										[](string m, int l, int c) {
											throw TimeoutError(m, l, c);
										}},
									{"MemoryError",
										[](string m, int l, int c) {
											throw MemoryError(m, l, c);
										}},
									{"SystemError",
										[](string m, int l, int c) {
											throw SystemError(m, l, c);
										}},

									// Specific Subtypes
									{"EmptyContainerError",
										[](string m, int l, int c) {
											throw EmptyContainerError(m, l, c);
										}},
									{"MutationError",
										[](string m, int l, int c) {
											throw MutationError(m, l, c);
										}},
									{"ModuleNotFoundError",
										[](string m, int l, int c) {
											throw ModuleNotFoundError(m, l, c);
										}},
									{"CircularImportError",
										[](string m, int l, int c) {
											throw CircularImportError(m, l, c);
										}},
									{"InvalidImportError",
										[](string m, int l, int c) {
											throw InvalidImportError(m, l, c);
										}},
									{"FileNotFoundError",
										[](string m, int l, int c) {
											throw FileNotFoundError(m, l, c);
										}},
									{"PermissionError",
										[](string m, int l, int c) {
											throw PermissionError(m, l, c);
										}},
									{"EOFError", [](string m, int l,
														 int c) { throw EOFError(m, l, c); }},
									{"FileClosedError",
										[](string m, int l, int c) {
											throw FileClosedError(m, l, c);
										}},
									{"DivisionByZeroError",
										[](string m, int l, int c) {
											throw DivisionByZeroError(m, l, c);
										}},
									{"OverflowError",
										[](string m, int l, int c) {
											throw OverflowError(m, l, c);
										}},
									{"UnderflowError",
										[](string m, int l, int c) {
											throw UnderflowError(m, l, c);
										}},
									{"DomainError",
										[](string m, int l, int c) {
											throw DomainError(m, l, c);
										}},
									{"OSError", [](string m, int l,
														int c) { throw OSError(m, l, c); }},
									{"EnvironmentError",
										[](string m, int l, int c) {
											throw EnvironmentError(m, l, c);
										}},
									{"SignalError",
										[](string m, int l, int c) {
											throw SignalError(m, l, c);
										}},
									{"DeprecationWarning",
										[](string m, int l,
											int c) { throw DeprecationWarning(m, l, c); }},
									{"RuntimeWarning",
										[](string m, int l,
											int c) { throw RuntimeWarning(m, l, c); }},
									{"ImportWarning", [](string m, int l, int c) {
										 throw ImportWarning(m, l, c);
									 }}};
							auto it = errorFactory.find(t);
							if (it != errorFactory.end())
								it->second(m, line, col);
							else
								throw LangError(t, m, -1, line, col);
						}
						DISPATCH();
					}
					OP(OP_CATCH) : {
						{
							uint8_t count = *ip++;
							Value errorVal = stack.back();
							bool match = false;
							static const std::unordered_map<std::string, std::string>
								parentMap = {// MathError Subtree
									{"DivisionByZeroError", "MathError"},
									{"OverflowError", "MathError"},
									{"UnderflowError", "MathError"},
									{"DomainError", "MathError"},
									{"MathError", "RuntimeError"},

									// IOError Subtree
									{"FileNotFoundError", "IOError"},
									{"PermissionError", "IOError"},
									{"EOFError", "IOError"},
									{"FileClosedError", "IOError"},
									{"IOError", "RuntimeError"},

									// ImportError Subtree
									{"ModuleNotFoundError", "ImportError"},
									{"CircularImportError", "ImportError"},
									{"InvalidImportError", "ImportError"},
									{"ImportError", "RuntimeError"},

									// ValueError/ConstError Subtrees
									{"EmptyContainerError", "ValueError"},
									{"ValueError", "RuntimeError"},
									{"MutationError", "ConstError"},
									{"ConstError", "RuntimeError"},

									// SystemError Subtree
									{"OSError", "SystemError"},
									{"EnvironmentError", "SystemError"},
									{"SignalError", "SystemError"},
									{"SystemError", "RuntimeError"},

									// ParseError Subtree
									{"SyntaxError", "ParseError"},
									{"IndentationError", "ParseError"},
									{"UnexpectedTokenError", "ParseError"},
									{"UnterminatedLiteralError", "ParseError"},
									{"ParseError", "Error"},

									// ControlFlowError Subtree
									{"ReturnSignal", "ControlFlowError"},
									{"BreakSignal", "ControlFlowError"},
									{"ContinueSignal", "ControlFlowError"},
									{"ControlFlowError", "Error"},

									// Warning Subtree
									{"DeprecationWarning", "Warning"},
									{"RuntimeWarning", "Warning"},
									{"ImportWarning", "Warning"},
									{"Warning", "Error"},

									// Direct RuntimeError Children
									{"NameError", "RuntimeError"},
									{"AttributeError", "RuntimeError"},
									{"TypeError", "RuntimeError"},
									{"ArgumentError", "RuntimeError"},
									{"OwnershipError", "RuntimeError"},
									{"IndexError", "RuntimeError"},
									{"KeyError", "RuntimeError"},
									{"RangeError", "RuntimeError"},
									{"AssertionError", "RuntimeError"},
									{"RecursionError", "RuntimeError"},
									{"CastError", "RuntimeError"},
									{"IteratorError", "RuntimeError"},
									{"TimeoutError", "RuntimeError"},
									{"MemoryError", "RuntimeError"},

									// Roots
									{"RuntimeError", "Error"},
									{"InternalError", "Error"}};
							if (errorVal.type == ValueType::ERROR) {
								auto *errObj =
									static_cast<ErrorObject *>(errorVal.ref.get());
								for (int i = 0; i < count; i++) {
									uint8_t typeIdx = *ip++;
									if (match)
										continue;
									string catchType =
										currentChunk->constants[typeIdx].asString();
									string currentType = errObj->errType;
									while (true) {
										if (currentType == catchType) {
											match = true;
											break;
										}
										if (catchType == "Error") {
											match = true;
											break;
										}
										auto it = parentMap.find(currentType);
										if (it == parentMap.end())
											break;
										currentType = it->second;
									}
								}
							} else
								ip += count;
							stack.push_back(Value::Bool(match));
						}
						DISPATCH();
					}
					OP(OP_RETHROW) : {
						{
							Value errorVal = pop();
							if (errorVal.type == ValueType::ERROR) {
								auto *errObj =
									static_cast<ErrorObject *>(errorVal.ref.get());
								throw LangError(errObj->errType, errObj->message,
									errObj->code, errObj->line, errObj->col);
							}
						}
						DISPATCH();
					}
					OP(OP_END_FINALLY) : {
						bool goToReturn = false;
						{
							if (isHandlingError) {
								isHandlingError = false;
								auto *errObj =
									static_cast<ErrorObject *>(pendingError.ref.get());
								throw LangError(errObj->errType, errObj->message,
									errObj->code, errObj->line, errObj->col);
							}
							if (isReturning) {
								isReturning = false;
								stack.push_back(pendingReturn);
								goToReturn = true;
							}
						}
						if (goToReturn) {
							goto execute_return;
						}
						DISPATCH();
					}
					OP(OP_DELETE) : {
						{
							Value refVal = pop();
							if (refVal.type != ValueType::REFERENCE || !refVal.get_ptr_safe())
								throw RuntimeError("Cannot delete a non-reference value",
									line, col);
							Value actualVal = *(refVal.ptr);
							*(refVal.ptr) = Value::NoType();
							if (actualVal.type == ValueType::INSTANCE) {
								auto *instance =
									static_cast<InstanceObject *>(actualVal.ref.get());
								ClassObject *cls = instance->klass;
								ClassObject::MethodInfo *delMethod = nullptr;
								for (auto *ancestor : cls->mro) {
									if (ancestor->methods.count("__destruct__")) {
										delMethod = &ancestor->methods["__destruct__"];
										break;
									}
								}
								if (delMethod) {
									stack.push_back(actualVal);
									Value objVal;
									objVal.type = ValueType::CLASS;
									objVal.ref = std::shared_ptr<HeapObject>(
										cls, [](HeapObject *) {});
									stack.push_back(objVal);
									callValue(delMethod->func, 2, line, col);
									DISPATCH();
								}
							}
							stack.push_back(Value::None());
						}
						DISPATCH();
					}
					OP(OP_RETURN) : {
					execute_return: // <--- Safely outside the Scope Lock!
						bool shouldReturnFromVM = false;
						{
							bool interceptedByFinally = false;
							if (!frame->handlerStack.empty()) {
								ExceptionHandler &h = frame->handlerStack.back();
								if (h.finallyAddress != -1 && !h.isInsideFinally) {
									isReturning = true;
									pendingReturn = pop();
									h.isInsideFinally = true;
									ip = currentChunk->code.data() + h.finallyAddress;
									interceptedByFinally = true; // Triggers dispatch instead of return logic
								}
							}
							if (!interceptedByFinally) {
								Value result = pop();
								FunctionObject *func = frame->function;
								if (func) {
									if (func->name == magic_methods_to_string(
																Magic_Methods::__construct__)) {
										result = stack[frame->basePointer];
									} else {
										if (result.type == ValueType::NOTYPE) {
											if (!func->defaultRetArgs.empty()) {
												result =
													executeDefault(func->defaultRetChunks.empty() ? nullptr : func->defaultRetChunks[0], line);
											} else if (func->returnType != ValueType::NOTYPE) {
												result = Value::None();
											} else {
												result = Value::None();
											}
										}
									}
									if (func->returnType != ValueType::NOTYPE &&
										 result.type != func->returnType) {
										bool converted = false;
										if (func->returnType == ValueType::FLOAT &&
											 result.type == ValueType::INT) {
											result = Value::Float((double)result.asInt());
											converted = true;
										} else if (func->returnType == ValueType::INT &&
													  result.type == ValueType::FLOAT) {
											result = Value::Int((long long)result.asFloat());
											converted = true;
										} else if (func->returnType == ValueType::INT &&
													  result.type == ValueType::BIGINT) {
											converted = true;
										} else if (func->returnType == ValueType::BIGINT &&
													  result.type == ValueType::INT) {
											result = Value::BigInt(result.asInt());
											converted = true;
										}
										if (!converted) {
											string typeName = "";
											switch (func->returnType) {
											case ValueType::INT:
												typeName = "int";
												break;
											case ValueType::FLOAT:
												typeName = "float";
												break;
											case ValueType::BOOL:
												typeName = "bool";
												break;
											case ValueType::STRING:
												typeName = "string";
												break;
											case ValueType::LIST:
												typeName = "list";
												break;
											case ValueType::SET:
												typeName = "set";
												break;
											case ValueType::DICT:
												typeName = "dict";
												break;
											case ValueType::TUPLE:
												typeName = "tuple";
												break;
											case ValueType::VECTOR:
												typeName = "vector";
												break;
											case ValueType::RANGE:
												typeName = "range";
												break;
											default:
												break;
											}
											if (!typeName.empty() && globals->exists(typeName)) {
												Value ctor = globals->get(typeName);
												vector<Value> args;
												if (result.type != ValueType::NONE &&
													 result.type != ValueType::NOTYPE) {
													args.push_back(result);
												}
												try {
													if (ctor.type == ValueType::NATIVE_FUNCTION) {
														auto *nat =
															static_cast<NativeFunctionObject *>(
																ctor.ref.get());
														result = nat->func(args, line, 0);
														converted = true;
													} else if (ctor.type == ValueType::OVERLOAD) {
														auto *ov = static_cast<OverloadObject *>(
															ctor.ref.get());
														for (const auto &v : ov->overloads) {
															if (v.type ==
																 ValueType::NATIVE_FUNCTION) {
																auto *nat = static_cast<
																	NativeFunctionObject *>(
																	v.ref.get());
																try {
																	result = nat->func(args, line, 0);
																	converted = true;
																	break;
																} catch (...) {
																}
															}
														}
													}
												} catch (...) {
												}
											}
										}
										if (!converted && result.type != ValueType::NONE) {
											throw TypeError(
												"Return type mismatch. Expected " +
													std::to_string((int)func->returnType) +
													" Got " + std::to_string((int)result.type),
												line, col);
										}
									}
									if (func->returnsConst)
										result.isConst = true;
									if (func->isCached) {
										if (frame->singleCacheKey.type != ValueType::NOTYPE) {
											func->singleArgCache[frame->singleCacheKey] = result;
										} else {
											func->cache[frame->cacheKey] = result;
										}
									}
								}
								int returnSlot = frame->basePointer;
								frames.pop_back();
								if (frames.empty()) {
									stack.push_back(result);
									shouldReturnFromVM = true;
								} else {
									frame = &frames.back();
									ip = frame->ip;
									stack.resize(returnSlot);
									stack.push_back(result);
								}
							}
						}
						if (shouldReturnFromVM) {
							return;
						}
						DISPATCH();
					}
					OP(OP_MULTI_SET) : OP(OP_AND) : OP(OP_OR) : OP(OP_IN) : OP(OP_INCREMENT) : OP(OP_DECREMENT) : OP(OP_PRE_INCREMENT) : OP(OP_PRE_DECREMENT) : OP(OP_BUILD_FILE) : OP(OP_BREAK) : OP(OP_CONTINUE) : OP(OP_SKIP) : OP(OP_OMIT) : OP(OP_ASSERT) : {
						throw InternalError("Optimized or unimplemented OpCode was executed!", line, col);
						DISPATCH();
					}
#ifndef USE_COMPUTED_GOTOS
				default:
					throw UnexpectedTokenError("Unknown OpCode encountered", line, col);
#endif
				}
			} catch (const LangError &e) {
				if (e.type == "Warning" || e.type == "ImportWarning" || e.type == "RuntimeWarning" || e.type == "DeprecationWarning") {
					std::cerr << "\033[1;36m" << e.type << ": " << e.message << "\033[0m\n";
				} else {
					if (frame->handlerStack.empty()) {
						if (e.type != "IteratorError") {
							std::cerr << "Uncaught " << e.type << ": " << e.message << "\n"; printf("tempVM is UNCAUGHT THROWING %s\n", e.type.c_str());
						}
						throw;
					}
					ExceptionHandler h = frame->handlerStack.back();
					frame->handlerStack.pop_back();
					while ((int)stack.size() > h.stackDepth)
						stack.pop_back();
					auto errObj = std::make_shared<ErrorObject>(
						e.type, e.message, e.code, e.line, e.col);
					Value errVal = Value::Error(errObj);
					if (h.catchAddress != -1) {
						stack.push_back(errVal);
						Chunk *currentChunk =
							frame->function ? frame->function->chunk : &chunk;
						ip = currentChunk->code.data() + h.catchAddress;
					} else if (h.finallyAddress != -1) {
						pendingError = errVal;
						isHandlingError = true;
						Chunk *currentChunk =
							frame->function ? frame->function->chunk : &chunk;
						ip = currentChunk->code.data() + h.finallyAddress;
					} else {
						throw;
					}
				}
			}
		}
	}

