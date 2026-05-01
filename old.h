#pragma once
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
#else // Who decided that?
#include <dlfcn.h>
#include <termios.h>
#include <unistd.h>
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
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
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
#include "raylib.h"
#undef KEY_ENTER
#undef KEY_HOME
#undef KEY_RIGHT
#undef KEY_LEFT
#undef KEY_UP
#undef KEY_DOWN
#undef KEY_BACKSPACE
#undef KEY_END
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
bool DEBUGGER_MODE_IS_ENABLED = false;
namespace fs = std::filesystem;
using std::string;
using std::unordered_map;
using std::vector;
using json = nlohmann::json;
using namespace qrcodegen;
// -------------------- TOKENIZER --------------------
enum class TokenType {
	LET,
	DEFINE,
	FUNCTION,
	RETURN,
	IF,
	ELSE_IF,
	ELSE,
	FOR,
	WHILE,
	DO,
	THEN,
	BREAK,
	CONTINUE,
	SKIP,
	DELETE,
	TRUE,
	FALSE,
	AT,
	DOLLAR,
	DOT_DOT,
	DOT_DOT_DOT,
	CONST,
	ASSERT,
	FSTRING,
	SWITCH,
	CASE,
	DEFAULT,
	IDENTIFIER,
	NUMBER,
	STRING,
	INCREMENT,
	DECREMENT,
	TRY,
	THROW,
	CATCH,
	FINALLY,
	IMPORT,
	FROM,
	ASSIGN,
	ARROW,
	LPAREN,
	RPAREN,
	COLON,
	COMMA,
	LBRACE,
	RBRACE,
	LBRACKET,
	RBRACKET,
	COLON_EQ,
	HASHTAG,
	PLUS,
	MINUS,
	STAR,
	SLASH,
	DOT,
	PLUS_EQ,
	MINUS_EQ,
	STAR_EQ,
	DIV_EQ,
	FLOOR_DIV,
	FLOOR_DIV_EQ,
	MOD,
	MOD_EQ,
	GT,
	LT,
	EQ,
	STRICT_EQ,
	STRICT_NEQ,
	GTE,
	LTE,
	NEQ,
	POW,
	POW_EQ,
	IS,
	IN,
	IS_IN,
	IS_NOT,
	IS_NOT_IN,
	PROTECTED,
	AND,
	OR,
	NOT,
	XOR,
	NAND,
	NOR,
	NXOR,
	AND_EQ,
	OR_EQ,
	XOR_EQ,
	CACHED,
	LAMBDA,
	OMIT,
	CLASS,
	INHERITS,
	PUBLIC,
	PRIVATE,
	END
};
struct Token {
	TokenType type;
	string value;
	int line;
	int col;
};
inline TokenType keywordType(const string &w) {
	if (w == "let")
		return TokenType::LET;
	if (w == "define")
		return TokenType::DEFINE;
	if (w == "function")
		return TokenType::FUNCTION;
	if (w == "return")
		return TokenType::RETURN;
	if (w == "if")
		return TokenType::IF;
	if (w == "elif")
		return TokenType::ELSE_IF;
	if (w == "else")
		return TokenType::ELSE;
	if (w == "for")
		return TokenType::FOR;
	if (w == "while")
		return TokenType::WHILE;
	if (w == "do")
		return TokenType::DO;
	if (w == "then")
		return TokenType::THEN;
	if (w == "break")
		return TokenType::BREAK;
	if (w == "continue")
		return TokenType::CONTINUE;
	if (w == "skip")
		return TokenType::SKIP;
	if (w == "delete")
		return TokenType::DELETE;
	if (w == "true")
		return TokenType::TRUE;
	if (w == "false")
		return TokenType::FALSE;
	if (w == "and")
		return TokenType::AND;
	if (w == "or")
		return TokenType::OR;
	if (w == "not")
		return TokenType::NOT;
	if (w == "xor")
		return TokenType::XOR;
	if (w == "nand")
		return TokenType::NAND;
	if (w == "nor")
		return TokenType::NOR;
	if (w == "nxor")
		return TokenType::NXOR;
	if (w == "cached")
		return TokenType::CACHED;
	if (w == "lambda")
		return TokenType::LAMBDA;
	if (w == "omit")
		return TokenType::OMIT;
	if (w == "is")
		return TokenType::IS;
	if (w == "in")
		return TokenType::IN;
	if (w == "const")
		return TokenType::CONST;
	if (w == "try")
		return TokenType::TRY;
	if (w == "catch")
		return TokenType::CATCH;
	if (w == "throw")
		return TokenType::THROW;
	if (w == "finally")
		return TokenType::FINALLY;
	if (w == "assert")
		return TokenType::ASSERT;
	if (w == "switch")
		return TokenType::SWITCH;
	if (w == "case")
		return TokenType::CASE;
	if (w == "default")
		return TokenType::DEFAULT;
	if (w == "import")
		return TokenType::IMPORT;
	if (w == "from")
		return TokenType::FROM;
	if (w == "class")
		return TokenType::CLASS;
	if (w == "inherits")
		return TokenType::INHERITS;
	if (w == "public")
		return TokenType::PUBLIC;
	if (w == "private")
		return TokenType::PRIVATE;
	if (w == "protected")
		return TokenType::PROTECTED;
	return TokenType::IDENTIFIER;
}
inline vector<Token> tokenize(const string &code) {
	vector<Token> tokens;
	size_t i = 0;
	int line = 1;
	int col = 1;
	auto pushToken = [&](TokenType t, string v) {
		tokens.push_back({t, v, line, col - (int)v.length()});
	};
	while (i < code.size()) {
		char c = code[i];
		if (c == '\n') {
			line++;
			col = 1;
			i++;
			continue;
		}
		if (c == '~') {
			while (i < code.size() && code[i] != '\n')
				i++;
			continue;
		}
		if (c == '`') {
			i++;
			col++;
			while (i < code.size() && code[i] != '`') {
				if (code[i] == '\n') {
					line++;
					col = 1;
				} else {
					col++;
				}
				i++;
			}
			if (i < code.size()) {
				i++;
				col++;
			}
			continue;
		}
		if (isspace(c)) {
			i++;
			col++;
			continue;
		}
		if ((c == 'f' || c == 'F') && i + 1 < code.size() &&
			 (code[i + 1] == '"' || code[i + 1] == '\'')) {
			char quote = code[i + 1];
			int startCol = col;
			i += 2;
			col += 2;
			string str;
			while (i < code.size() && code[i] != quote) {
				if (code[i] == '\n') {
					line++;
					col = 1;
				} else if (code[i] == '\\' && i + 1 < code.size()) {
					char next = code[i + 1];
					switch (next) {
					case 'n':
						str += '\n';
						break;
					case 't':
						str += '\t';
						break;
					case '\\':
						str += '\\';
						break;
					case '"':
						str += '"';
						break;
					case '\'':
						str += '\'';
						break;
					default:
						str += next;
						break;
					}
					i += 2;
					col += 2;
					continue;
				} else
					col++;
				str += code[i++];
			}
			if (i < code.size()) {
				i++;
				col++;
			}
			tokens.push_back({TokenType::FSTRING, str, line, startCol});
			continue;
		}
		if (isalpha(c) || c == '_') {
			string word;
			int startCol = col;
			while (i < code.size() && (isalnum(code[i]) || code[i] == '_')) {
				word += code[i++];
				col++;
			}
			tokens.push_back({keywordType(word), word, line, startCol});
			continue;
		}
		if (isdigit(c)) {
			string num;
			int startCol = col;
			bool isFloat = false;
			if (c == '0' && i + 1 < code.size() && (code[i + 1] == 'x' || code[i + 1] == 'X')) {
				i += 2;
				col += 2;
				while (i < code.size() && isxdigit(code[i])) {
					num += code[i++];
					col++;
				}
				if (num.empty())
					num = "0";
				unsigned long long val = std::stoull(num, nullptr, 16);
				tokens.push_back({TokenType::NUMBER, std::to_string(static_cast<long long>(val)), line, startCol});
				continue;
			}
			if (c == '0' && i + 1 < code.size() && (code[i + 1] == 'b' || code[i + 1] == 'B')) {
				i += 2;
				col += 2;
				while (i < code.size() && (code[i] == '0' || code[i] == '1')) {
					num += code[i++];
					col++;
				}
				if (num.empty())
					num = "0";
				unsigned long long val = std::stoull(num, nullptr, 2);
				tokens.push_back({TokenType::NUMBER, std::to_string(static_cast<long long>(val)), line, startCol});
				continue;
			}
			while (i < code.size()) {
				if (isdigit(code[i])) {
					num += code[i++];
					col++;
				} else if (code[i] == '.') {
					if (i + 1 < code.size() && code[i + 1] == '.')
						break;
					if (isFloat)
						break;
					isFloat = true;
					num += code[i++];
					col++;
				} else
					break;
			}
			if (i < code.size() && (code[i] == 'f' || code[i] == 'F')) {
				isFloat = true;
				i++;
				col++;
				if (num.find('.') == string::npos) {
					num += ".0";
				}
			}

			tokens.push_back({TokenType::NUMBER, num, line, startCol});
			continue;
		}
		if (c == '"' || c == '\'') {
			char quote = c;
			int startCol = col;
			i++;
			col++;
			string str;
			while (i < code.size() && code[i] != quote) {
				if (code[i] == '\n') {
					line++;
					col = 1;
				} else if (code[i] == '\\' && i + 1 < code.size()) {
					char next = code[i + 1];
					switch (next) {
					case 'n':
						str += '\n';
						break;
					case 't':
						str += '\t';
						break;
					case '\\':
						str += '\\';
						break;
					case '"':
						str += '"';
						break;
					case '\'':
						str += '\'';
						break;
					default:
						str += next;
						break;
					}
					i += 2;
					col += 2;
					continue;
				} else
					col++;
				str += code[i++];
			}
			i++;
			col++;
			tokens.push_back({TokenType::STRING, str, line, startCol});
			continue;
		}
#define CHECK_OP(str, typeEnum)                       \
	if (code.substr(i, string(str).length()) == str) { \
		pushToken(typeEnum, str);                       \
		i += string(str).length();                      \
		col += string(str).length();                    \
		continue;                                       \
	}
		CHECK_OP("...", TokenType::DOT_DOT_DOT);
		CHECK_OP("===", TokenType::STRICT_EQ);
		CHECK_OP("!==", TokenType::STRICT_NEQ);
		CHECK_OP("//=", TokenType::FLOOR_DIV_EQ);
		CHECK_OP("**=", TokenType::POW_EQ);
		CHECK_OP("..", TokenType::DOT_DOT);
		CHECK_OP("//", TokenType::FLOOR_DIV);
		CHECK_OP("**", TokenType::POW);
		CHECK_OP("%=", TokenType::MOD_EQ);
		CHECK_OP("++", TokenType::INCREMENT);
		CHECK_OP("--", TokenType::DECREMENT);
		CHECK_OP("+=", TokenType::PLUS_EQ);
		CHECK_OP("-=", TokenType::MINUS_EQ);
		CHECK_OP("->", TokenType::ARROW);
		CHECK_OP("*=", TokenType::STAR_EQ);
		CHECK_OP("!=", TokenType::NEQ);
		CHECK_OP("<=", TokenType::LTE);
		CHECK_OP(">=", TokenType::GTE);
		CHECK_OP("/=", TokenType::DIV_EQ);
		CHECK_OP("&=", TokenType::AND_EQ);
		CHECK_OP("|=", TokenType::OR_EQ);
		CHECK_OP("^=", TokenType::XOR_EQ);
		CHECK_OP("==", TokenType::EQ);
		switch (c) {
		case '=':
			pushToken(TokenType::ASSIGN, "=");
			break;
		case '|':
			pushToken(TokenType::OR, "|");
			break;
		case '&':
			pushToken(TokenType::AND, "&");
			break;
		case '>':
			pushToken(TokenType::GT, ">");
			break;
		case '^':
			pushToken(TokenType::XOR, "^");
			break;
		case '<':
			pushToken(TokenType::LT, "<");
			break;
		case '(':
			pushToken(TokenType::LPAREN, "(");
			break;
		case ')':
			pushToken(TokenType::RPAREN, ")");
			break;
		case '[':
			pushToken(TokenType::LBRACKET, "[");
			break;
		case ']':
			pushToken(TokenType::RBRACKET, "]");
			break;
		case '{':
			pushToken(TokenType::LBRACE, "{");
			break;
		case '}':
			pushToken(TokenType::RBRACE, "}");
			break;
		case ',':
			pushToken(TokenType::COMMA, ",");
			break;
		case '+':
			pushToken(TokenType::PLUS, "+");
			break;
		case '$':
			pushToken(TokenType::DOLLAR, "$");
			break;
		case '@':
			pushToken(TokenType::AT, "@");
			break;
		case '-':
			pushToken(TokenType::MINUS, "-");
			break;
		case '*':
			pushToken(TokenType::STAR, "*");
			break;
		case '/':
			pushToken(TokenType::SLASH, "/");
			break;
		case '.':
			pushToken(TokenType::DOT, ".");
			break;
		case '%':
			pushToken(TokenType::MOD, "%");
			break;
		case '#':
			pushToken(TokenType::HASHTAG, "#");
			break;
		case ':':
			if (i + 1 < code.size() && code[i + 1] == '=') {
				pushToken(TokenType::COLON_EQ, ":=");
				i++;
				col++;
			} else {
				pushToken(TokenType::COLON, ":");
			}
			break;
		default:
			std::cerr << "Unknown char: " << c << " at line " << line << "\n";
			break;
		}
		i++;
		col++;
	}
	int endLine = (col == 1 && line > 1) ? line - 1 : line;
	tokens.push_back({TokenType::END, "", line, col});
	return tokens;
}
// -------------------- ERROR SYSTEM --------------------
struct LangError : public std::exception {
	string type;
	string message;
	long long code;
	int line;
	int col;
	string fullMessage;
	LangError(string t, string m, long long c, int l, int col_)
		 : type(t), message(m), code(c), line(l), col(col_) {
		fullMessage = type + " [Line " + std::to_string(line) + ":" +
						  std::to_string(col) + "]: " + message;
	}
	const char *what() const noexcept override { return fullMessage.c_str(); }
};
struct InternalError : LangError {
	InternalError(string m, int l, int c) : LangError("InternalError", m, -1100000, l, c) {}
};
struct ControlFlowError : LangError {
	ControlFlowError(string m, int l, int c)
		 : LangError("ControlFlowError", m, -1200000, l, c) {}
};
struct ParseError : LangError {
	ParseError(string m, int l, int c)
		 : LangError("ParseError", m, -2000000, l, c) {}
};
struct RuntimeError : LangError {
	RuntimeError(string m, int l, int c)
		 : LangError("RuntimeError", m, -3000000, l, c) {}
};
struct Warning : LangError {
	Warning(string m, int l, int c) : LangError("Warning", m, -7000000, l, c) {}
};
// --- ControlFlowError Subtree ---
struct ReturnSignal : ControlFlowError {
	ReturnSignal(string m, int l, int c) : ControlFlowError(m, l, c) {
		type = "ReturnSignal";
		code = -1200100;
	}
};
struct BreakSignal : ControlFlowError {
	BreakSignal(string m, int l, int c) : ControlFlowError(m, l, c) {
		type = "BreakSignal";
		code = -1200200;
	}
};
struct ContinueSignal : ControlFlowError {
	ContinueSignal(string m, int l, int c) : ControlFlowError(m, l, c) {
		type = "ContinueSignal";
		code = -1200300;
	}
};
// --- ParseError Subtree ---
struct SyntaxError : ParseError {
	SyntaxError(string m, int l, int c) : ParseError(m, l, c) {
		type = "SyntaxError";
		code = -2000100;
	}
};
struct IndentationError : ParseError {
	IndentationError(string m, int l, int c) : ParseError(m, l, c) {
		type = "IndentationError";
		code = -2000200;
	}
};
struct UnexpectedTokenError : ParseError {
	UnexpectedTokenError(string m, int l, int c) : ParseError(m, l, c) {
		type = "UnexpectedTokenError";
		code = -2000300;
	}
};
struct UnterminatedLiteralError : ParseError {
	UnterminatedLiteralError(string m, int l, int c) : ParseError(m, l, c) {
		type = "UnterminatedLiteralError";
		code = -2000400;
	}
};
// --- RuntimeError Subtree (Direct Children) ---
struct NameError : RuntimeError {
	NameError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "NameError";
		code = -3010000;
	}
};
struct AttributeError : RuntimeError {
	AttributeError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "AttributeError";
		code = -3020000;
	}
};
struct TypeError : RuntimeError {
	TypeError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "TypeError";
		code = -3030000;
	}
};
struct ArgumentError : RuntimeError {
	ArgumentError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "ArgumentError";
		code = -3040000;
	}
};
struct ValueError : RuntimeError {
	ValueError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "ValueError";
		code = -3050000;
	}
};
struct ConstError : RuntimeError {
	ConstError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "ConstError";
		code = -3060000;
	}
};
struct OwnershipError : RuntimeError {
	OwnershipError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "OwnershipError";
		code = -3070000;
	}
};
struct IndexError : RuntimeError {
	IndexError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "IndexError";
		code = -3080000;
	}
};
struct KeyError : RuntimeError {
	KeyError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "KeyError";
		code = -3090000;
	}
};
struct RangeError : RuntimeError {
	RangeError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "RangeError";
		code = -3100000;
	}
};
struct AssertionError : RuntimeError {
	AssertionError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "AssertionError";
		code = -3110000;
	}
};
struct RecursionError : RuntimeError {
	RecursionError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "RecursionError";
		code = -3120000;
	}
};
struct ImportError : RuntimeError {
	ImportError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "ImportError";
		code = -3200000;
	}
};
struct IOError : RuntimeError {
	IOError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "IOError";
		code = -3300000;
	}
};
struct MathError : RuntimeError {
	MathError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "MathError";
		code = -3400000;
	}
};
struct CastError : RuntimeError {
	CastError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "CastError";
		code = -3500000;
	}
};
struct IteratorError : RuntimeError {
	IteratorError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "IteratorError";
		code = -3510000;
	}
};
struct TimeoutError : RuntimeError {
	TimeoutError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "TimeoutError";
		code = -3520000;
	}
};
struct MemoryError : RuntimeError {
	MemoryError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "MemoryError";
		code = -3600000;
	}
};
struct SystemError : RuntimeError {
	SystemError(string m, int l, int c) : RuntimeError(m, l, c) {
		type = "SystemError";
		code = -3700000;
	}
};
// --- ValueError Children ---
struct EmptyContainerError : ValueError {
	EmptyContainerError(string m, int l, int c) : ValueError(m, l, c) {
		type = "EmptyContainerError";
		code = -3050100;
	}
};
// --- ConstError Children ---
struct MutationError : ConstError {
	MutationError(string m, int l, int c) : ConstError(m, l, c) {
		type = "MutationError";
		code = -3060100;
	}
};
// --- ImportError Children ---
struct ModuleNotFoundError : ImportError {
	ModuleNotFoundError(string m, int l, int c) : ImportError(m, l, c) {
		type = "ModuleNotFoundError";
		code = -3200100;
	}
};
struct CircularImportError : ImportError {
	CircularImportError(string m, int l, int c) : ImportError(m, l, c) {
		type = "CircularImportError";
		code = -3200200;
	}
};
struct InvalidImportError : ImportError {
	InvalidImportError(string m, int l, int c) : ImportError(m, l, c) {
		type = "InvalidImportError";
		code = -3200300;
	}
};
// --- IOError Children ---
struct FileNotFoundError : IOError {
	FileNotFoundError(string m, int l, int c) : IOError(m, l, c) {
		type = "FileNotFoundError";
		code = -3300100;
	}
};
struct PermissionError : IOError {
	PermissionError(string m, int l, int c) : IOError(m, l, c) {
		type = "PermissionError";
		code = -3300200;
	}
};
struct EOFError : IOError {
	EOFError(string m, int l, int c) : IOError(m, l, c) {
		type = "EOFError";
		code = -3300300;
	}
};
struct FileClosedError : IOError {
	FileClosedError(string m, int l, int c) : IOError(m, l, c) {
		type = "FileClosedError";
		code = -3300400;
	}
};
// --- MathError Children ---
struct DivisionByZeroError : MathError {
	DivisionByZeroError(string m, int l, int c) : MathError(m, l, c) {
		type = "DivisionByZeroError";
		code = -3400100;
	}
};
struct OverflowError : MathError {
	OverflowError(string m, int l, int c) : MathError(m, l, c) {
		type = "OverflowError";
		code = -3400200;
	}
};
struct UnderflowError : MathError {
	UnderflowError(string m, int l, int c) : MathError(m, l, c) {
		type = "UnderflowError";
		code = -3400300;
	}
};
struct DomainError : MathError {
	DomainError(string m, int l, int c) : MathError(m, l, c) {
		type = "DomainError";
		code = -3400400;
	}
};
// --- SystemError Children ---
struct OSError : SystemError {
	OSError(string m, int l, int c) : SystemError(m, l, c) {
		type = "OSError";
		code = -3700100;
	}
};
struct EnvironmentError : SystemError {
	EnvironmentError(string m, int l, int c) : SystemError(m, l, c) {
		type = "EnvironmentError";
		code = -3700200;
	}
};
struct SignalError : SystemError {
	SignalError(string m, int l, int c) : SystemError(m, l, c) {
		type = "SignalError";
		code = -3700300;
	}
};
// --- Warning Children ---
struct DeprecationWarning : Warning {
	DeprecationWarning(string m, int l, int c) : Warning(m, l, c) {
		type = "DeprecationWarning";
		code = -7010000;
	}
};
struct RuntimeWarning : Warning {
	RuntimeWarning(string m, int l, int c) : Warning(m, l, c) {
		type = "RuntimeWarning";
		code = -7020000;
	}
};
struct ImportWarning : Warning {
	ImportWarning(string m, int l, int c) : Warning(m, l, c) {
		type = "ImportWarning";
		code = -7030000;
	}
};
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
	GET
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
struct Value;
struct Stmt;
struct Expr {
	ExprType type;
	int line = 0;
	int col = 0;
	Expr(ExprType t) : type(t) {}
	virtual ~Expr() = default;
};
struct NumberExpr : Expr {
	double val;
	bool isFloat;
	NumberExpr(double v, bool f) : Expr(ExprType::NUMBER), val(v), isFloat(f) {}
};
struct StringExpr : Expr {
	string val;
	StringExpr(const string &v) : Expr(ExprType::STRING), val(v) {}
};
struct VarExpr : Expr {
	string name;
	Value *cachedGlobal = nullptr;
	VarExpr(const string &n) : Expr(ExprType::VAR), name(n) {}
};
struct BinExpr : Expr {
	Expr *left, *right;
	TokenType op;
	BinExpr(Expr *l, Expr *r, TokenType o)
		 : Expr(ExprType::BINARY), left(l), right(r), op(o) {}
};
struct CallExpr : Expr {
	string name;
	vector<Expr *> args;
	vector<CopyMode> modes;
	Value *cachedFunc = nullptr;
	CallExpr(const string &n, const vector<Expr *> &a, vector<CopyMode> &m)
		 : Expr(ExprType::CALL), name(n), args(a), modes(m) {}
};
struct BoolExpr : Expr {
	bool value;
	BoolExpr(bool v) : Expr(ExprType::BOOL), value(v) {}
};
struct OwnershipExpr : Expr {
	CopyMode mode;
	Expr *expr;
	OwnershipExpr(CopyMode m, Expr *e)
		 : Expr(ExprType::OWNERSHIP), mode(m), expr(e) {}
};
struct TernaryExpr : Expr {
	Expr *condition;
	Expr *trueBranch;
	Expr *falseBranch;
	TernaryExpr(Expr *condition, Expr *trueBranch, Expr *falseBranch)
		 : Expr(ExprType::TERNARY),
			condition(condition),
			trueBranch(trueBranch),
			falseBranch(falseBranch) {}
};
struct FStringExpr : Expr {
	vector<Expr *> parts;
	FStringExpr(vector<Expr *> p) : Expr(ExprType::FSTRING), parts(p) {}
};
struct CompExpr : Expr {
	Expr *expression;
	Expr *valueExpr;
	string varName;
	Expr *iterable;
	Expr *filter;
	TokenType typeToken;
	CompExpr(Expr *e, Expr *v, string n, Expr *i, Expr *f, TokenType t)
		 : Expr(ExprType::COMPREHENSION),
			expression(e),
			valueExpr(v),
			varName(n),
			iterable(i),
			filter(f),
			typeToken(t) {}
};
struct ListExpr : Expr {
	vector<Expr *> elements;
	ListExpr(vector<Expr *> e) : Expr(ExprType::LIST), elements(e) {};
};
struct IndexExpr : Expr {
	Expr *base;
	Expr *index;
	IndexExpr(Expr *b, Expr *i) : Expr(ExprType::INDEX), base(b), index(i) {}
};
struct MethodCallExpr : Expr {
	Expr *object;
	string method;
	vector<Expr *> args;
	MethodCallExpr(Expr *o, const string &m, const vector<Expr *> &a)
		 : Expr(ExprType::METHOD_CALL), object(o), method(m), args(a) {};
};
struct VectorExpr : Expr {
	vector<Expr *> elements;
	VectorExpr(const vector<Expr *> &e) : Expr(ExprType::VECTOR), elements(e) {}
};
struct ParamSpec {
	string name;
	CopyMode mode = CopyMode::SHALLOW;
	ValueType type;
	Expr *defaultValue = nullptr;
	bool isConst;
	bool isVariadic = false;
	bool isKwargs = false;
};
struct LambdaExpr : Expr {
	vector<ParamSpec> params;
	ValueType returnType;
	vector<Expr *> defaultRetArgs;
	bool returnsConst;
	vector<Stmt *> body;
	bool isCached;
	LambdaExpr(const vector<ParamSpec> &p, ValueType rt, vector<Expr *> dra,
		bool rc, const vector<Stmt *> &b, bool cached)
		 : Expr(ExprType::LAMBDA),
			params(p),
			returnType(rt),
			defaultRetArgs(dra),
			returnsConst(rc),
			body(b),
			isCached(cached) {}
};
struct RangeExpr : Expr {
	Expr *start;
	Expr *end;
	Expr *step;
	bool startInclusive;
	bool endInclusive;
	RangeExpr(Expr *s, Expr *e, Expr *st, bool si, bool ei)
		 : Expr(ExprType::RANGE),
			start(s),
			end(e),
			step(st),
			startInclusive(si),
			endInclusive(ei) {}
};
struct SetExpr : Expr {
	vector<Expr *> elements;
	SetExpr(const vector<Expr *> &e) : Expr(ExprType::SET), elements(e) {}
};
struct TupleExpr : Expr {
	vector<Expr *> elements;
	TupleExpr(vector<Expr *> e) : Expr(ExprType::TUPLE), elements(e) {};
};
struct DictExpr : Expr {
	vector<std::pair<Expr *, Expr *>> items;
	DictExpr(const vector<std::pair<Expr *, Expr *>> &i)
		 : Expr(ExprType::DICT), items(i) {}
};
struct SliceExpr : Expr {
	Expr *start = nullptr;
	Expr *end = nullptr;
	Expr *step = nullptr;
	SliceExpr(Expr *s, Expr *e, Expr *p)
		 : Expr(ExprType::SLICE), start(s), end(e), step(p) {}
};
struct OmitExpr : Expr {
	OmitExpr(int l, int c) : Expr(ExprType::OMIT_MARKER_EXPR) {}
};
struct GetExpr : Expr {
	Expr *object;
	string name;
	GetExpr(Expr *object, string name)
		 : Expr(ExprType::GET), object(object), name(name) {}
};
struct Stmt {
	StmtType type;
	int line = 0;
	int col = 0;
	Stmt(StmtType t) : type(t) {}
	virtual ~Stmt() = default;
};
struct BreakStmt : Stmt {
	BreakStmt() : Stmt(StmtType::BREAK) {}
};
struct ContinueStmt : Stmt {
	ContinueStmt() : Stmt(StmtType::CONTINUE) {}
};
struct SkipStmt : Stmt {
	Expr *count;
	SkipStmt(Expr *c) : Stmt(StmtType::SKIP), count(c) {}
};
struct WhileStmt : Stmt {
	Expr *condition;
	vector<Stmt *> body;
	WhileStmt(Expr *c, vector<Stmt *> b)
		 : Stmt(StmtType::WHILE), condition(c), body(b) {}
};
struct DoWhileStmt : Stmt {
	vector<Stmt *> body;
	Expr *condition;
	DoWhileStmt(vector<Stmt *> b, Expr *c)
		 : Stmt(StmtType::DO_WHILE), body(b), condition(c) {}
};
struct ForStmt : Stmt {
	vector<Stmt *> inits;
	Expr *condition;
	vector<Stmt *> steps;
	vector<Stmt *> body;
	ForStmt(vector<Stmt *> i, Expr *c, vector<Stmt *> s, vector<Stmt *> b)
		 : Stmt(StmtType::FOR), inits(i), condition(c), steps(s), body(b) {}
};
struct ForEachStmt : Stmt {
	vector<string> loopVars;
	vector<Expr *> collections;
	vector<Stmt *> body;
	ForEachStmt(vector<string> vars, vector<Expr *> cols, vector<Stmt *> b)
		 : Stmt(StmtType::FOR_EACH), loopVars(vars), collections(cols), body(b) {}
};
struct LetStmt : Stmt {
	string name;
	Expr *value;
	bool isConst;
	bool isLocked;
	AccessLevel access;
	LetStmt(const string &n, Expr *v, bool c, bool l, AccessLevel a)
		 : Stmt(StmtType::LET), name(n), value(v), isConst(c), isLocked(l), access(a) {}
};
struct AssignStmt : Stmt {
	Expr *target;
	TokenType op;
	Expr *value;
	AssignStmt(Expr *n, TokenType o, Expr *v)
		 : Stmt(StmtType::ASSIGN), target(n), op(o), value(v) {}
};
struct ReturnStmt : Stmt {
	Expr *value;
	ReturnStmt(Expr *v) : Stmt(StmtType::RETURN), value(v) {}
};
struct FuncStmt : Stmt {
	string name;
	vector<ParamSpec> params;
	vector<Expr *> defaultRetArgs;
	vector<Stmt *> body;
	ValueType returnType = ValueType::NOTYPE;
	bool returnsConst;
	bool isCached;
	FuncStmt(const string &n, const vector<ParamSpec> &p, vector<Expr *> dra,
		const vector<Stmt *> &b, ValueType rt, bool rc, bool cached)
		 : Stmt(StmtType::FUNC),
			name(n),
			params(p),
			defaultRetArgs(dra),
			body(b),
			returnType(rt),
			returnsConst(rc),
			isCached(cached) {};
};
struct IfStmt : Stmt {
	Expr *condition;
	vector<Stmt *> body;
	vector<std::pair<Expr *, vector<Stmt *>>> elifs;
	vector<Stmt *> elseBody;
	IfStmt(Expr *cond, vector<Stmt *> b,
		vector<std::pair<Expr *, vector<Stmt *>>> eib, vector<Stmt *> eb = {})
		 : Stmt(StmtType::IF),
			condition(cond),
			body(b),
			elifs(eib),
			elseBody(eb) {}
};
struct ElifStmt : IfStmt {};
struct Case {
	Expr *value;
	vector<Stmt *> body;
};
struct SwitchStmt : Stmt {
	Expr *target;
	vector<Case> cases;
	vector<Stmt *> defaultBody;
	SwitchStmt(Expr *t, const vector<Case> &c, const vector<Stmt *> &d)
		 : Stmt(StmtType::SWITCH), target(t), cases(c), defaultBody(d) {}
};
struct ExprStmt : Stmt {
	Expr *expr;
	ExprStmt(Expr *e) : Stmt(StmtType::EXPR), expr(e) {}
};
struct AssertStmt : Stmt {
	Expr *condition;
	Expr *message;
	AssertStmt(Expr *condition, Expr *message = nullptr)
		 : Stmt(StmtType::ASSERT), condition(condition), message(message) {}
};
struct CatchBlock {
	vector<string> types;
	vector<Stmt *> body;
};
struct TryStmt : Stmt {
	vector<Stmt *> tryBlock;
	vector<CatchBlock> catches;
	vector<Stmt *> elseBlock;
	vector<Stmt *> finallyBlock;
	TryStmt(const vector<Stmt *> &t, const vector<CatchBlock> &c,
		const vector<Stmt *> &e, const vector<Stmt *> &f)
		 : Stmt(StmtType::TRY),
			tryBlock(t),
			catches(c),
			elseBlock(e),
			finallyBlock(f) {}
};
struct ThrowStmt : Stmt {
	string errorType;
	Expr *message;
	ThrowStmt(string errorType, Expr *message)
		 : Stmt(StmtType::THROW), errorType(errorType), message(message) {}
};
struct ImportStmt : Stmt {
	string libName;
	vector<string> symbols;
	ImportStmt(string lib, vector<string> syms)
		 : Stmt(StmtType::IMPORT), libName(lib), symbols(syms) {}
};
struct MultiLetStmt : Stmt {
	vector<string> names;
	vector<Expr *> values;
	vector<bool> isConsts;
	bool isLocked;
	vector<AccessLevel> accesses;
	MultiLetStmt(const vector<string> &n, const vector<Expr *> &v,
		const vector<bool> &c, bool l, vector<AccessLevel> a)
		 : Stmt(StmtType::MULTI_LET),
			names(n),
			values(v),
			isConsts(c),
			isLocked(l),
			accesses(a) {}
};
struct MultiAssignStmt : Stmt {
	vector<Expr *> targets;
	vector<Expr *> values;
	MultiAssignStmt(const vector<Expr *> &t, const vector<Expr *> &v)
		 : Stmt(StmtType::MULTI_ASSIGN), targets(t), values(v) {}
};
struct ClassStmt : Stmt {
	string name;
	vector<string> parents;
	vector<Stmt *> publicBody;
	vector<Stmt *> privateBody;
	vector<Stmt *> protectedBody;
	ClassStmt(string n, const vector<string> &p, const vector<Stmt *> &pub,
		const vector<Stmt *> &priv, const vector<Stmt *> &prot)
		 : Stmt(StmtType::CLASS),
			name(n),
			parents(p),
			publicBody(pub),
			privateBody(priv),
			protectedBody(prot) {}
};
struct DeleteStmt : Stmt {
	Expr *target;
	DeleteStmt(Expr *t) : Stmt(StmtType::DELETE), target(t) {};
};
// -------------------- PARSER --------------------
class Parser {
	vector<Token> &tokens;
	size_t pos = 0;
	bool allowGT = true;
	int classDepth = 0;
	int scopeDepth = 0;

 public:
	void synchronize() {
		advance();
		while (!isAtEnd()) {
			switch (peek().type) {
			case TokenType::DEFINE:
			case TokenType::CLASS:
			case TokenType::LET:
			case TokenType::IF:
			case TokenType::WHILE:
			case TokenType::FOR:
			case TokenType::TRY:
			case TokenType::SWITCH:
			case TokenType::RETURN:
			case TokenType::IMPORT:
				return;
			default:
				break;
			}
			advance();
		}
	}

 private:
	[[noreturn]] void error(const string &message) {
		Token t = peek();
		throw SyntaxError(message, t.line, t.col);
	}
	Token consume(TokenType type, const string &message) {
		if (peek().type == type)
			return advance();
		error(message);
	}
	template <typename T>
	T *setPos(T *node, const Token &t) {
		node->line = t.line;
		node->col = t.col;
		return node;
	}

 public:
	Parser(vector<Token> &t) : tokens(t) {}
	bool isAtEnd() { return peek().type == TokenType::END; }
	Token &peek() { return tokens[pos]; }
	Token &advance() { return tokens[pos++]; }
	bool match(TokenType t) {
		if (peek().type == t) {
			advance();
			return true;
		}
		return false;
	}
	Expr *parseExpr() { return parseTernary(); }
	Expr *parseTernary() {
		Expr *expr = parseRange();
		if (peek().type == TokenType::IF) {
			size_t current = pos + 1;
			int openParens = 0;
			bool hasElse = false;
			while (current < tokens.size()) {
				TokenType t = tokens[current].type;
				if (t == TokenType::LPAREN || t == TokenType::LBRACE ||
					 t == TokenType::LBRACKET)
					openParens++;
				if (t == TokenType::RPAREN || t == TokenType::RBRACE ||
					 t == TokenType::RBRACKET)
					openParens--;
				if (openParens == 0 && t == TokenType::ELSE) {
					hasElse = true;
					break;
				}
				if (openParens == 0 &&
					 (t == TokenType::THEN || t == TokenType::DO ||
						 t == TokenType::COLON || t == TokenType::RBRACE ||
						 t == TokenType::END)) {
					break;
				}
				current++;
			}
			if (!hasElse)
				return expr;
			advance();
			Token opToken = tokens[pos - 1];
			Expr *condition = parseTernary();
			consume(TokenType::ELSE, "Expected 'else' in ternary operator");
			Expr *falseBranch = parseTernary();
			return setPos(new TernaryExpr(condition, expr, falseBranch), opToken);
		}
		return expr;
	}
	Expr *parseRange() {
		Expr *start = nullptr;
		bool startImplied = false;
		if (peek().type == TokenType::DOT_DOT ||
			 peek().type == TokenType::DOT_DOT_DOT) {
			start = new NumberExpr(0, false);
			startImplied = true;
		} else
			start = parseOr();
		if (peek().type == TokenType::DOT_DOT ||
			 peek().type == TokenType::DOT_DOT_DOT) {
			Token opToken = advance();
			bool inclusive = (opToken.type == TokenType::DOT_DOT_DOT);
			Expr *end = parseOr();
			Expr *step = nullptr;
			if (peek().type == TokenType::DOT_DOT ||
				 peek().type == TokenType::DOT_DOT_DOT) {
				advance();
				step = parseOr();
			}
			return new RangeExpr(start, end, step, true, inclusive);
		}
		if (startImplied)
			error("Unexpected range operator without end value");
		return start;
	}
	vector<Stmt *> parseBlock() {
		vector<Stmt *> body;
		scopeDepth++;
		if (match(TokenType::LBRACE)) {
			while (!isAtEnd() && peek().type != TokenType::RBRACE) {
				body.push_back(parseStmt());
			}
			consume(TokenType::RBRACE, "Expected '}' to close block");
			scopeDepth--;
			return body;
		}
		body.push_back(parseStmt());
		scopeDepth--;
		return body;
	}
	Expr *parseOr() {
		Expr *left = parseXor();
		while (peek().type == TokenType::OR || peek().type == TokenType::NOR) {
			Token opToken = advance();
			Expr *right = parseXor();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
		}
		return left;
	}
	Expr *parseXor() {
		Expr *left = parseAnd();
		while (peek().type == TokenType::XOR || peek().type == TokenType::NXOR) {
			Token opToken = advance();
			Expr *right = parseAnd();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
		}
		return left;
	}
	Expr *parseAnd() {
		Expr *left = parseCompare();
		while (peek().type == TokenType::AND || peek().type == TokenType::NAND) {
			Token opToken = advance();
			Expr *right = parseCompare();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
		}
		return left;
	}
	Expr *parseCompare() {
		Expr *left = parseAdd();
		while (peek().type == TokenType::GT || peek().type == TokenType::LT ||
				 peek().type == TokenType::GTE || peek().type == TokenType::LTE ||
				 peek().type == TokenType::EQ || peek().type == TokenType::NEQ ||
				 peek().type == TokenType::STRICT_EQ ||
				 peek().type == TokenType::STRICT_NEQ ||
				 peek().type == TokenType::IS) {
			if (!allowGT && peek().type == TokenType::GT)
				break;
			Token opToken = peek();
			TokenType op;
			if (match(TokenType::IS)) {
				opToken = tokens[pos - 1];
				if (match(TokenType::NOT)) {
					if (match(TokenType::IN))
						op = TokenType::IS_NOT_IN;
					else
						op = TokenType::IS_NOT;
				} else if (match(TokenType::IN))
					op = TokenType::IS_IN;
				else
					op = TokenType::IS;
			} else {
				opToken = advance();
				op = opToken.type;
			}
			Expr *right = parseAdd();
			left = setPos(new BinExpr(left, right, op), opToken);
		}
		return left;
	}
	Expr *parseAdd() {
		Expr *left = parseMul();
		while (peek().type == TokenType::PLUS ||
				 peek().type == TokenType::MINUS) {
			Token opToken = advance();
			Expr *right = parseMul();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
		}
		return left;
	}
	Expr *parseMul() {
		Expr *left = parsePower();
		while (peek().type == TokenType::STAR ||
				 peek().type == TokenType::SLASH ||
				 peek().type == TokenType::FLOOR_DIV ||
				 peek().type == TokenType::MOD) {
			Token opToken = advance();
			Expr *right = parseUnary();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
		}
		return left;
	}
	Expr *parsePower() {
		Expr *left = parseUnary();
		if (match(TokenType::POW)) {
			Token opToken = tokens[pos - 1];
			Expr *right = parsePower();
			return setPos(new BinExpr(left, right, TokenType::POW), opToken);
		}
		return left;
	}
	Expr *parseUnary() {
		if (match(TokenType::MINUS)) {
			Token opToken = tokens[pos - 1];
			Expr *right = parseUnary();
			return setPos(new BinExpr(setPos(new NumberExpr(0, false), opToken),
								  right, TokenType::MINUS),
				opToken);
		}
		if (match(TokenType::AT)) {
			Token opToken = tokens[pos - 1];
			Expr *target = parsePrimary();
			if (!dynamic_cast<VarExpr *>(target) &&
				 !dynamic_cast<IndexExpr *>(target) &&
				 !dynamic_cast<GetExpr *>(target)) {
				throw SyntaxError("Cannot take reference of non-lvalue",
					opToken.line, opToken.col);
			}
			return setPos(new OwnershipExpr(CopyMode::REF, target), opToken);
		}
		if (match(TokenType::DOLLAR)) {
			Token opToken = tokens[pos - 1];
			return setPos(new OwnershipExpr(CopyMode::DEEP, parseUnary()),
				opToken);
		}
		if (match(TokenType::NOT)) {
			Token opToken = tokens[pos - 1];
			Expr *right = parseUnary();
			return setPos(new BinExpr(nullptr, right, TokenType::NOT), opToken);
		}
		return parsePrimary();
	}
	Expr *parseFString() {
		Token t = tokens[pos - 1];
		string raw = t.value;
		vector<Expr *> parts;
		string buffer = "";
		for (size_t i = 0; i < raw.length(); i++) {
			if (raw[i] == '{') {
				if (!buffer.empty()) {
					parts.push_back(setPos(new StringExpr(buffer), t));
					buffer = "";
				}
				string code = "";
				int depth = 1;
				i++;
				while (i < raw.length() && depth > 0) {
					if (raw[i] == '{')
						depth++;
					if (raw[i] == '}')
						depth--;
					if (depth > 0)
						code += raw[i];
					i++;
				}
				i--;
				auto subTokens = tokenize(code);
				Parser subParser(subTokens);
				parts.push_back(subParser.parseExpr());
			} else
				buffer += raw[i];
		}
		if (!buffer.empty())
			parts.push_back(setPos(new StringExpr(buffer), t));
		return setPos(new FStringExpr(parts), t);
	}
	Expr *parsePrimary() {
		Expr *expr = nullptr;
		Token startTok = peek();
		bool isLambda = false;
		bool isCached = false;
		if (match(TokenType::DEFINE)) {
			if (match(TokenType::CACHED)) {
				isCached = true;
			}
			if (match(TokenType::LAMBDA)) {
				consume(TokenType::FUNCTION, "Expected 'function' after 'lambda'");
				isLambda = true;
			} else {
				throw SyntaxError("Expected 'lambda' after 'define' in expression",
					tokens[pos - 1].line, tokens[pos - 1].col);
			}
		}
		if (isLambda) {
			consume(TokenType::LPAREN, "Expected '(' after lambda signature");
			vector<ParamSpec> params;
			bool seenDefault = false;
			bool seenVariadic = false;
			bool seenKwargs = false;
			if (peek().type != TokenType::RPAREN) {
				do {
					CopyMode mode = CopyMode::SHALLOW;
					if (match(TokenType::AT))
						mode = CopyMode::REF;
					else if (match(TokenType::DOLLAR))
						mode = CopyMode::DEEP;
					bool isConst = false;
					if (match(TokenType::CONST))
						isConst = true;
					bool isVariadic = false;
					bool isKwargs = false;
					if (match(TokenType::POW)) {
						if (seenKwargs)
							error("Cannot have multiple **kwargs.");
						if (seenVariadic) { /* OK: **kwargs follows *args */
						}
						isKwargs = true;
						seenKwargs = true;
					}
					if (match(TokenType::STAR)) {
						if (seenVariadic)
							error("Cannot have multiple variadic (*args) parameters.");
						if (seenKwargs)
							error("*args cannot follow **kwargs.");
						isVariadic = true;
						seenVariadic = true;
					}
					if (!isVariadic && !isKwargs) {
						if (seenVariadic)
							error("Parameter cannot follow *args.");
						if (seenKwargs)
							error("Parameter cannot follow **kwargs.");
					}
					consume(TokenType::IDENTIFIER, "Expected parameter name");
					string pname = tokens[pos - 1].value;
					ValueType ptype = ValueType::NOTYPE;
					Expr *defaultExpr = nullptr;
					if (match(TokenType::COLON)) {
						string t;
						if (match(TokenType::FUNCTION))
							t = "function";
						else if (match(TokenType::LAMBDA))
							t = "lambda";
						else {
							consume(TokenType::IDENTIFIER, "Expected type name");
							t = tokens[pos - 1].value;
						}

						if (t == "int")
							ptype = ValueType::INT;
						else if (t == "float")
							ptype = ValueType::FLOAT;
						else if (t == "bool")
							ptype = ValueType::BOOL;
						else if (t == "string")
							ptype = ValueType::STRING;
						else if (t == "list")
							ptype = ValueType::LIST;
						else if (t == "dict" || t == "dictionary")
							ptype = ValueType::DICT;
						else if (t == "set")
							ptype = ValueType::SET;
						else if (t == "tuple")
							ptype = ValueType::TUPLE;
						else if (t == "range")
							ptype = ValueType::RANGE;
						else if (t == "vector")
							ptype = ValueType::VECTOR;
						else if (t == "function")
							ptype = ValueType::FUNCTION;
						else if (t == "lambda")
							ptype = ValueType::FUNCTION;
						else if (t == "object")
							ptype = ValueType::INSTANCE;
						else if (t == "None")
							ptype = ValueType::NONE;
						else if (t == "file")
							ptype = ValueType::FILE;
						else
							throw SyntaxError("Unknown type '" + t + "'",
								tokens[pos - 1].line,
								tokens[pos - 1].col);
						consume(TokenType::LPAREN,
							"Expected '(' after type definition");
						if (peek().type != TokenType::RPAREN)
							defaultExpr = parseExpr();
						consume(TokenType::RPAREN,
							"Expected ')' after type definition");
					}
					if (defaultExpr == nullptr && match(TokenType::ASSIGN)) {
						if (isVariadic || isKwargs)
							error("Variadic/Kwargs cannot have default values.");
						defaultExpr = parseExpr();
						seenDefault = true;
					}
					if (!isVariadic && !isKwargs && !defaultExpr && seenDefault) {
						error("Non-default parameter '" + pname +
								"' cannot follow default parameters.");
					}
					params.push_back({pname, mode, ptype, defaultExpr, isConst,
						isVariadic, isKwargs});
				} while (match(TokenType::COMMA));
			}
			consume(TokenType::RPAREN, "Expected ')' after parameters");
			ValueType retType = ValueType::NOTYPE;
			bool retConst = false;
			vector<Expr *> retArgs;
			if (match(TokenType::ARROW)) {
				if (match(TokenType::CONST))
					retConst = true;
				string t;
				if (match(TokenType::FUNCTION))
					t = "function";
				else if (match(TokenType::LAMBDA))
					t = "lambda";
				else {
					consume(TokenType::IDENTIFIER, "Expected return type");
					t = tokens[pos - 1].value;
				}
				if (t == "int")
					retType = ValueType::INT;
				else if (t == "float")
					retType = ValueType::FLOAT;
				else if (t == "string")
					retType = ValueType::STRING;
				else if (t == "bool")
					retType = ValueType::BOOL;
				else if (t == "list")
					retType = ValueType::LIST;
				else if (t == "dict" || t == "dictionary")
					retType = ValueType::DICT;
				else if (t == "set")
					retType = ValueType::SET;
				else if (t == "tuple")
					retType = ValueType::TUPLE;
				else if (t == "range")
					retType = ValueType::RANGE;
				else if (t == "vector")
					retType = ValueType::VECTOR;
				else if (t == "function")
					retType = ValueType::FUNCTION;
				else if (t == "lambda")
					retType = ValueType::FUNCTION;
				else if (t == "object")
					retType = ValueType::INSTANCE;
				else if (t == "None")
					retType = ValueType::NONE;
				else if (t == "file")
					retType = ValueType::FILE;
				else
					throw SyntaxError("Unknown return type '" + t + "'",
						tokens[pos - 1].line, tokens[pos - 1].col);
				if (match(TokenType::LPAREN)) {
					if (peek().type != TokenType::RPAREN) {
						do {
							Expr *arg = parseExpr();
							if (match(TokenType::COLON)) {
								Token op = tokens[pos - 1];
								Expr *val = parseExpr();
								arg =
									setPos(new BinExpr(arg, val, TokenType::COLON), op);
							}
							retArgs.push_back(arg);
						} while (match(TokenType::COMMA));
					}
					consume(TokenType::RPAREN, "Expected ')' after return type");
				}
			}
			consume(TokenType::COLON, "Expected ':' before lambda body");
			vector<Stmt *> body = parseBlock();
			return setPos(
				new LambdaExpr(params, retType, retArgs, retConst, body, isCached),
				startTok);
		}
		if (match(TokenType::OMIT)) {
			return new OmitExpr(tokens[pos - 1].line, tokens[pos - 1].col);
		}
		if (match(TokenType::FSTRING)) {
			return parseFString();
		}
		if (match(TokenType::NUMBER)) {
			string v = tokens[pos - 1].value;
			bool isFloat = v.find('.') != string::npos;
			expr = setPos(new NumberExpr(std::stod(v), isFloat), tokens[pos - 1]);
		} else if (match(TokenType::STRING)) {
			expr = setPos(new StringExpr(tokens[pos - 1].value), tokens[pos - 1]);
		} else if (match(TokenType::TRUE)) {
			expr = setPos(new BoolExpr(true), tokens[pos - 1]);
		} else if (match(TokenType::FALSE)) {
			expr = setPos(new BoolExpr(false), tokens[pos - 1]);
		} else if (match(TokenType::IDENTIFIER)) {
			expr = setPos(new VarExpr(tokens[pos - 1].value), tokens[pos - 1]);
		} else if (match(TokenType::LPAREN)) {
			Token paren = tokens[pos - 1];
			bool oldGT = allowGT;
			allowGT = true;
			try {
				if (match(TokenType::COMMA)) {
					consume(TokenType::RPAREN, "Expected ')' to close empty tuple");
					expr = setPos(new TupleExpr({}), paren);
				} else {
					Expr *e = parseExpr();
					if (match(TokenType::FOR)) {
						consume(TokenType::IDENTIFIER,
							"Expected variable name after 'for'");
						string varName = tokens[pos - 1].value;
						consume(TokenType::IN, "Expected 'in'");
						Expr *iterable = parseExpr();
						Expr *filter = nullptr;
						if (match(TokenType::IF))
							filter = parseExpr();
						consume(TokenType::RPAREN,
							"Expected ')' to close comprehension");
						expr = setPos(new CompExpr(e, nullptr, varName, iterable, filter,
											  TokenType::LPAREN),
							paren);
					} else if (match(TokenType::COMMA)) {
						vector<Expr *> tupleElems;
						tupleElems.push_back(e);
						if (peek().type != TokenType::RPAREN) {
							do {
								if (peek().type == TokenType::RPAREN)
									break;
								tupleElems.push_back(parseExpr());
							} while (match(TokenType::COMMA));
						}
						consume(TokenType::RPAREN, "Expected ')' to close tuple");
						expr = setPos(new TupleExpr(tupleElems), paren);
					} else {
						consume(TokenType::RPAREN, "Expected ')' to close grouping");
						expr = e;
					}
				}
			} catch (...) {
				allowGT = oldGT;
				throw;
			}
			allowGT = oldGT;
		} else if (match(TokenType::LBRACE)) {
			Token brace = tokens[pos - 1];
			if (match(TokenType::COMMA)) {
				consume(TokenType::RBRACE, "Expected '}' to close empty set");
				expr = setPos(new SetExpr({}), brace);
			} else if (peek().type == TokenType::RBRACE) {
				advance();
				expr = setPos(new DictExpr({}), brace);
			} else {
				Expr *first = parseExpr();
				if (match(TokenType::COLON)) {
					Expr *val = parseExpr();
					if (match(TokenType::FOR)) {
						consume(TokenType::IDENTIFIER,
							"Expected variable name after 'for'");
						string varName = tokens[pos - 1].value;
						consume(TokenType::IN, "Expected 'in'");
						Expr *iterable = parseExpr();
						Expr *filter = nullptr;
						if (match(TokenType::IF))
							filter = parseExpr();
						consume(TokenType::RBRACE,
							"Expected '}' to close dict comprehension");
						expr = setPos(new CompExpr(first, val, varName, iterable,
											  filter, TokenType::LBRACE),
							brace);
					} else {
						vector<std::pair<Expr *, Expr *>> items;
						items.push_back({first, val});
						while (match(TokenType::COMMA)) {
							Expr *k = parseExpr();
							consume(TokenType::COLON,
								"Expected ':' in dictionary entry");
							Expr *v = parseExpr();
							items.push_back({k, v});
						}
						consume(TokenType::RBRACE,
							"Expected '}' to close dictionary");
						expr = setPos(new DictExpr(items), brace);
					}
				} else if (match(TokenType::FOR)) {
					consume(TokenType::IDENTIFIER,
						"Expected variable name after 'for'");
					string varName = tokens[pos - 1].value;
					consume(TokenType::IN, "Expected 'in'");
					Expr *iterable = parseExpr();
					Expr *filter = nullptr;
					if (match(TokenType::IF))
						filter = parseExpr();
					consume(TokenType::RBRACE,
						"Expected '}' to close set comprehension");
					expr = setPos(new CompExpr(first, nullptr, varName, iterable,
										  filter, TokenType::LBRACE),
						brace);
				} else {
					vector<Expr *> elems;
					elems.push_back(first);
					while (match(TokenType::COMMA))
						elems.push_back(parseExpr());
					consume(TokenType::RBRACE, "Expected '}' to close set");
					expr = setPos(new SetExpr(elems), brace);
				}
			}
		} else if (match(TokenType::LBRACKET)) {
			Token bracket = tokens[pos - 1];
			if (peek().type == TokenType::RBRACKET) {
				advance();
				expr = setPos(new ListExpr({}), bracket);
			} else {
				Expr *first = parseExpr();
				if (match(TokenType::FOR)) {
					consume(TokenType::IDENTIFIER,
						"Expected variable name after 'for'");
					string varName = tokens[pos - 1].value;
					consume(TokenType::IN, "Expected 'in'");
					Expr *iterable = parseExpr();
					Expr *filter = nullptr;
					if (match(TokenType::IF))
						filter = parseExpr();
					consume(TokenType::RBRACKET,
						"Expected ']' to close list comprehension");
					expr = setPos(new CompExpr(first, nullptr, varName, iterable,
										  filter, TokenType::LBRACKET),
						bracket);
				} else {
					vector<Expr *> elems;
					elems.push_back(first);
					while (match(TokenType::COMMA))
						elems.push_back(parseExpr());
					consume(TokenType::RBRACKET, "Expected ']' to close list");
					expr = setPos(new ListExpr(elems), bracket);
				}
			}
		} else if (match(TokenType::LT)) {
			Token startTok = tokens[pos - 1];
			vector<Expr *> elements;
			if (peek().type != TokenType::GT) {
				bool oldGT = allowGT;
				allowGT = false;
				try {
					do {
						elements.push_back(parseExpr());
					} while (match(TokenType::COMMA));
				} catch (...) {
					allowGT = oldGT;
					throw;
				}
				allowGT = oldGT;
			}
			consume(TokenType::GT, "Expected '>' to close vector literal");
			expr = setPos(new VectorExpr(elements), startTok);
		} else {
			Token current = peek();
			Token prev = (pos > 0) ? tokens[pos - 1] : current;
			if (current.line > prev.line) {
				throw SyntaxError("Expected expression after '" + prev.value + "'",
					prev.line, prev.col + (int)prev.value.length());
			}
			if (!expr)
				error("Expected expression");
		}
		while (true) {
			if (match(TokenType::LPAREN)) {
				vector<Expr *> args;
				vector<CopyMode> modes;
				if (peek().type != TokenType::RPAREN) {
					do {
						CopyMode mode = CopyMode::SHALLOW;
						if (match(TokenType::AT))
							mode = CopyMode::REF;
						else if (match(TokenType::DOLLAR))
							mode = CopyMode::DEEP;
						Expr *arg = parseExpr();
						if (match(TokenType::COLON) || match(TokenType::ASSIGN)) {
							Expr *val = parseExpr();
							if (auto v = dynamic_cast<VarExpr *>(arg)) {
								arg = setPos(new StringExpr(v->name), tokens[pos - 2]);
							}
							arg = setPos(new BinExpr(arg, val, TokenType::COLON),
								tokens[pos - 2]);
						}
						args.push_back(arg);
						modes.push_back(mode);
					} while (match(TokenType::COMMA));
				}
				match(TokenType::RPAREN);
				if (auto v = dynamic_cast<VarExpr *>(expr))
					expr = setPos(new CallExpr(v->name, args, modes), startTok);
				else {
					Token t = tokens[pos - 1];
					throw TypeError(
						"Expression is not callable (must be a variable name)",
						t.line, t.col);
				}
			} else if (match(TokenType::LBRACKET)) {
				Token bracket = tokens[pos - 1];
				Expr *start = nullptr;
				Expr *end = nullptr;
				Expr *step = nullptr;
				bool isSlice = false;
				if (match(TokenType::COLON)) {
					isSlice = true;
					if (peek().type != TokenType::COLON &&
						 peek().type != TokenType::RBRACKET)
						end = parseExpr();
					if (match(TokenType::COLON))
						if (peek().type != TokenType::RBRACKET)
							step = parseExpr();
				} else {
					Expr *first = parseExpr();
					if (match(TokenType::COLON)) {
						isSlice = true;
						start = first;
						if (peek().type != TokenType::COLON &&
							 peek().type != TokenType::RBRACKET)
							end = parseExpr();
						if (match(TokenType::COLON))
							if (peek().type != TokenType::RBRACKET)
								step = parseExpr();
					} else
						start = first;
				}
				consume(TokenType::RBRACKET, "Expected ']' to close index");
				if (isSlice)
					expr =
						setPos(new IndexExpr(expr, new SliceExpr(start, end, step)),
							bracket);
				else
					expr = setPos(new IndexExpr(expr, start), bracket);
			} else if (match(TokenType::DOT)) {
				Token dot = tokens[pos - 1];
				consume(TokenType::IDENTIFIER, "Expected method name after '.'");
				string method = tokens[pos - 1].value;
				if (match(TokenType::LPAREN)) {
					vector<Expr *> args;
					if (peek().type != TokenType::RPAREN)
						do {
							Expr *arg = parseExpr();
							if (match(TokenType::COLON) || match(TokenType::ASSIGN)) {
								Token op = tokens[pos - 1];
								Expr *val = parseExpr();
								if (auto v = dynamic_cast<VarExpr *>(arg)) {
									arg = setPos(new StringExpr(v->name), op);
								}
								arg = setPos(new BinExpr(arg, val, TokenType::COLON), op);
							}
							args.push_back(arg);
						} while (match(TokenType::COMMA));
					match(TokenType::RPAREN);
					expr = setPos(new MethodCallExpr(expr, method, args), dot);
				} else
					expr = setPos(new GetExpr(expr, method), dot);
			} else
				break;
		}
		return expr;
	}
	Stmt *parseForStep() {
		Expr *e = parseExpr();
		if (match(TokenType::ASSIGN)) {
			Token op = tokens[pos - 1];
			Expr *val = parseExpr();
			return setPos(new AssignStmt(e, TokenType::ASSIGN, val), op);
		}
		TokenType type = peek().type;
		if (type == TokenType::PLUS_EQ || type == TokenType::MINUS_EQ ||
			 type == TokenType::STAR_EQ || type == TokenType::DIV_EQ ||
			 type == TokenType::MOD_EQ || type == TokenType::POW_EQ ||
			 type == TokenType::FLOOR_DIV_EQ || type == TokenType::AND_EQ ||
			 type == TokenType::OR_EQ || type == TokenType::XOR_EQ) {
			Token op = advance();
			Expr *val = parseExpr();
			return setPos(new AssignStmt(e, type, val), op);
		}
		if (match(TokenType::INCREMENT)) {
			Token op = tokens[pos - 1];
			return setPos(new AssignStmt(e, TokenType::PLUS_EQ,
								  setPos(new NumberExpr(1, false), op)),
				op);
		}
		if (match(TokenType::DECREMENT)) {
			Token op = tokens[pos - 1];
			return setPos(new AssignStmt(e, TokenType::MINUS_EQ,
								  setPos(new NumberExpr(1, false), op)),
				op);
		}
		if (auto as = dynamic_cast<AssignStmt *>(e))
			return as;
		Stmt *s = new ExprStmt(e);
		s->line = e->line;
		s->col = e->col;
		return s;
	}
	Stmt *parseStmt() {
		if (scopeDepth == 0 && classDepth == 0 && !isAtEnd()) {
			TokenType t = peek().type;
			if (t != TokenType::IMPORT && t != TokenType::LET &&
				 t != TokenType::DEFINE && t != TokenType::CLASS) {
				error("Executable statements (loops, functions, logic) are not allowed at the global scope.");
			}
		}
		if (match(TokenType::SWITCH)) {
			Token t = tokens[pos - 1];
			Expr *target = parseExpr();
			consume(TokenType::COLON, "Expected ':' after switch value");
			consume(TokenType::LBRACE, "Expected '{' to open switch block");
			vector<Case> cases;
			bool expectingMore = true;
			while (match(TokenType::CASE)) {
				if (!expectingMore) {
					Token errTok = tokens[pos - 1];
					throw SyntaxError(
						"Unreachable case. Previous case used 'do' (implying "
						"terminal), "
						"expected 'then do'.",
						errTok.line, errTok.col);
				}
				Expr *val = parseExpr();
				if (match(TokenType::THEN)) {
					consume(TokenType::DO, "Expected 'do' after 'then'");
					expectingMore = true;
				} else {
					consume(TokenType::DO,
						"Expected 'do' or 'then do' after case value");
					expectingMore = false;
				}
				consume(TokenType::COLON, "Expected ':' after case declaration");
				cases.push_back({val, parseBlock()});
			}
			consume(TokenType::DEFAULT,
				"Expected 'default' block. Default is required.");
			consume(TokenType::DO, "Expected 'do' after default");
			consume(TokenType::COLON, "Expected ':' after default");
			vector<Stmt *> defaultBody = parseBlock();
			consume(TokenType::RBRACE, "Expected '}' to close switch block");
			return setPos(new SwitchStmt(target, cases, defaultBody), t);
		}
		if (match(TokenType::DELETE)) {
			Token t = tokens[pos - 1];
			Expr *e = parseExpr();
			if (!dynamic_cast<VarExpr *>(e) && !dynamic_cast<IndexExpr *>(e) &&
				 !dynamic_cast<GetExpr *>(e) && !dynamic_cast<OwnershipExpr *>(e)) {
				throw SyntaxError(
					"Invalid delete target. Can only delete variables, indices, or "
					"properties.",
					t.line, t.col);
			}
			return setPos(new DeleteStmt(e), t);
		}
		if (match(TokenType::IMPORT)) {
			Token t = tokens[pos - 1];
			vector<string> symbols;
			if (peek().type == TokenType::IDENTIFIER || peek().type == TokenType::STAR) {
				string firstWord = advance().value;
				if (peek().type == TokenType::COMMA ||
					 peek().type == TokenType::FROM) {
					symbols.push_back(firstWord);
					while (match(TokenType::COMMA)) {
						consume(TokenType::IDENTIFIER, "Expected function name");
						symbols.push_back(tokens[pos - 1].value);
					}
					consume(TokenType::FROM, "Expected 'from' keyword");
					string libName;
					if (match(TokenType::STRING))
						libName = tokens[pos - 1].value;
					else {
						consume(TokenType::IDENTIFIER, "Expected library name");
						libName = tokens[pos - 1].value;
					}
					return setPos(new ImportStmt(libName, symbols), t);
				}
				return setPos(new ImportStmt(firstWord, {}), t);
			} else if (match(TokenType::STRING)) {
				return setPos(new ImportStmt(tokens[pos - 1].value, {}), t);
			} else
				throw SyntaxError("Expected library name or identifier after 'import'", t.line, t.col);
		}
		if (match(TokenType::LET)) {
			Token t = tokens[pos - 1];
			vector<string> names;
			vector<bool> consts;
			vector<AccessLevel> accesses;
			vector<Expr *> values;
			do {
				bool isConst = false;
				AccessLevel access = AccessLevel::PUBLIC;
				while (peek().type == TokenType::CONST || peek().type == TokenType::PUBLIC ||
						 peek().type == TokenType::PRIVATE || peek().type == TokenType::PROTECTED) {
					if (match(TokenType::CONST))
						isConst = true;
					else if (match(TokenType::PRIVATE))
						access = AccessLevel::PRIVATE;
					else if (match(TokenType::PROTECTED))
						access = AccessLevel::PROTECTED;
					else if (match(TokenType::PUBLIC))
						access = AccessLevel::PUBLIC;
				}

				consume(TokenType::IDENTIFIER, "Expected variable name");
				std::string name = tokens[pos - 1].value;
				if (match(TokenType::DOT)) {
					consume(TokenType::IDENTIFIER, "Expected property name after '.'");
					string prop = tokens[pos - 1].value;
					if (classDepth == 0) {
						if (name == "self" || name == "obj") {
							throw SyntaxError("Cannot use '" + name + "' outside of a class definition.", t.line, t.col);
						}
					}
					name += "." + prop;
				} else if (classDepth == 0 && (name == "self" || name == "obj")) {
					throw SyntaxError("'" + name + "' is a reserved keyword inside classes.", t.line, t.col);
				}
				names.push_back(name);
				consts.push_back(isConst);
				accesses.push_back(access);
			} while (match(TokenType::COMMA));
			bool isLocked = false;
			bool hasAssign = false;
			if (match(TokenType::COLON_EQ)) {
				isLocked = true;
				hasAssign = true;
			} else if (match(TokenType::ASSIGN)) {
				isLocked = false;
				hasAssign = true;
			}
			if (hasAssign) {
				do {
					values.push_back(parseExpr());
				} while (match(TokenType::COMMA));
			}
			if (values.empty())
				for (size_t i = 0; i < names.size(); i++)
					values.push_back(nullptr);
			else if (names.size() != values.size())
				throw SyntaxError("Mismatch in number of variables and values",
					t.line, t.col);
			if (names.size() == 1)
				return setPos(new LetStmt(names[0], values[0], consts[0], isLocked, accesses[0]),
					t);
			return setPos(new MultiLetStmt(names, values, consts, isLocked, accesses), t);
		}
		if (match(TokenType::RETURN)) {
			Token t = tokens[pos - 1];
			Expr *value = nullptr;
			TokenType next = peek().type;
			if (next != TokenType::RBRACE && next != TokenType::LET &&
				 next != TokenType::IF && next != TokenType::FOR &&
				 next != TokenType::WHILE && next != TokenType::DO &&
				 next != TokenType::BREAK && next != TokenType::CONTINUE &&
				 next != TokenType::RETURN && next != TokenType::END) {
				value = parseExpr();
			}
			return setPos(new ReturnStmt(value), t);
		}
		if (match(TokenType::BREAK))
			return setPos(new BreakStmt(), tokens[pos - 1]);
		if (match(TokenType::CONTINUE))
			return setPos(new ContinueStmt(), tokens[pos - 1]);
		if (match(TokenType::SKIP)) {
			Token t = tokens[pos - 1];
			Expr *count = parseExpr();
			return setPos(new SkipStmt(count), t);
		}
		bool isNamedFunction = false;
		bool isClass = false;
		if (peek().type == TokenType::DEFINE) {
			int offset = 1;
			if (tokens[pos + offset].type == TokenType::CLASS)
				isClass = true;
			else {
				if (tokens[pos + offset].type == TokenType::CACHED)
					offset++;
				if (tokens[pos + offset].type == TokenType::FUNCTION)
					isNamedFunction = true;
			}
		}
		if (isClass) {
			Token startTok = advance();
			consume(TokenType::CLASS, "Expected 'class' keyword");
			consume(TokenType::IDENTIFIER, "Expected class name");
			string className = tokens[pos - 1].value;
			consume(TokenType::COLON, "Expected ':' after class name");
			vector<string> parents;
			if (match(TokenType::INHERITS)) {
				do {
					consume(TokenType::IDENTIFIER, "Expected parent class name");
					parents.push_back(tokens[pos - 1].value);
				} while (match(TokenType::COMMA));
				consume(TokenType::COLON,
					"Expected ':' after inheritance declaration");
			}
			consume(TokenType::LBRACE, "Expected '{' to open class body");
			classDepth++;
			vector<Stmt *> publicBody;
			vector<Stmt *> privateBody;
			vector<Stmt *> protectedBody;
			bool seenPublic = false;
			bool seenPrivate = false;
			bool seenProtected = false;
			TokenType currentMode = TokenType::END;
			while (!isAtEnd() && peek().type != TokenType::RBRACE) {
				if (match(TokenType::HASHTAG)) {
					if (match(TokenType::PUBLIC)) {
						currentMode = TokenType::PUBLIC;
						seenPublic = true;
					} else if (match(TokenType::PRIVATE)) {
						currentMode = TokenType::PRIVATE;
						seenPrivate = true;
					} else if (match(TokenType::PROTECTED)) {
						currentMode = TokenType::PROTECTED;
						seenProtected = true;
					} else
						error(
							"Expected 'public', 'private', or 'protected' after '#'");
					consume(TokenType::COLON,
						"Expected ':' after access modifier tag");
					continue;
				}
				if (currentMode == TokenType::END) {
					error(
						"All class statements must be inside a #public, #private, "
						"or "
						"#protected block.");
				}
				Stmt *stmt = parseStmt();
				if (currentMode == TokenType::PUBLIC)
					publicBody.push_back(stmt);
				else if (currentMode == TokenType::PRIVATE)
					privateBody.push_back(stmt);
				else if (currentMode == TokenType::PROTECTED)
					protectedBody.push_back(stmt);
			}
			consume(TokenType::RBRACE, "Expected '}' to close class body");
			classDepth--;
			if (!seenPublic || !seenPrivate || !seenProtected) {
				throw SyntaxError(
					"Class definition must contain #public, #private, and "
					"#protected "
					"blocks.",
					startTok.line, startTok.col);
			}
			return setPos(new ClassStmt(className, parents, publicBody,
								  privateBody, protectedBody),
				startTok);
		}
		if (isNamedFunction) {
			Token defTok = advance();
			bool isCached = false;
			if (match(TokenType::CACHED))
				isCached = true;
			consume(TokenType::FUNCTION, "Expected 'function' keyword");
			consume(TokenType::IDENTIFIER, "Expected function name");
			string fname = tokens[pos - 1].value;
			ValueType retType = ValueType::NOTYPE;
			match(TokenType::LPAREN);
			vector<ParamSpec> params;
			bool seenDefault = false;
			bool seenVariadic = false;
			bool seenKwargs = false;
			if (peek().type != TokenType::RPAREN) {
				do {
					CopyMode mode = CopyMode::SHALLOW;
					if (match(TokenType::AT))
						mode = CopyMode::REF;
					else if (match(TokenType::DOLLAR))
						mode = CopyMode::DEEP;
					bool isConst = false;
					if (match(TokenType::CONST))
						isConst = true;
					bool isVariadic = false;
					bool isKwargs = false;
					if (match(TokenType::POW)) {
						if (seenKwargs)
							error("Cannot have multiple **kwargs.");
						if (seenVariadic) {
						}
						isKwargs = true;
						seenKwargs = true;
					} else if (match(TokenType::STAR)) {
						if (seenVariadic)
							error("Cannot have multiple *args.");
						if (seenKwargs)
							error("*args cannot follow **kwargs.");
						isVariadic = true;
						seenVariadic = true;
					}
					if (!isVariadic && !isKwargs) {
						if (seenVariadic)
							error("Parameter cannot follow *args.");
						if (seenKwargs)
							error("Parameter cannot follow **kwargs.");
					}
					consume(TokenType::IDENTIFIER, "Expected parameter name");
					string pname = tokens[pos - 1].value;
					ValueType ptype = ValueType::NOTYPE;
					Expr *defaultExpr = nullptr;
					if (match(TokenType::COLON)) {
						string t;
						if (match(TokenType::FUNCTION))
							t = "function";
						else if (match(TokenType::LAMBDA))
							t = "lambda";
						else {
							consume(TokenType::IDENTIFIER, "Expected type name");
							t = tokens[pos - 1].value;
						}
						if (t == "int")
							ptype = ValueType::INT;
						else if (t == "float")
							ptype = ValueType::FLOAT;
						else if (t == "bool")
							ptype = ValueType::BOOL;
						else if (t == "string")
							ptype = ValueType::STRING;
						else if (t == "list")
							ptype = ValueType::LIST;
						else if (t == "range")
							ptype = ValueType::RANGE;
						else if (t == "set")
							ptype = ValueType::SET;
						else if (t == "tuple")
							ptype = ValueType::TUPLE;
						else if (t == "dictionary" || t == "dict")
							ptype = ValueType::DICT;
						else if (t == "vector")
							ptype = ValueType::VECTOR;
						else if (t == "function" || t == "lambda")
							ptype = ValueType::FUNCTION;
						else if (t == "object")
							ptype = ValueType::INSTANCE;
						else if (t == "file")
							ptype = ValueType::FILE;
						else
							throw SyntaxError("Unknown type '" + t + "'",
								tokens[pos - 1].line,
								tokens[pos - 1].col);
						consume(TokenType::LPAREN,
							"Expected '(' after type definition");
						if (peek().type != TokenType::RPAREN)
							defaultExpr = parseExpr();
						consume(TokenType::RPAREN,
							"Expected ')' after type definition");
					}
					if (defaultExpr == nullptr && match(TokenType::ASSIGN)) {
						if (isVariadic || isKwargs)
							error("Variadic/Kwargs cannot have default values.");
						defaultExpr = parseExpr();
						seenDefault = true;
					}
					if (!isVariadic && !isKwargs && !defaultExpr && seenDefault) {
						error("Non-default parameter '" + pname +
								"' cannot follow default parameters.");
					}
					params.push_back({pname, mode, ptype, defaultExpr, isConst,
						isVariadic, isKwargs});
				} while (match(TokenType::COMMA));
			}
			consume(TokenType::RPAREN, "Expected ')' after function parameters");
			bool retConst = false;
			vector<Expr *> retArgs;
			if (match(TokenType::ARROW)) {
				if (match(TokenType::CONST))
					retConst = true;
				string t;
				if (match(TokenType::FUNCTION))
					t = "function";
				else if (match(TokenType::LAMBDA))
					t = "lambda";
				else {
					consume(TokenType::IDENTIFIER, "Expected return type");
					t = tokens[pos - 1].value;
				}
				if (t == "int")
					retType = ValueType::INT;
				else if (t == "float")
					retType = ValueType::FLOAT;
				else if (t == "bool")
					retType = ValueType::BOOL;
				else if (t == "string")
					retType = ValueType::STRING;
				else if (t == "list")
					retType = ValueType::LIST;
				else if (t == "None")
					retType = ValueType::NONE;
				else if (t == "NoType")
					retType = ValueType::NOTYPE;
				else if (t == "range")
					retType = ValueType::RANGE;
				else if (t == "set")
					retType = ValueType::SET;
				else if (t == "tuple")
					retType = ValueType::TUPLE;
				else if (t == "dictionary" || t == "dict")
					retType = ValueType::DICT;
				else if (t == "function" || t == "lambda")
					retType = ValueType::FUNCTION;
				else if (t == "vector")
					retType = ValueType::VECTOR;
				else if (t == "object")
					retType = ValueType::INSTANCE;
				else if (t == "file")
					retType = ValueType::FILE;
				else
					throw SyntaxError("Unknown return type '" + t + "'",
						tokens[pos - 1].line, tokens[pos - 1].col);
				consume(TokenType::LPAREN, "Expected '(' after return type");
				if (peek().type != TokenType::RPAREN) {
					do {
						Expr *arg = parseExpr();
						if (match(TokenType::COLON)) {
							Token op = tokens[pos - 1];
							Expr *val = parseExpr();
							arg = setPos(new BinExpr(arg, val, TokenType::COLON), op);
						}
						retArgs.push_back(arg);
					} while (match(TokenType::COMMA));
				}
				consume(TokenType::RPAREN, "Expected ')' after return type");
			}
			consume(TokenType::COLON, "Expected ':' after function signature");
			vector<Stmt *> body = parseBlock();
			return setPos(new FuncStmt(fname, params, retArgs, body, retType,
								  retConst, isCached),
				defTok);
		}
		if (match(TokenType::WHILE)) {
			Token t = tokens[pos - 1];
			Expr *cond = parseExpr();
			consume(TokenType::DO, "Expected 'do' after while condition");
			consume(TokenType::COLON, "Expected ':' after do");
			vector<Stmt *> body = parseBlock();
			return setPos(new WhileStmt(cond, body), t);
		}
		if (match(TokenType::DO)) {
			Token t = tokens[pos - 1];
			consume(TokenType::COLON, "Expected ':' after do");
			vector<Stmt *> body = parseBlock();
			consume(TokenType::WHILE, "Expected 'while' after do block");
			Expr *cond = parseExpr();
			return setPos(new DoWhileStmt(body, cond), t);
		}
		if (match(TokenType::FOR)) {
			Token t = tokens[pos - 1];
			bool isCStyle = false;
			if (peek().type == TokenType::LET || peek().type == TokenType::OMIT)
				isCStyle = true;
			else {
				int look = 0;
				while (tokens[pos + look].type == TokenType::IDENTIFIER ||
						 tokens[pos + look].type == TokenType::COMMA) {
					look++;
				}
				if (tokens[pos + look].type != TokenType::IN)
					isCStyle = true;
			}
			if (!isCStyle) {
				vector<string> loopVars;
				do {
					consume(TokenType::IDENTIFIER, "Expected loop variable");
					loopVars.push_back(tokens[pos - 1].value);
				} while (match(TokenType::COMMA));
				consume(TokenType::IN, "Expected 'in' after loop variables");
				vector<Expr *> collections;
				do {
					collections.push_back(parseExpr());
				} while (match(TokenType::COMMA));
				consume(TokenType::DO, "Expected 'do'");
				consume(TokenType::COLON, "Expected ':'");
				vector<Stmt *> body = parseBlock();
				return setPos(new ForEachStmt(loopVars, collections, body), t);
			}
			vector<Stmt *> inits;
			if (!match(TokenType::OMIT)) {
				if (match(TokenType::LET)) {
					bool isMultiMode = false;
					if (tokens[pos].type == TokenType::IDENTIFIER &&
						 tokens[pos + 1].type == TokenType::COMMA) {
						isMultiMode = true;
					}

					if (isMultiMode) {
						vector<string> names;
						do {
							consume(TokenType::IDENTIFIER, "Expected variable name");
							names.push_back(tokens[pos - 1].value);
						} while (match(TokenType::COMMA));
						consume(TokenType::ASSIGN,
							"Expected '=' after variable names");
						vector<Expr *> values;
						do {
							values.push_back(parseExpr());
						} while (match(TokenType::COMMA));
						if (names.size() != values.size()) {
							error("Mismatch: " + std::to_string(names.size()) +
									" variables but " + std::to_string(values.size()) +
									" values.");
						}
						for (size_t k = 0; k < names.size(); k++) {
							inits.push_back(
								new LetStmt(names[k], values[k], false, false, AccessLevel::PUBLIC));
						}
					} else {
						do {
							consume(TokenType::IDENTIFIER, "Expected variable name");
							string name = tokens[pos - 1].value;
							Expr *val = nullptr;
							if (match(TokenType::ASSIGN)) {
								val = parseExpr();
							} else {
								val = new NumberExpr(0, false);
							}
							inits.push_back(new LetStmt(name, val, false, false, AccessLevel::PUBLIC));
						} while (match(TokenType::COMMA));
					}
				} else {
					do {
						inits.push_back(parseForStep());
					} while (match(TokenType::COMMA));
				}
			}
			consume(TokenType::IF, "Expected 'if' in for loop");
			Expr *condition = nullptr;
			if (!match(TokenType::OMIT)) {
				condition = parseExpr();
			}
			vector<Stmt *> steps;
			if (match(TokenType::THEN)) {
				if (!match(TokenType::OMIT)) {
					do {
						steps.push_back(parseForStep());
					} while (match(TokenType::COMMA));
				}
				consume(TokenType::DO, "Expected 'do' after loop steps");
			} else {
				consume(TokenType::DO, "Expected 'do' after loop condition");
			}
			consume(TokenType::COLON, "Expected ':'");
			vector<Stmt *> body = parseBlock();

			return setPos(new ForStmt(inits, condition, steps, body), t);
		}
		if (match(TokenType::ASSERT)) {
			Token t = tokens[pos - 1];
			Expr *condition = parseExpr();
			Expr *message = nullptr;
			if (match(TokenType::COMMA))
				message = parseExpr();
			return setPos(new AssertStmt(condition, message), t);
		}
		if (match(TokenType::IF)) {
			Token t = tokens[pos - 1];
			Expr *cond = parseExpr();
			match(TokenType::COLON);
			vector<Stmt *> body = parseBlock();
			vector<std::pair<Expr *, vector<Stmt *>>> elifs;
			while (match(TokenType::ELSE_IF)) {
				Expr *ec = parseExpr();
				match(TokenType::COLON);
				elifs.push_back({ec, parseBlock()});
			}
			vector<Stmt *> elseBody;
			if (match(TokenType::ELSE)) {
				match(TokenType::COLON);
				elseBody = parseBlock();
			}
			return setPos(new IfStmt{cond, body, elifs, elseBody}, t);
		}
		if (match(TokenType::TRY)) {
			Token t = tokens[pos - 1];
			consume(TokenType::COLON, "Expected ':' after try");
			vector<Stmt *> tryBody = parseBlock();
			vector<CatchBlock> catches;
			bool expectingMore = true;
			while (match(TokenType::CATCH)) {
				if (!expectingMore) {
					error(
						"Unreachable catch block. Previous catch used 'do' "
						"(implying "
						"terminal), expected 'then do'.");
				}
				vector<string> types;
				do {
					consume(TokenType::IDENTIFIER, "Expected error type");
					types.push_back(tokens[pos - 1].value);
				} while (match(TokenType::COMMA));
				if (match(TokenType::THEN)) {
					consume(TokenType::DO, "Expected 'do' after 'then'");
					expectingMore = true;
				} else {
					consume(TokenType::DO,
						"Expected 'do' or 'then do' after catch types");
					expectingMore = false;
				}
				consume(TokenType::COLON, "Expected ':'");
				catches.push_back({types, parseBlock()});
			}
			if (expectingMore && !catches.empty()) {
				error("Expected another 'catch' block after 'then do'.");
			}
			vector<Stmt *> elseBody;
			if (match(TokenType::ELSE)) {
				consume(TokenType::DO, "Expected 'do' after else");
				consume(TokenType::COLON, "Expected ':'");
				elseBody = parseBlock();
			}
			vector<Stmt *> finallyBody;
			if (match(TokenType::FINALLY)) {
				consume(TokenType::DO, "Expected 'do' after finally");
				consume(TokenType::COLON, "Expected ':'");
				finallyBody = parseBlock();
			}
			return setPos(new TryStmt(tryBody, catches, elseBody, finallyBody), t);
		}
		if (match(TokenType::THROW)) {
			Token t = tokens[pos - 1];
			consume(TokenType::IDENTIFIER,
				"Expected error type identifier after 'throw'");
			string typeName = tokens[pos - 1].value;
			consume(TokenType::COMMA, "Expected ',' after error type");
			Expr *msg = parseExpr();
			return setPos(new ThrowStmt(typeName, msg), t);
		}
		Expr *e = parseExpr();
		if (match(TokenType::COMMA)) {
			Token t = tokens[pos - 1];
			vector<Expr *> targets;
			targets.push_back(e);
			do {
				targets.push_back(parseExpr());
			} while (match(TokenType::COMMA));
			consume(TokenType::ASSIGN, "Expected '=' in multi-assignment");
			vector<Expr *> values;
			do {
				values.push_back(parseExpr());
			} while (match(TokenType::COMMA));
			if (targets.size() != values.size())
				throw SyntaxError("Mismatch in number of targets and values",
					t.line, t.col);
			return setPos(new MultiAssignStmt(targets, values), t);
		}
		if (match(TokenType::ASSIGN) || match(TokenType::PLUS_EQ) ||
			 match(TokenType::MINUS_EQ) || match(TokenType::STAR_EQ) ||
			 match(TokenType::DIV_EQ) || match(TokenType::MOD_EQ) ||
			 match(TokenType::FLOOR_DIV_EQ) || match(TokenType::POW_EQ) ||
			 match(TokenType::AND_EQ) || match(TokenType::OR_EQ) ||
			 match(TokenType::XOR_EQ)) {
			Token op = tokens[pos - 1];
			Expr *rhs = parseExpr();
			return setPos(new AssignStmt(e, op.type, rhs), op);
		}
		if (match(TokenType::INCREMENT)) {
			Token op = tokens[pos - 1];
			if (!dynamic_cast<VarExpr *>(e) && !dynamic_cast<IndexExpr *>(e) &&
				 !dynamic_cast<GetExpr *>(e)) {
				throw SyntaxError("++ requires assignable expression", op.line,
					op.col);
			}
			return setPos(new AssignStmt(e, TokenType::PLUS_EQ,
								  setPos(new NumberExpr(1, false), op)),
				op);
		}
		if (match(TokenType::DECREMENT)) {
			Token op = tokens[pos - 1];
			if (!dynamic_cast<VarExpr *>(e) && !dynamic_cast<IndexExpr *>(e) &&
				 !dynamic_cast<GetExpr *>(e)) {
				throw SyntaxError("-- requires assignable expression", op.line,
					op.col);
			}
			return setPos(new AssignStmt(e, TokenType::MINUS_EQ,
								  setPos(new NumberExpr(1, false), op)),
				op);
		}
		Stmt *s = new ExprStmt(e);
		s->line = e->line;
		s->col = e->col;
		return s;
	}
};
// -------------------- RUNTIME -------------------
struct Env;
struct HeapObject;
struct BigIntObject;
struct ErrorObject;
struct ClassObject;
struct InstanceObject;
using NativeFunc = std::function<Value(const std::vector<Value> &, int, int)>;
using Inspector = std::function<std::string(const Value &)>;
struct ValueHash {
	std::size_t operator()(const Value &v) const;
};
struct ValueEqual {
	bool operator()(const Value &a, const Value &b) const;
};
struct Value {
	ValueType type = ValueType::NOTYPE;
	bool isConst = false;
	bool isLocked = false;
#ifdef VM_DEBUG_MODE
	std::string __DEBUGGING__NAME__ = "";
#endif
	union {
		long long iVal;
		double fVal;
		bool bVal;
	};
	union {
		Value *ptr;
		void *adress;
	};
	std::shared_ptr<HeapObject> ref;
	static Value Reference(Value *p, std::shared_ptr<HeapObject> owner);
	static Value Int(long long v, bool locked = false, bool isConst = false);
	static Value pInt(void *v, bool locked = false, bool isConst = false);
	static Value BigInt(long long n);
	static Value BigInt(std::vector<uint32_t> chunks, bool isNegative);
	static Value BigInt(std::shared_ptr<BigIntObject> obj);
	static Value Float(double v, bool locked = false, bool isConst = false);
	static Value Bool(bool v, bool locked = false, bool isConst = false);
	static Value String(const string &v, bool locked = false,
		bool isConst = false);
	static Value None();
	static Value NoType();
	static Value List();
	static Value List(const std::vector<Value> &elems, bool locked = false,
		bool isConst = false);
	static Value Range(double s, double e, double st, bool si, bool ei, bool f,
		bool locked = false, bool isConst = false);
	static Value Set();
	static Value Set(const std::unordered_set<Value, ValueHash, ValueEqual> &elems, bool locked = false);
	static Value Tuple(const std::vector<Value> &elems, bool locked = false);
	static Value Dict(const std::unordered_map<Value, Value, ValueHash, ValueEqual> &m, bool locked = false);
	static Value Paired(const std::vector<std::pair<Value, Value>> &p);
	static Value Native(NativeFunc f);
	static Value Overload(const Value &first);
	static Value Omit();
	static Value File(const string &path);
	static Value Slice(Value s, Value e, Value p);
	static Value Vector(const std::vector<Value> &elems);
	static Value Error(std::shared_ptr<ErrorObject> e);
	static Value Class(const string &name);
	static Value Instance(Value classObj);
	static Value FromExisting(std::shared_ptr<HeapObject> existingRef, ValueType type);
	bool isTruthy() const;
	bool strictEquals(const Value &other) const;
	bool looseEquals(const Value &other) const;
	bool isNumber() const;
	long long asInt() const;
	void *aspInt() const;
	double asFloat() const;
	bool asBool() const;
	const string &asString() const;
	bool sameType(const Value &other) const { return type == other.type; }
};
struct HeapObject {
	std::string name = "";
	ValueType type;
	bool typeLocked = false;
	HeapObject(ValueType t, bool locked = false) : type(t), typeLocked(locked) {}
	virtual ~HeapObject() = default;
};
struct StringObject : HeapObject {
	string value;
	StringObject(const string &v, bool locked = false)
		 : HeapObject(ValueType::STRING, locked), value(v) {}
};
struct ListObject : HeapObject {
	std::vector<Value> elements;
	ListObject() : HeapObject(ValueType::LIST) {}
	ListObject(const std::vector<Value> &elems, bool locked = false)
		 : HeapObject(ValueType::LIST, locked), elements(elems) {}
};
struct RangeObject : HeapObject {
	double start, end, step;
	bool startInclusive, endInclusive, isFloat, isValid;
	RangeObject(double s, double e, double st, bool si, bool ei, bool f,
		bool valid, bool locked = false)
		 : HeapObject(ValueType::RANGE, locked),
			start(s),
			end(e),
			step(st),
			startInclusive(si),
			endInclusive(ei),
			isFloat(f),
			isValid(valid) {}
};
struct SetObject : HeapObject {
	std::unordered_set<Value, ValueHash, ValueEqual> elements;
	SetObject() : HeapObject(ValueType::SET) {}
	SetObject(const std::unordered_set<Value, ValueHash, ValueEqual> &e, bool locked = false)
		 : HeapObject(ValueType::SET, locked), elements(e) {}
};
struct TupleObject : HeapObject {
	std::vector<Value> elements;
	TupleObject() : HeapObject(ValueType::TUPLE) {}
	TupleObject(const std::vector<Value> &elems, bool locked = false)
		 : HeapObject(ValueType::TUPLE, locked), elements(elems) {}
};
struct DictObject : HeapObject {
	std::unordered_map<Value, Value, ValueHash, ValueEqual> items;
	DictObject() : HeapObject(ValueType::DICT) {}
	DictObject(const std::unordered_map<Value, Value, ValueHash, ValueEqual> &m,
		bool locked = false)
		 : HeapObject(ValueType::DICT, locked), items(m) {}
};
struct PairedObject : HeapObject {
	std::vector<std::pair<Value, Value>> pairs;
	PairedObject(const std::vector<std::pair<Value, Value>> &p)
		 : HeapObject(ValueType::PAIRED), pairs(p) {}
};
struct NativeFunctionObject : HeapObject {
	NativeFunc func;
	NativeFunctionObject(NativeFunc f)
		 : HeapObject(ValueType::NATIVE_FUNCTION), func(f) {}
};
struct OverloadObject : HeapObject {
	std::vector<Value> overloads;
	OverloadObject(const Value &v) : HeapObject(ValueType::OVERLOAD) {
		overloads.push_back(v);
	}
};
struct FileObject : HeapObject {
	std::fstream stream;
	string path;
	bool isOpen;
	FileObject(const string &p) : HeapObject(ValueType::FILE), path(p) {
		stream.open(p, std::ios::in | std::ios::out | std::ios::binary);
		isOpen = stream.is_open();
	}
	void Reset() {
		stream.close();
		stream.open(path, std::ios::in | std::ios::out | std::ios::binary |
									std::ios::trunc);
		isOpen = stream.is_open();
	}
	~FileObject() {
		if (isOpen)
			stream.close();
	}
};
struct SliceObject : HeapObject {
	Value start;
	Value end;
	Value step;
	SliceObject(Value s, Value e, Value p)
		 : HeapObject(ValueType::SLICE), start(s), end(e), step(p) {}
};
struct VectorObject : HeapObject {
	vector<Value> elements;
	VectorObject(const vector<Value> &e)
		 : HeapObject(ValueType::VECTOR), elements(e) {}
};
struct BigIntObject : HeapObject {
	bool isNegative;
	std::vector<uint32_t> chunks;
	BigIntObject(long long n) : HeapObject(ValueType::BIGINT) {
		if (n < 0) {
			isNegative = true;
			n = -n;
		} else
			isNegative = false;
		if (n == 0)
			chunks.push_back(0);
		while (n > 0) {
			chunks.push_back((uint32_t)(n & 0xFFFFFFFF));
			n >>= 32;
		}
	}
	BigIntObject(std::vector<uint32_t> c, bool neg)
		 : HeapObject(ValueType::BIGINT), chunks(c), isNegative(neg) {
		trim();
	}
	void trim() {
		while (chunks.size() > 1 && chunks.back() == 0)
			chunks.pop_back();
		if (chunks.size() == 1 && chunks[0] == 0)
			isNegative = false;
	}
	bool operator==(const BigIntObject &other) const {
		return isNegative == other.isNegative && chunks == other.chunks;
	}
	bool operator<(const BigIntObject &other) const {
		if (isNegative != other.isNegative)
			return isNegative;
		if (chunks.size() != other.chunks.size())
			return isNegative ? chunks.size() > other.chunks.size()
									: chunks.size() < other.chunks.size();
		for (int i = chunks.size() - 1; i >= 0; i--) {
			if (chunks[i] != other.chunks[i])
				return isNegative ? chunks[i] > other.chunks[i]
										: chunks[i] < other.chunks[i];
		}
		return false;
	}
	bool operator>(const BigIntObject &other) const { return other < *this; }
	bool absLess(const BigIntObject &other) const {
		if (chunks.size() != other.chunks.size())
			return chunks.size() < other.chunks.size();
		for (int i = chunks.size() - 1; i >= 0; i--)
			if (chunks[i] != other.chunks[i])
				return chunks[i] < other.chunks[i];
		return false;
	}
	BigIntObject absAdd(const BigIntObject &other) const {
		std::vector<uint32_t> res;
		uint64_t carry = 0;
		size_t n = std::max(chunks.size(), other.chunks.size());
		res.reserve(n + 1);
		for (size_t i = 0; i < n || carry; i++) {
			uint64_t sum = carry + (i < chunks.size() ? chunks[i] : 0) +
								(i < other.chunks.size() ? other.chunks[i] : 0);
			res.push_back((uint32_t)(sum & 0xFFFFFFFF));
			carry = sum >> 32;
		}
		return BigIntObject(res, false);
	}
	BigIntObject absSub(const BigIntObject &other) const {
		std::vector<uint32_t> res;
		int64_t borrow = 0;
		size_t n = chunks.size();
		res.reserve(n);
		for (size_t i = 0; i < n; i++) {
			int64_t sub = (int64_t)chunks[i] -
							  (i < other.chunks.size() ? other.chunks[i] : 0) - borrow;
			if (sub < 0) {
				sub += 4294967296LL;
				borrow = 1;
			} else
				borrow = 0;
			res.push_back((uint32_t)sub);
		}
		return BigIntObject(res, false);
	}
	BigIntObject operator+(const BigIntObject &other) const {
		if (isNegative == other.isNegative) {
			BigIntObject res = absAdd(other);
			res.isNegative = isNegative;
			return res;
		} else {
			if (absLess(other)) {
				BigIntObject res = other.absSub(*this);
				res.isNegative = other.isNegative;
				return res;
			} else {
				BigIntObject res = absSub(other);
				res.isNegative = isNegative;
				return res;
			}
		}
	}
	BigIntObject operator-(const BigIntObject &other) const {
		if (isNegative != other.isNegative) {
			BigIntObject res = absAdd(other);
			res.isNegative = isNegative;
			return res;
		} else {
			if (absLess(other)) {
				BigIntObject res = other.absSub(*this);
				res.isNegative = !isNegative;
				return res;
			} else {
				BigIntObject res = absSub(other);
				res.isNegative = isNegative;
				return res;
			}
		}
	}
	BigIntObject operator*(const BigIntObject &other) const {
		size_t n = chunks.size(), m = other.chunks.size();
		std::vector<uint32_t> res(n + m, 0);
		for (size_t i = 0; i < n; i++) {
			uint64_t carry = 0;
			for (size_t j = 0; j < m; j++) {
				uint64_t prod =
					(uint64_t)chunks[i] * other.chunks[j] + res[i + j] + carry;
				res[i + j] = (uint32_t)(prod & 0xFFFFFFFF);
				carry = prod >> 32;
			}
			res[i + m] += (uint32_t)carry;
		}
		return BigIntObject(res, isNegative != other.isNegative);
	}
	std::pair<BigIntObject, BigIntObject> divMod(const BigIntObject &other) const {
		if (other.chunks.size() == 1 && other.chunks[0] == 0)
			throw std::runtime_error("Divide by zero");
		BigIntObject dividend = *this;
		dividend.isNegative = false;
		BigIntObject divisor = other;
		divisor.isNegative = false;
		BigIntObject quotient(0);
		BigIntObject remainder(0);
		if (dividend < divisor)
			return {BigIntObject(0), *this};
		for (int i = dividend.chunks.size() * 32 - 1; i >= 0; i--) {
			remainder.lshift(1);
			int chunkIdx = i / 32;
			int bitIdx = i % 32;
			if ((dividend.chunks[chunkIdx] >> bitIdx) & 1) {
				remainder.chunks[0] |= 1;
			}
			if (!remainder.absLess(divisor)) {
				remainder = remainder.absSub(divisor);
				quotient.setBit(i);
			}
		}
		quotient.isNegative = isNegative != other.isNegative;
		remainder.isNegative = isNegative;
		return {quotient, remainder};
	}
	BigIntObject operator/(const BigIntObject &other) const {
		return divMod(other).first;
	}
	BigIntObject operator%(const BigIntObject &other) const {
		return divMod(other).second;
	}
	void lshift(int shift) {
		if (shift == 0)
			return;
		int chunkShift = shift / 32;
		int bitShift = shift % 32;

		std::vector<uint32_t> newChunks(chunks.size() + chunkShift, 0);

		uint32_t carry = 0;
		for (size_t i = 0; i < chunks.size(); i++) {
			uint64_t val = chunks[i];
			newChunks[i + chunkShift] = (val << bitShift) | carry;
			carry = (bitShift == 0) ? 0 : (val >> (32 - bitShift));
		}
		if (carry)
			newChunks.push_back(carry);
		chunks = std::move(newChunks);
		trim();
	}
	void setBit(int n) {
		int chunkIdx = n / 32;
		int bitIdx = n % 32;
		if (chunkIdx >= chunks.size())
			chunks.resize(chunkIdx + 1, 0);
		chunks[chunkIdx] |= (1U << bitIdx);
	}
	static Value add(Value a, Value b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(a.ref.get())
									 : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(b.ref.get())
									 : &(tempB = BigIntObject(b.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba + *bb));
	}
	static Value sub(Value a, Value b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(a.ref.get())
									 : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(b.ref.get())
									 : &(tempB = BigIntObject(b.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba - *bb));
	}
	static Value mul(Value a, Value b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(a.ref.get())
									 : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(b.ref.get())
									 : &(tempB = BigIntObject(b.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba * *bb));
	}
	static Value div(Value a, Value b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(a.ref.get())
									 : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(b.ref.get())
									 : &(tempB = BigIntObject(b.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba / *bb));
	}
	static Value mod(Value a, Value b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(a.ref.get())
									 : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(b.ref.get())
									 : &(tempB = BigIntObject(b.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba % *bb));
	}
	static Value pow(Value a, Value b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(a.ref.get())
									 : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(b.ref.get())
									 : &(tempB = BigIntObject(b.asInt()));
		if (bb->isNegative)
			return Value::Int(0);
		BigIntObject base = *ba;
		BigIntObject exp = *bb;
		BigIntObject res(1);
		while (!exp.absLess(BigIntObject(1)) &&
				 !(exp.chunks.size() == 1 && exp.chunks[0] == 0)) {
			if (exp.chunks[0] & 1)
				res = res * base;
			uint32_t carry = 0;
			for (int i = exp.chunks.size() - 1; i >= 0; i--) {
				uint32_t nextCarry = (exp.chunks[i] & 1) << 31;
				exp.chunks[i] = (exp.chunks[i] >> 1) | carry;
				carry = nextCarry;
			}
			exp.trim();
			base = base * base;
		}
		return Value::BigInt(std::make_shared<BigIntObject>(res));
	}
};
struct ErrorObject : HeapObject {
	string message;
	string errType;
	long long code;
	int line, col;
	ErrorObject(string type, string msg, long long co, int l, int c)
		 : HeapObject(ValueType::ERROR),
			errType(type),
			message(msg),
			code(co),
			line(l),
			col(c) {}
	string toString() const { return errType + ": " + message; }
};
struct ClassObject : HeapObject {
	string name;
	vector<Value> parents;
	vector<ClassObject *> mro;
	unordered_map<string, Value> staticFields;
	unordered_map<std::string, AccessLevel> fieldAccess;
	struct MethodInfo {
		Value func;
		AccessLevel access;
	};
	unordered_map<string, MethodInfo> methods;
	ClassObject(const string &n) : HeapObject(ValueType::CLASS), name(n) {
		mro.push_back(this);
	}
	void computeMRO() {
		mro.clear();
		mro.push_back(this);
		std::vector<std::vector<ClassObject *>> lists;
		for (auto &pVal : parents) {
			if (pVal.type == ValueType::CLASS) {
				auto *pClass = static_cast<ClassObject *>(pVal.ref.get());
				lists.push_back(pClass->mro);
			}
		}
		std::vector<ClassObject *> parentsList;
		for (auto &pVal : parents)
			if (pVal.type == ValueType::CLASS)
				parentsList.push_back(static_cast<ClassObject *>(pVal.ref.get()));
		if (!parentsList.empty())
			lists.push_back(parentsList);
		while (true) {
			for (size_t i = 0; i < lists.size();) {
				if (lists[i].empty())
					lists.erase(lists.begin() + i);
				else
					i++;
			}
			if (lists.empty())
				break;
			ClassObject *candidate = nullptr;
			for (size_t i = 0; i < lists.size(); i++) {
				ClassObject *head = lists[i][0];
				bool valid = true;
				for (size_t j = 0; j < lists.size(); j++) {
					for (size_t k = 1; k < lists[j].size(); k++) {
						if (lists[j][k] == head) {
							valid = false;
							break;
						}
					}
					if (!valid)
						break;
				}
				if (valid) {
					candidate = head;
					break;
				}
			}
			if (!candidate)
				break;
			mro.push_back(candidate);
			for (size_t i = 0; i < lists.size(); i++)
				if (lists[i].front() == candidate)
					lists[i].erase(lists[i].begin());
		}
	}
	string toString() const { return "<class '" + name + "'>"; }
};
struct InstanceObject : HeapObject {
	ClassObject *klass;
	unordered_map<string, Value> fields;
	InstanceObject(ClassObject *k) : HeapObject(ValueType::INSTANCE), klass(k) {}
	string toString() const { return "<instance of '" + klass->name + "'>"; }
};
struct SuperObject : HeapObject {
	Value instance;
	ClassObject *startClass;
	SuperObject(Value inst, ClassObject *start)
		 : HeapObject(ValueType::SUPER), instance(inst), startClass(start) {}
	string toString() const { return "<super: " + startClass->name + ">"; }
};
inline Value Value::Reference(Value *p, std::shared_ptr<HeapObject> owner) {
	Value v;
	v.type = ValueType::REFERENCE;
	v.ptr = p;
	v.ref = owner;
	return v;
}
inline Value Value::Int(long long v, bool locked, bool isConst) {
	Value x;
	x.type = ValueType::INT;
	x.iVal = v;
	x.isConst = isConst;
	return x;
}
inline Value Value::pInt(void *v, bool locked, bool isConst) {
	Value x;
	x.type = ValueType::INT;
	x.adress = v;
	x.isConst = isConst;
	return x;
}
inline Value Value::BigInt(long long n) {
	Value v;
	v.type = ValueType::BIGINT;
	v.ref = std::make_shared<BigIntObject>(n);
	return v;
}
inline Value Value::BigInt(std::vector<uint32_t> chunks, bool isNegative) {
	Value v;
	v.type = ValueType::BIGINT;
	v.ref = std::make_shared<BigIntObject>(chunks, isNegative);
	return v;
}
inline Value Value::BigInt(std::shared_ptr<BigIntObject> obj) {
	Value v;
	v.type = ValueType::BIGINT;
	v.ref = obj;
	return v;
}
inline Value Value::Float(double v, bool locked, bool isConst) {
	Value x;
	x.type = ValueType::FLOAT;
	x.fVal = v;
	x.isConst = isConst;
	return x;
}
inline Value Value::Bool(bool v, bool locked, bool isConst) {
	Value x;
	x.type = ValueType::BOOL;
	x.bVal = v;
	x.isConst = isConst;
	return x;
}
inline Value Value::String(const string &v, bool locked, bool isConst) {
	Value x;
	x.type = ValueType::STRING;
	x.ref = std::make_shared<StringObject>(v, locked);
	return x;
}
inline Value Value::None() {
	Value x;
	x.type = ValueType::NONE;
	return x;
}
inline Value Value::NoType() {
	Value x;
	x.type = ValueType::NOTYPE;
	return x;
}
inline Value Value::List() {
	Value x;
	x.type = ValueType::LIST;
	x.ref = std::make_shared<ListObject>();
	return x;
}
inline Value Value::List(const std::vector<Value> &elems, bool locked, bool isConst) {
	Value x;
	x.type = ValueType::LIST;
	x.ref = std::make_shared<ListObject>(elems, locked);
	return x;
}
inline Value Value::Set() {
	Value x;
	x.type = ValueType::SET;
	x.ref = std::make_shared<SetObject>();
	return x;
}
inline Value Value::Set(const std::unordered_set<Value, ValueHash, ValueEqual> &elems, bool locked) {
	Value x;
	x.type = ValueType::SET;
	x.ref = std::make_shared<SetObject>(elems, locked);
	return x;
}
inline Value Value::Tuple(const std::vector<Value> &elems, bool locked) {
	Value x;
	x.type = ValueType::TUPLE;
	std::vector<Value> constElems = elems;
	for (auto &e : constElems)
		e.isConst = true;
	x.ref = std::make_shared<TupleObject>(constElems, locked);
	return x;
}
inline Value Value::Dict(const std::unordered_map<Value, Value, ValueHash, ValueEqual> &m, bool locked) {
	Value x;
	x.type = ValueType::DICT;
	x.ref = std::make_shared<DictObject>(m, locked);
	return x;
}
inline Value Value::Paired(const std::vector<std::pair<Value, Value>> &p) {
	Value x;
	x.type = ValueType::PAIRED;
	x.ref = std::make_shared<PairedObject>(p);
	return x;
}
inline Value Value::Native(NativeFunc f) {
	Value x;
	x.type = ValueType::NATIVE_FUNCTION;
	x.ref = std::make_shared<NativeFunctionObject>(f);
	return x;
}
inline Value Value::Overload(const Value &first) {
	Value x;
	x.type = ValueType::OVERLOAD;
	x.ref = std::make_shared<OverloadObject>(first);
	return x;
}
inline Value Value::Omit() {
	Value v;
	v.type = ValueType::OMIT_MARKER;
	return v;
}
inline Value Value::File(const string &path) {
	Value x;
	x.type = ValueType::FILE;
	x.ref = std::make_shared<FileObject>(path);
	return x;
}
inline Value Value::Range(double s, double e, double st, bool si, bool ei, bool f, bool locked, bool isConst) {
	bool isInvalid = false;
	if (st == 0)
		isInvalid = true;
	else if (st > 0) {
		if (s > e)
			isInvalid = true;
		else if (s == e && (!si || !ei))
			isInvalid = true;
	} else {
		if (s < e)
			isInvalid = true;
		else if (s == e && (!si || !ei))
			isInvalid = true;
	}
	if (isInvalid) {
		s = 0;
		e = 0;
		st = 1;
		si = false;
		ei = false;
		f = false;
	}
	Value x;
	x.type = ValueType::RANGE;
	x.ref =
		std::make_shared<RangeObject>(s, e, st, si, ei, f, !isInvalid, locked);
	return x;
}
inline Value Value::Slice(Value s, Value e, Value p) {
	Value x;
	x.type = ValueType::SLICE;
	x.ref = std::make_shared<SliceObject>(s, e, p);
	return x;
}
inline Value Value::Vector(const std::vector<Value> &elems) {
	Value x;
	x.type = ValueType::VECTOR;
	x.ref = std::make_shared<VectorObject>(elems);
	return x;
}
inline Value Value::Error(std::shared_ptr<ErrorObject> e) {
	Value v;
	v.type = ValueType::ERROR;
	v.ref = e;
#ifdef VM_DEBUG_MODE
	v.__DEBUGGING__NAME__ = e->errType;
#endif
	return v;
}
inline Value Value::Class(const string &name) {
	Value v;
#ifdef VM_DEBUG_MODE
	v.__DEBUGGING__NAME__ = name;
#endif
	v.type = ValueType::CLASS;
	v.ref = make_shared<ClassObject>(name);
	return v;
}
inline Value Value::Instance(Value classDef) {
	if (classDef.type != ValueType::CLASS)
		throw RuntimeError("Cannot create instance from non-class type", 0, 0);
	Value v;
	v.type = ValueType::INSTANCE;
	auto *clsPtr = static_cast<ClassObject *>(classDef.ref.get());
#ifdef VM_DEBUG_MODE
	v.__DEBUGGING__NAME__ = clsPtr->name;
#endif
	v.ref = make_shared<InstanceObject>(clsPtr);
	return v;
}
inline bool Value::isTruthy() const {
	if (type == ValueType::NONE || type == ValueType::NOTYPE)
		return false;
	if (type == ValueType::BOOL)
		return bVal;
	if (type == ValueType::INT)
		return iVal != 0;
	if (type == ValueType::FLOAT)
		return fVal != 0.0;
	if (type == ValueType::STRING)
		return !static_cast<StringObject *>(ref.get())->value.empty();
	if (type == ValueType::LIST)
		return !static_cast<ListObject *>(ref.get())->elements.empty();
	if (type == ValueType::SET)
		return !static_cast<SetObject *>(ref.get())->elements.empty();
	if (type == ValueType::TUPLE)
		return !static_cast<TupleObject *>(ref.get())->elements.empty();
	if (type == ValueType::DICT)
		return !static_cast<DictObject *>(ref.get())->items.empty();
	if (type == ValueType::RANGE) {
		auto *r = static_cast<RangeObject *>(ref.get());
		if (!r->isValid || r->step == 0)
			return false;
		double s = r->start;
		if (!r->startInclusive)
			s += r->step;
		if (r->step > 0)
			return (s < r->end) || (r->endInclusive && s == r->end);
		else
			return (s > r->end) || (r->endInclusive && s == r->end);
	}
	return false;
}
inline long long Value::asInt() const {
	if (type == ValueType::INT)
		return iVal;
	if (type == ValueType::FLOAT)
		return (long long)fVal;
	if (type == ValueType::BOOL)
		return bVal ? 1 : 0;
	return 0;
}
inline void *Value::aspInt() const { return adress; }
inline double Value::asFloat() const {
	if (type == ValueType::FLOAT)
		return fVal;
	if (type == ValueType::INT)
		return (double)iVal;
	if (type == ValueType::BOOL)
		return bVal ? 1.0 : 0.0;
	return 0.0;
}
inline bool Value::asBool() const {
	if (type == ValueType::BOOL)
		return bVal;
	if (type == ValueType::INT)
		return iVal != 0;
	if (type == ValueType::FLOAT)
		return fVal != 0.0;
	return isTruthy();
}
inline const string &Value::asString() const {
	static string empty = "";
	if (type == ValueType::STRING)
		return static_cast<StringObject *>(ref.get())->value;
	return empty;
}
inline bool Value::isNumber() const {
	return type == ValueType::INT || type == ValueType::FLOAT ||
			 type == ValueType::BIGINT;
}
inline bool Value::strictEquals(const Value &other) const {
	if (type != other.type)
		return false;
	switch (type) {
	case ValueType::INT:
		return iVal == other.iVal;
	case ValueType::FLOAT:
		return fVal == other.fVal;
	case ValueType::BOOL:
		return bVal == other.bVal;
	case ValueType::STRING:
		return asString() == other.asString();
	case ValueType::NONE:
	case ValueType::NOTYPE:
		return true;
	case ValueType::BIGINT: {
		return *static_cast<BigIntObject *>(ref.get()) ==
				 *static_cast<BigIntObject *>(other.ref.get());
	}
	case ValueType::LIST: {
		auto *l1 = static_cast<ListObject *>(ref.get());
		auto *l2 = static_cast<ListObject *>(other.ref.get());
		if (l1->elements.size() != l2->elements.size())
			return false;
		for (size_t i = 0; i < l1->elements.size(); ++i)
			if (!l1->elements[i].strictEquals(l2->elements[i]))
				return false;
		return true;
	}
	case ValueType::SET: {
		auto *s1 = static_cast<SetObject *>(ref.get());
		auto *s2 = static_cast<SetObject *>(other.ref.get());
		if (s1->elements.size() != s2->elements.size())
			return false;
		for (const auto &v1 : s1->elements) {
			bool found = false;
			for (const auto &v2 : s2->elements)
				if (v1.strictEquals(v2)) {
					found = true;
					break;
				}
			if (!found)
				return false;
		}
		return true;
	}
	case ValueType::RANGE: {
		auto *r1 = static_cast<RangeObject *>(ref.get());
		auto *r2 = static_cast<RangeObject *>(other.ref.get());
		return r1->start == r2->start && r1->end == r2->end &&
				 r1->step == r2->step &&
				 r1->startInclusive == r2->startInclusive &&
				 r1->endInclusive == r2->endInclusive;
	}
	case ValueType::TUPLE: {
		auto *t1 = static_cast<TupleObject *>(ref.get());
		auto *t2 = static_cast<TupleObject *>(other.ref.get());
		if (t1->elements.size() != t2->elements.size())
			return false;
		for (size_t i = 0; i < t1->elements.size(); ++i)
			if (!t1->elements[i].strictEquals(t2->elements[i]))
				return false;
		return true;
	}
	case ValueType::DICT: {
		auto *d1 = static_cast<DictObject *>(ref.get());
		auto *d2 = static_cast<DictObject *>(other.ref.get());
		if (d1->items.size() != d2->items.size())
			return false;
		for (const auto &[k, v] : d1->items) {
			if (d2->items.find(k) == d2->items.end())
				return false;
			if (!v.strictEquals(d2->items.at(k)))
				return false;
		}
		return true;
	}
	case ValueType::PAIRED:
		return false;
	case ValueType::VECTOR: {
		auto *v1 = static_cast<VectorObject *>(ref.get());
		auto *v2 = static_cast<VectorObject *>(other.ref.get());
		if (v1->elements.size() != v2->elements.size())
			return false;
		for (size_t i = 0; i < v1->elements.size(); i++) {
			if (std::abs(v1->elements[i].asInt() - v2->elements[i].asInt()) >
				 1e-9)
				return false;
		}
		return true;
	}
	}
	return false;
}
inline bool Value::looseEquals(const Value &other) const {
	return isTruthy() == other.isTruthy();
}
inline Value Value::FromExisting(std::shared_ptr<HeapObject> existingRef, ValueType type) {
	Value v;
	v.type = type;
	v.ref = std::move(existingRef);
	return v;
}
inline std::size_t ValueHash::operator()(const Value &v) const {
	size_t seed = 0;
	auto hash_combine = [&](size_t hash) {
		seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	};
	switch (v.type) {
	case ValueType::INT:
		hash_combine(std::hash<long long>{}(v.asInt()));
		break;
	case ValueType::FLOAT:
		hash_combine(std::hash<double>{}(v.asFloat()));
		break;
	case ValueType::BOOL:
		hash_combine(std::hash<bool>{}(v.asBool()));
		break;
	case ValueType::STRING:
		hash_combine(std::hash<string>{}(v.asString()));
		break;
	case ValueType::NONE:
		hash_combine(0);
		break;
	case ValueType::INSTANCE: {
		auto *inst = static_cast<InstanceObject *>(v.ref.get());
		hash_combine(std::hash<string>{}(inst->klass->name));
		size_t fieldsXor = 0;
		for (const auto &[key, val] : inst->fields) {
			size_t pairHash = std::hash<string>{}(key);
			size_t valHash = ValueHash{}(val);
			pairHash ^=
				valHash + 0x9e3779b9 + (pairHash << 6) + (pairHash >> 2);
			fieldsXor ^= pairHash;
		}
		hash_combine(fieldsXor);
		break;
	}
	case ValueType::TUPLE: {
		auto *t = static_cast<TupleObject *>(v.ref.get());
		for (const auto &elem : t->elements)
			hash_combine(ValueHash{}(elem));
		break;
	}
	case ValueType::LIST: {
		auto *l = static_cast<ListObject *>(v.ref.get());
		for (const auto &elem : l->elements)
			hash_combine(ValueHash{}(elem));
		break;
	}
	case ValueType::SET: {
		auto *s = static_cast<SetObject *>(v.ref.get());
		size_t xorSum = 0;
		for (const auto &elem : s->elements)
			xorSum ^= ValueHash{}(elem);
		hash_combine(xorSum);
		break;
	}
	case ValueType::VECTOR: {
		auto *vec = static_cast<VectorObject *>(v.ref.get());
		hash_combine(vec->elements.size());
		for (auto &elem : vec->elements) {
			hash_combine(ValueHash{}(elem));
		}
		break;
	}
	default:
		hash_combine(std::hash<int>{}((int)v.type));
	}
	return seed;
}
inline bool ValueEqual::operator()(const Value &a, const Value &b) const {
	return a.strictEquals(b);
}
struct VectorHash {
	std::size_t operator()(const std::vector<Value> &vec) const {
		std::size_t seed = vec.size();
		ValueHash hasher;
		for (const auto &v : vec)
			seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};
struct VectorEqual {
	bool operator()(const std::vector<Value> &a,
		const std::vector<Value> &b) const {
		if (a.size() != b.size())
			return false;
		for (size_t i = 0; i < a.size(); ++i)
			if (!(a[i].strictEquals(b[i])))
				return false;
		return true;
	}
};
static void setAdd(std::unordered_set<Value, ValueHash, ValueEqual> &elems, const Value &v);
Value shallowCopy(const Value &v) {
	switch (v.type) {
	case ValueType::LIST: {
		auto *oldObj = static_cast<ListObject *>(v.ref.get());
		return Value::List(oldObj->elements);
	}
	case ValueType::SET: {
		auto *oldObj = static_cast<SetObject *>(v.ref.get());
		return Value::Set(oldObj->elements);
	}
	case ValueType::TUPLE: {
		auto *oldObj = static_cast<TupleObject *>(v.ref.get());
		return Value::Tuple(oldObj->elements);
	}
	case ValueType::DICT: {
		auto *oldObj = static_cast<DictObject *>(v.ref.get());
		return Value::Dict(oldObj->items, v.ref->typeLocked);
	}
	default:
		return v;
	}
}
Value deepCopy(const Value &v) {
	Value out;
	if (v.type == ValueType::INT || v.type == ValueType::FLOAT ||
		 v.type == ValueType::BOOL) {
		return v;
	}
	switch (v.type) {
	case ValueType::STRING:
		out = Value::String(static_cast<StringObject *>(v.ref.get())->value);
		break;
	case ValueType::LIST: {
		auto *oldList = static_cast<ListObject *>(v.ref.get());
		std::vector<Value> copied;
		for (const auto &el : oldList->elements)
			copied.push_back(deepCopy(el));
		out = Value::List(copied);
		break;
	}
	case ValueType::BIGINT: {
		auto *oldBig = static_cast<BigIntObject *>(v.ref.get());
		out = Value::BigInt(oldBig->chunks, oldBig->isNegative);
		break;
	}
	case ValueType::SET: {
		auto *oldSet = static_cast<SetObject *>(v.ref.get());
		std::unordered_set<Value, ValueHash, ValueEqual> copied;
		for (const auto &el : oldSet->elements)
			copied.insert(deepCopy(el));
		out = Value::Set(copied);
		break;
	}
	case ValueType::DICT: {
		auto *oldDict = static_cast<DictObject *>(v.ref.get());
		std::unordered_map<Value, Value, ValueHash, ValueEqual> copied;
		for (const auto &[key, val] : oldDict->items)
			copied[deepCopy(key)] = deepCopy(val);
		out = Value::Dict(copied, v.ref->typeLocked);
		break;
	}
	case ValueType::VECTOR: {
		auto *vec = static_cast<VectorObject *>(v.ref.get());
		std::vector<Value> copied;
		copied.reserve(vec->elements.size());
		for (const auto &el : vec->elements)
			copied.push_back(deepCopy(el));
		out = Value::Vector(copied);
		break;
	}
	default:
		out = v;
		break;
	}
	return out;
}
Value applyCopy(const Value &v, CopyMode mode) {
	switch (mode) {
	case CopyMode::REF:
		return v;
	case CopyMode::DEEP:
		return deepCopy(v);
	case CopyMode::SHALLOW:
	default:
		return shallowCopy(v);
	}
}
static Value defaultOf(ValueType t) {
	switch (t) {
	case ValueType::INT:
		return Value::Int(0);
	case ValueType::FLOAT:
		return Value::Float(0.0);
	case ValueType::BOOL:
		return Value::Bool(false);
	case ValueType::STRING:
		return Value::String("");
	case ValueType::NONE:
		return Value::None();
	case ValueType::NOTYPE:
		return Value::NoType();
	case ValueType::LIST:
		return Value::List({});
	case ValueType::DICT:
		return Value::Dict({});
	case ValueType::SET:
		return Value::Set({});
	case ValueType::TUPLE:
		return Value::Tuple({});
	}
	return Value::NoType();
}
inline int getGlobalVarId(const std::string &name) {
	static std::unordered_map<std::string, int> pool;
	static int nextId = 0;
	if (pool.find(name) == pool.end())
		pool[name] = nextId++;
	return pool[name];
}
struct Var {
	Value value;
	Value *alias = nullptr;
	bool isConst = false;
	bool isLocked = false;
};
struct LValue {
	Value *ref = nullptr;
	bool isRefTarget = false;
	bool isConstView = false;
	bool isLocked = false;
	LValue() = default;
	LValue(Value *r, bool i, bool c = false, bool l = false)
		 : ref(r), isRefTarget(i), isConstView(c), isLocked(l) {}
};
struct CallArg {
	Value value;
	LValue lvalue;
	bool hasLValue = false;
};
struct Env {
	unordered_map<string, Var> vars;
	std::shared_ptr<Env> parent = nullptr;
	void clear() {
		vars.clear();
	}
	Var &lookup(const string &n) {
		if (vars.count(n))
			return vars[n];
		if (parent)
			return parent->lookup(n);
		static Var nullVar;
		return nullVar;
	}
	bool existsLocal(const string &n) { return vars.count(n); }
	void set(const string &n, Value v, bool locked, bool isConstVar = false) {
#ifdef VM_DEBUG_MODE
		v.__DEBUGGING__NAME__ = n;
#endif
		if (vars.count(n)) {
			Var &existing = vars[n];
			if (existing.isLocked && existing.value.type != v.type) {
				throw RuntimeError(
					"Type mismatch: variable '" + n + "' is type-locked.", 0, 0);
			}
			if (existing.isConst)
				throw RuntimeError(
					"Cannot reassign a constant variable '" + n + "'", 0, 0);
			existing.value = v;
			v.isLocked = existing.isLocked;
			v.isConst = existing.isConst;
			return;
		}
		v.isLocked = locked;
		v.isConst = isConstVar;
		if (locked && v.ref)
			v.ref->typeLocked = true;
		vars[n] = Var{v, nullptr, isConstVar, locked};
	}
	Value get(const string &n) {
		Var &var = lookup(n);
		Value val = var.alias ? *var.alias : var.value;
		val.isConst = var.isConst;
		val.isLocked = var.isLocked;
		return val;
	}
	void assign(const string &n, Value v) {
		Var &var = lookup(n);

		if (var.alias) {
			*var.alias = v;
			return;
		}

		var.value = v;
	}
	bool exists(const string &n) {
		if (vars.count(n))
			return true;
		if (parent)
			return parent->exists(n); // <--- Recursively check parent
		return false;
	}
};
static inline int divMod10(std::vector<uint32_t> &chunks) {
	uint64_t remainder = 0;
	for (int i = chunks.size() - 1; i >= 0; i--) {
		uint64_t combined = (remainder << 32) | chunks[i];
		chunks[i] = (uint32_t)(combined / 10);
		remainder = combined % 10;
	}
	while (chunks.size() > 1 && chunks.back() == 0)
		chunks.pop_back();
	return (int)remainder;
}
static inline std::string bigIntToString(BigIntObject *big) {
	if (big->chunks.empty())
		return "0";
	if (big->chunks.size() == 1 && big->chunks[0] == 0)
		return "0";
	std::vector<uint32_t> temp = big->chunks;
	std::string res = "";
	while (temp.size() > 1 || temp[0] > 0) {
		int digit = divMod10(temp);
		res += std::to_string(digit);
	}
	if (res.empty())
		return "0";
	if (big->isNegative)
		res += "-";
	std::reverse(res.begin(), res.end());
	return res;
}
static inline std::string formatNumber(double val) {
	std::string s = std::to_string(val);
	s.erase(s.find_last_not_of('0') + 1, std::string::npos);
	if (s.back() == '.')
		s.pop_back();
	return s;
}
static inline std::string ptr_to_string(void *p) {
	std::ostringstream oss;
	oss << p;
	return oss.str();
}
static inline std::string valueToString(const Value &v, int line = 0, int col = 0) {
	switch (v.type) {
	case ValueType::BOOL:
		return v.asBool() ? "true" : "false";
	case ValueType::NONE:
		return "None";
	case ValueType::NOTYPE:
		return "Notype";
	case ValueType::FLOAT:
		return formatNumber(v.asFloat());
	case ValueType::INT:
		return std::to_string(v.asInt());
	case ValueType::STRING:
		return v.asString();
	case ValueType::FUNCTION:
		return "<function>";
	case ValueType::NATIVE_FUNCTION:
		return "<native_function>";
	case ValueType::CLASS:
		return "<class>";
	case ValueType::INSTANCE:
		return "<class-instance>";
	case ValueType::ERROR: {
		auto *err = static_cast<ErrorObject *>(v.ref.get());
		return "<Error: " + err->errType + ": " + err->message + ">";
	}
	case ValueType::SET: {
		auto *s = static_cast<SetObject *>(v.ref.get());
		string str = "{";
		size_t i = 0;
		for (const auto &key : s->elements) {
			str += valueToString(key, line, col);
			if (i + 1 < s->elements.size())
				str += ", ";
			i++;
		}
		str += "}";
		return str;
	}
	case ValueType::LIST: {
		auto *s = static_cast<ListObject *>(v.ref.get());
		string str = "[";
		for (size_t i = 0; i < s->elements.size(); i++) {
			str += valueToString(s->elements[i], line, col);
			if (i + 1 < s->elements.size())
				str += ", ";
		}
		str += "]";
		return str;
	}
	case ValueType::RANGE: {
		auto *r = static_cast<RangeObject *>(v.ref.get());
		if (!r->isValid)
			return "(...)";
		string s = "";
		auto fmt = [](double d, bool isF) {
			return isF ? std::to_string(d) : std::to_string((long long)d);
		};
		if (r->start > r->end) {
			s += (r->endInclusive ? "[" : "(") + fmt(r->end, r->isFloat);
			s += ", " + fmt(r->start, r->isFloat) +
				  (r->startInclusive ? "]" : ")");
		} else {
			s += (r->startInclusive ? "[" : "(") + fmt(r->start, r->isFloat);
			s += ", " + fmt(r->end, r->isFloat) + (r->endInclusive ? "]" : ")");
		}
		if (r->step != 1)
			s += ".." + fmt(r->step, r->isFloat);
		return s;
	}
	case ValueType::TUPLE: {
		auto *list = static_cast<TupleObject *>(v.ref.get());
		if (list->elements.empty())
			return "(,)";
		string str = "(";
		for (size_t i = 0; i < list->elements.size(); i++) {
			str += valueToString(list->elements[i], line, col);
			if (i + 1 < list->elements.size())
				str += ", ";
			else if (list->elements.size() == 1)
				str += ",";
		}
		str += ")";
		return str;
	}
	case ValueType::DICT: {
		auto *d = static_cast<DictObject *>(v.ref.get());
		string s = "{";
		size_t count = 0;
		for (const auto &[key, val] : d->items) {
			s += valueToString(key, line, col) + ": " +
				  valueToString(val, line, col);
			if (count++ < d->items.size() - 1)
				s += ", ";
		}
		s += "}";
		return s;
	}
	case ValueType::VECTOR: {
		auto *vec = static_cast<VectorObject *>(v.ref.get());
		string s = "<";
		for (size_t i = 0; i < vec->elements.size(); i++) {
			s += valueToString(vec->elements[i]);
			if (i < vec->elements.size() - 1)
				s += ", ";
		}
		s += ">";
		return s;
	}
	case ValueType::BIGINT: {
		auto *big = static_cast<BigIntObject *>(v.ref.get());
		return bigIntToString(big);
	}
	case ValueType::REFERENCE: {
		return valueToString(*v.ptr);
	}
	case ValueType::SUPER: {
		auto *super = static_cast<SuperObject *>(v.ref.get());
		return "<super" + (super->name.empty() ? "" : super->name) + ">";
	}
	case ValueType::PAIRED: {
		auto *pair = static_cast<PairedObject *>(v.ref.get());
		return "<Paired Object>";
	}
	case ValueType::OMIT_MARKER: {
		return "Omit_marker";
	}
	case ValueType::FILE: {
		auto *file = static_cast<FileObject *>(v.ref.get());
		return "<File: " + file->name + "at path: " + file->path;
	}
	default:
		throw TypeError("Cannot implicitly convert this type to string " +
								 to_string((int)v.type),
			line, col);
	}
}
static inline std::string PrintProperty(const Value &v) {
	if (v.isConst && v.isLocked)
		return "Locked Const ";
	else if (v.isConst)
		return "Const ";
	else if (v.isLocked)
		return "Locked ";
	else
		return "";
	return "failure";
}
static inline std::string PrintStackForDebug(const std::vector<Value> &stack) {
	std::string result = "start -> [";
	for (auto val = stack.begin(); val < stack.end(); val++) {
		result += PrintProperty(*val);
		switch (val->type) {
		case ValueType::NOTYPE:
			result += "NoType";
			break;
		case ValueType::NONE:
			result += "None";
			break;
		case ValueType::INT:
			result += "Int";
			break;
		case ValueType::FLOAT:
			result += "Float";
			break;
		case ValueType::STRING:
			result += "String";
			break;
		case ValueType::BOOL:
			result += "Bool";
			break;
		case ValueType::LIST:
			result += "List";
			break;
		case ValueType::VECTOR:
			result += "Vector";
			break;
		case ValueType::DICT:
			result += "Dict";
			break;
		case ValueType::SLICE:
			result += "Slice";
			break;
		case ValueType::BIGINT:
			result += "BigInt";
			break;
		case ValueType::PAIRED:
			result += "Paired";
			break;
		case ValueType::RANGE:
			result += "Range";
			break;
		case ValueType::TUPLE:
			result += "Tuple";
			break;
		case ValueType::SET:
			result += "Set";
			break;
		case ValueType::FUNCTION:
			result += "Func";
			break;
		case ValueType::NATIVE_FUNCTION:
			result += "NFunc";
			break;
		case ValueType::FILE:
			result += "File";
			break;
		case ValueType::OVERLOAD:
			result += "Overload";
			break;
		case ValueType::OMIT_MARKER:
			result += "OmitMarker";
			break;
		case ValueType::REFERENCE:
			result += "Refrance";
			break;
		case ValueType::ERROR:
			result += "Error";
			break;
		case ValueType::CLASS:
			result += "Class";
			break;
		case ValueType::INSTANCE:
			result += "Instance";
			break;
		case ValueType::SUPER:
			result += "Super";
			break;
		default:
			result += "Unknown";
			break;
		}
		if (DEBUGGER_MODE_IS_ENABLED) {
			result += " ";
#ifdef VM_DEBUG_MODE
			result += val->__DEBUGGING__NAME__;
#endif
			result += (" " + valueToString(*val));
		}
		if (val != stack.end() - 1)
			result += ", ";
	}
	return result + "] <- end";
}
static inline bool lessValue(const Value &a, const Value &b, std::shared_ptr<Env> globals, std::function<Value(MethodCallExpr *)> methodResolver, std::function<void(std::string, std::vector<std::string>)> importResolver);
struct CacheKeyCmp {
	bool operator()(const vector<Value> &a, const vector<Value> &b, std::shared_ptr<Env> globals, std::function<Value(MethodCallExpr *)> methodResolver, std::function<void(std::string, std::vector<std::string>)> importResolver) const {
		return std::lexicographical_compare(
			a.begin(), a.end(), b.begin(), b.end(),
			[&](const Value &x, const Value &y) { return lessValue(x, y, globals, methodResolver, importResolver); });
	}
};
struct FuncVal {
	FuncStmt *stmt;
	std::map<vector<Value>, Value, CacheKeyCmp> cache;
	FuncVal(FuncStmt *s) : stmt(s) {}
};
struct Chunk;
struct FunctionObject : HeapObject {
	vector<ParamSpec> params;
	Chunk *chunk;
	ValueType returnType;
	vector<Expr *> defaultRetArgs;
	bool returnsConst;
	vector<Stmt *> body;
	std::shared_ptr<Env> closure;
	bool isCached;
	ClassObject *owner = nullptr;
	std::unordered_map<vector<Value>, Value, VectorHash, VectorEqual> cache;
	FunctionObject(const vector<ParamSpec> &p, ValueType rt, vector<Expr *> dra,
		bool rc, const vector<Stmt *> &b, std::shared_ptr<Env> c,
		bool cached, Chunk *ch = nullptr)
		 : HeapObject(ValueType::FUNCTION),
			params(p),
			returnType(rt),
			defaultRetArgs(dra),
			returnsConst(rc),
			body(b),
			closure(c),
			isCached(cached),
			chunk(ch) {}
	~FunctionObject() {
		if (chunk)
			delete chunk;
	}
};
static void setAdd(std::unordered_set<Value, ValueHash, ValueEqual> &elems, const Value &v) {
	Value finalVal = deepCopy(v);
	finalVal.isConst = true;
	elems.insert(finalVal);
}
void enableColors() {
#ifdef _WIN32
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	if (GetConsoleMode(hOut, &dwMode)) {
		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(hOut, dwMode);
	}
#endif // _WIN32
}
void printValue(const Value &v, std::unordered_set<const HeapObject *> &seen, bool quoteStrings, Inspector *inspect = nullptr) {
	bool isContainer =
		(v.type == ValueType::LIST || v.type == ValueType::SET ||
			v.type == ValueType::VECTOR || v.type == ValueType::DICT);
	if (isContainer && v.ref && seen.count(v.ref.get())) {
		std::cout << "<self>";
		return;
	}
	if (v.type == ValueType::INSTANCE && inspect) {
		std::string custom = (*inspect)(v);
		if (!custom.empty()) {
			std::cout << custom;
			return;
		}
	}
	if (isContainer && v.ref)
		seen.insert(v.ref.get());
	switch (v.type) {
	case ValueType::INT:
		std::cout << v.asInt();
		break;
	case ValueType::FLOAT:
		std::cout << v.asFloat();
		break;
	case ValueType::BOOL:
		std::cout << (v.asBool() ? "true" : "false");
		break;
	case ValueType::NONE:
		std::cout << "None";
		break;
	case ValueType::NOTYPE:
		std::cout << "NoType";
		break;
	case ValueType::CLASS:
		std::cout << "class: ";
#ifdef VM_DEBUG_MODE
		std::cout << v.__DEBUGGING__NAME__;
#endif
		break;
	case ValueType::STRING:
		if (quoteStrings)
			std::cout << "\"" << v.asString() << "\"";
		else
			std::cout << v.asString();
		break;
	case ValueType::LIST: {
		auto *list = static_cast<ListObject *>(v.ref.get());
		std::cout << "[";
		for (size_t i = 0; i < list->elements.size(); i++) {
			printValue(list->elements[i], seen, true, inspect);
			if (i + 1 < list->elements.size())
				std::cout << ", ";
		}
		std::cout << "]";
		break;
	}
	case ValueType::RANGE: {
		auto *r = static_cast<RangeObject *>(v.ref.get());
		if (!r->isValid) {
			std::cout << "(...)";
			break;
		}
		std::cout << (r->startInclusive ? "[" : "(")
					 << (r->isFloat ? std::to_string(r->start)
										 : std::to_string((long long)r->start))
					 << ".."
					 << (r->isFloat ? std::to_string(r->end)
										 : std::to_string((long long)r->end));
		if (r->step != 1)
			std::cout << ".."
						 << (r->isFloat ? std::to_string(r->step)
											 : std::to_string((long long)r->step));
		std::cout << (r->endInclusive ? "]" : ")");
		break;
	}
	case ValueType::SET: {
		auto *s = static_cast<SetObject *>(v.ref.get());
		if (s->elements.empty()) {
			std::cout << "{,}";
			break;
		}
		std::cout << "{";
		size_t i = 0;
		for (const auto &key : s->elements) {
			printValue(key, seen, true, inspect);
			if (i + 1 < s->elements.size())
				std::cout << ", ";
			i++;
		}
		std::cout << "}";
		break;
	}
	case ValueType::TUPLE: {
		auto *t = static_cast<TupleObject *>(v.ref.get());
		if (t->elements.empty()) {
			std::cout << "(,)";
			break;
		}
		std::cout << "(";
		for (size_t i = 0; i < t->elements.size(); i++) {
			printValue(t->elements[i], seen, true, inspect);
			if (i + 1 < t->elements.size())
				std::cout << ", ";
			else if (t->elements.size() == 1)
				std::cout << ",";
		}
		std::cout << ")";
		break;
	}
	case ValueType::DICT: {
		auto *dict = static_cast<DictObject *>(v.ref.get());
		if (dict->items.empty()) {
			std::cout << "{:}";
			break;
		}
		std::cout << "{";
		size_t i = 0;
		for (const auto &[key, val] : dict->items) {
			printValue(key, seen, true, inspect);
			std::cout << " : ";
			printValue(val, seen, true, inspect);
			if (i + 1 < dict->items.size())
				std::cout << ", ";
			i++;
		}
		std::cout << "}";
		break;
	}
	case ValueType::PAIRED: {
		auto *pair = static_cast<PairedObject *>(v.ref.get());
		std::cout << "<Paired Object at " << &pair << ">";
		break;
	}
	case ValueType::FILE: {
		auto *f = static_cast<FileObject *>(v.ref.get());
		std::cout << "<File at path: '" << f->path << "' >";
		break;
	}
	case ValueType::VECTOR: {
		auto *vec = static_cast<VectorObject *>(v.ref.get());
		std::cout << valueToString(v);
		break;
	}
	case ValueType::BIGINT: {
		auto *big = static_cast<BigIntObject *>(v.ref.get());
		std::cout << bigIntToString(big);
		break;
	}
	case ValueType::REFERENCE: {
		std::cout << valueToString(*v.ptr);
		break;
	}

	default:
		std::cout << "<"
					 << (v.type == ValueType::INSTANCE
								 ? static_cast<InstanceObject *>(v.ref.get())->name
								 : "")
					 << "Object at " << v.ref << ">";
		break;
	}
}
void printValue(const Value &v) {
	std::unordered_set<const HeapObject *> seen;
	printValue(v, seen, false);
}
struct ValueExpr : public Expr {
	Value val;
	Value *sourcePtr = nullptr;
	ValueExpr(Value v, Value *src = nullptr)
		 : Expr(ExprType::NUMBER), val(v), sourcePtr(src) {}
};
enum class Magic_Methods : uint8_t {

	/*
	================================================================
	1. LIFECYCLE & IDENTITY
	Distinct from Python's "init/del", using Constructor terminology
	================================================================
	*/
	__construct__, // (was __init__)   Called on creation
	__destruct__,	// (was __del__)    Called on cleanup
	__copy__,		// (was __scopy__)  Shallow copy
	__clone__,		// (was __dcopy__)  Deep copy
	__ref__,			// (was __rcopy__)  Reference copy
	__mro__,			// Method Resolution Order

	/*
	================================================================
			  2. REPRESENTATION & DEBUGGING
			  Focus on "Displaying" vs "Inspecting"
	================================================================
	*/
	__display__, // (was __show__)   User-friendly string representation
	__inspect__, // (was __debug__)  Programmer-focused raw representation

	/*
	================================================================
			  3. ARITHMETIC (VERBS OVER ABBREVIATIONS)
			  Using full words makes it feel less like C macros
	================================================================
	*/
	// Standard           // Reverse (Right-hand side)
	__plus__,
	__r_plus__, // (was __add__)
	__minus__,
	__r_minus__, // (was __sub__)
	__times__,
	__r_times__, // (was __mul__)
	__divide__,
	__r_divide__, // (was __div__)
	__int_divide__,
	__r_int_divide__, // (was __div__)
	__power__,
	__r_power__, // (was __pow__)
	__modulo__,
	__r_modulo__, // (was __mod__ - added this, usually needed)

	/*
	================================================================
			  3.5 inplace assignment for arithmetic operators
			  Using full words makes it feel less like C macros
	================================================================
	*/
	__plus_eq__,		 // (was __iadd__)    obj1 += obj2
	__minus_eq__,		 // (was __isub__)    obj1 -= obj2
	__times_eq__,		 // (was __imul__)    obj1 *= obj2
	__divide_eq__,		 // (was __idiv__)    obj1 /= obj2
	__int_divide_eq__, // (was __idiv__)    obj1 //= obj2
	__power_eq__,		 // (was __ipow__)    obj1 **= obj
	__modulo_eq__,		 // (was __imod__)    obj1 %= obj2

	/*
	================================================================
			  4. UNARY & STATE MODIFIERS
	================================================================
	*/
	__positive__,	// (was __pos__)    +x
	__negative__,	// (was __neg__)    -x
	__increment__, // (was __inc__)    x++
	__decrement__, // (was __dec__)    x--
	__invert__,		// (was __not__)    ~x (Bitwise not)

	/*
	================================================================
			  5. LOGIC
			  Explicit naming to Boolean logic
	================================================================
	*/
	// Standard            // Reverse
	__and__,
	__r_and__, // (was __and__)
	__or__,
	__r_or__, // (was __or__)
	__xor__,
	__r_xor__, // (was __xor__)
	__nand__,
	__r_nand__, // (was __nand__)
	__nor__,
	__r_nor__, // (was __nor__)
	__nxor__,
	__r_nxor__, // (was __nxor__)

	/*
	================================================================
			  5.5 inplace assignment for bitwise operators
			  Explicit naming to separate Boolean logic from Bitwise logic
	================================================================
	*/
	__and_equals__, // (was __iand__)    obj1 &= obj2
	__or_equals__,	 // (was __ior__)     obj1 |= obj2
	__xor_equals__, // (was __ixor__)    obj1 ^= obj2

	/*
	================================================================
			  6. COMPARISON (THE JUDGES)
			  Using "is" makes the intent readable
	================================================================
	*/
	__equals__,		 // (was __eq__)     ==
	__differs__,	 // (was __neq__)    !=
	__identical__,	 // (was __seq__)    === (Strict equality)
	__distinct__,	 // (was __sneq__)   !== (Strict inequality)
	__less__,		 // (was __lt__)     <
	__less_eq__,	 // (was __lte__)    <=
	__greater__,	 // (was __gt__)     >
	__greater_eq__, // (was __gte__)    >=

	/*
	================================================================
			  7. CONTAINER & ACCESS
			  Moving away from "getitem/setitem" to "at/put" style
	================================================================
	*/
	__count__, // (was __len__)    Size of container
	__at__,	  // (was __get__)    val = obj[key]
	__put__,	  // (was __set__)    obj[key] = val
	__call__,
	__has__, // (was __contains__) if x is in obj
	__lacks__,
	__missing__,
	__not__missing__, // (was __getattr__) if x is not in obj
	__assign__,			// (was __setattr__) Called when setting a property

	/*
	================================================================
			  8. ITERATION (FLOW)
	================================================================
	*/
	__traverse__, // (was __iter__)   Returns the iterator object
	__advance__,  // (was __next__)   Moves to next item

	/*
	================================================================
			  9. CASTING (TRANSFORMATION)
			  "to" prefix implies conversion
	================================================================
	*/
	__to_int__,		// (was __int__)
	__to_float__,	// (was __float__)
	__to_string__, // (was __string__)
	__to_bool__,	// (was __bool__)
	__to_list__,	// (was __list__)
	__to_set__,		// (was __set__)
	__to_dict__,	// (was __dict__)
	__to_tuple__,	// (was __tuple__)
	__to_vector__, // (was __vector__)
	__to_range__,	// (was __range__)

	/*
	================================================================
			  10. INFO
	================================================================
	*/
	__var_count__,
	__var_names__,
	__var_values__,
	__var_pairs__,
	__var_reverse_pairs__,
	__function_count__,
	__function_names__,
	__all_count__,
	__all_names__,
};
static inline std::string magic_methods_to_string(Magic_Methods Magic_method) {
	switch (Magic_method) {
	case Magic_Methods::__construct__: // creates the object and returns the
												  // instance
		return "__construct__";
	case Magic_Methods::__destruct__: // deletes the instance and returns
												 // None
		return "__destruct__";
	case Magic_Methods::__copy__: // shallowley copies the instance
		return "__copy__";
	case Magic_Methods::__clone__: // deeply copies the instance
		return "__clone__";
	case Magic_Methods::__ref__: // creates an alias to the instance
		return "__ref__";
	case Magic_Methods::__mro__: // returns a list of meethod resolution
										  // order
		return "__mro__";
	case Magic_Methods::__display__: // called by the print() function
		return "__display__";
	case Magic_Methods::__inspect__: // used for debugging and printing
												// nested objects
		return "__inspect__";
	case Magic_Methods::__plus__: // + operator overload for normal addition
											// obj1 + obj2
		return "__plus__";
	case Magic_Methods::__r_plus__: // + opertaor overload for reverse
											  // addition obj2 + obj1
		return "__r_plus__";
	case Magic_Methods::__minus__: // - operator overload for normal
											 // subtraction obj1 - obj2
		return "__minus__";
	case Magic_Methods::__r_minus__: // - operator overload for reverse
												// subtraction obj2 - obj1
		return "__r_minus__";
	case Magic_Methods::__times__: // * operator overload for normal
											 // multiplication obj1 * obj2
		return "__times__";
	case Magic_Methods::__r_times__: // * operator overload for reverse
												// multiplication obj2 * obj1
		return "__r_times__";
	case Magic_Methods::__divide__: // / operator overload for normal
											  // division obj1 / obj2
		return "__divide__";
	case Magic_Methods::__r_divide__: // / operator overload for reverse
												 // division obj2 / obj1
		return "__r_divide__";
	case Magic_Methods::__int_divide__: // // operator overload for integer
													// division obj1 // obj2
		return "__int_divide__";
	case Magic_Methods::__r_int_divide__: // // operator overload for reverse
													  // integre division obj2 // obj1
		return "__r_int_divide__";
	case Magic_Methods::__power__: // ** operator overload for normal
											 // exponentiation obj1 ** obj2
		return "__power__";
	case Magic_Methods::__r_power__: // ** operator overload for reverse
												// exponentiation obj2 ** obj1
		return "__r_power__";
	case Magic_Methods::__modulo__: // % operator overlaod for normal
											  // modulation obj1 % obj2
		return "__modulo__";
	case Magic_Methods::__r_modulo__: // % operator overload for reverse
												 // modulation obj2 % obj1
		return "__r_modulo__";
	case Magic_Methods::__plus_eq__: // += operator overload for addition
												// assignment obj1 += obj2
		return "__plus_eq__";
	case Magic_Methods::__minus_eq__: // -= operator overload for subtraction
												 // assignment obj1 -= obj2
		return "__minus_eq__";
	case Magic_Methods::__times_eq__: // *= operator overload for
												 // multiplication assignment obj1 *=
												 // obj2
		return "__times_eq__";
	case Magic_Methods::__divide_eq__: // /= operator overload for division
												  // assignment obj1 /= obj2
		return "__divide_eq__";
	case Magic_Methods::__int_divide_eq__: // //= operator overload for
														// integer division assignment
														// obj1 //= obj2
		return "__int_divide_eq__";
	case Magic_Methods::__power_eq__: // **= operator overload for
												 // exponentiation assignment obj1 **=
												 // obj2
		return "__power_eq__";
	case Magic_Methods::__modulo_eq__: // %= operator overload for modulation
												  // assignment obj1 %= obj2
		return "__modulo_eq__";
	case Magic_Methods::__positive__: // + operator overload for unary
												 // positive +obj
		return "__positive__";
	case Magic_Methods::__negative__: // - operator overload for unary
												 // negative -obj
		return "__negative__";
	case Magic_Methods::__increment__: // ++ operator overload for unary
												  // increment obj++
		return "__increment__";
	case Magic_Methods::__decrement__: // -- operator overload for unary
												  // decrement obj--
		return "__decrement__";
	case Magic_Methods::__invert__: // ~ operator overload for unary bitwise
											  // not ~obj
		return "__invert__";
	case Magic_Methods::__and__: // and operator overload for normal
										  // bitwise and obj1 and obj2
		return "__bit_and__";
	case Magic_Methods::__r_and__: // and operator overload for reverse
											 // bitwise and obj2 and obj1
		return "__r_bit_and__";
	case Magic_Methods::__or__: // or operator overload for normal
										 // bitwise or obj1 or obj2
		return "__bit_or__";
	case Magic_Methods::__r_or__: // or operator overload for reverse
											// bitwise or obj2 or obj1
		return "__r_bit_or__";
	case Magic_Methods::__xor__: // xor operator overload for normal
										  // bitwise xor obj1 xor obj2
		return "__bit_xor__";
	case Magic_Methods::__r_xor__: // xor operator overload for reverse
											 // bitwise xor obj2 xor obj1
		return "__r_bit_xor__";
	case Magic_Methods::__nand__: // nand operator overload for normal
											// bitwise nand obj1 nand obj2
		return "__bit_nand__";
	case Magic_Methods::__r_nand__: // nand operator overload for reverse
											  // bitwise nand obj2 nand obj1
		return "__r_bit_nand__";
	case Magic_Methods::__nor__: // nor operator overload for normal
										  // bitwise nor obj1 nor obj2
		return "__bit_nor__";
	case Magic_Methods::__r_nor__: // nor operator overload for reverse
											 // bitwise nor obj2 nor obj1
		return "__r_bit_nor__";
	case Magic_Methods::__nxor__: // nxor operator overload for normal
											// bitwise nxor obj1 nxor obj2
		return "__bit_nxor__";
	case Magic_Methods::__r_nxor__: // nxor operator overload for reverse
											  // bitwise nxor obj2 nxor obj1
		return "__r_bit_nxor__";
	case Magic_Methods::__and_equals__: // &= operator overload for bitwise
													// and assignment obj1 &= obj2
		return "__and_equals__";
	case Magic_Methods::__or_equals__: // |= operator overload for bitwise or
												  // assignment obj1 |= obj2
		return "__or_equals__";
	case Magic_Methods::__xor_equals__: // ^= operator overload for bitwise
													// xor assignment obj1 ^= obj2
		return "__xor_equals__";
	case Magic_Methods::__equals__: // == operator overload for equality obj1
											  // == obj2
		return "__equals__";
	case Magic_Methods::__differs__: // != operator overload for inequality
												// obj1 != obj2
		return "__differs__";
	case Magic_Methods::__identical__: // === operator overload for strict
												  // equality obj1 === obj2
		return "__identical__";
	case Magic_Methods::__distinct__: // !== operator overload for strict
												 // inequality obj1 !== obj2
		return "__distinct__";
	case Magic_Methods::__less__: // < operator overload for less than obj1 <
											// obj2
		return "__less__";
	case Magic_Methods::__less_eq__: // <= operator overload for less than or
												// equal to obj1 <= obj2
		return "__less_eq__";
	case Magic_Methods::__greater__: // > operator overload for greater than
												// obj1 > obj2
		return "__greater__";
	case Magic_Methods::__greater_eq__: // >= operator overload for greater
													// than or equal to obj1 >= obj2
		return "__greater_eq__";
	case Magic_Methods::__count__: // length() operator overload for counting
											 // items in a container length(obj)
		return "__count__";
	case Magic_Methods::__at__: // [] operator overload for getting an item
										 // from a container obj[key]
		return "__at__";
	case Magic_Methods::__put__: // [] operator overload for setting an item
										  // in a container obj[key] = val
		return "__put__";
	case Magic_Methods::__has__: // is in operator overload for checking if a
										  // container has an item if x is in obj
		return "__has__";
	case Magic_Methods::__missing__: // is not in operator overload for
												// checking if a container is missing an
												// item if x is not in obj
		return "__missing__";
	case Magic_Methods::__assign__: // . operator overload for setting an
											  // attribute obj.attr = val
		return "__assign__";
	case Magic_Methods::__traverse__: // iter() operator overload for getting
												 // an iterator from a container
												 // iter(obj)
		return "__traverse__";
	case Magic_Methods::__advance__: // next() operator overload for
												// advancing an iterator to the next
												// item next(obj)
		return "__advance__";
	case Magic_Methods::__to_int__: // int() operator overload for converting
											  // an object to an integer int(obj)
		return "__to_int__";
	case Magic_Methods::__to_string__: // string() operator overload for
												  // converting an object to a string
												  // string(obj)
		return "__to_string__";
	case Magic_Methods::__to_bool__: // bool() operator overload for
												// converting an object to a boolean
												// bool(obj)
		return "__to_bool__";
	case Magic_Methods::__to_list__: // list() operator overload for
												// converting an object to a list
												// list(obj)
		return "__to_list__";
	case Magic_Methods::__to_set__: // set() operator overload for converting
											  // an object to a set set(obj)
		return "__to_set__";
	case Magic_Methods::__to_dict__: // dict() operator overload for
												// converting an object to a dictionary
												// dict(obj)
		return "__to_dict__";
	case Magic_Methods::__to_tuple__: // tuple() operator overload for
												 // converting an object to a tuple
												 // tuple(obj)
		return "__to_tuple__";
	case Magic_Methods::__to_vector__: // vector() operator overload for
												  // converting an object to a vector
												  // vector(obj)
		return "__to_vector__";
	case Magic_Methods::__var_count__: // returns the number of variables in
												  // the class
		return "__var_count__";
	case Magic_Methods::__var_names__: // returns a list of variable names in
												  // the class
		return "__var_names__";
	case Magic_Methods::__function_count__: // returns the number of
														 // functions in the class
		return "__function_count__";
	case Magic_Methods::__function_names__: // returns a list of function
														 // names in the class
		return "__function_names__";
	case Magic_Methods::__all_count__: // returns the total number of
												  // variables and functions in the
												  // class
		return "__all_count__";
	case Magic_Methods::__all_names__: // returns a list of all variable and
												  // function names in the class
		return "__all_names__";
	default: // Should never happen, but just in case
		return "_";
	}
}
static inline Color ValueToColor(const Value &v, int l, int c) {
	if (v.type != ValueType::INSTANCE)
		throw TypeError("Expected Color object", l, c);
	auto *inst = static_cast<InstanceObject *>(v.ref.get());
	if (inst->klass->name != "Color")
		throw TypeError("Expected Color object", l, c);
	return Color{(unsigned char)inst->fields["r"].asInt(),
		(unsigned char)inst->fields["g"].asInt(),
		(unsigned char)inst->fields["b"].asInt(),
		(unsigned char)inst->fields["a"].asInt()};
}
static inline Rectangle ValueToRect(const Value &v, int l, int c) {
	if (v.type != ValueType::INSTANCE)
		throw TypeError("Expected Rectangle object", l, c);
	auto *inst = static_cast<InstanceObject *>(v.ref.get());
	if (inst->klass->name != "Rectangle")
		throw TypeError("Expected Rectangle object", l, c);
	return Rectangle{(float)inst->fields["x"].asInt(),
		(float)inst->fields["y"].asInt(),
		(float)inst->fields["width"].asInt(),
		(float)inst->fields["height"].asInt()};
}
static inline Vector2 ValueToVector2(const Value &v, int l, int c) {
	if (v.type != ValueType::VECTOR)
		throw TypeError("Expected Vector object", l, c);
	auto *vec = static_cast<VectorObject *>(v.ref.get());
	if (vec->elements.size() < 2)
		throw ValueError("Vector must have at least 2 elements for Vector2", l,
			c);
	return Vector2{(float)vec->elements[0].asFloat(),
		(float)vec->elements[1].asFloat()};
}
static inline auto Vector2ToValue = [](Vector2 v) -> Value {
	std::vector<Value> elems;
	elems.reserve(2);
	elems.push_back(Value::Float(v.x));
	elems.push_back(Value::Float(v.y));
	return Value::Vector(elems);
};
static inline std::vector<Vector2> ValueToVectorList(const Value &v, int l, int c) {
	if (v.type != ValueType::LIST)
		throw TypeError("Expected List of Vectors", l, c);
	auto *list = static_cast<ListObject *>(v.ref.get());
	std::vector<Vector2> points;
	points.reserve(list->elements.size());
	for (const auto &el : list->elements)
		points.push_back(ValueToVector2(el, l, c));
	return points;
}
enum class OpCode : uint8_t {
	// Literals & Constants
	OP_CONSTANT,
	OP_CONSTANT_LONG,
	OP_TRUE,
	OP_FALSE,
	OP_NONE,
	OP_NOTYPE,
	// Variables & Scope
	OP_DEFINE_VAR,
	OP_GET_VAR,
	OP_SET_VAR,
	OP_DEEP_COPY,
	OP_REF_LOCAL,
	OP_DEFINE_REF,
	OP_REF_VAR,
	OP_REF_INDEX,
	OP_SET_REF,
	OP_SHALLOW_COPY,
	OP_MULTI_SET,
	OP_GET_LOCAL,
	OP_SET_LOCAL,
	OP_INC_LOCAL,
	OP_SET_FLAGS,
	OP_REF_PROPERTY,
	OP_DELETE,
	// Arithmetic & Logic
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_FLOOR_DIV,
	OP_MOD,
	OP_POW,
	OP_IADD,
	OP_ISUB,
	OP_IMUL,
	OP_IDIV,
	OP_IFLOOR_DIV,
	OP_IMOD,
	OP_IPOW,
	OP_DUP,
	OP_DUP_2,
	OP_EQ,
	OP_NEQ,
	OP_LT,
	OP_GT,
	OP_LTE,
	OP_GTE,
	OP_COLON,
	OP_STRICT_NEQ,
	OP_NOT,
	OP_AND,
	OP_OR,
	OP_XOR,
	OP_IS,
	OP_IN,
	OP_IS_NOT,
	OP_STRICT_EQ,
	OP_IS_IN,
	OP_IS_NOT_IN,
	OP_NXOR,
	OP_NAND,
	OP_NOR,
	OP_NEGATE,
	OP_INCREMENT,
	OP_DECREMENT,
	// Containers
	OP_BUILD_LIST,
	OP_BUILD_TUPLE,
	OP_BUILD_SET,
	OP_BUILD_DICT,
	OP_UNPACK_DICT,
	OP_BUILD_RANGE,
	OP_BUILD_VECTOR,
	OP_BUILD_FSTRING,
	OP_BUILD_FILE,
	OP_BUILD_SLICE,
	// OOP
	OP_CLASS,
	OP_METHOD,
	OP_GET_PROPERTY,
	OP_SET_PROPERTY,
	OP_CLASS_FIELD,
	OP_SUPER,
	// Comprehension
	OP_LIST_APPEND,
	OP_SET_ADD,
	OP_DICT_SET,
	OP_LIST_TO_TUPLE,
	OP_LIST_TO_VECTOR,
	// Access & Calls
	OP_GET_INDEX,
	OP_SET_INDEX,
	OP_INVOKE,
	OP_CALL,
	// Control Flow
	OP_JUMP,
	OP_JUMP_IF_FALSE,
	OP_LOOP,
	OP_RETURN,
	OP_TO_STREAM,
	OP_JUMP_IF_NOT_LT,
	OP_BREAK,
	OP_CONTINUE,
	OP_SKIP,
	OP_OMIT,
	OP_FOR_ITER,
	OP_SKIP_ITER,
	OP_SWITCH_TABLE,
	// Errors & Systems
	OP_THROW,
	OP_ASSERT,
	OP_IMPORT,
	OP_POP,
	OP_DEBUG_NAME,
	OP_TRY_ENTER,
	OP_TRY_EXIT,
	OP_CATCH,
	OP_RETHROW,
	OP_END_FINALLY
};
static inline std::string OpCodeToString(OpCode num) {
	switch (num) {
	case OpCode::OP_CONSTANT:
		return "OP_CONSTANT";
	case OpCode::OP_CONSTANT_LONG:
		return "OP_CONSTANT_LONG";
	case OpCode::OP_TRUE:
		return "OP_TRUE";
	case OpCode::OP_FALSE:
		return "OP_FALSE";
	case OpCode::OP_NONE:
		return "OP_NONE";
	case OpCode::OP_NOTYPE:
		return "OP_NOTYPE";
	case OpCode::OP_DEFINE_VAR:
		return "OP_DEFINE_VAR";
	case OpCode::OP_GET_VAR:
		return "OP_GET_VAR";
	case OpCode::OP_SET_VAR:
		return "OP_SET_VAR";
	case OpCode::OP_DEEP_COPY:
		return "OP_DEEP_COPY";
	case OpCode::OP_REF_LOCAL:
		return "OP_REF_LOCAL";
	case OpCode::OP_DEFINE_REF:
		return "OP_DEFINE_REF";
	case OpCode::OP_REF_VAR:
		return "OP_REF_VAR";
	case OpCode::OP_REF_INDEX:
		return "OP_REF_INDEX";
	case OpCode::OP_SET_REF:
		return "OP_SET_REF";
	case OpCode::OP_SHALLOW_COPY:
		return "OP_SHALLOW_COPY";
	case OpCode::OP_MULTI_SET:
		return "OP_MULTI_LET";
	case OpCode::OP_GET_LOCAL:
		return "OP_GET_LOCAL";
	case OpCode::OP_SET_LOCAL:
		return "OP_SET_LOCAL";
	case OpCode::OP_INC_LOCAL:
		return "OP_INC_LOCAL";
	case OpCode::OP_SET_FLAGS:
		return "OP_SET_FLAGS";
	case OpCode::OP_REF_PROPERTY:
		return "OP_REF_PROPERTY";
	case OpCode::OP_DELETE:
		return "OP_DELETE";
	case OpCode::OP_ADD:
		return "OP_ADD";
	case OpCode::OP_SUB:
		return "OP_SUB";
	case OpCode::OP_MUL:
		return "OP_MUL";
	case OpCode::OP_DIV:
		return "OP_DIV";
	case OpCode::OP_FLOOR_DIV:
		return "OP_FLOOR_DIV";
	case OpCode::OP_MOD:
		return "OP_MOD";
	case OpCode::OP_POW:
		return "OP_POW";
	case OpCode::OP_IADD:
		return "OP_IADD";
	case OpCode::OP_ISUB:
		return "OP_ISUB";
	case OpCode::OP_IMUL:
		return "OP_IMUL";
	case OpCode::OP_IDIV:
		return "OP_IDIV";
	case OpCode::OP_IFLOOR_DIV:
		return "OP_IFLOOR_DIV";
	case OpCode::OP_IMOD:
		return "OP_IMOD";
	case OpCode::OP_IPOW:
		return "OP_IPOW";
	case OpCode::OP_DUP:
		return "OP_DUP";
	case OpCode::OP_DUP_2:
		return "OP_DUP_2";
	case OpCode::OP_EQ:
		return "OP_EQ";
	case OpCode::OP_NEQ:
		return "OP_NEQ";
	case OpCode::OP_LT:
		return "OP_LT";
	case OpCode::OP_GT:
		return "OP_GT";
	case OpCode::OP_LTE:
		return "OP_LTE";
	case OpCode::OP_GTE:
		return "OP_GTE";
	case OpCode::OP_COLON:
		return "OP_COLON";
	case OpCode::OP_STRICT_NEQ:
		return "OP_STRICT_NEQ";
	case OpCode::OP_NOT:
		return "OP_NOT";
	case OpCode::OP_AND:
		return "OP_AND";
	case OpCode::OP_OR:
		return "OP_OR";
	case OpCode::OP_XOR:
		return "OP_XOR";
	case OpCode::OP_IS:
		return "OP_IS";
	case OpCode::OP_IN:
		return "OP_IN";
	case OpCode::OP_IS_NOT:
		return "OP_IS_NOT";
	case OpCode::OP_STRICT_EQ:
		return "OP_STRICT_EQ";
	case OpCode::OP_IS_IN:
		return "OP_IS_IN";
	case OpCode::OP_IS_NOT_IN:
		return "OP_IS_NOT_IN";
	case OpCode::OP_NXOR:
		return "OP_NXOR";
	case OpCode::OP_NAND:
		return "OP_NAND";
	case OpCode::OP_NOR:
		return "OP_NOR";
	case OpCode::OP_NEGATE:
		return "OP_NEGATE";
	case OpCode::OP_INCREMENT:
		return "OP_INCREMENT";
	case OpCode::OP_DECREMENT:
		return "OP_DECREMENT";
	case OpCode::OP_BUILD_LIST:
		return "OP_BUILD_LIST";
	case OpCode::OP_BUILD_TUPLE:
		return "OP_BUILD_TUPLE";
	case OpCode::OP_BUILD_SET:
		return "OP_BUILD_SET";
	case OpCode::OP_BUILD_DICT:
		return "OP_BUILD_DICT";
	case OpCode::OP_UNPACK_DICT:
		return "OP_UNPACK_DICT";
	case OpCode::OP_BUILD_RANGE:
		return "OP_BUILD_RANGE";
	case OpCode::OP_BUILD_VECTOR:
		return "OP_BUILD_VECTOR";
	case OpCode::OP_BUILD_FSTRING:
		return "OP_BUILD_FSTRING";
	case OpCode::OP_BUILD_FILE:
		return "OP_BUILD_FILE";
	case OpCode::OP_BUILD_SLICE:
		return "OP_BUILD_SLICE";
	case OpCode::OP_CLASS:
		return "OP_CLASS";
	case OpCode::OP_METHOD:
		return "OP_METHOD";
	case OpCode::OP_GET_PROPERTY:
		return "OP_GET_PROPERTY";
	case OpCode::OP_SET_PROPERTY:
		return "OP_SET_PROPERTY";
	case OpCode::OP_CLASS_FIELD:
		return "OP_CLASS_FIELD";
	case OpCode::OP_SUPER:
		return "OP_SUPER";
	case OpCode::OP_LIST_APPEND:
		return "OP_LIST_APPEND";
	case OpCode::OP_SET_ADD:
		return "OP_SET_ADD";
	case OpCode::OP_DICT_SET:
		return "OP_DICT_SET";
	case OpCode::OP_LIST_TO_TUPLE:
		return "OP_LIST_TO_TUPLE";
	case OpCode::OP_LIST_TO_VECTOR:
		return "OP_LIST_TO_VECTOR";
	case OpCode::OP_GET_INDEX:
		return "OP_GET_INDEX";
	case OpCode::OP_SET_INDEX:
		return "OP_SET_INDEX";
	case OpCode::OP_INVOKE:
		return "OP_INVOKE";
	case OpCode::OP_CALL:
		return "OP_CALL";
	case OpCode::OP_JUMP:
		return "OP_JUMP";
	case OpCode::OP_JUMP_IF_FALSE:
		return "OP_JUMP_IF_FALSE";
	case OpCode::OP_LOOP:
		return "OP_LOOP";
	case OpCode::OP_RETURN:
		return "OP_RETURN";
	case OpCode::OP_TO_STREAM:
		return "OP_TO_STREAM";
	case OpCode::OP_JUMP_IF_NOT_LT:
		return "OP_JUMP_IF_NOT_LT";
	case OpCode::OP_BREAK:
		return "OP_BREAK";
	case OpCode::OP_CONTINUE:
		return "OP_CONTINUE";
	case OpCode::OP_SKIP:
		return "OP_SKIP";
	case OpCode::OP_OMIT:
		return "OP_OMIT";
	case OpCode::OP_FOR_ITER:
		return "OP_FOR_ITER";
	case OpCode::OP_SKIP_ITER:
		return "OP_SKIP_ITER";
	case OpCode::OP_SWITCH_TABLE:
		return "OP_SWITCH_TABLE";
	case OpCode::OP_THROW:
		return "OP_THROW";
	case OpCode::OP_ASSERT:
		return "OP_ASSERT";
	case OpCode::OP_IMPORT:
		return "OP_IMPORT";
	case OpCode::OP_POP:
		return "OP_POP";
	case OpCode::OP_DEBUG_NAME:
		return "OP_DEBUG_NAME";
	case OpCode::OP_TRY_ENTER:
		return "OP_TRY_ENTER";
	case OpCode::OP_TRY_EXIT:
		return "OP_TRY_EXIT";
	case OpCode::OP_CATCH:
		return "OP_CATCH";
	case OpCode::OP_RETHROW:
		return "OP_RETHROW";
	case OpCode::OP_END_FINALLY:
		return "OP_END_FINALLY";
	default:
		return "UNKNOWN";
	}
}
struct Chunk {
	vector<uint8_t> code;
	vector<Value> constants;
	vector<int> lines;
	vector<int> columns;
	void write(uint8_t byte, int line, int col) {
		code.push_back(byte);
		lines.push_back(line);
		columns.push_back(col);
	}
	void write(OpCode op, int line, int col) {
		write(static_cast<uint8_t>(op), line, col);
	}
	int addConstant(Value v) {
		for (size_t i = 0; i < constants.size(); i++) {
			if (constants[i].strictEquals(v)) {
				return static_cast<int>(i);
			}
		}
		constants.push_back(v);
		return static_cast<int>(constants.size() - 1);
	}
};
struct LoopContext {
	int startAddress;
	int stepAddress;
	vector<int> breakJumps;
	vector<int> continueJumps;
	bool isForEach;
	int startLocalCount;
	int iteratorSlot;
};
struct Local {
	string name;
	int depth;
	bool isConst = false;
	bool hasKnownValue = false;
	Value knownValue;
};
struct ExceptionHandler {
	int catchAddress;
	int finallyAddress;
	int stackDepth;
	int scopeDepth;
	bool isInsideFinally = false;
};
struct CallFrame {
	FunctionObject *function;
	uint8_t *ip;
	int basePointer;
	vector<Value> cacheKey;
	vector<ExceptionHandler> handlerStack;
};
inline Value EvaluateConstBinary(TokenType op, const Value &a, const Value &b);
// ------------ AST WALKER -------------
struct Interpreter {
	std::shared_ptr<Env> env;
	unordered_map<string, FuncVal *> funcs;
	using ModuleLoader =
		std::function<void(std::shared_ptr<Env>, const vector<string> &)>;
	std::unordered_map<string, ModuleLoader> modules;
	bool returning = false;
	Value returnValue;
	bool breaking = false;
	bool continuing = false;
	int skipCount = 0;
	bool vectorEnabled = false;
	std::unordered_set<string> importStack;
	Interpreter() {
		enableColors();
		env = std::make_shared<Env>();
		env->set("None", Value::None(), true);
		registerStdLib();
	}
	Value nativePrint(const vector<Value> &args, int l, int c);
	void registerStdLib();
	LValue resolveLValue(Expr *e) {
		if (ValueExpr *ve = dynamic_cast<ValueExpr *>(e)) {
			if (ve->sourcePtr)
				return LValue(ve->sourcePtr, true);
			return LValue(&ve->val, false);
		}
		if (auto v = dynamic_cast<VarExpr *>(e)) {
			if (!env->exists(v->name))
				throw NameError("Undefined variable '" + v->name + "'", e->line,
					e->col);
			Var &var = env->lookup(v->name);
			if (var.alias)
				return LValue(var.alias, true, var.isConst, var.isLocked);
			return LValue(&var.value, false, var.isConst, var.isLocked);
		}
		if (auto idx = dynamic_cast<IndexExpr *>(e)) {
			LValue base = resolveLValue(idx->base);
			Value &container = *base.ref;
			int i = eval(idx->index).asInt();
			if (container.type == ValueType::LIST) {
				auto *listObj = static_cast<ListObject *>(container.ref.get());
				if (i < 0 || i > (int)listObj->elements.size())
					throw IndexError("List index out of range", e->line, e->col);
				if (i == (int)listObj->elements.size())
					listObj->elements.push_back(Value::None());

				return LValue(&listObj->elements[i], true,
					base.isConstView || container.isConst);
			}
			if (container.type == ValueType::STRING)
				return LValue(&container, true);

			throw TypeError("Type is not indexable or mutable", e->line, e->col);
		}
		throw TypeError("Expression is not assignable", e->line, e->col);
	}
	Value Resolve_methods(MethodCallExpr *m);
	bool matchesError(const LangError &e, const string &typeName) {
		// --- ROOT ---
		if (typeName == "Error")
			return true;
		// --- LEVEL 1: BASE CATEGORIES ---
		if (typeName == "InternalError")
			return dynamic_cast<const InternalError *>(&e);
		if (typeName == "ControlFlowError")
			return dynamic_cast<const ControlFlowError *>(&e);
		if (typeName == "ParseError")
			return dynamic_cast<const ParseError *>(&e);
		if (typeName == "RuntimeError")
			return dynamic_cast<const RuntimeError *>(&e);
		if (typeName == "Warning")
			return dynamic_cast<const Warning *>(&e);
		// --- LEVEL 2: RUNTIME ERROR SUBTREE ---
		if (typeName == "NameError")
			return dynamic_cast<const NameError *>(&e);
		if (typeName == "AttributeError")
			return dynamic_cast<const AttributeError *>(&e);
		if (typeName == "TypeError")
			return dynamic_cast<const TypeError *>(&e);
		if (typeName == "ArgumentError")
			return dynamic_cast<const ArgumentError *>(&e);
		if (typeName == "ValueError")
			return dynamic_cast<const ValueError *>(&e);
		if (typeName == "ConstError")
			return dynamic_cast<const ConstError *>(&e);
		if (typeName == "OwnershipError")
			return dynamic_cast<const OwnershipError *>(&e);
		if (typeName == "IndexError")
			return dynamic_cast<const IndexError *>(&e);
		if (typeName == "KeyError")
			return dynamic_cast<const KeyError *>(&e);
		if (typeName == "RangeError")
			return dynamic_cast<const RangeError *>(&e);
		if (typeName == "AssertionError")
			return dynamic_cast<const AssertionError *>(&e);
		if (typeName == "RecursionError")
			return dynamic_cast<const RecursionError *>(&e);
		if (typeName == "ImportError")
			return dynamic_cast<const ImportError *>(&e);
		if (typeName == "IOError")
			return dynamic_cast<const IOError *>(&e);
		if (typeName == "MathError")
			return dynamic_cast<const MathError *>(&e);
		if (typeName == "CastError")
			return dynamic_cast<const CastError *>(&e);
		if (typeName == "IteratorError")
			return dynamic_cast<const IteratorError *>(&e);
		if (typeName == "TimeoutError")
			return dynamic_cast<const TimeoutError *>(&e);
		if (typeName == "MemoryError")
			return dynamic_cast<const MemoryError *>(&e);
		if (typeName == "SystemError")
			return dynamic_cast<const SystemError *>(&e);
		// Children of ValueError
		if (typeName == "EmptyContainerError")
			return dynamic_cast<const EmptyContainerError *>(&e);
		// Children of ConstError
		if (typeName == "MutationError")
			return dynamic_cast<const MutationError *>(&e);
		// Children of ImportError
		if (typeName == "ModuleNotFoundError")
			return dynamic_cast<const ModuleNotFoundError *>(&e);
		if (typeName == "CircularImportError")
			return dynamic_cast<const CircularImportError *>(&e);
		if (typeName == "InvalidImportError")
			return dynamic_cast<const InvalidImportError *>(&e);
		// Children of IOError
		if (typeName == "FileNotFoundError")
			return dynamic_cast<const FileNotFoundError *>(&e);
		if (typeName == "PermissionError")
			return dynamic_cast<const PermissionError *>(&e);
		if (typeName == "EOFError")
			return dynamic_cast<const EOFError *>(&e);
		if (typeName == "FileClosedError")
			return dynamic_cast<const FileClosedError *>(&e);
		// Children of MathError
		if (typeName == "DivisionByZeroError")
			return dynamic_cast<const DivisionByZeroError *>(&e);
		if (typeName == "OverflowError")
			return dynamic_cast<const OverflowError *>(&e);
		if (typeName == "UnderflowError")
			return dynamic_cast<const UnderflowError *>(&e);
		if (typeName == "DomainError")
			return dynamic_cast<const DomainError *>(&e);
		// Children of SystemError
		if (typeName == "OSError")
			return dynamic_cast<const OSError *>(&e);
		if (typeName == "EnvironmentError")
			return dynamic_cast<const EnvironmentError *>(&e);
		if (typeName == "SignalError")
			return dynamic_cast<const SignalError *>(&e);
		// Children of ControlFlowError
		if (typeName == "ReturnSignal")
			return dynamic_cast<const ReturnSignal *>(&e);
		if (typeName == "BreakSignal")
			return dynamic_cast<const BreakSignal *>(&e);
		if (typeName == "ContinueSignal")
			return dynamic_cast<const ContinueSignal *>(&e);
		// Children of ParseError
		if (typeName == "SyntaxError")
			return dynamic_cast<const SyntaxError *>(&e);
		if (typeName == "IndentationError")
			return dynamic_cast<const IndentationError *>(&e);
		if (typeName == "UnexpectedTokenError")
			return dynamic_cast<const UnexpectedTokenError *>(&e);
		if (typeName == "UnterminatedLiteralError")
			return dynamic_cast<const UnterminatedLiteralError *>(&e);
		// Children of Warning
		if (typeName == "DeprecationWarning")
			return dynamic_cast<const DeprecationWarning *>(&e);
		if (typeName == "RuntimeWarning")
			return dynamic_cast<const RuntimeWarning *>(&e);
		if (typeName == "ImportWarning")
			return dynamic_cast<const ImportWarning *>(&e);
		// Fallback: Direct String Match (For exact matches or user-defined types)
		return e.type == typeName;
	}
	Value eval(Expr *e) {
		if (!e)
			return Value::None();
		if (ValueExpr *ve = dynamic_cast<ValueExpr *>(e))
			return ve->val;
		switch (e->type) {
		case ExprType::FSTRING: {
			auto fs = static_cast<FStringExpr *>(e);
			string res = "";
			for (auto *part : fs->parts) {
				Value v = eval(part);
				if (v.type == ValueType::STRING)
					res += v.asString();
				else
					res += valueToString(v);
			}
			return Value::String(res);
		}
		case ExprType::OWNERSHIP: {
			auto o = static_cast<OwnershipExpr *>(e);
			if (o->mode == CopyMode::REF) {
				LValue lv = resolveLValue(o->expr);
				return *lv.ref;
			}
			Value v = eval(o->expr);
			return applyCopy(v, o->mode);
		}
		case ExprType::BOOL: {
			return Value::Bool(static_cast<BoolExpr *>(e)->value);
		}
		case ExprType::NUMBER: {
			auto n = static_cast<NumberExpr *>(e);
			return n->isFloat ? Value::Float(n->val)
									: Value::Int((long long)n->val);
		}
		case ExprType::STRING: {
			return Value::String(static_cast<StringExpr *>(e)->val);
		}
		case ExprType::VAR: {
			auto *v = static_cast<VarExpr *>(e);
			if (v->cachedGlobal)
				return *v->cachedGlobal;
			if (!env->exists(v->name))
				throw NameError("Undefined variable '" + v->name + "'", e->line,
					e->col);
			Var &var = env->lookup(v->name);
			if (env->parent == nullptr)
				v->cachedGlobal = &var.value;
			if (var.alias)
				return *var.alias;
			return var.value;
		}
		case ExprType::LIST: {
			auto l = static_cast<ListExpr *>(e);
			vector<Value> vals;
			for (auto el : l->elements)
				vals.push_back(eval(el));
			return Value::List(vals);
		}
		case ExprType::BINARY: {
			auto b = static_cast<BinExpr *>(e);
			if (b->left && b->right && b->left->type == ExprType::NUMBER &&
				 b->right->type == ExprType::NUMBER) {
				double l = static_cast<NumberExpr *>(b->left)->val;
				double r = static_cast<NumberExpr *>(b->right)->val;
				switch (b->op) {
				case TokenType::PLUS:
					return Value::Float(l + r);
				case TokenType::MINUS:
					return Value::Float(l - r);
				case TokenType::STAR:
					return Value::Float(l * r);
				case TokenType::SLASH:
					return r != 0 ? Value::Float(l / r) : Value::None();
				case TokenType::POW:
					return Value::Float(pow(l, r));
				}
			}
			if (b->op == TokenType::AND) {
				Value l = eval(b->left);
				if (!l.isTruthy())
					return Value::Bool(false);
				return Value::Bool(eval(b->right).isTruthy());
			}
			if (b->op == TokenType::OR) {
				Value l = eval(b->left);
				if (l.isTruthy())
					return Value::Bool(true);
				return Value::Bool(eval(b->right).isTruthy());
			}
			Value l = b->left ? eval(b->left) : Value::None();
			Value r = eval(b->right);
			switch (b->op) {
			case TokenType::PLUS:
				if (l.type == ValueType::STRING ||
					 r.type == ValueType::STRING)
					return Value::String(
						(l.type == ValueType::STRING
								? l.asString()
								: (l.type == ValueType::FLOAT
										  ? std::to_string(l.asFloat())
										  : std::to_string(l.asInt()))) +
						(r.type == ValueType::STRING
								? r.asString()
								: (r.type == ValueType::FLOAT
										  ? std::to_string(r.asFloat())
										  : std::to_string(r.asInt()))));
				if (l.type == ValueType::INT && r.type == ValueType::INT) {
					long long a = l.asInt();
					long long b = r.asInt();
					if ((b > 0 && a > LLONG_MAX - b) ||
						 (b < 0 && a < LLONG_MIN - b))
						return Value::Float((double)a + (double)b);
					return Value::Int(a + b);
				}
				if (l.isNumber() && r.isNumber()) {
					return Value::Float(l.asFloat() + r.asFloat());
				}
				if (l.type == ValueType::VECTOR &&
					 r.type == ValueType::VECTOR) {
					auto *v1 = static_cast<VectorObject *>(l.ref.get());
					auto *v2 = static_cast<VectorObject *>(r.ref.get());
					if (v1->elements.size() != v2->elements.size())
						throw ValueError(
							"Vector dimension mismatch in addition", e->line,
							e->col);
					vector<Value> res;
					for (size_t i = 0; i < v1->elements.size(); i++)
						res.push_back(Value::Float(v1->elements[i].asFloat() +
															v2->elements[i].asFloat()));
					return Value::Vector(res);
				}
				throw TypeError("Invalid operands for +", e->line, e->col);
			case TokenType::FLOOR_DIV:
				if (r.asFloat() == 0)
					throw DivisionByZeroError("Division by zero", e->line,
						e->col);
				if (l.isNumber() && r.isNumber())
					return Value::Int(
						(long long)std::floor(l.asFloat() / r.asFloat()));
				break;
			case TokenType::MOD:
				if (r.asInt() == 0)
					throw DivisionByZeroError("Modulo by zero", e->line,
						e->col);
				if (l.type == ValueType::INT && r.type == ValueType::INT)
					return Value::Int(l.asInt() % r.asInt());
				if (l.isNumber() && r.isNumber()) {
					return Value::Float(std::fmod(l.asFloat(), r.asFloat()));
				}
				break;
			case TokenType::POW:
				if (l.isNumber() && r.isNumber())
					return Value::Float(std::pow(l.asFloat(), r.asFloat()));
				break;
			case TokenType::MINUS:
				if (l.isNumber() && r.isNumber()) {
					if (l.type == ValueType::FLOAT ||
						 r.type == ValueType::FLOAT)
						return Value::Float(l.asFloat() - r.asFloat());
					return Value::Int(l.asInt() - r.asInt());
				}
				if (l.type == ValueType::VECTOR &&
					 r.type == ValueType::VECTOR) {
					auto *v1 = static_cast<VectorObject *>(l.ref.get());
					auto *v2 = static_cast<VectorObject *>(r.ref.get());
					if (v1->elements.size() != v2->elements.size())
						throw ValueError(
							"Vector dimension mismatch in subtraction", e->line,
							e->col);
					vector<Value> res;
					for (size_t i = 0; i < v1->elements.size(); i++)
						res.push_back(Value::Float(v1->elements[i].asFloat() -
															v2->elements[i].asFloat()));
					return Value::Vector(res);
				}
				break;
			case TokenType::STAR:
				if (l.type == ValueType::INT && r.type == ValueType::INT) {
					long long a = l.asInt();
					long long b = r.asInt();
					if (a != 0 &&
						 (b > LLONG_MAX / abs(a) || b < LLONG_MIN / abs(a)))
						return Value::Float((double)a * (double)b);
					return Value::Int(a * b);
				}
				if (l.type == ValueType::STRING && r.type == ValueType::INT) {
					string res = "";
					string base = l.asString();
					long long count = r.asInt();
					if (count < 0)
						count = 0;
					if (count > 1000000)
						throw MemoryError("String repetition too large",
							e->line, e->col);
					for (long long i = 0; i < count; i++)
						res += base;
					return Value::String(res);
				}
				if (l.type == ValueType::LIST && r.type == ValueType::INT) {
					auto *listObj = static_cast<ListObject *>(l.ref.get());
					long long count = r.asInt();
					vector<Value> res;
					if (count > 0) {
						if (listObj->elements.size() * count > 1000000)
							throw MemoryError("List repetition too large",
								e->line, e->col);
						res.reserve(listObj->elements.size() * count);
						for (int i = 0; i < count; i++)
							for (const auto &elem : listObj->elements)
								res.push_back(deepCopy(elem));
					}
					return Value::List(res);
				}
				if (l.isNumber() && r.isNumber()) {
					return Value::Float(l.asFloat() * r.asFloat());
				}
				if (l.type == ValueType::VECTOR && r.isNumber()) {
					auto *v = static_cast<VectorObject *>(l.ref.get());
					double s = r.asFloat();
					vector<Value> res;
					for (auto val : v->elements)
						res.push_back(Value::Float(val.asFloat() * s));
					return Value::Vector(res);
				}
				if (l.isNumber() && r.type == ValueType::VECTOR) {
					auto *v = static_cast<VectorObject *>(r.ref.get());
					double s = l.asFloat();
					vector<Value> res;
					for (auto val : v->elements)
						res.push_back(Value::Float(val.asFloat() * s));
					return Value::Vector(res);
				}
				if (l.type == ValueType::VECTOR &&
					 r.type == ValueType::VECTOR) {
					auto *v1 = static_cast<VectorObject *>(l.ref.get());
					auto *v2 = static_cast<VectorObject *>(r.ref.get());
					if (v1->elements.size() != v2->elements.size())
						throw ValueError(
							"Vector dimension mismatch in multiplication",
							e->line, e->col);
					vector<Value> res;
					for (size_t i = 0; i < v1->elements.size(); i++)
						res.push_back(Value::Float(v1->elements[i].asFloat() *
															v2->elements[i].asFloat()));
					return Value::Vector(res);
				}
				break;
			case TokenType::SLASH:
				if (r.asFloat() == 0)
					throw DivisionByZeroError("Division by zero", e->line,
						e->col);
				if (l.isNumber() && r.isNumber())
					return Value::Float(l.asFloat() / r.asFloat());
				if (l.type == ValueType::VECTOR ||
					 r.type == ValueType::VECTOR)
					throw TypeError("Vector division is not supported",
						e->line, e->col);
				break;
			case TokenType::GT:
				return Value::Bool(l.asFloat() > r.asFloat());
			case TokenType::LT:
				return Value::Bool(l.asFloat() < r.asFloat());
			case TokenType::GTE:
				return Value::Bool(l.asFloat() >= r.asFloat());
			case TokenType::LTE:
				return Value::Bool(l.asFloat() <= r.asFloat());
			case TokenType::EQ:
				return Value::Bool(l.looseEquals(r));
			case TokenType::STRICT_EQ:
				return Value::Bool(l.strictEquals(r));
			case TokenType::STRICT_NEQ:
				return Value::Bool(!l.strictEquals(r));
			case TokenType::NEQ:
				return Value::Bool(!l.looseEquals(r));
			case TokenType::AND:
				if (l.type == ValueType::SET && r.type == ValueType::SET)
					break;
				return Value::Bool(l.isTruthy() && r.isTruthy());
			case TokenType::OR:
				if (l.type == ValueType::SET && r.type == ValueType::SET)
					break;
				return Value::Bool(l.isTruthy() || r.isTruthy());
			case TokenType::XOR:
				if (l.type == ValueType::SET && r.type == ValueType::SET)
					break;
				return Value::Bool(l.isTruthy() != r.isTruthy());
			case TokenType::NAND:
				return Value::Bool(!(l.isTruthy() && r.isTruthy()));
			case TokenType::NOR:
				return Value::Bool(!(l.isTruthy() || r.isTruthy()));
			case TokenType::NXOR:
				return Value::Bool(l.isTruthy() == r.isTruthy());
			case TokenType::NOT:
				return Value::Bool(!r.isTruthy());
			case TokenType::IS: {
				if (l.ref && r.ref)
					return Value::Bool(l.ref.get() == r.ref.get());
				if (l.type == ValueType::NONE && r.type == ValueType::NONE)
					return Value::Bool(true);
				return Value::Bool(false);
			}
			case TokenType::IS_IN: {
				if (r.type == ValueType::STRING) {
					if (l.type != ValueType::STRING) {
						return Value::Bool(false);
					}
					return Value::Bool(r.asString().find(l.asString()) !=
											 string::npos);
				}
				if (r.type == ValueType::LIST) {
					auto *listObj = static_cast<ListObject *>(r.ref.get());
					for (const auto &item : listObj->elements) {
						if (item.strictEquals(l))
							return Value::Bool(true);
					}
					return Value::Bool(false);
				}
				if (r.type == ValueType::DICT) {
					auto *d = static_cast<DictObject *>(r.ref.get());
					return Value::Bool(d->items.count(l));
				}
				if (r.type == ValueType::SET) {
					auto *s = static_cast<SetObject *>(r.ref.get());
					for (const auto &item : s->elements) {
						if (item.strictEquals(l))
							return Value::Bool(true);
					}
					return Value::Bool(false);
				}
				if (r.type == ValueType::RANGE) {
					auto *rng = static_cast<RangeObject *>(r.ref.get());
					if (!l.isNumber())
						return Value::Bool(false);
					double val = l.asFloat();
					if (!rng->isValid)
						return Value::Bool(false);
					bool inBounds =
						(rng->step > 0)
							? (val >= rng->start &&
								  (rng->endInclusive ? val <= rng->end
															: val < rng->end))
							: (val <= rng->start &&
								  (rng->endInclusive ? val >= rng->end
															: val > rng->end));
					if (!inBounds)
						return Value::Bool(false);
					if (!rng->isFloat && l.type == ValueType::INT) {
						long long diff = (long long)(val - rng->start);
						if (diff % (long long)rng->step != 0)
							return Value::Bool(false);
					}
					return Value::Bool(true);
				}
				throw TypeError("RHS of 'in' must be iterable", e->line,
					e->col);
			}
			case TokenType::IS_NOT: {
				if (l.ref && r.ref)
					return Value::Bool(l.ref.get() != r.ref.get());
				if (l.type == ValueType::NONE && r.type == ValueType::NONE)
					return Value::Bool(false);
				return Value::Bool(true);
			}
			case TokenType::IS_NOT_IN: {
				if (r.type == ValueType::STRING) {
					if (l.type != ValueType::STRING)
						throw TypeError(
							"LHS of 'is not in' string must be a string",
							e->line, e->col);
					return Value::Bool(r.asString().find(l.asString()) ==
											 string::npos);
				} else if (r.type == ValueType::LIST) {
					auto *listObj = static_cast<ListObject *>(r.ref.get());
					for (const auto &item : listObj->elements)
						if (item.looseEquals(l))
							return Value::Bool(false);
					return Value::Bool(true);
				} else if (r.type == ValueType::RANGE) {
					auto *rng = static_cast<RangeObject *>(r.ref.get());
					if (!l.isNumber())
						return Value::Bool(true);
					double val = l.asFloat();
					if (!rng->isValid)
						return Value::Bool(true);
					bool inBounds =
						(rng->step > 0)
							? (val >= rng->start &&
								  (rng->endInclusive ? val <= rng->end
															: val < rng->end))
							: (val <= rng->start &&
								  (rng->endInclusive ? val >= rng->end
															: val > rng->end));
					if (!inBounds)
						return Value::Bool(true);
					if (!rng->isFloat && l.type == ValueType::INT) {
						long long diff = (long long)(val - rng->start);
						if (diff % (long long)rng->step != 0)
							return Value::Bool(true);
					}
					return Value::Bool(false);
				} else if (r.type == ValueType::SET) {
					auto *s = static_cast<SetObject *>(r.ref.get());
					for (const auto &el : s->elements)
						if (el.strictEquals(l))
							return Value::Bool(false);
					return Value::Bool(true);
				} else if (r.type == ValueType::TUPLE) {
					auto *t = static_cast<TupleObject *>(r.ref.get());
					for (const auto &item : t->elements)
						if (item.looseEquals(l))
							return Value::Bool(false);
					return Value::Bool(true);
				}
				throw TypeError("RHS of 'is not in' must be iterable",
					e->line, e->col);
			}
			case TokenType::COLON: {
				Value k = l;
				Value v = r;
				vector<std::pair<Value, Value>> p;
				p.push_back({k, v});
				return Value::Paired(p);
			}
			default:
				return Value::None();
			}
			if (l.type == ValueType::SET && r.type == ValueType::SET) {
				auto *s1 = static_cast<SetObject *>(l.ref.get());
				auto *s2 = static_cast<SetObject *>(r.ref.get());
				std::unordered_set<Value, ValueHash, ValueEqual> result;
				if (b->op == TokenType::OR) {
					result = s1->elements;
					for (auto &v : s2->elements)
						setAdd(result, v);
					return Value::Set(result);
				}
				if (b->op == TokenType::AND) {
					for (auto &v2 : s2->elements) {
						if (s1->elements.count(v2))
							setAdd(result, v2);
					}
					return Value::Set(result);
				}
				if (b->op == TokenType::MINUS || b->op == TokenType::SLASH) {
					for (auto &v1 : s1->elements) {
						if (s2->elements.count(v1))
							continue;
						setAdd(result, v1);
					}
					return Value::Set(result);
				}
				if (b->op == TokenType::XOR) {
					for (auto &v1 : s1->elements) {
						if (s2->elements.count(v1))
							continue;
						setAdd(result, v1);
					}
					for (auto &v2 : s2->elements) {
						if (s1->elements.count(v2))
							continue;
						setAdd(result, v2);
					}
					return Value::Set(result);
				}
			}
		}
		case ExprType::OMIT_MARKER_EXPR: {
			return Value::Omit();
		}
		case ExprType::CALL: {
			auto c = static_cast<CallExpr *>(e);
			Value callee;
			if (c->cachedFunc) {
				callee = *c->cachedFunc;
			} else {
				if (!env->exists(c->name))
					throw NameError("Undefined function '" + c->name + "'",
						c->line, c->col);
				Var &v = env->lookup(c->name);
				callee = v.value;
				if (env->parent == nullptr)
					c->cachedFunc = &v.value;
			}
			vector<CallArg> callArgs;
			for (size_t i = 0; i < c->args.size(); i++) {
				CallArg ca;
				if (c->modes[i] == CopyMode::REF) {
					LValue lv = resolveLValue(c->args[i]);
					ca.lvalue = lv;
					ca.hasLValue = true;
					ca.value = *lv.ref;
				} else
					ca.value = eval(c->args[i]);
				callArgs.push_back(ca);
			}
			return call(callee, callArgs, c->modes, c->line, c->col);
		}
		case ExprType::INDEX: {
			auto idx = static_cast<IndexExpr *>(e);
			Value base = eval(idx->base);
			Value index = eval(idx->index);
			auto normalize = [&](long long i, size_t len) {
				if (i < 0)
					i += len;
				if (i < 0 || i >= (long long)len)
					throw IndexError("Index out of bounds", idx->line, idx->col);
				return i;
			};
			auto getSliceIndices = [&](size_t len) -> vector<long long> {
				auto *s = static_cast<SliceObject *>(index.ref.get());
				long long step =
					(s->step.type == ValueType::INT) ? s->step.asInt() : 1;
				if (step == 0)
					throw ValueError("Slice step cannot be zero", idx->line,
						idx->col);
				long long start, end;
				if (step > 0) {
					start =
						(s->start.type == ValueType::INT) ? s->start.asInt() : 0;
					end = (s->end.type == ValueType::INT) ? s->end.asInt()
																	  : (long long)len;
				} else {
					start = (s->start.type == ValueType::INT)
								  ? s->start.asInt()
								  : (long long)len - 1;
					end = (s->end.type == ValueType::INT) ? s->end.asInt()
																	  : -1LL - 1LL;
				}
				auto normalizeSlice = [&](long long val) {
					return (val < 0) ? val + (long long)len : val;
				};
				if (s->start.type == ValueType::INT)
					start = normalizeSlice(start);
				if (s->end.type == ValueType::INT)
					end = normalizeSlice(end);
				auto clamp = [&](long long val) {
					if (val < 0)
						return (step > 0) ? 0LL : -1LL;
					if (val > (long long)len)
						return (long long)len;
					return val;
				};
				if (step > 0) {
					if (start < 0)
						start = 0;
					if (end < 0)
						end = 0;
					if (start > (long long)len)
						start = len;
					if (end > (long long)len)
						end = len;
				} else {
					if (start < -1)
						start = -1;
					if (end < -1)
						end = -1;
					if (start > (long long)len - 1)
						start = len - 1;
					if (end > (long long)len - 1)
						end = len - 1;
				}
				vector<long long> result;
				if (step > 0) {
					for (long long i = start; i < end; i += step) {
						if (i >= 0 && i < (long long)len)
							result.push_back(i);
					}
				} else {
					for (long long i = start; i > end; i += step) {
						if (i >= 0 && i < (long long)len)
							result.push_back(i);
					}
				}
				return result;
			};
			if (base.type == ValueType::DICT) {
				auto *dictObj = static_cast<DictObject *>(base.ref.get());
				if (dictObj->items.find(index) == dictObj->items.end()) {
					throw KeyError("Key not found in dictionary", idx->line,
						idx->col);
				}
				return dictObj->items.at(index);
			}
			if (base.type == ValueType::LIST) {
				auto *list = static_cast<ListObject *>(base.ref.get());
				size_t len = list->elements.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					Value el = list->elements[i];
					if (base.isConst)
						el.isConst = true;
					return el;
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					auto *newList = new ListObject();
					for (long long i : indices) {
						newList->elements.push_back(list->elements[i]);
					}
					Value ret;
					ret.type = ValueType::LIST;
					ret.ref = shared_ptr<HeapObject>(newList);
					ret.isConst = base.isConst;
					return ret;
				}
				throw TypeError("List index must be int or slice", idx->line,
					idx->col);
			}
			if (base.type == ValueType::STRING) {
				const string &s = base.asString();
				size_t len = s.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					return Value::String(string(1, s[i]));
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					string newStr = "";
					for (long long i : indices)
						newStr += s[i];
					return Value::String(newStr);
				}
				throw TypeError("String index must be int or slice", idx->line,
					idx->col);
			}
			if (base.type == ValueType::TUPLE) {
				auto *t = static_cast<TupleObject *>(base.ref.get());
				size_t len = t->elements.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					return t->elements[i];
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					auto *newTuple = new TupleObject();
					for (long long i : indices)
						newTuple->elements.push_back(t->elements[i]);
					Value ret;
					ret.type = ValueType::TUPLE;
					ret.ref = shared_ptr<HeapObject>(newTuple);
					ret.isConst = base.isConst;
					return ret;
				}
				throw TypeError("Tuple index must be int or slice", idx->line,
					idx->col);
			}
			if (base.type == ValueType::RANGE) {
				if (index.type != ValueType::INT)
					throw TypeError("slice index must be int", idx->line,
						idx->col);
				auto *r = static_cast<RangeObject *>(base.ref.get());
				long long i = index.asInt();
				double val = r->start;
				if (!r->startInclusive)
					val += r->step;
				val += (r->step * i);
				bool outOfBounds = false;
				if (r->step > 0) {
					if (r->endInclusive) {
						if (val > r->end)
							outOfBounds = true;
					} else {
						if (val >= r->end)
							outOfBounds = true;
					}
				} else {
					if (r->endInclusive) {
						if (val < r->end)
							outOfBounds = true;
					} else {
						if (val <= r->end)
							outOfBounds = true;
					}
				}
				if (outOfBounds || i < 0)
					throw IndexError("slice index out of bounds", idx->line,
						idx->col);
				return r->isFloat ? Value::Float(val)
										: Value::Int((long long)val);
			}
			if (base.type == ValueType::SET) {
				auto *setObj = static_cast<SetObject *>(base.ref.get());
				size_t len = setObj->elements.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					auto it = setObj->elements.begin();
					std::advance(it, i);
					Value val = *it;
					if (base.isConst)
						val.isConst = true;
					return val;
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					auto *newSet = new SetObject();
					for (long long idx_val : indices) {
						auto it = setObj->elements.begin();
						std::advance(it, idx_val);
						setAdd(newSet->elements, *it);
					}
					Value ret;
					ret.type = ValueType::SET;
					ret.ref = shared_ptr<HeapObject>(newSet);
					ret.isConst = base.isConst;
					return ret;
				}
				throw TypeError("Set index must be int or slice", idx->line,
					idx->col);
			}
			if (base.type == ValueType::VECTOR) {
				auto *vec = static_cast<VectorObject *>(base.ref.get());
				size_t len = vec->elements.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					return Value::Float(vec->elements[i].asFloat());
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					vector<Value> newElems;
					for (long long i : indices) {
						newElems.push_back(vec->elements[i]);
					}
					return Value::Vector(newElems);
				}
				throw TypeError("Vector index must be int or slice", idx->line,
					idx->col);
			}
			throw TypeError("Indexing non-indexable type", idx->line, idx->col);
		}
		case ExprType::METHOD_CALL: {
			return Resolve_methods(static_cast<MethodCallExpr *>(e));
		}
		case ExprType::LAMBDA: {
			auto lam = static_cast<LambdaExpr *>(e);
			auto *funcObj = new FunctionObject(
				lam->params, lam->returnType, lam->defaultRetArgs,
				lam->returnsConst, lam->body, env, lam->isCached);
			Value v;
			v.type = ValueType::FUNCTION;
			v.ref = std::shared_ptr<HeapObject>(funcObj);
			return v;
		}
		case ExprType::RANGE: {
			auto r = static_cast<RangeExpr *>(e);
			Value s = eval(r->start);
			Value en = eval(r->end);
			if (!s.isNumber() || !en.isNumber())
				throw TypeError("Range start and end must be numbers", r->line,
					r->col);
			double startVal = s.asFloat();
			double endVal = en.asFloat();
			double stepVal = 1.0;
			bool isFloat =
				(s.type == ValueType::FLOAT || en.type == ValueType::FLOAT);
			if (r->step == nullptr) {
				if (startVal > endVal)
					stepVal = -1.0;
				else
					stepVal = 1.0;
			} else {
				Value st = eval(r->step);
				if (!st.isNumber())
					throw TypeError("Range step must be a number", r->line,
						r->col);
				stepVal = st.asFloat();
				if (st.type == ValueType::FLOAT)
					isFloat = true;
			}
			if (stepVal == 0)
				throw ValueError("Range step cannot be zero", r->line, r->col);
			return Value::Range(startVal, endVal, stepVal, r->startInclusive,
				r->endInclusive, isFloat);
		}
		case ExprType::SET: {
			auto s = static_cast<SetExpr *>(e);
			std::unordered_set<Value, ValueHash, ValueEqual> vals;
			for (auto el : s->elements)
				setAdd(vals, eval(el));
			return Value::Set(vals);
		}
		case ExprType::DICT: {
			auto d = static_cast<DictExpr *>(e);
			std::unordered_map<Value, Value, ValueHash, ValueEqual> map;
			for (auto &item : d->items) {
				Value key = eval(item.first);
				Value val = eval(item.second);
				if (key.type == ValueType::LIST || key.type == ValueType::SET) {
					key = deepCopy(key);
					key.isConst = true;
				}
				if (key.type == ValueType::DICT)
					throw TypeError("Dictionary cannot be used as a key", e->line,
						e->col);
				map[key] = val;
			}
			return Value::Dict(map);
		}
		case ExprType::TUPLE: {
			auto t = static_cast<TupleExpr *>(e);
			vector<Value> vals;
			for (auto el : t->elements)
				vals.push_back(eval(el));
			return Value::Tuple(vals);
		}
		case ExprType::TERNARY: {
			auto t = static_cast<TernaryExpr *>(e);
			Value cond = eval(t->condition);
			if (cond.isTruthy())
				return eval(t->trueBranch);
			else
				return eval(t->falseBranch);
		}
		case ExprType::COMPREHENSION: {
			auto comp = static_cast<CompExpr *>(e);
			Value collection = eval(comp->iterable);
			vector<Value> items;
			bool isSet = false;
			if (collection.type == ValueType::LIST)
				items = static_cast<ListObject *>(collection.ref.get())->elements;
			else if (collection.type == ValueType::SET) {
				auto *s = static_cast<SetObject *>(collection.ref.get());
				items.assign(s->elements.begin(), s->elements.end());
			} else if (collection.type == ValueType::TUPLE)
				items =
					static_cast<TupleObject *>(collection.ref.get())->elements;
			else if (collection.type == ValueType::STRING) {
				for (char c : collection.asString())
					items.push_back(Value::String(string(1, c)));
			} else if (collection.type == ValueType::RANGE) {
				auto *r = static_cast<RangeObject *>(collection.ref.get());
				double cur = r->start;
				if (!r->startInclusive)
					cur += r->step;
				while (true) {
					bool cond =
						(r->step > 0)
							? (r->endInclusive ? cur <= r->end : cur < r->end)
							: (r->endInclusive ? cur >= r->end : cur > r->end);
					if (!cond)
						break;
					items.push_back(r->isFloat ? Value::Float(cur)
														: Value::Int((long long)cur));
					cur += r->step;
				}
			} else if (collection.type == ValueType::DICT) {
				auto *d = static_cast<DictObject *>(collection.ref.get());
				for (auto &pair : d->items)
					items.push_back(pair.first);
			} else if (collection.type == ValueType::VECTOR) {
				auto *v = static_cast<VectorObject *>(collection.ref.get());
				for (auto d : v->elements)
					items.push_back(d);
			} else
				throw TypeError("Comprehension 'in' target must be iterable", comp->line, comp->col);
			vector<Value> listResults;
			std::unordered_set<Value, ValueHash, ValueEqual> setResults;
			std::unordered_map<Value, Value, ValueHash, ValueEqual> dictResults;
			bool isDict = (comp->typeToken == TokenType::LBRACE && comp->valueExpr != nullptr);
			bool isSetComp = (comp->typeToken == TokenType::LBRACE && comp->valueExpr == nullptr);
			std::shared_ptr<Env> prevEnv = env;
			for (const auto &item : items) {
				env = std::make_shared<Env>();
				env->parent = prevEnv;
				env->set(comp->varName, item, false, false);
				if (comp->filter) {
					Value cond = eval(comp->filter);
					if (!cond.isTruthy()) {
						continue;
					}
				}
				if (isDict) {
					Value k = eval(comp->expression);
					if (k.type == ValueType::LIST || k.type == ValueType::SET || k.type == ValueType::DICT) {
						k = deepCopy(k);
						k.isConst = true;
					}
					Value v = eval(comp->valueExpr);
					dictResults[k] = v;
				} else {
					Value v = eval(comp->expression);
					if (isSetComp) {
						if (v.type == ValueType::LIST || v.type == ValueType::SET || v.type == ValueType::DICT) {
							v = deepCopy(v);
							v.isConst = true;
						}
						setResults.insert(v);
					} else {
						listResults.push_back(v);
					}
				}
			}
			env = prevEnv;
			if (comp->typeToken == TokenType::LBRACKET)
				return Value::List(listResults);
			if (comp->typeToken == TokenType::LPAREN)
				return Value::Tuple(listResults);
			if (comp->typeToken == TokenType::LBRACE) {
				if (isDict)
					return Value::Dict(dictResults);
				else
					return Value::Set(setResults);
			}
			return Value::None();
		}
		case ExprType::SLICE: {
			auto sl = static_cast<SliceExpr *>(e);
			Value s = sl->start ? eval(sl->start) : Value::None();
			Value en = sl->end ? eval(sl->end) : Value::None();
			Value st = sl->step ? eval(sl->step) : Value::None();
			return Value::Slice(s, en, st);
		}
		case ExprType::VECTOR: {
			auto ve = static_cast<VectorExpr *>(e);
			if (!vectorEnabled)
				throw RuntimeError(
					"Vector syntax <...> requires 'import Vector'", e->line,
					e->col);
			vector<Value> elems;
			for (auto *el : ve->elements) {
				Value v = eval(el);
				if (!v.isNumber())
					throw TypeError("Vector elements must be numbers", e->line,
						e->col);
				elems.push_back(v);
			}
			return Value::Vector(elems);
		}
		default: {
			return Value::None();
		}
		}
	}
	Value exec(Stmt *s) {
		switch (s->type) {
		case StmtType::ASSIGN: {
			auto as = static_cast<AssignStmt *>(s);
			if (as->target->type == ExprType::INDEX) {
				auto idx = static_cast<IndexExpr *>(as->target);
				Value base = eval(idx->base);
				if (base.type == ValueType::STRING) {
					Value rhs = eval(as->value);
					if (rhs.type != ValueType::STRING)
						throw TypeError("Can only insert string into string",
							s->line, s->col);
					int i = eval(idx->index).asInt();
					auto *strObj = static_cast<StringObject *>(base.ref.get());
					string &str = strObj->value;
					if (i < 0 || i > (int)str.size())
						throw IndexError("String index out of bounds", s->line,
							s->col);
					if (i == (int)str.size())
						str += rhs.asString();
					else {
						const string &r = rhs.asString();
						int eraseLen =
							std::min((int)r.size(), (int)str.size() - i);
						str.erase(i, eraseLen);
						str.insert(i, r);
					}
					return Value::None();
				}
				if (base.type == ValueType::DICT) {
					if (base.isConst)
						throw ConstError("Cannot modify const dictionary", s->line,
							s->col);
					Value key = eval(idx->index);
					Value val = eval(as->value);
					if (key.type == ValueType::LIST ||
						 key.type == ValueType::SET) {
						key = deepCopy(key);
						key.isConst = true;
					}
					if (key.type == ValueType::DICT)
						throw TypeError("Dictionary cannot be used as a key",
							s->line, s->col);
					auto *dictObj = static_cast<DictObject *>(base.ref.get());
					dictObj->items[key] = val;
					return Value::None();
				}
			}
			LValue lv = resolveLValue(as->target);
			Value &cur = *lv.ref;
			if (cur.isConst || lv.isConstView)
				throw ConstError("Cannot assign to or modify const variable",
					s->line, s->col);
			Value rhs = eval(as->value);
			if (lv.isLocked && cur.type != rhs.type) {
				throw TypeError("Cannot change type of locked variable", s->line,
					s->col);
			}
			if (as->op == TokenType::ASSIGN) {
				if (lv.isRefTarget) {
					*lv.ref = rhs;
					return Value::None();
				}
				if (cur.ref && cur.ref->typeLocked && cur.type != rhs.type) {
					throw TypeError("Cannot change type of locked variable",
						s->line, s->col);
				}
				if (cur.ref && cur.type == rhs.type) {
					if (cur.type == ValueType::STRING)
						static_cast<StringObject *>(cur.ref.get())->value =
							rhs.asString();
					else
						cur = rhs;
				} else
					cur = rhs;
				return Value::None();
			}
			if (cur.type == ValueType::STRING) {
				if (rhs.type == ValueType::STRING &&
					 as->op == TokenType::PLUS_EQ) {
					static_cast<StringObject *>(cur.ref.get())->value +=
						rhs.asString();
					return Value::None();
				}
				if (rhs.type == ValueType::INT && as->op == TokenType::STAR_EQ) {
					string base =
						static_cast<StringObject *>(cur.ref.get())->value;
					long long count = rhs.asInt();
					if (count < 0)
						count = 0;
					string res = "";
					if (base.length() * count > 10000000)
						throw MemoryError("Resulting string too large", s->line,
							s->col);
					for (int k = 0; k < count; k++)
						res += base;
					static_cast<StringObject *>(cur.ref.get())->value = res;
					return Value::None();
				}
			}
			if (cur.type == ValueType::LIST && rhs.type == ValueType::INT &&
				 as->op == TokenType::STAR_EQ) {
				auto *listObj = static_cast<ListObject *>(cur.ref.get());
				int count = rhs.asInt();
				if (count <= 0)
					listObj->elements.clear();
				else if (count > 1) {
					vector<Value> orig = listObj->elements;
					if (orig.size() * count > 1000000)
						throw MemoryError("Resulting list too large", s->line,
							s->col);
					listObj->elements.reserve(orig.size() * count);
					for (int k = 1; k < count; k++)
						listObj->elements.insert(listObj->elements.end(),
							orig.begin(), orig.end());
				}
				return Value::None();
			}
			if (cur.type == ValueType::FLOAT && rhs.type == ValueType::FLOAT) {
				switch (as->op) {
				case TokenType::PLUS_EQ:
					cur.fVal += rhs.asFloat();
					break;
				case TokenType::MINUS_EQ:
					cur.fVal -= rhs.asFloat();
					break;
				case TokenType::STAR_EQ:
					cur.fVal *= rhs.asFloat();
					break;
				case TokenType::DIV_EQ:
					if (rhs.asFloat() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.fVal /= rhs.asFloat();
					break;
				case TokenType::POW_EQ:
					cur.fVal = std::pow(cur.fVal, rhs.asFloat());
					break;
				default:
					throw TypeError("Invalid float operation", s->line,
						s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::FLOAT && rhs.type == ValueType::INT) {
				switch (as->op) {
				case TokenType::PLUS_EQ:
					cur.fVal += rhs.asFloat();
					break;
				case TokenType::MINUS_EQ:
					cur.fVal -= rhs.asFloat();
					break;
				case TokenType::STAR_EQ:
					cur.fVal *= rhs.asFloat();
					break;
				case TokenType::DIV_EQ:
					if (rhs.asFloat() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.fVal /= rhs.asFloat();
					break;
				case TokenType::POW_EQ:
					cur.fVal = std::pow(cur.fVal, rhs.asFloat());
					break;
				default:
					throw TypeError("Invalid float operation", s->line,
						s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::INT && rhs.type == ValueType::INT) {
				switch (as->op) {
				case TokenType::PLUS_EQ:
					cur.iVal += rhs.asInt();
					break;
				case TokenType::MINUS_EQ:
					cur.iVal -= rhs.asInt();
					break;
				case TokenType::STAR_EQ:
					cur.iVal *= rhs.asInt();
					break;
				case TokenType::DIV_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.iVal /= rhs.asInt();
					break;
				case TokenType::MOD_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Modulo by zero", s->line,
							s->col);
					cur.iVal %= rhs.asInt();
					break;
				case TokenType::AND_EQ:
					cur.iVal &= rhs.asInt();
					break;
				case TokenType::OR_EQ:
					cur.iVal |= rhs.asInt();
					break;
				case TokenType::XOR_EQ:
					cur.iVal ^= rhs.asInt();
					break;
				case TokenType::FLOOR_DIV_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.iVal =
						(long long)std::floor((double)cur.iVal / rhs.asInt());
					break;
				case TokenType::POW_EQ:
					cur.iVal = (long long)std::pow(cur.iVal, rhs.asInt());
					break;
				default:
					throw TypeError("Invalid int operation", s->line, s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::INT && rhs.type == ValueType::FLOAT) {
				switch (as->op) {
				case TokenType::PLUS_EQ:
					cur.iVal += rhs.asInt();
					break;
				case TokenType::MINUS_EQ:
					cur.iVal -= rhs.asInt();
					break;
				case TokenType::STAR_EQ:
					cur.iVal *= rhs.asInt();
					break;
				case TokenType::DIV_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.iVal /= rhs.asInt();
					break;
				case TokenType::MOD_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Modulo by zero", s->line,
							s->col);
					cur.iVal %= rhs.asInt();
					break;
				case TokenType::AND_EQ:
					cur.iVal &= rhs.asInt();
					break;
				case TokenType::OR_EQ:
					cur.iVal |= rhs.asInt();
					break;
				case TokenType::XOR_EQ:
					cur.iVal ^= rhs.asInt();
					break;
				case TokenType::FLOOR_DIV_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.iVal =
						(long long)std::floor((double)cur.iVal / rhs.asInt());
					break;
				case TokenType::POW_EQ:
					cur.iVal = (long long)std::pow(cur.iVal, rhs.asInt());
					break;
				default:
					throw TypeError("Invalid int operation", s->line, s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::BOOL && rhs.type == ValueType::BOOL) {
				switch (as->op) {
				case TokenType::AND_EQ:
					cur.bVal = cur.bVal & rhs.asBool();
					break;
				case TokenType::OR_EQ:
					cur.bVal = cur.bVal | rhs.asBool();
					break;
				case TokenType::XOR_EQ:
					cur.bVal = cur.bVal ^ rhs.asBool();
					break;
				default:
					throw TypeError("Invalid bool operation", s->line, s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::SET && rhs.type == ValueType::SET) {
				auto *s1 = static_cast<SetObject *>(cur.ref.get());
				auto *s2 = static_cast<SetObject *>(rhs.ref.get());
				if (as->op == TokenType::OR_EQ) {
					s1->elements.insert(s2->elements.begin(), s2->elements.end());
					return Value::None();
				}
				if (as->op == TokenType::AND_EQ) {
					std::unordered_set<Value, ValueHash, ValueEqual> keep;
					for (const auto &v1 : s1->elements) {
						if (s2->elements.count(v1) > 0) {
							keep.insert(v1);
						}
					}
					s1->elements = std::move(keep);
					return Value::None();
				}
				if (as->op == TokenType::MINUS_EQ || as->op == TokenType::DIV_EQ) {
					for (const auto &v2 : s2->elements) {
						s1->elements.erase(v2);
					}
					return Value::None();
				}
				if (as->op == TokenType::XOR_EQ) {
					for (const auto &v2 : s2->elements) {
						if (s1->elements.count(v2) > 0) {
							s1->elements.erase(v2);
						} else {
							s1->elements.insert(v2);
						}
					}
					return Value::None();
				}
			}
			if (cur.type == ValueType::VECTOR) {
				auto *v = static_cast<VectorObject *>(cur.ref.get());
				if (as->op == TokenType::PLUS_EQ) {
					if (rhs.type != ValueType::VECTOR)
						throw TypeError("+= requires a vector", s->line, s->col);
					auto *v2 = static_cast<VectorObject *>(rhs.ref.get());
					if (v->elements.size() != v2->elements.size())
						throw ValueError("Dimension mismatch", s->line, s->col);
					for (size_t i = 0; i < v->elements.size();
						i++)
						; // v->elements[i].asFloat() +=
						  // v2->elements[i].asFloat();
					return Value::None();
				}
				if (as->op == TokenType::MINUS_EQ) {
					if (rhs.type != ValueType::VECTOR)
						throw TypeError("-= requires a vector", s->line, s->col);
					auto *v2 = static_cast<VectorObject *>(rhs.ref.get());
					if (v->elements.size() != v2->elements.size())
						throw ValueError("Dimension mismatch", s->line, s->col);
					for (size_t i = 0; i < v->elements.size();
						i++)
						; // v->elements[i].asFloat() -=
						  // v2->elements[i].asFloat();
					return Value::None();
				}
				if (as->op == TokenType::STAR_EQ) {
					if (rhs.isNumber()) {
						double scalar = rhs.asFloat();
						for (auto &val : v->elements)
							; // val.asFloat() *= scalar;
					} else if (rhs.type == ValueType::VECTOR) {
						auto *v2 = static_cast<VectorObject *>(rhs.ref.get());
						if (v->elements.size() != v2->elements.size())
							throw ValueError("Dimension mismatch", s->line, s->col);
						for (size_t i = 0; i < v->elements.size();
							i++)
							; // v->elements[i].asFloat() *=
							  // v2->elements[i].asFloat();
					} else
						throw TypeError("*= requires number or vector", s->line,
							s->col);
					return Value::None();
				}
				if (as->op == TokenType::DIV_EQ)
					throw TypeError("/= not supported for vectors", s->line,
						s->col);
			}
			throw TypeError("Invalid operands for compound assignment", s->line,
				s->col);
		}
		case StmtType::LET: {
			auto let = static_cast<LetStmt *>(s);
			if (env->existsLocal(let->name)) {
				throw RuntimeError("Variable '" + let->name +
											 "' is already defined in this scope.",
					s->line, s->col);
			}
			if (env->exists(let->name)) {
				Var &existing = env->lookup(let->name);
				if (existing.isConst) {
					throw ConstError(
						"Cannot shadow constant '" + let->name + "'.", s->line,
						s->col);
				}
			}
			if (!let->value) {
				env->set(let->name, Value::NoType(), false);
				return Value::None();
			}
			if (let->value->type == ExprType::OWNERSHIP) {
				auto o = static_cast<OwnershipExpr *>(let->value);
				if (o->mode == CopyMode::REF) {
					LValue lv = resolveLValue(o->expr);
					Var varStruct;
					varStruct.alias = lv.ref;
					varStruct.isConst = let->isConst;
					env->vars[let->name] = varStruct;
					return Value::None();
				}
			}
			Value v = eval(let->value);
			if (let->isConst)
				v.isConst = true;
			bool lock = false;
			if (auto call = dynamic_cast<CallExpr *>(let->value)) {
				if (call->name == "int" || call->name == "float" ||
					 call->name == "bool" || call->name == "tuple" ||
					 call->name == "string" || call->name == "list" ||
					 call->name == "range" || call->name == "set") {
					lock = true;
				}
			}
			env->set(let->name, v, let->isLocked, let->isConst);
			return Value::None();
		}
		case StmtType::RETURN: {
			auto ret = static_cast<ReturnStmt *>(s);
			returning = true;
			returnValue =
				(ret->value != nullptr) ? eval(ret->value) : Value::NoType();
			return returnValue;
		}
		case StmtType::FUNC: {
			auto f = static_cast<FuncStmt *>(s);
			auto *func =
				new FunctionObject(f->params, f->returnType, f->defaultRetArgs,
					f->returnsConst, f->body, env, f->isCached);
			Value funcVal;
			funcVal.type = ValueType::FUNCTION;
			funcVal.ref = std::shared_ptr<HeapObject>(func);
			if (env->existsLocal(f->name)) {
				Var &existing = env->lookup(f->name);
				if (existing.value.type == ValueType::FUNCTION) {
					Value group = Value::Overload(existing.value);
					static_cast<OverloadObject *>(group.ref.get())
						->overloads.push_back(funcVal);
					existing.value = group;
				} else if (existing.value.type == ValueType::OVERLOAD) {
					static_cast<OverloadObject *>(existing.value.ref.get())
						->overloads.push_back(funcVal);
				} else {
					Var v;
					v.value = funcVal;
					v.isConst = f->returnsConst;
					env->vars[f->name] = v;
				}
			} else {
				Var v;
				v.value = funcVal;
				v.isConst = f->returnsConst;
				env->vars[f->name] = v;
			}
			return Value::None();
		}
		case StmtType::IF: {
			auto ifs = static_cast<IfStmt *>(s);
			if (eval(ifs->condition).isTruthy()) {
				for (auto st : ifs->body) {
					exec(st);
					if (returning || breaking || continuing)
						break;
				}
				return returnValue;
			}
			for (auto &[cond, block] : ifs->elifs) {
				if (eval(cond).isTruthy()) {
					for (auto st : block) {
						exec(st);
						if (returning || breaking || continuing)
							break;
					}
					return returnValue;
				}
			}
			for (auto st : ifs->elseBody) {
				exec(st);
				if (returning || breaking || continuing)
					break;
			}
			return returnValue;
		}
		case StmtType::EXPR: {
			auto es = static_cast<ExprStmt *>(s);
			eval(es->expr);
			return Value::None();
		}
		case StmtType::BREAK: {
			breaking = true;
			return Value::None();
		}
		case StmtType::CONTINUE: {
			continuing = true;
			return Value::None();
		}
		case StmtType::SKIP: {
			auto sk = static_cast<SkipStmt *>(s);
			skipCount = eval(sk->count).asInt();
			return Value::None();
		}
		case StmtType::WHILE: {
			auto w = static_cast<WhileStmt *>(s);
			std::shared_ptr<Env> loopEnv = std::make_shared<Env>();
			loopEnv->parent = env;
			std::shared_ptr<Env> previousGlobal = env;
			if (canOptimizeScope(w->body)) {
				env = loopEnv;
				while (eval(w->condition).isTruthy()) {
					for (auto st : w->body) {
						exec(st);
						if (breaking || continuing || returning)
							break;
					}
					if (breaking) {
						breaking = false;
						break;
					}
					if (continuing) {
						continuing = false;
					}
					if (returning)
						break;
				}
				env = previousGlobal;
			} else {
				while (eval(w->condition).isTruthy()) {
					env = loopEnv;
					for (auto st : w->body) {
						exec(st);
						if (breaking || continuing || returning)
							break;
					}
					env = previousGlobal;
					if (breaking) {
						breaking = false;
						break;
					}
					if (continuing) {
						continuing = false;
					}
					if (returning)
						break;
					loopEnv = std::make_shared<Env>();
					loopEnv->parent = previousGlobal;
				}
			}
			return Value::None();
		}
		case StmtType::DO_WHILE: {
			auto d = static_cast<DoWhileStmt *>(s);
			do {
				std::shared_ptr<Env> prevEnv = env;
				env = std::make_shared<Env>();
				env->parent = prevEnv;
				for (auto st : d->body) {
					exec(st);
					if (breaking || continuing || returning)
						break;
				}
				env = prevEnv;
				if (breaking) {
					breaking = false;
					return Value::None();
				}
				if (continuing) {
					continuing = false;
				}
				if (returning)
					return returnValue;
			} while (eval(d->condition).isTruthy());
			return Value::None();
		}
		case StmtType::FOR: {
			auto f = static_cast<ForStmt *>(s);
			std::shared_ptr<Env> loopEnv = std::make_shared<Env>();
			loopEnv->parent = env;
			std::shared_ptr<Env> previousGlobal = env;
			env = loopEnv;
			for (auto *st : f->inits)
				exec(st);
			bool staticScope = true;
			for (auto *st : f->body) {
				if (st->type == StmtType::LET ||
					 st->type == StmtType::MULTI_LET ||
					 st->type == StmtType::FUNC || st->type == StmtType::IMPORT) {
					staticScope = false;
					break;
				}
			}
			Value *fastCounter = nullptr;
			if (f->steps.size() == 1 && f->steps[0]->type == StmtType::ASSIGN) {
				auto as = static_cast<AssignStmt *>(f->steps[0]);
				try {
					LValue lv = resolveLValue(as->target);
					if (lv.ref->type == ValueType::INT ||
						 lv.ref->type == ValueType::FLOAT)
						fastCounter = lv.ref;
				} catch (...) {
				}
			}
			double fastLimit = 0;
			TokenType fastOp = TokenType::LET;
			bool useFastCondition = false;
			if (fastCounter && f->condition->type == ExprType::BINARY) {
				auto be = static_cast<BinExpr *>(f->condition);
				if (be->left->type == ExprType::VAR &&
					 be->right->type == ExprType::NUMBER) {
					auto ve = static_cast<VarExpr *>(be->left);
					if (static_cast<AssignStmt *>(f->steps[0])->target->type ==
						 ExprType::VAR) {
						auto stepVar = static_cast<VarExpr *>(
							static_cast<AssignStmt *>(f->steps[0])->target);
						if (stepVar->name == ve->name) {
							Value limitVal = eval(be->right);
							fastLimit = limitVal.asFloat();
							fastOp = be->op;
							useFastCondition = true;
						}
					}
				}
			}
			Value *fastBodyTarget = nullptr;
			Value *fastBodyOperand = nullptr;
			bool fastBodyIsNumber = false;
			double fastBodyLiteral = 0;
			TokenType fastBodyOp = TokenType::LET;
			bool useFastBody = false;
			if (staticScope && f->body.size() == 1 &&
				 f->body[0]->type == StmtType::ASSIGN) {
				auto as = static_cast<AssignStmt *>(f->body[0]);
				if (as->op == TokenType::PLUS_EQ ||
					 as->op == TokenType::MINUS_EQ ||
					 as->op == TokenType::STAR_EQ ||
					 as->op == TokenType::DIV_EQ) {
					try {
						LValue lv = resolveLValue(as->target);
						if (lv.ref->type == ValueType::INT ||
							 lv.ref->type == ValueType::FLOAT) {
							fastBodyTarget = lv.ref;
							fastBodyOp = as->op;
							if (as->value->type == ExprType::VAR) {
								LValue opLv = resolveLValue(as->value);
								if (opLv.ref->type == ValueType::INT ||
									 opLv.ref->type == ValueType::FLOAT) {
									fastBodyOperand = opLv.ref;
									if (fastBodyTarget->type ==
										 fastBodyOperand->type) {
										useFastBody = true;
									}
								}
							} else if (as->value->type == ExprType::NUMBER) {
								fastBodyLiteral =
									static_cast<NumberExpr *>(as->value)->val;
								fastBodyIsNumber = true;
								useFastBody = true;
							}
						}
					} catch (...) {
					}
				}
			}
			if (staticScope) {
				while (true) {
					if (useFastCondition) {
						double current = (fastCounter->type == ValueType::INT)
												  ? (double)fastCounter->iVal
												  : fastCounter->fVal;
						bool cond = false;
						switch (fastOp) {
						case TokenType::LT:
							cond = current < fastLimit;
							break;
						case TokenType::LTE:
							cond = current <= fastLimit;
							break;
						case TokenType::GT:
							cond = current > fastLimit;
							break;
						case TokenType::GTE:
							cond = current >= fastLimit;
							break;
						default:
							cond = eval(f->condition).isTruthy();
							break;
						}
						if (!cond)
							break;
					} else {
						if (!eval(f->condition).isTruthy())
							break;
					}
					if (useFastBody) {
						double operand = 0;
						if (fastBodyIsNumber)
							operand = fastBodyLiteral;
						else
							operand = (fastBodyOperand->type == ValueType::INT)
											 ? (double)fastBodyOperand->iVal
											 : fastBodyOperand->fVal;
						if (fastBodyTarget->type == ValueType::INT) {
							long long opInt = (long long)operand;
							switch (fastBodyOp) {
							case TokenType::PLUS_EQ:
								fastBodyTarget->iVal += opInt;
								break;
							case TokenType::MINUS_EQ:
								fastBodyTarget->iVal -= opInt;
								break;
							case TokenType::STAR_EQ:
								fastBodyTarget->iVal *= opInt;
								break;
							case TokenType::DIV_EQ:
								if (opInt != 0)
									fastBodyTarget->iVal /= opInt;
								break;
							}
						} else {
							switch (fastBodyOp) {
							case TokenType::PLUS_EQ:
								fastBodyTarget->fVal += operand;
								break;
							case TokenType::MINUS_EQ:
								fastBodyTarget->fVal -= operand;
								break;
							case TokenType::STAR_EQ:
								fastBodyTarget->fVal *= operand;
								break;
							case TokenType::DIV_EQ:
								if (operand != 0)
									fastBodyTarget->fVal /= operand;
								break;
							}
						}
					} else {
						for (auto st : f->body) {
							exec(st);
							if (breaking || continuing || returning)
								break;
						}
						if (breaking) {
							breaking = false;
							break;
						}
						if (continuing) {
							continuing = false;
						}
						if (returning)
							break;
					}
					if (fastCounter) {
						if (fastCounter->type == ValueType::INT)
							fastCounter->iVal += 1;
						else
							fastCounter->fVal += 1.0;
					} else {
						for (auto *st : f->steps)
							exec(st);
					}
				}
			} else {
				env = previousGlobal;
				while (eval(f->condition).isTruthy()) {
					env = loopEnv;
					for (auto st : f->body)
						exec(st);
					env = previousGlobal;
					if (breaking) {
						breaking = false;
						break;
					}
					if (continuing) {
						continuing = false;
					}
					if (returning)
						break;
					env = loopEnv;
					for (auto *st : f->steps)
						exec(st);
					env = previousGlobal;
				}
			}
			env = previousGlobal;
			if (returning)
				return returnValue;
			return Value::None();
		}
		case StmtType::FOR_EACH: {
			auto fe = static_cast<ForEachStmt *>(s);
			vector<vector<Value>> allStreams;
			size_t minLen = (size_t)-1;
			bool dictUnpack =
				fe->collections.size() == 1 && fe->loopVars.size() == 2;
			for (size_t i = 0; i < fe->collections.size(); i++) {
				Value collection = eval(fe->collections[i]);
				vector<Value> currentStream;
				vector<Value> secondaryStream;
				if (collection.type == ValueType::DICT) {
					auto *dict = static_cast<DictObject *>(collection.ref.get());
					if (dictUnpack) {
						for (auto &pair : dict->items) {
							currentStream.push_back(pair.first);
							secondaryStream.push_back(pair.second);
						}
					} else {
						for (auto &pair : dict->items)
							currentStream.push_back(pair.first);
					}
				} else if (collection.type == ValueType::LIST) {
					auto *list = static_cast<ListObject *>(collection.ref.get());
					for (auto &v : list->elements)
						currentStream.push_back(v);
				} else if (collection.type == ValueType::SET) {
					auto *set = static_cast<SetObject *>(collection.ref.get());
					for (auto &v : set->elements)
						currentStream.push_back(v);
				} else if (collection.type == ValueType::TUPLE) {
					auto *tuple = static_cast<TupleObject *>(collection.ref.get());
					for (auto &v : tuple->elements)
						currentStream.push_back(v);
				} else if (collection.type == ValueType::STRING) {
					string str = collection.asString();
					for (char c : str)
						currentStream.push_back(Value::String(string(1, c)));
				} else if (collection.type == ValueType::RANGE) {
					auto *r = static_cast<RangeObject *>(collection.ref.get());
					double cur = r->start;
					if (!r->startInclusive)
						cur += r->step;
					while (true) {
						bool keep =
							(r->step > 0)
								? (r->endInclusive ? cur <= r->end : cur < r->end)
								: (r->endInclusive ? cur >= r->end : cur > r->end);
						if (!keep)
							break;
						currentStream.push_back(r->isFloat
															? Value::Float(cur)
															: Value::Int((long long)cur));
						cur += r->step;
					}
				} else if (collection.type == ValueType::VECTOR) {
					auto *v = static_cast<VectorObject *>(collection.ref.get());
					for (auto d : v->elements)
						currentStream.push_back(d);
				} else {
					throw TypeError("Object is not iterable", s->line, s->col);
				}
				if (currentStream.size() < minLen)
					minLen = currentStream.size();
				allStreams.push_back(currentStream);
				if (dictUnpack && !secondaryStream.empty()) {
					if (secondaryStream.size() < minLen)
						minLen = secondaryStream.size();
					allStreams.push_back(secondaryStream);
				}
			}
			if (minLen == (size_t)-1)
				minLen = 0;
			if (allStreams.size() != fe->loopVars.size()) {
				throw RuntimeError(
					"Mismatch between loop variables and collections", s->line,
					s->col);
			}
			std::shared_ptr<Env> prevEnv = env;
			for (size_t i = 0; i < minLen; i++) {
				if (skipCount > 0) {
					skipCount--;
					continue;
				}
				env = std::make_shared<Env>();
				env->parent = prevEnv;
				for (size_t v = 0; v < fe->loopVars.size(); v++) {
					if (i < allStreams[v].size()) {
						env->set(fe->loopVars[v], allStreams[v][i], false, false);
					}
				}
				for (auto stmt : fe->body) {
					exec(stmt);
					if (breaking || returning || continuing)
						break;
				}
				env = prevEnv;
				if (breaking) {
					breaking = false;
					break;
				}
				if (returning)
					break;
				if (continuing)
					continuing = false;
			}
			if (returning)
				return returnValue;
			return Value::None();
		}
		case StmtType::TRY: {
			auto t = static_cast<TryStmt *>(s);
			bool errorCaught = false;
			try {
				for (auto st : t->tryBlock) {
					exec(st);
					if (returning || breaking || continuing)
						break;
				}
			} catch (const LangError &e) {
				if (dynamic_cast<const ControlFlowError *>(&e)) {
					if (!t->finallyBlock.empty()) {
						bool r = returning;
						Value rv = returnValue;
						returning = false;
						try {
							for (auto st : t->finallyBlock)
								exec(st);
						} catch (...) {
							throw;
						}
						if (!returning && !breaking && !continuing) {
							returning = r;
							returnValue = rv;
						}
					}
					throw;
				}
				errorCaught = true;
				bool handled = false;
				for (auto &catchBlock : t->catches) {
					for (auto &typeName : catchBlock.types) {
						if (matchesError(e, typeName)) {
							try {
								for (auto st : catchBlock.body) {
									exec(st);
									if (returning || breaking || continuing)
										break;
								}
							} catch (...) {
								if (!t->finallyBlock.empty())
									for (auto st : t->finallyBlock)
										exec(st);
								throw;
							}
							handled = true;
							goto after_catches;
						}
					}
				}
			after_catches:
				if (!handled) {
					if (!t->finallyBlock.empty())
						for (auto st : t->finallyBlock)
							exec(st);
					throw;
				}
			}
			if (!errorCaught && !returning && !breaking && !continuing &&
				 !t->elseBlock.empty()) {
				for (auto st : t->elseBlock) {
					exec(st);
					if (returning || breaking || continuing)
						break;
				}
			}
			if (!t->finallyBlock.empty()) {
				bool oldRet = returning;
				Value oldVal = returnValue;
				bool oldBrk = breaking;
				bool oldCont = continuing;
				returning = false;
				breaking = false;
				continuing = false;
				try {
					for (auto st : t->finallyBlock) {
						exec(st);
						if (returning || breaking || continuing)
							break;
					}
				} catch (...) {
					throw;
				}
				if (!returning && !breaking && !continuing) {
					returning = oldRet;
					returnValue = oldVal;
					breaking = oldBrk;
					continuing = oldCont;
				}
			}
			return Value::None();
		}
		case StmtType::THROW: {
			auto ts = static_cast<ThrowStmt *>(s);
			Value msgVal = eval(ts->message);
			throw LangError(ts->errorType, valueToString(msgVal), -1000000,
				s->line, s->col);
		}
		case StmtType::ASSERT: {
			auto a = static_cast<AssertStmt *>(s);
			Value cond = eval(a->condition);
			if (!cond.isTruthy()) {
				string msg = "Assertion failed";
				if (a->message) {
					Value m = eval(a->message);
					msg += ": " + valueToString(m);
				}
				throw RuntimeError(msg, s->line, s->col);
			}
			return Value::None();
		}
		case StmtType::SWITCH: {
			auto sw = dynamic_cast<SwitchStmt *>(s);
			while (true) {
				Value targetVal = eval(sw->target);
				bool matchFound = false;
				for (auto &c : sw->cases) {
					Value caseVal = eval(c.value);
					if (targetVal.strictEquals(caseVal)) {
						matchFound = true;
						for (auto st : c.body) {
							exec(st);
							if (returning)
								return returnValue;
							if (continuing)
								goto exit_switch;
							if (breaking) {
								breaking = false;
								goto exit_switch;
							}
						}
						goto restart_loop;
					}
				}
				if (!matchFound) {
					for (auto st : sw->defaultBody) {
						exec(st);
						if (returning)
							return returnValue;
						if (continuing)
							goto exit_switch;
						if (breaking) {
							breaking = false;
							goto exit_switch;
						}
					}
					goto restart_loop;
				}
			restart_loop:;
			}
		exit_switch:
			return Value::None();
		}
		case StmtType::IMPORT: {
			auto imp = static_cast<ImportStmt *>(s);
			if (imp->libName.size() > 4 &&
				 imp->libName.substr(imp->libName.size() - 4) == ".ymm") {
				fs::path p(imp->libName);
				if (!fs::exists(p)) {
					throw InvalidImportError(
						"Module file not found: " + imp->libName, s->line,
						s->col);
				}
				std::error_code ec;
				string absPath = fs::absolute(p, ec).string();
				if (importStack.count(absPath)) {
					throw CircularImportError(
						"Circular import detected involving: " + imp->libName,
						s->line, s->col);
				}
				std::ifstream file(absPath);
				if (!file)
					throw InvalidImportError(
						"Unable to read module: " + imp->libName, s->line,
						s->col);
				std::stringstream buffer;
				buffer << file.rdbuf();
				string code = buffer.str();
				std::shared_ptr<Env> previousEnv = this->env;
				std::shared_ptr<Env> moduleEnv = std::make_shared<Env>();
				moduleEnv->set("None", Value::None(), true);
				importStack.insert(absPath);
				this->env = moduleEnv;
				try {
					auto tokens = tokenize(code);
					Parser parser(tokens);
					vector<Stmt *> moduleStmts;
					while (!parser.isAtEnd())
						moduleStmts.push_back(parser.parseStmt());
					for (Stmt *stmt : moduleStmts) {
						if (!isGlobalAllowed(stmt))
							throw RuntimeError(
								"Module code must only contain definitions (let, "
								"func, "
								"import).",
								stmt->line, stmt->col);
					}
					for (Stmt *ms : moduleStmts)
						exec(ms);
				} catch (...) {
					this->env = previousEnv;
					importStack.erase(absPath);
					throw;
				}
				this->env = previousEnv;
				importStack.erase(absPath);
				if (imp->symbols.empty()) {
					for (auto &[name, var] : moduleEnv->vars) {
						if (name == "None")
							continue;
						this->env->vars[name] = var;
					}
				} else {
					for (const string &sym : imp->symbols) {
						if (moduleEnv->vars.count(sym)) {
							this->env->vars[sym] = moduleEnv->vars[sym];
						} else {
							throw AttributeError("Module '" + imp->libName +
															"' does not export '" + sym +
															"'",
								s->line, s->col);
						}
					}
				}
				return Value::None();
			}
			if (modules.find(imp->libName) == modules.end()) {
				if (imp->libName.find('.') != string::npos) {
					throw InvalidImportError("Import filenames must end in .ymm",
						s->line, s->col);
				}
				throw ImportError("Unknown module '" + imp->libName + "'",
					s->line, s->col);
			}
			modules[imp->libName](this->env, imp->symbols);
			return Value::None();
		}
		case StmtType::MULTI_LET: {
			auto ml = static_cast<MultiLetStmt *>(s);
			vector<Value> results;
			for (auto *expr : ml->values)
				results.push_back(expr ? eval(expr) : Value::NoType());
			for (size_t i = 0; i < ml->names.size(); i++) {
				if (env->existsLocal(ml->names[i])) {
					throw RuntimeError("Variable '" + ml->names[i] +
												 "' is already defined in this scope.",
						s->line, s->col);
				}
				Value v = results[i];
				if (ml->isConsts[i])
					v.isConst = true;
				bool lock = false;
				env->set(ml->names[i], v, ml->isLocked, ml->isConsts[i]);
			}
			return Value::None();
		}
		case StmtType::MULTI_ASSIGN: {
			auto ma = static_cast<MultiAssignStmt *>(s);
			vector<LValue> lvals;
			for (auto *t : ma->targets)
				lvals.push_back(resolveLValue(t));
			vector<Value> rvals;
			for (auto *v : ma->values)
				rvals.push_back(eval(v));
			for (size_t i = 0; i < lvals.size(); i++) {
				Value &cur = *lvals[i].ref;
				if (cur.isConst || lvals[i].isConstView)
					throw ConstError("Cannot assign to const variable", s->line,
						s->col);
				if (cur.ref && cur.ref->typeLocked &&
					 cur.type != rvals[i].type) {
					throw TypeError("Cannot change type of locked variable",
						s->line, s->col);
				}
				if (lvals[i].isRefTarget)
					*lvals[i].ref = rvals[i];
				else if (cur.ref && cur.type == rvals[i].type) {
					if (cur.type == ValueType::STRING)
						static_cast<StringObject *>(cur.ref.get())->value =
							rvals[i].asString();
					else
						cur = rvals[i];
				} else
					cur = rvals[i];
			}
			return Value::None();
		}
		default: {
			return Value::None();
		}
		}
		return Value::None();
	}
	Value call(Value callee, const vector<CallArg> &args,
		const vector<CopyMode> &modes, int line = 0, int col = 0) {
		auto error = [&](const string &msg, const string &type = "RuntimeError") {
			if (type == "TypeError")
				throw TypeError(msg, line, col);
			if (type == "ValueError")
				throw ValueError(msg, line, col);
			if (type == "ArgumentError")
				throw ArgumentError(msg, line, col);
			if (type == "NameError")
				throw NameError(msg, line, col);
			throw RuntimeError(msg, line, col);
		};
		// ------------------ RESOLVE CALLEE OBJECT ------------------
		FunctionObject *lambdaObj = nullptr;
		NativeFunctionObject *nativeObj = nullptr;
		if (callee.type == ValueType::FUNCTION) {
			lambdaObj = static_cast<FunctionObject *>(callee.ref.get());
		} else if (callee.type == ValueType::NATIVE_FUNCTION) {
			nativeObj = static_cast<NativeFunctionObject *>(callee.ref.get());
		} else if (callee.type == ValueType::OVERLOAD) {
			auto *ov = static_cast<OverloadObject *>(callee.ref.get());
			bool found = false;
			for (int i = ov->overloads.size() - 1; i >= 0; i--) {
				Value candVal = ov->overloads[i];
				if (candVal.type != ValueType::FUNCTION)
					continue;
				auto *candidate = static_cast<FunctionObject *>(candVal.ref.get());
				size_t minArgs = 0;
				bool isVariadic = false;
				for (const auto &p : candidate->params) {
					if (p.isVariadic || p.isKwargs)
						isVariadic = true;
					else if (p.defaultValue == nullptr)
						minArgs++;
				}
				if (args.size() < minArgs && !isVariadic)
					continue;
				if (!isVariadic && args.size() > candidate->params.size())
					continue;
				bool typesMatch = true;
				size_t paramIdx = 0;
				for (size_t argIdx = 0; argIdx < args.size(); argIdx++) {
					if (paramIdx >= candidate->params.size()) {
						if (!isVariadic) {
							typesMatch = false;
							break;
						}
						break;
					}
					const ParamSpec &p = candidate->params[paramIdx];
					if (p.isVariadic || p.isKwargs)
						break;
					Value argVal = args[argIdx].value;
					if (p.type != ValueType::NOTYPE && argVal.type != p.type) {
						if (!(p.type == ValueType::FLOAT &&
								 argVal.type == ValueType::INT)) {
							typesMatch = false;
							break;
						}
					}
					paramIdx++;
				}
				if (typesMatch) {
					lambdaObj = candidate;
					found = true;
					break;
				}
			}
			if (!found)
				error("No matching overload found with provided arguments",
					"TypeError");
		} else {
			error("Object is not callable", "TypeError");
		}
		if (nativeObj) {
			vector<Value> argValues;
			for (const auto &ca : args)
				argValues.push_back(ca.value);
			return nativeObj->func(argValues, line, col);
		}
		if (!lambdaObj)
			error("Critical: Lambda Object is null", "RuntimeError");
		vector<Value> cacheKey;
		if (lambdaObj->isCached) {
			for (const auto &arg : args)
				cacheKey.push_back(arg.value);
			if (lambdaObj->cache.count(cacheKey))
				return lambdaObj->cache[cacheKey];
		}
		std::shared_ptr<Env> previous = env;
		std::shared_ptr<Env> newEnv = std::make_shared<Env>();
		newEnv->parent = lambdaObj->closure ? lambdaObj->closure : previous;
		env = newEnv;
		size_t argIndex = 0;
		for (size_t i = 0; i < lambdaObj->params.size(); i++) {
			const ParamSpec &p = lambdaObj->params[i];
			Value argVal = Value::None();
			if (p.isKwargs) {
				std::unordered_map<Value, Value, ValueHash, ValueEqual> dictItems;
				while (argIndex < args.size()) {
					Value v = args[argIndex].value;
					if (v.type != ValueType::PAIRED)
						throw RuntimeError(
							"Positional argument found after keyword arguments.",
							line, col);
					auto *pairObj = static_cast<PairedObject *>(v.ref.get());
					for (const auto &entry : pairObj->pairs) {
						Value key = entry.first;
						Value val = entry.second;
						if (p.mode == CopyMode::DEEP) {
							if (key.type == ValueType::LIST ||
								 key.type == ValueType::DICT)
								key = deepCopy(key);
							val = deepCopy(val);
						}
						dictItems[key] = val;
					}
					argIndex++;
				}
				Value dictVal = Value::Dict(dictItems, p.isConst);
				env->set(p.name, dictVal, false, p.isConst);
			} else if (p.isVariadic) {
				vector<Value> tupleItems;
				while (argIndex < args.size()) {
					if (args[argIndex].value.type == ValueType::PAIRED)
						break;
					Value v = args[argIndex].value;
					if (p.mode == CopyMode::DEEP)
						v = deepCopy(v);
					tupleItems.push_back(v);
					argIndex++;
				}
				env->set(p.name, Value::Tuple(tupleItems), false, true);
			} else {
				bool foundValue = false;
				if (argIndex < args.size() &&
					 args[argIndex].value.type != ValueType::PAIRED) {
					if (args[argIndex].value.type == ValueType::OMIT_MARKER) {
						if (p.defaultValue != nullptr) {
							argVal = eval(p.defaultValue);
							foundValue = true;
						} else {
							env = previous;
							error("Argument '" + p.name +
										"' cannot be omitted (no default value).",
								"ArgumentError");
						}
					} else {
						argVal = args[argIndex].value;
						if (p.mode == CopyMode::DEEP)
							argVal = deepCopy(argVal);
						foundValue = true;
					}
					argIndex++;
				}
				if (!foundValue) {
					if (p.defaultValue != nullptr)
						argVal = eval(p.defaultValue);
					else {
						env = previous;
						error("Missing required argument '" + p.name + "'",
							"ArgumentError");
					}
				}
				if (p.type != ValueType::NOTYPE && argVal.type != p.type) {
					if (p.type == ValueType::FLOAT && argVal.type == ValueType::INT)
						argVal = Value::Float((double)argVal.asInt());
					else {
						env = previous;
						error("Type mismatch for '" + p.name + "'", "TypeError");
					}
				}
				env->set(p.name, argVal, false, p.isConst);
			}
		}
		bool prevReturning = returning;
		Value prevReturnValue = returnValue;
		returning = false;
		for (auto stmt : lambdaObj->body) {
			exec(stmt);
			if (returning)
				break;
		}
		bool didReturn = returning;
		Value ret = returnValue;
		returning = prevReturning;
		returnValue = prevReturnValue;
		env = previous;
		auto generateDefault = [&]() -> Value {
			vector<CallArg> defArgs;
			vector<CopyMode> defModes;
			for (auto *e : lambdaObj->defaultRetArgs) {
				defArgs.push_back({eval(e)});
				defModes.push_back(CopyMode::SHALLOW);
			}
			string typeName = "";
			switch (lambdaObj->returnType) {
			case ValueType::INT:
				typeName = "int";
				break;
			case ValueType::FLOAT:
				typeName = "float";
				break;
			case ValueType::STRING:
				typeName = "string";
				break;
			case ValueType::BOOL:
				typeName = "bool";
				break;
			case ValueType::LIST:
				typeName = "list";
				break;
			case ValueType::DICT:
				typeName = "dict";
				break;
			case ValueType::SET:
				typeName = "set";
				break;
			case ValueType::TUPLE:
				typeName = "tuple";
				break;
			case ValueType::RANGE:
				typeName = "range";
				break;
			case ValueType::VECTOR:
				typeName = "vector";
				break;
			default:
				break;
			}
			if (!typeName.empty()) {
				if (env->exists(typeName)) {
					return call(env->get(typeName), defArgs, defModes, line, col);
				}
			}
			return Value::None();
		};
		Value finalResult;
		if (didReturn) {
			if (ret.type == ValueType::NOTYPE) {
				if (!lambdaObj->defaultRetArgs.empty())
					finalResult = generateDefault();
				else if (lambdaObj->returnType != ValueType::NOTYPE)
					finalResult = defaultOf(lambdaObj->returnType);
				else
					finalResult = Value::NoType();
			} else if (lambdaObj->returnType != ValueType::NOTYPE &&
						  ret.type != lambdaObj->returnType) {
				if (lambdaObj->returnType == ValueType::FLOAT &&
					 ret.type == ValueType::INT)
					finalResult = Value::Float((double)ret.asInt());
				else if (lambdaObj->returnType == ValueType::INT &&
							ret.type == ValueType::FLOAT)
					finalResult = Value::Int((long long)ret.fVal);
				else
					error("Return type mismatch", "TypeError");
			} else
				finalResult = ret;
		} else {
			if (!lambdaObj->defaultRetArgs.empty())
				finalResult = generateDefault();
			else
				finalResult = (lambdaObj->returnType != ValueType::NOTYPE)
									  ? defaultOf(lambdaObj->returnType)
									  : Value::NoType();
		}
		if (lambdaObj->returnsConst)
			finalResult.isConst = true;
		if (lambdaObj->isCached)
			lambdaObj->cache[cacheKey] = finalResult;
		return finalResult;
	}
	void interpret(const vector<Stmt *> &statements) {
		try {
			for (Stmt *stmt : statements) {
				if (!isGlobalAllowed(stmt))
					throw RuntimeError(
						"Executable code must be inside a 'main' function.",
						stmt->line, stmt->col);
			}

			for (Stmt *stmt : statements)
				exec(stmt);
			if (env->exists("main")) {
				Value mainVal = env->get("main");
				if (mainVal.type != ValueType::FUNCTION)
					throw RuntimeError("'main' is defined but is not a function.", 0,
						0);
				auto *func = static_cast<FunctionObject *>(mainVal.ref.get());
				if (func->params.size() > 0) {
					throw RuntimeError("'main' function cannot accept arguments.", 0,
						0);
				}
				std::shared_ptr<Env> globalEnv = this->env;
				this->env = std::make_shared<Env>();
				this->env->parent = func->closure;
				try {
					// Execute Main Body
					for (Stmt *bodyStmt : func->body) {
						exec(bodyStmt);
						if (returning) {
							returning = false;
							break;
						}
					}
				} catch (...) {
					this->env = globalEnv;
					throw;
				}
				this->env = globalEnv;
			} else
				throw RuntimeError(
					"No entry point found. Define a 'main' function.", 0, 0);
		} catch (...) {
			throw;
		}
	}

 private:
	bool canOptimizeScope(const vector<Stmt *> &body) {
		for (auto *st : body) {
			if (st->type == StmtType::LET || st->type == StmtType::MULTI_LET ||
				 st->type == StmtType::FUNC || st->type == StmtType::IMPORT ||
				 st->type == StmtType::MULTI_ASSIGN) {
				return false;
			}
		}
		return true;
	}
	bool isGlobalAllowed(Stmt *s) {
		if (dynamic_cast<LetStmt *>(s))
			return true;
		if (dynamic_cast<FuncStmt *>(s))
			return true;
		if (dynamic_cast<ImportStmt *>(s))
			return true;
		if (dynamic_cast<MultiLetStmt *>(s))
			return true;
		if (dynamic_cast<MultiAssignStmt *>(s))
			return true;
		return false;
	}
};
// ---------------- JIT ----------------

// ------------ BYTECODE VM ------------
struct ByteCodeCompiler {
	Chunk *chunk;
	vector<LoopContext> loopStack;
	vector<Local> locals;
	int scopeDepth = 0;
	ByteCodeCompiler(Chunk *c) : chunk(c) {}
	int resolveLocal(string name) {
		for (int i = (int)locals.size() - 1; i >= 0; i--)
			if (locals[i].name == name)
				return i;
		return -1;
	}
	void addLocal(string name) { locals.push_back({name, scopeDepth}); }
	void beginScope() { scopeDepth++; }
	void endScope(int line, int col) {
		scopeDepth--;
		while (!locals.empty() && locals.back().depth > scopeDepth) {
			emitByte(OpCode::OP_POP, line, col);
			locals.pop_back();
		}
	}
	void compile(Expr *e) {
		if (!e)
			return;
		switch (e->type) {
		case ExprType::GET: {
			auto g = static_cast<GetExpr *>(e);
			compile(g->object);
			emitIdentifier(OpCode::OP_GET_PROPERTY, g->name, g->line, g->col);
			break;
		}
		case ExprType::COMPREHENSION: {
			auto comp = static_cast<CompExpr *>(e);
			beginScope();
			bool isDict = (comp->typeToken == TokenType::LBRACE &&
								comp->valueExpr != nullptr);
			bool isSet = (comp->typeToken == TokenType::LBRACE && !isDict);
			bool isTuple = (comp->typeToken == TokenType::LPAREN);
			bool isVector = (comp->typeToken == TokenType::LT);
			if (isDict) {
				emitByte(OpCode::OP_BUILD_DICT, comp->line, comp->col);
				chunk->write(0, comp->line, comp->col);
			} else if (isSet) {
				emitByte(OpCode::OP_BUILD_SET, comp->line, comp->col);
				chunk->write(0, comp->line, comp->col);
			} else {
				emitByte(OpCode::OP_BUILD_LIST, comp->line, comp->col);
				chunk->write(0, comp->line, comp->col);
			}
			int containerSlot = (int)locals.size();
			addLocal("");
			compile(comp->iterable);
			emitByte(OpCode::OP_TO_STREAM, comp->line, comp->col);
			addLocal("");
			emitConstant(Value::Int(0), comp->line, comp->col);
			addLocal("");
			int startAddr = (int)chunk->code.size();
			emitByte(OpCode::OP_FOR_ITER, comp->line, comp->col);
			int exitJump = (int)chunk->code.size();
			chunk->write(0xff, comp->line, comp->col);
			chunk->write(0xff, comp->line, comp->col);
			chunk->write(1, comp->line, comp->col);
			beginScope();
			addLocal(comp->varName);
			int filterJump = -1;
			if (comp->filter) {
				compile(comp->filter);
				filterJump =
					emitJump(OpCode::OP_JUMP_IF_FALSE, comp->line, comp->col);
				emitByte(OpCode::OP_POP, comp->line, comp->col);
			}
			if (isDict) {
				compile(comp->expression);
				compile(comp->valueExpr);
				emitByte(OpCode::OP_DICT_SET, comp->line, comp->col);
				chunk->write((uint8_t)containerSlot, comp->line, comp->col);
			} else if (isSet) {
				compile(comp->expression);
				emitByte(OpCode::OP_SET_ADD, comp->line, comp->col);
				chunk->write((uint8_t)containerSlot, comp->line, comp->col);
			} else {
				compile(comp->expression);
				emitByte(OpCode::OP_LIST_APPEND, comp->line, comp->col);
				chunk->write((uint8_t)containerSlot, comp->line, comp->col);
			}
			if (filterJump != -1) {
				int skipAppend =
					emitJump(OpCode::OP_JUMP, comp->line, comp->col);
				patchJump(filterJump);
				emitByte(OpCode::OP_POP, comp->line, comp->col);
				patchJump(skipAppend);
			}
			endScope(comp->line, comp->col);
			emitLoop(startAddr, comp->line, comp->col);
			patchJump(exitJump);
			emitByte(OpCode::OP_POP, comp->line, comp->col);
			emitByte(OpCode::OP_POP, comp->line, comp->col);
			// emitByte(OpCode::OP_GET_LOCAL, comp->line, comp->col);
			// chunk->write((uint8_t)containerSlot, comp->line, comp->col);
			if (isTuple)
				emitByte(OpCode::OP_LIST_TO_TUPLE, comp->line, comp->col);
			else if (isVector)
				emitByte(OpCode::OP_LIST_TO_VECTOR, comp->line, comp->col);
			locals.pop_back();
			locals.pop_back();
			locals.pop_back();
			scopeDepth--;
			break;
		}
		case ExprType::OMIT_MARKER_EXPR: {
			auto o = static_cast<OmitExpr *>(e);
			Value val = Value::Omit();
			emitConstant(val, o->line, o->col);
			break;
		}
		case ExprType::LAMBDA: {
			auto lam = static_cast<LambdaExpr *>(e);
			Chunk *funcChunk = new Chunk();
			ByteCodeCompiler subCompiler(funcChunk);
			subCompiler.beginScope();
			for (const auto &param : lam->params)
				subCompiler.addLocal(param.name);
			for (auto *stmt : lam->body)
				subCompiler.compileStmt(stmt);
			subCompiler.emitByte(OpCode::OP_NOTYPE, lam->line, 0);
			subCompiler.emitByte(OpCode::OP_RETURN, lam->line, 0);
			auto *funcObj = new FunctionObject(
				lam->params, lam->returnType, lam->defaultRetArgs,
				lam->returnsConst, lam->body, nullptr, lam->isCached,
				funcChunk);
			Value funcVal;
			funcVal.type = ValueType::FUNCTION;
			funcVal.ref = std::shared_ptr<HeapObject>(funcObj);
			emitConstant(funcVal, lam->line, 0);
			break;
		}
		case ExprType::NUMBER: {
			auto n = static_cast<NumberExpr *>(e);
			Value val = n->isFloat ? Value::Float(n->val)
										  : Value::Int((long long)n->val);
			emitConstant(val, n->line, n->col);
			break;
		}
		case ExprType::BOOL: {
			auto b = static_cast<BoolExpr *>(e);
			emitByte(b->value ? OpCode::OP_TRUE : OpCode::OP_FALSE, b->line,
				b->col);
			break;
		}
		case ExprType::STRING: {
			auto s = static_cast<StringExpr *>(e);
			emitConstant(Value::String(s->val), s->line, s->col);
			break;
		}
		case ExprType::FSTRING: {
			auto fs = static_cast<FStringExpr *>(e);
			for (auto *part : fs->parts)
				compile(part);
			if (fs->parts.size() > 255)
				throw SyntaxError("F-String has too many parts (limit 255)",
					fs->line, fs->col);
			emitByte(OpCode::OP_BUILD_FSTRING, fs->line, fs->col);
			chunk->write((uint8_t)fs->parts.size(), fs->line, fs->col);
			break;
		}
		case ExprType::BINARY: {
			auto b = static_cast<BinExpr *>(e);
			if (b->op == TokenType::AND || b->op == TokenType::OR || b->op == TokenType::NAND || b->op == TokenType::NOR)
				compileLogical(b);
			else
				compileBinary(b);
			break;
		}
		case ExprType::VAR: {
			auto v = static_cast<VarExpr *>(e);
			if (v->name == "None") {
				emitByte(OpCode::OP_NONE, v->line, v->col);
				break;
			}
			int arg = resolveLocal(v->name);
			if (arg != -1) {
				if (locals[arg].hasKnownValue) {
					emitConstant(locals[arg].knownValue, v->line, v->col);
					break; // Bypass OP_GET_LOCAL entirely!
				}
				emitByte(OpCode::OP_GET_LOCAL, v->line, v->col);
				chunk->write((uint8_t)arg, v->line, v->col);
			} else
				emitIdentifier(OpCode::OP_GET_VAR, v->name, v->line, v->col);
			break;
		}
		case ExprType::OWNERSHIP: {
			auto o = static_cast<OwnershipExpr *>(e);
			if (o->mode == CopyMode::DEEP) {
				compile(o->expr);
				emitByte(OpCode::OP_DEEP_COPY, o->line, o->col);
			} else if (o->mode == CopyMode::REF) {
				if (auto v = dynamic_cast<VarExpr *>(o->expr)) {
					int local = resolveLocal(v->name);
					if (local != -1) {
						emitByte(OpCode::OP_REF_LOCAL, o->line,
							o->col); // New OpCode
						chunk->write((uint8_t)local, o->line, o->col);
					} else {
						emitIdentifier(OpCode::OP_REF_VAR, v->name, o->line,
							o->col); // New usage
					}
				} else if (auto idx = dynamic_cast<IndexExpr *>(o->expr)) {
					compile(idx->base);
					compile(idx->index);
					emitByte(OpCode::OP_REF_INDEX, o->line, o->col);
				} else if (auto get = dynamic_cast<GetExpr *>(o->expr)) {
					compile(get->object);
					emitIdentifier(OpCode::OP_REF_PROPERTY, get->name, o->line,
						o->col);
				} else {
					throw OwnershipError(
						"Cannot take reference of this expression", o->line,
						o->col);
				}
			}
			break;
		}
		case ExprType::INDEX: {
			auto idx = static_cast<IndexExpr *>(e);
			compile(idx->base);
			compile(idx->index);
			emitByte(OpCode::OP_GET_INDEX, idx->line, idx->col);
			break;
		}
		case ExprType::SLICE: {
			auto s = static_cast<SliceExpr *>(e);
			if (s->start)
				compile(s->start);
			else
				emitByte(OpCode::OP_NONE, s->line, s->col);
			if (s->end)
				compile(s->end);
			else
				emitByte(OpCode::OP_NONE, s->line, s->col);
			if (s->step)
				compile(s->step);
			else
				emitByte(OpCode::OP_NONE, s->line, s->col);
			emitByte(OpCode::OP_BUILD_SLICE, s->line, s->col);
			break;
		}
		case ExprType::CALL: {
			auto c = static_cast<CallExpr *>(e);
			if (c->name == "super") {
				if (!c->args.empty())
					throw ArgumentError("super() takes no arguments", c->line,
						c->col);
				emitByte(OpCode::OP_SUPER, c->line, c->col);
				break;
			}
			// for (auto arg : c->args) compile(arg);
			for (size_t i = 0; i < c->args.size(); i++) {
				Expr *arg = c->args[i];
				CopyMode mode =
					(i < c->modes.size()) ? c->modes[i] : CopyMode::SHALLOW;

				if (mode == CopyMode::DEEP) {
					compile(arg);
					emitByte(OpCode::OP_DEEP_COPY, c->line, c->col);
				} else if (mode == CopyMode::REF) {
					if (auto v = dynamic_cast<VarExpr *>(arg)) {
						int local = resolveLocal(v->name);
						if (local != -1) {
							emitByte(OpCode::OP_REF_LOCAL, c->line, c->col);
							chunk->write((uint8_t)local, c->line, c->col);
						} else {
							emitIdentifier(OpCode::OP_REF_VAR, v->name, c->line,
								c->col);
						}
					} else if (auto idx = dynamic_cast<IndexExpr *>(arg)) {
						compile(idx->base);
						compile(idx->index);
						emitByte(OpCode::OP_REF_INDEX, c->line, c->col);
					} else if (auto get = dynamic_cast<GetExpr *>(arg)) {
						compile(get->object);
						emitIdentifier(OpCode::OP_REF_PROPERTY, get->name, c->line,
							c->col);
					} else {
						throw OwnershipError(
							"Cannot take reference of this expression in call",
							c->line, c->col);
					}
				} else {
					compile(arg);
				}
			}
			int arg = resolveLocal(c->name);
			if (arg != -1) {
				emitByte(OpCode::OP_GET_LOCAL, c->line, c->col);
				chunk->write((uint8_t)arg, c->line, c->col);
			} else
				emitIdentifier(OpCode::OP_GET_VAR, c->name, c->line, c->col);
			emitByte(OpCode::OP_CALL, c->line, c->col);
			chunk->write(static_cast<uint8_t>(c->args.size()), c->line, c->col);
			break;
		}
		case ExprType::TERNARY: {
			auto t = static_cast<TernaryExpr *>(e);
			compile(t->condition);
			int elseJump = emitJump(OpCode::OP_JUMP_IF_FALSE, t->line, t->col);
			emitByte(OpCode::OP_POP, t->line, t->col);
			compile(t->trueBranch);
			int endJump = emitJump(OpCode::OP_JUMP, t->line, t->col);
			patchJump(elseJump);
			emitByte(OpCode::OP_POP, t->line, t->col);
			compile(t->falseBranch);
			patchJump(endJump);
			break;
		}
		case ExprType::LIST: {
			auto l = static_cast<ListExpr *>(e);
			for (auto item : l->elements)
				compile(item);
			emitByte(OpCode::OP_BUILD_LIST, l->line, l->col);
			chunk->write((uint8_t)l->elements.size(), l->line, l->col);
			break;
		}
		case ExprType::SET: {
			auto s = static_cast<SetExpr *>(e);
			for (auto item : s->elements)
				compile(item);
			emitByte(OpCode::OP_BUILD_SET, s->line, s->col);
			chunk->write((uint8_t)s->elements.size(), s->line, s->col);
			break;
		}
		case ExprType::TUPLE: {
			auto t = static_cast<TupleExpr *>(e);
			for (auto item : t->elements)
				compile(item);
			emitByte(OpCode::OP_BUILD_TUPLE, t->line, t->col);
			chunk->write((uint8_t)t->elements.size(), t->line, t->col);
			break;
		}
		case ExprType::DICT: {
			auto d = static_cast<DictExpr *>(e);
			for (auto &item : d->items) {
				compile(item.first);
				compile(item.second);
				emitByte(OpCode::OP_COLON, d->line, d->col);
			}
			emitByte(OpCode::OP_BUILD_DICT, d->line, d->col);
			chunk->write((uint8_t)d->items.size(), d->line, d->col);
			break;
		}
		case ExprType::RANGE: {
			auto r = static_cast<RangeExpr *>(e);
			compile(r->start);
			compile(r->end);
			if (r->step)
				compile(r->step);
			else
				emitConstant(Value::Int(1), r->line, r->col);
			emitByte(OpCode::OP_BUILD_RANGE, r->line, r->col);
			break;
		}
		case ExprType::VECTOR: {
			auto ve = static_cast<VectorExpr *>(e);
			for (auto *el : ve->elements)
				compile(el);
			emitByte(OpCode::OP_BUILD_VECTOR, ve->line, ve->col);
			chunk->write((uint8_t)ve->elements.size(), ve->line, ve->col);
			break;
		}
		case ExprType::METHOD_CALL: {
			auto m = static_cast<MethodCallExpr *>(e);
			compile(m->object);
			for (auto arg : m->args) {
				compile(arg);
			}
			emitByte(OpCode::OP_INVOKE, m->line, m->col);
			int nameIdx = chunk->addConstant(Value::String(m->method));
			chunk->write((uint8_t)nameIdx, m->line, m->col);
			chunk->write((uint8_t)m->args.size(), m->line, m->col);
			break;
		}
		// will add more cases
		default:
			break;
		}
	}
	void compileBinary(BinExpr *b) {
		if (b->op == TokenType::NOT) {
			compile(b->right);
			emitByte(OpCode::OP_NOT, b->line, b->col);
			return; // Exit early!
		}
		// constant folding (experimental):
		Value foldedResult;
		if (tryExtractConstant(b, foldedResult)) {
			emitConstant(foldedResult, b->line, b->col);
			return;
		}
		compile(b->left);
		compile(b->right);
		switch (b->op) {
		case TokenType::PLUS:
			emitByte(OpCode::OP_ADD, b->line, b->col);
			break;
		case TokenType::MINUS:
			emitByte(OpCode::OP_SUB, b->line, b->col);
			break;
		case TokenType::STAR:
			emitByte(OpCode::OP_MUL, b->line, b->col);
			break;
		case TokenType::SLASH:
			emitByte(OpCode::OP_DIV, b->line, b->col);
			break;
		case TokenType::FLOOR_DIV:
			emitByte(OpCode::OP_FLOOR_DIV, b->line, b->col);
			break;
		case TokenType::MOD:
			emitByte(OpCode::OP_MOD, b->line, b->col);
			break;
		case TokenType::POW:
			emitByte(OpCode::OP_POW, b->line, b->col);
			break;
			// Logic
		case TokenType::GT:
			emitByte(OpCode::OP_GT, b->line, b->col);
			break;
		case TokenType::GTE:
			emitByte(OpCode::OP_GTE, b->line, b->col);
			break;
		case TokenType::EQ:
			emitByte(OpCode::OP_EQ, b->line, b->col);
			break;
		case TokenType::NEQ:
			emitByte(OpCode::OP_NEQ, b->line, b->col);
			break;
		case TokenType::STRICT_EQ:
			emitByte(OpCode::OP_STRICT_EQ, b->line, b->col);
			break;
		case TokenType::STRICT_NEQ:
			emitByte(OpCode::OP_STRICT_NEQ, b->line, b->col);
			break;
		case TokenType::LT:
			emitByte(OpCode::OP_LT, b->line, b->col);
			break;
		case TokenType::LTE:
			emitByte(OpCode::OP_LTE, b->line, b->col);
			break;
			// Identity & Membership
		case TokenType::IS:
			emitByte(OpCode::OP_IS, b->line, b->col);
			break;
		case TokenType::IS_NOT:
			emitByte(OpCode::OP_IS_NOT, b->line, b->col);
			break;
		case TokenType::IS_IN:
			emitByte(OpCode::OP_IS_IN, b->line, b->col);
			break;
		case TokenType::IS_NOT_IN:
			emitByte(OpCode::OP_IS_NOT_IN, b->line, b->col);
			break;
			// Logical (non-short-circuiting)
		case TokenType::XOR:
			emitByte(OpCode::OP_XOR, b->line, b->col);
			break;
		case TokenType::NXOR:
			emitByte(OpCode::OP_NXOR, b->line, b->col);
			break;
			//"pairing" op
		case TokenType::COLON:
			emitByte(OpCode::OP_COLON, b->line, b->col);
			break;
		default:
			break;
		}
	}
	void compileLogical(BinExpr *l) {
		if (l->op == TokenType::AND) {
			compile(l->left);
			int endJump = emitJump(OpCode::OP_JUMP_IF_FALSE, l->line, l->col);
			emitByte(OpCode::OP_POP, l->line, l->col); // Pop left if it was true
			compile(l->right);
			patchJump(endJump);
		} else if (l->op == TokenType::OR) {
			compile(l->left);
			int elseJump = emitJump(OpCode::OP_JUMP_IF_FALSE, l->line, l->col);
			int endJump = emitJump(OpCode::OP_JUMP, l->line, l->col);
			patchJump(elseJump);
			emitByte(OpCode::OP_POP, l->line, l->col); // Pop left if it was false
			compile(l->right);
			patchJump(endJump);
		} else if (l->op == TokenType::NAND) {
			compile(l->left);
			int shortCircuitJump = emitJump(OpCode::OP_JUMP_IF_FALSE, l->line, l->col);
			emitByte(OpCode::OP_POP, l->line, l->col);
			compile(l->right);
			emitByte(OpCode::OP_NOT, l->line, l->col);
			int endJump = emitJump(OpCode::OP_JUMP, l->line, l->col);
			patchJump(shortCircuitJump);
			emitByte(OpCode::OP_POP, l->line, l->col);
			emitByte(OpCode::OP_TRUE, l->line, l->col);
			patchJump(endJump);
		} else if (l->op == TokenType::NOR) {
			compile(l->left);
			int evalRightJump = emitJump(OpCode::OP_JUMP_IF_FALSE, l->line, l->col);
			emitByte(OpCode::OP_POP, l->line, l->col);
			emitByte(OpCode::OP_FALSE, l->line, l->col);
			int endJump = emitJump(OpCode::OP_JUMP, l->line, l->col);
			patchJump(evalRightJump);
			emitByte(OpCode::OP_POP, l->line, l->col);
			compile(l->right);
			emitByte(OpCode::OP_NOT, l->line, l->col);

			patchJump(endJump);
		}
	}
	void emitByte(OpCode op, int line, int col) { chunk->write(op, line, col); }
	void emitConstant(Value v, int line, int col) {
		int index = chunk->addConstant(v);
		if (index > 255) {
			if (index > 65535)
				throw MemoryError("Too many constants in one chunk (limit 65535)",
					line, col);
			emitByte(OpCode::OP_CONSTANT_LONG, line, col);
			chunk->write((index >> 8) & 0xff, line, col);
			chunk->write(index & 0xff, line, col);
		} else {
			emitByte(OpCode::OP_CONSTANT, line, col);
			chunk->write(static_cast<uint8_t>(index), line, col);
		}
	}
	int emitJump(OpCode instruction, int line, int col) {
		emitByte(instruction, line, col);
		chunk->write(0xff, line, col);
		chunk->write(0xff, line, col);
		return (int)chunk->code.size() - 2;
	}
	void patchJump(int offset) {
		int jump = (int)chunk->code.size() - offset - 2;
		if (jump > 65535)
			throw RangeError("Too much code to jump over!", 0, 0);
		chunk->code[offset] = (jump >> 8) & 0xff;
		chunk->code[offset + 1] = jump & 0xff;
	}
	void compileWithMode(Expr *expr, int line, int col) {
		if (auto o = dynamic_cast<OwnershipExpr *>(expr)) {
			if (o->mode == CopyMode::DEEP) {
				compile(o->expr);
				emitByte(OpCode::OP_DEEP_COPY, line, col);
			} else if (o->mode == CopyMode::REF)
				compile(expr);
		} else {
			compile(expr);
			if (expr->type != ExprType::LIST && expr->type != ExprType::DICT &&
				 expr->type != ExprType::SET && expr->type != ExprType::VECTOR &&
				 expr->type != ExprType::LAMBDA && expr->type != ExprType::NUMBER &&
				 expr->type != ExprType::STRING && expr->type != ExprType::BOOL)
				emitByte(OpCode::OP_SHALLOW_COPY, line, col);
		}
	}
	void compileStmt(Stmt *s) {
		if (!s)
			return;
		switch (s->type) {
		case StmtType::CLASS: {
			auto c = static_cast<ClassStmt *>(s);
			for (const auto &p : c->parents) {
				int arg = resolveLocal(p);
				if (arg != -1) {
					emitByte(OpCode::OP_GET_LOCAL, c->line, 0);
					chunk->write((uint8_t)arg, c->line, 0);
				} else
					emitIdentifier(OpCode::OP_GET_VAR, p, c->line, 0);
			}
			emitConstant(Value::String(c->name), c->line, 0);
			emitByte(OpCode::OP_CLASS, c->line, 0);
			chunk->write((uint8_t)c->parents.size(), c->line, 0);
			vector<Stmt *> instanceFields;
			auto collectFields = [&](const vector<Stmt *> &body) {
				for (auto *stmt : body) {
					if (stmt->type == StmtType::LET) {
						auto let = static_cast<LetStmt *>(stmt);
						if (let->name.rfind("self.", 0) == 0)
							instanceFields.push_back(let);
					}
				}
			};
			collectFields(c->publicBody);
			collectFields(c->privateBody);
			collectFields(c->protectedBody);
			auto compileClassBody = [&](const vector<Stmt *> &body,
												AccessLevel access) {
				for (auto *stmt : body) {
					if (stmt->type == StmtType::FUNC) {
						auto *func = static_cast<FuncStmt *>(stmt);
						if (func->name == "__construct__")
							emitFunction(func, true, instanceFields);
						else
							emitFunction(func, true);
						emitByte(OpCode::OP_METHOD, c->line, 0);
						chunk->write((uint8_t)access, c->line, 0);
					} else if (stmt->type == StmtType::LET) {
						auto let = static_cast<LetStmt *>(stmt);
						string propName = let->name;
						bool isInstance = false;
						if (propName.rfind("self.", 0) == 0) {
							propName = propName.substr(5);
							isInstance = true;
						} else if (propName.rfind("obj.", 0) == 0)
							propName = propName.substr(4);
						if (isInstance)
							emitByte(OpCode::OP_NOTYPE, let->line, let->col);
						else if (let->value)
							compile(let->value);
						else
							emitByte(OpCode::OP_NOTYPE, let->line, let->col);
						emitByte(OpCode::OP_CLASS_FIELD, let->line, let->col);
						chunk->write((uint8_t)access, let->line, let->col);
						int nameIdx = chunk->addConstant(Value::String(propName));
						chunk->write((uint8_t)nameIdx, let->line, let->col);
					} else if (stmt->type == StmtType::MULTI_LET) {
						auto mlet = static_cast<MultiLetStmt *>(stmt);
						for (size_t i = 0; i < mlet->names.size(); i++) {
							string propName = mlet->names[i];
							bool isInstance = false;
							if (propName.rfind("self.", 0) == 0) {
								propName = propName.substr(5);
								isInstance = true;
							} else if (propName.rfind("obj.", 0) == 0)
								propName = propName.substr(4);
							if (isInstance)
								emitByte(OpCode::OP_NOTYPE, mlet->line, 0);
							else if (mlet->values[i])
								compile(mlet->values[i]);
							else
								emitByte(OpCode::OP_NOTYPE, mlet->line, 0);
							emitByte(OpCode::OP_CLASS_FIELD, mlet->line, 0);
							chunk->write((uint8_t)access, mlet->line, 0);
							int nameIdx =
								chunk->addConstant(Value::String(propName));
							chunk->write((uint8_t)nameIdx, mlet->line, 0);
						}
					}
				}
			};
			compileClassBody(c->publicBody, AccessLevel::PUBLIC);
			compileClassBody(c->privateBody, AccessLevel::PRIVATE);
			compileClassBody(c->protectedBody, AccessLevel::PROTECTED);
			if (scopeDepth > 0) {
				addLocal(c->name);
				emitByte(OpCode::OP_SET_LOCAL, c->line, 0);
				chunk->write((uint8_t)(locals.size() - 1), c->line, 0);
			} else {
				emitIdentifier(OpCode::OP_DEFINE_VAR, c->name, c->line, 0);
				chunk->write((uint8_t)0, c->line, 0);
			}
			break;
		}
		case StmtType::DELETE: {
			auto delStmt = static_cast<DeleteStmt *>(s);
			if (auto v = dynamic_cast<VarExpr *>(delStmt->target)) {
				int local = resolveLocal(v->name);
				if (local != -1) {
					emitByte(OpCode::OP_REF_LOCAL, delStmt->line, delStmt->col);
					chunk->write((uint8_t)local, delStmt->line, delStmt->col);
				} else
					emitIdentifier(OpCode::OP_REF_VAR, v->name, delStmt->line,
						delStmt->col);
			} else if (auto idx = dynamic_cast<IndexExpr *>(delStmt->target)) {
				compile(idx->base);
				compile(idx->index);
				emitByte(OpCode::OP_REF_INDEX, delStmt->line, delStmt->col);
			}
			/*else if (auto own = dynamic_cast<OwnershipExpr*>(delStmt->target))
			{ compile(own->expr); emitByte(OpCode::OP_)
			}*/
			else
				throw SyntaxError("Can only delete variables or indices",
					delStmt->line, delStmt->col);
			emitByte(OpCode::OP_DELETE, delStmt->line, delStmt->col);
			emitByte(OpCode::OP_POP, delStmt->line, delStmt->col);
			break;
		}
		case StmtType::EXPR: {
			auto es = static_cast<ExprStmt *>(s);
			compile(es->expr);
			emitByte(OpCode::OP_POP, es->line, es->col);
			break;
		}
		case StmtType::ASSERT: {
			auto as = static_cast<AssertStmt *>(s);
			compile(as->condition);
			int failJump = emitJump(OpCode::OP_JUMP_IF_FALSE, as->line, 0);
			emitByte(OpCode::OP_POP, as->line, 0);
			int successJump = emitJump(OpCode::OP_JUMP, as->line, 0);
			patchJump(failJump);
			emitByte(OpCode::OP_POP, as->line, 0);
			if (as->message)
				compile(as->message);
			else {
				std::string msg =
					"Assertion failed on line " + std::to_string(as->line);
				emitConstant(Value::String(msg), as->line, 0);
			}
			emitConstant(Value::String("AssertionError"), as->line, 0);
			emitByte(OpCode::OP_THROW, as->line, 0);
			patchJump(successJump);
			break;
		}
		case StmtType::THROW: {
			auto ts = static_cast<ThrowStmt *>(s);
			compile(ts->message);
			emitConstant(Value::String(ts->errorType), ts->line, 0);
			emitByte(OpCode::OP_THROW, ts->line, 0);
			break;
		}
		case StmtType::TRY: {
			auto t = static_cast<TryStmt *>(s);
			emitByte(OpCode::OP_TRY_ENTER, t->line, 0);
			int tryEnterAddr = (int)chunk->code.size();
			chunk->write(0xff, t->line, 0);
			chunk->write(0xff, t->line, 0);
			chunk->write(0xff, t->line, 0);
			chunk->write(0xff, t->line, 0);
			for (auto st : t->tryBlock)
				compileStmt(st);
			emitByte(OpCode::OP_TRY_EXIT, t->line, 0);
			int skipCatchJump = emitJump(OpCode::OP_JUMP, t->line, 0);
			int catchStartAddr = (int)chunk->code.size();
			int catchOffset = catchStartAddr - tryEnterAddr;
			chunk->code[tryEnterAddr] = (catchOffset >> 8) & 0xff;
			chunk->code[tryEnterAddr + 1] = catchOffset & 0xff;
			vector<int> exitJumps;
			for (auto &catchBlock : t->catches) {
				emitByte(OpCode::OP_CATCH, t->line, 0);
				chunk->write((uint8_t)catchBlock.types.size(), t->line, 0);
				for (const auto &typeName : catchBlock.types) {
					int idx = chunk->addConstant(Value::String(typeName));
					chunk->write((uint8_t)idx, t->line, 0);
				}
				int nextCatchJump =
					emitJump(OpCode::OP_JUMP_IF_FALSE, t->line, 0);
				emitByte(OpCode::OP_POP, t->line, 0);
				emitByte(OpCode::OP_POP, t->line, 0);
				beginScope();
				for (auto st : catchBlock.body)
					compileStmt(st);
				endScope(t->line, 0);
				exitJumps.push_back(emitJump(OpCode::OP_JUMP, t->line, 0));
				patchJump(nextCatchJump);
				emitByte(OpCode::OP_POP, t->line, 0);
			}
			emitByte(OpCode::OP_RETHROW, t->line, 0);
			patchJump(skipCatchJump);
			for (int j : exitJumps)
				patchJump(j);
			int finallyStartAddr = (int)chunk->code.size();
			if (!t->finallyBlock.empty()) {
				int finallyOffset = finallyStartAddr - tryEnterAddr;
				chunk->code[tryEnterAddr + 2] = (finallyOffset >> 8) & 0xff;
				chunk->code[tryEnterAddr + 3] = finallyOffset & 0xff;
				beginScope();
				for (auto st : t->finallyBlock)
					compileStmt(st);
				endScope(t->line, 0);
				emitByte(OpCode::OP_END_FINALLY, t->line, 0);
			} else {
				chunk->code[tryEnterAddr + 2] = 0;
				chunk->code[tryEnterAddr + 3] = 0;
			}
			break;
		}
		case StmtType::IMPORT: {
			auto imp = static_cast<ImportStmt *>(s);
			emitConstant(Value::String(imp->libName), imp->line, 0);
			for (const auto &sym : imp->symbols)
				emitConstant(Value::String(sym), imp->line, 0);
			emitByte(OpCode::OP_IMPORT, imp->line, 0);
			chunk->write((uint8_t)imp->symbols.size(), imp->line, 0);
			if (imp->symbols.empty()) {
				if (scopeDepth > 0) {
					addLocal(imp->libName);
				} else {
					emitIdentifier(OpCode::OP_DEFINE_VAR, imp->libName, imp->line, 0);
					chunk->write(0, imp->line, 0);
				}
			} else {
				if (scopeDepth > 0) {
					for (const auto &sym : imp->symbols) {
						addLocal(sym);
					}
				} else {
					for (int i = (int)imp->symbols.size() - 1; i >= 0; i--) {
						emitIdentifier(OpCode::OP_DEFINE_VAR, imp->symbols[i], imp->line, 0);
						chunk->write(0, imp->line, 0);
					}
				}
			}
			break;
		}
		case StmtType::LET: {
			auto let = static_cast<LetStmt *>(s);
			bool isSelf = (let->name.rfind("self.", 0) == 0);
			bool isObj = (let->name.rfind("obj.", 0) == 0);
			if (isSelf || isObj) {
				string propName = let->name.substr(isSelf ? 5 : 4);
				int objSlot = resolveLocal(isSelf ? "self" : "obj");
				if (objSlot == -1)
					throw NameError("Cannot use '" + string(isSelf ? "self" : "obj") + "' outside of a method.", let->line, let->col);
				emitByte(OpCode::OP_GET_LOCAL, let->line, let->col);
				chunk->write((uint8_t)objSlot, let->line, let->col);
				if (let->value)
					compile(let->value);
				else
					emitByte(OpCode::OP_NOTYPE, let->line, let->col);
				emitIdentifier(OpCode::OP_SET_PROPERTY, propName, let->line, let->col);
				chunk->write((uint8_t)let->access, let->line, let->col);
				emitByte(OpCode::OP_POP, let->line, let->col);
				break;
			}
			if (scopeDepth == 0 && let->value &&
				 let->value->type == ExprType::OWNERSHIP) {
				auto o = static_cast<OwnershipExpr *>(let->value);
				if (o->mode == CopyMode::REF) {
					emitIdentifier(OpCode::OP_DEFINE_REF, let->name, let->line,
						let->col);
					if (auto v = dynamic_cast<VarExpr *>(o->expr))
						emitIdentifier(OpCode::OP_REF_VAR, v->name, let->line,
							let->col);
					else if (auto idx = dynamic_cast<IndexExpr *>(o->expr)) {
						compile(idx->base);
						compile(idx->index);
						emitByte(OpCode::OP_REF_INDEX, let->line, let->col);
					}
					break;
				}
			}
			if (let->value) {
				compile(let->value);
				if (let->value->type != ExprType::OWNERSHIP &&
					 let->value->type != ExprType::LIST &&
					 let->value->type != ExprType::DICT &&
					 let->value->type != ExprType::SET &&
					 let->value->type != ExprType::TUPLE &&
					 let->value->type != ExprType::VECTOR &&
					 let->value->type != ExprType::LAMBDA &&
					 let->value->type != ExprType::NUMBER &&
					 let->value->type != ExprType::STRING &&
					 let->value->type != ExprType::BOOL) {
					emitByte(OpCode::OP_SHALLOW_COPY, let->line, let->col);
				}
			} else
				emitByte(OpCode::OP_NOTYPE, let->line, let->col);
			if (scopeDepth > 0) {
				if (let->isConst || let->isLocked) {
					emitByte(OpCode::OP_SET_FLAGS, let->line, let->col);
					uint8_t flags = 0;
					if (let->isConst)
						flags |= 0x01;
					if (let->isLocked)
						flags |= 0x02;
					chunk->write(flags, let->line, let->col);
				}
				addLocal(let->name);
				if (let->isConst && let->value) {
					Value constVal;
					if (tryExtractConstant(let->value, constVal)) {
						locals.back().isConst = true;
						locals.back().hasKnownValue = true;
						locals.back().knownValue = constVal;
					}
				}
				if (DEBUGGER_MODE_IS_ENABLED)
					emitIdentifier(OpCode::OP_DEBUG_NAME, let->name, let->line,
						let->col);
			} else {
				emitIdentifier(OpCode::OP_DEFINE_VAR, let->name, let->line,
					let->col);
				uint8_t flags = 0;
				if (let->isConst)
					flags |= 0x01;
				if (let->isLocked)
					flags |= 0x02;
				chunk->write(flags, let->line, let->col);
			}
			break;
		}
		case StmtType::ASSIGN: {
			auto as = static_cast<AssignStmt *>(s);
			if (auto get = dynamic_cast<GetExpr *>(as->target)) {
				compile(get->object);
				if (as->op != TokenType::ASSIGN) {
					emitByte(OpCode::OP_DUP, as->line, as->col);
					emitIdentifier(OpCode::OP_GET_PROPERTY, get->name, as->line,
						as->col);
					compile(as->value);
					switch (as->op) {
					case TokenType::PLUS_EQ:
						emitByte(OpCode::OP_IADD, as->line, as->col);
						break;
					case TokenType::MINUS_EQ:
						emitByte(OpCode::OP_ISUB, as->line, as->col);
						break;
					case TokenType::STAR_EQ:
						emitByte(OpCode::OP_IMUL, as->line, as->col);
						break;
					case TokenType::DIV_EQ:
						emitByte(OpCode::OP_IDIV, as->line, as->col);
						break;
					case TokenType::FLOOR_DIV_EQ:
						emitByte(OpCode::OP_IFLOOR_DIV, as->line, as->col);
						break;
					case TokenType::MOD_EQ:
						emitByte(OpCode::OP_IMOD, as->line, as->col);
						break;
					case TokenType::POW_EQ:
						emitByte(OpCode::OP_IPOW, as->line, as->col);
						break;

					default:
						throw SyntaxError(
							"Unknown/Unsupported augmented assignment operator",
							as->line, as->col);
					}
				} else
					compile(as->value); // Stack: [obj, val]
				emitIdentifier(OpCode::OP_SET_PROPERTY, get->name, as->line, as->col);
				chunk->write((uint8_t)AccessLevel::PUBLIC, as->line, as->col);
				emitByte(OpCode::OP_POP, as->line, as->col);
				break;
			}
			if (auto idx = dynamic_cast<IndexExpr *>(as->target)) {
				if (as->op != TokenType::ASSIGN) {
					compile(idx->base);
					compile(idx->index);
					emitByte(OpCode::OP_DUP_2, as->line, as->col);
					emitByte(OpCode::OP_GET_INDEX, as->line, as->col);
					compile(as->value);
					switch (as->op) {
					case TokenType::PLUS_EQ:
						emitByte(OpCode::OP_IADD, as->line, as->col);
						break;
					case TokenType::MINUS_EQ:
						emitByte(OpCode::OP_ISUB, as->line, as->col);
						break;
					case TokenType::STAR_EQ:
						emitByte(OpCode::OP_IMUL, as->line, as->col);
						break;
					case TokenType::DIV_EQ:
						emitByte(OpCode::OP_IDIV, as->line, as->col);
						break;
					case TokenType::FLOOR_DIV_EQ:
						emitByte(OpCode::OP_IFLOOR_DIV, as->line, as->col);
						break;
					case TokenType::MOD_EQ:
						emitByte(OpCode::OP_IMOD, as->line, as->col);
						break;
					case TokenType::POW_EQ:
						emitByte(OpCode::OP_IPOW, as->line, as->col);
						break;
					default:
						throw SyntaxError("Unknown augmented assignment",
							as->line, as->col);
					}
					emitByte(OpCode::OP_SET_INDEX, as->line, as->col);
					emitByte(OpCode::OP_POP, as->line, as->col);
					break;
				}
				compile(idx->base);
				compile(idx->index);
				compile(as->value);
				emitByte(OpCode::OP_SET_INDEX, as->line, as->col);
				emitByte(OpCode::OP_POP, as->line, as->col);
				break;
			}
			auto v = static_cast<VarExpr *>(as->target);
			if (auto o = dynamic_cast<OwnershipExpr *>(as->value)) {
				if (scopeDepth == 0 && o->mode == CopyMode::REF) {
					emitIdentifier(OpCode::OP_SET_REF, v->name, as->line,
						as->col);
					if (auto targetVar = dynamic_cast<VarExpr *>(o->expr)) {
						emitIdentifier(OpCode::OP_REF_VAR, targetVar->name,
							as->line, as->col);
					} else if (auto idx = dynamic_cast<IndexExpr *>(o->expr)) {
						compile(idx->base);
						compile(idx->index);
						emitByte(OpCode::OP_REF_INDEX, as->line, as->col);
					}
					break;
				}
			}
			int arg = resolveLocal(v->name);
			if (arg != -1) {
				bool isInc = (as->op == TokenType::PLUS_EQ);
				if (isInc) {
					if (auto num = dynamic_cast<NumberExpr *>(as->value)) {
						if (num->val == 1) {
							emitByte(OpCode::OP_INC_LOCAL, as->line, as->col);
							chunk->write((uint8_t)arg, as->line, as->col);
							break;
						}
					}
				}
			}
			if (as->op == TokenType::ASSIGN)
				compileWithMode(as->value, as->line, as->col);
			else {
				if (arg != -1) {
					emitByte(OpCode::OP_GET_LOCAL, as->line, as->col);
					chunk->write((uint8_t)arg, as->line, as->col);
				} else
					emitIdentifier(OpCode::OP_GET_VAR, v->name, as->line,
						as->col);
				compile(as->value);
				switch (as->op) {
				case TokenType::PLUS_EQ:
					emitByte(OpCode::OP_IADD, as->line, as->col);
					break;
				case TokenType::MINUS_EQ:
					emitByte(OpCode::OP_ISUB, as->line, as->col);
					break;
				case TokenType::STAR_EQ:
					emitByte(OpCode::OP_IMUL, as->line, as->col);
					break;
				case TokenType::DIV_EQ:
					emitByte(OpCode::OP_IDIV, as->line, as->col);
					break;
				case TokenType::FLOOR_DIV_EQ:
					emitByte(OpCode::OP_IFLOOR_DIV, as->line, as->col);
					break;
				case TokenType::POW_EQ:
					emitByte(OpCode::OP_IPOW, as->line, as->col);
					break;
				case TokenType::MOD_EQ:
					emitByte(OpCode::OP_IMOD, as->line, as->col);
					break;
				}
			}
			if (arg != -1) {
				emitByte(OpCode::OP_SET_LOCAL, as->line, as->col);
				chunk->write((uint8_t)arg, as->line, as->col);
			} else
				emitIdentifier(OpCode::OP_SET_VAR, v->name, as->line, as->col);
			emitByte(OpCode::OP_POP, as->line, as->col);
			break;
		}
		case StmtType::IF: {
			auto ifs = static_cast<IfStmt *>(s);
			vector<int> exitJumps;
			compile(ifs->condition);
			int jumpToNext =
				emitJump(OpCode::OP_JUMP_IF_FALSE, ifs->line, ifs->col);
			emitByte(OpCode::OP_POP, ifs->line, ifs->col);
			beginScope();
			for (auto stmt : ifs->body)
				compileStmt(stmt);
			endScope(ifs->line, ifs->col);
			exitJumps.push_back(emitJump(OpCode::OP_JUMP, ifs->line, ifs->col));
			patchJump(jumpToNext);
			emitByte(OpCode::OP_POP, ifs->line, ifs->col);
			for (const auto &elif : ifs->elifs) {
				compile(elif.first);
				jumpToNext =
					emitJump(OpCode::OP_JUMP_IF_FALSE, ifs->line, ifs->col);
				emitByte(OpCode::OP_POP, ifs->line, ifs->col);
				beginScope();
				for (auto stmt : elif.second)
					compileStmt(stmt);
				endScope(ifs->line, ifs->col);
				exitJumps.push_back(
					emitJump(OpCode::OP_JUMP, ifs->line, ifs->col));
				patchJump(jumpToNext);
				emitByte(OpCode::OP_POP, ifs->line, ifs->col);
			}
			if (!ifs->elseBody.empty()) {
				beginScope();
				for (auto stmt : ifs->elseBody)
					compileStmt(stmt);
				endScope(ifs->line, ifs->col);
			}
			for (int offset : exitJumps) {
				patchJump(offset);
			}
			break;
		}
		case StmtType::SWITCH: {
			auto sw = static_cast<SwitchStmt *>(s);
			int startAddr = (int)chunk->code.size();
			beginScope();
			compile(sw->target);
			addLocal("");
			LoopContext switchLoop = {startAddr, -1, {}, {}, false, (int)locals.size(), -1};
			loopStack.push_back(switchLoop);
			bool useTable = true;
			vector<long long> tableKeys;
			for (const auto &c : sw->cases) {
				if (c.value->type == ExprType::NUMBER) {
					auto num = static_cast<NumberExpr *>(c.value);
					if (!num->isFloat)
						tableKeys.push_back((long long)num->val);
					else {
						useTable = false;
						break;
					}
				} else {
					useTable = false;
					break;
				}
			}
			if (useTable && !tableKeys.empty()) {
				auto minVal =
					*std::min_element(tableKeys.begin(), tableKeys.end());
				auto maxVal =
					*std::max_element(tableKeys.begin(), tableKeys.end());
				if ((maxVal - minVal) > 256)
					useTable = false;
			} else
				useTable = false;
			// JUMP TABLE (Fast)
			if (useTable && !tableKeys.empty()) {
				long long minVal =
					*std::min_element(tableKeys.begin(), tableKeys.end());
				long long maxVal =
					*std::max_element(tableKeys.begin(), tableKeys.end());
				long long count = maxVal - minVal + 1;
				emitByte(OpCode::OP_SWITCH_TABLE, sw->line, sw->col);
				int minIdx = chunk->addConstant(Value::Int(minVal));
				chunk->write((uint8_t)minIdx, sw->line, sw->col);
				chunk->write((uint8_t)count, sw->line, sw->col);
				int tableStart = (int)chunk->code.size();
				for (int i = 0; i < count; i++) {
					chunk->write(0xff, sw->line, sw->col);
					chunk->write(0xff, sw->line, sw->col);
				}
				auto compileBodyWithLoop = [&](const vector<Stmt *> &stmts) {
					beginScope();
					for (auto stmt : stmts)
						compileStmt(stmt);
					endScope(sw->line, sw->col);
					emitByte(OpCode::OP_POP, sw->line, sw->col);
					emitLoop(startAddr, sw->line, sw->col);
				};
				int defaultAddr = (int)chunk->code.size();
				compileBodyWithLoop(sw->defaultBody);
				std::map<long long, int> caseAddresses;
				for (size_t i = 0; i < sw->cases.size(); i++) {
					caseAddresses[(long long)((NumberExpr *)sw->cases[i].value)
							->val] = (int)chunk->code.size();
					compileBodyWithLoop(sw->cases[i].body);
				}
				int currentPos = tableStart;
				for (long long i = 0; i < count; i++) {
					long long val = minVal + i;
					int target = caseAddresses.count(val) ? caseAddresses[val]
																	  : defaultAddr;
					int offset = target - currentPos - 2;
					chunk->code[currentPos] = (offset >> 8) & 0xff;
					chunk->code[currentPos + 1] = offset & 0xff;
					currentPos += 2;
				}
			}
			// LINEAR SCAN (Compatible)
			else {
				vector<int> nextCaseJumps;
				for (const auto &c : sw->cases) {
					emitByte(OpCode::OP_GET_LOCAL, sw->line, sw->col);
					chunk->write((uint8_t)(locals.size() - 1), sw->line, sw->col);
					compile(c.value);
					emitByte(OpCode::OP_STRICT_EQ, sw->line, sw->col);
					int nextJump =
						emitJump(OpCode::OP_JUMP_IF_FALSE, sw->line, sw->col);
					emitByte(OpCode::OP_POP, sw->line, sw->col);
					beginScope();
					for (auto stmt : c.body)
						compileStmt(stmt);
					endScope(sw->line, sw->col);
					emitByte(OpCode::OP_POP, sw->line, sw->col);
					emitLoop(startAddr, sw->line, sw->col);
					patchJump(nextJump);
					emitByte(OpCode::OP_POP, sw->line, sw->col);
				}
				beginScope();
				for (auto stmt : sw->defaultBody)
					compileStmt(stmt);
				endScope(sw->line, sw->col);
				emitByte(OpCode::OP_POP, sw->line, sw->col);
				emitLoop(startAddr, sw->line, sw->col);
			}
			for (int b : loopStack.back().breakJumps)
				patchJump(b);
			loopStack.pop_back();
			endScope(sw->line, sw->col);
			break;
		}
		/*
		case StmtType::FUNC: {
				  auto f = static_cast<FuncStmt*>(s);
				  Chunk* funcChunk = new Chunk();
				  ByteCodeCompiler subCompiler(funcChunk);
				  subCompiler.beginScope();
				  for (const auto& param : f->params)
		subCompiler.addLocal(param.name); for (auto bodyStmt : f->body)
		subCompiler.compileStmt(bodyStmt);
		subCompiler.emitByte(OpCode::OP_NOTYPE, f->line, f->col);
		subCompiler.emitByte(OpCode::OP_RETURN, f->line, f->col); auto* funcObj
		= new FunctionObject(f->params, f->returnType, f->defaultRetArgs,
		f->returnsConst, f->body, nullptr, f->isCached, funcChunk);
		funcObj->name= f->name; Value funcVal; funcVal.type =
		ValueType::FUNCTION; if (DEBUGGER_MODE_IS_ENABLED)
		funcVal.__DEBUGGING__NAME__=funcObj->name; funcVal.ref =
		std::shared_ptr<HeapObject>(funcObj); emitConstant(funcVal, f->line,
		f->col); emitIdentifier(OpCode::OP_DEFINE_VAR, f->name, f->line,
		f->col); chunk->write((uint8_t)0, f->line, f->col); break;
		}
		*/
		case StmtType::FUNC: {
			auto f = static_cast<FuncStmt *>(s);
			emitFunction(f);
			emitIdentifier(OpCode::OP_DEFINE_VAR, f->name, f->line, f->col);
			chunk->write((uint8_t)0, f->line, f->col);
			break;
		}
		case StmtType::RETURN: {
			auto r = static_cast<ReturnStmt *>(s);
			if (r->value)
				compile(r->value);
			else
				emitByte(OpCode::OP_NOTYPE, r->line, r->col);
			emitByte(OpCode::OP_RETURN, r->line, r->col);
			break;
		}
		case StmtType::MULTI_LET: {
			auto m = static_cast<MultiLetStmt *>(s);
			for (auto val : m->values) {
				if (val)
					compileWithMode(val, val->line, val->col);
				else
					emitByte(OpCode::OP_NOTYPE, m->line, m->col);
			}
			if (scopeDepth > 0) {
				for (size_t i = 0; i < m->names.size(); i++) {
					addLocal(m->names[i]);
				}
			} else {
				for (int i = (int)m->names.size() - 1; i >= 0; i--) {
					Expr *valExpr = m->values[i];
					if (valExpr && valExpr->type == ExprType::OWNERSHIP &&
						 static_cast<OwnershipExpr *>(valExpr)->mode ==
							 CopyMode::REF) {
						auto o = static_cast<OwnershipExpr *>(valExpr);
						emitIdentifier(OpCode::OP_DEFINE_REF, m->names[i], m->line,
							m->col);
						if (auto v = dynamic_cast<VarExpr *>(o->expr))
							emitIdentifier(OpCode::OP_REF_VAR, v->name, m->line,
								m->col);
						else if (auto idx = dynamic_cast<IndexExpr *>(o->expr)) {
							compile(idx->base);
							compile(idx->index);
							emitByte(OpCode::OP_REF_INDEX, m->line, m->col);
						}
					} else {
						emitIdentifier(OpCode::OP_DEFINE_VAR, m->names[i], m->line,
							m->col);
						uint8_t flags = 0;
						if (m->isConsts[i])
							flags |= 0x01;
						if (m->isLocked)
							flags |= 0x02;
						chunk->write(flags, m->line, m->col);
					}
				}
			}
			break;
		}
		case StmtType::MULTI_ASSIGN: {
			auto ma = static_cast<MultiAssignStmt *>(s);
			for (auto val : ma->values)
				compileWithMode(val, val->line, val->col);
			for (int i = (int)ma->targets.size() - 1; i >= 0; i--) {
				auto v = static_cast<VarExpr *>(ma->targets[i]);
				int arg = resolveLocal(v->name);
				if (arg != -1) {
					emitByte(OpCode::OP_SET_LOCAL, ma->line, ma->col);
					chunk->write((uint8_t)arg, ma->line, ma->col);
				} else
					emitIdentifier(OpCode::OP_SET_VAR, v->name, ma->line,
						ma->col);
				emitByte(OpCode::OP_POP, ma->line, ma->col);
			}
			break;
		}
		case StmtType::WHILE: {
			beginScope();
			auto w = static_cast<WhileStmt *>(s);
			int startAddr = (int)chunk->code.size();
			LoopContext loop = {startAddr, startAddr, {}, {}, false, locals.size(), -1};
			loopStack.push_back(loop);
			bool optimized = false;
			int exitJump = -1;
			if (w->condition->type == ExprType::BINARY) {
				auto bin = static_cast<BinExpr *>(w->condition);
				if (bin->op == TokenType::LT) {
					if (bin->left->type == ExprType::VAR) {
						auto var = static_cast<VarExpr *>(bin->left);
						int localSlot = resolveLocal(var->name);
						if (localSlot != -1 &&
							 bin->right->type == ExprType::NUMBER) {
							auto num = static_cast<NumberExpr *>(bin->right);
							optimized = true;
							emitByte(OpCode::OP_JUMP_IF_NOT_LT, w->line, w->col);
							chunk->write((uint8_t)localSlot, w->line, w->col);
							int constIdx =
								chunk->addConstant(Value::Int((long long)num->val));
							chunk->write((uint8_t)constIdx, w->line, w->col);
							exitJump = (int)chunk->code.size();
							chunk->write(0xff, w->line, w->col);
							chunk->write(0xff, w->line, w->col);
						}
					}
				}
			}
			if (!optimized) {
				compile(w->condition);
				exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE, w->line, w->col);
				emitByte(OpCode::OP_POP, w->line, w->col);
			}
			beginScope();
			for (auto stmt : w->body)
				compileStmt(stmt);
			endScope(w->line, w->col);
			emitLoop(startAddr, w->line, w->col);
			patchJump(exitJump);
			if (!optimized)
				emitByte(OpCode::OP_POP, w->line, w->col);
			for (int b : loopStack.back().breakJumps)
				patchJump(b);
			loopStack.pop_back();
			endScope(w->line, w->col);
			break;
		}
		case StmtType::DO_WHILE: {
			auto dw = static_cast<DoWhileStmt *>(s);
			int startAddr = (int)chunk->code.size();
			beginScope();
			LoopContext loop = {startAddr, -1, {}, {}, false, locals.size(), -1};
			loopStack.push_back(loop);
			beginScope();
			for (auto stmt : dw->body)
				compileStmt(stmt);
			endScope(dw->line, dw->col);
			for (int jump : loopStack.back().continueJumps)
				patchJump(jump);
			endScope(dw->line, dw->col);
			compile(dw->condition);
			int exitJump =
				emitJump(OpCode::OP_JUMP_IF_FALSE, dw->line, dw->col);
			emitByte(OpCode::OP_POP, dw->line, dw->col);
			emitLoop(startAddr, dw->line, dw->col);
			patchJump(exitJump);
			emitByte(OpCode::OP_POP, dw->line, dw->col);
			for (int b : loopStack.back().breakJumps)
				patchJump(b);
			loopStack.pop_back();
			endScope(dw->line, dw->col);
			break;
		}
		case StmtType::FOR: {
			beginScope();
			auto f = static_cast<ForStmt *>(s);
			for (auto init : f->inits)
				compileStmt(init);
			int condAddr = (int)chunk->code.size();
			int exitJump = -1;
			if (f->condition) {
				compile(f->condition);
				exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE, f->line, f->col);
				emitByte(OpCode::OP_POP, f->line, f->col);
			}
			LoopContext loop = {condAddr, -1, {}, {}, false, locals.size(), -1};
			loopStack.push_back(loop);
			beginScope();
			for (auto stmt : f->body)
				compileStmt(stmt);
			endScope(f->line, f->col);
			int stepStart = (int)chunk->code.size();
			for (int jump : loopStack.back().continueJumps) {
				patchJump(jump);
			}
			loopStack.back().stepAddress = stepStart;
			for (auto step : f->steps)
				compileStmt(step);
			emitLoop(condAddr, f->line, f->col);
			if (exitJump != -1) {
				patchJump(exitJump);
				emitByte(OpCode::OP_POP, f->line, f->col);
			}
			for (int b : loopStack.back().breakJumps)
				patchJump(b);
			loopStack.pop_back();
			endScope(f->line, f->col);
			break;
		}
		case StmtType::FOR_EACH: {
			auto fe = static_cast<ForEachStmt *>(s);
			beginScope();
			size_t colCount = fe->collections.size();
			size_t varCount = fe->loopVars.size();
			bool isDictUnpack = (colCount == 1 && varCount == 2);
			bool isOneToOne = (colCount == varCount);
			if (!isDictUnpack && !isOneToOne) {
				throw ValueError(
					"Mismatch between loop variables and collections.", fe->line,
					fe->col);
			}
			int streamCount = isDictUnpack ? 2 : (int)colCount;
			if (isDictUnpack) {
				compile(fe->collections[0]);
				emitByte(OpCode::OP_UNPACK_DICT, fe->line, fe->col);
			} else {
				for (auto col : fe->collections) {
					compile(col);
					emitByte(OpCode::OP_TO_STREAM, fe->line, fe->col);
				}
			}
			for (int i = 0; i < streamCount; i++)
				addLocal("");
			emitConstant(Value::Int(0), fe->line, fe->col);
			addLocal("");
			int startAddr = (int)chunk->code.size();
			emitByte(OpCode::OP_FOR_ITER, fe->line, fe->col);
			int exitJump = (int)chunk->code.size();
			chunk->write(0xff, fe->line, fe->col);
			chunk->write(0xff, fe->line, fe->col);
			chunk->write((uint8_t)streamCount, fe->line, fe->col);
			beginScope();
			int iterSlot = (int)locals.size() - 1;
			if (scopeDepth > 0) {
				for (size_t i = 0; i < fe->loopVars.size(); i++)
					addLocal(fe->loopVars[i]);
			} else {
				for (int i = (int)fe->loopVars.size() - 1; i >= 0; i--) {
					emitIdentifier(OpCode::OP_DEFINE_VAR, fe->loopVars[i],
						fe->line, fe->col);
					chunk->write(0, fe->line, fe->col);
				}
			}
			loopStack.push_back({startAddr,
				startAddr,
				{},
				{},
				true,
				(int)locals.size() - streamCount,
				iterSlot});
			beginScope();
			for (auto stmt : fe->body)
				compileStmt(stmt);
			endScope(fe->line, fe->col);
			for (int i = 0; i < streamCount; i++) {
				emitByte(OpCode::OP_POP, fe->line, fe->col);
			}
			for (int jump : loopStack.back().continueJumps)
				patchJump(jump);
			emitLoop(startAddr, fe->line, fe->col);
			patchJump(exitJump);
			scopeDepth--;
			while (locals.size() > 0 && locals.back().depth > scopeDepth)
				locals.pop_back();
			emitByte(OpCode::OP_POP, fe->line, fe->col);
			if (locals.size() > 0)
				locals.pop_back();
			for (int i = 0; i < streamCount; i++) {
				emitByte(OpCode::OP_POP, fe->line, fe->col);
				if (locals.size() > 0)
					locals.pop_back();
			}
			for (int b : loopStack.back().breakJumps)
				patchJump(b);
			loopStack.pop_back();
			scopeDepth--;
			break;
		}
		case StmtType::BREAK: {
			if (loopStack.empty())
				throw ControlFlowError("break outside of loop", s->line, s->col);
			int localsToPop =
				(int)locals.size() - loopStack.back().startLocalCount;
			for (int i = 0; i < localsToPop; i++)
				emitByte(OpCode::OP_POP, s->line, s->col);
			loopStack.back().breakJumps.push_back(
				emitJump(OpCode::OP_JUMP, s->line, s->col));
			break;
		}
		case StmtType::CONTINUE: {
			if (loopStack.empty())
				throw ControlFlowError("continue outside of loop", s->line,
					s->col);
			int localsToPop =
				(int)locals.size() - loopStack.back().startLocalCount;
			for (int i = 0; i < localsToPop; i++)
				emitByte(OpCode::OP_POP, s->line, s->col);
			int target = loopStack.back().stepAddress;
			if (target == -1) {
				int jump = emitJump(OpCode::OP_JUMP, s->line, s->col);
				loopStack.back().continueJumps.push_back(jump);
			} else
				emitLoop(target, s->line, s->col);
			break;
		}
		case StmtType::SKIP: {
			auto sk = static_cast<SkipStmt *>(s);
			if (loopStack.empty())
				throw ControlFlowError("skip statement outside of loop", s->line,
					s->col);
			if (!loopStack.back().isForEach) {
				throw ControlFlowError(
					"skip statement is only valid in for-each loops.", s->line,
					s->col);
			}
			compile(sk->count);
			emitByte(OpCode::OP_SKIP_ITER, s->line, s->col);
			int slot = loopStack.back().iteratorSlot;
			chunk->write((uint8_t)slot, s->line, s->col);
			break;
		}
		default:
			break;
		}
	}
	void emitIdentifier(OpCode op, const string &name, int line, int col) {
		int index = chunk->addConstant(Value::String(name));
		emitByte(op, line, col);
		chunk->write(static_cast<uint8_t>(index), line, col);
	}
	void emitLoop(int loopStart, int line, int col) {
		emitByte(OpCode::OP_LOOP, line, col);
		int offset = (int)chunk->code.size() - loopStart + 2;
		if (offset > 65535)
			throw RangeError("Loop body too large", line, col);
		chunk->write((offset >> 8) & 0xff, line, col);
		chunk->write(offset & 0xff, line, col);
	}
	void emitFunction(FuncStmt *f, bool isMethod = false, const vector<Stmt *> &fieldInits = {}) {
		Chunk *funcChunk = new Chunk();
		ByteCodeCompiler subCompiler(funcChunk);
		subCompiler.beginScope();
		vector<ParamSpec> actualParams;
		if (isMethod) {
			actualParams.push_back({"self", CopyMode::SHALLOW, ValueType::NOTYPE,
				nullptr, false, false, false});
			actualParams.push_back({"obj", CopyMode::SHALLOW, ValueType::NOTYPE,
				nullptr, false, false, false});
		}
		for (const auto &param : f->params)
			actualParams.push_back(param);
		for (const auto &param : actualParams)
			subCompiler.addLocal(param.name);
		for (auto *init : fieldInits)
			subCompiler.compileStmt(init);
		for (auto bodyStmt : f->body)
			subCompiler.compileStmt(bodyStmt);
		subCompiler.emitByte(OpCode::OP_NOTYPE, f->line, f->col);
		subCompiler.emitByte(OpCode::OP_RETURN, f->line, f->col);
		auto *funcObj = new FunctionObject(
			actualParams, f->returnType, f->defaultRetArgs, f->returnsConst,
			f->body, nullptr, f->isCached, funcChunk);
		funcObj->name = f->name;
		Value funcVal;
		funcVal.type = ValueType::FUNCTION;
#ifdef VM_DEBUG_MODE
		if (DEBUGGER_MODE_IS_ENABLED)
			funcVal.__DEBUGGING__NAME__ = funcObj->name;
#endif
		funcVal.ref = std::shared_ptr<HeapObject>(funcObj);
		emitConstant(funcVal, f->line, f->col);
	}
	bool tryExtractConstant(Expr *e, Value &outVal) {
		try {
			std::function<Value(Expr *)> extract = [&](Expr *expr) -> Value {
				if (expr->type == ExprType::NUMBER) {
					auto n = static_cast<NumberExpr *>(expr);
					return n->isFloat ? Value::Float(n->val) : Value::Int((long long)n->val);
				}
				if (expr->type == ExprType::STRING)
					return Value::String(static_cast<StringExpr *>(expr)->val);
				if (expr->type == ExprType::BOOL)
					return Value::Bool(static_cast<BoolExpr *>(expr)->value);
				if (expr->type == ExprType::VECTOR) {
					auto ve = static_cast<VectorExpr *>(expr);
					std::vector<Value> elems;
					for (auto *el : ve->elements)
						elems.push_back(extract(el));
					return Value::Vector(elems);
				}
				if (expr->type == ExprType::BINARY) {
					auto bin = static_cast<BinExpr *>(expr);
					return EvaluateConstBinary(bin->op, extract(bin->left), extract(bin->right));
				}
				if (expr->type == ExprType::VAR) {
					auto v = static_cast<VarExpr *>(expr);
					int arg = resolveLocal(v->name);
					if (arg != -1 && locals[arg].hasKnownValue) {
						return locals[arg].knownValue; // INJECT THE VALUE!
					}
				}
				throw std::runtime_error("Not a constant");
			};
			outVal = extract(e);
			return true;
		} catch (...) {
			return false;
		}
	}
};
// 1. Detect the Compiler
#if defined(__GNUC__) || defined(__clang__)
#define USE_COMPUTED_GOTOS
#endif
#ifdef VM_DEBUG_MODE
#define RUN_DEBUGGER()                                                                  \
	do {                                                                                 \
		Chunk *currentChunk = frame->function ? frame->function->chunk : &chunk;          \
		int currentOffset = (int)(ip - currentChunk->code.data());                        \
		std::cout << "LINE: " << std::left << std::setw(4) << line << " | ";              \
		std::cout << std::left << std::setw(18) << OpCodeToString((OpCode) * ip) << " | " \
					 << PrintStackForDebug(stack) << "\n";                                   \
		std::cout << "Stack Size: " << stack.size() << "\n";                              \
	} while (false)
#else
#define RUN_DEBUGGER() \
	do {                \
	} while (false)
#endif
// 2. Define the cross-platform OpCode label
#ifdef USE_COMPUTED_GOTOS
#define OP(name) TARGET_##name
#else
#define OP(name) case OpCode::name
#endif

// 3. Define the jump mechanism!
#ifdef USE_COMPUTED_GOTOS
#define DISPATCH()                                                      \
	do {                                                                 \
		RUN_DEBUGGER();                                                   \
		currentChunk = frame->function ? frame->function->chunk : &chunk; \
		instruction = static_cast<OpCode>(*ip++);                         \
		int offset = (int)(ip - currentChunk->code.data());               \
		line = currentChunk->lines[offset - 1];                           \
		col = currentChunk->columns[offset - 1];                          \
		goto *dispatch_table[static_cast<size_t>(instruction)];           \
	} while (false)
#else
#define DISPATCH() goto loop_start
#endif
// Helper: Attempts to synchronously execute a native dunder method.
// Returns a pair: <bool found, Value result>
static inline std::pair<bool, Value> tryCastDunder(Value v, const std::string &dunderName, int l, int c, std::shared_ptr<Env> globals, std::function<Value(MethodCallExpr *)> methodResolver, std::function<void(std::string, std::vector<std::string>)> importResolver);
struct VM {
	std::vector<Value> stack;
	std::shared_ptr<Env> globals;
	std::function<Value(MethodCallExpr *)> methodResolver;
	std::vector<CallFrame> frames;
	std::unordered_set<std::string> importStack;
	std::function<void(std::string, std::vector<std::string>)> importResolver;
	CallFrame *frame;
	uint8_t *ip;
	VM() {
		globals = std::make_shared<Env>();
		stack.reserve(65536);
		frame = nullptr;
		ip = nullptr;
	}
	void run(Chunk &chunk) {
		CallFrame mainFrame;
		mainFrame.function = nullptr;
		mainFrame.ip = chunk.code.data();
		mainFrame.basePointer = 0;
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
			&&TARGET_OP_ADD, &&TARGET_OP_SUB, &&TARGET_OP_MUL, &&TARGET_OP_DIV,
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

			// Containers
			&&TARGET_OP_BUILD_LIST, &&TARGET_OP_BUILD_TUPLE,
			&&TARGET_OP_BUILD_SET, &&TARGET_OP_BUILD_DICT,
			&&TARGET_OP_UNPACK_DICT, &&TARGET_OP_BUILD_RANGE,
			&&TARGET_OP_BUILD_VECTOR, &&TARGET_OP_BUILD_FSTRING,
			&&TARGET_OP_BUILD_FILE, &&TARGET_OP_BUILD_SLICE,

			// OOP
			&&TARGET_OP_CLASS, &&TARGET_OP_METHOD, &&TARGET_OP_GET_PROPERTY,
			&&TARGET_OP_SET_PROPERTY, &&TARGET_OP_CLASS_FIELD, &&TARGET_OP_SUPER,

			// Comprehension
			&&TARGET_OP_LIST_APPEND, &&TARGET_OP_SET_ADD, &&TARGET_OP_DICT_SET,
			&&TARGET_OP_LIST_TO_TUPLE, &&TARGET_OP_LIST_TO_VECTOR,

			// Access & Calls
			&&TARGET_OP_GET_INDEX, &&TARGET_OP_SET_INDEX, &&TARGET_OP_INVOKE,
			&&TARGET_OP_CALL,

			// Control Flow
			&&TARGET_OP_JUMP, &&TARGET_OP_JUMP_IF_FALSE, &&TARGET_OP_LOOP,
			&&TARGET_OP_RETURN, &&TARGET_OP_TO_STREAM, &&TARGET_OP_JUMP_IF_NOT_LT,
			&&TARGET_OP_BREAK, &&TARGET_OP_CONTINUE, &&TARGET_OP_SKIP,
			&&TARGET_OP_OMIT, &&TARGET_OP_FOR_ITER, &&TARGET_OP_SKIP_ITER,
			&&TARGET_OP_SWITCH_TABLE,

			// Errors & Systems
			&&TARGET_OP_THROW, &&TARGET_OP_ASSERT, &&TARGET_OP_IMPORT,
			&&TARGET_OP_POP, &&TARGET_OP_DEBUG_NAME, &&TARGET_OP_TRY_ENTER,
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
				currentChunk = frame->function ? frame->function->chunk : &chunk;
				RUN_DEBUGGER();
				instruction = static_cast<OpCode>(*ip++);
				int offset = (int)(ip - currentChunk->code.data());
				line = currentChunk->lines[offset - 1];
				col = currentChunk->columns[offset - 1];
				switch (instruction)
#endif
				{
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
									if (obj.ptr == nullptr) {
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
								} else
									throw AttributeError(
										"Only instances and classes have properties", line,
										col);
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
								if (obj.ptr == nullptr) {
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
								Value *ptr = &instance->fields[name];
								stack.push_back(Value::Reference(ptr, obj.ref));
							} else if (obj.type == ValueType::CLASS) {
								auto *cls = static_cast<ClassObject *>(obj.ref.get());
								Value *ptr = &cls->staticFields[name];
								stack.push_back(Value::Reference(ptr, obj.ref));
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
								if (obj.ptr == nullptr) {
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
							} else
								throw AttributeError("Cannot set property on non-object",
									line, col);
							stack.push_back(val);
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
								vector<Value> args;
								for (int i = 0; i < argCount; i++)
									args.insert(args.begin(), pop());
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
										if (count == 0 || isStar) {
											if (!globals->exists(key)) {
												globals->set(key, var.value, var.isLocked,
													var.isConst);
											}
										}
									}
									moduleResult = Value::Dict(exportDict->items);
								} catch (...) {
									importStack.erase(absPath);
									throw;
								}
								importStack.erase(absPath);
							} else {
								if (this->importResolver) {
									this->importResolver(libName, symbols);
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
							Value val = stack[frame->basePointer + slot];
							if (val.type == ValueType::REFERENCE)
								stack.push_back(*val.ptr);
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
								stack.push_back(Value::Reference(ptr, nullptr));
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
								ptr = &list->elements[idx];
							} else if (base.type == ValueType::VECTOR) {
								auto *vec = static_cast<VectorObject *>(base.ref.get());
								long long idx = index.asInt();
								ptr = &vec->elements[idx];
							} else
								throw OwnershipError("Cannot take reference of this type",
									line, col);
							stack.push_back(Value::Reference(ptr, owner));
						}
						DISPATCH();
					}
					OP(OP_INC_LOCAL) : {
						{
							uint8_t slot = *ip++;
							int idx = frame->basePointer + slot;
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
										int digit = divMod10(tempChunks);
										result += (floatBase * digit * powerOf10);
										powerOf10 *= 10.0;
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
							const Value &valA = (a.type == ValueType::REFERENCE && a.ptr) ? *a.ptr : a;
							const Value &valB = (b.type == ValueType::REFERENCE && b.ptr) ? *b.ptr : b;
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
							const Value &valA = (a.type == ValueType::REFERENCE && a.ptr) ? *a.ptr : a;
							const Value &valB = (b.type == ValueType::REFERENCE && b.ptr) ? *b.ptr : b;
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
							Value step = pop();
							Value end = pop();
							Value start = pop();
							bool isFloat = (start.type == ValueType::FLOAT || end.type == ValueType::FLOAT || step.type == ValueType::FLOAT);
							stack.push_back(Value::Range(start.asFloat(), end.asFloat(),
								step.asFloat(), true, false,
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
								if (!base.ptr)
									throw RuntimeError("Dead-end reference", line, col);
								base = *base.ptr;
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
							if (refVal.type != ValueType::REFERENCE || !refVal.ptr)
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
													executeDefault(func->defaultRetArgs[0], line);
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
									if (func->isCached)
										func->cache[frame->cacheKey] = result;
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
					OP(OP_MULTI_SET) : OP(OP_AND) : OP(OP_OR) : OP(OP_IN) : OP(OP_INCREMENT) : OP(OP_DECREMENT) : OP(OP_BUILD_FILE) : OP(OP_BREAK) : OP(OP_CONTINUE) : OP(OP_SKIP) : OP(OP_OMIT) : OP(OP_ASSERT) : {
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
							std::cerr << "Uncaught " << e.type << ": " << e.message << "\n";
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

 private:
	void callValue(Value callee, int argCount, int line, int col) {
		FunctionObject *function = nullptr;
		NativeFunctionObject *nativeObj = nullptr;
		if (callee.type == ValueType::FUNCTION) {
			function = static_cast<FunctionObject *>(callee.ref.get());
		} else if (callee.type == ValueType::NATIVE_FUNCTION) {
			nativeObj = static_cast<NativeFunctionObject *>(callee.ref.get());
		} else if (callee.type == ValueType::OVERLOAD) {
			auto *ov = static_cast<OverloadObject *>(callee.ref.get());
			bool found = false;
			FunctionObject *bestFunc = nullptr;
			NativeFunctionObject *bestNative = nullptr;
			int bestScore = -1;
			for (int i = (int)ov->overloads.size() - 1; i >= 0; i--) {
				Value candVal = ov->overloads[i];
				if (candVal.type == ValueType::FUNCTION) {
					auto *candidate =
						static_cast<FunctionObject *>(candVal.ref.get());
					size_t minArgs = 0;
					bool isVariadic = false;
					for (const auto &p : candidate->params) {
						if (p.isVariadic || p.isKwargs)
							isVariadic = true;
						else if (p.defaultValue == nullptr)
							minArgs++;
					}
					if (argCount < minArgs && !isVariadic)
						continue;
					if (!isVariadic && argCount > candidate->params.size())
						continue;
					bool typesMatch = true;
					int currentScore = 0;
					size_t paramIdx = 0;
					for (int argIdx = 0; argIdx < argCount; argIdx++) {
						if (paramIdx >= candidate->params.size()) {
							if (!isVariadic) {
								typesMatch = false;
								break;
							}
							currentScore++;
							break;
						}
						const ParamSpec &p = candidate->params[paramIdx];
						if (p.isVariadic || p.isKwargs)
							break;
						Value argVal = stack[stack.size() - argCount + argIdx];
						ValueType checkType = argVal.type;
						if (argVal.type == ValueType::REFERENCE && argVal.ptr != nullptr) {
							checkType = argVal.ptr->type;
						}
						if (p.type != ValueType::NOTYPE && checkType != p.type) {
							if (p.type == ValueType::FLOAT && checkType == ValueType::INT) {
								currentScore += 5;
							} else {
								typesMatch = false;
								break;
							}
						} else if (p.type != ValueType::NOTYPE && checkType == p.type) {
							currentScore += 10;
						} else if (p.type == ValueType::NOTYPE) {
							currentScore += 2;
						}
						paramIdx++;
					}
					if (typesMatch && currentScore > bestScore) {
						bestScore = currentScore;
						bestFunc = candidate;
						bestNative = nullptr;
					}
				} else if (candVal.type == ValueType::NATIVE_FUNCTION) {
					if (0 > bestScore) {
						bestScore = 0;
						bestFunc = nullptr;
						bestNative = static_cast<NativeFunctionObject *>(candVal.ref.get());
					}
				}
			}
			if (bestFunc) {
				function = bestFunc;
				found = true;
			} else if (bestNative) {
				nativeObj = bestNative;
				found = true;
			}
			if (!found)
				throw TypeError(
					"No matching overload found with provided arguments", line,
					col);
		} else
			throw TypeError("Object is not callable", line, col);
		if (nativeObj) {
			vector<Value> args;
			for (int i = 0; i < argCount; i++)
				args.push_back(pop());
			std::reverse(args.begin(), args.end());
			stack.push_back(nativeObj->func(args, line, 0));
			return;
		}
		vector<Value> providedArgs;
		providedArgs.resize(argCount);
		for (int i = argCount - 1; i >= 0; i--)
			providedArgs[i] = pop();
		// Debugging: Verify we have params
		// std::cout << "DEBUG: Call " << argCount << " args. Func expects " <<
		// function->params.size() << "\n";
		vector<Value> finalArgs;
		finalArgs.resize(function->params.size());
		size_t argIndex = 0;
		std::vector<bool> consumed(providedArgs.size(), false);
		for (size_t i = 0; i < function->params.size(); i++) {
			const ParamSpec &p = function->params[i];
			Value argVal = Value::None();
			if (p.isKwargs) {
				auto dict = std::make_shared<DictObject>();
				while (argIndex < providedArgs.size()) {
					Value v = providedArgs[argIndex];
					if (v.type != ValueType::PAIRED)
						throw SyntaxError("Positional argument after keyword args",
							line, col);
					auto *pairObj = static_cast<PairedObject *>(v.ref.get());
					for (const auto &entry : pairObj->pairs) {
						Value key = entry.first;
						Value val = entry.second;
						if (p.mode == CopyMode::DEEP) {
							key = deepCopy(key);
							val = deepCopy(val);
						}
						dict->items[key] = val;
					}
					consumed[argIndex] = true;
					argIndex++;
				}
				argVal = Value::Dict(dict->items);
				if (p.isConst)
					argVal.isConst = true;
			} else if (p.isVariadic) {
				vector<Value> tupleItems;
				while (argIndex < providedArgs.size()) {
					if (providedArgs[argIndex].type == ValueType::PAIRED)
						break;
					Value v = providedArgs[argIndex];
					if (p.mode == CopyMode::DEEP)
						v = deepCopy(v);
					tupleItems.push_back(v);
					consumed[argIndex] = true;
					argIndex++;
				}
				argVal = Value::Tuple(tupleItems);
			} else {
				bool foundValue = false;
				if (argIndex < providedArgs.size() &&
					 providedArgs[argIndex].type != ValueType::PAIRED) {
					if (providedArgs[argIndex].type == ValueType::OMIT_MARKER) {
						if (p.defaultValue != nullptr) {
							argVal = executeDefault(p.defaultValue,
								line); // <--- EXECUTE AST
							foundValue = true;
						} else
							throw ArgumentError(
								"Argument '" + p.name + "' cannot be omitted.", line,
								col);
					} else {
						argVal = providedArgs[argIndex];
						if (p.mode == CopyMode::DEEP)
							argVal = deepCopy(argVal);
						else if (p.mode == CopyMode::SHALLOW)
							argVal = shallowCopy(argVal);
						foundValue = true;
					}
					consumed[argIndex] = true;
					argIndex++;
				}
				if (!foundValue) {
					for (size_t k = argIndex; k < providedArgs.size(); k++) {
						if (!consumed[k] && providedArgs[k].type == ValueType::PAIRED) {
							auto *pairObj = static_cast<PairedObject *>(providedArgs[k].ref.get());
							for (const auto &entry : pairObj->pairs) {
								if (entry.first.asString() == p.name) {
									argVal = entry.second;
									if (p.mode == CopyMode::DEEP)
										argVal = deepCopy(argVal);
									else if (p.mode == CopyMode::SHALLOW)
										argVal = shallowCopy(argVal);
									foundValue = true;
									consumed[k] = true;
									break;
								}
							}
						}
						if (foundValue)
							break;
					}
				}
				if (!foundValue) {
					if (p.defaultValue != nullptr)
						argVal = executeDefault(p.defaultValue, line);
					else
						throw ArgumentError(
							"Missing required argument '" + p.name + "'", line, col);
				}
				if (p.type != ValueType::NOTYPE && argVal.type != p.type) {
					bool mismatch = true;
					if (p.type == ValueType::FLOAT &&
						 argVal.type == ValueType::INT) {
						argVal = Value::Float((double)argVal.asInt());
						mismatch = false;
					} else if (p.type == ValueType::INT &&
								  argVal.type == ValueType::FLOAT) {
						argVal = Value::Int((long long)argVal.asFloat());
						mismatch = false;
					} else if (p.type == ValueType::INT &&
								  argVal.type == ValueType::BIGINT) {
						mismatch = false;
					} else if (p.type == ValueType::BIGINT &&
								  argVal.type == ValueType::INT) {
						argVal = Value::BigInt(argVal.asInt());
						mismatch = false;
					} else if (argVal.type == ValueType::REFERENCE && argVal.ptr != nullptr && argVal.ptr->type == p.type) {
						mismatch = false;
					} else if (argVal.type == ValueType::OMIT_MARKER) {
						mismatch = false;
					}
					if (mismatch) {
						throw TypeError("Type mismatch for '" + p.name +
												 "'. Expected " +
												 std::to_string((int)p.type) + " got " +
												 std::to_string((int)argVal.type),
							line, col);
					}
				}
			}
			finalArgs[i] = argVal;
		}
		if (function->isCached) {
			if (function->cache.count(finalArgs)) {
				stack.push_back(function->cache[finalArgs]);
				return;
			}
		}
		for (size_t k = 0; k < providedArgs.size(); k++) {
			if (!consumed[k]) {
				throw ArgumentError("Too many arguments or unexpected keyword argument passed.", line, col);
			}
		}
		if (frame)
			frame->ip = ip;
		CallFrame newFrame;
		newFrame.function = function;
		newFrame.ip = function->chunk->code.data();
		newFrame.basePointer = stack.size();
		if (function->isCached)
			newFrame.cacheKey = finalArgs;
		for (const auto &v : finalArgs)
			stack.push_back(v);
		frames.push_back(newFrame);
		frame = &frames.back();
		ip = frame->ip;
	}
	Value executeDefault(Expr *expr, int line) {
		if (!expr)
			return Value::None();
		Chunk tempChunk;
		ByteCodeCompiler compiler(&tempChunk);
		compiler.compile(expr);
		compiler.emitByte(OpCode::OP_RETURN, line, 0);
		VM tempVM;
		tempVM.globals = this->globals;
		tempVM.methodResolver = this->methodResolver;
		tempVM.run(tempChunk);
		if (tempVM.stack.empty())
			return Value::None();
		return tempVM.stack.back();
	}
	Value prepareIterable(Value collection, int line, int col) {
		if (collection.type == ValueType::LIST ||
			 collection.type == ValueType::SET ||
			 collection.type == ValueType::TUPLE ||
			 collection.type == ValueType::STRING ||
			 collection.type == ValueType::VECTOR ||
			 collection.type == ValueType::RANGE) {
			return collection;
		}
		if (collection.type == ValueType::REFERENCE) {
			return *collection.ptr;
		}
		auto list = std::make_shared<ListObject>();
		if (collection.type == ValueType::DICT) {
			auto *dict = static_cast<DictObject *>(collection.ref.get());
			for (const auto &pair : dict->items)
				list->elements.push_back(pair.first);
		} else
			throw TypeError("Object is not iterable", line, col);
		return Value::List(list->elements);
	}
	inline Value pop() {
		// if (stack.empty()) throw UnderflowError("Stack underflow", 0, 0);
		Value v = stack.back();
		stack.pop_back();
		return v;
	}
};
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
												std::vector<std::string> sym) {
				if (this->modules.count(lib))
					this->modules[lib](this->env, sym);
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

	auto iRes = [this](std::string lib, std::vector<std::string> sym) {
		if (this->modules.count(lib)) {
			this->modules[lib](this->env, sym);
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
		if (!target.ptr)
			throw RuntimeError("Dead-end reference", m->line, m->col);
		target = *target.ptr;
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
				return safeSub(term1, term2);
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
	return Value::None();
}
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
			Vector2 size = MeasureTextEx(
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
		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Raylib", modVal, false, false);
	};
#ifndef _WIN32
	modules["Ncurses"] = [](std::shared_ptr<Env> env, const vector<string>& symbols) {
		auto moduleNamespace = std::make_shared<ClassObject>("Ncurses");
		moduleNamespace->mro.push_back(moduleNamespace.get());
		auto define = [&](string name, NativeFunc f) {
			moduleNamespace->staticFields[name] = Value::Native(f);
			if (symbols.size() == 1 && symbols[0] == "*") {
				env->set(name, Value::Native(f), true);
				return;
			}
			for (const auto& s : symbols)
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
		define("InitScr", [](const vector<Value>& args, int l, int c) {
			setlocale(LC_ALL, "");
			initscr();
			cbreak();
			noecho();
			keypad(stdscr, true);
			return Value::None();
			});
		define("EndWin", [](const vector<Value>& args, int l, int c) {
			endwin();
			return Value::None();
			});
		define("Print", [](const vector<Value>& args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("Print(string)", l, c);
			printw("%s", args[0].asString().c_str());
			return Value::None();
			});
		define("MovePrint", [](const vector<Value>& args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("MovePrint(x, y, string)", l, c);
			mvprintw((int)args[1].asInt(), (int)args[0].asInt(), "%s",
				args[2].asString().c_str());
			return Value::None();
			});
		define("Refresh", [](const vector<Value>& args, int l, int c) {
			refresh();
			return Value::None();
			});
		define("Clear", [](const vector<Value>& args, int l, int c) {
			clear();
			return Value::None();
			});
		define("GetCh", [](const vector<Value>& args, int l, int c) {
			return Value::Int(getch());
			});
		define("GetMaxX", [](const vector<Value>& args, int l, int c) {
			return Value::Int(getmaxx(stdscr));
			});
		define("GetMaxY", [](const vector<Value>& args, int l, int c) {
			return Value::Int(getmaxy(stdscr));
			});
		define("CursSet", [](const vector<Value>& args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError(
					"CursSet(visibility: 0=invisible, 1=normal, 2=bright)", l, c);
			curs_set((int)args[0].asInt());
			return Value::None();
			});
		define("NoDelay", [](const vector<Value>& args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("NoDelay(bool)", l, c);
			nodelay(stdscr, args[0].asBool());
			return Value::None();
			});
		define("StartColor", [](const vector<Value>& args, int l, int c) {
			start_color();
			return Value::None();
			});
		define("InitPair", [](const vector<Value>& args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("InitPair(pair_id, fg_color, bg_color)", l, c);
			init_pair((short)args[0].asInt(), (short)args[1].asInt(),
				(short)args[2].asInt());
			return Value::None();
			});
		define("ColorPair", [](const vector<Value>& args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ColorPair(pair_id)", l, c);
			return Value::Int(COLOR_PAIR((int)args[0].asInt()));
			});
		define("AttrOn", [](const vector<Value>& args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("AttrOn(attribute_or_color)", l, c);
			attron((int)args[0].asInt());
			return Value::None();
			});
		define("AttrOff", [](const vector<Value>& args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("AttrOff(attribute_or_color)", l, c);
			attroff((int)args[0].asInt());
			return Value::None();
			});
		define("DrawBox", [](const vector<Value>& args, int l, int c) {
			box(stdscr, 0, 0);
			return Value::None();
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
			if (!res)
				return Value::None();
			auto inst = std::make_shared<InstanceObject>(responseClass.get());
			inst->fields["status"] = Value::Int(res->status);
			inst->fields["body"] = Value::String(res->body);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};
		define("Get", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("Http.Get(host, path)", l, c);
			httplib::Client cli(args[0].asString());
			return ResultToValue(cli.Get(args[1].asString()));
		});
		define("Post", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("Http.Post(host, path, body, contentType)", l,
					c);
			httplib::Client cli(args[0].asString());
			return ResultToValue(cli.Post(args[1].asString(), args[2].asString(),
				args[3].asString()));
		});
		define("Put", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("Http.Put(host, path, body, contentType)", l,
					c);
			httplib::Client cli(args[0].asString());
			return ResultToValue(cli.Put(args[1].asString(), args[2].asString(),
				args[3].asString()));
		});
		define("Delete", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("Http.Delete(host, path)", l, c);
			httplib::Client cli(args[0].asString());
			return ResultToValue(cli.Delete(args[1].asString()));
		});
		static httplib::Server svr;
		auto bindRoute = [this, env](const string &method, const string &path,
								  Value lambda, int l, int c) {
			auto handler = [=](const httplib::Request &req,
									httplib::Response &res) {
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
				tempVM.methodResolver = [&](MethodCallExpr *m) {
					return this->Resolve_methods(m);
				};
				try {
					tempVM.run(tempChunk);
					Value ret =
						tempVM.stack.empty() ? Value::None() : tempVM.stack.back();
					if (ret.type == ValueType::STRING) {
						res.set_content(ret.asString(), "application/json");
					} else {
						res.set_content("Success", "text/plain");
					}
				} catch (...) {
					res.status = 500;
					res.set_content("Internal Server Error in y_lang VM",
						"text/plain");
				}
			};
			if (method == "GET")
				svr.Get(path, handler);
			else if (method == "POST")
				svr.Post(path, handler);
		};
		define("ServerGet", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2 || args[1].type != ValueType::FUNCTION)
				throw ArgumentError("Http.ServerGet(path, lambda)", l, c);
			bindRoute("GET", args[0].asString(), args[1], l, c);
			return Value::None();
		});
		define("ServerPost", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2 || args[1].type != ValueType::FUNCTION)
				throw ArgumentError("Http.ServerPost(path, lambda)", l, c);
			bindRoute("POST", args[0].asString(), args[1], l, c);
			return Value::None();
		});
		define("Listen", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("Http.Listen(host, port)", l, c);
			string host = args[0].asString();
			int port = (int)args[1].asInt();
			std::cout << "[ymm Server] Listening on " << host << ":" << port
						 << "...\n";
			svr.listen(host.c_str(), port);
			return Value::None();
		});
		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Http", modVal, false, false);
	};
	// ========= CASTING ==========
	env->set("int", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.empty())
			return Value::Int(0);
		auto mRes = [this](MethodCallExpr *m) {
			return this->Resolve_methods(m);
		};
		auto iRes = [this](std::string lib, std::vector<std::string> sym) {
			if (this->modules.count(lib)) {
				this->modules[lib](this->env, sym);
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
		auto iRes = [this](std::string lib, std::vector<std::string> sym) {
			if (this->modules.count(lib)) {
				this->modules[lib](this->env, sym);
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
		auto iRes = [this](std::string lib, std::vector<std::string> sym) {
			if (this->modules.count(lib)) {
				this->modules[lib](this->env, sym);
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
		auto iRes = [this](std::string lib, std::vector<std::string> sym) {
			if (this->modules.count(lib)) {
				this->modules[lib](this->env, sym);
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
			auto iRes = [this](std::string lib, std::vector<std::string> sym) {
				if (this->modules.count(lib)) {
					this->modules[lib](this->env, sym);
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
			auto iRes = [this](std::string lib, std::vector<std::string> sym) {
				if (this->modules.count(lib)) {
					this->modules[lib](this->env, sym);
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
			auto iRes = [this](std::string lib, std::vector<std::string> sym) {
				if (this->modules.count(lib)) {
					this->modules[lib](this->env, sym);
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
			auto iRes = [this](std::string lib, std::vector<std::string> sym) {
				if (this->modules.count(lib)) {
					this->modules[lib](this->env, sym);
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
			auto iRes = [this](std::string lib, std::vector<std::string> sym) {
				if (this->modules.count(lib)) {
					this->modules[lib](this->env, sym);
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
				if (!v.ptr)
					throw RuntimeError("Null reference in dict()", l, c);
				v = *v.ptr;
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
		auto iRes = [this](std::string lib, std::vector<std::string> sym) {
			if (this->modules.count(lib))
				this->modules[lib](this->env, sym);
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
		auto *val1 = args[0].ptr;
		auto *val2 = args[1].ptr;
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

		auto iRes = [this](std::string lib, std::vector<std::string> sym) {
			if (this->modules.count(lib)) {
				this->modules[lib](this->env, sym);
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

		auto iRes = [this](std::string lib, std::vector<std::string> sym) {
			if (this->modules.count(lib)) {
				this->modules[lib](this->env, sym);
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
			throw TypeError("cppCompile expects (String code, Value return_type, Dict args)", l, c);
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
			cpp << "#include \"value_for_cpp_compile.h\"\n";
			cpp << "#include <string>\n";
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
			std::string cpp_file = "y_macro_" + hash_str + ".cpp";
			std::string lib_file = "y_macro_" + hash_str + ".dll";
			std::string cmd = "g++ -std=c++17 -shared -O3 -fopenmp " + debug_flag + "-I\"" + current_dir + "\" " + cpp_file + " -o " + lib_file + " 2>&1";
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
			HINSTANCE handle = LoadLibraryA(lib_file.c_str());
			if (!handle)
				throw RuntimeError("Failed to load DLL", l, c);
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
inline std::pair<bool, Value> tryCastDunder(Value v, const std::string &dunderName, int l, int c, std::shared_ptr<Env> globals, std::function<Value(MethodCallExpr *)> methodResolver, std::function<void(std::string, std::vector<std::string>)> importResolver) {
	while (v.type == ValueType::REFERENCE) {
		if (!v.ptr)
			throw RuntimeError("Attempted to dereference a null pointer!", l, c);
		v = *v.ptr;
	}

	if (v.type == ValueType::INSTANCE) {
		auto *inst = static_cast<InstanceObject *>(v.ref.get());
		ClassObject *cls = inst->klass;
		ClassObject::MethodInfo *dunderMethod = nullptr;
		ClassObject *methodOwner = nullptr;

		for (auto *ancestor : cls->mro) {
			if (ancestor->methods.count(dunderName)) {
				dunderMethod = &ancestor->methods[dunderName];
				methodOwner = ancestor;
				break;
			}
		}

		if (dunderMethod) {
			Value classObjVal;
			classObjVal.type = ValueType::CLASS;
			classObjVal.ref = std::shared_ptr<HeapObject>(methodOwner, [](HeapObject *) {});

			if (dunderMethod->func.type == ValueType::NATIVE_FUNCTION) {
				auto *nat = static_cast<NativeFunctionObject *>(dunderMethod->func.ref.get());
				std::vector<Value> args = {v, classObjVal};
				return {true, nat->func(args, l, c)};
			} else {
				// ---> INJECT THE CONTEXT INTO THE TEMP VM <---
				VM tempVM;
				tempVM.globals = globals;
				tempVM.methodResolver = methodResolver;
				tempVM.importResolver = importResolver;

				Chunk tempChunk;
				int selfIdx = tempChunk.addConstant(v);
				tempChunk.write(OpCode::OP_CONSTANT, l, c);
				tempChunk.write((uint8_t)selfIdx, l, c);

				int objIdx = tempChunk.addConstant(classObjVal);
				tempChunk.write(OpCode::OP_CONSTANT, l, c);
				tempChunk.write((uint8_t)objIdx, l, c);

				int methIdx = tempChunk.addConstant(dunderMethod->func);
				tempChunk.write(OpCode::OP_CONSTANT, l, c);
				tempChunk.write((uint8_t)methIdx, l, c);

				tempChunk.write(OpCode::OP_CALL, l, c);
				tempChunk.write((uint8_t)2, l, c);
				tempChunk.write(OpCode::OP_RETURN, l, c);

				try {
					tempVM.run(tempChunk);
					if (!tempVM.stack.empty())
						return {true, tempVM.stack.back()};
				} catch (...) {
					throw;
				}
				return {true, Value::None()};
			}
		}
	}
	return {false, v};
}
inline bool lessValue(const Value &a, const Value &b, std::shared_ptr<Env> globals, std::function<Value(MethodCallExpr *)> methodResolver, std::function<void(std::string, std::vector<std::string>)> importResolver) {
	if (a.type == ValueType::REFERENCE) {
		if (b.type == ValueType::REFERENCE) {
			return lessValue(*a.ptr, *b.ptr, globals, methodResolver, importResolver);
		} else {
			return lessValue(*a.ptr, b, globals, methodResolver, importResolver);
		}
	} else if (b.type == ValueType::REFERENCE) {
		return lessValue(a, *b.ptr, globals, methodResolver, importResolver);
	}
	if (a.type == ValueType::INSTANCE) {
		auto *instA = static_cast<InstanceObject *>(a.ref.get());
		ClassObject *clsA = instA->klass;
		ClassObject::MethodInfo *lessMethod = nullptr;
		ClassObject *methodOwner = nullptr;

		for (auto *ancestor : clsA->mro) {
			if (ancestor->methods.count("__less__")) {
				lessMethod = &ancestor->methods["__less__"];
				methodOwner = ancestor;
				break;
			}
		}

		if (lessMethod) {
			Value classObjVal;
			classObjVal.type = ValueType::CLASS;
			classObjVal.ref = std::shared_ptr<HeapObject>(methodOwner, [](HeapObject *) {});

			// PATH A: Native C++ Dunder
			if (lessMethod->func.type == ValueType::NATIVE_FUNCTION) {
				auto *nat = static_cast<NativeFunctionObject *>(lessMethod->func.ref.get());
				std::vector<Value> args = {a, classObjVal, b};
				return nat->func(args, 0, 0).isTruthy();
			}
			// PATH B: y-- Bytecode Dunder (Temp VM)
			else {
				VM tempVM;
				tempVM.globals = globals;
				tempVM.methodResolver = methodResolver;
				tempVM.importResolver = importResolver;

				Chunk tempChunk;

				// Push self
				int selfIdx = tempChunk.addConstant(a);
				tempChunk.write(OpCode::OP_CONSTANT, 0, 0);
				tempChunk.write((uint8_t)selfIdx, 0, 0);

				// Push classObj
				int objIdx = tempChunk.addConstant(classObjVal);
				tempChunk.write(OpCode::OP_CONSTANT, 0, 0);
				tempChunk.write((uint8_t)objIdx, 0, 0);

				// Push other (b)
				int otherIdx = tempChunk.addConstant(b);
				tempChunk.write(OpCode::OP_CONSTANT, 0, 0);
				tempChunk.write((uint8_t)otherIdx, 0, 0);

				// Push method
				int methIdx = tempChunk.addConstant(lessMethod->func);
				tempChunk.write(OpCode::OP_CONSTANT, 0, 0);
				tempChunk.write((uint8_t)methIdx, 0, 0);

				// Call (self, class, other)
				tempChunk.write(OpCode::OP_CALL, 0, 0);
				tempChunk.write((uint8_t)3, 0, 0);
				tempChunk.write(OpCode::OP_RETURN, 0, 0);

				try {
					tempVM.run(tempChunk);
					if (!tempVM.stack.empty())
						return tempVM.stack.back().isTruthy();
				} catch (...) {
					throw;
				} // Let user crashes propagate cleanly
				return false;
			}
		}
	}
	if (a.type != b.type) {
		bool aIsNum = (a.type == ValueType::INT || a.type == ValueType::FLOAT || a.type == ValueType::BIGINT);
		bool bIsNum = (b.type == ValueType::INT || b.type == ValueType::FLOAT || b.type == ValueType::BIGINT);
		if (aIsNum && bIsNum) {
			if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
				return a.asFloat() < b.asFloat();
			}
			BigIntObject tempA(0), tempB(0);
			BigIntObject *ba = (a.type == ValueType::BIGINT) ? static_cast<BigIntObject *>(a.ref.get()) : &(tempA = BigIntObject(a.asInt()));
			BigIntObject *bb = (b.type == ValueType::BIGINT) ? static_cast<BigIntObject *>(b.ref.get()) : &(tempB = BigIntObject(b.asInt()));
			return *ba < *bb;
		}
		return a.type < b.type;
	}
	switch (a.type) {
	case ValueType::INT:
		return a.asInt() < b.asInt();
	case ValueType::FLOAT:
		return a.asFloat() < b.asFloat();
	case ValueType::STRING:
		return a.asString() < b.asString();
	case ValueType::BOOL:
		return a.asBool() < b.asBool();
	case ValueType::BIGINT: {
		auto *b1 = static_cast<BigIntObject *>(a.ref.get());
		auto *b2 = static_cast<BigIntObject *>(b.ref.get());
		return *b1 < *b2;
	}
	case ValueType::TUPLE: {
		auto *t1 = static_cast<TupleObject *>(a.ref.get());
		auto *t2 = static_cast<TupleObject *>(b.ref.get());
		if (t1->elements.empty() && t2->elements.empty())
			return false;
		if (t1->elements.empty())
			return true;
		if (t2->elements.empty())
			return false;
		return lessValue(t1->elements[0], t2->elements[0], globals, methodResolver, importResolver);
	}
	case ValueType::VECTOR: {
		auto *v1 = static_cast<VectorObject *>(a.ref.get());
		auto *v2 = static_cast<VectorObject *>(b.ref.get());
		bool hasFloat = false;
		for (const auto &val : v1->elements)
			if (val.type == ValueType::FLOAT)
				hasFloat = true;
		for (const auto &val : v2->elements)
			if (val.type == ValueType::FLOAT)
				hasFloat = true;
		if (hasFloat) {
			double mag1 = 0.0, mag2 = 0.0;
			for (const auto &val : v1->elements) {
				double v = val.asFloat();
				mag1 += (v * v);
			}
			for (const auto &val : v2->elements) {
				double v = val.asFloat();
				mag2 += (v * v);
			}
			return mag1 < mag2;
		} else {
			Value mag1 = Value::Int(0);
			for (const auto &val : v1->elements) {
				Value sq = BigIntObject::mul(val, val);
				mag1 = BigIntObject::add(mag1, sq);
			}
			Value mag2 = Value::Int(0);
			for (const auto &val : v2->elements) {
				Value sq = BigIntObject::mul(val, val);
				mag2 = BigIntObject::add(mag2, sq);
			}
			return lessValue(mag1, mag2, globals, methodResolver, importResolver);
		}
	}
	case ValueType::LIST: {
		auto *l1 = static_cast<ListObject *>(a.ref.get());
		auto *l2 = static_cast<ListObject *>(b.ref.get());
		return std::lexicographical_compare(
			l1->elements.begin(), l1->elements.end(), l2->elements.begin(),
			l2->elements.end(),
			// Recursive call for elements
			[&](const Value &x, const Value &y) { return lessValue(x, y, globals, methodResolver, importResolver); });
	}
	default:
		return false;
	}
}
inline Value EvaluateConstBinary(TokenType op, const Value& a, const Value& b) {
	if (op == TokenType::PLUS) {
		if (a.type == ValueType::INT && b.type == ValueType::INT) {
			long long res = a.iVal + b.iVal;
			bool overflow = ((a.iVal ^ res) & (b.iVal ^ res)) < 0;
			if (overflow)
				return BigIntObject::add(Value::BigInt(a.iVal), Value::BigInt(b.iVal));
			else
				return Value::Int(res);
		}
		else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
			return BigIntObject::add(a, b);
		}
		else if (a.type == ValueType::STRING || b.type == ValueType::STRING) {
			return Value::String(valueToString(a) + valueToString(b));
		}
		else if (a.type == ValueType::VECTOR && b.type == ValueType::VECTOR) {
			auto* v1 = static_cast<VectorObject*>(a.ref.get());
			auto* v2 = static_cast<VectorObject*>(b.ref.get());
			if (v1->elements.size() != v2->elements.size())
				throw std::runtime_error("Vector dimension mismatch");
			std::vector<Value> res;
			res.reserve(v1->elements.size());
			for (size_t i = 0; i < v1->elements.size(); i++) {
				Value x = v1->elements[i];
				Value y = v2->elements[i];
				if (x.type == ValueType::INT && y.type == ValueType::INT) {
					long long r = x.iVal + y.iVal;
					bool ovf = ((x.iVal ^ r) & (y.iVal ^ r)) < 0;
					if (ovf)
						res.push_back(BigIntObject::add(Value::BigInt(x.iVal), Value::BigInt(y.iVal)));
					else
						res.push_back(Value::Int(r));
				}
				else if (x.type == ValueType::BIGINT || y.type == ValueType::BIGINT) {
					res.push_back(BigIntObject::add(x, y));
				}
				else {
					res.push_back(Value::Float(x.asFloat() + y.asFloat()));
				}
			}
			return Value::Vector(res);
		}
		else if (a.isNumber() || b.isNumber()) {
			return Value::Float(a.asFloat() + b.asFloat());
		}
	}
	else if (op == TokenType::MINUS) {
		if (a.type == ValueType::INT && b.type == ValueType::INT) {
			long long res = a.iVal - b.iVal;
			bool overflow = ((a.iVal ^ b.iVal) & (a.iVal ^ res)) < 0;
			if (overflow)
				return BigIntObject::sub(Value::BigInt(a.iVal), Value::BigInt(b.iVal));
			else
				return Value::Int(res);
		}
		else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
			return BigIntObject::sub(a, b);
		}
		else if (a.type == ValueType::VECTOR && b.type == ValueType::VECTOR) {
			auto* v1 = static_cast<VectorObject*>(a.ref.get());
			auto* v2 = static_cast<VectorObject*>(b.ref.get());
			if (v1->elements.size() != v2->elements.size())
				throw std::runtime_error("Vector dimension mismatch");
			std::vector<Value> res;
			res.reserve(v1->elements.size());
			for (size_t i = 0; i < v1->elements.size(); i++) {
				Value x = v1->elements[i];
				Value y = v2->elements[i];
				if (x.type == ValueType::INT && y.type == ValueType::INT) {
					long long r = x.iVal - y.iVal;
					bool ovf = ((x.iVal ^ y.iVal) & (x.iVal ^ r)) < 0;
					if (ovf)
						res.push_back(BigIntObject::sub(Value::BigInt(x.iVal), Value::BigInt(y.iVal)));
					else
						res.push_back(Value::Int(r));
				}
				else if (x.type == ValueType::BIGINT || y.type == ValueType::BIGINT) {
					res.push_back(BigIntObject::sub(x, y));
				}
				else {
					res.push_back(Value::Float(x.asFloat() - y.asFloat()));
				}
			}
			return Value::Vector(res);
		}
		else if (a.isNumber() || b.isNumber()) {
			return Value::Float(a.asFloat() - b.asFloat());
		}
	}
	else if (op == TokenType::SLASH) {
		if (a.type == ValueType::VECTOR) {
			if (!b.isNumber())
				throw std::runtime_error("Vector can only be divided by a number");
			double s = b.asFloat();
			if (s == 0.0)
				throw std::runtime_error("Vector division by zero");
			auto* v = static_cast<VectorObject*>(a.ref.get());
			std::vector<Value> res;
			res.reserve(v->elements.size());
			for (const auto& elem : v->elements) {
				res.push_back(Value::Float(elem.asFloat() / s));
			}
			return Value::Vector(res);
		}
		else if (b.type == ValueType::VECTOR) {
			throw std::runtime_error("Cannot divide by a vector");
		}
		else if (a.isNumber() && b.isNumber()) {
			double db = b.asFloat();
			if (db == 0.0)
				throw std::runtime_error("Division by zero");
			return Value::Float(a.asFloat() / db);
		}
	}
	else if (op == TokenType::STAR) {
		if (a.type == ValueType::STRING && b.type == ValueType::INT) {
			std::string res = "";
			std::string base = a.asString();
			long long count = b.asInt();
			if (count < 0)
				count = 0;
			if (count > 1000000)
				throw std::runtime_error("String repetition too large");
			for (long long i = 0; i < count; i++)
				res += base;
			return Value::String(res);
		}
		else if (a.type == ValueType::VECTOR && b.type == ValueType::VECTOR) {
			auto* v1 = static_cast<VectorObject*>(a.ref.get());
			auto* v2 = static_cast<VectorObject*>(b.ref.get());
			if (v1->elements.size() != v2->elements.size())
				throw std::runtime_error("Vector dimension mismatch");
			Value dot = Value::Int(0);
			for (size_t i = 0; i < v1->elements.size(); i++) {
				Value x = v1->elements[i];
				Value y = v2->elements[i];
				Value prod;
				if (x.type == ValueType::INT && y.type == ValueType::INT) {
					long long r = x.iVal * y.iVal;
					bool ovf = (x.iVal != 0 && r / x.iVal != y.iVal);
					if (ovf)
						prod = BigIntObject::mul(Value::BigInt(x.iVal), Value::BigInt(y.iVal));
					else
						prod = Value::Int(r);
				}
				else if (x.type == ValueType::BIGINT || y.type == ValueType::BIGINT) {
					prod = BigIntObject::mul(x, y);
				}
				else {
					prod = Value::Float(x.asFloat() * y.asFloat());
				}
				if (dot.type == ValueType::INT && prod.type == ValueType::INT) {
					long long r = dot.iVal + prod.iVal;
					bool ovf = ((dot.iVal ^ r) & (prod.iVal ^ r)) < 0;
					if (ovf)
						dot = BigIntObject::add(Value::BigInt(dot.iVal), Value::BigInt(prod.iVal));
					else
						dot = Value::Int(r);
				}
				else if (dot.type == ValueType::BIGINT || prod.type == ValueType::BIGINT) {
					dot = BigIntObject::add(dot, prod);
				}
				else {
					dot = Value::Float(dot.asFloat() + prod.asFloat());
				}
			}
			return dot;
		}
		else if ((a.type == ValueType::VECTOR && b.isNumber()) || (a.isNumber() && b.type == ValueType::VECTOR)) {
			VectorObject* vec = (a.type == ValueType::VECTOR) ? static_cast<VectorObject*>(a.ref.get()) : static_cast<VectorObject*>(b.ref.get());
			Value scalar = (a.type == ValueType::VECTOR) ? b : a;
			std::vector<Value> res;
			res.reserve(vec->elements.size());
			for (const auto& elem : vec->elements) {
				if (elem.type == ValueType::INT && scalar.type == ValueType::INT) {
					long long r = elem.iVal * scalar.iVal;
					bool ovf = (elem.iVal != 0 && r / elem.iVal != scalar.iVal);
					if (ovf)
						res.push_back(BigIntObject::mul(Value::BigInt(elem.iVal), Value::BigInt(scalar.iVal)));
					else
						res.push_back(Value::Int(r));
				}
				else if (elem.type == ValueType::BIGINT || scalar.type == ValueType::BIGINT) {
					res.push_back(BigIntObject::mul(elem, scalar));
				}
				else {
					res.push_back(Value::Float(elem.asFloat() * scalar.asFloat()));
				}
			}
			return Value::Vector(res);
		}
		else if (a.type == ValueType::INT && b.type == ValueType::INT) {
			long long res = a.iVal * b.iVal;
			bool overflow = (a.iVal != 0 && res / a.iVal != b.iVal);
			if (overflow)
				return BigIntObject::mul(Value::BigInt(a.iVal), Value::BigInt(b.iVal));
			else
				return Value::Int(res);
		}
		else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
			return BigIntObject::mul(a, b);
		}
		else if (a.isNumber() && b.isNumber()) {
			return Value::Float(a.asFloat() * b.asFloat());
		}
	}
	else if (op == TokenType::FLOOR_DIV) {
		if (a.type == ValueType::VECTOR) {
			if (!b.isNumber())
				throw std::runtime_error("Vector can only be floor-divided by a number");
			if (b.asFloat() == 0.0)
				throw std::runtime_error("Vector floor division by zero");
			auto* v = static_cast<VectorObject*>(a.ref.get());
			std::vector<Value> res;
			res.reserve(v->elements.size());
			double db = b.asFloat();
			for (const auto& elem : v->elements) {
				if (elem.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
					res.push_back(BigIntObject::div(elem, b));
				}
				else {
					res.push_back(Value::Int((long long)(elem.asFloat() / db)));
				}
			}
			return Value::Vector(res);
		}
		else if (b.type == ValueType::VECTOR) {
			throw std::runtime_error("Cannot floor-divide by a vector");
		}
		else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
			if (b.asFloat() == 0.0)
				throw std::runtime_error("Division by zero");
			return BigIntObject::div(a, b);
		}
		else if (a.isNumber() && b.isNumber()) {
			double db = b.asFloat();
			if (db == 0.0)
				throw std::runtime_error("Division by zero");
			return Value::Int((long long)(a.asFloat() / db));
		}
	}
	else if (op == TokenType::MOD) {
		if (a.type == ValueType::INT && b.type == ValueType::INT) {
			if (b.iVal == 0)
				throw std::runtime_error("Modulo by zero");
			return Value::Int(a.iVal % b.iVal);
		}
		else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
			if (b.asFloat() == 0.0)
				throw std::runtime_error("Modulo by zero");
			return BigIntObject::mod(a, b);
		}
		else if (a.isNumber() && b.isNumber()) {
			if (b.asFloat() == 0.0)
				throw std::runtime_error("Modulo by zero");
			return Value::Float(std::fmod(a.asFloat(), b.asFloat()));
		}
	}
	else if (op == TokenType::POW) {
		if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
			return BigIntObject::pow(a, b);
		}
		else if (a.type == ValueType::INT && b.type == ValueType::INT) {
			double resultLog = (double)b.iVal * std::log10(std::abs((double)a.iVal));
			double maxLog = std::log10(LLONG_MAX);
			if (resultLog >= maxLog) {
				return BigIntObject::pow(a, b);
			}
			else {
				return Value::Int(static_cast<long long>(std::pow(a.iVal, b.iVal)));
			}
		}
		else if (a.isNumber() && b.isNumber()) {
			return Value::Float(std::pow(a.asFloat(), b.asFloat()));
		}
	}
	throw std::runtime_error("Cannot fold this operation at compile time.");
}