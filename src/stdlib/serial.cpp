#include "ylang/vm.hpp"
#include "objects.hpp"
#include "types.hpp"

#ifdef _WIN32
#include <shellapi.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif

void Interpreter::registerSerialModule() {
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
				char *buf = new char[n + 1];
				DWORD bytesRead = 0;
				ReadFile(hComm, buf, n, &bytesRead, NULL);
				buf[bytesRead] = '\0';
				string res(buf, bytesRead);
				delete[] buf;
				return Value::String(res);
#else
				int fd = (int)(intptr_t)self->fields["ptr"].aspInt();
				char *buf = new char[n + 1];
				int bytesRead = ::read(fd, buf, n);
				if (bytesRead < 0)
					bytesRead = 0;
				buf[bytesRead] = '\0';
				string res(buf, bytesRead);
				delete[] buf;
				return Value::String(res);
#endif
			}),
			.access = AccessLevel::PUBLIC};
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
			.access = AccessLevel::PUBLIC};
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
			.access = AccessLevel::PUBLIC};
		define("open", [=](const vector<Value> &args, int l, int c) -> Value {
			string port = args[0].asString();
			int baud = args[1].asInt();
			auto inst = std::make_shared<InstanceObject>(serialClass.get());
#ifdef _WIN32
			HANDLE hComm = CreateFileA(port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (hComm == INVALID_HANDLE_VALUE)
				throw RuntimeError("Failed to open port", l, c);
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
			inst->fields["ptr"] = Value::pInt((void *)hComm);
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
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		});
		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Serial", modVal, false, false);
	};
}
