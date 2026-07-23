#pragma once
#ifndef YLANG_TYPES_HPP
#define YLANG_TYPES_HPP
#define _CRT_SECURE_NO_WARNINGS
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define TokenType Win_TokenType
#define Rectangle Win_Rectangle
#define ShowCursor Win_ShowCursor
#define CloseWindow Win_CloseWindow
#define DrawText Win_DrawText
#define DrawTextEx Win_DrawTextEx
#define DrawTextW Win_DrawTextW
#define LoadImageW Win_LoadImageW
#define LoadImage Win_LoadImage
#define DELETE Win_DELETE
#include <windows.h> // My attacks have no effect on you?
#undef DELETE
#undef Rectangle
#undef ShowCursor
#undef CloseWindow
#undef DrawText
#undef DrawTextW
#undef DrawTextEx
#undef LoadImageW
#undef TokenType
#undef TRUE
#undef FALSE
#undef CONST
#undef IN
#undef ERROR
#undef _TOKEN_INFORMATION_CLASS
#undef min
#undef max
#undef LoadImage
#include <conio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else // Who decided that?
#include <dlfcn.h>
#include <termios.h>
#include <fcntl.h>
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
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif // !ENABLE_VIRTUAL_TERMINAL_PROCESSING
#include "pystring.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <locale.h>
#include <map>
#include <math.h>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <regex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "crypto.h"
#ifdef _MSC_VER
#include <intrin.h>
#elif defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#elif defined(__ARM_NEON)
#include <arm_neon.h>
#endif
#ifndef _WIN32
#define TRUE N_TRUE
#define FALSE N_FALSE
#include "ncurses.h"
#undef TRUE
#undef FALSE
#endif
#include <deque>
#define KEY_ENTER RAY_ENTER
#define KEY_HOME RAY_HOME
#define KEY_RIGHT RAY_RIGHT
#define KEY_LEFT RAY_LEFT
#define KEY_DOWN RAY_DOWN
#define KEY_UP RAY_UP
#define KEY_BACKSPACE RAY_BACKSPACE
#define KEY_END RAY_END
#define Matrix RaylibMatrix
#include "raylib.h"
#undef KEY_ENTER
#undef KEY_HOME
#undef KEY_RIGHT
#undef KEY_LEFT
#undef KEY_UP
#undef KEY_DOWN
#undef KEY_BACKSPACE
#undef KEY_END
#undef Matrix
#include "qrcodegen.hpp"
#include <nlohmann/json.hpp>
#ifdef _WIN32
#define IN
#define CONST const
#endif
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#ifdef _WIN32
#undef IN
#undef CONST
#endif
#undef DELETE
#undef DEFAULT
#undef MAGENTA
#undef CYAN
#undef YELLOW
#undef GREEN
#undef BLACK
#undef RED
#undef BROWN
#undef BLUE
#undef LIGHTGRAY
#undef GRAY
#undef WHITE
#define REPLXX_STATIC
#include <replxx.hxx>
#include <opennn/opennn.h>
extern bool DEBUGGER_MODE_IS_ENABLED;
namespace fs = std::filesystem;
using std::string;
using std::unordered_map;
using std::vector;
using json = nlohmann::json;
using namespace qrcodegen;
// -------------------- TOKENIZER --------------------
enum class ValueType {
	NOTYPE,
	NONE,
	INT,
	FLOAT,
	STRING,
	BOOL,
	LIST,
	VECTOR,
	DICT,
	SLICE,
	BIGINT,
	REFERENCE,
	PAIRED,
	SUPER,
	RANGE,
	TUPLE,
	SET,
	FUNCTION,
	NATIVE_FUNCTION,
	FILE,
	OVERLOAD,
	OMIT_MARKER,
	ERROR,
	CLASS,
	INSTANCE
};
enum class CopyMode { SHALLOW,
	DEEP,
	REF };
// -------------------- AST --------------------
enum class AccessLevel {
	PUBLIC,
	PRIVATE,
	PROTECTED
};
enum class ExprType {
	FSTRING,
	OWNERSHIP,
	BOOL,
	NUMBER,
	STRING,
	VAR,
	LIST,
	BINARY,
	CALL,
	INDEX,
	METHOD_CALL,
	LAMBDA,
	RANGE,
	SET,
	DICT,
	TUPLE,
	TERNARY,
	COMPREHENSION,
	SLICE,
	VECTOR,
	OMIT_MARKER_EXPR,
	GET,
	INC_DEC
};
enum class StmtType {
	ASSIGN,
	LET,
	RETURN,
	FUNC,
	IF,
	EXPR,
	BREAK,
	CONTINUE,
	SKIP,
	WHILE,
	DO_WHILE,
	FOR,
	FOR_EACH,
	TRY,
	THROW,
	ASSERT,
	SWITCH,
	IMPORT,
	MULTI_LET,
	MULTI_ASSIGN,
	CLASS,
	DELETE,
};

#endif // YLANG_TYPES_HPP
