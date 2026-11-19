#include "ylang/vm.hpp"
#include "objects.hpp"
#include "types.hpp"

#ifdef _WIN32
#include <shellapi.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#if !defined(_WIN32) && defined(YLANG_HAS_XTEST)
#define Font X11_Font
#define Status X11_Status
#define Success X11_Success
#define None X11_None
#define Bool X11_Bool
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#undef Font
#undef Status
#undef Success
#undef None
#undef Bool
#elif !defined(_WIN32)
// shortcut: no X11/XTest here (macOS, headless Linux), so the Events module is a no-op; port to Quartz events for macOS support
typedef struct _XDisplay Display;
typedef unsigned char KeyCode;
#define CurrentTime 0L
static inline Display *XOpenDisplay(const char *) { return nullptr; }
static inline int XFlush(Display *) { return 0; }
static inline KeyCode XKeysymToKeycode(Display *, unsigned long) { return 0; }
static inline int XQueryKeymap(Display *, char *) { return 0; }
static inline int XTestFakeKeyEvent(Display *, unsigned, int, unsigned long) { return 0; }
static inline int XTestFakeButtonEvent(Display *, unsigned, int, unsigned long) { return 0; }
static inline int XTestFakeMotionEvent(Display *, int, int, int, unsigned long) { return 0; }
#endif


void Interpreter::registerEventsModule() {
	modules["Events"] = [this](std::shared_ptr<Env> env, const vector<string> &symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Events");
		moduleNamespace->mro.push_back(moduleNamespace.get());
		auto define = [&](string name, NativeFunc f) {
			moduleNamespace->staticFields[name] = Value::Native(f);
		};

#ifndef _WIN32
		// Setup global display for XTest
		static Display *x_display = XOpenDisplay(NULL);
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
			INPUT input = {0};
			input.type = INPUT_MOUSE;
			if (btn == 0)
				input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			else if (btn == 1)
				input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
			else if (btn == 2)
				input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
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
			INPUT input = {0};
			input.type = INPUT_MOUSE;
			if (btn == 0)
				input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
			else if (btn == 1)
				input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
			else if (btn == 2)
				input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
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
			inputs[0].type = INPUT_MOUSE;
			inputs[1].type = INPUT_MOUSE;
			if (btn == 0) {
				inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
				inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
			} else if (btn == 1) {
				inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
				inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
			} else if (btn == 2) {
				inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
				inputs[1].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
			}
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
				inputs[0].type = INPUT_MOUSE;
				inputs[1].type = INPUT_MOUSE;
				if (btn == 0) {
					inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
					inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
				} else if (btn == 1) {
					inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
					inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
				} else if (btn == 2) {
					inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
					inputs[1].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
				}
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

		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Events", modVal, false, false);
	};
}
