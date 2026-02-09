#pragma once
#define _CRT_SECURE_NO_WARNINGS
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define TokenType Win_TokenType 
#include <windows.h>
#undef TokenType
#undef TRUE
#undef FALSE
#undef CONST
#undef IN
#undef ERROR
#undef _TOKEN_INFORMATION_CLASS
#undef min
#undef max
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif // !ENABLE_VIRTUAL_TERMINAL_PROCESSING

#include <iostream>
#include <math.h> 
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <unordered_set>
#include <algorithm>
#include <random>
#include <functional>
#include <sstream>
#include <fstream>
#include <cstdio>
#include "pystring.h"
#include <filesystem>
#include <chrono>
#include <thread>
#include <ctime>
#include <math.h>
#include <intrin.h>
#include <deque>

bool DEBUGGER_MODE_IS_ENABLED = false;
namespace fs = std::filesystem;
using std::string;
using std::vector;
using std::unordered_map;
// -------------------- TOKENIZER --------------------
enum class TokenType {
	LET, DEFINE, FUNCTION, RETURN, IF, ELSE_IF, ELSE, FOR, WHILE, DO, THEN, BREAK, CONTINUE, SKIP,
	TRUE, FALSE, AT, DOLLAR, DOT_DOT, DOT_DOT_DOT, CONST, ASSERT, FSTRING, SWITCH, CASE, DEFAULT,
	IDENTIFIER, NUMBER, STRING, INCREMENT, DECREMENT, TRY, THROW, CATCH, FINALLY, IMPORT, FROM,
	ASSIGN, ARROW, LPAREN, RPAREN, COLON, COMMA, LBRACE, RBRACE, LBRACKET, RBRACKET, COLON_EQ,
	PLUS, MINUS, STAR, SLASH, DOT, PLUS_EQ, MINUS_EQ, STAR_EQ, DIV_EQ, FLOOR_DIV, FLOOR_DIV_EQ, MOD, MOD_EQ,
	GT, LT, EQ, STRICT_EQ, STRICT_NEQ, GTE, LTE, NEQ, POW, POW_EQ, IS, IN, IS_IN, IS_NOT, IS_NOT_IN,
	AND, OR, NOT, XOR, NAND, NOR, NXOR, AND_EQ, OR_EQ, XOR_EQ, CACHED, LAMBDA, OMIT,
	END
};
struct Token {
	TokenType type;
	string value;
	int line;
	int col;
};
inline TokenType keywordType(const string& w) {
	if (w == "let") return TokenType::LET;
	if (w == "define") return TokenType::DEFINE;
	if (w == "function") return TokenType::FUNCTION;
	if (w == "return") return TokenType::RETURN;
	if (w == "if") return TokenType::IF;
	if (w == "elif") return TokenType::ELSE_IF;
	if (w == "else") return TokenType::ELSE;
	if (w == "for") return TokenType::FOR;
	if (w == "while") return TokenType::WHILE;
	if (w == "do") return TokenType::DO;
	if (w == "then") return TokenType::THEN;
	if (w == "break") return TokenType::BREAK;
	if (w == "continue") return TokenType::CONTINUE;
	if (w == "skip") return TokenType::SKIP;
	if (w == "true") return TokenType::TRUE;
	if (w == "false") return TokenType::FALSE;
	if (w == "and") return TokenType::AND;
	if (w == "or") return TokenType::OR;
	if (w == "not") return TokenType::NOT;
	if (w == "xor") return TokenType::XOR;
	if (w == "nand") return TokenType::NAND;
	if (w == "nor")  return TokenType::NOR;
	if (w == "nxor") return TokenType::NXOR;
	if (w == "cached") return TokenType::CACHED;
	if (w == "lambda") return TokenType::LAMBDA;
	if (w == "omit") return TokenType::OMIT;
	if (w == "is") return TokenType::IS;
	if (w == "in") return TokenType::IN;
	if (w == "const") return TokenType::CONST;
	if (w == "try") return TokenType::TRY;
	if (w == "catch") return TokenType::CATCH;
	if (w == "throw") return TokenType::THROW;
	if (w == "finally") return TokenType::FINALLY;
	if (w == "assert") return TokenType::ASSERT;
	if (w == "switch") return TokenType::SWITCH;
	if (w == "case") return TokenType::CASE;
	if (w == "default") return TokenType::DEFAULT;
	if (w == "import") return TokenType::IMPORT;
	if (w == "from") return TokenType::FROM;
	return TokenType::IDENTIFIER;
}
inline vector<Token> tokenize(const string& code) {
	vector<Token> tokens;
	size_t i = 0;
	int line = 1;
	int col = 1;
	auto pushToken = [&](TokenType t, string v) {
		tokens.push_back({ t, v, line, col - (int)v.length() });
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
			while (i < code.size() && code[i] != '\n') i++;
			continue;
		}
		if (c == '`') {
			i++; col++;
			while (i < code.size() && code[i] != '`') {
				if (code[i] == '\n') { line++; col = 1; }
				else { col++; }
				i++;
			}
			if (i < code.size()) { i++; col++; }
			continue;
		}
		if (isspace(c)) {
			i++; col++;
			continue;
		}
		if ((c == 'f' || c == 'F') && i + 1 < code.size() && (code[i + 1] == '"' || code[i + 1] == '\'')) {
			char quote = code[i + 1];
			int startCol = col;
			i += 2; col += 2;
			string str;
			while (i < code.size() && code[i] != quote) {
				if (code[i] == '\n') {
					line++;
					col = 1;
				}
				else if (code[i] == '\\' && i + 1 < code.size()) {
					char next = code[i + 1];
					switch (next) {
					case 'n': str += '\n'; break;
					case 't': str += '\t'; break;
					case '\\': str += '\\'; break;
					case '"': str += '"'; break;
					case '\'': str += '\''; break;
					default: str += next; break;
					}
					i += 2; col += 2;
					continue;
				}
				else col++;
				str += code[i++];
			}
			if (i < code.size()) { i++; col++; }
			tokens.push_back({ TokenType::FSTRING, str, line, startCol });
			continue;
		}
		if (isalpha(c) || c == '_') {
			string word;
			int startCol = col;
			while (i < code.size() && (isalnum(code[i]) || code[i] == '_')) {
				word += code[i++];
				col++;
			}
			tokens.push_back({ keywordType(word), word, line, startCol });
			continue;
		}
		if (isdigit(c)) {
			string num;
			int startCol = col;
			bool isFloat = false;
			while (i < code.size()) {
				if (isdigit(code[i])) { num += code[i++]; col++; }
				else if (code[i] == '.') {
					if (i + 1 < code.size() && code[i + 1] == '.') break;
					if (isFloat) break;
					isFloat = true;
					num += code[i++]; col++;
				}
				else break;
			}
			tokens.push_back({ TokenType::NUMBER, num, line, startCol });
			continue;
		}
		if (c == '"' || c == '\'') {
			char quote = c;
			int startCol = col;
			i++; col++;
			string str;
			while (i < code.size() && code[i] != quote) {
				if (code[i] == '\n') {
					line++;
					col = 1;
				}
				else if (code[i] == '\\' && i + 1 < code.size()) {
					char next = code[i + 1];
					switch (next) {
					case 'n': str += '\n'; break;
					case 't': str += '\t'; break;
					case '\\': str += '\\'; break;
					case '"': str += '"'; break;
					case '\'': str += '\''; break;
					default: str += next; break;
					}
					i += 2; col += 2;
					continue;
				}
				else col++;
				str += code[i++];
			}
			i++; col++;
			tokens.push_back({ TokenType::STRING, str, line, startCol });
			continue;
		}
#define CHECK_OP(str, typeEnum) \
            if (code.substr(i, string(str).length()) == str) { \
                pushToken(typeEnum, str); \
                i += string(str).length(); \
                col += string(str).length(); \
                continue; \
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
		case '=': pushToken(TokenType::ASSIGN, "="); break;
		case '|': pushToken(TokenType::OR, "|"); break;
		case '&': pushToken(TokenType::AND, "&"); break;
		case '>': pushToken(TokenType::GT, ">"); break;
		case '^': pushToken(TokenType::XOR, "^"); break;
		case '<': pushToken(TokenType::LT, "<"); break;
		case '(': pushToken(TokenType::LPAREN, "("); break;
		case ')': pushToken(TokenType::RPAREN, ")"); break;
		case '[': pushToken(TokenType::LBRACKET, "["); break;
		case ']': pushToken(TokenType::RBRACKET, "]"); break;
		case '{': pushToken(TokenType::LBRACE, "{"); break;
		case '}': pushToken(TokenType::RBRACE, "}"); break;
		case ',': pushToken(TokenType::COMMA, ","); break;
		case '+': pushToken(TokenType::PLUS, "+"); break;
		case '$': pushToken(TokenType::DOLLAR, "$"); break;
		case '@': pushToken(TokenType::AT, "@"); break;
		case '-': pushToken(TokenType::MINUS, "-"); break;
		case '*': pushToken(TokenType::STAR, "*"); break;
		case '/': pushToken(TokenType::SLASH, "/"); break;
		case '.': pushToken(TokenType::DOT, "."); break;
		case '%': pushToken(TokenType::MOD, "%"); break;
		case ':':
			if (i + 1 < code.size() && code[i + 1] == '=') {
				pushToken(TokenType::COLON_EQ, ":=");
				i++; col++;
			}
			else {
				pushToken(TokenType::COLON, ":");
			}
			break;
		default:
			std::cerr << "Unknown char: " << c << " at line " << line << "\n";
			break;
		}
		i++; col++;
	}
	int endLine = (col == 1 && line > 1) ? line - 1 : line;
	tokens.push_back({ TokenType::END, "", line, col });
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
		fullMessage = type + " [Line " + std::to_string(line) + ":" + std::to_string(col) + "]: " + message;
	}
	const char* what() const noexcept override {
		return fullMessage.c_str();
	}
};
struct InternalError : LangError { InternalError(string m, int l, int c) :LangError("InternalError", m, -1100000, l, c) {} };
struct ControlFlowError : LangError { ControlFlowError(string m, int l, int c) :LangError("ControlFlowError", m, -1200000, l, c) {} };
struct ParseError : LangError { ParseError(string m, int l, int c) :LangError("ParseError", m, -2000000, l, c) {} };
struct RuntimeError : LangError { RuntimeError(string m, int l, int c) :LangError("RuntimeError", m, -3000000, l, c) {} };
struct Warning : LangError { Warning(string m, int l, int c) :LangError("Warning", m, -7000000, l, c) {} };
// --- ControlFlowError Subtree ---
struct ReturnSignal : ControlFlowError { ReturnSignal(string m, int l, int c) :ControlFlowError(m, l, c) { type = "ReturnSignal"; code = -1200100; } };
struct BreakSignal : ControlFlowError { BreakSignal(string m, int l, int c) :ControlFlowError(m, l, c) { type = "BreakSignal"; code = -1200200; } };
struct ContinueSignal : ControlFlowError { ContinueSignal(string m, int l, int c) :ControlFlowError(m, l, c) { type = "ContinueSignal"; code = -1200300; } };
// --- ParseError Subtree ---
struct SyntaxError : ParseError { SyntaxError(string m, int l, int c) :ParseError(m, l, c) { type = "SyntaxError"; code = -2000100; } };
struct IndentationError : ParseError { IndentationError(string m, int l, int c) :ParseError(m, l, c) { type = "IndentationError"; code = -2000200; } };
struct UnexpectedTokenError : ParseError { UnexpectedTokenError(string m, int l, int c) :ParseError(m, l, c) { type = "UnexpectedTokenError"; code = -2000300; } };
struct UnterminatedLiteralError : ParseError { UnterminatedLiteralError(string m, int l, int c) :ParseError(m, l, c) { type = "UnterminatedLiteralError"; code = -2000400; } };
// --- RuntimeError Subtree (Direct Children) ---
struct NameError : RuntimeError { NameError(string m, int l, int c) :RuntimeError(m, l, c) { type = "NameError"; code = -3010000; } };
struct AttributeError : RuntimeError { AttributeError(string m, int l, int c) :RuntimeError(m, l, c) { type = "AttributeError"; code = -3020000; } };
struct TypeError : RuntimeError { TypeError(string m, int l, int c) :RuntimeError(m, l, c) { type = "TypeError"; code = -3030000; } };
struct ArgumentError : RuntimeError { ArgumentError(string m, int l, int c) :RuntimeError(m, l, c) { type = "ArgumentError"; code = -3040000; } };
struct ValueError : RuntimeError { ValueError(string m, int l, int c) :RuntimeError(m, l, c) { type = "ValueError"; code = -3050000; } };
struct ConstError : RuntimeError { ConstError(string m, int l, int c) :RuntimeError(m, l, c) { type = "ConstError"; code = -3060000; } };
struct OwnershipError : RuntimeError { OwnershipError(string m, int l, int c) :RuntimeError(m, l, c) { type = "OwnershipError"; code = -3070000; } };
struct IndexError : RuntimeError { IndexError(string m, int l, int c) :RuntimeError(m, l, c) { type = "IndexError"; code = -3080000; } };
struct KeyError : RuntimeError { KeyError(string m, int l, int c) :RuntimeError(m, l, c) { type = "KeyError"; code = -3090000; } };
struct RangeError : RuntimeError { RangeError(string m, int l, int c) :RuntimeError(m, l, c) { type = "RangeError"; code = -3100000; } };
struct AssertionError : RuntimeError { AssertionError(string m, int l, int c) :RuntimeError(m, l, c) { type = "AssertionError"; code = -3110000; } };
struct RecursionError : RuntimeError { RecursionError(string m, int l, int c) :RuntimeError(m, l, c) { type = "RecursionError"; code = -3120000; } };
struct ImportError : RuntimeError { ImportError(string m, int l, int c) :RuntimeError(m, l, c) { type = "ImportError"; code = -3200000; } };
struct IOError : RuntimeError { IOError(string m, int l, int c) :RuntimeError(m, l, c) { type = "IOError"; code = -3300000; } };
struct MathError : RuntimeError { MathError(string m, int l, int c) :RuntimeError(m, l, c) { type = "MathError"; code = -3400000; } };
struct CastError : RuntimeError { CastError(string m, int l, int c) :RuntimeError(m, l, c) { type = "CastError"; code = -3500000; } };
struct IteratorError : RuntimeError { IteratorError(string m, int l, int c) :RuntimeError(m, l, c) { type = "IteratorError"; code = -3510000; } };
struct TimeoutError : RuntimeError { TimeoutError(string m, int l, int c) :RuntimeError(m, l, c) { type = "TimeoutError"; code = -3520000; } };
struct MemoryError : RuntimeError { MemoryError(string m, int l, int c) :RuntimeError(m, l, c) { type = "MemoryError"; code = -3600000; } };
struct SystemError : RuntimeError { SystemError(string m, int l, int c) :RuntimeError(m, l, c) { type = "SystemError"; code = -3700000; } };
// --- ValueError Children ---
struct EmptyContainerError : ValueError { EmptyContainerError(string m, int l, int c) :ValueError(m, l, c) { type = "EmptyContainerError"; code = -3050100; } };
// --- ConstError Children ---
struct MutationError : ConstError { MutationError(string m, int l, int c) :ConstError(m, l, c) { type = "MutationError"; code = -3060100; } };
// --- ImportError Children ---
struct ModuleNotFoundError : ImportError { ModuleNotFoundError(string m, int l, int c) :ImportError(m, l, c) { type = "ModuleNotFoundError"; code = -3200100; } };
struct CircularImportError : ImportError { CircularImportError(string m, int l, int c) :ImportError(m, l, c) { type = "CircularImportError"; code = -3200200; } };
struct InvalidImportError : ImportError { InvalidImportError(string m, int l, int c) :ImportError(m, l, c) { type = "InvalidImportError"; code = -3200300; } };
// --- IOError Children ---
struct FileNotFoundError : IOError { FileNotFoundError(string m, int l, int c) :IOError(m, l, c) { type = "FileNotFoundError"; code = -3300100; } };
struct PermissionError : IOError { PermissionError(string m, int l, int c) :IOError(m, l, c) { type = "PermissionError"; code = -3300200; } };
struct EOFError : IOError { EOFError(string m, int l, int c) :IOError(m, l, c) { type = "EOFError"; code = -3300300; } };
struct FileClosedError : IOError { FileClosedError(string m, int l, int c) :IOError(m, l, c) { type = "FileClosedError"; code = -3300400; } };
// --- MathError Children ---
struct DivisionByZeroError : MathError { DivisionByZeroError(string m, int l, int c) :MathError(m, l, c) { type = "DivisionByZeroError"; code = -3400100; } };
struct OverflowError : MathError { OverflowError(string m, int l, int c) :MathError(m, l, c) { type = "OverflowError"; code = -3400200; } };
struct UnderflowError : MathError { UnderflowError(string m, int l, int c) :MathError(m, l, c) { type = "UnderflowError"; code = -3400300; } };
struct DomainError : MathError { DomainError(string m, int l, int c) :MathError(m, l, c) { type = "DomainError"; code = -3400400; } };
// --- SystemError Children ---
struct OSError : SystemError { OSError(string m, int l, int c) :SystemError(m, l, c) { type = "OSError"; code = -3700100; } };
struct EnvironmentError : SystemError { EnvironmentError(string m, int l, int c) :SystemError(m, l, c) { type = "EnvironmentError"; code = -3700200; } };
struct SignalError : SystemError { SignalError(string m, int l, int c) :SystemError(m, l, c) { type = "SignalError"; code = -3700300; } };
// --- Warning Children ---
struct DeprecationWarning : Warning { DeprecationWarning(string m, int l, int c) :Warning(m, l, c) { type = "DeprecationWarning"; code = -7010000; } };
struct RuntimeWarning : Warning { RuntimeWarning(string m, int l, int c) :Warning(m, l, c) { type = "RuntimeWarning"; code = -7020000; } };
struct ImportWarning : Warning { ImportWarning(string m, int l, int c) :Warning(m, l, c) { type = "ImportWarning"; code = -7030000; } };
enum class ValueType { 
	NOTYPE, NONE, INT, FLOAT, STRING, BOOL, LIST, VECTOR, DICT, SLICE, BIGINT, REFERENCE, PAIRED,
	RANGE, TUPLE, SET, FUNCTION, NATIVE_FUNCTION, FILE, OVERLOAD, OMIT_MARKER, ERROR
};
enum class CopyMode { SHALLOW, DEEP, REF };
// -------------------- AST --------------------
enum class ExprType {
	FSTRING, OWNERSHIP, BOOL, NUMBER, STRING, VAR, LIST, BINARY,
	CALL, INDEX, METHOD_CALL, LAMBDA, RANGE, SET, DICT, TUPLE,
	TERNARY, COMPREHENSION, SLICE, VECTOR, OMIT_MARKER_EXPR
};
enum class StmtType {
	ASSIGN, LET, RETURN, FUNC, IF, EXPR, BREAK, CONTINUE, SKIP,
	WHILE, DO_WHILE, FOR, FOR_EACH, TRY, THROW, ASSERT, SWITCH,
	IMPORT, MULTI_LET, MULTI_ASSIGN
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
	StringExpr(const string& v) : Expr(ExprType::STRING), val(v) {}
}; 
struct VarExpr : Expr {
	string name;
	Value* cachedGlobal = nullptr;
	VarExpr(const string& n) : Expr(ExprType::VAR), name(n) {}
};
struct BinExpr : Expr {
	Expr* left, * right;
	TokenType op;
	BinExpr(Expr* l, Expr* r, TokenType o) : Expr(ExprType::BINARY), left(l), right(r), op(o) {}
};
struct CallExpr : Expr {
	string name;
	vector<Expr*> args;
	vector<CopyMode> modes;
	Value* cachedFunc = nullptr;
	CallExpr(const string& n, const vector<Expr*>& a, vector<CopyMode>& m)
		: Expr(ExprType::CALL), name(n), args(a), modes(m) {}
};
struct BoolExpr : Expr {
	bool value;
	BoolExpr(bool v) : Expr(ExprType::BOOL), value(v) {}
};
struct OwnershipExpr : Expr {
	CopyMode mode;
	Expr* expr;
	OwnershipExpr(CopyMode m, Expr* e) : Expr(ExprType::OWNERSHIP), mode(m), expr(e) {}
};
struct TernaryExpr : Expr {
	Expr* condition;
	Expr* trueBranch;
	Expr* falseBranch;
	TernaryExpr(Expr* condition, Expr* trueBranch, Expr* falseBranch)
		: Expr(ExprType::TERNARY), condition(condition), trueBranch(trueBranch), falseBranch(falseBranch) {}
};
struct FStringExpr : Expr {
	vector<Expr*> parts;
	FStringExpr(vector<Expr*> p) : Expr(ExprType::FSTRING), parts(p) {}
};
struct CompExpr : Expr {
	Expr* expression;
	Expr* valueExpr;
	string varName;
	Expr* iterable;
	Expr* filter;
	TokenType typeToken;
	CompExpr(Expr* e, Expr* v, string n, Expr* i, Expr* f, TokenType t)
		: Expr(ExprType::COMPREHENSION), expression(e), valueExpr(v), varName(n), iterable(i), filter(f), typeToken(t) {}
};
struct ListExpr : Expr {
	vector<Expr*> elements;
	ListExpr(vector<Expr*> e) : Expr(ExprType::LIST), elements(e) {};
};
struct IndexExpr : Expr {
	Expr* base;
	Expr* index;
	IndexExpr(Expr* b, Expr* i) : Expr(ExprType::INDEX), base(b), index(i) {}
};
struct MethodCallExpr : Expr {
	Expr* object;
	string method;
	vector<Expr*> args;
	MethodCallExpr(Expr* o, const string& m, const vector<Expr*>& a)
		: Expr(ExprType::METHOD_CALL), object(o), method(m), args(a) {};
};
struct VectorExpr : Expr {
	vector<Expr*> elements;
	VectorExpr(const vector<Expr*>& e) : Expr(ExprType::VECTOR), elements(e) {}
};
struct ParamSpec {
	string name;
	CopyMode mode = CopyMode::SHALLOW;
	ValueType type;
	Expr* defaultValue = nullptr;
	bool isConst;
	bool isVariadic = false;
	bool isKwargs = false;
};
struct LambdaExpr : Expr {
	vector<ParamSpec> params;
	ValueType returnType;
	vector<Expr*> defaultRetArgs;
	bool returnsConst;
	vector<Stmt*> body;
	bool isCached;
	LambdaExpr(const vector<ParamSpec>& p, ValueType rt, vector<Expr*> dra, bool rc, const vector<Stmt*>& b, bool cached)
		: Expr(ExprType::LAMBDA), params(p), returnType(rt), defaultRetArgs(dra), returnsConst(rc), body(b), isCached(cached) {}
};
struct RangeExpr : Expr {
	Expr* start;
	Expr* end;
	Expr* step;
	bool startInclusive;
	bool endInclusive;
	RangeExpr(Expr* s, Expr* e, Expr* st, bool si, bool ei)
		: Expr(ExprType::RANGE), start(s), end(e), step(st), startInclusive(si), endInclusive(ei) {}
};
struct SetExpr : Expr {
	vector<Expr*> elements;
	SetExpr(const vector<Expr*>& e) : Expr(ExprType::SET), elements(e) {}
};
struct TupleExpr : Expr {
	vector<Expr*> elements;
	TupleExpr(vector<Expr*> e) : Expr(ExprType::TUPLE), elements(e) {};
};
struct DictExpr : Expr {
	vector<std::pair<Expr*, Expr*>> items;
	DictExpr(const vector<std::pair<Expr*, Expr*>>& i) : Expr(ExprType::DICT), items(i) {}
};
struct SliceExpr : Expr {
	Expr* start = nullptr;
	Expr* end = nullptr;
	Expr* step = nullptr;
	SliceExpr(Expr* s, Expr* e, Expr* p) : Expr(ExprType::SLICE), start(s), end(e), step(p) {}
};
struct OmitExpr : Expr {
	OmitExpr(int l, int c) : Expr(ExprType::OMIT_MARKER_EXPR) {}
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
	Expr* count;
	SkipStmt(Expr* c) : Stmt(StmtType::SKIP), count(c) {}
};
struct WhileStmt : Stmt {
	Expr* condition;
	vector<Stmt*> body;
	WhileStmt(Expr* c, vector<Stmt*> b) : Stmt(StmtType::WHILE), condition(c), body(b) {}
};
struct DoWhileStmt : Stmt {
	vector<Stmt*> body;
	Expr* condition;
	DoWhileStmt(vector<Stmt*> b, Expr* c) : Stmt(StmtType::DO_WHILE), body(b), condition(c) {}
};
struct ForStmt : Stmt {
	vector<Stmt*> inits;
	Expr* condition;
	vector<Stmt*> steps;
	vector<Stmt*> body;
	ForStmt(vector<Stmt*> i, Expr* c, vector<Stmt*> s, vector<Stmt*> b)
		: Stmt(StmtType::FOR), inits(i), condition(c), steps(s), body(b) {}
};
struct ForEachStmt : Stmt {
	vector<string> loopVars;
	vector<Expr*> collections;
	vector<Stmt*> body;
	ForEachStmt(vector<string> vars, vector<Expr*> cols, vector<Stmt*> b)
		: Stmt(StmtType::FOR_EACH), loopVars(vars), collections(cols), body(b) {}
};
struct LetStmt : Stmt {
	string name;
	Expr* value;
	bool isConst;
	bool isLocked;
	LetStmt(const string& n, Expr* v, bool c, bool l)
		: Stmt(StmtType::LET), name(n), value(v), isConst(c), isLocked(l) {}
};
struct AssignStmt : Stmt {
	Expr* target;
	TokenType op;
	Expr* value;
	AssignStmt(Expr* n, TokenType o, Expr* v)
		: Stmt(StmtType::ASSIGN), target(n), op(o), value(v) {}
};
struct ReturnStmt : Stmt {
	Expr* value;
	ReturnStmt(Expr* v) : Stmt(StmtType::RETURN), value(v) {}
};
struct FuncStmt : Stmt {
	string name;
	vector<ParamSpec> params;
	vector<Expr*> defaultRetArgs;
	vector<Stmt*> body;
	ValueType returnType = ValueType::NOTYPE;
	bool returnsConst;
	bool isCached;
	FuncStmt(const string& n, const vector<ParamSpec>& p, vector<Expr*> dra, const vector<Stmt*>& b, ValueType rt, bool rc, bool cached)
		: Stmt(StmtType::FUNC), name(n), params(p), defaultRetArgs(dra), body(b), returnType(rt), returnsConst(rc), isCached(cached) {};
};
struct IfStmt : Stmt {
	Expr* condition;
	vector<Stmt*> body;
	vector<std::pair<Expr*, vector<Stmt*>>> elifs;
	vector<Stmt*> elseBody;
	IfStmt(Expr* cond, vector<Stmt*> b, vector<std::pair<Expr*, vector<Stmt*>>> eib, vector<Stmt*> eb = {})
		: Stmt(StmtType::IF), condition(cond), body(b), elifs(eib), elseBody(eb) {}
};
struct ElifStmt : IfStmt {

};
struct Case {
	Expr* value;
	vector<Stmt*> body;
};
struct SwitchStmt : Stmt {
	Expr* target;
	vector<Case> cases;
	vector<Stmt*> defaultBody;
	SwitchStmt(Expr* t, const vector<Case>& c, const vector<Stmt*>& d)
		: Stmt(StmtType::SWITCH), target(t), cases(c), defaultBody(d) {}
};
struct ExprStmt : Stmt {
	Expr* expr;
	ExprStmt(Expr* e) : Stmt(StmtType::EXPR), expr(e) {}
};
struct AssertStmt : Stmt {
	Expr* condition;
	Expr* message;
	AssertStmt(Expr* condition, Expr* message = nullptr)
		: Stmt(StmtType::ASSERT), condition(condition), message(message) {}
};
struct CatchBlock {
	vector<string> types;
	vector<Stmt*> body;
};
struct TryStmt : Stmt {
	vector<Stmt*> tryBlock;
	vector<CatchBlock> catches;
	vector<Stmt*> elseBlock;
	vector<Stmt*> finallyBlock;
	TryStmt(const vector<Stmt*>& t, const vector<CatchBlock>& c, const vector<Stmt*>& e, const vector<Stmt*>& f)
		: Stmt(StmtType::TRY), tryBlock(t), catches(c), elseBlock(e), finallyBlock(f) {}
};
struct ThrowStmt : Stmt {
	string errorType;
	Expr* message;
	ThrowStmt(string errorType, Expr* message)
		: Stmt(StmtType::THROW), errorType(errorType), message(message) {}
};
struct ImportStmt : Stmt {
	string libName;
	vector<string> symbols;
	ImportStmt(string lib, vector<string> syms) : Stmt(StmtType::IMPORT), libName(lib), symbols(syms) {}
};
struct MultiLetStmt : Stmt {
	vector<string> names;
	vector<Expr*> values;
	vector<bool> isConsts;
	bool isLocked;
	MultiLetStmt(const vector<string>& n, const vector<Expr*>& v, const vector<bool>& c, bool l)
		: Stmt(StmtType::MULTI_LET), names(n), values(v), isConsts(c), isLocked(l) {}
};
struct MultiAssignStmt : Stmt {
	vector<Expr*> targets;
	vector<Expr*> values;
	MultiAssignStmt(const vector<Expr*>& t, const vector<Expr*>& v)
		: Stmt(StmtType::MULTI_ASSIGN), targets(t), values(v) {}
};
// -------------------- PARSER --------------------
class Parser {
	vector<Token>& tokens; size_t pos = 0; bool allowGT = true;
private:
	[[noreturn]] void error(const string& message) {
		Token t = peek();
		throw SyntaxError(message, t.line, t.col);
	}
	Token consume(TokenType type, const string& message) {
		if (peek().type == type) return advance();
		error(message);
	}
	template <typename T>
	T* setPos(T* node, const Token& t) {
		node->line = t.line;
		node->col = t.col;
		return node;
	}
public:
	Parser(vector<Token>& t) :tokens(t) {}
	bool isAtEnd() { return peek().type == TokenType::END; }
	Token& peek() { return tokens[pos]; }
	Token& advance() { return tokens[pos++]; }
	bool match(TokenType t) { if (peek().type == t) { advance(); return true; } return false; }
	Expr* parseExpr() { return parseTernary(); }
	Expr* parseTernary() {
		Expr* expr = parseRange();
		if (peek().type == TokenType::IF) {
			size_t current = pos + 1;
			int openParens = 0;
			bool hasElse = false;
			while (current < tokens.size()) {
				TokenType t = tokens[current].type;
				if (t == TokenType::LPAREN || t == TokenType::LBRACE || t == TokenType::LBRACKET) openParens++;
				if (t == TokenType::RPAREN || t == TokenType::RBRACE || t == TokenType::RBRACKET) openParens--;
				if (openParens == 0 && t == TokenType::ELSE) {
					hasElse = true;
					break;
				}
				if (openParens == 0 && (
					t == TokenType::THEN ||
					t == TokenType::DO ||
					t == TokenType::COLON ||
					t == TokenType::RBRACE ||
					t == TokenType::END
					)) {
					break;
				}
				current++;
			}
			if (!hasElse) return expr;
			advance();
			Token opToken = tokens[pos - 1];
			Expr* condition = parseTernary();
			consume(TokenType::ELSE, "Expected 'else' in ternary operator");
			Expr* falseBranch = parseTernary();
			return setPos(new TernaryExpr(condition, expr, falseBranch), opToken);
		}
		return expr;
	}
	Expr* parseRange() {
		Expr* start = nullptr;
		bool startImplied = false;
		if (peek().type == TokenType::DOT_DOT || peek().type == TokenType::DOT_DOT_DOT) {
			start = new NumberExpr(0, false);
			startImplied = true;
		}
		else start = parseOr();
		if (peek().type == TokenType::DOT_DOT || peek().type == TokenType::DOT_DOT_DOT) {
			Token opToken = advance();
			bool inclusive = (opToken.type == TokenType::DOT_DOT_DOT);
			Expr* end = parseOr();
			Expr* step = nullptr;
			if (peek().type == TokenType::DOT_DOT || peek().type == TokenType::DOT_DOT_DOT) {
				advance();
				step = parseOr();
			}
			return new RangeExpr(start, end, step, true, inclusive);
		}
		if (startImplied) error("Unexpected range operator without end value");
		return start;
	}
	vector<Stmt*> parseBlock() {
		vector<Stmt*> body;
		if (match(TokenType::LBRACE)) {
			while (!isAtEnd() && peek().type != TokenType::RBRACE) {
				body.push_back(parseStmt());
			}
			consume(TokenType::RBRACE, "Expected '}' to close block");
			return body;
		}
		body.push_back(parseStmt());
		return body;
	}
	Expr* parseOr() {
		Expr* left = parseXor();
		while (peek().type == TokenType::OR || peek().type == TokenType::NOR) {
			Token opToken = advance();
			Expr* right = parseXor();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
		}
		return left;
	}
	Expr* parseXor() {
		Expr* left = parseAnd();
		while (peek().type == TokenType::XOR || peek().type == TokenType::NXOR) {
			Token opToken = advance();
			Expr* right = parseAnd();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
		}
		return left;
	}
	Expr* parseAnd() {
		Expr* left = parseCompare();
		while (peek().type == TokenType::AND || peek().type == TokenType::NAND) {
			Token opToken = advance();
			Expr* right = parseCompare();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
		}
		return left;
	}
	Expr* parseCompare() {
		Expr* left = parseAdd();
		while (peek().type == TokenType::GT || peek().type == TokenType::LT ||
			peek().type == TokenType::GTE || peek().type == TokenType::LTE ||
			peek().type == TokenType::EQ || peek().type == TokenType::NEQ ||
			peek().type == TokenType::STRICT_EQ || peek().type == TokenType::STRICT_NEQ ||
			peek().type == TokenType::IS) {
			if (!allowGT && peek().type == TokenType::GT) break;
			Token opToken = peek();
			TokenType op;
			if (match(TokenType::IS)) {
				opToken = tokens[pos - 1];
				if (match(TokenType::NOT)) {
					if (match(TokenType::IN)) op = TokenType::IS_NOT_IN;
					else op = TokenType::IS_NOT;
				}
				else if (match(TokenType::IN)) op = TokenType::IS_IN;
				else  op = TokenType::IS;
			}
			else {
				opToken = advance();
				op = opToken.type;
			}
			Expr* right = parseAdd();
			left = setPos(new BinExpr(left, right, op), opToken);
		}
		return left;
	}
	Expr* parseAdd() {
		Expr* left = parseMul();
		while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
			Token opToken = advance();
			Expr* right = parseMul();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
		}
		return left;
	}
	Expr* parseMul() {
		Expr* left = parsePower();
		while (peek().type == TokenType::STAR || peek().type == TokenType::SLASH || 
			peek().type == TokenType::FLOOR_DIV || peek().type == TokenType::MOD) {
			Token opToken = advance();
			Expr* right = parseUnary();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
		}
		return left;
	}
	Expr* parsePower() {
		Expr* left = parseUnary();
		if (match(TokenType::POW)) {
			Token opToken = tokens[pos - 1];
			Expr* right = parsePower();
			return setPos(new BinExpr(left, right, TokenType::POW), opToken);
		}
		return left;
	}
	Expr* parseUnary() {
		if (match(TokenType::MINUS)) {
			Token opToken = tokens[pos - 1];
			Expr* right = parseUnary();
			return setPos(new BinExpr(setPos(new NumberExpr(0, false), opToken), right, TokenType::MINUS), opToken);
		}
		if (match(TokenType::AT)) {
			Token opToken = tokens[pos - 1];
			Expr* target = parsePrimary();
			if (!dynamic_cast<VarExpr*>(target) && !dynamic_cast<IndexExpr*>(target)) {
				throw SyntaxError("Cannot take reference of non-lvalue", opToken.line, opToken.col);
			}
			return setPos(new OwnershipExpr(CopyMode::REF, target), opToken);
		}
		if (match(TokenType::DOLLAR)) {
			Token opToken = tokens[pos - 1];
			return setPos(new OwnershipExpr(CopyMode::DEEP, parseUnary()), opToken);
		}
		if (match(TokenType::NOT)) {
			Token opToken = tokens[pos - 1];
			Expr* right = parseUnary();
			return setPos(new BinExpr(nullptr, right, TokenType::NOT), opToken);
		}
		return parsePrimary();
	}
	Expr* parseFString() {
		Token t = tokens[pos - 1];
		string raw = t.value;
		vector<Expr*> parts;
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
					if (raw[i] == '{') depth++;
					if (raw[i] == '}') depth--;
					if (depth > 0) code += raw[i];
					i++;
				}
				i--;
				auto subTokens = tokenize(code);
				Parser subParser(subTokens);
				parts.push_back(subParser.parseExpr());
			}
			else buffer += raw[i];
		}
		if (!buffer.empty()) parts.push_back(setPos(new StringExpr(buffer), t));
		return setPos(new FStringExpr(parts), t);
	}
	Expr* parsePrimary() {
		Expr* expr = nullptr;
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
			}
			else {
				throw SyntaxError("Expected 'lambda' after 'define' in expression", tokens[pos - 1].line, tokens[pos - 1].col);
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
					if (match(TokenType::AT)) mode = CopyMode::REF;
					else if (match(TokenType::DOLLAR)) mode = CopyMode::DEEP;
					bool isConst = false;
					if (match(TokenType::CONST)) isConst = true;
					bool isVariadic = false;
					bool isKwargs = false;
					if (match(TokenType::POW)) {
						if (seenKwargs) error("Cannot have multiple **kwargs.");
						if (seenVariadic) { /* OK: **kwargs follows *args */ }
						isKwargs = true;
						seenKwargs = true;
					}
					if (match(TokenType::STAR)) {
						if (seenVariadic) error("Cannot have multiple variadic (*args) parameters.");
						if (seenKwargs) error("*args cannot follow **kwargs.");
						isVariadic = true;
						seenVariadic = true;
					}
					if (!isVariadic && !isKwargs) {
						if (seenVariadic) error("Parameter cannot follow *args.");
						if (seenKwargs) error("Parameter cannot follow **kwargs.");
					}
					consume(TokenType::IDENTIFIER, "Expected parameter name");
					string pname = tokens[pos - 1].value;
					ValueType ptype = ValueType::NOTYPE;
					Expr* defaultExpr = nullptr;
					if (match(TokenType::COLON)) {
						string t;
						if (match(TokenType::FUNCTION)) t = "function";
						else if (match(TokenType::LAMBDA)) t = "lambda";
						else {
							consume(TokenType::IDENTIFIER, "Expected type name");
							t = tokens[pos - 1].value;
						}
						
						if (t == "int") ptype = ValueType::INT;
						else if (t == "float") ptype = ValueType::FLOAT;
						else if (t == "bool") ptype = ValueType::BOOL;
						else if (t == "string") ptype = ValueType::STRING;
						else if (t == "list") ptype = ValueType::LIST;
						else if (t == "dict" || t == "dictionary") ptype = ValueType::DICT;
						else if (t == "set") ptype = ValueType::SET;
						else if (t == "tuple") ptype = ValueType::TUPLE;
						else if (t == "range") ptype = ValueType::RANGE;
						else if (t == "vector") ptype = ValueType::VECTOR;
						else if (t == "function") ptype = ValueType::FUNCTION;
						else if (t == "lambda") ptype = ValueType::FUNCTION;
						else if (t == "None") ptype = ValueType::NONE;
						else throw SyntaxError("Unknown type '" + t + "'", tokens[pos - 1].line, tokens[pos - 1].col);
						consume(TokenType::LPAREN, "Expected '(' after type definition");
						if (peek().type != TokenType::RPAREN) defaultExpr = parseExpr();
						consume(TokenType::RPAREN, "Expected ')' after type definition");
					}
					if (defaultExpr == nullptr && match(TokenType::ASSIGN)) {
						if (isVariadic || isKwargs) error("Variadic/Kwargs cannot have default values.");
						defaultExpr = parseExpr();
						seenDefault = true;
					}
					if (!isVariadic && !isKwargs && !defaultExpr && seenDefault) {
						error("Non-default parameter '" + pname + "' cannot follow default parameters.");
					}
					params.push_back({ pname, mode, ptype, defaultExpr, isConst, isVariadic, isKwargs });
				} while (match(TokenType::COMMA));
			}
			consume(TokenType::RPAREN, "Expected ')' after parameters");
			ValueType retType = ValueType::NOTYPE;
			bool retConst = false;
			vector<Expr*> retArgs;
			if (match(TokenType::ARROW)) {
				if (match(TokenType::CONST)) retConst = true;
				string t;
				if (match(TokenType::FUNCTION)) t = "function";
				else if (match(TokenType::LAMBDA)) t = "lambda";
				else {
					consume(TokenType::IDENTIFIER, "Expected return type");
					t = tokens[pos - 1].value;
				}
				if (t == "int") retType = ValueType::INT;
				else if (t == "float") retType = ValueType::FLOAT;
				else if (t == "string") retType = ValueType::STRING;
				else if (t == "bool") retType = ValueType::BOOL;
				else if (t == "list") retType = ValueType::LIST;
				else if (t == "dict" || t == "dictionary") retType = ValueType::DICT;
				else if (t == "set") retType = ValueType::SET;
				else if (t == "tuple") retType = ValueType::TUPLE;
				else if (t == "range") retType = ValueType::RANGE;
				else if (t == "vector") retType = ValueType::VECTOR;
				else if (t == "function") retType = ValueType::FUNCTION;
				else if (t == "lambda") retType = ValueType::FUNCTION;
				else if (t == "None") retType = ValueType::NONE;
				else throw SyntaxError("Unknown return type '" + t + "'", tokens[pos - 1].line, tokens[pos - 1].col);
				if (match(TokenType::LPAREN)) {
					if (peek().type != TokenType::RPAREN) {
						do {
							Expr* arg = parseExpr();
							if (match(TokenType::COLON)) {
								Token op = tokens[pos - 1];
								Expr* val = parseExpr();
								arg = setPos(new BinExpr(arg, val, TokenType::COLON), op);
							}
							retArgs.push_back(arg);
						} while (match(TokenType::COMMA));
					}
					consume(TokenType::RPAREN, "Expected ')' after return type");
				}
			}
			consume(TokenType::COLON, "Expected ':' before lambda body");
			vector<Stmt*> body = parseBlock();
			return setPos(new LambdaExpr(params, retType, retArgs, retConst, body, isCached), startTok);
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
		}
		else if (match(TokenType::STRING)) {
			expr = setPos(new StringExpr(tokens[pos - 1].value), tokens[pos - 1]);
		}
		else if (match(TokenType::TRUE)) {
			expr = setPos(new BoolExpr(true), tokens[pos - 1]);
		}
		else if (match(TokenType::FALSE)) {
			expr = setPos(new BoolExpr(false), tokens[pos - 1]);
		}
		else if (match(TokenType::IDENTIFIER)) {
			expr = setPos(new VarExpr(tokens[pos - 1].value), tokens[pos - 1]);
		}
		else if (match(TokenType::LPAREN)) {
			Token paren = tokens[pos - 1];
			bool oldGT = allowGT;
			allowGT = true;
			if (match(TokenType::COMMA)) {
				consume(TokenType::RPAREN, "Expected ')' to close empty tuple");
				expr = setPos(new TupleExpr({}), paren);
			}
			else {
				Expr* e = parseExpr();
				if (match(TokenType::FOR)) {
					consume(TokenType::IDENTIFIER, "Expected variable name after 'for'");
					string varName = tokens[pos - 1].value;
					consume(TokenType::IN, "Expected 'in'");
					Expr* iterable = parseExpr();
					Expr* filter = nullptr;
					if (match(TokenType::IF)) filter = parseExpr();
					consume(TokenType::RPAREN, "Expected ')' to close comprehension");
					expr = setPos(new CompExpr(e, nullptr, varName, iterable, filter, TokenType::LPAREN), paren);
				}
				else if (match(TokenType::COMMA)) {
					vector<Expr*> tupleElems;
					tupleElems.push_back(e);
					if (peek().type != TokenType::RPAREN) {
						do {
							if (peek().type == TokenType::RPAREN) break;
							tupleElems.push_back(parseExpr());
						} while (match(TokenType::COMMA));
					}
					consume(TokenType::RPAREN, "Expected ')' to close tuple");
					expr = setPos(new TupleExpr(tupleElems), paren);
				}
				else {
					consume(TokenType::RPAREN, "Expected ')' to close grouping");
					expr = e;
				}
			}
		}
		else if (match(TokenType::LBRACE)) {
			Token brace = tokens[pos - 1];
			if (match(TokenType::COMMA)) {
				consume(TokenType::RBRACE, "Expected '}' to close empty set");
				expr = setPos(new SetExpr({}), brace);
			}
			else if (peek().type == TokenType::RBRACE) {
				advance();
				expr = setPos(new DictExpr({}), brace);
			}
			else {
				Expr* first = parseExpr();
				if (match(TokenType::COLON)) {
					Expr* val = parseExpr();
					if (match(TokenType::FOR)) {
						consume(TokenType::IDENTIFIER, "Expected variable name after 'for'");
						string varName = tokens[pos - 1].value;
						consume(TokenType::IN, "Expected 'in'");
						Expr* iterable = parseExpr();
						Expr* filter = nullptr;
						if (match(TokenType::IF)) filter = parseExpr();
						consume(TokenType::RBRACE, "Expected '}' to close dict comprehension");
						expr = setPos(new CompExpr(first, val, varName, iterable, filter, TokenType::LBRACE), brace);
					}
					else {
						vector<std::pair<Expr*, Expr*>> items;
						items.push_back({ first, val });
						while (match(TokenType::COMMA)) {
							Expr* k = parseExpr();
							consume(TokenType::COLON, "Expected ':' in dictionary entry");
							Expr* v = parseExpr();
							items.push_back({ k, v });
						}
						consume(TokenType::RBRACE, "Expected '}' to close dictionary");
						expr = setPos(new DictExpr(items), brace);
					}
				}
				else if (match(TokenType::FOR)) {
					consume(TokenType::IDENTIFIER, "Expected variable name after 'for'");
					string varName = tokens[pos - 1].value;
					consume(TokenType::IN, "Expected 'in'");
					Expr* iterable = parseExpr();
					Expr* filter = nullptr;
					if (match(TokenType::IF)) filter = parseExpr();
					consume(TokenType::RBRACE, "Expected '}' to close set comprehension");
					expr = setPos(new CompExpr(first, nullptr, varName, iterable, filter, TokenType::LBRACE), brace);
				}
				else {
					vector<Expr*> elems;
					elems.push_back(first);
					while (match(TokenType::COMMA)) elems.push_back(parseExpr());
					consume(TokenType::RBRACE, "Expected '}' to close set");
					expr = setPos(new SetExpr(elems), brace);
				}
			}
		}
		else if (match(TokenType::LBRACKET)) {
			Token bracket = tokens[pos - 1];
			if (peek().type == TokenType::RBRACKET) {
				advance();
				expr = setPos(new ListExpr({}), bracket);
			}
			else {
				Expr* first = parseExpr();
				if (match(TokenType::FOR)) {
					consume(TokenType::IDENTIFIER, "Expected variable name after 'for'");
					string varName = tokens[pos - 1].value;
					consume(TokenType::IN, "Expected 'in'");
					Expr* iterable = parseExpr();
					Expr* filter = nullptr;
					if (match(TokenType::IF)) filter = parseExpr();
					consume(TokenType::RBRACKET, "Expected ']' to close list comprehension");
					expr = setPos(new CompExpr(first, nullptr, varName, iterable, filter, TokenType::LBRACKET), bracket);
				}
				else {
					vector<Expr*> elems;
					elems.push_back(first);
					while (match(TokenType::COMMA)) elems.push_back(parseExpr());
					consume(TokenType::RBRACKET, "Expected ']' to close list");
					expr = setPos(new ListExpr(elems), bracket);
				}
			}
		}
		else if (match(TokenType::LT)) {
		Token startTok = tokens[pos - 1];
		vector<Expr*> elements;
		if (peek().type != TokenType::GT) {
			bool oldGT = allowGT;
			allowGT = false;
			try {
				do {
					elements.push_back(parseExpr());
				} while (match(TokenType::COMMA));
			}
			catch (...) { allowGT = oldGT; throw; }
			allowGT = oldGT;
		}
		consume(TokenType::GT, "Expected '>' to close vector literal");
		expr = setPos(new VectorExpr(elements), startTok);
		}
		else {
			Token current = peek();
			Token prev = (pos > 0) ? tokens[pos - 1] : current;
			if (current.line > prev.line) {
				throw SyntaxError("Expected expression after '" + prev.value + "'", prev.line, prev.col + (int)prev.value.length());
			}
			if (!expr) error("Expected expression");
		}
		while (true) {
			if (match(TokenType::LPAREN)) {
				vector<Expr*> args;
				vector<CopyMode> modes;
				if (peek().type != TokenType::RPAREN) {
					do {
						CopyMode mode = CopyMode::SHALLOW;
						if (match(TokenType::AT)) mode = CopyMode::REF;
						else if (match(TokenType::DOLLAR)) mode = CopyMode::DEEP;
						Expr* arg = parseExpr();
						if (match(TokenType::COLON)) {
							Expr* val = parseExpr();
							arg = setPos(new BinExpr(arg, val, TokenType::COLON), tokens[pos - 2]);
						}
						args.push_back(arg);
						modes.push_back(mode);
					} while (match(TokenType::COMMA));
				}
				match(TokenType::RPAREN);
				if (auto v = dynamic_cast<VarExpr*>(expr)) expr = setPos(new CallExpr(v->name, args, modes), startTok);
				else {
					Token t = tokens[pos - 1];
					throw TypeError("Expression is not callable (must be a variable name)", t.line, t.col);
				}
			}
			else if (match(TokenType::LBRACKET)) {
				Token bracket = tokens[pos - 1];
				Expr* start = nullptr;
				Expr* end = nullptr;
				Expr* step = nullptr;
				bool isSlice = false;
				if (match(TokenType::COLON)) {
					isSlice = true;
					if (peek().type != TokenType::COLON && peek().type != TokenType::RBRACKET) end = parseExpr();
					if (match(TokenType::COLON)) if (peek().type != TokenType::RBRACKET) step = parseExpr();
				}
				else {
					Expr* first = parseExpr();
					if (match(TokenType::COLON)) {
						isSlice = true;
						start = first;
						if (peek().type != TokenType::COLON && peek().type != TokenType::RBRACKET) end = parseExpr();
						if (match(TokenType::COLON)) if (peek().type != TokenType::RBRACKET) step = parseExpr();
					}
					else start = first;
				}
				consume(TokenType::RBRACKET, "Expected ']' to close index");
				if (isSlice) expr = setPos(new IndexExpr(expr, new SliceExpr(start, end, step)), bracket);
				else expr = setPos(new IndexExpr(expr, start), bracket);
			}
			else if (match(TokenType::DOT)) {
				Token dot = tokens[pos - 1];
				consume(TokenType::IDENTIFIER, "Expected method name after '.'");
				string method = tokens[pos - 1].value;
				consume(TokenType::LPAREN, "Expected '(' after method name");
				vector<Expr*> args;
				if (peek().type != TokenType::RPAREN)
					do { args.push_back(parseExpr()); } while (match(TokenType::COMMA));
				match(TokenType::RPAREN);
				expr = setPos(new MethodCallExpr(expr, method, args), dot);
			}
			else break;
		}
		return expr;
	}
	Stmt* parseForStep() {
		Expr* e = parseExpr();
		if (match(TokenType::ASSIGN)) {
			Token op = tokens[pos - 1];
			Expr* val = parseExpr();
			return setPos(new AssignStmt(e, TokenType::ASSIGN, val), op);
		}
		TokenType type = peek().type;
		if (type == TokenType::PLUS_EQ || type == TokenType::MINUS_EQ ||
			type == TokenType::STAR_EQ || type == TokenType::DIV_EQ ||
			type == TokenType::MOD_EQ || type == TokenType::POW_EQ ||
			type == TokenType::FLOOR_DIV_EQ || type == TokenType::AND_EQ ||
			type == TokenType::OR_EQ || type == TokenType::XOR_EQ) {
			Token op = advance();
			Expr* val = parseExpr();
			return setPos(new AssignStmt(e, type, val), op);
		}
		if (match(TokenType::INCREMENT)) {
			Token op = tokens[pos - 1];
			return setPos(new AssignStmt(e, TokenType::PLUS_EQ, setPos(new NumberExpr(1, false), op)), op);
		}
		if (match(TokenType::DECREMENT)) {
			Token op = tokens[pos - 1];
			return setPos(new AssignStmt(e, TokenType::MINUS_EQ, setPos(new NumberExpr(1, false), op)), op);
		}
		if (auto as = dynamic_cast<AssignStmt*>(e)) return as;
		Stmt* s = new ExprStmt(e);
		s->line = e->line;
		s->col = e->col;
		return s;
	}
	Stmt* parseStmt() {
		if (match(TokenType::SWITCH)) {
			Token t = tokens[pos - 1];
			consume(TokenType::LPAREN, "Expected '(' after 'switch'");
			Expr* target = parseExpr();
			consume(TokenType::RPAREN, "Expected ')' after switch value");
			consume(TokenType::COLON, "Expected ':' after ')'");
			consume(TokenType::LBRACE, "Expected '{' to open switch block");
			vector<Case> cases;
			bool expectingMore = true;
			while (match(TokenType::CASE)) {
				if (!expectingMore) {
					Token errTok = tokens[pos - 1];
					throw SyntaxError("Unreachable case. Previous case used 'do' (implying terminal), expected 'then do'.", errTok.line, errTok.col);
				}
				Expr* val = parseExpr();
				if (match(TokenType::THEN)) {
					consume(TokenType::DO, "Expected 'do' after 'then'");
					expectingMore = true;
				}
				else {
					consume(TokenType::DO, "Expected 'do' or 'then do' after case value");
					expectingMore = false;
				}
				consume(TokenType::COLON, "Expected ':' after case declaration");
				cases.push_back({ val, parseBlock() });
			}
			consume(TokenType::DEFAULT, "Expected 'default' block. Default is required.");
			consume(TokenType::DO, "Expected 'do' after default");
			consume(TokenType::COLON, "Expected ':' after default");
			vector<Stmt*> defaultBody = parseBlock();
			consume(TokenType::RBRACE, "Expected '}' to close switch block");
			return setPos(new SwitchStmt(target, cases, defaultBody), t);
		}
		if (match(TokenType::IMPORT)) {
			Token t = tokens[pos - 1];
			vector<string> symbols;
			if (peek().type == TokenType::IDENTIFIER) {
				string firstWord = advance().value;
				if (peek().type == TokenType::COMMA || peek().type == TokenType::FROM) {
					symbols.push_back(firstWord);
					while (match(TokenType::COMMA)) {
						consume(TokenType::IDENTIFIER, "Expected function name");
						symbols.push_back(tokens[pos - 1].value);
					}
					consume(TokenType::FROM, "Expected 'from' keyword");
					string libName;
					if (match(TokenType::STRING)) libName = tokens[pos - 1].value;
					else {
						consume(TokenType::IDENTIFIER, "Expected library name");
						libName = tokens[pos - 1].value;
					}
					return setPos(new ImportStmt(libName, symbols), t);
				}
				return setPos(new ImportStmt(firstWord, {}), t);
			}
			else if (match(TokenType::STRING)) return setPos(new ImportStmt(tokens[pos - 1].value, {}), t);
			else throw SyntaxError("Expected library name or identifier after 'import'", t.line, t.col);
		}
		if (match(TokenType::LET)) {
			Token t = tokens[pos - 1];
			vector<string> names;
			vector<bool> consts;
			vector<Expr*> values;
			do {
				bool isConst = match(TokenType::CONST);
				consume(TokenType::IDENTIFIER, "Expected variable name");
				names.push_back(tokens[pos - 1].value);
				consts.push_back(isConst);
			} while (match(TokenType::COMMA));
			bool isLocked = false;
			bool hasAssign = false;
			if (match(TokenType::COLON_EQ)) {
				isLocked = true;
				hasAssign = true;
			}
			else if (match(TokenType::ASSIGN)) {
				isLocked = false;
				hasAssign = true;
			}
			if (hasAssign) {
				do {values.push_back(parseExpr());} while (match(TokenType::COMMA));
			}
			if (values.empty()) for (size_t i = 0; i < names.size(); i++) values.push_back(nullptr);
			else if (names.size() != values.size()) throw SyntaxError("Mismatch in number of variables and values", t.line, t.col);
			if (names.size() == 1) return setPos(new LetStmt(names[0], values[0], consts[0], isLocked), t);
			return setPos(new MultiLetStmt(names, values, consts, isLocked), t);
		}
		if (match(TokenType::RETURN)) {
			Token t = tokens[pos - 1];
			Expr* value = nullptr;
			TokenType next = peek().type;
			if (next != TokenType::RBRACE &&
				next != TokenType::LET &&
				next != TokenType::IF &&
				next != TokenType::FOR &&
				next != TokenType::WHILE &&
				next != TokenType::DO &&
				next != TokenType::BREAK &&
				next != TokenType::CONTINUE &&
				next != TokenType::RETURN &&
				next != TokenType::END) {
				value = parseExpr();
			}
			return setPos(new ReturnStmt(value), t);
		}
		if (match(TokenType::BREAK)) return setPos(new BreakStmt(), tokens[pos - 1]);
		if (match(TokenType::CONTINUE)) return setPos(new ContinueStmt(), tokens[pos - 1]);
		if (match(TokenType::SKIP)) {
			Token t = tokens[pos - 1];
			Expr* count = parseExpr();
			return setPos(new SkipStmt(count), t);
		}
		bool isNamedFunction = false;
		if (peek().type == TokenType::DEFINE) {
			int offset = 1;
			if (tokens[pos + offset].type == TokenType::CACHED) offset++;
			if (tokens[pos + offset].type == TokenType::FUNCTION) isNamedFunction = true;
		}
		if (isNamedFunction) {
			Token defTok = advance();
			bool isCached = false;
			if (match(TokenType::CACHED)) isCached = true;
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
					if (match(TokenType::AT)) mode = CopyMode::REF;
					else if (match(TokenType::DOLLAR)) mode = CopyMode::DEEP;
					bool isConst = false;
					if (match(TokenType::CONST)) isConst = true;
					bool isVariadic = false;
					bool isKwargs = false;
					if (match(TokenType::POW)) {
						if (seenKwargs) error("Cannot have multiple **kwargs.");
						if (seenVariadic) {}
						isKwargs = true;
						seenKwargs = true;
					}
					else if (match(TokenType::STAR)) {
						if (seenVariadic) error("Cannot have multiple *args.");
						if (seenKwargs) error("*args cannot follow **kwargs.");
						isVariadic = true;
						seenVariadic = true;
					}
					if (!isVariadic && !isKwargs) {
						if (seenVariadic) error("Parameter cannot follow *args.");
						if (seenKwargs) error("Parameter cannot follow **kwargs.");
					}
					consume(TokenType::IDENTIFIER, "Expected parameter name");
					string pname = tokens[pos - 1].value;
					ValueType ptype = ValueType::NOTYPE;
					Expr* defaultExpr = nullptr;
					if (match(TokenType::COLON)) {
						string t;
						if (match(TokenType::FUNCTION)) t = "function";
						else if (match(TokenType::LAMBDA)) t = "lambda";
						else {
							consume(TokenType::IDENTIFIER, "Expected type name");
							t = tokens[pos - 1].value;
						}
						if (t == "int") ptype = ValueType::INT;
						else if (t == "float") ptype = ValueType::FLOAT;
						else if (t == "bool") ptype = ValueType::BOOL;
						else if (t == "string") ptype = ValueType::STRING;
						else if (t == "list") ptype = ValueType::LIST;
						else if (t == "range") ptype = ValueType::RANGE;
						else if (t == "set") ptype = ValueType::SET;
						else if (t == "tuple") ptype = ValueType::TUPLE;
						else if (t == "dictionary" || t == "dict") ptype = ValueType::DICT;
						else if (t == "vector") ptype = ValueType::VECTOR;
						else if (t == "function" || t == "lambda" ) ptype = ValueType::FUNCTION;
						else throw SyntaxError("Unknown type '" + t + "'", tokens[pos - 1].line, tokens[pos - 1].col);
						consume(TokenType::LPAREN, "Expected '(' after type definition");
						if (peek().type != TokenType::RPAREN) defaultExpr = parseExpr();
						consume(TokenType::RPAREN, "Expected ')' after type definition");
					}
					if (defaultExpr == nullptr && match(TokenType::ASSIGN)) {
						if (isVariadic || isKwargs) error("Variadic/Kwargs cannot have default values.");
						defaultExpr = parseExpr();
						seenDefault = true;
					}
					if (!isVariadic && !isKwargs && !defaultExpr && seenDefault) {
						error("Non-default parameter '" + pname + "' cannot follow default parameters.");
					}
					params.push_back({ pname, mode, ptype, defaultExpr, isConst, isVariadic, isKwargs});
				} while (match(TokenType::COMMA));
			}
			consume(TokenType::RPAREN, "Expected ')' after function parameters");
			bool retConst = false;
			vector<Expr*> retArgs;
			if (match(TokenType::ARROW)) {
				if (match(TokenType::CONST)) retConst = true;
				string t;
				if (match(TokenType::FUNCTION)) t = "function";
				else if (match(TokenType::LAMBDA)) t = "lambda";
				else {
					consume(TokenType::IDENTIFIER, "Expected return type");
					t = tokens[pos - 1].value;
				}
				if (t == "int") retType = ValueType::INT;
				else if (t == "float") retType = ValueType::FLOAT;
				else if (t == "bool") retType = ValueType::BOOL;
				else if (t == "string") retType = ValueType::STRING;
				else if (t == "list") retType = ValueType::LIST;
				else if (t == "None") retType = ValueType::NONE;
				else if (t == "NoType") retType = ValueType::NOTYPE;
				else if (t == "range") retType = ValueType::RANGE;
				else if (t == "set") retType = ValueType::SET;
				else if (t == "tuple") retType = ValueType::TUPLE;
				else if (t == "dictionary" || t == "dict") retType = ValueType::DICT;
				else if (t == "function" || t == "lambda") retType = ValueType::FUNCTION;
				else if (t == "vector") retType = ValueType::VECTOR;
				else throw SyntaxError("Unknown return type '" + t + "'", tokens[pos - 1].line, tokens[pos - 1].col);
				consume(TokenType::LPAREN, "Expected '(' after return type");
				if (peek().type != TokenType::RPAREN) {
					do {
						Expr* arg = parseExpr();
						if (match(TokenType::COLON)) {
							Token op = tokens[pos - 1];
							Expr* val = parseExpr();
							arg = setPos(new BinExpr(arg, val, TokenType::COLON), op);
						}
						retArgs.push_back(arg);
					} while (match(TokenType::COMMA));
				}
				consume(TokenType::RPAREN, "Expected ')' after return type");
			}
			consume(TokenType::COLON, "Expected ':' after function signature");
			vector<Stmt*> body = parseBlock();
			return setPos(new FuncStmt(fname, params, retArgs, body, retType, retConst, isCached), defTok);
		}
		if (match(TokenType::WHILE)) {
			Token t = tokens[pos - 1];
			Expr* cond = parseExpr();
			consume(TokenType::DO, "Expected 'do' after while condition");
			consume(TokenType::COLON, "Expected ':' after do");
			vector<Stmt*> body = parseBlock();
			return setPos(new WhileStmt(cond, body), t);
		}
		if (match(TokenType::DO)) {
			Token t = tokens[pos - 1];
			consume(TokenType::COLON, "Expected ':' after do");
			vector<Stmt*> body = parseBlock();
			consume(TokenType::WHILE, "Expected 'while' after do block");
			Expr* cond = parseExpr();
			return setPos(new DoWhileStmt(body, cond), t);
		}
		if (match(TokenType::FOR)) {
			Token t = tokens[pos - 1];
			bool isCStyle = false;
			if (peek().type == TokenType::LET || peek().type == TokenType::OMIT) isCStyle = true;
			else {
				int look = 0;
				while (tokens[pos + look].type == TokenType::IDENTIFIER || tokens[pos + look].type == TokenType::COMMA) {
					look++;
				}
				if (tokens[pos + look].type != TokenType::IN) isCStyle = true;
			}
			if (!isCStyle) {
				vector<string> loopVars;
				do {
					consume(TokenType::IDENTIFIER, "Expected loop variable");
					loopVars.push_back(tokens[pos - 1].value);
				} while (match(TokenType::COMMA));
				consume(TokenType::IN, "Expected 'in' after loop variables");
				vector<Expr*> collections;
				do {
					collections.push_back(parseExpr());
				} while (match(TokenType::COMMA));
				consume(TokenType::DO, "Expected 'do'");
				consume(TokenType::COLON, "Expected ':'");
				vector<Stmt*> body = parseBlock();
				return setPos(new ForEachStmt(loopVars, collections, body), t);
			}
			vector<Stmt*> inits;
			if (!match(TokenType::OMIT)) {
				if (match(TokenType::LET)) {
					bool isMultiMode = false;
					if (tokens[pos].type == TokenType::IDENTIFIER && tokens[pos + 1].type == TokenType::COMMA) {
						isMultiMode = true;
					}

					if (isMultiMode) {
						vector<string> names;
						do {
							consume(TokenType::IDENTIFIER, "Expected variable name");
							names.push_back(tokens[pos - 1].value);
						} while (match(TokenType::COMMA));
						consume(TokenType::ASSIGN, "Expected '=' after variable names");
						vector<Expr*> values;
						do {
							values.push_back(parseExpr());
						} while (match(TokenType::COMMA));
						if (names.size() != values.size()) {
							error("Mismatch: " + std::to_string(names.size()) + " variables but " + std::to_string(values.size()) + " values.");
						}
						for (size_t k = 0; k < names.size(); k++) {
							inits.push_back(new LetStmt(names[k], values[k], false, false));
						}
					}
					else {
						do {
							consume(TokenType::IDENTIFIER, "Expected variable name");
							string name = tokens[pos - 1].value;
							Expr* val = nullptr;
							if (match(TokenType::ASSIGN)) {
								val = parseExpr();
							}
							else {
								val = new NumberExpr(0, false);
							}
							inits.push_back(new LetStmt(name, val, false, false));
						} while (match(TokenType::COMMA));
					}
				}
				else {
					do {
						inits.push_back(parseForStep());
					} while (match(TokenType::COMMA));
				}
			}
			consume(TokenType::IF, "Expected 'if' in for loop");
			Expr* condition = nullptr;
			if (!match(TokenType::OMIT)) {
				condition = parseExpr();
			}
			vector<Stmt*> steps;
			if (match(TokenType::THEN)) {
				if (!match(TokenType::OMIT)) {
					do {
						steps.push_back(parseForStep());
					} while (match(TokenType::COMMA));
				}
				consume(TokenType::DO, "Expected 'do' after loop steps");
			}
			else {
				consume(TokenType::DO, "Expected 'do' after loop condition");
			}
			consume(TokenType::COLON, "Expected ':'");
			vector<Stmt*> body = parseBlock();

			return setPos(new ForStmt(inits, condition, steps, body), t);
		}
		if (match(TokenType::ASSERT)) {
			Token t = tokens[pos - 1];
			Expr* condition = parseExpr();
			Expr* message = nullptr;
			if (match(TokenType::COMMA)) message = parseExpr();
			return setPos(new AssertStmt(condition, message), t);
		}
		if (match(TokenType::IF)) {
			Token t = tokens[pos - 1];
			Expr* cond = parseExpr();
			match(TokenType::COLON);
			vector<Stmt*> body = parseBlock();
			vector<std::pair<Expr*, vector<Stmt*>>> elifs;
			while (match(TokenType::ELSE_IF)) {
				Expr* ec = parseExpr();
				match(TokenType::COLON);
				elifs.push_back({ ec, parseBlock() });
			}
			vector<Stmt*> elseBody;
			if (match(TokenType::ELSE)) {
				match(TokenType::COLON);
				elseBody = parseBlock();
			}
			return setPos(new IfStmt{ cond, body, elifs, elseBody }, t);
		}
		if (match(TokenType::TRY)) {
			Token t = tokens[pos - 1];
			consume(TokenType::COLON, "Expected ':' after try");
			vector<Stmt*> tryBody = parseBlock();
			vector<CatchBlock> catches;
			bool expectingMore = true;
			while (match(TokenType::CATCH)) {
				if (!expectingMore) {
					error("Unreachable catch block. Previous catch used 'do' (implying terminal), expected 'then do'.");
				}
				vector<string> types;
				do {
					consume(TokenType::IDENTIFIER, "Expected error type");
					types.push_back(tokens[pos - 1].value);
				} while (match(TokenType::COMMA));
				if (match(TokenType::THEN)) {
					consume(TokenType::DO, "Expected 'do' after 'then'");
					expectingMore = true;
				}
				else {
					consume(TokenType::DO, "Expected 'do' or 'then do' after catch types");
					expectingMore = false;
				}
				consume(TokenType::COLON, "Expected ':'");
				catches.push_back({ types, parseBlock() });
			}
			if (expectingMore && !catches.empty()) {
				error("Expected another 'catch' block after 'then do'.");
			}
			vector<Stmt*> elseBody;
			if (match(TokenType::ELSE)) {
				consume(TokenType::DO, "Expected 'do' after else");
				consume(TokenType::COLON, "Expected ':'");
				elseBody = parseBlock();
			}
			vector<Stmt*> finallyBody;
			if (match(TokenType::FINALLY)) {
				consume(TokenType::DO, "Expected 'do' after finally");
				consume(TokenType::COLON, "Expected ':'");
				finallyBody = parseBlock();
			}
			return setPos(new TryStmt(tryBody, catches, elseBody, finallyBody), t);
		}
		if (match(TokenType::THROW)) {
			Token t = tokens[pos - 1];
			consume(TokenType::IDENTIFIER, "Expected error type identifier after 'throw'");
			string typeName = tokens[pos - 1].value;
			consume(TokenType::COMMA, "Expected ',' after error type");
			Expr* msg = parseExpr();
			return setPos(new ThrowStmt(typeName, msg), t);
		}
		Expr* e = parseExpr();
		if (match(TokenType::COMMA)) {
			Token t = tokens[pos - 1];
			vector<Expr*> targets;
			targets.push_back(e);
			do {
				targets.push_back(parseExpr());
			} while (match(TokenType::COMMA));
			consume(TokenType::ASSIGN, "Expected '=' in multi-assignment");
			vector<Expr*> values;
			do {
				values.push_back(parseExpr());
			} while (match(TokenType::COMMA));
			if (targets.size() != values.size()) throw SyntaxError("Mismatch in number of targets and values", t.line, t.col);
			return setPos(new MultiAssignStmt(targets, values), t);
		}
		if (match(TokenType::ASSIGN) ||
			match(TokenType::PLUS_EQ) ||
			match(TokenType::MINUS_EQ) ||
			match(TokenType::STAR_EQ) ||
			match(TokenType::DIV_EQ) ||
			match(TokenType::MOD_EQ) ||
			match(TokenType::FLOOR_DIV_EQ) ||
			match(TokenType::POW_EQ) ||
			match(TokenType::AND_EQ) ||
			match(TokenType::OR_EQ) ||
			match(TokenType::XOR_EQ)) {
			Token op = tokens[pos - 1];
			Expr* rhs = parseExpr();
			return setPos(new AssignStmt(e, op.type, rhs), op);
		}
		if (match(TokenType::INCREMENT)) {
			Token op = tokens[pos - 1];
			if (!dynamic_cast<VarExpr*>(e) && !dynamic_cast<IndexExpr*>(e)) {
				throw SyntaxError("++ requires assignable expression", op.line, op.col);
			}
			return setPos(new AssignStmt(e, TokenType::PLUS_EQ, setPos(new NumberExpr(1, false), op)), op);
		}
		if (match(TokenType::DECREMENT)) {
			Token op = tokens[pos - 1];
			if (!dynamic_cast<VarExpr*>(e) && !dynamic_cast<IndexExpr*>(e)) {
				throw SyntaxError("-- requires assignable expression", op.line, op.col);
			}
			return setPos(new AssignStmt(e, TokenType::MINUS_EQ, setPos(new NumberExpr(1, false), op)), op);
		}
		Stmt* s = new ExprStmt(e);
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
using NativeFunc = std::function<Value(const std::vector<Value>&, int, int)>;
struct ValueHash {
	std::size_t operator()(const Value& v) const;
};
struct ValueEqual {
	bool operator()(const Value& a, const Value& b) const;
};
struct Value {
	ValueType type = ValueType::NOTYPE;
	std::string __DEBUGGING__NAME__ = "";
	long long iVal = 0;
	double fVal = 0.0;
	bool bVal = false;
	std::shared_ptr<HeapObject> ref;
	bool isConst = false;
	bool isLocked = false;
	Value* ptr = nullptr;
	void* adress = nullptr;
	static Value Reference(Value* p);
	static Value Int(long long v, bool locked = false, bool isConst = false);
	static Value pInt(void* v, bool locked = false, bool isConst = false);
	static Value BigInt(long long n);
	static Value BigInt(std::vector<uint32_t> chunks, bool isNegative);
	static Value BigInt(std::shared_ptr<BigIntObject> obj);
	static Value Float(double v, bool locked = false, bool isConst = false);
	static Value Bool(bool v, bool locked = false, bool isConst = false);
	static Value String(const string& v, bool locked = false, bool isConst = false);
	static Value None();
	static Value NoType();
	static Value List();
	static Value List(const std::vector<Value>& elems, bool locked = false, bool isConst = false);
	static Value Range(double s, double e, double st, bool si, bool ei, bool f, bool locked = false, bool isConst = false);
	static Value Set();
	static Value Set(const std::vector<Value>& elems, bool locked = false);
	static Value Tuple(const std::vector<Value>& elems, bool locked = false);
	static Value Dict(const std::unordered_map<Value, Value, ValueHash, ValueEqual>& m, bool locked = false);
	static Value Paired(const std::vector<std::pair<Value, Value>>& p);
	static Value Native(NativeFunc f);
	static Value Overload(const Value& first);
	static Value Omit();
	static Value File(const string& path);
	static Value Slice(Value s, Value e, Value p);
	static Value Vector(const std::vector<Value>& elems);
	static Value Error(std::shared_ptr<ErrorObject> e);
	bool isTruthy() const;
	bool strictEquals(const Value& other) const;
	bool looseEquals(const Value& other) const;
	bool isNumber() const;
	long long asInt() const;
	void* aspInt() const;
	double asFloat() const;
	bool asBool() const;
	const string& asString() const;
	bool sameType(const Value& other) const { return type == other.type; }
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
	StringObject(const string& v, bool locked = false) : HeapObject(ValueType::STRING, locked), value(v) {}
};
struct ListObject : HeapObject {
	std::vector<Value> elements;
	ListObject() : HeapObject(ValueType::LIST) {}
	ListObject(const std::vector<Value>& elems, bool locked = false) : HeapObject(ValueType::LIST, locked), elements(elems) {}
};
struct RangeObject : HeapObject {
	double start, end, step;
	bool startInclusive, endInclusive, isFloat, isValid;
	RangeObject(double s, double e, double st, bool si, bool ei, bool f, bool valid, bool locked = false)
		: HeapObject(ValueType::RANGE, locked), start(s), end(e), step(st), startInclusive(si), endInclusive(ei), isFloat(f), isValid(valid) {}
};
struct SetObject : HeapObject {
	std::vector<Value> elements;
	SetObject() : HeapObject(ValueType::SET) {}
	SetObject(const std::vector<Value>& e, bool locked = false) : HeapObject(ValueType::SET, locked), elements(e) {}
};
struct TupleObject : HeapObject {
	std::vector<Value> elements;
	TupleObject() : HeapObject(ValueType::TUPLE) {}
	TupleObject(const std::vector<Value>& elems, bool locked = false) : HeapObject(ValueType::TUPLE, locked), elements(elems) {}
};
struct DictObject : HeapObject {
	std::unordered_map<Value, Value, ValueHash, ValueEqual> items;
	DictObject() : HeapObject(ValueType::DICT) {}
	DictObject(const std::unordered_map<Value, Value, ValueHash, ValueEqual>& m, bool locked = false)
		: HeapObject(ValueType::DICT, locked), items(m) {}
};
struct PairedObject : HeapObject {
	std::vector<std::pair<Value, Value>> pairs;
	PairedObject(const std::vector<std::pair<Value, Value>>& p) : HeapObject(ValueType::PAIRED), pairs(p) {}
};
struct NativeFunctionObject : HeapObject {
	NativeFunc func;
	NativeFunctionObject(NativeFunc f) : HeapObject(ValueType::NATIVE_FUNCTION), func(f) {}
};
struct OverloadObject : HeapObject {
	std::vector<Value> overloads;
	OverloadObject(const Value& v) : HeapObject(ValueType::OVERLOAD) { overloads.push_back(v); }
};
struct FileObject : HeapObject {
	std::fstream stream;
	string path;
	bool isOpen;
	FileObject(const string& p) : HeapObject(ValueType::FILE), path(p) {
		stream.open(p, std::ios::in | std::ios::out | std::ios::binary);
		isOpen = stream.is_open();
	}
	~FileObject() { if (isOpen) stream.close(); }
};
struct SliceObject : HeapObject {
	Value start;
	Value end;
	Value step;
	SliceObject(Value s, Value e, Value p) : HeapObject(ValueType::SLICE), start(s), end(e), step(p) {}
};
struct VectorObject : HeapObject {
	vector<Value> elements;
	VectorObject(const vector<Value>& e) : HeapObject(ValueType::VECTOR), elements(e) {}
};
struct BigIntObject : HeapObject {
	bool isNegative;
	std::vector<uint32_t> chunks;
	BigIntObject(long long n) : HeapObject(ValueType::BIGINT) {
		if (n < 0) { isNegative = true; n = -n; }
		else isNegative = false;
		if (n == 0) chunks.push_back(0);
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
		while (chunks.size() > 1 && chunks.back() == 0) chunks.pop_back();
		if (chunks.size() == 1 && chunks[0] == 0) isNegative = false;
	}
	bool operator==(const BigIntObject& other) const {
		return isNegative == other.isNegative && chunks == other.chunks;
	}
	bool operator<(const BigIntObject& other) const {
		if (isNegative != other.isNegative) return isNegative;
		if (chunks.size() != other.chunks.size())
			return isNegative ? chunks.size() > other.chunks.size() : chunks.size() < other.chunks.size();
		for (int i = chunks.size() - 1; i >= 0; i--) {
			if (chunks[i] != other.chunks[i]) return isNegative ? chunks[i] > other.chunks[i] : chunks[i] < other.chunks[i];
		}
		return false;
	}
	bool operator>(const BigIntObject& other) const { return other < *this; }
	bool absLess(const BigIntObject& other) const {
		if (chunks.size() != other.chunks.size()) return chunks.size() < other.chunks.size();
		for (int i = chunks.size() - 1; i >= 0; i--) if (chunks[i] != other.chunks[i]) return chunks[i] < other.chunks[i];
		return false;
	}
	BigIntObject absAdd(const BigIntObject& other) const {
		std::vector<uint32_t> res;
		uint64_t carry = 0;
		size_t n = std::max(chunks.size(), other.chunks.size());
		res.reserve(n + 1);
		for (size_t i = 0; i < n || carry; i++) {
			uint64_t sum = carry + (i < chunks.size() ? chunks[i] : 0) + (i < other.chunks.size() ? other.chunks[i] : 0);
			res.push_back((uint32_t)(sum & 0xFFFFFFFF));
			carry = sum >> 32;
		}
		return BigIntObject(res, false);
	}
	BigIntObject absSub(const BigIntObject& other) const {
		std::vector<uint32_t> res;
		int64_t borrow = 0;
		size_t n = chunks.size();
		res.reserve(n);
		for (size_t i = 0; i < n; i++) {
			int64_t sub = (int64_t)chunks[i] - (i < other.chunks.size() ? other.chunks[i] : 0) - borrow;
			if (sub < 0) {
				sub += 4294967296LL;
				borrow = 1;
			}
			else borrow = 0;
			res.push_back((uint32_t)sub);
		}
		return BigIntObject(res, false);
	}
	BigIntObject operator+(const BigIntObject& other) const {
		if (isNegative == other.isNegative) {
			BigIntObject res = absAdd(other);
			res.isNegative = isNegative;
			return res;
		}
		else {
			if (absLess(other)) {
				BigIntObject res = other.absSub(*this);
				res.isNegative = other.isNegative;
				return res;
			}
			else {
				BigIntObject res = absSub(other);
				res.isNegative = isNegative;
				return res;
			}
		}
	}
	BigIntObject operator-(const BigIntObject& other) const {
		if (isNegative != other.isNegative) {
			BigIntObject res = absAdd(other);
			res.isNegative = isNegative;
			return res;
		}
		else {
			if (absLess(other)) {
				BigIntObject res = other.absSub(*this);
				res.isNegative = !isNegative;
				return res;
			}
			else {
				BigIntObject res = absSub(other);
				res.isNegative = isNegative;
				return res;
			}
		}
	}
	BigIntObject operator*(const BigIntObject& other) const {
		size_t n = chunks.size(), m = other.chunks.size();
		std::vector<uint32_t> res(n + m, 0);
		for (size_t i = 0; i < n; i++) {
			uint64_t carry = 0;
			for (size_t j = 0; j < m; j++) {
				uint64_t prod = (uint64_t)chunks[i] * other.chunks[j] + res[i + j] + carry;
				res[i + j] = (uint32_t)(prod & 0xFFFFFFFF);
				carry = prod >> 32;
			}
			res[i + m] += (uint32_t)carry;
		}
		std::fill(res.begin(), res.end(), 0);
		for (size_t i = 0; i < n; i++) {
			uint64_t carry = 0;
			for (size_t j = 0; j < m; j++) {
				uint64_t prod = (uint64_t)chunks[i] * other.chunks[j] + res[i + j] + carry;
				res[i + j] = (uint32_t)(prod & 0xFFFFFFFF);
				carry = prod >> 32;
			}
			res[i + m] += (uint32_t)carry;
		}
		return BigIntObject(res, isNegative != other.isNegative);
	}
	std::pair<BigIntObject, BigIntObject> divMod(const BigIntObject& other) const {
		if (other.chunks.size() == 1 && other.chunks[0] == 0) throw std::runtime_error("Divide by zero");
		BigIntObject dividend = *this; dividend.isNegative = false;
		BigIntObject divisor = other; divisor.isNegative = false;
		BigIntObject quotient(0);
		BigIntObject remainder(0);
		if (dividend < divisor) return { BigIntObject(0), *this };
		size_t nBits = dividend.chunks.size() * 64;
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
		return { quotient, remainder };
	}
	BigIntObject operator/(const BigIntObject& other) const { return divMod(other).first; }
	BigIntObject operator%(const BigIntObject& other) const { return divMod(other).second; }
	void lshift(int shift) {
		if (shift == 0) return;
		uint32_t carry = 0;
		for (size_t i = 0; i < chunks.size(); i++) {
			uint64_t nextCarry = chunks[i] >> 31;
			chunks[i] = (chunks[i] << 1) | carry;
			carry = nextCarry;
		}
		if (carry) chunks.push_back(carry);
	}
	void setBit(int n) {
		int chunkIdx = n / 32;
		int bitIdx = n % 32;
		if (chunkIdx >= chunks.size()) chunks.resize(chunkIdx + 1, 0);
		chunks[chunkIdx] |= (1U << bitIdx);
	}
	static std::pair<BigIntObject*, BigIntObject*> promote(Value& a, Value& b) {
		BigIntObject* ba = (a.type == ValueType::BIGINT) ? static_cast<BigIntObject*>(a.ref.get()) : new BigIntObject(a.asInt());
		BigIntObject* bb = (b.type == ValueType::BIGINT) ? static_cast<BigIntObject*>(b.ref.get()) : new BigIntObject(b.asInt());
		return { ba, bb };
	}
	static Value add(Value a, Value b) { auto [ba, bb] = promote(a, b); return Value::BigInt(std::make_shared<BigIntObject>(*ba + *bb)); }
	static Value sub(Value a, Value b) { auto [ba, bb] = promote(a, b); return Value::BigInt(std::make_shared<BigIntObject>(*ba - *bb)); }
	static Value mul(Value a, Value b) { auto [ba, bb] = promote(a, b); return Value::BigInt(std::make_shared<BigIntObject>(*ba * *bb)); }
	static Value div(Value a, Value b) { auto [ba, bb] = promote(a, b); return Value::BigInt(std::make_shared<BigIntObject>(*ba / *bb)); }
	static Value mod(Value a, Value b) { auto [ba, bb] = promote(a, b); return Value::BigInt(std::make_shared<BigIntObject>(*ba % *bb)); }
	static Value pow(Value a, Value b) {
		auto [ba, bb] = promote(a, b);
		if (bb->isNegative) return Value::Int(0);
		BigIntObject base = *ba;
		BigIntObject exp = *bb;
		BigIntObject res(1);
		while (!exp.absLess(BigIntObject(1)) && !(exp.chunks.size() == 1 && exp.chunks[0] == 0)) {
			if (exp.chunks[0] & 1) res = res * base;
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
	ErrorObject(string type, string msg, long long co ,int l, int c)
		: HeapObject(ValueType::ERROR), errType(type), message(msg), code(co), line(l), col(c) {}
	string toString() const  {
		return errType + ": " + message;
	}
};
inline Value Value::Reference(Value* p) {
	Value v;
	v.type = ValueType::REFERENCE;
	v.ptr = p;
	return v;
}
inline Value Value::Int(long long v, bool locked, bool isConst) {
	Value x;
	x.type = ValueType::INT;
	x.iVal = v;
	x.isConst = isConst;
	return x;
}
inline Value Value::pInt(void* v, bool locked, bool isConst) {
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
inline Value Value::String(const string& v, bool locked, bool isConst) {
	Value x; x.type = ValueType::STRING; x.ref = std::make_shared<StringObject>(v, locked); return x;
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
	Value x; x.type = ValueType::LIST; x.ref = std::make_shared<ListObject>(); return x;
}
inline Value Value::List(const std::vector<Value>& elems, bool locked, bool isConst) {
	Value x; x.type = ValueType::LIST; x.ref = std::make_shared<ListObject>(elems, locked); return x;
}
inline Value Value::Set() {
	Value x; x.type = ValueType::SET; x.ref = std::make_shared<SetObject>(); return x;
}
inline Value Value::Set(const std::vector<Value>& elems, bool locked) {
	Value x; x.type = ValueType::SET; x.ref = std::make_shared<SetObject>(elems, locked); return x;
}
inline Value Value::Tuple(const std::vector<Value>& elems, bool locked) {
	Value x; x.type = ValueType::TUPLE;
	std::vector<Value> constElems = elems;
	for (auto& e : constElems) e.isConst = true;
	x.ref = std::make_shared<TupleObject>(constElems, locked);
	return x;
}
inline Value Value::Dict(const std::unordered_map<Value, Value, ValueHash, ValueEqual>& m, bool locked) {
	Value x; x.type = ValueType::DICT; x.ref = std::make_shared<DictObject>(m, locked); return x;
}
inline Value Value::Paired(const std::vector<std::pair<Value, Value>>& p) {
	Value x; x.type = ValueType::PAIRED; x.ref = std::make_shared<PairedObject>(p); return x;
}
inline Value Value::Native(NativeFunc f) {
	Value x; x.type = ValueType::NATIVE_FUNCTION; x.ref = std::make_shared<NativeFunctionObject>(f); return x;
}
inline Value Value::Overload(const Value& first) {
	Value x; x.type = ValueType::OVERLOAD; x.ref = std::make_shared<OverloadObject>(first); return x;
}
inline Value Value::Omit() {
	Value v;
	v.type = ValueType::OMIT_MARKER;
	return v;
}
inline Value Value::File(const string& path) {
	Value x; x.type = ValueType::FILE; x.ref = std::make_shared<FileObject>(path); return x;
}
inline Value Value::Range(double s, double e, double st, bool si, bool ei, bool f, bool locked, bool isConst) {
	bool isInvalid = false;
	if (st == 0) isInvalid = true;
	else if (st > 0) { if (s > e) isInvalid = true; else if (s == e && (!si || !ei)) isInvalid = true; }
	else { if (s < e) isInvalid = true; else if (s == e && (!si || !ei)) isInvalid = true; }
	if (isInvalid) { s = 0; e = 0; st = 1; si = false; ei = false; f = false; }
	Value x; x.type = ValueType::RANGE;
	x.ref = std::make_shared<RangeObject>(s, e, st, si, ei, f, !isInvalid, locked);
	return x;
}
inline Value Value::Slice(Value s, Value e, Value p) {
	Value x; x.type = ValueType::SLICE;
	x.ref = std::make_shared<SliceObject>(s, e, p);
	return x;
}
inline Value Value::Vector(const std::vector<Value>& elems) {
	Value x; x.type = ValueType::VECTOR;
	x.ref = std::make_shared<VectorObject>(elems);
	return x;
}
inline Value Value::Error(std::shared_ptr<ErrorObject> e) {
	Value v; v.type = ValueType::ERROR;
	v.ref = e; v.__DEBUGGING__NAME__=e->errType;
	return v;
}
inline bool Value::isTruthy() const {
	if (type == ValueType::NONE || type == ValueType::NOTYPE) return false;
	if (type == ValueType::BOOL) return bVal;
	if (type == ValueType::INT) return iVal != 0;
	if (type == ValueType::FLOAT) return fVal != 0.0;
	if (type == ValueType::STRING) return !static_cast<StringObject*>(ref.get())->value.empty();
	if (type == ValueType::LIST) return !static_cast<ListObject*>(ref.get())->elements.empty();
	if (type == ValueType::SET) return !static_cast<SetObject*>(ref.get())->elements.empty();
	if (type == ValueType::TUPLE) return !static_cast<TupleObject*>(ref.get())->elements.empty();
	if (type == ValueType::DICT) return !static_cast<DictObject*>(ref.get())->items.empty();
	if (type == ValueType::RANGE) {
		auto* r = static_cast<RangeObject*>(ref.get());
		if (!r->isValid || r->step == 0) return false;
		double s = r->start;
		if (!r->startInclusive) s += r->step;
		if (r->step > 0) return (s < r->end) || (r->endInclusive && s == r->end);
		else return (s > r->end) || (r->endInclusive && s == r->end);
	}
	return false;
}
inline long long Value::asInt() const {
	if (type == ValueType::INT) return iVal;
	if (type == ValueType::FLOAT) return (long long)fVal;
	if (type == ValueType::BOOL) return bVal ? 1 : 0;
	return 0;
}
inline void* Value::aspInt() const {
	return adress;
}
inline double Value::asFloat() const {
	if (type == ValueType::FLOAT) return fVal;
	if (type == ValueType::INT) return (double)iVal;
	if (type == ValueType::BOOL) return bVal ? 1.0 : 0.0;
	return 0.0;
}
inline bool Value::asBool() const {
	if (type == ValueType::BOOL) return bVal;
	if (type == ValueType::INT) return iVal != 0;
	if (type == ValueType::FLOAT) return fVal != 0.0;
	return isTruthy();
}
inline const string& Value::asString() const {
	static string empty = "";
	if (type == ValueType::STRING) return static_cast<StringObject*>(ref.get())->value;
	return empty;
}
inline bool Value::isNumber() const { return type == ValueType::INT || type == ValueType::FLOAT || type == ValueType::BIGINT; }
inline bool Value::strictEquals(const Value& other) const {
	if (type != other.type) return false;
	switch (type) {
	case ValueType::INT: return iVal == other.iVal;
	case ValueType::FLOAT: return fVal == other.fVal;
	case ValueType::BOOL: return bVal == other.bVal;
	case ValueType::STRING: return asString() == other.asString();
	case ValueType::NONE:
	case ValueType::NOTYPE: return true;
	case ValueType::BIGINT: {
		return *static_cast<BigIntObject*>(ref.get())== *static_cast<BigIntObject*>(other.ref.get());
	}
	case ValueType::LIST: {
		auto* l1 = static_cast<ListObject*>(ref.get());
		auto* l2 = static_cast<ListObject*>(other.ref.get());
		if (l1->elements.size() != l2->elements.size()) return false;
		for (size_t i = 0; i < l1->elements.size(); ++i) if (!l1->elements[i].strictEquals(l2->elements[i])) return false;
		return true;
	}
	case ValueType::SET: {
		auto* s1 = static_cast<SetObject*>(ref.get());
		auto* s2 = static_cast<SetObject*>(other.ref.get());
		if (s1->elements.size() != s2->elements.size()) return false;
		for (const auto& v1 : s1->elements) {
			bool found = false;
			for (const auto& v2 : s2->elements) if (v1.strictEquals(v2)) { found = true; break; }
			if (!found) return false;
		}
		return true;
	}
	case ValueType::RANGE: {
		auto* r1 = static_cast<RangeObject*>(ref.get());
		auto* r2 = static_cast<RangeObject*>(other.ref.get());
		return r1->start == r2->start && r1->end == r2->end && r1->step == r2->step && r1->startInclusive == r2->startInclusive && r1->endInclusive == r2->endInclusive;
	}
	case ValueType::TUPLE: {
		auto* t1 = static_cast<TupleObject*>(ref.get());
		auto* t2 = static_cast<TupleObject*>(other.ref.get());
		if (t1->elements.size() != t2->elements.size()) return false;
		for (size_t i = 0; i < t1->elements.size(); ++i) if (!t1->elements[i].strictEquals(t2->elements[i])) return false;
		return true;
	}
	case ValueType::DICT: {
		auto* d1 = static_cast<DictObject*>(ref.get());
		auto* d2 = static_cast<DictObject*>(other.ref.get());
		if (d1->items.size() != d2->items.size()) return false;
		for (const auto& [k, v] : d1->items) {
			if (d2->items.find(k) == d2->items.end()) return false;
			if (!v.strictEquals(d2->items.at(k))) return false;
		}
		return true;
	}
	case ValueType::PAIRED: return false;
	case ValueType::VECTOR: {
		auto* v1 = static_cast<VectorObject*>(ref.get());
		auto* v2 = static_cast<VectorObject*>(other.ref.get());
		if (v1->elements.size() != v2->elements.size()) return false;
		for (size_t i = 0; i < v1->elements.size(); i++) {
			if (std::abs(v1->elements[i].asInt() - v2->elements[i].asInt()) > 1e-9) return false;
		}
		return true;
	}
	}
	return false;
}
inline bool Value::looseEquals(const Value& other) const {
	return isTruthy() == other.isTruthy();
}
inline std::size_t ValueHash::operator()(const Value& v) const {
	size_t seed = 0;
	auto hash_combine = [&](size_t hash) {
		seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	};
	switch (v.type) {
	case ValueType::INT: hash_combine(std::hash<long long>{}(v.asInt())); break;
	case ValueType::FLOAT: hash_combine(std::hash<double>{}(v.asFloat())); break;
	case ValueType::BOOL: hash_combine(std::hash<bool>{}(v.asBool())); break;
	case ValueType::STRING: hash_combine(std::hash<string>{}(v.asString())); break;
	case ValueType::NONE: hash_combine(0); break;
	case ValueType::TUPLE: {
		auto* t = static_cast<TupleObject*>(v.ref.get());
		for (const auto& elem : t->elements)
			hash_combine(ValueHash{}(elem));
		break;
	}
	case ValueType::LIST: {
		auto* l = static_cast<ListObject*>(v.ref.get());
		for (const auto& elem : l->elements)
			hash_combine(ValueHash{}(elem));
		break;
	}
	case ValueType::SET: {
		auto* s = static_cast<SetObject*>(v.ref.get());
		size_t xorSum = 0;
		for (const auto& elem : s->elements)
			xorSum ^= ValueHash{}(elem);
		hash_combine(xorSum);
		break;
	}
	case ValueType::VECTOR: {
		auto* vec = static_cast<VectorObject*>(v.ref.get());
		std::size_t seed = vec->elements.size();
		for (auto& elem : vec->elements) {
			hash_combine(ValueHash{}(elem));
		}
		break;
	}
	default:
		hash_combine(std::hash<int>{}((int)v.type));
	}
	return seed;
}
inline bool ValueEqual::operator()(const Value& a, const Value& b) const {
	return a.strictEquals(b);
}
struct VectorHash {
	std::size_t operator()(const std::vector<Value>& vec) const {
		std::size_t seed = vec.size();
		ValueHash hasher;
		for (const auto& v : vec) seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};
struct VectorEqual {
	bool operator()(const std::vector<Value>& a, const std::vector<Value>& b) const {
		if (a.size() != b.size()) return false;
		for (size_t i = 0; i < a.size(); ++i) if (!(a[i].strictEquals(b[i]))) return false;
		return true;
	}
};
static void setAdd(std::vector<Value>& elems, const Value& v);
Value shallowCopy(const Value& v) {
	switch (v.type) {
	case ValueType::LIST: {
		auto* oldObj = static_cast<ListObject*>(v.ref.get());
		return Value::List(oldObj->elements);
	}
	case ValueType::SET: {
		auto* oldObj = static_cast<SetObject*>(v.ref.get());
		return Value::Set(oldObj->elements);
	}
	case ValueType::TUPLE: {
		auto* oldObj = static_cast<TupleObject*>(v.ref.get());
		return Value::Tuple(oldObj->elements);
	}
	case ValueType::DICT: {
		auto* oldObj = static_cast<DictObject*>(v.ref.get());
		return Value::Dict(oldObj->items, v.ref->typeLocked);
	}
	default:
		return v;
	}
}
Value deepCopy(const Value& v) {
	Value out;
	if (v.type == ValueType::INT || v.type == ValueType::FLOAT || v.type == ValueType::BOOL) {
		return v;
	}
	switch (v.type) {
	case ValueType::STRING:
		out = Value::String(static_cast<StringObject*>(v.ref.get())->value);
		break;
	case ValueType::LIST: {
		auto* oldList = static_cast<ListObject*>(v.ref.get());
		std::vector<Value> copied;
		for (const auto& el : oldList->elements) copied.push_back(deepCopy(el));
		out = Value::List(copied);
		break;
	}
	case ValueType::BIGINT: {
		auto* oldBig = static_cast<BigIntObject*>(v.ref.get());
		out = Value::BigInt(oldBig->chunks, oldBig->isNegative);
		break;
	}
	case ValueType::SET: {
		auto* oldSet = static_cast<SetObject*>(v.ref.get());
		std::vector<Value> copied;
		for (const auto& el : oldSet->elements) copied.push_back(deepCopy(el));
		out = Value::Set(copied);
		break;
	}
	case ValueType::DICT: {
		auto* oldDict = static_cast<DictObject*>(v.ref.get());
		std::unordered_map<Value, Value, ValueHash, ValueEqual> copied;
		for (const auto& [key, val] : oldDict->items) copied[deepCopy(key)] = deepCopy(val);
		out = Value::Dict(copied, v.ref->typeLocked);
		break;
	}
	case ValueType::VECTOR: {
		auto* vec = static_cast<VectorObject*>(v.ref.get());
		std::vector<Value> copied;
		copied.reserve(vec->elements.size());
		for (const auto& el : vec->elements) copied.push_back(deepCopy(el));
		out = Value::Vector(vec->elements);
		break;
	}
	default:
		out = v;
		break;
	}
	return out;
}
Value applyCopy(const Value& v, CopyMode mode) {
	switch (mode) {
	case CopyMode::REF: return v;
	case CopyMode::DEEP: return deepCopy(v);
	case CopyMode::SHALLOW:
	default: return shallowCopy(v);
	}
}
static Value defaultOf(ValueType t) {
	switch (t) {
	case ValueType::INT: return Value::Int(0);
	case ValueType::FLOAT: return Value::Float(0.0);
	case ValueType::BOOL: return Value::Bool(false);
	case ValueType::STRING: return Value::String("");
	case ValueType::NONE: return Value::None();
	case ValueType::NOTYPE: return Value::NoType();
	case ValueType::LIST: return Value::List({});
	case ValueType::DICT: return Value::Dict({});
	case ValueType::SET: return Value::Set({});
	case ValueType::TUPLE: return Value::Tuple({});
	}
	return Value::NoType();
}
struct Var {
	Value value;
	Value* alias = nullptr;
	bool isConst = false;
	bool isLocked = false;
};
struct LValue {
	Value* ref = nullptr;
	bool isRefTarget = false;
	bool isConstView = false;
	bool isLocked = false;
	LValue() = default;
	LValue(Value* r, bool i, bool c = false, bool l = false) : ref(r), isRefTarget(i), isConstView(c), isLocked(l) {}
};
struct CallArg { Value value; LValue lvalue; bool hasLValue = false; };
struct Env {
	unordered_map<string, Var> vars;
	std::shared_ptr<Env> parent = nullptr;
	Var& lookup(const string& n) {
		if (vars.count(n)) return vars[n];
		if (parent) return parent->lookup(n);
		static Var nullVar;
		return nullVar;
	}
	bool existsLocal(const string& n) {
		return vars.count(n);
	}
	void set(const string& n, Value v, bool locked, bool isConstVar = false) {
		v.__DEBUGGING__NAME__=n;
		if (vars.count(n)) {
			Var& existing = vars[n];
			if (existing.isLocked && existing.value.type != v.type) {
				throw RuntimeError("Type mismatch: variable '" + n + "' is type-locked.", 0, 0);
			}
			if (existing.isConst) throw RuntimeError("Cannot reassign a constant variable '" + n + "'", 0, 0);
			existing.value = v;
			v.isLocked = existing.isLocked;
			v.isConst = existing.isConst;
			return;
		}
		v.isLocked = locked;
		v.isConst = isConstVar;
		if (locked && v.ref) v.ref->typeLocked = true;
		vars[n] = Var{ v, nullptr, isConstVar, locked };
	}
	Value get(const string& n) {
		Var& var = lookup(n);
		Value val = var.alias ? *var.alias : var.value;
		val.isConst = var.isConst;
		val.isLocked = var.isLocked;
		return val;
	}
	void assign(const string& n, Value v) {
		Var& var = lookup(n);

		if (var.alias) {
			*var.alias = v;
			return;
		}

		var.value = v;
	}
	bool exists(const string& n) {
		if (vars.count(n)) return true;
		if (parent) return parent->exists(n); // <--- Recursively check parent
		return false;
	}
};
static inline int divMod10(std::vector<uint32_t>& chunks) {
	uint64_t remainder = 0;
	for (int i = chunks.size() - 1; i >= 0; i--) {
		uint64_t combined = (remainder << 32) | chunks[i];
		chunks[i] = (uint32_t)(combined / 10);
		remainder = combined % 10;
	}
	while (chunks.size() > 1 && chunks.back() == 0) chunks.pop_back();
	return (int)remainder;
}
static inline std::string bigIntToString(BigIntObject* big) {
	if (big->chunks.empty()) return "0";
	if (big->chunks.size() == 1 && big->chunks[0] == 0) return "0";
	std::vector<uint32_t> temp = big->chunks;
	std::string res = "";
	while (temp.size() > 1 || temp[0] > 0) {
		int digit = divMod10(temp);
		res += std::to_string(digit);
	}
	if (res.empty()) return "0";
	if (big->isNegative) res += "-";
	std::reverse(res.begin(), res.end());
	return res;
}
static inline std::string formatNumber(double val) {
	std::string s = std::to_string(val);
	s.erase(s.find_last_not_of('0') + 1, std::string::npos);
	if (s.back() == '.') s.pop_back();
	return s;
}
static inline std::string ptr_to_string(void* p) {
	std::ostringstream oss;
	oss << p;
	return oss.str();
}
static inline std::string valueToString(const Value& v, int line = 0, int col = 0) {
	switch (v.type) {
	case ValueType::BOOL:   return v.asBool() ? "true" : "false";
	case ValueType::NONE:   return "None";
	case ValueType::NOTYPE: return "Notype";
	case ValueType::FLOAT:  return formatNumber(v.asFloat());
	case ValueType::INT:    return v.adress? ptr_to_string(v.adress):std::to_string(v.asInt());
	case ValueType::STRING: return v.asString();
	case ValueType::FUNCTION: return "<function>";
	case ValueType::NATIVE_FUNCTION: return "<native_function>";
	case ValueType::ERROR: {
		auto* err = static_cast<ErrorObject*>(v.ref.get());
		return "<Error: " + err->errType + ": " + err->message + ">";
	}
	case ValueType::SET: {
		auto* s = static_cast<SetObject*>(v.ref.get());
		string str = "{";
		for (size_t i = 0; i < s->elements.size(); i++) {
			str += valueToString(s->elements[i], line, col);
			if (i + 1 < s->elements.size()) str += ", ";
		}
		str += "}";
		return str;
	}
	case ValueType::LIST: {
		auto* s = static_cast<ListObject*>(v.ref.get());
		string str = "[";
		for (size_t i = 0; i < s->elements.size(); i++) {
			str += valueToString(s->elements[i], line, col);
			if (i + 1 < s->elements.size()) str += ", ";
		}
		str += "]";
		return str;
	}
	case ValueType::RANGE: {
		auto* r = static_cast<RangeObject*>(v.ref.get());
		if (!r->isValid) return "(...)";
		string s = "";
		auto fmt = [](double d, bool isF) { return isF ? std::to_string(d) : std::to_string((long long)d); };
		if (r->start > r->end) {
			s += (r->endInclusive ? "[" : "(") + fmt(r->end, r->isFloat);
			s += ", " + fmt(r->start, r->isFloat) + (r->startInclusive ? "]" : ")");
		}
		else {
			s += (r->startInclusive ? "[" : "(") + fmt(r->start, r->isFloat);
			s += ", " + fmt(r->end, r->isFloat) + (r->endInclusive ? "]" : ")");
		}
		if (r->step != 1) s += ".." + fmt(r->step, r->isFloat);
		return s;
	}
	case ValueType::TUPLE: {
		auto* list = static_cast<TupleObject*>(v.ref.get());
		if (list->elements.empty()) return "(,)";
		string str = "(";
		for (size_t i = 0; i < list->elements.size(); i++) {
			str += valueToString(list->elements[i], line, col);
			if (i + 1 < list->elements.size()) str += ", ";
			else if (list->elements.size() == 1) str += ",";
		}
		str += ")";
		return str;
	}
	case ValueType::DICT: {
		auto* d = static_cast<DictObject*>(v.ref.get());
		string s = "{";
		size_t count = 0;
		for (const auto& [key, val] : d->items) {
			s += valueToString(key, line, col) + ": " + valueToString(val, line, col);
			if (count++ < d->items.size() - 1) s += ", ";
		}
		s += "}";
		return s;
	}
	case ValueType::VECTOR: {
		auto* vec = static_cast<VectorObject*>(v.ref.get());
		string s = "<";
		for (size_t i = 0; i < vec->elements.size(); i++) {
			s += valueToString(vec->elements[i]);
			if (i < vec->elements.size() - 1) s += ", ";
		}
		s += ">";
		return s;
	}
	case ValueType::BIGINT: {
		auto* big = static_cast<BigIntObject*>(v.ref.get());
		return bigIntToString(big);
	}
	case ValueType::REFERENCE:{
		return valueToString(*v.ptr);
	}
	default: throw TypeError("Cannot implicitly convert this type to string", line, col);
	}
}
static inline std::string PrintProperty(const Value& v) {
	if(v.isConst && v.isLocked) return "Locked Const ";
	else if (v.isConst) return "Const ";
	else if (v.isLocked) return "Locked ";
	else return "";
	return "failure";
}
static inline std::string PrintStackForDebug(const std::deque<Value>& stack) {
	std::string result = "start -> [";
	for (auto val = stack.begin(); val < stack.end(); val++) {
		result+= PrintProperty(*val);
		switch (val->type) {
		case ValueType::NOTYPE:           result += "NoType"; break;
		case ValueType::NONE:             result += "None"; break;
		case ValueType::INT:              result += "Int"; break;
		case ValueType::FLOAT:            result += "Float"; break;
		case ValueType::STRING:           result += "String"; break;
		case ValueType::BOOL:             result += "Bool"; break;
		case ValueType::LIST:             result += "List"; break;
		case ValueType::VECTOR:           result += "Vector"; break;
		case ValueType::DICT:             result += "Dict"; break;
		case ValueType::SLICE:            result += "Slice"; break;
		case ValueType::BIGINT:           result += "BigInt"; break;
		case ValueType::PAIRED:           result += "Paired"; break;
		case ValueType::RANGE:            result += "Range"; break;
		case ValueType::TUPLE:            result += "Tuple"; break;
		case ValueType::SET:              result += "Set"; break;
		case ValueType::FUNCTION:         result += "Func"; break;
		case ValueType::NATIVE_FUNCTION:  result += "NFunc"; break;
		case ValueType::FILE:             result += "File"; break;
		case ValueType::OVERLOAD:         result += "Overload"; break;
		case ValueType::OMIT_MARKER:      result += "OmitMarker"; break;
		case ValueType::REFERENCE:        result += "Refrance"; break;
		case ValueType::ERROR:            result += "Error"; break;
		default:                          result += "Unknown"; break;
		}
		if (DEBUGGER_MODE_IS_ENABLED) result+=" "+val->__DEBUGGING__NAME__+ (" " + valueToString(*val));
		if (val != stack.end() - 1) result += ", ";
	}
	return result + "] <- end";
}
static inline bool lessValue(const Value& a, const Value& b) {
	if (a.type != b.type) return a.type < b.type;
	switch (a.type) {
	case ValueType::INT:    return a.asInt() < b.asInt();
	case ValueType::FLOAT:  return a.asFloat() < b.asFloat();
	case ValueType::STRING: return a.asString() < b.asString();
	case ValueType::BOOL:   return a.asBool() < b.asBool();
	case ValueType::LIST: {
		auto* l1 = static_cast<ListObject*>(a.ref.get());
		auto* l2 = static_cast<ListObject*>(b.ref.get());
		return std::lexicographical_compare(
			l1->elements.begin(), l1->elements.end(),
			l2->elements.begin(), l2->elements.end(),
			// Recursive call for elements
			[](const Value& x, const Value& y) { return lessValue(x, y); }
		);
	}
	default:
		return false;
	}
}
struct CacheKeyCmp {
	bool operator()(const vector<Value>& a, const vector<Value>& b) const {
		return std::lexicographical_compare(
			a.begin(), a.end(),
			b.begin(), b.end(),
			[](const Value& x, const Value& y) { return lessValue(x, y); }
		);
	}
};
struct FuncVal { FuncStmt* stmt; std::map<vector<Value>, Value, CacheKeyCmp> cache; FuncVal(FuncStmt* s) :stmt(s) {} };
struct Chunk;
struct FunctionObject : HeapObject {
	vector<ParamSpec> params;
	Chunk* chunk;
	ValueType returnType;
	vector<Expr*> defaultRetArgs;
	bool returnsConst;
	vector<Stmt*> body;
	std::shared_ptr<Env> closure;
	bool isCached;
	std::unordered_map<vector<Value>, Value, VectorHash, VectorEqual> cache;
	FunctionObject(const vector<ParamSpec>& p, ValueType rt, vector<Expr*> dra,bool rc, const vector<Stmt*>& b, std::shared_ptr<Env> c, bool cached, Chunk* ch = nullptr): HeapObject(ValueType::FUNCTION), params(p), returnType(rt), defaultRetArgs(dra) ,returnsConst(rc), body(b), closure(c), isCached(cached), chunk(ch) {}
	~FunctionObject() {
		if (chunk) delete chunk;
	}
};
void setAdd(std::vector<Value>& elems, const Value& v) {
	Value finalVal = deepCopy(v);
	finalVal.isConst = true;
	for (const auto& existing : elems) if (existing.strictEquals(finalVal)) return;
	elems.push_back(finalVal);
}
void enableColors() {
#ifdef _WIN32
	HANDLE hOut= GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	if (GetConsoleMode(hOut, &dwMode)) {
		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(hOut,dwMode);
	}
#endif // _WIN32
}
void printValue(const Value& v, std::unordered_set<const HeapObject*>& seen, bool quoteStrings) {
	bool isContainer = (v.type == ValueType::LIST || v.type == ValueType::SET ||
		v.type == ValueType::VECTOR || v.type == ValueType::DICT);
	if (isContainer && v.ref && seen.count(v.ref.get())) {
		std::cout << "<self>";
		return;
	}
	if (isContainer && v.ref) seen.insert(v.ref.get());
	switch (v.type) {
	case ValueType::INT: if (v.adress) std::cout << v.aspInt(); else std::cout << v.asInt(); break;
	case ValueType::FLOAT: std::cout << v.asFloat(); break;
	case ValueType::BOOL: std::cout << (v.asBool() ? "true" : "false"); break;
	case ValueType::NONE: std::cout << "None"; break;
	case ValueType::NOTYPE: std::cout << "NoType"; break;
	case ValueType::STRING:
		if (quoteStrings) std::cout << "\"" << v.asString() << "\"";
		else std::cout << v.asString();
		break;
	case ValueType::LIST: {
		auto* list = static_cast<ListObject*>(v.ref.get());
		std::cout << "[";
		for (size_t i = 0; i < list->elements.size(); i++) {
			printValue(list->elements[i], seen, true);
			if (i + 1 < list->elements.size()) std::cout << ", ";
		}
		std::cout << "]";
		break;
	}
	case ValueType::RANGE: {
		auto* r = static_cast<RangeObject*>(v.ref.get());
		if (!r->isValid) { std::cout << "(...)"; break; }
		std::cout << (r->startInclusive ? "[" : "(")
			<< (r->isFloat ? std::to_string(r->start) : std::to_string((long long)r->start)) << ".."
			<< (r->isFloat ? std::to_string(r->end) : std::to_string((long long)r->end));
		if (r->step != 1) std::cout << ".." << (r->isFloat ? std::to_string(r->step) : std::to_string((long long)r->step));
		std::cout << (r->endInclusive ? "]" : ")");
		break;
	}
	case ValueType::SET: {
		auto* list = static_cast<SetObject*>(v.ref.get());
		if (list->elements.empty()) {
			std::cout << "{,}";
			break;
		}
		std::cout << "{";
		for (size_t i = 0; i < list->elements.size(); i++) {
			printValue(list->elements[i], seen, true);
			if (i + 1 < list->elements.size()) std::cout << ", ";
		}
		std::cout << "}";
		break;
	}
	case ValueType::TUPLE: {
		auto* t = static_cast<TupleObject*>(v.ref.get());
		if (t->elements.empty()) { std::cout << "(,)"; break; }
		std::cout << "(";
		for (size_t i = 0; i < t->elements.size(); i++) {
			printValue(t->elements[i], seen, true);
			if (i + 1 < t->elements.size()) std::cout << ", ";
			else if (t->elements.size() == 1) std::cout << ",";
		}
		std::cout << ")";
		break;
	}
	case ValueType::DICT: {
		auto* dict = static_cast<DictObject*>(v.ref.get());
		if (dict->items.empty()) { std::cout << "{:}"; break; }
		std::cout << "{";
		size_t i = 0;
		for (const auto& [key, val] : dict->items) {
			printValue(key, seen, true);
			std::cout << " : ";
			printValue(val, seen, true);
			if (i + 1 < dict->items.size()) std::cout << ", ";
			i++;
		}
		std::cout << "}";
		break;
	}
	case ValueType::PAIRED: {
		auto* pair = static_cast<PairedObject*>(v.ref.get());
		std::cout << "<Paired Object at " << &pair << ">";
		break;
	}
	case ValueType::FILE: {
		auto* f = static_cast<FileObject*>(v.ref.get());
		std::cout << "<File at path: '" << f->path << "' >";
		break;
	}
	case ValueType::VECTOR: {
		auto* vec = static_cast<VectorObject*>(v.ref.get());
		std::cout << valueToString(v);
		break;
	}
	case ValueType::BIGINT: {
		auto* big = static_cast<BigIntObject*>(v.ref.get());
		std::cout<< bigIntToString(big);
		break;
	}
	case ValueType::REFERENCE: {
		std::cout<<valueToString(*v.ptr);
		break;
	}
	default: std::cout<<"<Object at "<<v.ref<<">"; break;
	}
}
void printValue(const Value& v) {
	std::unordered_set<const HeapObject*> seen;
	printValue(v, seen, false);
}
struct ValueExpr : public Expr {
	Value val;
	Value* sourcePtr = nullptr;
	ValueExpr(Value v, Value* src = nullptr) : Expr(ExprType::NUMBER), val(v), sourcePtr(src) {}
};
// ------------ AST WALKER -------------
struct Interpreter {
	std::shared_ptr<Env> env;
	unordered_map<string, FuncVal*> funcs;
	using ModuleLoader = std::function<void(std::shared_ptr<Env>, const vector<string>&)>;
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
	void registerStdLib() {
		modules["FileStream"] = [](std::shared_ptr<Env> env, const vector<string>& symbols) {
			auto define = [&](string name, NativeFunc f) {
				if (symbols.empty()) { env->set(name, Value::Native(f), true); return; }
				for (const auto& s : symbols) if (s == name) { env->set(name, Value::Native(f), true); break; }
			};
			define("Open", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Open() expects exactly 1 argument (path)", l, c);
				string path = valueToString(args[0]);
				auto* fObj = new FileObject(path);
				if (!fObj->isOpen) { delete fObj; throw FileNotFoundError("Cannot find or open file: " + path, l, c); }
				Value v; v.type = ValueType::FILE; v.ref = std::shared_ptr<HeapObject>(fObj);
				return v;
			});
			define("SafeOpen", [](const vector<Value>& args, int l, int c) {
				if (args.size() < 1 || args.size() > 2) throw ArgumentError("SafeOpen() expects path and optional failure value", l, c);
				string path = valueToString(args[0]);
				Value failVal = (args.size() == 2) ? args[1] : Value::None();
				auto* fObj = new FileObject(path);
				if (!fObj->isOpen) { delete fObj; return failVal; }
				Value v; v.type = ValueType::FILE; v.ref = std::shared_ptr<HeapObject>(fObj);
				return v;
			});
		};
		modules["Os"] = [](std::shared_ptr<Env> env, const vector<string>& symbols) {
			auto define = [&](string name, NativeFunc f) {
				if (symbols.empty()) { env->set(name, Value::Native(f), true); return; }
				for (const auto& s : symbols) if (s == name) { env->set(name, Value::Native(f), true); break; }
			};
			define("Make", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 2) throw ArgumentError("Make() expects (path, name)", l, c);
				string loc = valueToString(args[0]);
				string name = valueToString(args[1]);
				string fullPath = loc;
				if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\') fullPath += "/";
				fullPath += name;
				std::ofstream outfile(fullPath);
				if (!outfile) throw PermissionError("Cannot create file at: " + fullPath, l, c);
				outfile.close();
				return Value::None();
			});
			define("Remove", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Remove() expects 1 argument (path)", l, c);
				string path = valueToString(args[0]);
				if (std::remove(path.c_str()) != 0) throw PermissionError("Cannot remove: " + path, l, c);
				return Value::None();
			});
			define("Exists", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 2) throw ArgumentError("Exists() expects 2 arguments (path, name)", l, c);
				string loc = valueToString(args[0]);
				string name = valueToString(args[1]);
				string fullPath = loc;
				if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\') fullPath += "/";
				fullPath += name;
				std::ifstream f(fullPath);
				return Value::Bool(f.good());
			});
			define("ListNames", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("ListNames() expects 1 argument (path)", l, c);
				string pathStr = valueToString(args[0]);
				fs::path p(pathStr);
				if (!fs::exists(p)) throw FileNotFoundError("Path not found: " + pathStr, l, c);
				vector<Value> results;
				if (fs::is_directory(p)) {
					for (const auto& entry : fs::directory_iterator(p)) {
						results.push_back(Value::String(entry.path().filename().string()));
					}
				}
				else if (fs::is_regular_file(p)) {
					std::ifstream file(p);
					string line;
					while (std::getline(file, line)) {
						if (!line.empty() && line.back() == '\r') line.pop_back();
						results.push_back(Value::String(line));
					}
				}
				else throw FileNotFoundError("Path is not a valid file or directory: " + pathStr, l, c);
				return Value::List(results);
			});
			define("MkDir", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("MkDir() expects 1 argument (path)", l, c);
				string path = valueToString(args[0]);
				if (!fs::create_directories(path)) {
				}
				return Value::None();
			});
			define("RmDir", [](const vector<Value>& args, int l, int c) {
				if (args.size() < 1 || args.size() > 2) throw ArgumentError("RmDir() expects 2 arguments (path, recursive?)", l, c);
				string path = valueToString(args[0]);
				bool recursive = (args.size() == 2) ? args[1].asBool() : false;
				if (recursive) fs::remove_all(path);
				else fs::remove(path);
				return Value::None();
			});
			define("Cwd", [](const vector<Value>& args, int l, int c) {
				return Value::String(fs::current_path().string());
			});
			define("Cd", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Cd() expects 1 argument (path)", l, c);
				string path = valueToString(args[0]);
				try {
					fs::current_path(path);
				}
				catch (const fs::filesystem_error& e) {
					throw FileNotFoundError("Cannot change directory to: " + path, l, c);
				}
				return Value::None();
			});
			define("Rename", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 2) throw ArgumentError("Rename() expects 2 arguments (old, new)", l, c);
				string oldP = valueToString(args[0]);
				string newP = valueToString(args[1]);
				try {
					fs::rename(oldP, newP);
				}
				catch (...) {
					throw PermissionError("Rename failed", l, c);
				}
				return Value::None();
			});
			define("Env", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Env() expects 1 argument (name)", l, c);
				const char* val = std::getenv(valueToString(args[0]).c_str());
				return val ? Value::String(val) : Value::None();
			});
			define("Console", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Console() expects 1 argument (command)", l, c);
				string cmd = valueToString(args[0]);
				int result = std::system(cmd.c_str());
				return Value::Int(result);
			});
		};
		modules["Time"] = [](std::shared_ptr<Env> env, const vector<string>& symbols) {
			auto define = [&](string name, NativeFunc f) {
				if (symbols.empty()) { env->set(name, Value::Native(f), true); return; }
				for (const auto& s : symbols) if (s == name) { env->set(name, Value::Native(f), true); break; }
			};
			define("Sleep", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Sleep(milliseconds)", l, c);
				long long ms = args[0].asInt();
				std::this_thread::sleep_for(std::chrono::milliseconds(ms));
				return Value::None();
			});
			define("Now", [](const vector<Value>& args, int l, int c) {
				auto now = std::chrono::system_clock::now();
				auto duration = now.time_since_epoch();
				double seconds = std::chrono::duration<double>(duration).count();
				return Value::Float(seconds);
			});
			define("Clock", [](const vector<Value>& args, int l, int c) {
				auto now = std::chrono::high_resolution_clock::now();
				double ms = std::chrono::duration<double, std::milli>(now.time_since_epoch()).count();
				return Value::Float(ms);
			});
			define("Format", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 2) throw ArgumentError("Format(timestamp, formatStr)", l, c);
				time_t rawTime = (time_t)args[0].asFloat();
				string fmt = valueToString(args[1]);
				struct tm* timeInfo = std::localtime(&rawTime);
				char buffer[80];
				std::strftime(buffer, 80, fmt.c_str(), timeInfo);
				return Value::String(string(buffer));
			});
			define("Local", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Local(timestamp)", l, c);
				time_t rawTime = (time_t)args[0].asFloat();
				struct tm* t = std::localtime(&rawTime);
				unordered_map<Value, Value, ValueHash, ValueEqual> parts;
				parts[Value::String("year")] = Value::Int(t->tm_year + 1900);
				parts[Value::String("month")] = Value::Int(t->tm_mon + 1);
				parts[Value::String("day")] = Value::Int(t->tm_mday);
				parts[Value::String("hour")] = Value::Int(t->tm_hour);
				parts[Value::String("min")] = Value::Int(t->tm_min);
				parts[Value::String("sec")] = Value::Int(t->tm_sec);
				return Value::Dict(parts);
				});
		};
		modules["System"] = [](std::shared_ptr<Env> env, const vector<string>& symbols) {
			auto define = [&](string name, NativeFunc f) {
				if (symbols.empty()) { env->set(name, Value::Native(f), true); return; }
				for (const auto& s : symbols) if (s == name) { env->set(name, Value::Native(f), true); break; }
			};
			define("Exit", [](const vector<Value>& args, int l, int c) {
				int code = (args.size() > 0) ? (int)args[0].asInt() : 0;
				exit(code);
				return Value::None(); // Never reached
			});
			define("Platform", [](const vector<Value>& args, int l, int c) {
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
			define("Color", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 2) throw ArgumentError("Color() expects 2 arguments (text, colorName)", l, c);
				string text = valueToString(args[0]);
				string color = valueToString(args[1]);
				string code = "37";
				if (color == "red") code = "31";
				else if (color == "green") code = "32";
				else if (color == "yellow") code = "33";
				else if (color == "blue") code = "34";
				else if (color == "magenta") code = "35";
				else if (color == "cyan") code = "36";
				else if (color == "white") code = "37";
				else if (color == "reset") code = "0";
				else throw ValueError("invalid color", l, c);
				return Value::String("\033[" + code + "m" + text + "\033[0m");
			});
			define("Beep", [](const vector<Value>& args, int l, int c) {
				if (args.size() != 2) throw ArgumentError("Beep() expects 2 arguments (frequency,duration)",l,c);
				if (args[0].asFloat()<=0 || args[1].asFloat()<=0) throw ValueError("Frequency or duration should be positive",l,c);
				Beep(args[0].asFloat(),args[1].asFloat());
				return Value::None();
			});
		};
		modules["Math"] = [](std::shared_ptr<Env> env, const vector<string>& symbols) {
			auto define = [&](string name, NativeFunc f) {
				if (symbols.empty()) { env->set(name, Value::Native(f), true); return; }
				for (const auto& s : symbols) if (s == name) { env->set(name, Value::Native(f), true); break; }
			};
			auto valueToFloat = [&](const Value& v, int l, int c) -> double {
				if (v.type == ValueType::FLOAT) return v.asFloat();
				if (v.type == ValueType::INT) return (double)v.iVal;
				if (v.type == ValueType::BOOL) return v.asBool() ? 1.0 : 0.0;
				if (v.type == ValueType::BIGINT) {
					try { return std::stod(valueToString(v)); }
					catch (...) { return INFINITY; }
				}
				throw TypeError("Expected a number", l, c);
			};
			// Constants
			env->set("PI", Value::Float(3.141592653589793), true, true);
			env->set("E", Value::Float(2.718281828459045), true, true);
			env->set("PHI", Value::Float(1.618033988749894), true, true);
			// Basic Functions
			define("Abs", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Abs() expects 1 argument (num)", l, c);
				const Value& v = args[0];
				if (v.type == ValueType::INT) return Value::Int(std::abs(v.asInt()));
				if (v.type == ValueType::FLOAT) return Value::Float(std::abs(v.asFloat()));
				if (v.type == ValueType::BIGINT) {
					auto* big = static_cast<BigIntObject*>(v.ref.get());
					if (!big->isNegative) return v;
					auto copy = std::make_shared<BigIntObject>(*big);
					copy->isNegative = false;
					return Value::BigInt(copy);
				}
				return Value::Float(std::abs(valueToFloat(v, l, c)));
			});
			define("Floor", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Floor() expects 1 argument (num)", l, c);
				if (args[0].type == ValueType::INT || args[0].type == ValueType::BIGINT) return args[0];
				return Value::Int((long long)std::floor(valueToFloat(args[0], l, c)));
			});
			define("Ceil", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Ceil() expects 1 argument (num)", l, c);
				if (args[0].type == ValueType::INT || args[0].type == ValueType::BIGINT) return args[0];
				return Value::Int((long long)std::ceil(valueToFloat(args[0], l, c)));
			});
			define("Sqrt", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Sqrt() expects 1 argument (num)", l, c);
				double val = valueToFloat(args[0], l, c);
				if (val < 0.0) throw ValueError("argument cannot be negative", l, c);
				return Value::Float(std::sqrt(val));
			});
			define("Cbrt", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Cbrt() expects 1 argument (num)", l, c);
				return Value::Float(std::cbrt(valueToFloat(args[0], l, c)));
			});
			define("Sgn", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Sgn() expects 1 argument (num)", l, c);
				double val = valueToFloat(args[0], l, c);
				if (val > 0.0) return Value::Int(1);
				else if (val < 0.0) return Value::Int(-1);
				else return Value::Int(0);
			});
			define("RadToDeg", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("RadToDeg() expects 1 argument (num)", l, c);
				return Value::Float(valueToFloat(args[0], l, c) * (long double)180 / 3.141592653589793);
			});
			define("DegToRad", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("DegToRad() expects 1 argument (num)", l, c);
				return Value::Float(valueToFloat(args[0], l, c) * 3.141592653589793 / (long double)180);
			});
			// Trig
			define("Sin", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Sin() expects 1 argument (num)", l, c);
				return Value::Float(std::sin(valueToFloat(args[0], l, c)));
			});
			define("Cos", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Cos() expects 1 argument (num)", l, c);
				return Value::Float(std::cos(valueToFloat(args[0], l, c)));
			});
			define("Tan", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Tan() expects 1 argument (num)", l, c);
				return Value::Float(std::tan(valueToFloat(args[0], l, c)));
			});
			define("Sinh", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Sinh() expects 1 argument (num)", l, c);
				return Value::Float(std::sinh(valueToFloat(args[0], l, c)));
			});
			define("Cosh", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Cosh() expects 1 argument (num)", l, c);
				return Value::Float(std::cosh(valueToFloat(args[0], l, c)));
			});
			define("Tanh", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Tanh() expects 1 argument (num)", l, c);
				return Value::Float(std::tanh(valueToFloat(args[0], l, c)));
			});
			// Arc
			define("Arcsin", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Arcsin() expects 1 argument (num)", l, c);
				return Value::Float(std::asin(valueToFloat(args[0], l, c)));
			});
			define("Arccos", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Arccos() expects 1 argument (num)", l, c);
				return Value::Float(std::acos(valueToFloat(args[0], l, c)));
			});
			define("Arctan", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Arctan() expects 1 argument (num)", l, c);
				return Value::Float(std::atan(valueToFloat(args[0], l, c)));
			});
			define("Arcsinh", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Arcsinh() expects 1 argument (num)", l, c);
				return Value::Float(std::asinh(valueToFloat(args[0], l, c)));
			});
			define("Arccosh", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Arccosh() expects 1 argument (num)", l, c);
				return Value::Float(std::acosh(valueToFloat(args[0], l, c)));
			});
			define("Arctanh", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Arctanh() expects 1 argument (num)", l, c);
				return Value::Float(std::atanh(valueToFloat(args[0], l, c)));
			});
			// Log
			define("Log", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Log() expects 1 argument (num)", l, c);
				double val = valueToFloat(args[0], l, c);
				if (val < 0.0) throw ValueError("argument cannot be negative", l, c);
				return Value::Float(std::log(val));
			});
			define("Log2", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Log2() expects 1 argument (num)", l, c);
				double val = valueToFloat(args[0], l, c);
				if (val < 0.0) throw ValueError("argument cannot be negative", l, c);
				return Value::Float(std::log2(val));
			});
			define("Log10", [&](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Log10() expects 1 argument (num)", l, c);
				double val = valueToFloat(args[0], l, c);
				if (val < 0.0) throw ValueError("argument cannot be negative", l, c);
				return Value::Float(std::log10(val));
			});
		};
		modules["Random"] = [](std::shared_ptr<Env> env, const vector<string>& symbols) {
			auto define = [&](string name, NativeFunc f) {
				if (symbols.empty()) { env->set(name, Value::Native(f), true); return; }
				for (const auto& s : symbols) if (s == name) { env->set(name, Value::Native(f), true); break; }
			};
			auto getGen = []() -> std::mt19937& {
				static std::random_device rd;
				static std::mt19937 gen(rd());
				return gen;
			};
			define("RandFloat", [=](const vector<Value>& args, int l, int c) {
				std::uniform_real_distribution<> dis(0.0, 1.0);
				return Value::Float(dis(getGen()));
			});
			define("RandChoice", [=](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("RandChoice() expects 1 argument", l, c);
				Value v = args[0];
				auto pickIndex = [&](size_t size) {
					if (size == 0) throw EmptyContainerError("Cannot choose from empty container", l, c);
					std::uniform_int_distribution<size_t> dis(0, size - 1);
					return dis(getGen());
				};
				if (v.type == ValueType::LIST) {
					auto* list = static_cast<ListObject*>(v.ref.get());
					return list->elements[pickIndex(list->elements.size())];
				}
				if (v.type == ValueType::TUPLE) {
					auto* tuple = static_cast<TupleObject*>(v.ref.get());
					return tuple->elements[pickIndex(tuple->elements.size())];
				}
				if (v.type == ValueType::STRING) {
					const string& str = v.asString();
					char c_char = str[pickIndex(str.size())];
					return Value::String(string(1, c_char));
				}
				if (v.type == ValueType::SET) {
					auto* set = static_cast<SetObject*>(v.ref.get());
					size_t idx = pickIndex(set->elements.size());
					return set->elements[idx];
				}
				if (v.type == ValueType::RANGE) {
					auto* r = static_cast<RangeObject*>(v.ref.get());
					if (!r->isValid) throw EmptyContainerError("Cannot choose from invalid range", l, c);
					double diff = std::abs(r->end - r->start);
					double steps = diff / std::abs(r->step);
					long long count = 0;
					if (r->step > 0 && r->end > r->start) count = (long long)(r->endInclusive ? floor(steps) + 1 : ceil(steps));
					else if (r->step < 0 && r->end < r->start) count = (long long)(r->endInclusive ? floor(steps) + 1 : ceil(steps));
					if (count <= 0) throw EmptyContainerError("Cannot choose from empty range", l, c);
					std::uniform_int_distribution<long long> dis(0, count - 1);
					long long offset = dis(getGen());
					double val = r->start + (offset * r->step);
					return r->isFloat ? Value::Float(val) : Value::Int((long long)val);
				}
				throw TypeError("RandChoice requires a container (list, set, tuple, string, range)", l, c);
			});
			define("Shuffle", [=](const vector<Value>& args, int l, int c) {
				if (args.size() != 1) throw ArgumentError("Shuffle() expects 1 argument", l, c);
				Value v = args[0];
				if (v.type == ValueType::LIST) {
					Value newVal = deepCopy(v);
					auto* list = static_cast<ListObject*>(newVal.ref.get());
					std::shuffle(list->elements.begin(), list->elements.end(), getGen());
					return newVal;
				}
				if (v.type == ValueType::TUPLE) {
					Value newVal = deepCopy(v);
					auto* t = static_cast<TupleObject*>(newVal.ref.get());
					std::shuffle(t->elements.begin(), t->elements.end(), getGen());
					return newVal;
				}
				if (v.type == ValueType::STRING) {
					string s = v.asString();
					std::shuffle(s.begin(), s.end(), getGen());
					return Value::String(s);
				}
				if (v.type == ValueType::SET) {
					Value newVal = deepCopy(v);
					auto* s = static_cast<SetObject*>(newVal.ref.get());
					std::shuffle(s->elements.begin(), s->elements.end(), getGen());
					return newVal;
				}
				if (v.type == ValueType::RANGE) {
					Value listVer = Value::List({});
					throw TypeError("Cannot shuffle a Range (result would not be a range). Use Sample(range, len) to get a shuffled list.", l, c);
				}
				throw TypeError("Shuffle requires a mutable sequence or string", l, c);
			});
			define("Sample", [=](const vector<Value>& args, int l, int c) {
				if (args.size() != 2) throw ArgumentError("Sample() expects 2 arguments (container, count)", l, c);
				Value v = args[0];
				long long k = args[1].asInt();
				if (k < 0) throw ValueError("Sample count cannot be negative", l, c);
				vector<Value> pool;
				bool isString = (v.type == ValueType::STRING);
				if (v.type == ValueType::LIST) pool = static_cast<ListObject*>(v.ref.get())->elements;
				else if (v.type == ValueType::TUPLE) pool = static_cast<TupleObject*>(v.ref.get())->elements;
				else if (v.type == ValueType::SET) pool = static_cast<SetObject*>(v.ref.get())->elements;
				else if (v.type == ValueType::RANGE) {
					auto* r = static_cast<RangeObject*>(v.ref.get());
					double cur = r->start;
					if (!r->startInclusive) cur += r->step;
					while (true) {
						bool cond = (r->step > 0) ? (r->endInclusive ? cur <= r->end : cur < r->end)
							: (r->endInclusive ? cur >= r->end : cur > r->end);
						if (!cond) break;
						pool.push_back(r->isFloat ? Value::Float(cur) : Value::Int((long long)cur));
						cur += r->step;
					}
				}
				else if (isString) {
					string s = v.asString();
					for (char ch : s) pool.push_back(Value::String(string(1, ch)));
				}
				else throw TypeError("Sample requires a container", l, c);
				if (k > (long long)pool.size()) throw ValueError("Sample larger than population", l, c);
				vector<Value> result;
				result.reserve(k);
				std::sample(pool.begin(), pool.end(), std::back_inserter(result), k, getGen());
				std::shuffle(result.begin(), result.end(), getGen());
				vector<Value> deepResult;
				for (auto& val : result) deepResult.push_back(deepCopy(val));

				if (v.type == ValueType::LIST || v.type == ValueType::RANGE) return Value::List(deepResult);
				if (v.type == ValueType::TUPLE) return Value::Tuple(deepResult);
				if (v.type == ValueType::SET) return Value::Set(deepResult);
				if (isString) {
					string s = "";
					for (const auto& val : deepResult) s += val.asString();
					return Value::String(s);
				}
				return Value::None();
			});
		};
		modules["Vector"] = [&](std::shared_ptr<Env> env, const vector<string>& symbols) {
			this->vectorEnabled = true;
			auto vecConstructor = [](const vector<Value>& args, int l, int c) {
				vector<Value> elems;
				if (args.empty()) elems = { Value::Float(0.0), Value::Float(0.0), Value::Float(0.0) };
				else {
					for (const auto& arg : args) {
						if (!arg.isNumber()) throw TypeError("Vector arguments must be numbers", l, c);
						elems.push_back(arg);
					}
				}
				return Value::Vector(elems);
			};
			if (symbols.empty()) env->set("vector", Value::Native(vecConstructor), true);
			else for (const auto& s : symbols) if (s == "vector") env->set("vector", Value::Native(vecConstructor), true);
		};
		// ========= CASTING ==========
		env->set("int", Value::Native([this](const vector<Value>& args, int l, int c) {
			if (args.empty()) return Value::Int(0);
			Value v = args[0];
			if (v.type == ValueType::INT) return v;
			if (v.type == ValueType::BIGINT) return v;
			if (v.type == ValueType::BOOL) return Value::Int(v.asBool() ? 1 : 0);
			if (v.type == ValueType::FLOAT) {
				double d = v.asFloat();
				if (d >= (double)LLONG_MIN && d <= (double)LLONG_MAX) return Value::Int((long long)d);
				std::ostringstream ss;
				ss << std::fixed << std::setprecision(0) << std::abs(d);
				std::string s = ss.str();
				BigIntObject res(0);
				BigIntObject ten(10);
				for (char ch : s) if (ch >= '0' && ch <= '9') res = (res * ten) + BigIntObject(ch - '0');
				res.isNegative = (d < 0);
				return Value::BigInt(std::make_shared<BigIntObject>(res));
			}
			if (v.type == ValueType::STRING) {
				string s = v.asString();
				try {return Value::Int(std::stoll(s));}
				catch (...) {
					bool neg = false;
					if (!s.empty() && s[0] == '-') {neg = true; s = s.substr(1);}
					BigIntObject res(0);
					BigIntObject ten(10);
					for (char ch : s) {
						if (ch >= '0' && ch <= '9') res = (res * ten) + BigIntObject(ch - '0');
						else throw ValueError("Invalid literal for int(): " + v.asString(), l, c);
					}
					res.isNegative = neg;
					return Value::BigInt(std::make_shared<BigIntObject>(res));
				}
			}
			throw TypeError("Cannot cast '" + valueToString(v) + "' to int", l, c);
		}), false);
		env->set("float", Value::Native([this](const vector<Value>& args, int l, int c) {
			if (args.empty()) return Value::Int(0, false);
			Value v = args[0];
			if (v.type == ValueType::FLOAT) return Value::Float(v.asFloat());
			if (v.type == ValueType::INT) return Value::Float(v.asInt());
			if (v.type == ValueType::BOOL) return Value::Float(v.asBool() ? 1.0 : 0.0);
			if (v.type == ValueType::STRING && isdecimal_str(v.asString())) {
				try {
					return Value::Int(std::stof(v.asString()));
				}
				catch (...) {
					throw ValueError("ValueError: String too large for float", l, c);
				}
			}
			throw TypeError("Cannot cast '" + valueToString(v) + "' to float", l, c);
			}), false);
		env->set("bool", Value::Native([this](const vector<Value>& args, int l, int c) {
			if (args.empty()) return Value::Bool(false);
			return Value::Bool(args[0].isTruthy());
		}), false);
		env->set("string", Value::Native([this](const vector<Value>& args, int l, int c) {
			if (args.empty()) return Value::String("");
			Value v = args[0];
			switch (v.type) {
			case ValueType::INT: return Value::String(v.adress ? ptr_to_string(v.adress) : std::to_string(v.asInt()));
			case ValueType::FLOAT: return Value::String(std::to_string(v.asFloat()));
			case ValueType::BOOL: return Value::String(v.asBool() ? "true" : "false");
			case ValueType::STRING: return Value::String(v.asString());
			case ValueType::NONE: return Value::String("None", true);
			case ValueType::RANGE: return Value::String(valueToString(v));
			case ValueType::SET: return Value::String(valueToString(v));
			case ValueType::TUPLE: return Value::String(valueToString(v));
			case ValueType::LIST: return Value::String(valueToString(v));
			case ValueType::BIGINT: return Value::String(valueToString(v));
			default: return Value::String("");
			}
		}), false);
		// ======== CONSTRUCTOR ========
		env->set("range", Value::Native([this](const vector<Value>& args, int l, int c) {
			if (args.empty()) return Value::Range(0, 0, 1, false, false, false);
			if (args.size() == 1 && args[0].type == ValueType::RANGE) return args[0];
			double start = 0, end = 0, step = 1;
			bool isFloat = false;
			auto checkFloat = [&](const Value& v) { if (v.type == ValueType::FLOAT) isFloat = true; };
			if (args.size() == 1) {
				checkFloat(args[0]);
				end = args[0].asFloat();
			}
			else if (args.size() >= 2) {
				checkFloat(args[0]);
				checkFloat(args[1]);
				start = args[0].asFloat();
				end = args[1].asFloat();
			}
			if (args.size() == 3) {
				checkFloat(args[2]);
				step = args[2].asFloat();
			}
			return Value::Range(start, end, step, true, false, isFloat);
		}), false);
		env->set("list", Value::Native([this](const vector<Value>& args, int l, int c) {
			if (!args.empty() && args[0].type == ValueType::RANGE) {
				auto* r = static_cast<RangeObject*>(args[0].ref.get());
				vector<Value> elems;
				double current = r->start;
				if (!r->startInclusive) current += r->step;
				if ((r->step > 0 && r->start > r->end) || (r->step < 0 && r->start < r->end) || r->step == 0) return Value::List({});
				double diff = std::abs(r->end - r->start);
				double steps = diff / std::abs(r->step);
				if (steps > 1000000) throw MemoryError("MemoryError: Range too large to convert to list", l, c);
				while (true) {
					bool cond;
					if (r->step > 0) cond = r->endInclusive ? (current <= r->end) : (current < r->end);
					else cond = r->endInclusive ? (current >= r->end) : (current > r->end);
					if (!cond) break;
					if (r->isFloat) elems.push_back(Value::Float(current));
					else elems.push_back(Value::Int((long long)current));
					current += r->step;
				}
				return Value::List(elems);
			}
			if (!args.empty() && args[0].type == ValueType::SET) {
				auto* s = static_cast<SetObject*>(args[0].ref.get());
				return Value::List(s->elements);
			}
			if (!args.empty() && args[0].type == ValueType::TUPLE) {
				auto* t = static_cast<TupleObject*>(args[0].ref.get());
				return Value::List(t->elements);
			}
			if (!args.empty() && args[0].type == ValueType::VECTOR) {
				auto* v = static_cast<VectorObject*>(args[0].ref.get());
				vector<Value> elems;
				for (auto d : v->elements) elems.push_back(d);
				return Value::List(elems);
			}
			vector<Value> vals;
			for (auto& v : args) vals.push_back(v);
			return Value::List(vals);
		}), false);
		env->set("set", Value::Native([this](const vector<Value>& args, int l, int c) {
			vector<Value> elems;
			if (args.size() > 1) {
				for (auto& arg : args) setAdd(elems, arg);
				return Value::Set(elems);
			}
			if (args.empty()) return Value::Set({});
			Value src = args[0];
			if (src.type == ValueType::LIST) {
				auto* l = static_cast<ListObject*>(src.ref.get());
				for (auto& e : l->elements) setAdd(elems, e);
			}
			else if (src.type == ValueType::RANGE) {
				auto* r = static_cast<RangeObject*>(src.ref.get());
				double current = r->start;
				if (!r->startInclusive) current += r->step;
				while (true) {
					bool cond;
					if (r->step > 0) cond = r->endInclusive ? (current <= r->end) : (current < r->end);
					else cond = r->endInclusive ? (current >= r->end) : (current > r->end);
					if (!cond) break;
					Value v = r->isFloat ? Value::Float(current) : Value::Int((long long)current);
					setAdd(elems, v);
					current += r->step;
				}
			}
			else if (src.type == ValueType::VECTOR) {
				auto* v = static_cast<VectorObject*>(src.ref.get());
				for (auto d : v->elements) setAdd(elems, d);
			}
			else if (src.type == ValueType::STRING) {
				for (char c : src.asString()) setAdd(elems, Value::String(string(1, c)));
			}
			else if (src.type == ValueType::SET) {
				auto* s = static_cast<SetObject*>(src.ref.get());
				for (auto& e : s->elements) setAdd(elems, e);
			}
			else if (src.type == ValueType::TUPLE) {
				auto* t = static_cast<TupleObject*>(src.ref.get());
				for (auto& e : t->elements) setAdd(elems, e);
			}
			else {
				setAdd(elems, src);
			}
			return Value::Set(elems);
		}), false);
		env->set("tuple", Value::Native([this](const vector<Value>& args, int l, int c) {
			if (args.empty()) return Value::Tuple({});
			if (args.size() == 1) {
				Value src = args[0];
				if (src.type == ValueType::LIST) return Value::Tuple(static_cast<ListObject*>(src.ref.get())->elements);
				if (src.type == ValueType::SET) return Value::Tuple(static_cast<SetObject*>(src.ref.get())->elements);
				if (src.type == ValueType::RANGE) {
					auto* r = static_cast<RangeObject*>(src.ref.get());
					vector<Value> elems;
					double current = r->start;
					if (!r->startInclusive) current += r->step;
					while (true) {
						bool cond = (r->step > 0) ? (r->endInclusive ? current <= r->end : current < r->end)
							: (r->endInclusive ? current >= r->end : current > r->end);
						if (!cond) break;
						elems.push_back(r->isFloat ? Value::Float(current) : Value::Int((long long)current));
						current += r->step;
					}
					return Value::Tuple(elems);
				}
				if (src.type == ValueType::TUPLE) return src;
				if (src.type == ValueType::VECTOR) {
					auto* v = static_cast<VectorObject*>(src.ref.get());
					vector<Value> elems;
					for (auto d : v->elements) elems.push_back(d);
					return Value::Tuple(elems);
				}
			}
			vector<Value> elems;
			for (auto& arg : args) elems.push_back(arg);
			return Value::Tuple(elems);
		}), false);
		env->set("dict", Value::Native([this](const vector<Value>& args, int l, int c) {
			std::unordered_map<Value, Value, ValueHash, ValueEqual> map;
			for (const auto& arg : args) {
				Value v = arg;
				if (v.type != ValueType::PAIRED) {
					throw TypeError("dict() requires 'key : value' arguments or 'pair()' objects", l, c);
				}
				auto* pObj = static_cast<PairedObject*>(v.ref.get());
				for (const auto& pair : pObj->pairs) {
					Value key = pair.first;
					Value val = pair.second;
					if (key.type == ValueType::LIST || key.type == ValueType::SET || key.type == ValueType::DICT) {
						if (key.type == ValueType::DICT) throw TypeError("Dictionary cannot be used as a key (unhashable)", l, c);
						key = deepCopy(key);
						key.isConst = true;
					}
					map[key] = val;
				}
			}
			return Value::Dict(map);
		}), false);
		// ======= INTROSPECTION =======
		env->set("typeof", Value::Native([this](const vector<Value>& args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("typeof() takes exactly one argument", l, c);
			switch (args[0].type) {
			case ValueType::INT: return Value::String("integer");
			case ValueType::FLOAT: return Value::String("float");
			case ValueType::BOOL: return Value::String("boolean");
			case ValueType::STRING: return Value::String("string");
			case ValueType::LIST: return Value::String("list");
			case ValueType::RANGE: return Value::String("range");
			case ValueType::SET: return Value::String("set");
			case ValueType::TUPLE: return Value::String("tuple");
			case ValueType::DICT: return Value::String("dictionary");
			case ValueType::FUNCTION: return Value::String("function");
			case ValueType::VECTOR: return Value::String("vector");
			case ValueType::NATIVE_FUNCTION: return Value::String("native function");
			case ValueType::FILE: return Value::String("file");
			case ValueType::PAIRED: return Value::String("pair");
			case ValueType::BIGINT: return Value::String("integer");
			case ValueType::NONE: return Value::String("None");
			default: return Value::String("NoType");
			}
		}), false);
		env->set("isLocked", Value::Native([this](const vector<Value>& args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("isLocked() takes exactly one argument", l, c);
			return Value::Bool(args[0].isLocked);
		}), false);
		env->set("isConst", Value::Native([this](const vector<Value>& args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("isConst() takes exactly one argument", l, c);
			return Value::Bool(args[0].isConst);
		}), false);
		env->set("length", Value::Native([this](const vector<Value>& args, int l, int c) {
			if (args.empty()) return Value::Int(0);
			if (args.size() > 1) throw ArgumentError("length() takes exactly one argument", l, c);
			Value v = args[0];
			if (v.type == ValueType::LIST) {
				auto* list = static_cast<ListObject*>(v.ref.get());
				return Value::Int(list->elements.size());
			}
			else if (v.type == ValueType::STRING) return Value::Int(v.asString().size());
			else if (v.type == ValueType::SET) {
				auto* s = static_cast<SetObject*>(v.ref.get());
				return Value::Int(s->elements.size());
			}
			else if (v.type == ValueType::TUPLE) {
				auto* t = static_cast<TupleObject*>(v.ref.get());
				return Value::Int(t->elements.size());
			}
			else if (v.type == ValueType::RANGE) {
				auto* r = static_cast<RangeObject*>(v.ref.get());
				if (r->step == 0) return Value::Int(0);
				double s = r->start;
				if (!r->startInclusive) s += r->step;
				double e = r->end;
				if (r->endInclusive) e += (r->step > 0 ? 1 : -1) * (r->step * 0.000000001);
				long long count = 0;
				if (r->step > 0 && r->end > s) {
					if (r->endInclusive) count = (long long)floor((r->end - s) / r->step) + 1;
					else count = (long long)ceil((r->end - s) / r->step);
				}
				else if (r->step < 0 && r->end < s) {
					if (r->endInclusive) count = (long long)floor((s - r->end) / -r->step) + 1;
					else count = (long long)ceil((s - r->end) / -r->step);
				}
				return Value::Int(count < 0 ? 0 : count);
			}
			else {
				throw TypeError("Object of type " + valueToString(v) + " has no length", l, c);
			}
		}), false);
		env->set("sum", Value::Native([this](const vector<Value>& args, int l, int c) {
			long double total = 0;
			bool isFloat = false;
			vector<Value> worklist;
			worklist.reserve(args.size() * 2);
			for (const auto& arg : args) worklist.push_back(arg);
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
					auto* list = static_cast<ListObject*>(v.ref.get());
					worklist.insert(worklist.end(), list->elements.begin(), list->elements.end());
					break;
				}
				case ValueType::TUPLE: {
					auto* t = static_cast<TupleObject*>(v.ref.get());
					worklist.insert(worklist.end(), t->elements.begin(), t->elements.end());
					break;
				}
				case ValueType::SET: {
					auto* s = static_cast<SetObject*>(v.ref.get());
					worklist.insert(worklist.end(), s->elements.begin(), s->elements.end());
					break;
				}
				case ValueType::RANGE: {
					auto* r = static_cast<RangeObject*>(v.ref.get());
					if (r->isFloat) isFloat = true;
					double current = r->start;
					if (!r->startInclusive) current += r->step;
					while (true) {
						bool cond = (r->step > 0) ?
							(r->endInclusive ? current <= r->end : current < r->end) :
							(r->endInclusive ? current >= r->end : current > r->end);
						if (!cond) break;
						total += current;
						current += r->step;
					}
					break;
				}
				case ValueType::BOOL:
					total += v.asBool() ? 1 : 0;
					break;
				default:
					throw TypeError("sum() encountered non-numeric type: " + valueToString(v), l, c);
				}
			}
			return isFloat ? Value::Float(total) : Value::Int((long long)total);
		}), false);
		env->set("pair", Value::Native([this](const vector<Value>& args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("pair() takes exactly two arguments (keys, values)", l, c);
			auto extract = [&](Value v) -> vector<Value> {
				if (v.type == ValueType::LIST) return static_cast<ListObject*>(v.ref.get())->elements;
				if (v.type == ValueType::SET) return static_cast<SetObject*>(v.ref.get())->elements;
				if (v.type == ValueType::TUPLE) return static_cast<TupleObject*>(v.ref.get())->elements;
				if (v.type == ValueType::RANGE) {
					auto* r = static_cast<RangeObject*>(v.ref.get());
					vector<Value> rvals;
					double current = r->start;
					if (!r->startInclusive) current += r->step;
					while (true) {
						bool cond = (r->step > 0) ? (r->endInclusive ? current <= r->end : current < r->end)
							: (r->endInclusive ? current >= r->end : current > r->end);
						if (!cond) break;
						rvals.push_back(r->isFloat ? Value::Float(current) : Value::Int((long long)current));
						current += r->step;
					}
					return rvals;
				}
				throw TypeError("pair() arguments must be containers (list, set, tuple, range)", l, c);
				return {};
			};
			vector<Value> rawKeys = extract(args[0]);
			vector<Value> vals = extract(args[1]);
			vector<Value> uniqueKeys;
			for (const auto& k : rawKeys) {
				bool seen = false;
				for (const auto& u : uniqueKeys) if (k.strictEquals(u)) { seen = true; break; }
				if (!seen) uniqueKeys.push_back(k);
			}
			size_t count = std::min(uniqueKeys.size(), vals.size());
			std::vector<std::pair<Value, Value>> finalPairs;
			for (size_t i = 0; i < count; i++) {
				finalPairs.push_back({ uniqueKeys[i], vals[i] });
			}
			return Value::Paired(finalPairs);
		}), false);
		// ============ I/O ============
		env->set("print", Value::Native([this](const vector<Value>& args, int l, int c) {
			for (auto& v : args) {
				printValue(v);
				std::cout << " ";
			}
			std::cout << "\n";
			return Value::None();
		}), false);
		env->set("input", Value::Native([this](const vector<Value>& args, int l, int c) {
			string prompt = "";
			if (!args.empty()) prompt = valueToString(args[0]);
			std::cout << prompt;
			if (std::cin.fail()) std::cin.clear();
			if (std::cin.peek() == '\n') std::cin.ignore();
			string line;
			if (!std::getline(std::cin, line)) return Value::None();
			if (args.size() > 1) {
				ValueType targetType = args[1].type;
				try {
					if (targetType == ValueType::INT) return Value::Int(std::stoll(line));
					else if (targetType == ValueType::FLOAT) return Value::Float(std::stod(line));
					else if (targetType == ValueType::BOOL) return Value::Bool(line == "true");
					else if (targetType == ValueType::LIST || targetType == ValueType::SET || targetType == ValueType::RANGE) {
						throw TypeError("Complex type input not fully supported yet", l, c);
					}
				}
				catch (const LangError&) { throw; }
				catch (...) {
					throw ValueError("ValueError: Could not convert input '" + line + "' to target type", l, c);
				}
			}
			return Value::String(line);
		}), false);
	}
	LValue resolveLValue(Expr* e) {
		if (ValueExpr* ve = dynamic_cast<ValueExpr*>(e)) {
			if (ve->sourcePtr) return LValue(ve->sourcePtr, true);
			return LValue(&ve->val, false);
		}
		if (auto v = dynamic_cast<VarExpr*>(e)) {
			if (!env->exists(v->name)) throw NameError("Undefined variable '" + v->name + "'", e->line, e->col);
			Var& var = env->lookup(v->name);
			if (var.alias) return LValue(var.alias, true, var.isConst, var.isLocked);
			return LValue(&var.value, false, var.isConst, var.isLocked);
		}
		if (auto idx = dynamic_cast<IndexExpr*>(e)) {
			LValue base = resolveLValue(idx->base);
			Value& container = *base.ref;
			int i = eval(idx->index).asInt();
			if (container.type == ValueType::LIST) {
				auto* listObj = static_cast<ListObject*>(container.ref.get());
				if (i < 0 || i >(int)listObj->elements.size()) throw IndexError("List index out of range", e->line, e->col);
				if (i == (int)listObj->elements.size()) listObj->elements.push_back(Value::None());

				return LValue(&listObj->elements[i], true, base.isConstView || container.isConst);
			}
			if (container.type == ValueType::STRING) return LValue(&container, true);

			throw TypeError("Type is not indexable or mutable", e->line, e->col);
		}
		throw TypeError("Expression is not assignable", e->line, e->col);
	}
	Value Resolve_methods(MethodCallExpr* m) {
		auto error = [&](const string& msg, const string& type = "RuntimeError") {
			if (type == "TypeError") throw TypeError(msg, m->line, m->col);
			if (type == "ValueError") throw ValueError(msg, m->line, m->col);
			if (type == "IndexError") throw IndexError(msg, m->line, m->col);
			if (type == "ConstError") throw ConstError(msg, m->line, m->col);
			if (type == "KeyError") throw KeyError(msg, m->line, m->col);
			if (type == "ArgumentError") throw ArgumentError(msg, m->line, m->col);
			if (type == "EmptyContainerError") throw EmptyContainerError(msg, m->line, m->col);
			if (type == "AttributeError") throw AttributeError(msg, m->line, m->col);
			throw RuntimeError(msg, m->line, m->col);
		};
		Value* targetPtr = nullptr;
		Value tempVal;
		bool isConstView = false;
		if (dynamic_cast<VarExpr*>(m->object) || dynamic_cast<IndexExpr*>(m->object)) {
			LValue lv = resolveLValue(m->object);
			targetPtr = lv.ref;
			isConstView = lv.isConstView;
		}
		else {
			tempVal = eval(m->object);
			targetPtr = &tempVal;
			isConstView = tempVal.isConst;
		}
		Value& target = *targetPtr;
		auto checkConst = [&]() {
			if (target.isConst || isConstView) {
				error("Cannot call mutating method '" + m->method + "' on const object", "ConstError");
			}
		};
		if (m->method == "adress") {
			if (!m->args.empty()) error("adress() takes no arguments", "ArgumentError");
			if (target.ref) {
				return Value::pInt(target.ref.get());
			}
			if (dynamic_cast<VarExpr*>(m->object) || dynamic_cast<IndexExpr*>(m->object)) {
				return Value::pInt(targetPtr);
			}
			return Value::pInt(nullptr);
		}
		if (m->method == "base") {
			if (m->args.size() != 1) error("base() takes exactly one argument", "ArgumentError");
			int base = eval(m->args[0]).asInt();
			if (base < 2 || base > 36) error("base() target must be between 2 and 36", "ValueError");
			if (target.type == ValueType::INT || target.type == ValueType::FLOAT) {
				string res = "";
				bool isNeg = false;
				double val = target.asFloat();
				if (val < 0) { isNeg = true; val = -val; }
				long long intPart = (long long)val;
				double fracPart = val - intPart;
				if (intPart == 0) res = "0";
				else {
					while (intPart > 0) {
						int rem = intPart % base;
						res += (rem < 10 ? '0' + rem : 'A' + (rem - 10));
						intPart /= base;
					}
				}
				if (isNeg) res += '-';
				std::reverse(res.begin(), res.end());
				if (fracPart > 0) {
					res += '.';
					int precision = 8;
					while (fracPart > 0 && precision-- > 0) {
						fracPart *= base;
						int digit = (int)fracPart;
						res += (digit < 10 ? '0' + digit : 'A' + (digit - 10));
						fracPart -= digit;
						if (fracPart < 1e-9) break;
					}
				}
				return Value::String(res);
			}
			error("base() requires an int or float", "TypeError");
		}
		// ---------------- REVERSE ---------------
		if (m->method == "reverse") {
			checkConst();
			if (!m->args.empty()) error("reverse() does not accept arguments", "ArgumentError");
			if (target.type == ValueType::STRING) {
				auto* str = static_cast<StringObject*>(target.ref.get());
				if (str->value.size() < 2) return target;
				std::reverse(str->value.begin(), str->value.end());
				return target;
			}
			if (target.type == ValueType::LIST) {
				auto* list = static_cast<ListObject*>(target.ref.get());
				if (list->elements.size() < 2) return target;
				std::reverse(list->elements.begin(), list->elements.end());
				return target;
			}
			if (target.type == ValueType::RANGE) {
				auto* rang = static_cast<RangeObject*>(target.ref.get());
				std::swap(rang->start, rang->end);
				std::swap(rang->startInclusive, rang->endInclusive);
				rang->step = -rang->step;
				return target;
			}
			if (target.type == ValueType::SET) {
				auto* set = static_cast<SetObject*>(target.ref.get());
				if (set->elements.size() < 2) return target;
				reverse(set->elements.begin(),set->elements.end());
				return target;
			}
			error("reverse() not supported on this type", "TypeError");
		}
		// ---------------- APPEND ----------------
		if (m->method == "append") {
			checkConst();
			if (m->args.size() != 1) error("append() takes exactly one argument", "ArgumentError");
			Value v = eval(m->args[0]);
			if (target.type == ValueType::LIST) {
				auto* list = static_cast<ListObject*>(target.ref.get());
				list->elements.push_back(v);
				return target;
			}
			if (target.type == ValueType::STRING) {
				auto* str = static_cast<StringObject*>(target.ref.get());
				str->value += valueToString(v);
				return target;
			}
			error("append() not supported on this type", "TypeError");
		}
		// ---------------- SORT ----------------
		if (m->method == "sort") {
			checkConst();
			if (m->args.size() > 1) error("sort() takes zero or one argument", "ArgumentError");

			bool reverseSort = false;
			if (!m->args.empty()) reverseSort = eval(m->args[0]).asBool();

			if (target.type == ValueType::STRING) {
				auto* str = static_cast<StringObject*>(target.ref.get());
				if (str->value.size() <= 1) return Value::String(str->value);
				std::sort(str->value.begin(), str->value.end());
				if (reverseSort) std::reverse(str->value.begin(), str->value.end());
				return target;
			}
			if (target.type == ValueType::LIST) {
				auto* list = static_cast<ListObject*>(target.ref.get());
				auto& elems = list->elements;
				if (elems.size() <= 1) return Value::List(list->elements);

				ValueType t = elems[0].type;
				for (size_t i = 1; i < elems.size(); ++i) {
					if (elems[i].type != t) error("cannot sort list with mixed types", "TypeError");
				}
				if (t != ValueType::INT && t != ValueType::FLOAT &&
					t != ValueType::STRING && t != ValueType::BOOL && t != ValueType::LIST) {
					error("unsupported type in sort()", "TypeError");
				}
				std::sort(elems.begin(), elems.end(), [](const Value& a, const Value& b) { return lessValue(a, b); });
				if (reverseSort) std::reverse(elems.begin(), elems.end());
				return target;
			}
			error("sort() only works on mutable types", "TypeError");
		}
		// ---------------- FUNCTIONAL METHODS ----------------
		if (m->method == "All_Of" || m->method == "Any_Of" || m->method == "None_Of" || m->method == "One_Of" ||
			m->method == "find" || m->method == "select" || m->method == "reject" ||
			m->method == "partition" || m->method == "map") {
			if (m->args.size() != 1) error(m->method + "() expects exactly one argument (a lambda function)", "ArgumentError");
			Value lambda = eval(m->args[0]);
			if (lambda.type != ValueType::FUNCTION) error(m->method + "() argument must be a function or lambda", "TypeError");
			vector<Value> elements;
			if (target.type == ValueType::LIST) elements = static_cast<ListObject*>(target.ref.get())->elements;
			else if (target.type == ValueType::SET) elements = static_cast<SetObject*>(target.ref.get())->elements;
			else if (target.type == ValueType::TUPLE) elements = static_cast<TupleObject*>(target.ref.get())->elements;
			else if (target.type == ValueType::VECTOR) {
				for (auto d : static_cast<VectorObject*>(target.ref.get())->elements) elements.push_back(d);
			}
			else if (target.type == ValueType::DICT) {
				auto* d = static_cast<DictObject*>(target.ref.get());
				for (auto& pair : d->items) elements.push_back(pair.first); // Iterate keys
			}
			else if (target.type == ValueType::STRING) {
				string& s = static_cast<StringObject*>(target.ref.get())->value;
				for (char c : s) elements.push_back(Value::String(string(1, c)));
			}
			else error("Method '" + m->method + "' requires a container (List, Set, Tuple, Vector, Dict, or String)", "TypeError");
			vector<Value> resultsTrue;
			vector<Value> resultsFalse;
			vector<Value> resultsMap;
			int matchCount = 0;
			for (const auto& elem : elements) {
				vector<CallArg> args;
				CallArg arg;
				arg.value = elem;
				arg.hasLValue = false;
				args.push_back(arg);
				Value ret = call(lambda, args, {}, m->line, m->col);
				bool matches = ret.isTruthy();
				if (m->method == "All_Of") {
					if (!matches) return Value::Bool(false);
				}
				else if (m->method == "Any_Of") {
					if (matches) return Value::Bool(true);
				}
				else if (m->method == "None_Of") {
					if (matches) return Value::Bool(false);
				}
				else if (m->method == "One_Of") {
					if (matches) {
						matchCount++;
						if (matchCount > 1) return Value::Bool(false);
					}
				}
				else if (m->method == "find") {
					if (matches) return elem;
				}
				else if (m->method == "select") {
					if (matches) resultsTrue.push_back(elem);
				}
				else if (m->method == "reject") {
					if (!matches) resultsTrue.push_back(elem);
				}
				else if (m->method == "partition") {
					if (matches) resultsTrue.push_back(elem);
					else resultsFalse.push_back(elem);
				}
				else if (m->method == "map") {
					resultsMap.push_back(ret);
				}
			}
			if (m->method == "All_Of") return Value::Bool(true);
			if (m->method == "Any_Of") return Value::Bool(false);
			if (m->method == "None_Of") return Value::Bool(true);
			if (m->method == "One_Of") return Value::Bool(matchCount == 1);
			if (m->method == "find") return Value::None();
			auto reconstruct = [&](const vector<Value>& src) -> Value {
				if (target.type == ValueType::LIST || target.type == ValueType::DICT) return Value::List(src);
				if (target.type == ValueType::TUPLE) return Value::Tuple(src);
				if (target.type == ValueType::SET) return Value::Set(src);
				if (target.type == ValueType::VECTOR) {
					vector<Value> nums;
					bool allNums = true;
					for (auto& v : src) {
						if (!v.isNumber()) { allNums = false; break; }
						nums.push_back(v);
					}
					if (allNums) return Value::Vector(nums);
					return Value::List(src);
				}
				if (target.type == ValueType::STRING) {
					string s = "";
					bool allStr = true;
					for (auto& v : src) {
						if (v.type != ValueType::STRING) { allStr = false; break; }
						s += v.asString();
					}
					if (allStr) return Value::String(s);
					return Value::List(src);
				}
				return Value::List(src);
				};
			if (m->method == "select" || m->method == "reject") return reconstruct(resultsTrue);
			if (m->method == "partition") return Value::List({ reconstruct(resultsTrue), reconstruct(resultsFalse) });
			if (m->method == "map") {
				if (target.type == ValueType::DICT) return Value::List(resultsMap);
				return reconstruct(resultsMap);
			}
		}
		// ------- STRING TRANSFORMATIONS -------
		if (target.type == ValueType::STRING) {
			if (m->method == "ascii") {
				if (!m->args.empty()) error("ascii() does not accept arguments", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				if (str->value.empty()) return target;
				if (str->value.size() != 1) error("string size cannot exceed 1", "ArgumentError");
				return Value::Int(static_cast<int>(str->value[0]));
			}
			if (m->method == "capitalize") {
				checkConst();
				if (!m->args.empty()) error("capitalize() does not accept arguments", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				str->value = capitalize(str->value);
				return target;
			}
			if (m->method == "chars") {
				auto* str = static_cast<StringObject*>(target.ref.get());
				int start = 0;
				int end = -1;
				if (m->args.size() >= 1) start = eval(m->args[0]).asInt();
				if (m->args.size() >= 2) end = eval(m->args[1]).asInt();
				std::vector<std::string> parts = chars(str->value, start, end);
				std::vector<Value> valList;
				valList.reserve(parts.size());
				for (const auto& p : parts) valList.push_back(Value::String(p));
				return Value::List(valList);
			}
			if (m->method == "casefold") {
				checkConst();
				if (!m->args.empty()) error("casefold() does not accept arguments", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				str->value = casefold(str->value);
				return target;
			}
			if (m->method == "center") {
				checkConst();
				if (m->args.empty() || m->args.size() > 2) error("center() needs 1 or 2 arguments", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				if (m->args.size() == 1) {
					str->value = center(str->value, eval(m->args[0]).asInt());
					return target;
				}
				if (m->args.size() == 2) {
					string a = eval(m->args[1]).asString();
					if (a.empty() || a.size() > 1) error("padding can only be one character", "ValueError");
					str->value = center(str->value, eval(m->args[0]).asInt(), a[0]);
					return target;
				}
			}
			if (m->method == "count") {
				if (m->args.empty() || m->args.size() > 3) error("count() needs 1 to 3 arguments", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				switch (m->args.size()) {
				case 1:
					return Value::Int(::count(str->value, eval(m->args[0]).asString()));
				case 2:
					if (eval(m->args[1]).asInt() < 0) error("starting position cannot be negative", "ValueError");
					return Value::Int(::count(str->value, eval(m->args[0]).asString(), eval(m->args[1]).asInt()));
				case 3:
					if (eval(m->args[1]).asInt() < 0) error("starting position cannot be negative", "ValueError");
					return Value::Int(::count(str->value, eval(m->args[0]).asString(), eval(m->args[1]).asInt(), eval(m->args[2]).asInt()));
				}
			}
			if (m->method == "endswith") {
				if (m->args.empty() || m->args.size() > 1) error("endswith() needs one argument", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				return Value::Bool(endswith(str->value, eval(m->args[0]).asString()));
			}
			if (m->method == "index") {
				if (m->args.empty() || m->args.size() > 1) error("index() needs one argument", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				return Value::Int(index(str->value, eval(m->args[0]).asString()));
			}
			if (m->method == "isalnum") {
				if (!m->args.empty()) error("isalnum() does not accept arguments", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				return Value::Bool(isalnum_str(str->value));
			}
			if (m->method == "isalpha") {
				if (!m->args.empty()) error("isalpha() does not accept arguments", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				return Value::Bool(isalpha_str(str->value));
			}
			if (m->method == "isdecimal") {
				if (!m->args.empty()) error("isdecimal() does not accept arguments", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				return Value::Bool(isdecimal_str(str->value));
			}
			if (m->method == "islower") {
				if (!m->args.empty()) error("islower() does not accept arguments", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				return Value::Bool(islower_str(str->value));
			}
			if (m->method == "isupper") {
				if (!m->args.empty()) error("isupper() does not accept arguments", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				return Value::Bool(isupper_str(str->value));
			}
			if (m->method == "ljust") {
				checkConst();
				if (m->args.empty() || m->args.size() > 2) error("ljust() needs 1 or 2 arguments", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				if (m->args.size() == 1)
					str->value = ljust(str->value, eval(m->args[0]).asInt());
				else {
					string p = eval(m->args[1]).asString();
					if (p.size() != 1) error("padding can only be one character", "ValueError");
					str->value = ljust(str->value, eval(m->args[0]).asInt(), p[0]);
				}
				return target;
			}
			if (m->method == "lower") {
				checkConst();
				if (!m->args.empty()) error("lower() does not accept arguments", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				str->value = lower(str->value);
				return target;
			}
			if (m->method == "lstrip") {
				checkConst();
				if (m->args.size() > 1) error("lstrip() takes at most one argument", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				str->value = lstrip(str->value, m->args.size() == 1 ? eval(m->args[0]).asString() : " \t\n\r\v\f");
				return target;
			}
			if (m->method == "rstrip") {
				checkConst();
				if (m->args.size() > 1) error("rstrip() takes at most one argument", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				str->value = rstrip(str->value, m->args.size() == 1 ? eval(m->args[0]).asString() : " \t\n\r\v\f");
				return target;
			}
			if (m->method == "strip") {
				checkConst();
				if (m->args.size() > 1) error("strip() takes at most one argument", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				str->value = strip(str->value, m->args.size() == 1 ? eval(m->args[0]).asString() : " \t\n\r\v\f");
				return target;
			}
			if (m->method == "split") {
				string delimiter = " ";
				if (!m->args.empty()) {
					if (m->args.size() > 1) error("split() expects at most 1 argument", "ArgumentError");
					Value delimVal = eval(m->args[0]);
					if (delimVal.type != ValueType::STRING) error("split() delimiter must be a string", "TypeError");
					delimiter = delimVal.asString();
				}
				vector<string> parts = split(target.asString(), delimiter);
				vector<Value> resultList;
				resultList.reserve(parts.size());
				for (const auto& part : parts)  resultList.push_back(Value::String(part));
				return Value::List(resultList);
			}
			if (m->method == "upper") {
				checkConst();
				if (!m->args.empty()) error("upper() does not accept arguments", "ArgumentError");
				auto* str = static_cast<StringObject*>(target.ref.get());
				str->value = upper(str->value);
				return target;
			}
			error("Object '" + m->method + "' is not a string method", "AttributeError");
		}
		// ---------- RANGE METHODS ----------
		if (target.type == ValueType::RANGE) {
			if (m->method == "min") {
				if (!m->args.empty()) error("min() does not accept arguments", "ArgumentError");
				auto* rang = static_cast<RangeObject*>(target.ref.get());
				if (!rang->isValid) error("min() arg is an empty range", "ValueError");
				return Value::Int(rang->step < 0 ? (rang->endInclusive ? rang->end : rang->end - rang->step) : (rang->startInclusive ? rang->start : rang->start + rang->step));
			}
			if (m->method == "max") {
				if (!m->args.empty()) error("max() does not accept arguments", "ArgumentError");
				auto* rang = static_cast<RangeObject*>(target.ref.get());
				if (!rang->isValid) error("max() arg is an empty range", "ValueError");

				return Value::Int(rang->step < 0 ? (rang->startInclusive ? rang->start : rang->start + rang->step) : (rang->endInclusive ? rang->end : rang->end - rang->step));
			}
			if (m->method == "step") {
				if (!m->args.empty()) error("step() does not accept arguments", "ArgumentError");
				auto* rang = static_cast<RangeObject*>(target.ref.get());
				if (!rang->isValid) error("step() arg is an empty range", "ValueError");
				return Value::Int(rang->step);
			}
			error("Object '" + m->method + "' is not a range method", "AttributeError");
		}
		// ---------------- LIST METHODS ----------------
		if (target.type == ValueType::LIST) {
			auto* listObj = static_cast<ListObject*>(target.ref.get());
			auto& elems = listObj->elements;
			if (m->method == "count") {
				if (m->args.size() != 1) error("list.count() takes exactly one argument", "ArgumentError");
				Value needle = eval(m->args[0]);
				long long c = 0;
				for (const auto& el : elems) {
					if (el.strictEquals(needle)) c++;
				}
				return Value::Int(c);
			}
			if (m->method == "index") {
				if (m->args.size() != 1) error("list.index() takes exactly one argument", "ArgumentError");
				Value needle = eval(m->args[0]);
				for (size_t i = 0; i < elems.size(); i++) {
					if (elems[i].strictEquals(needle)) return Value::Int((long long)i);
				}
				return Value::Int(-1);
			}
			if (m->method == "insert") {
				checkConst();
				if (m->args.size() != 2) error("insert() takes exactly two arguments (index, value)", "ArgumentError");
				int idx = eval(m->args[0]).asInt();
				Value val = eval(m->args[1]);
				if (idx < 0 || idx >(int)elems.size()) error("Index out of bounds", "IndexError");
				elems.insert(elems.begin() + idx, val);
				return target;
			}
			if (m->method == "pop") {
				checkConst();
				if (m->args.size() > 1) error("pop() takes at most one argument", "ArgumentError");
				if (elems.empty()) error("pop from empty list", "EmptyContainerError");
				int idx = elems.size() - 1;
				if (!m->args.empty()) idx = eval(m->args[0]).asInt();
				if (idx < 0 || idx >= (int)elems.size()) error("pop index out of bounds", "IndexError");
				Value val = elems[idx];
				elems.erase(elems.begin() + idx);
				return val;
			}
			if (m->method == "remove") {
				checkConst();
				if (m->args.size() != 1) error("remove() takes exactly one argument", "ArgumentError");
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
				if (m->args.size() != 1) error("extend() takes exactly one argument", "ArgumentError");
				Value other = eval(m->args[0]);
				if (other.type == ValueType::LIST) {
					auto* o = static_cast<ListObject*>(other.ref.get());
					elems.insert(elems.end(), o->elements.begin(), o->elements.end());
				}
				else if (other.type == ValueType::SET) {
					auto* o = static_cast<SetObject*>(other.ref.get());
					elems.insert(elems.end(), o->elements.begin(), o->elements.end());
				}
				else if (other.type == ValueType::RANGE) {
					auto* r = static_cast<RangeObject*>(other.ref.get());
					double current = r->start;
					if (!r->startInclusive) current += r->step;
					while (true) {
						bool cond = (r->step > 0) ? (r->endInclusive ? current <= r->end : current < r->end)
							: (r->endInclusive ? current >= r->end : current > r->end);
						if (!cond) break;
						elems.push_back(r->isFloat ? Value::Float(current) : Value::Int((long long)current));
						current += r->step;
					}
				}
				else error("extend() requires an iterable (list, set, or range)", "TypeError");
				return target;
			}
			if (m->method == "sum") {
				double total = 0;
				bool isFloat = false;
				for (const auto& el : elems) {
					if (el.type == ValueType::INT) total += el.asInt();
					else if (el.type == ValueType::FLOAT) { total += el.asFloat(); isFloat = true; }
					else error("sum() requires numeric values", "TypeError");
				}
				return isFloat ? Value::Float(total) : Value::Int((long long)total);
			}
			if (m->method == "min") {
				if (elems.empty()) error("min() on empty list", "ValueError");
				Value minVal = elems[0];
				for (size_t i = 1; i < elems.size(); i++) {
					bool smaller = false;
					if (elems[i].type == ValueType::INT && minVal.type == ValueType::INT)
						smaller = elems[i].asInt() < minVal.asInt();
					else if (elems[i].type == ValueType::FLOAT || minVal.type == ValueType::FLOAT)
						smaller = elems[i].asFloat() < minVal.asFloat();
					else if (elems[i].type == ValueType::STRING && minVal.type == ValueType::STRING)
						smaller = elems[i].asString() < minVal.asString();
					if (smaller) minVal = elems[i];
				}
				return minVal;
			}
			if (m->method == "max") {
				if (elems.empty()) error("max() on empty list", "ValueError");
				Value maxVal = elems[0];
				for (size_t i = 1; i < elems.size(); i++) {
					bool larger = false;
					if (elems[i].type == ValueType::INT && maxVal.type == ValueType::INT)
						larger = elems[i].asInt() > maxVal.asInt();
					else if (elems[i].type == ValueType::FLOAT || maxVal.type == ValueType::FLOAT)
						larger = elems[i].asFloat() > maxVal.asFloat();
					else if (elems[i].type == ValueType::STRING && maxVal.type == ValueType::STRING)
						larger = elems[i].asString() > maxVal.asString();

					if (larger) maxVal = elems[i];
				}
				return maxVal;
			}
			if (m->method == "average") {
				if (elems.empty()) return Value::Float(0);
				double total = 0;
				for (const auto& el : elems) total += el.asFloat();
				return Value::Float(total / elems.size());
			}
			if (m->method == "shuffle") {
				checkConst();
				if (elems.empty()) return Value::None();
				static std::random_device rd;
				static std::mt19937 gen(rd());
				std::shuffle(elems.begin(), elems.end(), gen);
				return target;
			}
			if (m->method == "sample") {
				int k = eval(m->args[0]).asInt();
				if (k > (int)elems.size()) error("Sample larger than population", "ValueError");
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
				std::function<void(const vector<Value>&)> recursiveFlatten;
				recursiveFlatten = [&](const vector<Value>& currentElems) {
					for (const auto& el : currentElems) {
						if (el.type == ValueType::LIST) {
							auto* sub = static_cast<ListObject*>(el.ref.get());
							recursiveFlatten(sub->elements);
						}
						else flatResult.push_back(el);
					}
				};
				recursiveFlatten(elems);
				elems = flatResult;
				return target;
			}
			if (m->method == "chunk") {
				int size = eval(m->args[0]).asInt();
				if (size <= 0) error("Chunk size must be > 0", "ValueError");
				vector<Value> chunks;
				vector<Value> current;
				for (const auto& el : elems) {
					current.push_back(el);
					if (current.size() == size) {
						chunks.push_back(Value::List(current));
						current.clear();
					}
				}
				if (!current.empty()) chunks.push_back(Value::List(current));
				return Value::List(chunks);
			}
			if (m->method == "rotate") {
				checkConst();
				if (elems.empty()) return Value::None();
				int n = eval(m->args[0]).asInt();
				n %= (int)elems.size();
				if (n < 0) n += elems.size();
				std::rotate(elems.rbegin(), elems.rbegin() + n, elems.rend());
				return target;
			}
			if (m->method == "unique") {
				checkConst();
				vector<Value> unique;
				for (const auto& item : elems) {
					bool exists = false;
					for (const auto& u : unique) if (u.strictEquals(item)) { exists = true; break; }
					if (!exists) unique.push_back(item);
				}
				elems = unique;
				return target;
			}
			if (m->method == "join") {
				string sep = "";
				if (!m->args.empty()) sep = eval(m->args[0]).asString();
				string res = "";
				for (size_t i = 0; i < elems.size(); i++) {
					if (elems[i].type == ValueType::STRING) res += elems[i].asString();
					else res += valueToString(elems[i]);
					if (i + 1 < elems.size()) res += sep;
				}
				return Value::String(res);
			}
			if (m->method == "fill") {
				checkConst();
				Value val = eval(m->args[0]);
				if (m->args.size() > 1) {
					int count = eval(m->args[1]).asInt();
					if (count < 0) count = 0;
					elems.clear();
					elems.reserve(count);
					for (int i = 0; i < count; i++) elems.push_back(deepCopy(val));
				}
				else for (auto& el : elems) el = deepCopy(val);
				return target;
			}
			error("Object '" + m->method + "' is not a list method", "AttributeError");
		}
		//--------- SET METHODS ----------
		if (target.type == ValueType::SET) {
			auto* setObj = static_cast<SetObject*>(target.ref.get());
			//--- MODIFIERS ---
			if (m->method == "add") {
				checkConst();
				if (m->args.size() != 1) error("add() takes exactly one argument", "ArgumentError");
				setAdd(setObj->elements, eval(m->args[0]));
				return target;
			}
			if (m->method == "remove") {
				checkConst();
				if (m->args.size() != 1) error("remove() takes exactly one argument", "ArgumentError");
				Value val = eval(m->args[0]);
				auto& elems = setObj->elements;
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
				if (m->args.size() != 1) error("discard() takes exactly one argument", "ArgumentError");
				Value val = eval(m->args[0]);
				auto& elems = setObj->elements;
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
				if (!m->args.empty()) error("pop() takes no arguments", "ArgumentError");
				if (setObj->elements.empty()) error("pop from empty set", "EmptyContainerError");
				Value val = setObj->elements.back();
				setObj->elements.pop_back();
				return val;
			}
			if (m->method == "clear") {
				checkConst();
				if (!m->args.empty()) error("clear() takes no arguments", "ArgumentError");
				setObj->elements.clear();
				return target;
			}
			//--- OPERATIONS ---
			if (m->method == "union") {
				if (m->args.size() != 1) error("union() takes one argument", "ArgumentError");
				Value other = eval(m->args[0]);
				if (other.type != ValueType::SET) error("union() requires a set", "TypeError");
				vector<Value> result = setObj->elements;
				auto* otherSet = static_cast<SetObject*>(other.ref.get());
				for (const auto& v : otherSet->elements) setAdd(result, v);
				return Value::Set(result);
			}
			if (m->method == "intersection") {
				if (m->args.size() != 1) error("intersection() takes one argument", "ArgumentError");
				Value other = eval(m->args[0]);
				if (other.type != ValueType::SET) error("intersection() requires a set", "TypeError");
				vector<Value> result;
				auto* otherSet = static_cast<SetObject*>(other.ref.get());
				for (const auto& v1 : setObj->elements) {
					for (const auto& v2 : otherSet->elements) if (v1.strictEquals(v2)) { result.push_back(v1); break; }
				}
				return Value::Set(result);
			}
			if (m->method == "difference") {
				if (m->args.size() != 1) error("difference() takes one argument", "ArgumentError");
				Value other = eval(m->args[0]);
				if (other.type != ValueType::SET) error("difference() requires a set", "TypeError");
				vector<Value> result;
				auto* otherSet = static_cast<SetObject*>(other.ref.get());
				for (const auto& v1 : setObj->elements) {
					bool found = false;
					for (const auto& v2 : otherSet->elements) if (v1.strictEquals(v2)) { found = true; break; }
					if (!found) result.push_back(v1);
				}
				return Value::Set(result);
			}
			if (m->method == "symmetric_difference") {
				if (m->args.size() != 1) error("symmetric_difference() takes one argument", "ArgumentError");
				Value other = eval(m->args[0]);
				if (other.type != ValueType::SET) error("symmetric_difference() requires a set", "TypeError");
				vector<Value> result;
				auto* s2 = static_cast<SetObject*>(other.ref.get());
				for (const auto& v1 : setObj->elements) {
					bool found = false;
					for (const auto& v2 : s2->elements) if (v1.strictEquals(v2)) { found = true; break; }
					if (!found) result.push_back(v1);
				}
				for (const auto& v2 : s2->elements) {
					bool found = false;
					for (const auto& v1 : setObj->elements) if (v2.strictEquals(v1)) { found = true; break; }
					if (!found) result.push_back(v2);
				}
				return Value::Set(result);
			}
			if (m->method == "issubset") {
				if (m->args.size() != 1) error("issubset() takes one argument", "ArgumentError");
				Value other = eval(m->args[0]);
				if (other.type != ValueType::SET) error("issubset() requires a set", "TypeError");
				auto* parent = static_cast<SetObject*>(other.ref.get());
				for (const auto& childElem : setObj->elements) {
					bool found = false;
					for (const auto& pElem : parent->elements) if (childElem.strictEquals(pElem)) { found = true; break; }
					if (!found) return Value::Bool(false);
				}
				return Value::Bool(true);
			}
			if (m->method == "issuperset") {
				if (m->args.size() != 1) error("issuperset() takes one argument", "ArgumentError");
				Value other = eval(m->args[0]);
				if (other.type != ValueType::SET) error("issuperset() requires a set", "TypeError");
				auto* child = static_cast<SetObject*>(other.ref.get());
				for (const auto& cElem : child->elements) {
					bool found = false;
					for (const auto& pElem : setObj->elements) if (cElem.strictEquals(pElem)) { found = true; break; }
					if (!found) return Value::Bool(false);
				}
				return Value::Bool(true);
			}
			if (m->method == "isdisjoint") {
				if (m->args.size() != 1) error("isdisjoint() takes one argument", "ArgumentError");
				Value other = eval(m->args[0]);
				if (other.type != ValueType::SET) error("isdisjoint() requires a set", "TypeError");
				auto* otherSet = static_cast<SetObject*>(other.ref.get());
				for (const auto& v1 : setObj->elements) {
					for (const auto& v2 : otherSet->elements) if (v1.strictEquals(v2)) return Value::Bool(false);
				}
				return Value::Bool(true);
			}
			if (m->method == "join") {
				string sep = "";
				if (!m->args.empty()) sep = eval(m->args[0]).asString();
				string res = "";
				auto& elems = setObj->elements; // Access set elements
				for (size_t i = 0; i < elems.size(); i++) {
					if (elems[i].type == ValueType::STRING) res += elems[i].asString();
					else res += valueToString(elems[i]);
					if (i + 1 < elems.size()) res += sep;
				}
				return Value::String(res);
			}
			if (m->method == "join") {
				string sep = "";
				if (!m->args.empty()) sep = eval(m->args[0]).asString();
				string res = "";
				auto& elems = setObj->elements;
				for (size_t i = 0; i < elems.size(); i++) {
					if (elems[i].type == ValueType::STRING) res += elems[i].asString();
					else res += valueToString(elems[i]);
					if (i + 1 < elems.size()) res += sep;
				}
				return Value::String(res);
			}
			error("Object '" + m->method + "' is not a set method", "AttributeError");
		}
		//-------- TUPLE METHODS ---------
		if (target.type == ValueType::TUPLE) {
			auto* tObj = static_cast<TupleObject*>(target.ref.get());
			if (m->method == "count") {
				if (m->args.size() != 1) error("tuple.count() takes 1 arg", "ArgumentError");
				Value needle = eval(m->args[0]);
				long long c = 0;
				for (const auto& el : tObj->elements) if (el.strictEquals(needle)) c++;
				return Value::Int(c);
			}
			if (m->method == "index") {
				if (m->args.size() != 1) error("tuple.index() takes 1 arg", "ArgumentError");
				Value needle = eval(m->args[0]);
				for (size_t i = 0; i < tObj->elements.size(); i++) {
					if (tObj->elements[i].strictEquals(needle)) return Value::Int((long long)i);
				}
				return Value::Int(-1);
			}
			if (m->method == "join") {
				string sep = "";
				if (!m->args.empty()) sep = eval(m->args[0]).asString();
				string res = "";
				auto& elems = tObj->elements;
				for (size_t i = 0; i < elems.size(); i++) {
					if (elems[i].type == ValueType::STRING) res += elems[i].asString();
					else res += valueToString(elems[i]);
					if (i + 1 < elems.size()) res += sep;
				}
				return Value::String(res);
			}
			error("Object '" + m->method + "' is not a tuple method", "AttributeError");
		}
		// -------- DICTIONARY METHODS ---------
		if (target.type == ValueType::DICT) {
			auto* d = static_cast<DictObject*>(target.ref.get());
			if (m->method == "get") {
				if (m->args.size() < 1 || m->args.size() > 2) error("get() takes 1 or 2 arguments", "ArgumentError");
				Value key = eval(m->args[0]);
				Value defVal = (m->args.size() == 2) ? eval(m->args[1]) : Value::None();

				if (d->items.count(key)) return d->items.at(key);
				return defVal;
			}
			if (m->method == "get_default") {
				checkConst();
				if (m->args.size() < 1 || m->args.size() > 2) error("get_default() takes 1 or 2 arguments", "ArgumentError");
				Value key = eval(m->args[0]);
				if (d->items.count(key)) return d->items.at(key);
				Value defVal = (m->args.size() == 2) ? eval(m->args[1]) : Value::None();
				Value insertKey = key;
				if (insertKey.type == ValueType::LIST || insertKey.type == ValueType::SET) {
					insertKey = deepCopy(insertKey);
					insertKey.isConst = true;
				}
				if (insertKey.type == ValueType::DICT) error("Dictionary cannot be used as a key", "TypeError");
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
				if (m->args.size() != 1) error("update() takes exactly 1 argument", "ArgumentError");
				Value other = eval(m->args[0]);
				if (other.type != ValueType::DICT) error("update() requires a dictionary argument", "TypeError");
				auto* otherDict = static_cast<DictObject*>(other.ref.get());
				for (const auto& [k, v] : otherDict->items) {
					d->items[k] = v;
				}
				return Value::None();
			}
			if (m->method == "pop") {
				checkConst();
				if (m->args.size() > 1) error("pop() takes 0 or 1 argument", "ArgumentError");
				if (m->args.empty()) {
					if (d->items.empty()) error("pop from empty dictionary", "EmptyContainerError");
					auto it = d->items.begin();
					Value val = it->second;
					d->items.erase(it);
					return val;
				}
				else {
					Value key = eval(m->args[0]);
					auto it = d->items.find(key);
					if (it == d->items.end()) error("KeyError: " + valueToString(key), "KeyError");
					Value val = it->second;
					d->items.erase(it);
					return val;
				}
			}
			if (m->method == "keys") {
				vector<Value> keys;
				keys.reserve(d->items.size());
				for (auto& [k, v] : d->items) keys.push_back(k);
				return Value::List(keys);
			}
			if (m->method == "values") {
				vector<Value> vals;
				vals.reserve(d->items.size());
				for (auto& [k, v] : d->items) vals.push_back(v);
				return Value::List(vals);
			}
			if (m->method == "items") {
				vector<Value> pairs;
				pairs.reserve(d->items.size());
				for (auto& [k, v] : d->items) {
					vector<Value> pair = { k, v };
					pairs.push_back(Value::Tuple(pair));
				}
				return Value::List(pairs);
			}
			error("Object '" + m->method + "' is not a dict method", "AttributeError");
		}
		// ------------------ FILE METHODS ------------------
		if (target.type == ValueType::FILE) {
			auto* f = static_cast<FileObject*>(target.ref.get());
			if (!f->isOpen && m->method != "isOpen") throw FileClosedError("Cannot perform operation on closed file", m->line, m->col);
			if (m->method == "Write") {
				if (m->args.empty() || m->args.size() > 2) error("write() expects message and optional replace bool", "ArgumentError");
				string msg = valueToString(eval(m->args[0]));
				bool replace = false;
				if (m->args.size() == 2) replace = eval(m->args[1]).asBool();
				f->stream.clear();
				if (replace) f->stream.seekp(0, std::ios::beg);
				else f->stream.seekp(0, std::ios::end);
				if (!(f->stream << msg)) throw PermissionError("Failed to write to file", m->line, m->col);
				f->stream.flush();
				return Value::None();
			}
			if (m->method == "Read") {
				if (!m->args.empty()) error("Read() takes no arguments", "ArgumentError");
				f->stream.clear();
				f->stream.seekg(0, std::ios::beg);
				std::stringstream buffer;
				buffer << f->stream.rdbuf();
				return Value::String(buffer.str());
			}
			if (m->method == "ReadLines") {
				bool includeN = false;
				if (m->args.size() == 1) includeN = eval(m->args[0]).asBool();
				else if (m->args.size() > 1) error("ReadLines() takes optional boolean", "ArgumentError");
				f->stream.clear();
				f->stream.seekg(0, std::ios::beg);
				vector<Value> lines;
				string line;
				while (std::getline(f->stream, line)) {
					if (!line.empty() && line.back() == '\r') line.pop_back();
					if (includeN) line += "\n";
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
			error("Object '" + m->method + "' is not a file method", "AttributeError");
		}
		//-------- VECTOR METHODS --------
		if (target.type == ValueType::VECTOR) {
			auto* v = static_cast<VectorObject*>(target.ref.get());
			if (m->method == "dimension") {
				if (!m->args.empty()) error("dimension() takes no arguments", "ArgumentError");
				return Value::Int((long long)v->elements.size());
			}
			if (m->method == "magnitude") {
				if (!m->args.empty()) error("magnitude() takes no arguments", "ArgumentError");
				Value sum = Value::Int(0);
				for (const auto& d : v->elements) {
					Value sq;
					if (d.type == ValueType::INT) {
						long long r = d.iVal * d.iVal;
						bool ovf = (d.iVal != 0 && r / d.iVal != d.iVal);
						if (ovf) sq = BigIntObject::mul(Value::BigInt(d.iVal), Value::BigInt(d.iVal));
						else sq = Value::Int(r);
					}
					else if (d.type == ValueType::BIGINT) sq = BigIntObject::mul(d, d);
					else sq = Value::Float(d.asFloat() * d.asFloat());
					if (sum.type == ValueType::INT && sq.type == ValueType::INT) {
						long long r = sum.iVal + sq.iVal;
						bool ovf = ((sum.iVal ^ r) & (sq.iVal ^ r)) < 0;
						if (ovf) sum = BigIntObject::add(Value::BigInt(sum.iVal), Value::BigInt(sq.iVal));
						else sum = Value::Int(r);
					}
					else if (sum.type == ValueType::BIGINT || sq.type == ValueType::BIGINT) sum = BigIntObject::add(sum, sq);
					else sum = Value::Float(sum.asFloat() + sq.asFloat());
				}
				return Value::Float(std::sqrt(sum.asFloat()));
			}
			if (m->method == "reverse") {
				checkConst();
				std::reverse(v->elements.begin(), v->elements.end());
				return target;
			}
			if (m->method == "expand") {
				checkConst();
				if (m->args.size() != 1) error("expand() takes 1 argument", "ArgumentError");
				long long n = eval(m->args[0]).asInt();
				if (n < 0) error("Cannot expand by negative amount", "ValueError");
				for (int i = 0; i < n; i++) v->elements.push_back(Value::Int(0));
				return target;
			}
			if (m->method == "shrink") {
				checkConst();
				if (m->args.size() != 1) error("shrink() takes 1 argument", "ArgumentError");
				long long n = eval(m->args[0]).asInt();
				if (n < 0) error("Cannot shrink by negative amount", "ValueError");
				if ((long long)v->elements.size() - n < 1) error("Vector cannot be shrunk below 1 dimension", "ValueError");
				for (int i = 0; i < n; i++) v->elements.pop_back();
				return target;
			}
			if (m->method == "unitVec") {
				Value sum = Value::Int(0);
				for (const auto& d : v->elements) {
					Value sq;
					if (d.type == ValueType::INT) {
						long long r = d.iVal * d.iVal;
						bool ovf = (d.iVal != 0 && r / d.iVal != d.iVal);
						if (ovf) sq = BigIntObject::mul(Value::BigInt(d.iVal), Value::BigInt(d.iVal));
						else sq = Value::Int(r);
					}
					else if (d.type == ValueType::BIGINT) sq = BigIntObject::mul(d, d);
					else sq = Value::Float(d.asFloat() * d.asFloat());
					if (sum.type == ValueType::INT && sq.type == ValueType::INT) {
						long long r = sum.iVal + sq.iVal;
						bool ovf = ((sum.iVal ^ r) & (sq.iVal ^ r)) < 0;
						if (ovf) sum = BigIntObject::add(Value::BigInt(sum.iVal), Value::BigInt(sq.iVal));
						else sum = Value::Int(r);
					}
					else if (sum.type == ValueType::BIGINT || sq.type == ValueType::BIGINT) sum = BigIntObject::add(sum, sq);
					else sum = Value::Float(sum.asFloat() + sq.asFloat());
				}
				double mag = std::sqrt(sum.asFloat());
				if (mag == 0) error("Cannot get unit vector of zero vector", "MathError");
				std::vector<Value> res;
				res.reserve(v->elements.size());
				for (auto d : v->elements) res.push_back(Value::Float(d.asFloat() / mag));
				return Value::Vector(res);
			}
			if (m->method == "projectOnto") {
				if (m->args.size() != 1) error("projectOnto() takes 1 argument", "ArgumentError");
				Value other = eval(m->args[0]);
				if (other.type != ValueType::VECTOR) error("Argument must be a vector", "TypeError");
				auto* u = v;
				auto* v2 = static_cast<VectorObject*>(other.ref.get());
				if (u->elements.size() != v2->elements.size()) error("Dimension mismatch", "ValueError");
				Value dot = Value::Int(0);
				Value mag2 = Value::Int(0);
				for (size_t i = 0; i < u->elements.size(); i++) {
					Value valU = u->elements[i];
					Value valV = v2->elements[i];
					Value prod;
					if (valU.type == ValueType::INT && valV.type == ValueType::INT) {
						long long r = valU.iVal * valV.iVal;
						bool ovf = (valU.iVal != 0 && r / valU.iVal != valV.iVal);
						if (ovf) prod = BigIntObject::mul(Value::BigInt(valU.iVal), Value::BigInt(valV.iVal));
						else prod = Value::Int(r);
					}
					else if (valU.type == ValueType::BIGINT || valV.type == ValueType::BIGINT) prod = BigIntObject::mul(valU, valV);
					else prod = Value::Float(valU.asFloat() * valV.asFloat());
					if (dot.type == ValueType::INT && prod.type == ValueType::INT) {
						long long r = dot.iVal + prod.iVal;
						bool ovf = ((dot.iVal ^ r) & (prod.iVal ^ r)) < 0;
						if (ovf) dot = BigIntObject::add(Value::BigInt(dot.iVal), Value::BigInt(prod.iVal));
						else dot = Value::Int(r);
					}
					else if (dot.type == ValueType::BIGINT || prod.type == ValueType::BIGINT) dot = BigIntObject::add(dot, prod);
					else dot = Value::Float(dot.asFloat() + prod.asFloat());
					Value sq;
					if (valV.type == ValueType::INT) {
						long long r = valV.iVal * valV.iVal;
						bool ovf = (valV.iVal != 0 && r / valV.iVal != valV.iVal);
						if (ovf) sq = BigIntObject::mul(Value::BigInt(valV.iVal), Value::BigInt(valV.iVal));
						else sq = Value::Int(r);
					}
					else if (valV.type == ValueType::BIGINT) sq = BigIntObject::mul(valV, valV);
					else sq = Value::Float(valV.asFloat() * valV.asFloat());
					if (mag2.type == ValueType::INT && sq.type == ValueType::INT) {
						long long r = mag2.iVal + sq.iVal;
						bool ovf = ((mag2.iVal ^ r) & (sq.iVal ^ r)) < 0;
						if (ovf) mag2 = BigIntObject::add(Value::BigInt(mag2.iVal), Value::BigInt(sq.iVal));
						else mag2 = Value::Int(r);
					}
					else if (mag2.type == ValueType::BIGINT || sq.type == ValueType::BIGINT) mag2 = BigIntObject::add(mag2, sq);
					else mag2 = Value::Float(mag2.asFloat() + sq.asFloat());
				}
				if (mag2.asFloat() == 0) error("Cannot project onto zero vector", "MathError");
				double scalar = dot.asFloat() / mag2.asFloat();
				std::vector<Value> res;
				res.reserve(v2->elements.size());
				for (auto d : v2->elements) res.push_back(Value::Float(d.asFloat() * scalar));
				return Value::Vector(res);
			}
			if (m->method == "dot") {
				if (m->args.size() != 1) error("dot() takes 1 argument", "ArgumentError");
				Value other = eval(m->args[0]);
				if (other.type != ValueType::VECTOR) error("Argument must be a vector", "TypeError");
				auto* v2 = static_cast<VectorObject*>(other.ref.get());
				if (v->elements.size() != v2->elements.size()) error("Dimension mismatch", "ValueError");
				Value dot = Value::Int(0);
				for (size_t i = 0; i < v->elements.size(); i++) {
					Value x = v->elements[i];
					Value y = v2->elements[i];
					Value prod;
					if (x.type == ValueType::INT && y.type == ValueType::INT) {
						long long r = x.iVal * y.iVal;
						bool ovf = (x.iVal != 0 && r / x.iVal != y.iVal);
						if (ovf) prod = BigIntObject::mul(Value::BigInt(x.iVal), Value::BigInt(y.iVal));
						else prod = Value::Int(r);
					}
					else if (x.type == ValueType::BIGINT || y.type == ValueType::BIGINT) prod = BigIntObject::mul(x, y);
					else prod = Value::Float(x.asFloat() * y.asFloat());
					if (dot.type == ValueType::INT && prod.type == ValueType::INT) {
						long long r = dot.iVal + prod.iVal;
						bool ovf = ((dot.iVal ^ r) & (prod.iVal ^ r)) < 0;
						if (ovf) dot = BigIntObject::add(Value::BigInt(dot.iVal), Value::BigInt(prod.iVal));
						else dot = Value::Int(r);
					}
					else if (dot.type == ValueType::BIGINT || prod.type == ValueType::BIGINT) dot = BigIntObject::add(dot, prod);
					else dot = Value::Float(dot.asFloat() + prod.asFloat());
				}
				return dot;
			}
			if (m->method == "cross") {
				if (m->args.size() != 1) error("cross() takes 1 argument", "ArgumentError");
				Value other = eval(m->args[0]);
				if (other.type != ValueType::VECTOR) error("Argument must be a vector", "TypeError");
				auto* v2 = static_cast<VectorObject*>(other.ref.get());
				size_t dim = v->elements.size();
				if (dim != v2->elements.size()) error("Dimension mismatch", "ValueError");
				auto safeMul = [](const Value& a, const Value& b) -> Value {
					if (a.type == ValueType::INT && b.type == ValueType::INT) {
						long long r = a.iVal * b.iVal;
						bool ovf = (a.iVal != 0 && r / a.iVal != b.iVal);
						if (ovf) return BigIntObject::mul(Value::BigInt(a.iVal), Value::BigInt(b.iVal));
						return Value::Int(r);
					}
					if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) return BigIntObject::mul(a, b);
					return Value::Float(a.asFloat() * b.asFloat());
				};
				auto safeSub = [](const Value& a, const Value& b) -> Value {
					if (a.type == ValueType::INT && b.type == ValueType::INT) {
						long long r = a.iVal - b.iVal;
						bool ovf = ((a.iVal ^ b.iVal) & (a.iVal ^ r)) < 0;
						if (ovf) return BigIntObject::sub(Value::BigInt(a.iVal), Value::BigInt(b.iVal));
						return Value::Int(r);
					}
					if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) return BigIntObject::sub(a, b);
					return Value::Float(a.asFloat() - b.asFloat());
				};
				if (dim == 1) return Value::Int(0);
				else if (dim == 2) {
					Value term1 = safeMul(v->elements[0], v2->elements[1]);
					Value term2 = safeMul(v->elements[1], v2->elements[0]);
					return safeSub(term1, term2);
				}
				else if (dim == 3) {
					Value x = safeSub(safeMul(v->elements[1], v2->elements[2]), safeMul(v->elements[2], v2->elements[1]));
					Value y = safeSub(safeMul(v->elements[2], v2->elements[0]), safeMul(v->elements[0], v2->elements[2]));
					Value z = safeSub(safeMul(v->elements[0], v2->elements[1]), safeMul(v->elements[1], v2->elements[0]));
					std::vector<Value> res = { x, y, z };
					return Value::Vector(res);
				}
				else {
					error("Binary cross product is not defined for dimensions > 3", "ValueError");
				}
			}
			error("Object '" + m->method + "' is not a vector method", "AttributeError");
		}
		return Value::None();
	}
	bool matchesError(const LangError& e, const string& typeName) {
		// --- ROOT ---
		if (typeName == "Error") return true;
		// --- LEVEL 1: BASE CATEGORIES ---
		if (typeName == "InternalError") return dynamic_cast<const InternalError*>(&e);
		if (typeName == "ControlFlowError") return dynamic_cast<const ControlFlowError*>(&e);
		if (typeName == "ParseError") return dynamic_cast<const ParseError*>(&e);
		if (typeName == "RuntimeError") return dynamic_cast<const RuntimeError*>(&e);
		if (typeName == "Warning") return dynamic_cast<const Warning*>(&e);
		// --- LEVEL 2: RUNTIME ERROR SUBTREE ---
		if (typeName == "NameError") return dynamic_cast<const NameError*>(&e);
		if (typeName == "AttributeError") return dynamic_cast<const AttributeError*>(&e);
		if (typeName == "TypeError") return dynamic_cast<const TypeError*>(&e);
		if (typeName == "ArgumentError") return dynamic_cast<const ArgumentError*>(&e);
		if (typeName == "ValueError") return dynamic_cast<const ValueError*>(&e);
		if (typeName == "ConstError") return dynamic_cast<const ConstError*>(&e);
		if (typeName == "OwnershipError") return dynamic_cast<const OwnershipError*>(&e);
		if (typeName == "IndexError") return dynamic_cast<const IndexError*>(&e);
		if (typeName == "KeyError") return dynamic_cast<const KeyError*>(&e);
		if (typeName == "RangeError") return dynamic_cast<const RangeError*>(&e);
		if (typeName == "AssertionError") return dynamic_cast<const AssertionError*>(&e);
		if (typeName == "RecursionError") return dynamic_cast<const RecursionError*>(&e);
		if (typeName == "ImportError") return dynamic_cast<const ImportError*>(&e);
		if (typeName == "IOError") return dynamic_cast<const IOError*>(&e);
		if (typeName == "MathError") return dynamic_cast<const MathError*>(&e);
		if (typeName == "CastError") return dynamic_cast<const CastError*>(&e);
		if (typeName == "IteratorError") return dynamic_cast<const IteratorError*>(&e);
		if (typeName == "TimeoutError") return dynamic_cast<const TimeoutError*>(&e);
		if (typeName == "MemoryError") return dynamic_cast<const MemoryError*>(&e);
		if (typeName == "SystemError") return dynamic_cast<const SystemError*>(&e);
		// Children of ValueError
		if (typeName == "EmptyContainerError") return dynamic_cast<const EmptyContainerError*>(&e);
		// Children of ConstError
		if (typeName == "MutationError") return dynamic_cast<const MutationError*>(&e);
		// Children of ImportError
		if (typeName == "ModuleNotFoundError") return dynamic_cast<const ModuleNotFoundError*>(&e);
		if (typeName == "CircularImportError") return dynamic_cast<const CircularImportError*>(&e);
		if (typeName == "InvalidImportError") return dynamic_cast<const InvalidImportError*>(&e);
		// Children of IOError
		if (typeName == "FileNotFoundError") return dynamic_cast<const FileNotFoundError*>(&e);
		if (typeName == "PermissionError") return dynamic_cast<const PermissionError*>(&e);
		if (typeName == "EOFError") return dynamic_cast<const EOFError*>(&e);
		if (typeName == "FileClosedError") return dynamic_cast<const FileClosedError*>(&e);
		// Children of MathError
		if (typeName == "DivisionByZeroError") return dynamic_cast<const DivisionByZeroError*>(&e);
		if (typeName == "OverflowError") return dynamic_cast<const OverflowError*>(&e);
		if (typeName == "UnderflowError") return dynamic_cast<const UnderflowError*>(&e);
		if (typeName == "DomainError") return dynamic_cast<const DomainError*>(&e);
		// Children of SystemError
		if (typeName == "OSError") return dynamic_cast<const OSError*>(&e);
		if (typeName == "EnvironmentError") return dynamic_cast<const EnvironmentError*>(&e);
		if (typeName == "SignalError") return dynamic_cast<const SignalError*>(&e);
		// Children of ControlFlowError
		if (typeName == "ReturnSignal") return dynamic_cast<const ReturnSignal*>(&e);
		if (typeName == "BreakSignal") return dynamic_cast<const BreakSignal*>(&e);
		if (typeName == "ContinueSignal") return dynamic_cast<const ContinueSignal*>(&e);
		// Children of ParseError
		if (typeName == "SyntaxError") return dynamic_cast<const SyntaxError*>(&e);
		if (typeName == "IndentationError") return dynamic_cast<const IndentationError*>(&e);
		if (typeName == "UnexpectedTokenError") return dynamic_cast<const UnexpectedTokenError*>(&e);
		if (typeName == "UnterminatedLiteralError") return dynamic_cast<const UnterminatedLiteralError*>(&e);
		// Children of Warning
		if (typeName == "DeprecationWarning") return dynamic_cast<const DeprecationWarning*>(&e);
		if (typeName == "RuntimeWarning") return dynamic_cast<const RuntimeWarning*>(&e);
		if (typeName == "ImportWarning") return dynamic_cast<const ImportWarning*>(&e);
		// Fallback: Direct String Match (For exact matches or user-defined types)
		return e.type == typeName;
	}
	Value eval(Expr* e) {
		if (!e) return Value::None();
		if (ValueExpr* ve = dynamic_cast<ValueExpr*>(e)) return ve->val;
		switch(e->type){
			case ExprType::FSTRING: {
				auto fs = static_cast<FStringExpr*>(e);
				string res = "";
				for (auto* part : fs->parts) {
					Value v = eval(part);
					if (v.type == ValueType::STRING) res += v.asString();
					else res += valueToString(v);
				}
				return Value::String(res);
			}
			case ExprType::OWNERSHIP: {
				auto o = static_cast<OwnershipExpr*>(e);
				if (o->mode == CopyMode::REF) {
					LValue lv = resolveLValue(o->expr);
					return *lv.ref;
				}
				Value v = eval(o->expr);
				return applyCopy(v, o->mode);
			}
			case ExprType::BOOL: {
				return Value::Bool(static_cast<BoolExpr*>(e)->value);
			}
			case ExprType::NUMBER:{
				auto n = static_cast<NumberExpr*>(e);
				return n->isFloat ? Value::Float(n->val) : Value::Int((long long)n->val);
			}
			case ExprType::STRING:{ 
				return Value::String(static_cast<StringExpr*>(e)->val);
			}
			case ExprType::VAR: {
				auto* v = static_cast<VarExpr*>(e);
				if (v->cachedGlobal) return *v->cachedGlobal;
				if (!env->exists(v->name)) throw NameError("Undefined variable '" + v->name + "'", e->line, e->col);
				Var& var = env->lookup(v->name);
				if (env->parent == nullptr) v->cachedGlobal = &var.value;
				if (var.alias) return *var.alias;
				return var.value;
			}
			case ExprType::LIST: {
				auto l = static_cast<ListExpr*>(e);
				vector<Value> vals;
				for (auto el : l->elements) vals.push_back(eval(el));
				return Value::List(vals);
			}
			case ExprType::BINARY: {
				auto b = static_cast<BinExpr*>(e);
				if (b->left && b->right && b->left->type == ExprType::NUMBER && b->right->type == ExprType::NUMBER) {
					double l = static_cast<NumberExpr*>(b->left)->val;
					double r = static_cast<NumberExpr*>(b->right)->val;
					switch (b->op) {
					case TokenType::PLUS: return Value::Float(l + r);
					case TokenType::MINUS: return Value::Float(l - r);
					case TokenType::STAR: return Value::Float(l * r);
					case TokenType::SLASH: return r != 0 ? Value::Float(l / r) : Value::None();
					case TokenType::POW: return Value::Float(pow(l, r));
					}
				}
				if (b->op == TokenType::AND) {
					Value l = eval(b->left);
					if (!l.isTruthy()) return Value::Bool(false);
					return Value::Bool(eval(b->right).isTruthy());
				}
				if (b->op == TokenType::OR) {
					Value l = eval(b->left);
					if (l.isTruthy()) return Value::Bool(true);
					return Value::Bool(eval(b->right).isTruthy());
				}
				Value l = b->left ? eval(b->left) : Value::None();
				Value r = eval(b->right);
				switch (b->op) {
				case TokenType::PLUS:
					if (l.type == ValueType::STRING || r.type == ValueType::STRING)
						return Value::String((l.type == ValueType::STRING ? l.asString() : (l.type == ValueType::FLOAT ? std::to_string(l.asFloat()) : std::to_string(l.asInt()))) + (r.type == ValueType::STRING ? r.asString() : (r.type == ValueType::FLOAT ? std::to_string(r.asFloat()) : std::to_string(r.asInt()))));
					if (l.type == ValueType::INT && r.type == ValueType::INT) {
						long long a = l.asInt();
						long long b = r.asInt();
						if ((b > 0 && a > LLONG_MAX - b) || (b < 0 && a < LLONG_MIN - b)) return Value::Float((double)a + (double)b);
						return Value::Int(a + b);
					}
					if (l.isNumber() && r.isNumber()) {
						return Value::Float(l.asFloat() + r.asFloat());
					}
					if (l.type == ValueType::VECTOR && r.type == ValueType::VECTOR) {
						auto* v1 = static_cast<VectorObject*>(l.ref.get());
						auto* v2 = static_cast<VectorObject*>(r.ref.get());
						if (v1->elements.size() != v2->elements.size())
							throw ValueError("Vector dimension mismatch in addition", e->line, e->col);
						vector<Value> res;
						for (size_t i = 0; i < v1->elements.size(); i++) res.push_back(Value::Float(v1->elements[i].asFloat() + v2->elements[i].asFloat()));
						return Value::Vector(res);
					}
					throw TypeError("Invalid operands for +", e->line, e->col);
				case TokenType::FLOOR_DIV:
					if (r.asFloat() == 0) throw DivisionByZeroError("Division by zero", e->line, e->col);
					if (l.isNumber() && r.isNumber()) return Value::Int((long long)std::floor(l.asFloat() / r.asFloat()));
					break;
				case TokenType::MOD:
					if (r.asInt() == 0) throw DivisionByZeroError("Modulo by zero", e->line, e->col);
					if (l.type == ValueType::INT && r.type == ValueType::INT) return Value::Int(l.asInt() % r.asInt());
					if (l.isNumber() && r.isNumber()) {
						return Value::Float(std::fmod(l.asFloat(), r.asFloat()));
					}
					break;
				case TokenType::POW:
					if (l.isNumber() && r.isNumber()) return Value::Float(std::pow(l.asFloat(), r.asFloat()));
					break;
				case TokenType::MINUS:
					if (l.isNumber() && r.isNumber()) {
						if (l.type == ValueType::FLOAT || r.type == ValueType::FLOAT) return Value::Float(l.asFloat() - r.asFloat());
						return Value::Int(l.asInt() - r.asInt());
					}
					if (l.type == ValueType::VECTOR && r.type == ValueType::VECTOR) {
						auto* v1 = static_cast<VectorObject*>(l.ref.get());
						auto* v2 = static_cast<VectorObject*>(r.ref.get());
						if (v1->elements.size() != v2->elements.size())
							throw ValueError("Vector dimension mismatch in subtraction", e->line, e->col);
						vector<Value> res;
						for (size_t i = 0; i < v1->elements.size(); i++) res.push_back(Value::Float(v1->elements[i].asFloat() - v2->elements[i].asFloat()));
						return Value::Vector(res);
					}
					break;
				case TokenType::STAR:
					if (l.type == ValueType::INT && r.type == ValueType::INT) {
						long long a = l.asInt();
						long long b = r.asInt();
						if (a != 0 && (b > LLONG_MAX / abs(a) || b < LLONG_MIN / abs(a))) return Value::Float((double)a * (double)b);
						return Value::Int(a * b);
					}
					if (l.type == ValueType::STRING && r.type == ValueType::INT) {
						string res = "";
						string base = l.asString();
						long long count = r.asInt();
						if (count < 0) count = 0;
						if (count > 1000000) throw MemoryError("String repetition too large", e->line, e->col);
						for (long long i = 0; i < count; i++) res += base;
						return Value::String(res);
					}
					if (l.type == ValueType::LIST && r.type == ValueType::INT) {
						auto* listObj = static_cast<ListObject*>(l.ref.get());
						long long count = r.asInt();
						vector<Value> res;
						if (count > 0) {
							if (listObj->elements.size() * count > 1000000)
								throw MemoryError("List repetition too large", e->line, e->col);
							res.reserve(listObj->elements.size() * count);
							for (int i = 0; i < count; i++) for (const auto& elem : listObj->elements) res.push_back(deepCopy(elem));
						}
						return Value::List(res);
					}
					if (l.isNumber() && r.isNumber()) {
						return Value::Float(l.asFloat() * r.asFloat());
					}
					if (l.type == ValueType::VECTOR && r.isNumber()) {
						auto* v = static_cast<VectorObject*>(l.ref.get());
						double s = r.asFloat();
						vector<Value> res;
						for (auto val : v->elements) res.push_back(Value::Float(val.asFloat() * s));
						return Value::Vector(res);
					}
					if (l.isNumber() && r.type == ValueType::VECTOR) {
						auto* v = static_cast<VectorObject*>(r.ref.get());
						double s = l.asFloat();
						vector<Value> res;
						for (auto val : v->elements) res.push_back(Value::Float(val.asFloat() * s));
						return Value::Vector(res);
					}
					if (l.type == ValueType::VECTOR && r.type == ValueType::VECTOR) {
						auto* v1 = static_cast<VectorObject*>(l.ref.get());
						auto* v2 = static_cast<VectorObject*>(r.ref.get());
						if (v1->elements.size() != v2->elements.size())
							throw ValueError("Vector dimension mismatch in multiplication", e->line, e->col);
						vector<Value> res;
						for (size_t i = 0; i < v1->elements.size(); i++) res.push_back(Value::Float(v1->elements[i].asFloat() * v2->elements[i].asFloat()));
						return Value::Vector(res);
					}
					break;
				case TokenType::SLASH:
					if (r.asFloat() == 0) throw DivisionByZeroError("Division by zero", e->line, e->col);
					if (l.isNumber() && r.isNumber()) return Value::Float(l.asFloat() / r.asFloat());
					if (l.type == ValueType::VECTOR || r.type == ValueType::VECTOR)
						throw TypeError("Vector division is not supported", e->line, e->col);
					break;
				case TokenType::GT: return Value::Bool(l.asFloat() > r.asFloat());
				case TokenType::LT: return Value::Bool(l.asFloat() < r.asFloat());
				case TokenType::GTE: return Value::Bool(l.asFloat() >= r.asFloat());
				case TokenType::LTE: return Value::Bool(l.asFloat() <= r.asFloat());
				case TokenType::EQ: return Value::Bool(l.looseEquals(r));
				case TokenType::STRICT_EQ: return Value::Bool(l.strictEquals(r));
				case TokenType::STRICT_NEQ: return Value::Bool(!l.strictEquals(r));
				case TokenType::NEQ: return Value::Bool(!l.looseEquals(r));
				case TokenType::AND:
					if (l.type == ValueType::SET && r.type == ValueType::SET) break;
					return Value::Bool(l.isTruthy() && r.isTruthy());
				case TokenType::OR:
					if (l.type == ValueType::SET && r.type == ValueType::SET) break;
					return Value::Bool(l.isTruthy() || r.isTruthy());
				case TokenType::XOR:
					if (l.type == ValueType::SET && r.type == ValueType::SET) break;
					return Value::Bool(l.isTruthy() != r.isTruthy());
				case TokenType::NAND: return Value::Bool(!(l.isTruthy() && r.isTruthy()));
				case TokenType::NOR: return Value::Bool(!(l.isTruthy() || r.isTruthy()));
				case TokenType::NXOR: return Value::Bool(l.isTruthy() == r.isTruthy());
				case TokenType::NOT: return Value::Bool(!r.isTruthy());
				case TokenType::IS: {
					if (l.ref && r.ref) return Value::Bool(l.ref.get() == r.ref.get());
					if (l.type == ValueType::NONE && r.type == ValueType::NONE) return Value::Bool(true);
					return Value::Bool(false);
				}
				case TokenType::IS_IN: {
					if (r.type == ValueType::STRING) {
						if (l.type != ValueType::STRING) {
							return Value::Bool(false);
						}
						return Value::Bool(r.asString().find(l.asString()) != string::npos);
					}
					if (r.type == ValueType::LIST) {
						auto* listObj = static_cast<ListObject*>(r.ref.get());
						for (const auto& item : listObj->elements) {
							if (item.strictEquals(l)) return Value::Bool(true);
						}
						return Value::Bool(false);
					}
					if (r.type == ValueType::DICT) {
						auto* d = static_cast<DictObject*>(r.ref.get());
						return Value::Bool(d->items.count(l));
					}
					if (r.type == ValueType::SET) {
						auto* s = static_cast<SetObject*>(r.ref.get());
						for (const auto& item : s->elements) {
							if (item.strictEquals(l)) return Value::Bool(true);
						}
						return Value::Bool(false);
					}
					if (r.type == ValueType::RANGE) {
						auto* rng = static_cast<RangeObject*>(r.ref.get());
						if (!l.isNumber()) return Value::Bool(false);
						double val = l.asFloat();
						if (!rng->isValid) return Value::Bool(false);
						bool inBounds = (rng->step > 0) ?
							(val >= rng->start && (rng->endInclusive ? val <= rng->end : val < rng->end)) :
							(val <= rng->start && (rng->endInclusive ? val >= rng->end : val > rng->end));
						if (!inBounds) return Value::Bool(false);
						if (!rng->isFloat && l.type == ValueType::INT) {
							long long diff = (long long)(val - rng->start);
							if (diff % (long long)rng->step != 0) return Value::Bool(false);
						}
						return Value::Bool(true);
					}
					throw TypeError("RHS of 'in' must be iterable", e->line, e->col);
				}
				case TokenType::IS_NOT: {
					if (l.ref && r.ref) return Value::Bool(l.ref.get() != r.ref.get());
					if (l.type == ValueType::NONE && r.type == ValueType::NONE) return Value::Bool(false);
					return Value::Bool(true);
				}
				case TokenType::IS_NOT_IN: {
					if (r.type == ValueType::STRING) {
						if (l.type != ValueType::STRING) throw TypeError("LHS of 'is not in' string must be a string", e->line, e->col);
						return Value::Bool(r.asString().find(l.asString()) == string::npos);
					}
					else if (r.type == ValueType::LIST) {
						auto* listObj = static_cast<ListObject*>(r.ref.get());
						for (const auto& item : listObj->elements) if (item.looseEquals(l)) return Value::Bool(false);
						return Value::Bool(true);
					}
					else if (r.type == ValueType::RANGE) {
						auto* rng = static_cast<RangeObject*>(r.ref.get());
						if (!l.isNumber()) return Value::Bool(true);
						double val = l.asFloat();
						if (!rng->isValid) return Value::Bool(true);
						bool inBounds = (rng->step > 0) ?
							(val >= rng->start && (rng->endInclusive ? val <= rng->end : val < rng->end)) :
							(val <= rng->start && (rng->endInclusive ? val >= rng->end : val > rng->end));
						if (!inBounds) return Value::Bool(true);
						if (!rng->isFloat && l.type == ValueType::INT) {
							long long diff = (long long)(val - rng->start);
							if (diff % (long long)rng->step != 0) return Value::Bool(true);
						}
						return Value::Bool(false);
					}
					else if (r.type == ValueType::SET) {
						auto* s = static_cast<SetObject*>(r.ref.get());
						for (const auto& el : s->elements) if (el.strictEquals(l)) return Value::Bool(false);
						return Value::Bool(true);
					}
					else if (r.type == ValueType::TUPLE) {
						auto* t = static_cast<TupleObject*>(r.ref.get());
						for (const auto& item : t->elements) if (item.looseEquals(l)) return Value::Bool(false);
						return Value::Bool(true);
					}
					throw TypeError("RHS of 'is not in' must be iterable", e->line, e->col);
				}
				case TokenType::COLON: {
					Value k = l;
					Value v = r;
					vector<std::pair<Value, Value>> p;
					p.push_back({ k, v });
					return Value::Paired(p);
				}
				default: return Value::None();
				}
				if (l.type == ValueType::SET && r.type == ValueType::SET) {
					auto* s1 = static_cast<SetObject*>(l.ref.get());
					auto* s2 = static_cast<SetObject*>(r.ref.get());
					vector<Value> result;
					if (b->op == TokenType::OR) {
						result = s1->elements;
						for (auto& v : s2->elements) setAdd(result, v);
						return Value::Set(result);
					}
					if (b->op == TokenType::AND) {
						for (auto& v1 : s1->elements) {
							for (auto& v2 : s2->elements) if (v1.strictEquals(v2)) { result.push_back(v1); break; }
						}
						return Value::Set(result);
					}
					if (b->op == TokenType::MINUS || b->op == TokenType::SLASH) {
						for (auto& v1 : s1->elements) {
							bool found = false;
							for (auto& v2 : s2->elements) if (v1.strictEquals(v2)) { found = true; break; }
							if (!found) result.push_back(v1);
						}
						return Value::Set(result);
					}
					if (b->op == TokenType::XOR) {
						for (auto& v1 : s1->elements) {
							bool found = false;
							for (auto& v2 : s2->elements) if (v1.strictEquals(v2)) { found = true; break; }
							if (!found) result.push_back(v1);
						}
						for (auto& v2 : s2->elements) {
							bool found = false;
							for (auto& v1 : s1->elements) if (v2.strictEquals(v1)) { found = true; break; }
							if (!found) result.push_back(v2);
						}
						return Value::Set(result);
					}
				}
			}
			case ExprType::OMIT_MARKER_EXPR: {
				return Value::Omit();
			}
			case ExprType::CALL: {
				auto c = static_cast<CallExpr*>(e);
				Value callee;
				if (c-> cachedFunc) {
					callee = *c->cachedFunc;
				}
				else {
					if (!env->exists(c->name)) throw NameError("Undefined function '" + c->name + "'", c->line, c->col);
					Var& v = env->lookup(c->name);
					callee = v.value;
					if (env->parent == nullptr) c->cachedFunc = &v.value;
				}
				vector<CallArg> callArgs;
				for (size_t i = 0; i < c->args.size(); i++) {
					CallArg ca;
					if (c->modes[i] == CopyMode::REF) {
						LValue lv = resolveLValue(c->args[i]);
						ca.lvalue = lv;
						ca.hasLValue = true;
						ca.value = *lv.ref;
					}
					else ca.value = eval(c->args[i]);
					callArgs.push_back(ca);
				}
				return call(callee, callArgs, c->modes, c->line, c->col);
			}
			case ExprType::INDEX: {
				auto idx = static_cast<IndexExpr*>(e);
				Value base = eval(idx->base);
				Value index = eval(idx->index);
				auto normalize = [&](long long i, size_t len) {
					if (i < 0) i += len;
					if (i < 0 || i >= (long long)len)
						throw IndexError("Index out of bounds", idx->line, idx->col);
					return i;
				};
				auto getSliceIndices = [&](size_t len) -> vector<long long> {
					auto* s = static_cast<SliceObject*>(index.ref.get());
					long long step = (s->step.type == ValueType::INT) ? s->step.asInt() : 1;
					if (step == 0) throw ValueError("Slice step cannot be zero", idx->line, idx->col);
					long long start, end;
					if (step > 0) {
						start = (s->start.type == ValueType::INT) ? s->start.asInt() : 0;
						end = (s->end.type == ValueType::INT) ? s->end.asInt() : (long long)len;
					}
					else {
						start = (s->start.type == ValueType::INT) ? s->start.asInt() : (long long)len - 1;
						end = (s->end.type == ValueType::INT) ? s->end.asInt() : -1LL - 1LL;
					}
					auto normalizeSlice = [&](long long val) {
						return (val < 0) ? val + (long long)len : val;
					};
					if (s->start.type == ValueType::INT) start = normalizeSlice(start);
					if (s->end.type == ValueType::INT)   end = normalizeSlice(end);
					auto clamp = [&](long long val) {
						if (val < 0) return (step > 0) ? 0LL : -1LL;
						if (val > (long long)len) return (long long)len;
						return val;
					};
					if (step > 0) {
						if (start < 0) start = 0;
						if (end < 0) end = 0;
						if (start > (long long)len) start = len;
						if (end > (long long)len) end = len;
					}
					else {
						if (start < -1) start = -1;
						if (end < -1) end = -1;
						if (start > (long long)len - 1) start = len - 1;
						if (end > (long long)len - 1) end = len - 1;
					}
					vector<long long> result;
					if (step > 0) {
						for (long long i = start; i < end; i += step) {
							if (i >= 0 && i < (long long)len) result.push_back(i);
						}
					}
					else {
						for (long long i = start; i > end; i += step) {
							if (i >= 0 && i < (long long)len) result.push_back(i);
						}
					}
					return result;
				};
				if (base.type == ValueType::DICT) {
					auto* dictObj = static_cast<DictObject*>(base.ref.get());
					if (dictObj->items.find(index) == dictObj->items.end()) {
						throw KeyError("Key not found in dictionary", idx->line, idx->col);
					}
					return dictObj->items.at(index);
				}
				if (base.type == ValueType::LIST) {
					auto* list = static_cast<ListObject*>(base.ref.get());
					size_t len = list->elements.size();
					if (index.type == ValueType::INT) {
						long long i = normalize(index.asInt(), len);
						Value el = list->elements[i];
						if (base.isConst) el.isConst = true;
						return el;
					}
					else if (index.type == ValueType::SLICE) {
						auto indices = getSliceIndices(len);
						auto* newList = new ListObject();
						for (long long i : indices) {
							newList->elements.push_back(list->elements[i]);
						}
						Value ret; ret.type = ValueType::LIST; ret.ref = shared_ptr<HeapObject>(newList);
						ret.isConst = base.isConst;
						return ret;
					}
					throw TypeError("List index must be int or slice", idx->line, idx->col);
				}
				if (base.type == ValueType::STRING) {
					const string& s = base.asString();
					size_t len = s.size();
					if (index.type == ValueType::INT) {
						long long i = normalize(index.asInt(), len);
						return Value::String(string(1, s[i]));
					}
					else if (index.type == ValueType::SLICE) {
						auto indices = getSliceIndices(len);
						string newStr = "";
						for (long long i : indices) newStr += s[i];
						return Value::String(newStr);
					}
					throw TypeError("String index must be int or slice", idx->line, idx->col);
				}
				if (base.type == ValueType::TUPLE) {
					auto* t = static_cast<TupleObject*>(base.ref.get());
					size_t len = t->elements.size();
					if (index.type == ValueType::INT) {
						long long i = normalize(index.asInt(), len);
						return t->elements[i];
					}
					else if (index.type == ValueType::SLICE) {
						auto indices = getSliceIndices(len);
						auto* newTuple = new TupleObject();
						for (long long i : indices) newTuple->elements.push_back(t->elements[i]);
						Value ret; ret.type = ValueType::TUPLE; ret.ref = shared_ptr<HeapObject>(newTuple);
						ret.isConst = base.isConst;
						return ret;
					}
					throw TypeError("Tuple index must be int or slice", idx->line, idx->col);
				}
				if (base.type == ValueType::RANGE) {
					if (index.type != ValueType::INT) throw TypeError("slice index must be int", idx->line, idx->col);
					auto* r = static_cast<RangeObject*>(base.ref.get());
					long long i = index.asInt();
					double val = r->start;
					if (!r->startInclusive) val += r->step;
					val += (r->step * i);
					bool outOfBounds = false;
					if (r->step > 0) {
						if (r->endInclusive) { if (val > r->end) outOfBounds = true; }
						else { if (val >= r->end) outOfBounds = true; }
					}
					else {
						if (r->endInclusive) { if (val < r->end) outOfBounds = true; }
						else { if (val <= r->end) outOfBounds = true; }
					}
					if (outOfBounds || i < 0) throw IndexError("slice index out of bounds", idx->line, idx->col);
					return r->isFloat ? Value::Float(val) : Value::Int((long long)val);
				}
				if (base.type == ValueType::SET) {
					auto* setObj = static_cast<SetObject*>(base.ref.get());
					size_t len = setObj->elements.size();
					if (index.type == ValueType::INT) {
						long long i = normalize(index.asInt(), len);
						Value val = setObj->elements[i];
						if (base.isConst) val.isConst = true;
						return val;
					}
					else if (index.type == ValueType::SLICE) {
						auto indices = getSliceIndices(len);
						auto* newSet = new SetObject();
						for (long long i : indices) {
							newSet->elements.push_back(setObj->elements[i]);
						}
						Value ret; ret.type = ValueType::SET; ret.ref = shared_ptr<HeapObject>(newSet);
						ret.isConst = base.isConst;
						return ret;
					}
					throw TypeError("Set index must be int or slice", idx->line, idx->col);
				}
				if (base.type == ValueType::VECTOR) {
					auto* vec = static_cast<VectorObject*>(base.ref.get());
					size_t len = vec->elements.size();
					if (index.type == ValueType::INT) {
						long long i = normalize(index.asInt(), len);
						return Value::Float(vec->elements[i].asFloat());
					}
					else if (index.type == ValueType::SLICE) {
						auto indices = getSliceIndices(len);
						vector<Value> newElems;
						for (long long i : indices) {
							newElems.push_back(vec->elements[i]);
						}
						return Value::Vector(newElems);
					}
					throw TypeError("Vector index must be int or slice", idx->line, idx->col);
				}
				throw TypeError("Indexing non-indexable type", idx->line, idx->col);
			}
			case ExprType::METHOD_CALL: {
				return Resolve_methods(static_cast<MethodCallExpr*>(e));
			}
			case ExprType::LAMBDA: {
				auto lam = static_cast<LambdaExpr*>(e);
				auto* funcObj = new FunctionObject(lam->params, lam->returnType, lam->defaultRetArgs, lam->returnsConst, lam->body, env, lam->isCached);
				Value v;
				v.type = ValueType::FUNCTION;
				v.ref = std::shared_ptr<HeapObject>(funcObj);
				return v;
			}
			case ExprType::RANGE: {
				auto r = static_cast<RangeExpr*>(e);
				Value s = eval(r->start);
				Value en = eval(r->end);
				if (!s.isNumber() || !en.isNumber()) throw TypeError("Range start and end must be numbers", r->line, r->col);
				double startVal = s.asFloat();
				double endVal = en.asFloat();
				double stepVal = 1.0;
				bool isFloat = (s.type == ValueType::FLOAT || en.type == ValueType::FLOAT);
				if (r->step == nullptr) {
					if (startVal > endVal) stepVal = -1.0;
					else stepVal = 1.0;
				}
				else {
					Value st = eval(r->step);
					if (!st.isNumber()) throw TypeError("Range step must be a number", r->line, r->col);
					stepVal = st.asFloat();
					if (st.type == ValueType::FLOAT) isFloat = true;
				}
				if (stepVal == 0) throw ValueError("Range step cannot be zero", r->line, r->col);
				return Value::Range(startVal, endVal, stepVal, r->startInclusive, r->endInclusive, isFloat);
			}
			case ExprType::SET: {
				auto s = static_cast<SetExpr*>(e);
				vector<Value> vals;
				for (auto el : s->elements) setAdd(vals, eval(el));
				return Value::Set(vals);
			}
			case ExprType::DICT: {
				auto d = static_cast<DictExpr*>(e);
				std::unordered_map<Value, Value, ValueHash, ValueEqual> map;
				for (auto& item : d->items) {
					Value key = eval(item.first);
					Value val = eval(item.second);
					if (key.type == ValueType::LIST || key.type == ValueType::SET) {
						key = deepCopy(key);
						key.isConst = true;
					}
					if (key.type == ValueType::DICT) throw TypeError("Dictionary cannot be used as a key", e->line, e->col);
					map[key] = val;
				}
				return Value::Dict(map);
			}
			case ExprType::TUPLE: {
				auto t = static_cast<TupleExpr*>(e);
				vector<Value> vals;
				for (auto el : t->elements) vals.push_back(eval(el));
				return Value::Tuple(vals);
			}
			case ExprType::TERNARY: {
				auto t = static_cast<TernaryExpr*>(e);
				Value cond = eval(t->condition);
				if (cond.isTruthy()) return eval(t->trueBranch);
				else return eval(t->falseBranch);
			}
			case ExprType::COMPREHENSION: {
				auto comp = static_cast<CompExpr*>(e);
				Value collection = eval(comp->iterable);
				vector<Value> items;
				if (collection.type == ValueType::LIST) items = static_cast<ListObject*>(collection.ref.get())->elements;
				else if (collection.type == ValueType::SET) items = static_cast<SetObject*>(collection.ref.get())->elements;
				else if (collection.type == ValueType::TUPLE) items = static_cast<TupleObject*>(collection.ref.get())->elements;
				else if (collection.type == ValueType::STRING) {
					for (char c : collection.asString()) items.push_back(Value::String(string(1, c)));
				}
				else if (collection.type == ValueType::RANGE) {
					auto* r = static_cast<RangeObject*>(collection.ref.get());
					double cur = r->start;
					if (!r->startInclusive) cur += r->step;
					while (true) {
						bool cond = (r->step > 0) ? (r->endInclusive ? cur <= r->end : cur < r->end)
							: (r->endInclusive ? cur >= r->end : cur > r->end);
						if (!cond) break;
						items.push_back(r->isFloat ? Value::Float(cur) : Value::Int((long long)cur));
						cur += r->step;
					}
				}
				else if (collection.type == ValueType::DICT) {
					auto* d = static_cast<DictObject*>(collection.ref.get());
					for (auto& pair : d->items) items.push_back(pair.first);
				}
				else if (collection.type == ValueType::VECTOR) {
					auto* v = static_cast<VectorObject*>(collection.ref.get());
					for (auto d : v->elements) items.push_back(d);
				}
				else throw TypeError("Comprehension 'in' target must be iterable", comp->line, comp->col);
				vector<Value> results;
				std::unordered_map<Value, Value, ValueHash, ValueEqual> dictResults;
				bool isDict = (comp->typeToken == TokenType::LBRACE && comp->valueExpr != nullptr);
				std::shared_ptr<Env> prevEnv = env;
				for (const auto& item : items) {
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
							k = deepCopy(k); k.isConst = true;
						}
						Value v = eval(comp->valueExpr);
						dictResults[k] = v;
					}
					else {
						Value v = eval(comp->expression);
						if (comp->typeToken == TokenType::LBRACE) setAdd(results, v);
						else results.push_back(v);
					}
				}
				env = prevEnv;
				if (comp->typeToken == TokenType::LBRACKET) return Value::List(results);
				if (comp->typeToken == TokenType::LPAREN) return Value::Tuple(results);
				if (comp->typeToken == TokenType::LBRACE) {
					if (isDict) return Value::Dict(dictResults);
					else return Value::Set(results);
				}
				return Value::None();
			}
			case ExprType::SLICE: {
				auto sl = static_cast<SliceExpr*>(e);
				Value s = sl->start ? eval(sl->start) : Value::None();
				Value en = sl->end ? eval(sl->end) : Value::None();
				Value st = sl->step ? eval(sl->step) : Value::None();
				return Value::Slice(s, en, st);
			}
			case ExprType::VECTOR: {
				auto ve = static_cast<VectorExpr*>(e);
				if (!vectorEnabled) throw RuntimeError("Vector syntax <...> requires 'import Vector'", e->line, e->col);
				vector<Value> elems;
				for (auto* el : ve->elements) {
					Value v = eval(el);
					if (!v.isNumber()) throw TypeError("Vector elements must be numbers", e->line, e->col);
					elems.push_back(v);
				}
				return Value::Vector(elems);
			}
			default:{
				return Value::None();
			}
		}
	}
	Value exec(Stmt* s)	 {
		switch (s->type){
			case StmtType::ASSIGN: {
				auto as = static_cast<AssignStmt*>(s);
				if (as->target->type== ExprType::INDEX) {
					auto idx = static_cast<IndexExpr*>(as->target);
					Value base = eval(idx->base);
					if (base.type == ValueType::STRING) {
						Value rhs = eval(as->value);
						if (rhs.type != ValueType::STRING) throw TypeError("Can only insert string into string", s->line, s->col);
						int i = eval(idx->index).asInt();
						auto* strObj = static_cast<StringObject*>(base.ref.get());
						string& str = strObj->value;
						if (i < 0 || i >(int)str.size()) throw IndexError("String index out of bounds", s->line, s->col);
						if (i == (int)str.size()) str += rhs.asString();
						else {
							const string& r = rhs.asString();
							int eraseLen = std::min((int)r.size(), (int)str.size() - i);
							str.erase(i, eraseLen);
							str.insert(i, r);
						}
						return Value::None();
					}
					if (base.type == ValueType::DICT) {
						if (base.isConst) throw ConstError("Cannot modify const dictionary", s->line, s->col);
						Value key = eval(idx->index);
						Value val = eval(as->value);
						if (key.type == ValueType::LIST || key.type == ValueType::SET) {
							key = deepCopy(key);
							key.isConst = true;
						}
						if (key.type == ValueType::DICT) throw TypeError("Dictionary cannot be used as a key", s->line, s->col);
						auto* dictObj = static_cast<DictObject*>(base.ref.get());
						dictObj->items[key] = val;
						return Value::None();
					}
				}
				LValue lv = resolveLValue(as->target);
				Value& cur = *lv.ref;
				if (cur.isConst || lv.isConstView) throw ConstError("Cannot assign to or modify const variable", s->line, s->col);
				Value rhs = eval(as->value);
				if (lv.isLocked && cur.type != rhs.type) {
					throw TypeError("Cannot change type of locked variable", s->line, s->col);
				}
				if (as->op == TokenType::ASSIGN) {
					if (lv.isRefTarget) {
						*lv.ref = rhs;
						return Value::None();
					}
					if (cur.ref && cur.ref->typeLocked && cur.type != rhs.type) {
						throw TypeError("Cannot change type of locked variable", s->line, s->col);
					}
					if (cur.ref && cur.type == rhs.type) {
						if (cur.type == ValueType::STRING) static_cast<StringObject*>(cur.ref.get())->value = rhs.asString();
						else cur = rhs;
					}
					else cur = rhs;
					return Value::None();
				}
				if (cur.type == ValueType::STRING) {
					if (rhs.type == ValueType::STRING && as->op == TokenType::PLUS_EQ) {
						static_cast<StringObject*>(cur.ref.get())->value += rhs.asString();
						return Value::None();
					}
					if (rhs.type == ValueType::INT && as->op == TokenType::STAR_EQ) {
						string base = static_cast<StringObject*>(cur.ref.get())->value;
						long long count = rhs.asInt();
						if (count < 0) count = 0;
						string res = "";
						if (base.length() * count > 10000000) throw MemoryError("Resulting string too large", s->line, s->col);
						for (int k = 0; k < count; k++) res += base;
						static_cast<StringObject*>(cur.ref.get())->value = res;
						return Value::None();
					}
				}
				if (cur.type == ValueType::LIST && rhs.type == ValueType::INT && as->op == TokenType::STAR_EQ) {
					auto* listObj = static_cast<ListObject*>(cur.ref.get());
					int count = rhs.asInt();
					if (count <= 0) listObj->elements.clear();
					else if (count > 1) {
						vector<Value> orig = listObj->elements;
						if (orig.size() * count > 1000000) throw MemoryError("Resulting list too large", s->line, s->col);
						listObj->elements.reserve(orig.size() * count);
						for (int k = 1; k < count; k++) listObj->elements.insert(listObj->elements.end(), orig.begin(), orig.end());
					}
					return Value::None();
				}
				if (cur.type == ValueType::FLOAT && rhs.type == ValueType::FLOAT) {
					switch (as->op) {
					case TokenType::PLUS_EQ: cur.fVal += rhs.asFloat(); break;
					case TokenType::MINUS_EQ: cur.fVal -= rhs.asFloat(); break;
					case TokenType::STAR_EQ: cur.fVal *= rhs.asFloat(); break;
					case TokenType::DIV_EQ:
						if (rhs.asFloat() == 0) throw DivisionByZeroError("Division by zero", s->line, s->col);
						cur.fVal /= rhs.asFloat();
						break;
					case TokenType::POW_EQ: cur.fVal = std::pow(cur.fVal, rhs.asFloat()); break;
					default: throw TypeError("Invalid float operation", s->line, s->col);
					}
					return Value::None();
				}
				if (cur.type == ValueType::FLOAT && rhs.type == ValueType::INT) {
					switch (as->op) {
					case TokenType::PLUS_EQ: cur.fVal += rhs.asFloat(); break;
					case TokenType::MINUS_EQ: cur.fVal -= rhs.asFloat(); break;
					case TokenType::STAR_EQ: cur.fVal *= rhs.asFloat(); break;
					case TokenType::DIV_EQ:
						if (rhs.asFloat() == 0) throw DivisionByZeroError("Division by zero", s->line, s->col);
						cur.fVal /= rhs.asFloat();
						break;
					case TokenType::POW_EQ: cur.fVal = std::pow(cur.fVal, rhs.asFloat()); break;
					default: throw TypeError("Invalid float operation", s->line, s->col);
					}
					return Value::None();
				}
				if (cur.type == ValueType::INT && rhs.type == ValueType::INT) {
					switch (as->op) {
					case TokenType::PLUS_EQ: cur.iVal += rhs.asInt(); break;
					case TokenType::MINUS_EQ: cur.iVal -= rhs.asInt(); break;
					case TokenType::STAR_EQ: cur.iVal *= rhs.asInt(); break;
					case TokenType::DIV_EQ:
						if (rhs.asInt() == 0) throw DivisionByZeroError("Division by zero", s->line, s->col);
						cur.iVal /= rhs.asInt();
						break;
					case TokenType::MOD_EQ:
						if (rhs.asInt() == 0) throw DivisionByZeroError("Modulo by zero", s->line, s->col);
						cur.iVal %= rhs.asInt();
						break;
					case TokenType::AND_EQ: cur.iVal &= rhs.asInt(); break;
					case TokenType::OR_EQ:  cur.iVal |= rhs.asInt(); break;
					case TokenType::XOR_EQ: cur.iVal ^= rhs.asInt(); break;
					case TokenType::FLOOR_DIV_EQ:
						if (rhs.asInt() == 0) throw DivisionByZeroError("Division by zero", s->line, s->col);
						cur.iVal = (long long)std::floor((double)cur.iVal / rhs.asInt());
						break;
					case TokenType::POW_EQ:
						cur.iVal = (long long)std::pow(cur.iVal, rhs.asInt());
						break;
					default: throw TypeError("Invalid int operation", s->line, s->col);
					}
					return Value::None();
				}
				if (cur.type == ValueType::INT && rhs.type == ValueType::FLOAT) {
					switch (as->op) {
					case TokenType::PLUS_EQ: cur.iVal += rhs.asInt(); break;
					case TokenType::MINUS_EQ: cur.iVal -= rhs.asInt(); break;
					case TokenType::STAR_EQ: cur.iVal *= rhs.asInt(); break;
					case TokenType::DIV_EQ:
						if (rhs.asInt() == 0) throw DivisionByZeroError("Division by zero", s->line, s->col);
						cur.iVal /= rhs.asInt();
						break;
					case TokenType::MOD_EQ:
						if (rhs.asInt() == 0) throw DivisionByZeroError("Modulo by zero", s->line, s->col);
						cur.iVal %= rhs.asInt();
						break;
					case TokenType::AND_EQ: cur.iVal &= rhs.asInt(); break;
					case TokenType::OR_EQ:  cur.iVal |= rhs.asInt(); break;
					case TokenType::XOR_EQ: cur.iVal ^= rhs.asInt(); break;
					case TokenType::FLOOR_DIV_EQ:
						if (rhs.asInt() == 0) throw DivisionByZeroError("Division by zero", s->line, s->col);
						cur.iVal = (long long)std::floor((double)cur.iVal / rhs.asInt());
						break;
					case TokenType::POW_EQ:
						cur.iVal = (long long)std::pow(cur.iVal, rhs.asInt());
						break;
					default: throw TypeError("Invalid int operation", s->line, s->col);
					}
					return Value::None();
				}
				if (cur.type == ValueType::BOOL && rhs.type == ValueType::BOOL) {
					switch (as->op) {
					case TokenType::AND_EQ: cur.bVal = cur.bVal & rhs.asBool(); break;
					case TokenType::OR_EQ: cur.bVal = cur.bVal | rhs.asBool(); break;
					case TokenType::XOR_EQ: cur.bVal = cur.bVal ^ rhs.asBool(); break;
					default: throw TypeError("Invalid bool operation", s->line, s->col);
					}
					return Value::None();
				}
				if (cur.type == ValueType::SET && rhs.type == ValueType::SET) {
					auto* s1 = static_cast<SetObject*>(cur.ref.get());
					auto* s2 = static_cast<SetObject*>(rhs.ref.get());
					if (as->op == TokenType::OR_EQ) {
						for (auto& v : s2->elements) setAdd(s1->elements, v);
						return Value::None();
					}
					if (as->op == TokenType::AND_EQ) {
						vector<Value> keep;
						for (auto& v1 : s1->elements) {
							for (auto& v2 : s2->elements) {
								if (v1.strictEquals(v2)) { keep.push_back(v1); break; }
							}
						}
						s1->elements = keep;
						return Value::None();
					}
					if (as->op == TokenType::MINUS_EQ || as->op == TokenType::DIV_EQ) {
						vector<Value> keep;
						for (auto& v1 : s1->elements) {
							bool found = false;
							for (auto& v2 : s2->elements) if (v1.strictEquals(v2)) { found = true; break; }
							if (!found) keep.push_back(v1);
						}
						s1->elements = keep;
						return Value::None();
					}
					if (as->op == TokenType::XOR_EQ) {
						vector<Value> res;
						for (auto& v1 : s1->elements) {
							bool found = false;
							for (auto& v2 : s2->elements) if (v1.strictEquals(v2)) { found = true; break; }
							if (!found) res.push_back(v1);
						}
						for (auto& v2 : s2->elements) {
							bool found = false;
							for (auto& v1 : s1->elements) if (v2.strictEquals(v1)) { found = true; break; }
							if (!found) res.push_back(v2);
						}
						s1->elements = res;
						return Value::None();
					}
				}
				if (cur.type == ValueType::VECTOR) {
					auto* v = static_cast<VectorObject*>(cur.ref.get());
					if (as->op == TokenType::PLUS_EQ) {
						if (rhs.type != ValueType::VECTOR) throw TypeError("+= requires a vector", s->line, s->col);
						auto* v2 = static_cast<VectorObject*>(rhs.ref.get());
						if (v->elements.size() != v2->elements.size()) throw ValueError("Dimension mismatch", s->line, s->col);
						for (size_t i = 0; i < v->elements.size(); i++); //v->elements[i].asFloat() += v2->elements[i].asFloat();
						return Value::None();
					}
					if (as->op == TokenType::MINUS_EQ) {
						if (rhs.type != ValueType::VECTOR) throw TypeError("-= requires a vector", s->line, s->col);
						auto* v2 = static_cast<VectorObject*>(rhs.ref.get());
						if (v->elements.size() != v2->elements.size()) throw ValueError("Dimension mismatch", s->line, s->col);
						for (size_t i = 0; i < v->elements.size(); i++); //v->elements[i].asFloat() -= v2->elements[i].asFloat();
						return Value::None();
					}
					if (as->op == TokenType::STAR_EQ) {
						if (rhs.isNumber()) {
							double scalar = rhs.asFloat();
							for (auto& val : v->elements); //val.asFloat() *= scalar;
						}
						else if (rhs.type == ValueType::VECTOR) {
							auto* v2 = static_cast<VectorObject*>(rhs.ref.get());
							if (v->elements.size() != v2->elements.size()) throw ValueError("Dimension mismatch", s->line, s->col);
							for (size_t i = 0; i < v->elements.size(); i++); //v->elements[i].asFloat() *= v2->elements[i].asFloat();
						}
						else throw TypeError("*= requires number or vector", s->line, s->col);
						return Value::None();
					}
					if (as->op == TokenType::DIV_EQ) throw TypeError("/= not supported for vectors", s->line, s->col);
				}
				throw TypeError("Invalid operands for compound assignment", s->line, s->col);
			}
			case StmtType::LET: {
				auto let = static_cast<LetStmt*>(s);
				if (env->existsLocal(let->name)) {
					throw RuntimeError("Variable '" + let->name + "' is already defined in this scope.", s->line, s->col);
				}
				if (env->exists(let->name)) {
					Var& existing = env->lookup(let->name);
					if (existing.isConst) {
						throw ConstError("Cannot shadow constant '" + let->name + "'.", s->line, s->col);
					}
				}
				if (!let->value) {
					env->set(let->name, Value::NoType(), false);
					return Value::None();
				}
				if (let->value->type==ExprType::OWNERSHIP) {
					auto o = static_cast<OwnershipExpr*>(let->value);
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
				if (let->isConst) v.isConst = true;
				bool lock = false;
				if (auto call = dynamic_cast<CallExpr*>(let->value)) {
					if (call->name == "int" || call->name == "float" || call->name == "bool" || call->name == "tuple" ||
						call->name == "string" || call->name == "list" || call->name == "range" || call->name == "set") {
						lock = true;
					}
				}
				env->set(let->name, v, let->isLocked, let->isConst);
				return Value::None();
			}
			case StmtType::RETURN: {
				auto ret = static_cast<ReturnStmt*>(s);
				returning = true;
				returnValue = (ret->value != nullptr) ? eval(ret->value) : Value::NoType();
				return returnValue;
			}
			case StmtType::FUNC: {
				auto f = static_cast<FuncStmt*>(s);
				auto* func = new FunctionObject(f->params,f->returnType,f->defaultRetArgs,f->returnsConst,f->body,env,f->isCached);
				Value funcVal;
				funcVal.type = ValueType::FUNCTION;
				funcVal.ref = std::shared_ptr<HeapObject>(func);
				if (env->existsLocal(f->name)) {
					Var& existing = env->lookup(f->name);
					if (existing.value.type == ValueType::FUNCTION) {
						Value group = Value::Overload(existing.value);
						static_cast<OverloadObject*>(group.ref.get())->overloads.push_back(funcVal);
						existing.value = group;
					}
					else if (existing.value.type == ValueType::OVERLOAD) {
						static_cast<OverloadObject*>(existing.value.ref.get())->overloads.push_back(funcVal);
					}
					else {
						Var v; v.value = funcVal; v.isConst = f->returnsConst;
						env->vars[f->name] = v;
					}
				}
				else{
					Var v; v.value = funcVal;
					v.isConst = f->returnsConst; env->vars[f->name] = v;
				}
				return Value::None();
			}
			case StmtType::IF: {
				auto ifs = static_cast<IfStmt*>(s);
				if (eval(ifs->condition).isTruthy()) {
					for (auto st : ifs->body) {
						exec(st);
						if (returning || breaking || continuing) break;
					}
					return returnValue;
				}
				for (auto& [cond, block] : ifs->elifs) {
					if (eval(cond).isTruthy()) {
						for (auto st : block) {
							exec(st);
							if (returning || breaking || continuing) break;
						}
						return returnValue;
					}
				}
				for (auto st : ifs->elseBody) {
					exec(st);
					if (returning || breaking || continuing) break;
				}
				return returnValue;
			}
			case StmtType::EXPR: {
				auto es = static_cast<ExprStmt*>(s);
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
				auto sk = static_cast<SkipStmt*>(s);
				skipCount = eval(sk->count).asInt();
				return Value::None();
			}
			case StmtType::WHILE: {
				auto w = static_cast<WhileStmt*>(s);
				std::shared_ptr<Env> loopEnv = std::make_shared<Env>();
				loopEnv->parent = env;
				std::shared_ptr<Env> previousGlobal = env;
				if (canOptimizeScope(w->body)) {
					env = loopEnv;
					while (eval(w->condition).isTruthy()) {
						for (auto st : w->body) {
							exec(st);
							if (breaking || continuing || returning) break;
						}
						if (breaking) { breaking = false; break; }
						if (continuing) { continuing = false; }
						if (returning) break;
					}
					env = previousGlobal;
				}
				else {
					while (eval(w->condition).isTruthy()) {
						env = loopEnv;
						for (auto st : w->body) {
							exec(st);
							if (breaking || continuing || returning) break;
						}
						env = previousGlobal;
						if (breaking) { breaking = false; break; }
						if (continuing) { continuing = false; }
						if (returning) break;
						loopEnv = std::make_shared<Env>();
						loopEnv->parent = previousGlobal;
					}
				}
				return Value::None();
			}
			case StmtType::DO_WHILE: {
				auto d = static_cast<DoWhileStmt*>(s);
				do {
					std::shared_ptr<Env> prevEnv = env;
					env = std::make_shared<Env>();
					env->parent = prevEnv;
					for (auto st : d->body) {
						exec(st);
						if (breaking || continuing || returning) break;
					}
					env = prevEnv;
					if (breaking) { breaking = false; return Value::None(); }
					if (continuing) { continuing = false;}
					if (returning) return returnValue;
				} while (eval(d->condition).isTruthy());
				return Value::None();
			}
			case StmtType::FOR: {
				auto f = static_cast<ForStmt*>(s);
				std::shared_ptr<Env> loopEnv = std::make_shared<Env>();
				loopEnv->parent = env;
				std::shared_ptr<Env> previousGlobal = env;
				env = loopEnv;
				for (auto* st : f->inits) exec(st);
				bool staticScope = true;
				for (auto* st : f->body) {
					if (st->type == StmtType::LET || st->type == StmtType::MULTI_LET ||
						st->type == StmtType::FUNC || st->type == StmtType::IMPORT) {
						staticScope = false; break;
					}
				}
				Value* fastCounter = nullptr;
				if (f->steps.size() == 1 && f->steps[0]->type == StmtType::ASSIGN) {
					auto as = static_cast<AssignStmt*>(f->steps[0]);
					try {
						LValue lv = resolveLValue(as->target);
						if (lv.ref->type == ValueType::INT || lv.ref->type == ValueType::FLOAT) fastCounter = lv.ref;
					}
					catch (...) {}
				}
				double fastLimit = 0;
				TokenType fastOp = TokenType::LET;
				bool useFastCondition = false;
				if (fastCounter && f->condition->type == ExprType::BINARY) {
					auto be = static_cast<BinExpr*>(f->condition);
					if (be->left->type == ExprType::VAR && be->right->type == ExprType::NUMBER) {
						auto ve = static_cast<VarExpr*>(be->left);
						if (static_cast<AssignStmt*>(f->steps[0])->target->type == ExprType::VAR) {
							auto stepVar = static_cast<VarExpr*>(static_cast<AssignStmt*>(f->steps[0])->target);
							if (stepVar->name == ve->name) {
								Value limitVal = eval(be->right);
								fastLimit = limitVal.asFloat();
								fastOp = be->op;
								useFastCondition = true;
							}
						}
					}
				}
				Value* fastBodyTarget = nullptr;
				Value* fastBodyOperand = nullptr;
				bool fastBodyIsNumber = false;
				double fastBodyLiteral = 0;
				TokenType fastBodyOp = TokenType::LET;
				bool useFastBody = false;
				if (staticScope && f->body.size() == 1 && f->body[0]->type == StmtType::ASSIGN) {
					auto as = static_cast<AssignStmt*>(f->body[0]);
					if (as->op == TokenType::PLUS_EQ || as->op == TokenType::MINUS_EQ ||
						as->op == TokenType::STAR_EQ || as->op == TokenType::DIV_EQ) {
						try {
							LValue lv = resolveLValue(as->target);
							if (lv.ref->type == ValueType::INT || lv.ref->type == ValueType::FLOAT) {
								fastBodyTarget = lv.ref;
								fastBodyOp = as->op;
								if (as->value->type == ExprType::VAR) {
									LValue opLv = resolveLValue(as->value);
									if (opLv.ref->type == ValueType::INT || opLv.ref->type == ValueType::FLOAT) {
										fastBodyOperand = opLv.ref;
										if (fastBodyTarget->type == fastBodyOperand->type) {
											useFastBody = true;
										}
									}
								}
								else if (as->value->type == ExprType::NUMBER) {
									fastBodyLiteral = static_cast<NumberExpr*>(as->value)->val;
									fastBodyIsNumber = true;
									useFastBody = true;
								}
							}
						}
						catch (...) {}
					}
				}
				if (staticScope) {
					while (true) {
						if (useFastCondition) {
							double current = (fastCounter->type == ValueType::INT) ? (double)fastCounter->iVal : fastCounter->fVal;
							bool cond = false;
							switch (fastOp) {
							case TokenType::LT: cond = current < fastLimit; break;
							case TokenType::LTE: cond = current <= fastLimit; break;
							case TokenType::GT: cond = current > fastLimit; break;
							case TokenType::GTE: cond = current >= fastLimit; break;
							default: cond = eval(f->condition).isTruthy(); break;
							}
							if (!cond) break;
						}
						else {
							if (!eval(f->condition).isTruthy()) break;
						}
						if (useFastBody) {
							double operand = 0;
							if (fastBodyIsNumber) operand = fastBodyLiteral;
							else operand = (fastBodyOperand->type == ValueType::INT) ? (double)fastBodyOperand->iVal : fastBodyOperand->fVal;
							if (fastBodyTarget->type == ValueType::INT) {
								long long opInt = (long long)operand;
								switch (fastBodyOp) {
								case TokenType::PLUS_EQ: fastBodyTarget->iVal += opInt; break;
								case TokenType::MINUS_EQ: fastBodyTarget->iVal -= opInt; break;
								case TokenType::STAR_EQ: fastBodyTarget->iVal *= opInt; break;
								case TokenType::DIV_EQ: if (opInt != 0) fastBodyTarget->iVal /= opInt; break;
								}
							}
							else {
								switch (fastBodyOp) {
								case TokenType::PLUS_EQ: fastBodyTarget->fVal += operand; break;
								case TokenType::MINUS_EQ: fastBodyTarget->fVal -= operand; break;
								case TokenType::STAR_EQ: fastBodyTarget->fVal *= operand; break;
								case TokenType::DIV_EQ: if (operand != 0) fastBodyTarget->fVal /= operand; break;
								}
							}
						}
						else {
							for (auto st : f->body) {
								exec(st);
								if (breaking || continuing || returning) break;
							}
							if (breaking) { breaking = false; break; }
							if (continuing) { continuing = false; }
							if (returning) break;
						}
						if (fastCounter) {
							if (fastCounter->type == ValueType::INT) fastCounter->iVal += 1;
							else fastCounter->fVal += 1.0;
						}
						else {
							for (auto* st : f->steps) exec(st);
						}
					}
				}
				else {
					env = previousGlobal;
					while (eval(f->condition).isTruthy()) {
						env = loopEnv;
						for (auto st : f->body) exec(st);
						env = previousGlobal;
						if (breaking) { breaking = false; break; }
						if (continuing) { continuing = false; }
						if (returning) break;
						env = loopEnv;
						for (auto* st : f->steps) exec(st);
						env = previousGlobal;
					}
				}
				env = previousGlobal;
				if (returning) return returnValue;
				return Value::None();
			}
			case StmtType::FOR_EACH: {
				auto fe = static_cast<ForEachStmt*>(s);
				vector<vector<Value>> allStreams;
				size_t minLen = (size_t)-1;
				bool dictUnpack = fe->collections.size() == 1 && fe->loopVars.size() == 2;
				for (size_t i = 0; i < fe->collections.size(); i++) {
					Value collection = eval(fe->collections[i]);
					vector<Value> currentStream;
					vector<Value> secondaryStream;
					if (collection.type == ValueType::DICT) {
						auto* dict = static_cast<DictObject*>(collection.ref.get());
						if (dictUnpack) {
							for (auto& pair : dict->items) {
								currentStream.push_back(pair.first);
								secondaryStream.push_back(pair.second);
							}
						}
						else {
							for (auto& pair : dict->items) currentStream.push_back(pair.first);
						}
					}
					else if (collection.type == ValueType::LIST) {
						auto* list = static_cast<ListObject*>(collection.ref.get());
						for (auto& v : list->elements) currentStream.push_back(v);
					}
					else if (collection.type == ValueType::SET) {
						auto* set = static_cast<SetObject*>(collection.ref.get());
						for (auto& v : set->elements) currentStream.push_back(v);
					}
					else if (collection.type == ValueType::TUPLE) {
						auto* tuple = static_cast<TupleObject*>(collection.ref.get());
						for (auto& v : tuple->elements) currentStream.push_back(v);
					}
					else if (collection.type == ValueType::STRING) {
						string str = collection.asString();
						for (char c : str) currentStream.push_back(Value::String(string(1, c)));
					}
					else if (collection.type == ValueType::RANGE) {
						auto* r = static_cast<RangeObject*>(collection.ref.get());
						double cur = r->start;
						if (!r->startInclusive) cur += r->step;
						while (true) {
							bool keep = (r->step > 0) ? (r->endInclusive ? cur <= r->end : cur < r->end)
								: (r->endInclusive ? cur >= r->end : cur > r->end);
							if (!keep) break;
							currentStream.push_back(r->isFloat ? Value::Float(cur) : Value::Int((long long)cur));
							cur += r->step;
						}
					}
					else if (collection.type == ValueType::VECTOR) {
						auto* v = static_cast<VectorObject*>(collection.ref.get());
						for (auto d : v->elements) currentStream.push_back(d);
					}
					else {
						throw TypeError("Object is not iterable", s->line, s->col);
					}
					if (currentStream.size() < minLen) minLen = currentStream.size();
					allStreams.push_back(currentStream);
					if (dictUnpack && !secondaryStream.empty()) {
						if (secondaryStream.size() < minLen) minLen = secondaryStream.size();
						allStreams.push_back(secondaryStream);
					}
				}
				if (minLen == (size_t)-1) minLen = 0;
				if (allStreams.size() != fe->loopVars.size()) {
					throw RuntimeError("Mismatch between loop variables and collections", s->line, s->col);
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
						if (breaking || returning || continuing) break;
					}
					env = prevEnv;
					if (breaking) { breaking = false; break; }
					if (returning) break;
					if (continuing) continuing = false;
				}
				if (returning) return returnValue;
				return Value::None();
			}
			case StmtType::TRY: {
				auto t = static_cast<TryStmt*>(s);
				bool errorCaught = false;
				try {
					for (auto st : t->tryBlock) {
						exec(st);
						if (returning || breaking || continuing) break;
					}
				}
				catch (const LangError& e) {
					if (dynamic_cast<const ControlFlowError*>(&e)) {
						if (!t->finallyBlock.empty()) {
							bool r = returning; Value rv = returnValue;
							returning = false;
							try { for (auto st : t->finallyBlock) exec(st); }
							catch (...) { throw; }
							if (!returning && !breaking && !continuing) { returning = r; returnValue = rv; }
						}
						throw;
					}
					errorCaught = true;
					bool handled = false;
					for (auto& catchBlock : t->catches) {
						for (auto& typeName : catchBlock.types) {
							if (matchesError(e, typeName)) {
								try {
									for (auto st : catchBlock.body) {
										exec(st);
										if (returning || breaking || continuing) break;
									}
								}
								catch (...) {
									if (!t->finallyBlock.empty()) for (auto st : t->finallyBlock) exec(st);
									throw;
								}
								handled = true;
								goto after_catches;
							}
						}
					}
				after_catches:
					if (!handled) {
						if (!t->finallyBlock.empty()) for (auto st : t->finallyBlock) exec(st);
						throw;
					}
				}
				if (!errorCaught && !returning && !breaking && !continuing && !t->elseBlock.empty()) {
					for (auto st : t->elseBlock) {
						exec(st);
						if (returning || breaking || continuing) break;
					}
				}
				if (!t->finallyBlock.empty()) {
					bool oldRet = returning; Value oldVal = returnValue;
					bool oldBrk = breaking; bool oldCont = continuing;
					returning = false; breaking = false; continuing = false;
					try {
						for (auto st : t->finallyBlock) {
							exec(st);
							if (returning || breaking || continuing) break;
						}
					}
					catch (...) { throw; }
					if (!returning && !breaking && !continuing) {
						returning = oldRet; returnValue = oldVal;
						breaking = oldBrk; continuing = oldCont;
					}
				}
				return Value::None();
			}
			case StmtType::THROW: {
				auto ts = static_cast<ThrowStmt*>(s);
				Value msgVal = eval(ts->message);
				throw LangError(ts->errorType, valueToString(msgVal), -1000000, s->line, s->col);
			}
			case StmtType::ASSERT: {
				auto a = static_cast<AssertStmt*>(s);
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
				auto sw = dynamic_cast<SwitchStmt*>(s);
				while (true) {
					Value targetVal = eval(sw->target);
					bool matchFound = false;
					for (auto& c : sw->cases) {
						Value caseVal = eval(c.value);
						if (targetVal.strictEquals(caseVal)) {
							matchFound = true;
							for (auto st : c.body) {
								exec(st);
								if (returning) return returnValue;
								if (continuing) goto exit_switch;
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
							if (returning) return returnValue;
							if (continuing) goto exit_switch;
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
				auto imp = static_cast<ImportStmt*>(s);
				if (imp->libName.size() > 4 && imp->libName.substr(imp->libName.size() - 4) == ".ymm") {
					fs::path p(imp->libName);
					if (!fs::exists(p)) {
						throw InvalidImportError("Module file not found: " + imp->libName, s->line, s->col);
					}
					std::error_code ec;
					string absPath = fs::absolute(p, ec).string();
					if (importStack.count(absPath)) {
						throw CircularImportError("Circular import detected involving: " + imp->libName, s->line, s->col);
					}
					std::ifstream file(absPath);
					if (!file) throw InvalidImportError("Unable to read module: " + imp->libName, s->line, s->col);
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
						vector<Stmt*> moduleStmts;
						while (!parser.isAtEnd()) moduleStmts.push_back(parser.parseStmt());
						for (Stmt* stmt : moduleStmts) {
							if (!isGlobalAllowed(stmt)) throw RuntimeError("Module code must only contain definitions (let, func, import).", stmt->line, stmt->col);
						}
						for (Stmt* ms : moduleStmts) exec(ms);
					}
					catch (...) {
						this->env = previousEnv;
						importStack.erase(absPath);
						throw;
					}
					this->env = previousEnv;
					importStack.erase(absPath);
					if (imp->symbols.empty()) {
						for (auto& [name, var] : moduleEnv->vars) {
							if (name == "None") continue;
							this->env->vars[name] = var;
						}
					}
					else {
						for (const string& sym : imp->symbols) {
							if (moduleEnv->vars.count(sym)) {
								this->env->vars[sym] = moduleEnv->vars[sym];
							}
							else {
								throw AttributeError("Module '" + imp->libName + "' does not export '" + sym + "'", s->line, s->col);
							}
						}
					}
					return Value::None();
				}
				if (modules.find(imp->libName) == modules.end()) {
					if (imp->libName.find('.') != string::npos) {
						throw InvalidImportError("Import filenames must end in .ymm", s->line, s->col);
					}
					throw ImportError("Unknown module '" + imp->libName + "'", s->line, s->col);
				}
				modules[imp->libName](this->env, imp->symbols);
				return Value::None();
			}
			case StmtType::MULTI_LET: {
				auto ml = static_cast<MultiLetStmt*>(s);
				vector<Value> results;
				for (auto* expr : ml->values) results.push_back(expr ? eval(expr) : Value::NoType());
				for (size_t i = 0; i < ml->names.size(); i++) {
					if (env->existsLocal(ml->names[i])) {
						throw RuntimeError("Variable '" + ml->names[i] + "' is already defined in this scope.", s->line, s->col);
					}
					Value v = results[i];
					if (ml->isConsts[i]) v.isConst = true;
					bool lock = false;
					env->set(ml->names[i], v, ml->isLocked, ml->isConsts[i]);
				}
				return Value::None();
			}
			case StmtType::MULTI_ASSIGN: {
				auto ma = static_cast<MultiAssignStmt*>(s);
				vector<LValue> lvals;
				for (auto* t : ma->targets) lvals.push_back(resolveLValue(t));
				vector<Value> rvals;
				for (auto* v : ma->values) rvals.push_back(eval(v));
				for (size_t i = 0; i < lvals.size(); i++) {
					Value& cur = *lvals[i].ref;
					if (cur.isConst || lvals[i].isConstView) throw ConstError("Cannot assign to const variable", s->line, s->col);
					if (cur.ref && cur.ref->typeLocked && cur.type != rvals[i].type) {
						throw TypeError("Cannot change type of locked variable", s->line, s->col);
					}
					if (lvals[i].isRefTarget) *lvals[i].ref = rvals[i];
					else if (cur.ref && cur.type == rvals[i].type) {
						if (cur.type == ValueType::STRING) static_cast<StringObject*>(cur.ref.get())->value = rvals[i].asString();
						else cur = rvals[i];
					}
					else cur = rvals[i];
				}
				return Value::None();
			}
			default: {
				return Value::None();
			}
		}
		return Value::None();
	}
	Value call(Value callee, const vector<CallArg>& args, const vector<CopyMode>& modes, int line = 0, int col = 0) {
		auto error = [&](const string& msg, const string& type = "RuntimeError") {
			if (type == "TypeError") throw TypeError(msg, line, col);
			if (type == "ValueError") throw ValueError(msg, line, col);
			if (type == "ArgumentError") throw ArgumentError(msg, line, col);
			if (type == "NameError") throw NameError(msg, line, col);
			throw RuntimeError(msg, line, col);
		};
		// ------------------ RESOLVE CALLEE OBJECT ------------------
		FunctionObject* lambdaObj = nullptr;
		NativeFunctionObject* nativeObj = nullptr;
		if (callee.type == ValueType::FUNCTION) {
			lambdaObj = static_cast<FunctionObject*>(callee.ref.get());
		}
		else if (callee.type == ValueType::NATIVE_FUNCTION) {
			nativeObj = static_cast<NativeFunctionObject*>(callee.ref.get());
		}
		else if (callee.type == ValueType::OVERLOAD) {
			auto* ov = static_cast<OverloadObject*>(callee.ref.get());
			bool found = false;
			for (int i = ov->overloads.size() - 1; i >= 0; i--) {
				Value candVal = ov->overloads[i];
				if (candVal.type != ValueType::FUNCTION) continue;
				auto* candidate = static_cast<FunctionObject*>(candVal.ref.get());
				size_t minArgs = 0;
				bool isVariadic = false;
				for (const auto& p : candidate->params) {
					if (p.isVariadic || p.isKwargs) isVariadic = true;
					else if (p.defaultValue == nullptr) minArgs++;
				}
				if (args.size() < minArgs && !isVariadic) continue;
				if (!isVariadic && args.size() > candidate->params.size()) continue;
				bool typesMatch = true;
				size_t paramIdx = 0;
				for (size_t argIdx = 0; argIdx < args.size(); argIdx++) {
					if (paramIdx >= candidate->params.size()) {
						if (!isVariadic) { typesMatch = false; break; }
						break;
					}
					const ParamSpec& p = candidate->params[paramIdx];
					if (p.isVariadic || p.isKwargs) break;
					Value argVal = args[argIdx].value;
					if (p.type != ValueType::NOTYPE && argVal.type != p.type) {
						if (!(p.type == ValueType::FLOAT && argVal.type == ValueType::INT)) {
							typesMatch = false; break;
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
			if (!found) error("No matching overload found with provided arguments", "TypeError");
		}
		else {
			error("Object is not callable", "TypeError");
		}
		if (nativeObj) {
			vector<Value> argValues;
			for (const auto& ca : args) argValues.push_back(ca.value);
			return nativeObj->func(argValues, line, col);
		}
		if (!lambdaObj) error("Critical: Lambda Object is null", "RuntimeError");
		vector<Value> cacheKey;
		if (lambdaObj->isCached) {
			for (const auto& arg : args) cacheKey.push_back(arg.value);
			if (lambdaObj->cache.count(cacheKey)) return lambdaObj->cache[cacheKey];
		}
		std::shared_ptr<Env> previous = env;
		std::shared_ptr<Env> newEnv = std::make_shared<Env>();
		newEnv->parent = lambdaObj->closure ? lambdaObj->closure : previous;
		env = newEnv;
		size_t argIndex = 0;
		for (size_t i = 0; i < lambdaObj->params.size(); i++) {
			const ParamSpec& p = lambdaObj->params[i];
			Value argVal = Value::None();
			if (p.isKwargs) {
				std::unordered_map<Value, Value, ValueHash, ValueEqual> dictItems;
				while (argIndex < args.size()) {
					Value v = args[argIndex].value;
					if (v.type != ValueType::PAIRED) throw RuntimeError("Positional argument found after keyword arguments.", line, col);
					auto* pairObj = static_cast<PairedObject*>(v.ref.get());
					for (const auto& entry : pairObj->pairs) {
						Value key = entry.first;
						Value val = entry.second;
						if (p.mode == CopyMode::DEEP) {
							if (key.type == ValueType::LIST || key.type == ValueType::DICT) key = deepCopy(key);
							val = deepCopy(val);
						}
						dictItems[key] = val;
					}
					argIndex++;
				}
				Value dictVal = Value::Dict(dictItems, p.isConst);
				env->set(p.name, dictVal, false, p.isConst);
			}
			else if (p.isVariadic) {
				vector<Value> tupleItems;
				while (argIndex < args.size()) {
					if (args[argIndex].value.type == ValueType::PAIRED) break;
					Value v = args[argIndex].value;
					if (p.mode == CopyMode::DEEP) v = deepCopy(v);
					tupleItems.push_back(v);
					argIndex++;
				}
				env->set(p.name, Value::Tuple(tupleItems), false, true);
			}
			else {
				bool foundValue = false;
				if (argIndex < args.size() && args[argIndex].value.type != ValueType::PAIRED) {
					if (args[argIndex].value.type == ValueType::OMIT_MARKER) {
						if (p.defaultValue != nullptr) {
							argVal = eval(p.defaultValue);
							foundValue = true;
						}
						else {
							env = previous;
							error("Argument '" + p.name + "' cannot be omitted (no default value).", "ArgumentError");
						}
					}
					else {
						argVal = args[argIndex].value;
						if (p.mode == CopyMode::DEEP) argVal = deepCopy(argVal);
						foundValue = true;
					}
					argIndex++;
				}
				if (!foundValue) {
					if (p.defaultValue != nullptr) argVal = eval(p.defaultValue);
					else {env = previous; error("Missing required argument '" + p.name + "'", "ArgumentError"); }
				}
				if (p.type != ValueType::NOTYPE && argVal.type != p.type) {
					if (p.type == ValueType::FLOAT && argVal.type == ValueType::INT) argVal = Value::Float((double)argVal.asInt());
					else {env = previous; error("Type mismatch for '" + p.name + "'", "TypeError"); }
				}
				env->set(p.name, argVal, false, p.isConst);
			}
		}
		bool prevReturning = returning;
		Value prevReturnValue = returnValue;
		returning = false;
		for (auto stmt : lambdaObj->body) {
			exec(stmt);
			if (returning) break;
		}
		bool didReturn = returning;
		Value ret = returnValue;
		returning = prevReturning;
		returnValue = prevReturnValue;
		env = previous;
		auto generateDefault = [&]() -> Value {
			vector<CallArg> defArgs;
			vector<CopyMode> defModes;
			for (auto* e : lambdaObj->defaultRetArgs) {
				defArgs.push_back({ eval(e) });
				defModes.push_back(CopyMode::SHALLOW);
			}
			string typeName = "";
			switch (lambdaObj->returnType) {
			case ValueType::INT: typeName = "int"; break;
			case ValueType::FLOAT: typeName = "float"; break;
			case ValueType::STRING: typeName = "string"; break;
			case ValueType::BOOL: typeName = "bool"; break;
			case ValueType::LIST: typeName = "list"; break;
			case ValueType::DICT: typeName = "dict"; break;
			case ValueType::SET: typeName = "set"; break;
			case ValueType::TUPLE: typeName = "tuple"; break;
			case ValueType::RANGE: typeName = "range"; break;
			case ValueType::VECTOR: typeName = "vector"; break;
			default: break;
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
				if (!lambdaObj->defaultRetArgs.empty()) finalResult = generateDefault();
				else if (lambdaObj->returnType != ValueType::NOTYPE) finalResult = defaultOf(lambdaObj->returnType);
				else finalResult = Value::NoType();
			}
			else if (lambdaObj->returnType != ValueType::NOTYPE && ret.type != lambdaObj->returnType) {
				if (lambdaObj->returnType == ValueType::FLOAT && ret.type == ValueType::INT) 
					finalResult = Value::Float((double)ret.asInt());
				else if (lambdaObj->returnType == ValueType::INT && ret.type == ValueType::FLOAT) 
					finalResult = Value::Int((long long)ret.fVal);
				else error("Return type mismatch", "TypeError");
			}
			else finalResult = ret;
		}
		else {
			if (!lambdaObj->defaultRetArgs.empty()) finalResult = generateDefault();
			else finalResult = (lambdaObj->returnType != ValueType::NOTYPE) ? defaultOf(lambdaObj->returnType) : Value::NoType();
		}
		if (lambdaObj->returnsConst) finalResult.isConst = true;
		if (lambdaObj->isCached) lambdaObj->cache[cacheKey] = finalResult;
		return finalResult;
	}
	void interpret(const vector<Stmt*>& statements) {
		try {
			for (Stmt* stmt : statements) {
				if (!isGlobalAllowed(stmt)) throw RuntimeError("Executable code must be inside a 'main' function.", stmt->line, stmt->col);
			}

			for (Stmt* stmt : statements) exec(stmt);
			if (env->exists("main")) {
				Value mainVal = env->get("main");
				if (mainVal.type != ValueType::FUNCTION) throw RuntimeError("'main' is defined but is not a function.", 0, 0);
				auto* func = static_cast<FunctionObject*>(mainVal.ref.get());
				if (func->params.size() > 0) {
					throw RuntimeError("'main' function cannot accept arguments.", 0, 0);
				}
				std::shared_ptr<Env> globalEnv = this->env;
				this->env = std::make_shared<Env>();
				this->env->parent = func->closure;
				try {
					// Execute Main Body
					for (Stmt* bodyStmt : func->body) {
						exec(bodyStmt);
						if (returning) { returning = false; break; }
					}
				}
				catch (...) {
					this->env = globalEnv;
					throw;
				}
				this->env = globalEnv;
			}
			else throw RuntimeError("No entry point found. Define a 'main' function.", 0, 0);
		}
		catch (...) {throw;}
	}
private:
	bool canOptimizeScope(const vector<Stmt*>& body) {
		for (auto* st : body) {
			if (st->type == StmtType::LET || st->type == StmtType::MULTI_LET ||
				st->type == StmtType::FUNC || st->type == StmtType::IMPORT ||
				st->type == StmtType::MULTI_ASSIGN) {
				return false;
			}
		}
		return true;
	}
	bool isGlobalAllowed(Stmt* s) {
		if (dynamic_cast<LetStmt*>(s)) return true;
		if (dynamic_cast<FuncStmt*>(s)) return true;
		if (dynamic_cast<ImportStmt*>(s)) return true;
		if (dynamic_cast<MultiLetStmt*>(s)) return true;
		if (dynamic_cast<MultiAssignStmt*>(s)) return true;
		return false;
	}
};
// ------------ BYTECODE VM ------------
enum class OpCode : uint8_t {
	// Literals & Constants
	OP_CONSTANT, OP_TRUE, OP_FALSE, OP_NONE, OP_NOTYPE,
	// Variables & Scope
	OP_DEFINE_VAR, OP_GET_VAR, OP_SET_VAR, OP_DEEP_COPY, OP_REF_LOCAL,
	OP_DEFINE_REF, OP_REF_VAR, OP_REF_INDEX, OP_SET_REF, OP_SHALLOW_COPY,
	OP_MULTI_SET, OP_GET_LOCAL,OP_SET_LOCAL, OP_INC_LOCAL, OP_SET_FLAGS,
	// Arithmetic & Logic
	OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_FLOOR_DIV, OP_MOD, OP_POW,
	OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LTE, OP_GTE, OP_COLON, OP_STRICT_NEQ,
	OP_NOT, OP_AND, OP_OR, OP_XOR, OP_IS, OP_IN, OP_IS_NOT, OP_STRICT_EQ,
	OP_IS_IN, OP_IS_NOT_IN, OP_NXOR, OP_NAND, OP_NOR, OP_NEGATE, OP_INCREMENT, OP_DECREMENT,
	// Containers
	OP_BUILD_LIST, OP_BUILD_TUPLE, OP_BUILD_SET, OP_BUILD_DICT, OP_UNPACK_DICT,
	OP_BUILD_RANGE, OP_BUILD_VECTOR, OP_BUILD_FSTRING, OP_BUILD_FILE, OP_BUILD_SLICE,
	// Comprehension
	OP_LIST_APPEND, OP_SET_ADD, OP_DICT_SET, OP_LIST_TO_TUPLE, OP_LIST_TO_VECTOR,
	// Access & Calls
	OP_GET_INDEX, OP_SET_INDEX, OP_INVOKE, OP_CALL,
	// Control Flow
	OP_JUMP, OP_JUMP_IF_FALSE, OP_LOOP, OP_RETURN, OP_TO_STREAM, OP_JUMP_IF_NOT_LT,
	OP_BREAK, OP_CONTINUE, OP_SKIP, OP_OMIT, OP_FOR_ITER, OP_SKIP_ITER, OP_SWITCH_TABLE,
	// Errors & Systems
	OP_THROW, OP_ASSERT, OP_IMPORT, OP_POP, OP_DEBUG_NAME, OP_TRY_ENTER, OP_TRY_EXIT,
	OP_CATCH, OP_RETHROW, OP_END_FINALLY
};
static inline std::string OpCodeToString(OpCode num){
	switch (num){
		case OpCode::OP_CONSTANT: return "OP_CONSTANT";
		case OpCode::OP_TRUE: return "OP_TRUE";
		case OpCode::OP_FALSE: return "OP_FALSE";
		case OpCode::OP_NONE: return "OP_NONE";
		case OpCode::OP_NOTYPE: return "OP_NOTYPE";
		case OpCode::OP_DEFINE_VAR: return "OP_DEFINE_VAR";
		case OpCode::OP_GET_VAR: return "OP_GET_VAR";
		case OpCode::OP_SET_VAR: return "OP_SET_VAR";
		case OpCode::OP_DEEP_COPY: return "OP_DEEP_COPY";
		case OpCode::OP_REF_LOCAL: return "OP_REF_LOCAL";
		case OpCode::OP_DEFINE_REF: return "OP_DEFINE_REF";
		case OpCode::OP_REF_VAR: return "OP_REF_VAR";
		case OpCode::OP_REF_INDEX: return "OP_REF_INDEX";
		case OpCode::OP_SET_REF: return "OP_SET_REF";
		case OpCode::OP_SHALLOW_COPY: return "OP_SHALLOW_COPY";
		case OpCode::OP_MULTI_SET: return "OP_MULTI_LET";
		case OpCode::OP_GET_LOCAL: return "OP_GET_LOCAL";
		case OpCode::OP_SET_LOCAL: return "OP_SET_LOCAL";
		case OpCode::OP_INC_LOCAL: return "OP_INC_LOCAL";
		case OpCode::OP_SET_FLAGS: return "OP_SET_FLAGS";
		case OpCode::OP_ADD: return "OP_ADD";
		case OpCode::OP_SUB: return "OP_SUB";
		case OpCode::OP_MUL: return "OP_MUL";
		case OpCode::OP_DIV: return "OP_DIV";
		case OpCode::OP_FLOOR_DIV: return "OP_FLOOR_DIV";
		case OpCode::OP_MOD: return "OP_MOD";
		case OpCode::OP_POW: return "OP_POW";
		case OpCode::OP_EQ: return "OP_EQ";
		case OpCode::OP_NEQ: return "OP_NEQ";
		case OpCode::OP_LT: return "OP_LT";
		case OpCode::OP_GT: return "OP_GT";
		case OpCode::OP_LTE: return "OP_LTE";
		case OpCode::OP_GTE: return "OP_GTE";
		case OpCode::OP_COLON: return "OP_COLON";
		case OpCode::OP_STRICT_NEQ: return "OP_STRICT_NEQ";
		case OpCode::OP_NOT: return "OP_NOT";
		case OpCode::OP_AND: return "OP_AND";
		case OpCode::OP_OR: return "OP_OR";
		case OpCode::OP_XOR: return "OP_XOR";
		case OpCode::OP_IS: return "OP_IS";
		case OpCode::OP_IN: return "OP_IN";
		case OpCode::OP_IS_NOT: return "OP_IS_NOT";
		case OpCode::OP_STRICT_EQ: return "OP_STRICT_EQ";
		case OpCode::OP_IS_IN: return "OP_IS_IN";
		case OpCode::OP_IS_NOT_IN: return "OP_IS_NOT_IN";
		case OpCode::OP_NXOR: return "OP_NXOR";
		case OpCode::OP_NAND: return "OP_NAND";
		case OpCode::OP_NOR: return "OP_NOR";
		case OpCode::OP_NEGATE: return "OP_NEGATE";
		case OpCode::OP_INCREMENT: return "OP_INCREMENT";
		case OpCode::OP_DECREMENT: return "OP_DECREMENT";
		case OpCode::OP_BUILD_LIST: return "OP_BUILD_LIST";
		case OpCode::OP_BUILD_TUPLE: return "OP_BUILD_TUPLE";
		case OpCode::OP_BUILD_SET: return "OP_BUILD_SET";
		case OpCode::OP_BUILD_DICT: return "OP_BUILD_DICT";
		case OpCode::OP_UNPACK_DICT: return "OP_UNPACK_DICT";
		case OpCode::OP_BUILD_RANGE: return "OP_BUILD_RANGE";
		case OpCode::OP_BUILD_VECTOR: return "OP_BUILD_VECTOR";
		case OpCode::OP_BUILD_FSTRING: return "OP_BUILD_FSTRING";
		case OpCode::OP_BUILD_FILE: return "OP_BUILD_FILE";
		case OpCode::OP_BUILD_SLICE: return "OP_BUILD_SLICE";
		case OpCode::OP_LIST_APPEND: return "OP_LIST_APPEND";
		case OpCode::OP_SET_ADD: return "OP_SET_ADD";
		case OpCode::OP_DICT_SET: return "OP_DICT_SET";
		case OpCode::OP_LIST_TO_TUPLE: return "OP_LIST_TO_TUPLE";
		case OpCode::OP_LIST_TO_VECTOR: return "OP_LIST_TO_VECTOR";
		case OpCode::OP_GET_INDEX: return "OP_GET_INDEX";
		case OpCode::OP_SET_INDEX: return "OP_SET_INDEX";
		case OpCode::OP_INVOKE: return "OP_INVOKE";
		case OpCode::OP_CALL: return "OP_CALL";
		case OpCode::OP_JUMP: return "OP_JUMP";
		case OpCode::OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
		case OpCode::OP_LOOP: return "OP_LOOP";
		case OpCode::OP_RETURN: return "OP_RETURN";
		case OpCode::OP_TO_STREAM: return "OP_TO_STREAM";
		case OpCode::OP_JUMP_IF_NOT_LT: return "OP_JUMP_IF_NOT_LT";
		case OpCode::OP_BREAK: return "OP_BREAK";
		case OpCode::OP_CONTINUE: return "OP_CONTINUE";
		case OpCode::OP_SKIP: return "OP_SKIP";
		case OpCode::OP_OMIT: return "OP_OMIT";
		case OpCode::OP_FOR_ITER: return "OP_FOR_ITER";
		case OpCode::OP_SKIP_ITER: return "OP_SKIP_ITER";
		case OpCode::OP_SWITCH_TABLE: return "OP_SWITCH_TABLE";
		case OpCode::OP_THROW: return "OP_THROW";
		case OpCode::OP_ASSERT: return "OP_ASSET";
		case OpCode::OP_IMPORT: return "OP_IMPORT";
		case OpCode::OP_POP: return "OP_POP";
		case OpCode::OP_DEBUG_NAME: return "OP_DEBUG_NAME";
		case OpCode::OP_TRY_ENTER: return "OP_TRY_ENTER";
		case OpCode::OP_TRY_EXIT: return "OP_TRY_EXIT";
		case OpCode::OP_CATCH: return "OP_CATCH";
		case OpCode::OP_RETHROW: return "OP_RETHROW";
		case OpCode::OP_END_FINALLY: return "OP_END_FINALLY";
		default: return "UNKNOWN";
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
};
struct ExceptionHandler {
	int catchAddress;
	int finallyAddress;
	int stackDepth;
	int scopeDepth;
	bool isInsideFinally = false;
};
struct CallFrame {
	FunctionObject* function;
	uint8_t* ip;
	int basePointer;
	vector<Value> cacheKey;
	vector<ExceptionHandler> handlerStack;
};
struct ByteCodeCompiler {
	Chunk* chunk;
	vector<LoopContext> loopStack;
	vector<Local> locals;
	int scopeDepth = 0;
	ByteCodeCompiler(Chunk* c) : chunk(c) {}
	int resolveLocal(string name) {
		for (int i = (int)locals.size() - 1; i >= 0; i--) if (locals[i].name == name) return i;
		return -1;
	}
	void addLocal(string name) {
		locals.push_back({ name, scopeDepth });
	}
	void beginScope() { scopeDepth++; }
	void endScope(int line, int col) {
		scopeDepth--;
		while (!locals.empty() && locals.back().depth > scopeDepth) {emitByte(OpCode::OP_POP, line, col);locals.pop_back();}
	}
	void compile(Expr* e) {
		if (!e) return;
		switch (e->type) {
		case ExprType::COMPREHENSION: {
			auto comp = static_cast<CompExpr*>(e);
			beginScope();
			bool isDict = (comp->typeToken == TokenType::LBRACE && comp->valueExpr != nullptr);
			bool isSet = (comp->typeToken == TokenType::LBRACE && !isDict);
			bool isTuple = (comp->typeToken == TokenType::LPAREN);
			bool isVector = (comp->typeToken == TokenType::LT);
			if (isDict) {
				emitByte(OpCode::OP_BUILD_DICT, comp->line, comp->col);
				chunk->write(0, comp->line, comp->col);
			}
			else if (isSet) {
				emitByte(OpCode::OP_BUILD_SET, comp->line, comp->col);
				chunk->write(0, comp->line, comp->col);
			}
			else {
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
				filterJump = emitJump(OpCode::OP_JUMP_IF_FALSE, comp->line, comp->col);
				emitByte(OpCode::OP_POP, comp->line, comp->col);
			}
			if (isDict) {
				compile(comp->expression);
				compile(comp->valueExpr);
				emitByte(OpCode::OP_DICT_SET, comp->line, comp->col);
				chunk->write((uint8_t)containerSlot, comp->line, comp->col);
			}
			else if (isSet) {
				compile(comp->expression);
				emitByte(OpCode::OP_SET_ADD, comp->line, comp->col);
				chunk->write((uint8_t)containerSlot, comp->line, comp->col);
			}
			else {
				compile(comp->expression);
				emitByte(OpCode::OP_LIST_APPEND, comp->line, comp->col);
				chunk->write((uint8_t)containerSlot, comp->line, comp->col);
			}
			if (filterJump != -1) {
				int skipAppend = emitJump(OpCode::OP_JUMP, comp->line, comp->col);
				patchJump(filterJump);
				emitByte(OpCode::OP_POP, comp->line, comp->col);
				patchJump(skipAppend);
			}
			endScope(comp->line, comp->col);
			emitLoop(startAddr, comp->line, comp->col);
			patchJump(exitJump);
			emitByte(OpCode::OP_POP, comp->line, comp->col);
			emitByte(OpCode::OP_POP, comp->line, comp->col);
			emitByte(OpCode::OP_GET_LOCAL, comp->line, comp->col);
			chunk->write((uint8_t)containerSlot, comp->line, comp->col);
			if (isTuple) emitByte(OpCode::OP_LIST_TO_TUPLE, comp->line, comp->col);
			else if (isVector) emitByte(OpCode::OP_LIST_TO_VECTOR, comp->line, comp->col);
			locals.pop_back();
			locals.pop_back();
			locals.pop_back();
			scopeDepth--;
			break;
		}
		case ExprType::OMIT_MARKER_EXPR: {
			auto o = static_cast<OmitExpr*>(e);
			Value val = Value::Omit();
			emitConstant(val,o->line,o->col);
			break;
		}
		case ExprType::LAMBDA: {
			auto lam = static_cast<LambdaExpr*>(e);
			Chunk* funcChunk = new Chunk();
			ByteCodeCompiler subCompiler(funcChunk);
			subCompiler.beginScope();
			for (const auto& param : lam->params) subCompiler.addLocal(param.name);
			for (auto* stmt : lam->body) subCompiler.compileStmt(stmt);
			subCompiler.emitByte(OpCode::OP_NOTYPE, lam->line, 0);
			subCompiler.emitByte(OpCode::OP_RETURN, lam->line, 0);
			auto* funcObj = new FunctionObject(lam->params, lam->returnType, lam->defaultRetArgs, lam->returnsConst, lam->body, nullptr, lam->isCached, funcChunk);
			Value funcVal;
			funcVal.type = ValueType::FUNCTION;
			funcVal.ref = std::shared_ptr<HeapObject>(funcObj);
			emitConstant(funcVal, lam->line, 0);
			break;
		}
		case ExprType::NUMBER: {
			auto n = static_cast<NumberExpr*>(e);
				Value val = n->isFloat ? Value::Float(n->val) : Value::Int((long long)n->val);
				emitConstant(val, n->line,  n->col);
			break;
		}
		case ExprType::BOOL: {
			auto b = static_cast<BoolExpr*>(e);
				emitByte(b->value ? OpCode::OP_TRUE : OpCode::OP_FALSE, b->line, b->col);
			break;
		}
		case ExprType::STRING: {
			auto s = static_cast<StringExpr*>(e);
				emitConstant(Value::String(s->val), s->line, s->col);
				break;
		}
		case ExprType::FSTRING: {
			auto fs = static_cast<FStringExpr*>(e);
			for (auto* part : fs->parts) compile(part);
			if (fs->parts.size() > 255) throw SyntaxError("F-String has too many parts (limit 255)", fs->line, fs->col);
			emitByte(OpCode::OP_BUILD_FSTRING, fs->line, fs->col);
			chunk->write((uint8_t)fs->parts.size(), fs->line, fs->col);
			break;
		}
		case ExprType::BINARY: {
			compileBinary(static_cast<BinExpr*>(e));
				break;
		}
		case ExprType::VAR: {
			auto v = static_cast<VarExpr*>(e);
			if (v->name == "None") {
				emitByte(OpCode::OP_NONE, v->line, v->col);
				break;
			}
			int arg = resolveLocal(v->name);
			if (arg != -1) {
				emitByte(OpCode::OP_GET_LOCAL, v->line, v->col);
				chunk->write((uint8_t)arg, v->line, v->col);
			}
			else emitIdentifier(OpCode::OP_GET_VAR, v->name, v->line, v->col);
			break;
		}
		case ExprType::OWNERSHIP: {
			auto o = static_cast<OwnershipExpr*>(e);
			if (o->mode == CopyMode::DEEP) {
				compile(o->expr);
				emitByte(OpCode::OP_DEEP_COPY, o->line, o->col);
			}
			else if (o->mode == CopyMode::REF) {
				if (auto v = dynamic_cast<VarExpr*>(o->expr)) {
					int local = resolveLocal(v->name);
					if (local != -1) {
						emitByte(OpCode::OP_REF_LOCAL, o->line, o->col); // New OpCode
						chunk->write((uint8_t)local, o->line, o->col);
					}
					else {
						emitIdentifier(OpCode::OP_REF_VAR, v->name, o->line, o->col); // New usage
					}
				}
				else if (auto idx = dynamic_cast<IndexExpr*>(o->expr)) {
					compile(idx->base);
					compile(idx->index);
					emitByte(OpCode::OP_REF_INDEX, o->line, o->col);
				}
				else {
					throw OwnershipError("Cannot take reference of this expression", o->line, o->col);
				}
			}
			break;
		}
		case ExprType::INDEX: {
			auto idx = static_cast<IndexExpr*>(e);
			compile(idx->base);
			compile(idx->index);
			emitByte(OpCode::OP_GET_INDEX, idx->line, idx->col);
			break;
		}
		case ExprType::SLICE: {
			auto s = static_cast<SliceExpr*>(e);
			if (s->start) compile(s->start);
			else emitByte(OpCode::OP_NONE, s->line, s->col);
			if (s->end) compile(s->end);
			else emitByte(OpCode::OP_NONE, s->line, s->col);
			if (s->step) compile(s->step);
			else emitByte(OpCode::OP_NONE, s->line, s->col);
			emitByte(OpCode::OP_BUILD_SLICE, s->line, s->col);
			break;
		}
		case ExprType::CALL: {
			auto c = static_cast<CallExpr*>(e);
			for (auto arg : c->args) compile(arg);
			int arg = resolveLocal(c->name);
			if (arg != -1) {
				emitByte(OpCode::OP_GET_LOCAL, c->line, c->col);
				chunk->write((uint8_t)arg, c->line, c->col);
			}
			else emitIdentifier(OpCode::OP_GET_VAR, c->name, c->line, c->col);
			emitByte(OpCode::OP_CALL, c->line, c->col);
			chunk->write(static_cast<uint8_t>(c->args.size()), c->line, c->col);
			break;
		}
		case ExprType::TERNARY: {
			auto t = static_cast<TernaryExpr*>(e);
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
			auto l = static_cast<ListExpr*>(e);
			for (auto item : l->elements) compile(item);
			emitByte(OpCode::OP_BUILD_LIST, l->line, l->col);
			chunk->write((uint8_t)l->elements.size(), l->line, l->col);
			break;
		}
		case ExprType::SET: {
			auto s = static_cast<SetExpr*>(e);
			for (auto item : s->elements) compile(item);
			emitByte(OpCode::OP_BUILD_SET, s->line, s->col);
			chunk->write((uint8_t)s->elements.size(), s->line, s->col);
			break;
		}
		case ExprType::TUPLE: {
			auto t = static_cast<TupleExpr*>(e);
			for (auto item : t->elements) compile(item);
			emitByte(OpCode::OP_BUILD_TUPLE, t->line, t->col);
			chunk->write((uint8_t)t->elements.size(), t->line, t->col);
			break;
		}
		case ExprType::DICT: {
			auto d = static_cast<DictExpr*>(e);
			for (auto& item : d->items) {
				compile(item.first);
				compile(item.second);
				emitByte(OpCode::OP_COLON, d->line, d->col);
			}
			emitByte(OpCode::OP_BUILD_DICT, d->line, d->col);
			chunk->write((uint8_t)d->items.size(), d->line, d->col);
			break;
		}
		case ExprType::RANGE: {
			auto r = static_cast<RangeExpr*>(e);
			compile(r->start);
			compile(r->end);
			if (r->step) compile(r->step);
			else emitConstant(Value::Int(1), r->line, r->col);
			emitByte(OpCode::OP_BUILD_RANGE, r->line, r->col);
			break;
		}
		case ExprType::VECTOR: {
			auto ve = static_cast<VectorExpr*>(e);
			for (auto* el : ve->elements) compile(el);
			emitByte(OpCode::OP_BUILD_VECTOR, ve->line, ve->col);
			chunk->write((uint8_t)ve->elements.size(), ve->line, ve->col);
			break;
		}
		case ExprType::METHOD_CALL: {
			auto m = static_cast<MethodCallExpr*>(e);
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
	void compileBinary(BinExpr* b) {
		if (b->op == TokenType::NOT) {
			compile(b->right);
			emitByte(OpCode::OP_NOT, b->line, b->col);
			return; // Exit early!
		}
		compile(b->left);
		compile(b->right);
		switch (b->op) {
		case TokenType::PLUS:      emitByte(OpCode::OP_ADD, b->line, b->col); break;
		case TokenType::MINUS:     emitByte(OpCode::OP_SUB, b->line, b->col); break;
		case TokenType::STAR:      emitByte(OpCode::OP_MUL, b->line, b->col); break;
		case TokenType::SLASH:     emitByte(OpCode::OP_DIV, b->line, b->col); break;
		case TokenType::FLOOR_DIV: emitByte(OpCode::OP_FLOOR_DIV, b->line, b->col); break;
		case TokenType::MOD:       emitByte(OpCode::OP_MOD, b->line, b->col); break;
		case TokenType::POW:       emitByte(OpCode::OP_POW, b->line, b->col); break;
			//Logic
		case TokenType::GT:    emitByte(OpCode::OP_GT, b->line, b->col); break;
		case TokenType::GTE:   emitByte(OpCode::OP_GTE, b->line, b->col); break;
		case TokenType::EQ:    emitByte(OpCode::OP_EQ, b->line, b->col); break;
		case TokenType::NEQ:   emitByte(OpCode::OP_NEQ, b->line, b->col); break;
		case TokenType::STRICT_EQ:    emitByte(OpCode::OP_STRICT_EQ, b->line, b->col); break;
		case TokenType::STRICT_NEQ:   emitByte(OpCode::OP_STRICT_NEQ, b->line, b->col); break;
		case TokenType::LT:    emitByte(OpCode::OP_LT, b->line, b->col); break;
		case TokenType::LTE:   emitByte(OpCode::OP_LTE, b->line, b->col); break;
			// Identity & Membership
		case TokenType::IS:        emitByte(OpCode::OP_IS, b->line, b->col); break;
		case TokenType::IS_NOT:    emitByte(OpCode::OP_IS_NOT, b->line, b->col); break;
		case TokenType::IS_IN:     emitByte(OpCode::OP_IS_IN, b->line, b->col); break;
		case TokenType::IS_NOT_IN: emitByte(OpCode::OP_IS_NOT_IN, b->line, b->col); break;
			// Logical (non-short-circuiting)
		case TokenType::XOR:  emitByte(OpCode::OP_XOR, b->line, b->col); break;
		case TokenType::NXOR: emitByte(OpCode::OP_NXOR, b->line, b->col); break;
		case TokenType::NOR:  emitByte(OpCode::OP_NOR, b->line, b->col); break;
		case TokenType::NAND: emitByte(OpCode::OP_NAND, b->line, b->col); break;
			//"pairing" op
		case TokenType::COLON: emitByte(OpCode::OP_COLON, b->line, b->col); break;
		default: break;
		}
	}
	void compileLogical(BinExpr* l) {
		if (l->op == TokenType::AND) {
			compile(l->left);
			int endJump = emitJump(OpCode::OP_JUMP_IF_FALSE, l->line, l->col);
			emitByte(OpCode::OP_POP, l->line, l->col); // Pop left if it was true
			compile(l->right);
			patchJump(endJump);
		}
		else if (l->op == TokenType::OR) {
			compile(l->left);
			int elseJump = emitJump(OpCode::OP_JUMP_IF_FALSE, l->line, l->col);
			int endJump = emitJump(OpCode::OP_JUMP, l->line, l->col);
			patchJump(elseJump);
			emitByte(OpCode::OP_POP, l->line, l->col); // Pop left if it was false
			compile(l->right);
			patchJump(endJump);
		}
	}
	void emitByte(OpCode op, int line, int col) {
		chunk->write(op, line, col);
	}
	void emitConstant(Value v, int line, int col) {
		int index = chunk->addConstant(v);
		if (index > 255) throw MemoryError("Too many constants in one chunk", line, col);
		emitByte(OpCode::OP_CONSTANT, line, col);
		chunk->write(static_cast<uint8_t>(index), line, col);
	}
	int emitJump(OpCode instruction, int line, int col) {
		emitByte(instruction, line, col);
		chunk->write(0xff, line, col);
		chunk->write(0xff, line, col);
		return (int)chunk->code.size() - 2;
	}
	void patchJump(int offset) {
		int jump = (int)chunk->code.size() - offset - 2;
		if (jump > 65535) throw RangeError("Too much code to jump over!", 0, 0);
		chunk->code[offset] = (jump >> 8) & 0xff;
		chunk->code[offset + 1] = jump & 0xff;
	}
	void compileWithMode(Expr* expr, int line, int col) {
		if (auto o = dynamic_cast<OwnershipExpr*>(expr)) {
			if (o->mode == CopyMode::DEEP) {
				compile(o->expr);
				emitByte(OpCode::OP_DEEP_COPY, line, col);
			}
			else if (o->mode == CopyMode::REF) compile(expr);
		}
		else {
			compile(expr);
			if (expr->type != ExprType::LIST && expr->type != ExprType::DICT &&
				expr->type != ExprType::SET && expr->type != ExprType::VECTOR &&
				expr->type != ExprType::LAMBDA && expr->type != ExprType::NUMBER &&
				expr->type != ExprType::STRING && expr->type != ExprType::BOOL)
				emitByte(OpCode::OP_SHALLOW_COPY, line, col);
		}
	}
	void compileStmt(Stmt* s) {
		if (!s) return;
		switch (s->type) {
			case StmtType::EXPR: {
				auto es = static_cast<ExprStmt*>(s);
				compile(es->expr);
				emitByte(OpCode::OP_POP, es->line, es->col);
				break;
			}
			case StmtType::ASSERT: {
				auto as = static_cast<AssertStmt*>(s);
				compile(as->condition);
				int failJump = emitJump(OpCode::OP_JUMP_IF_FALSE, as->line, 0);
				emitByte(OpCode::OP_POP, as->line, 0);
				int successJump = emitJump(OpCode::OP_JUMP, as->line, 0);
				patchJump(failJump);
				emitByte(OpCode::OP_POP, as->line, 0);
				if (as->message) compile(as->message);
				else {
					std::string msg = "Assertion failed on line " + std::to_string(as->line);
					emitConstant(Value::String(msg), as->line, 0);
				}
				emitConstant(Value::String("AssertionError"), as->line, 0);
				emitByte(OpCode::OP_THROW, as->line, 0);
				patchJump(successJump);
				break;
			}
			case StmtType::THROW: {
				auto ts = static_cast<ThrowStmt*>(s);
				compile(ts->message);
				emitConstant(Value::String(ts->errorType), ts->line, 0);
				emitByte(OpCode::OP_THROW, ts->line, 0);
				break;
			}
			case StmtType::TRY: {
				auto t = static_cast<TryStmt*>(s);
				emitByte(OpCode::OP_TRY_ENTER, t->line, 0);
				int tryEnterAddr = (int)chunk->code.size();
				chunk->write(0xff, t->line, 0); chunk->write(0xff, t->line, 0);
				chunk->write(0xff, t->line, 0); chunk->write(0xff, t->line, 0);
				for (auto st : t->tryBlock) compileStmt(st);
				emitByte(OpCode::OP_TRY_EXIT, t->line, 0);
				int skipCatchJump = emitJump(OpCode::OP_JUMP, t->line, 0);
				int catchStartAddr = (int)chunk->code.size();
				int catchOffset = catchStartAddr - tryEnterAddr;
				chunk->code[tryEnterAddr] = (catchOffset >> 8) & 0xff;
				chunk->code[tryEnterAddr + 1] = catchOffset & 0xff;
				vector<int> exitJumps;
				for (auto& catchBlock : t->catches) {
					emitByte(OpCode::OP_CATCH, t->line, 0);
					chunk->write((uint8_t)catchBlock.types.size(), t->line, 0);
					for (const auto& typeName : catchBlock.types) {
						int idx = chunk->addConstant(Value::String(typeName));
						chunk->write((uint8_t)idx, t->line, 0);
					}
					int nextCatchJump = emitJump(OpCode::OP_JUMP_IF_FALSE, t->line, 0);
					emitByte(OpCode::OP_POP, t->line, 0);
					emitByte(OpCode::OP_POP, t->line, 0);
					beginScope();
					for (auto st : catchBlock.body) compileStmt(st);
					endScope(t->line, 0);
					exitJumps.push_back(emitJump(OpCode::OP_JUMP, t->line, 0));
					patchJump(nextCatchJump);
					emitByte(OpCode::OP_POP, t->line, 0);
				}
				emitByte(OpCode::OP_RETHROW, t->line, 0);
				patchJump(skipCatchJump);
				for (int j : exitJumps) patchJump(j);
				int finallyStartAddr = (int)chunk->code.size();
				if (!t->finallyBlock.empty()) {
					int finallyOffset = finallyStartAddr - tryEnterAddr;
					chunk->code[tryEnterAddr + 2] = (finallyOffset >> 8) & 0xff;
					chunk->code[tryEnterAddr + 3] = finallyOffset & 0xff;
					beginScope();
					for (auto st : t->finallyBlock) compileStmt(st);
					endScope(t->line, 0);
					emitByte(OpCode::OP_END_FINALLY, t->line, 0);
				}
				else {
					chunk->code[tryEnterAddr + 2] = 0;
					chunk->code[tryEnterAddr + 3] = 0;
				}
				break;
			}
			case StmtType::IMPORT: {
				auto imp = static_cast<ImportStmt*>(s);
				emitConstant(Value::String(imp->libName), imp->line, 0);
				for (const auto& sym : imp->symbols) emitConstant(Value::String(sym), imp->line, 0);
				emitByte(OpCode::OP_IMPORT, imp->line, 0);
				chunk->write((uint8_t)imp->symbols.size(), imp->line, 0);
				if (imp->symbols.empty()) {
					if (scopeDepth > 0) addLocal(imp->libName);
					emitIdentifier(OpCode::OP_DEFINE_VAR, imp->libName, imp->line, 0);
					chunk->write(0, imp->line, 0);
				}
				else {
					for (const auto& sym : imp->symbols) {
						if (scopeDepth > 0) addLocal(sym);
						emitIdentifier(OpCode::OP_DEFINE_VAR, sym, imp->line, 0);
						chunk->write(0, imp->line, 0);
					}
				}
				break;
			}
			case StmtType::LET: {
				auto let = static_cast<LetStmt*>(s);
				if (scopeDepth == 0 && let->value && let->value->type == ExprType::OWNERSHIP) {
					auto o = static_cast<OwnershipExpr*>(let->value);
					if (o->mode == CopyMode::REF) {
						emitIdentifier(OpCode::OP_DEFINE_REF, let->name, let->line, let->col);
						if (auto v = dynamic_cast<VarExpr*>(o->expr)) emitIdentifier(OpCode::OP_REF_VAR, v->name, let->line, let->col);
						else if (auto idx = dynamic_cast<IndexExpr*>(o->expr)) {
							compile(idx->base);
							compile(idx->index);
							emitByte(OpCode::OP_REF_INDEX, let->line, let->col);
						}
						break;
					}
				}
				if (let->value) {
					compile(let->value);
					if (let->value->type != ExprType::OWNERSHIP && let->value->type != ExprType::LIST &&
						let->value->type != ExprType::DICT && let->value->type != ExprType::SET &&
						let->value->type != ExprType::TUPLE && let->value->type != ExprType::VECTOR &&
						let->value->type != ExprType::LAMBDA && let->value->type != ExprType::NUMBER &&
						let->value->type != ExprType::STRING && let->value->type != ExprType::BOOL) {
						emitByte(OpCode::OP_SHALLOW_COPY, let->line, let->col);
					}
				}
				else emitByte(OpCode::OP_NOTYPE, let->line, let->col);
				if (scopeDepth>0){
					if (let->isConst || let->isLocked) {
						emitByte(OpCode::OP_SET_FLAGS, let->line, let->col);
						uint8_t flags = 0;
						if (let->isConst) flags |= 0x01;
						if (let->isLocked) flags |= 0x02;
						chunk->write(flags, let->line, let->col);
					}
					addLocal(let->name);
					if (DEBUGGER_MODE_IS_ENABLED) emitIdentifier(OpCode::OP_DEBUG_NAME, let->name, let->line, let->col);
				}
				else {
					emitIdentifier(OpCode::OP_DEFINE_VAR, let->name, let->line, let->col);
					uint8_t flags = 0;
					if (let->isConst) flags |= 0x01;
					if (let->isLocked) flags |= 0x02;
					chunk->write(flags, let->line, let->col);
				}
				break;
			}
			case StmtType::ASSIGN: {
				auto as = static_cast<AssignStmt*>(s);
				if (auto idx = dynamic_cast<IndexExpr*>(as->target)) {
					if (as->op != TokenType::ASSIGN) {
						// Support for a[0] += 1 is skipped for now
						throw RuntimeError("Augmented assignment on index not supported yet", as->line, as->col);
					}
					compile(idx->base);
					compile(idx->index);
					compile(as->value);
					emitByte(OpCode::OP_SET_INDEX, as->line, as->col);
					emitByte(OpCode::OP_POP, as->line, as->col);
					break;
				}
				auto v = static_cast<VarExpr*>(as->target);
				if (auto o = dynamic_cast<OwnershipExpr*>(as->value)) {
					if (scopeDepth == 0 && o->mode == CopyMode::REF) {
						emitIdentifier(OpCode::OP_SET_REF, v->name, as->line, as->col);
						if (auto targetVar = dynamic_cast<VarExpr*>(o->expr)) {
							emitIdentifier(OpCode::OP_REF_VAR, targetVar->name, as->line, as->col);
						}
						else if (auto idx = dynamic_cast<IndexExpr*>(o->expr)) {
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
						if (auto num = dynamic_cast<NumberExpr*>(as->value)) {
							if (num->val == 1) {
								emitByte(OpCode::OP_INC_LOCAL, as->line, as->col);
								chunk->write((uint8_t)arg, as->line, as->col);
								break;
							}
						}
					}
				}
				if (as->op == TokenType::ASSIGN) compileWithMode(as->value, as->line, as->col);
				else {
					if (arg != -1) {
						emitByte(OpCode::OP_GET_LOCAL, as->line, as->col);
						chunk->write((uint8_t)arg, as->line, as->col);
					}
					else emitIdentifier(OpCode::OP_GET_VAR, v->name, as->line, as->col);
					compile(as->value);
					switch (as->op) {
						case TokenType::PLUS_EQ: emitByte(OpCode::OP_ADD, as->line, as->col); break;
						case TokenType::MINUS_EQ: emitByte(OpCode::OP_SUB, as->line, as->col); break;
						case TokenType::STAR_EQ: emitByte(OpCode::OP_MUL, as->line, as->col); break;
						case TokenType::DIV_EQ: emitByte(OpCode::OP_DIV, as->line, as->col); break;
						case TokenType::FLOOR_DIV_EQ: emitByte(OpCode::OP_FLOOR_DIV, as->line, as->col); break;
						case TokenType::POW_EQ: emitByte(OpCode::OP_POW, as->line, as->col); break;
						case TokenType::MOD_EQ: emitByte(OpCode::OP_MOD, as->line, as->col); break;
					}
				}
				if (arg != -1) {
					emitByte(OpCode::OP_SET_LOCAL, as->line, as->col);
					chunk->write((uint8_t)arg, as->line, as->col);
				}
				else emitIdentifier(OpCode::OP_SET_VAR, v->name, as->line, as->col);
				emitByte(OpCode::OP_POP, as->line, as->col);
				break;
			}
			case StmtType::IF: {
				auto ifs = static_cast<IfStmt*>(s);
				vector<int> exitJumps;
				compile(ifs->condition);
				int jumpToNext = emitJump(OpCode::OP_JUMP_IF_FALSE, ifs->line, ifs->col);
				emitByte(OpCode::OP_POP, ifs->line, ifs->col);
				beginScope();
				for (auto stmt : ifs->body) compileStmt(stmt);
				endScope(ifs->line, ifs->col);
				exitJumps.push_back(emitJump(OpCode::OP_JUMP, ifs->line, ifs->col));
				patchJump(jumpToNext);
				emitByte(OpCode::OP_POP, ifs->line, ifs->col);
				for (const auto& elif : ifs->elifs) {
					compile(elif.first);
					jumpToNext = emitJump(OpCode::OP_JUMP_IF_FALSE, ifs->line, ifs->col);
					emitByte(OpCode::OP_POP, ifs->line, ifs->col);
					beginScope();
					for (auto stmt : elif.second) compileStmt(stmt);
					endScope(ifs->line, ifs->col);
					exitJumps.push_back(emitJump(OpCode::OP_JUMP, ifs->line, ifs->col));
					patchJump(jumpToNext);
					emitByte(OpCode::OP_POP, ifs->line, ifs->col);
				}
				if (!ifs->elseBody.empty()) {
					beginScope();
					for (auto stmt : ifs->elseBody) compileStmt(stmt);
					endScope(ifs->line, ifs->col);
				}
				for (int offset : exitJumps) {
					patchJump(offset);
				}
				break;
			}
			case StmtType::SWITCH: {
				auto sw = static_cast<SwitchStmt*>(s);
				int startAddr = (int)chunk->code.size();
				beginScope();
				compile(sw->target);
				addLocal("");
				LoopContext switchLoop = { startAddr, -1, {}, {}, false, (int)locals.size(), -1 };
				loopStack.push_back(switchLoop);
				bool useTable = true;
				vector<long long> tableKeys;
				for (const auto& c : sw->cases) {
					if (c.value->type == ExprType::NUMBER) {
						auto num = static_cast<NumberExpr*>(c.value);
						if (!num->isFloat) tableKeys.push_back((long long)num->val);
						else { useTable = false; break; }
					}
					else { useTable = false; break; }
				}
				if (useTable && !tableKeys.empty()) {
					auto minVal = *std::min_element(tableKeys.begin(), tableKeys.end());
					auto maxVal = *std::max_element(tableKeys.begin(), tableKeys.end());
					if ((maxVal - minVal) > 256) useTable = false;
				}
				else useTable = false;
				// JUMP TABLE (Fast)
				if (useTable && !tableKeys.empty()) {
					long long minVal = *std::min_element(tableKeys.begin(), tableKeys.end());
					long long maxVal = *std::max_element(tableKeys.begin(), tableKeys.end());
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
					auto compileBodyWithLoop = [&](const vector<Stmt*>& stmts) {
						for (auto stmt : stmts) compileStmt(stmt);
						emitByte(OpCode::OP_POP, sw->line, sw->col);
						emitLoop(startAddr, sw->line, sw->col);
					};
					int defaultAddr = (int)chunk->code.size();
					compileBodyWithLoop(sw->defaultBody);
					std::map<long long, int> caseAddresses;
					for (size_t i = 0; i < sw->cases.size(); i++) {
						caseAddresses[(long long)((NumberExpr*)sw->cases[i].value)->val] = (int)chunk->code.size();
						compileBodyWithLoop(sw->cases[i].body);
					}
					int currentPos = tableStart;
					for (long long i = 0; i < count; i++) {
						long long val = minVal + i;
						int target = caseAddresses.count(val) ? caseAddresses[val] : defaultAddr;
						int offset = target - currentPos - 2;
						chunk->code[currentPos] = (offset >> 8) & 0xff;
						chunk->code[currentPos + 1] = offset & 0xff;
						currentPos += 2;
					}
				}
				// LINEAR SCAN (Compatible)
				else {
					vector<int> nextCaseJumps;
					for (const auto& c : sw->cases) {
						emitByte(OpCode::OP_GET_LOCAL, sw->line, sw->col);
						chunk->write((uint8_t)(locals.size() - 1), sw->line, sw->col);
						compile(c.value);
						emitByte(OpCode::OP_STRICT_EQ, sw->line, sw->col);
						int nextJump = emitJump(OpCode::OP_JUMP_IF_FALSE, sw->line, sw->col);
						emitByte(OpCode::OP_POP, sw->line, sw->col);
						for (auto stmt : c.body) compileStmt(stmt);
						emitByte(OpCode::OP_POP, sw->line, sw->col);
						emitLoop(startAddr, sw->line, sw->col);
						patchJump(nextJump);
						emitByte(OpCode::OP_POP, sw->line, sw->col);
					}
					for (auto stmt : sw->defaultBody) compileStmt(stmt);
					emitByte(OpCode::OP_POP, sw->line, sw->col);
					emitLoop(startAddr, sw->line, sw->col);
				}
				for (int b : loopStack.back().breakJumps) patchJump(b);
				loopStack.pop_back();
				endScope(sw->line, sw->col);
				break;
			}
			case StmtType::FUNC: {
				auto f = static_cast<FuncStmt*>(s);
				Chunk* funcChunk = new Chunk();
				ByteCodeCompiler subCompiler(funcChunk);
				subCompiler.beginScope();
				for (const auto& param : f->params) subCompiler.addLocal(param.name);
				for (auto bodyStmt : f->body) subCompiler.compileStmt(bodyStmt);
				subCompiler.emitByte(OpCode::OP_NOTYPE, f->line, f->col);
				subCompiler.emitByte(OpCode::OP_RETURN, f->line, f->col);
				auto* funcObj = new FunctionObject(f->params, f->returnType, f->defaultRetArgs, f->returnsConst, f->body, nullptr, f->isCached, funcChunk);
				funcObj->name= f->name;
				Value funcVal;
				funcVal.type = ValueType::FUNCTION; 
				if (DEBUGGER_MODE_IS_ENABLED) funcVal.__DEBUGGING__NAME__=funcObj->name;
				funcVal.ref = std::shared_ptr<HeapObject>(funcObj);
				emitConstant(funcVal, f->line, f->col);
				emitIdentifier(OpCode::OP_DEFINE_VAR, f->name, f->line, f->col);
				chunk->write((uint8_t)0, f->line, f->col);
				break;
			}
			case StmtType::RETURN: {
				auto r = static_cast<ReturnStmt*>(s);
				if (r->value) compile(r->value);
				else emitByte(OpCode::OP_NOTYPE, r->line, r->col);
				emitByte(OpCode::OP_RETURN, r->line, r->col);
				break;
			}
			case StmtType::MULTI_LET: {
				auto m = static_cast<MultiLetStmt*>(s);
				for (auto val : m->values) {
					if (val) compileWithMode(val, val->line, val->col);
					else emitByte(OpCode::OP_NOTYPE, m->line, m->col);
				}
				if (scopeDepth > 0) {
					for (size_t i = 0; i < m->names.size(); i++) {
						addLocal(m->names[i]);
					}
				}
				else {
					for (int i = (int)m->names.size() - 1; i >= 0; i--) {
						Expr* valExpr = m->values[i];
						if (valExpr && valExpr->type == ExprType::OWNERSHIP && static_cast<OwnershipExpr*>(valExpr)->mode == CopyMode::REF) {
							auto o = static_cast<OwnershipExpr*>(valExpr);
							emitIdentifier(OpCode::OP_DEFINE_REF, m->names[i], m->line, m->col);
							if (auto v = dynamic_cast<VarExpr*>(o->expr)) emitIdentifier(OpCode::OP_REF_VAR, v->name, m->line, m->col);
							else if (auto idx = dynamic_cast<IndexExpr*>(o->expr)) {
								compile(idx->base);
								compile(idx->index);
								emitByte(OpCode::OP_REF_INDEX, m->line, m->col);
							}
						}
						else {
							emitIdentifier(OpCode::OP_DEFINE_VAR, m->names[i], m->line, m->col);
							uint8_t flags = 0;
							if (m->isConsts[i]) flags |= 0x01;
							if (m->isLocked)  flags |= 0x02;
							chunk->write(flags, m->line, m->col);
						}
					}
				}
				break;
			}
			case StmtType::MULTI_ASSIGN: {
				auto ma = static_cast<MultiAssignStmt*>(s);
				for (auto val : ma->values) compileWithMode(val, val->line, val->col);
				for (int i = (int)ma->targets.size() - 1; i >= 0; i--) {
					auto v = static_cast<VarExpr*>(ma->targets[i]);
					int arg = resolveLocal(v->name);
					if (arg != -1) {
						emitByte(OpCode::OP_SET_LOCAL, ma->line, ma->col);
						chunk->write((uint8_t)arg, ma->line, ma->col);
					}
					else emitIdentifier(OpCode::OP_SET_VAR, v->name, ma->line, ma->col);
					emitByte(OpCode::OP_POP, ma->line, ma->col);
				}
				break;
			}
			case StmtType::WHILE: {
				beginScope();
				auto w = static_cast<WhileStmt*>(s);
				int startAddr = (int)chunk->code.size();
				LoopContext loop = { startAddr, startAddr, {}, {}, false, locals.size(), -1 };
				loopStack.push_back(loop);
				bool optimized = false;
				int exitJump = -1;
				if (w->condition->type == ExprType::BINARY) {
					auto bin = static_cast<BinExpr*>(w->condition);
					if (bin->op == TokenType::LT) {
						if (bin->left->type == ExprType::VAR) {
							auto var = static_cast<VarExpr*>(bin->left);
							int localSlot = resolveLocal(var->name);
							if (localSlot != -1 && bin->right->type == ExprType::NUMBER) {
								auto num = static_cast<NumberExpr*>(bin->right);
								optimized = true;
								emitByte(OpCode::OP_JUMP_IF_NOT_LT, w->line, w->col);
								chunk->write((uint8_t)localSlot, w->line, w->col);
								int constIdx = chunk->addConstant(Value::Int((long long)num->val));
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
				for (auto stmt : w->body) compileStmt(stmt);
				endScope(w->line, w->col);
				emitLoop(startAddr, w->line, w->col);
				patchJump(exitJump);
				if (!optimized) emitByte(OpCode::OP_POP, w->line, w->col);
				for (int b : loopStack.back().breakJumps) patchJump(b);
				loopStack.pop_back();
				endScope(w->line, w->col);
				break;
			}
			case StmtType::DO_WHILE: {
				auto dw = static_cast<DoWhileStmt*>(s);
				int startAddr = (int)chunk->code.size();
				beginScope();
				LoopContext loop = { startAddr, -1, {}, {}, false, locals.size(), -1 };
				loopStack.push_back(loop);
				beginScope();
				for (auto stmt : dw->body) compileStmt(stmt);
				endScope(dw->line, dw->col);
				for (int jump : loopStack.back().continueJumps) patchJump(jump);
				endScope(dw->line, dw->col);
				compile(dw->condition);
				int exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE, dw->line, dw->col);
				emitByte(OpCode::OP_POP, dw->line, dw->col);
				emitLoop(startAddr, dw->line, dw->col);
				patchJump(exitJump);
				emitByte(OpCode::OP_POP, dw->line, dw->col);
				for (int b : loopStack.back().breakJumps) patchJump(b);
				loopStack.pop_back();
				endScope(dw->line, dw->col);
				break;
			}
			case StmtType::FOR: {
				beginScope();
				auto f = static_cast<ForStmt*>(s);
				for (auto init : f->inits) compileStmt(init);
				int condAddr = (int)chunk->code.size();
				int exitJump = -1;
				if (f->condition) {
					compile(f->condition);
					exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE, f->line, f->col);
					emitByte(OpCode::OP_POP, f->line, f->col);
				}
				LoopContext loop = { condAddr, -1, {}, {}, false, locals.size(), -1 };
				loopStack.push_back(loop);
				beginScope();
				for (auto stmt : f->body) compileStmt(stmt);
				endScope(f->line, f->col);
				int stepStart = (int)chunk->code.size();
				for (int jump : loopStack.back().continueJumps) {
					patchJump(jump);
				}
				loopStack.back().stepAddress = stepStart;
				for (auto step : f->steps) compileStmt(step);
				emitLoop(condAddr, f->line, f->col);
				if (exitJump != -1) {
					patchJump(exitJump);
					emitByte(OpCode::OP_POP, f->line, f->col);
				}
				for (int b : loopStack.back().breakJumps) patchJump(b);
				loopStack.pop_back();
				endScope(f->line, f->col);
				break;
			}
			case StmtType::FOR_EACH: {
				auto fe = static_cast<ForEachStmt*>(s);
				beginScope();
				size_t colCount = fe->collections.size();
				size_t varCount = fe->loopVars.size();
				bool isDictUnpack = (colCount == 1 && varCount == 2);
				bool isOneToOne = (colCount == varCount);
				if (!isDictUnpack && !isOneToOne) {
					throw ValueError("Mismatch between loop variables and collections.", fe->line, fe->col);
				}
				int streamCount = isDictUnpack ? 2 : (int)colCount;
				if (isDictUnpack) {
					compile(fe->collections[0]);
					emitByte(OpCode::OP_UNPACK_DICT, fe->line, fe->col);
				}
				else {
					for (auto col : fe->collections) {
						compile(col);
						emitByte(OpCode::OP_TO_STREAM, fe->line, fe->col);
					}
				}
				for (int i = 0; i < streamCount; i++) addLocal("");
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
					for (size_t i = 0; i < fe->loopVars.size(); i++) addLocal(fe->loopVars[i]);
				}
				else {
					for (int i = (int)fe->loopVars.size() - 1; i >= 0; i--) {
						emitIdentifier(OpCode::OP_DEFINE_VAR, fe->loopVars[i], fe->line, fe->col);
						chunk->write(0, fe->line, fe->col);
					}
				}
				loopStack.push_back({ startAddr, startAddr, {}, {}, true, (int)locals.size() - streamCount, iterSlot });
				beginScope();
				for (auto stmt : fe->body) compileStmt(stmt);
				endScope(fe->line, fe->col);
				for (int i = 0; i < streamCount; i++) {
					emitByte(OpCode::OP_POP, fe->line, fe->col);
				}
				for (int jump : loopStack.back().continueJumps) patchJump(jump);
				emitLoop(startAddr, fe->line, fe->col);
				patchJump(exitJump);
				scopeDepth--;
				while (locals.size() > 0 && locals.back().depth > scopeDepth) locals.pop_back();
				emitByte(OpCode::OP_POP, fe->line, fe->col);
				if (locals.size() > 0) locals.pop_back();
				for (int i = 0; i < streamCount; i++) {
					emitByte(OpCode::OP_POP, fe->line, fe->col);
					if (locals.size() > 0) locals.pop_back();
				}
				scopeDepth--;
				break;
			}
			case StmtType::BREAK: {
				if (loopStack.empty()) throw ControlFlowError("break outside of loop", s->line, s->col);
				int localsToPop = (int)locals.size() - loopStack.back().startLocalCount;
				for (int i = 0; i < localsToPop; i++) emitByte(OpCode::OP_POP, s->line, s->col);
				loopStack.back().breakJumps.push_back(emitJump(OpCode::OP_JUMP, s->line, s->col));
				break;
			}
			case StmtType::CONTINUE: {
				if (loopStack.empty()) throw ControlFlowError("continue outside of loop", s->line, s->col);
				int localsToPop = (int)locals.size() - loopStack.back().startLocalCount;
				for (int i = 0; i < localsToPop; i++) emitByte(OpCode::OP_POP, s->line, s->col);
				int target = loopStack.back().stepAddress;
				if (target == -1) {
					int jump = emitJump(OpCode::OP_JUMP, s->line, s->col);
					loopStack.back().continueJumps.push_back(jump);
				}
				else emitLoop(target, s->line, s->col);
				break;
			}
			case StmtType::SKIP: {
				auto sk = static_cast<SkipStmt*>(s);
				if (loopStack.empty()) throw ControlFlowError("skip statement outside of loop", s->line, s->col);
				if (!loopStack.back().isForEach) {
					throw ControlFlowError("skip statement is only valid in for-each loops.", s->line, s->col);
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
	void emitIdentifier(OpCode op, const string& name, int line, int col) {
		int index = chunk->addConstant(Value::String(name));
		emitByte(op, line, col);
		chunk->write(static_cast<uint8_t>(index), line, col);
	}
	void emitLoop(int loopStart, int line, int col) {
		emitByte(OpCode::OP_LOOP, line, col);
		int offset = (int)chunk->code.size() - loopStart + 2;
		if (offset > 65535) throw RangeError("Loop body too large", line, col);
		chunk->write((offset >> 8) & 0xff, line, col);
		chunk->write(offset & 0xff, line, col);
	}
};
struct VM {
	std::deque<Value> stack;
	std::shared_ptr<Env> globals;
	std::function<Value(MethodCallExpr*)> methodResolver;
	std::vector<CallFrame> frames;
	std::unordered_set<std::string> importStack;
	std::function<void(std::string, std::vector<std::string>)> importResolver;
	CallFrame* frame;
	uint8_t* ip;
	VM() {
		globals = std::make_shared<Env>();
		frame = nullptr;
		ip = nullptr;
	}
	void run(Chunk& chunk)	 {
		CallFrame mainFrame;
		mainFrame.function = nullptr;
		mainFrame.ip = chunk.code.data();
		mainFrame.basePointer = 0;
		frames.push_back(mainFrame);
		frame = &frames.back();
		ip = frame->ip;
		Value pendingReturn = Value::None();
		bool isReturning = false;
		Value pendingError = Value::None();
		bool isHandlingError = false;
		int line = 0;
		int col = 0;
		while (true) {
			Chunk* currentChunk = frame->function ? frame->function->chunk : &chunk;
			try {
				// --- DEBUGGER START ---
				if(DEBUGGER_MODE_IS_ENABLED){
					int currentOffset = (int)(ip - currentChunk->code.data());
					std::cout << "LINE: " << std::left << std::setw(4) <<line<<" | ";
					std::cout<< std::left<< std::setw(18)<<OpCodeToString((OpCode)*ip) <<" | "<< PrintStackForDebug(stack) << "\n";
				}
				// --- DEBUGGER END ---
				OpCode instruction = static_cast<OpCode>(*ip++);
				int offset = (int)(ip - currentChunk->code.data());
				line = currentChunk->lines[offset - 1];
				col = currentChunk->columns[offset - 1];
				switch (instruction) {
				case OpCode::OP_IMPORT: {
					uint8_t count = *ip++;
					std::vector<std::string> symbols;
					for (int i = 0; i < count; i++) symbols.push_back(pop().asString());
					std::reverse(symbols.begin(), symbols.end());
					std::string libName = pop().asString();
					Value moduleResult;
					if (libName.length() > 4 && libName.substr(libName.length() - 4) == ".ymm") {
						namespace fs = std::filesystem;
						fs::path p(libName);
						if (!fs::exists(p)) throw ImportError("Module file not found: " + libName, line, col);
						std::error_code ec;
						std::string absPath = fs::absolute(p, ec).string();
						if (importStack.count(absPath)) throw CircularImportError("Circular import detected: " + libName, line, col);
						std::ifstream file(absPath);
						if (!file) throw FileNotFoundError("Unable to read module: " + libName, line, col);
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
								Stmt* stmt = parser.parseStmt();
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
							for (const auto& [key, var] : moduleVM.globals->vars) {
								if (key == "None") continue;
								exportDict->items[Value::String(key)] = var.value;
								if (count == 0) {
									if (!globals->exists(key)) {
										globals->set(key, var.value, var.isLocked, var.isConst);
									}
								}
							}
							moduleResult = Value::Dict(exportDict->items);
						}
						catch (...) {
							importStack.erase(absPath);
							throw;
						}
						importStack.erase(absPath);
					}
					else {
						if (this->importResolver) {
							this->importResolver(libName, symbols);
							if (count == 0) {
								if (globals->exists(libName)) moduleResult = globals->get(libName);
								else moduleResult = Value::None();
							}
							else moduleResult = Value::None();
						}
						else throw EnvironmentError("Import resolver not linked!", line, col);
					}
					if (count == 0) stack.push_back(moduleResult);
					else {
						auto findVal = [&](std::string key) -> Value {
							if (moduleResult.type == ValueType::DICT) {
								auto* d = static_cast<DictObject*>(moduleResult.ref.get());
								Value k = Value::String(key);
								if (d->items.count(k)) return d->items.at(k);
							}
							if (globals->exists(key)) return globals->get(key);
							throw InvalidImportError("Module '" + libName + "' does not export '" + key + "'", line, col);
						};
						for (const auto& sym : symbols) stack.push_back(findVal(sym));
					}
					break;
				}
				case OpCode::OP_DEBUG_NAME: {
					uint8_t nameIndex = *ip++;
					string name = currentChunk->constants[nameIndex].asString();
					stack.back().__DEBUGGING__NAME__ = name;
					break;
				}
				case OpCode::OP_SET_FLAGS: {
					uint8_t flags = *ip++;
					if (stack.empty()) throw UnderflowError("Stack underflow", line, col);
					stack.back().isConst = (flags & 0x01);
					stack.back().isLocked = (flags & 0x02);
					break;
				}
				case OpCode::OP_LIST_APPEND: {
					uint8_t slot = *ip++;
					Value val = pop();
					Value& listVal = stack[frame->basePointer + slot];
					if (listVal.type != ValueType::LIST) throw TypeError("Append target is not a list", line, col);
					auto* list = static_cast<ListObject*>(listVal.ref.get());
					list->elements.push_back(val);
					break;
				}
				case OpCode::OP_SET_ADD: {
					uint8_t slot = *ip++;
					Value val = pop();
					Value& setVal = stack[frame->basePointer + slot];
					if (setVal.type != ValueType::SET) throw TypeError("Add target is not a set", line, col);
					auto* set = static_cast<SetObject*>(setVal.ref.get());
					setAdd(set->elements,val);
					break;
				}
				case OpCode::OP_DICT_SET: {
					uint8_t slot = *ip++;
					Value val = pop();
					Value key = pop();
					Value& dictVal = stack[frame->basePointer + slot];
					if (dictVal.type != ValueType::DICT) throw TypeError("Target is not a dict", line, col);
					auto* dict = static_cast<DictObject*>(dictVal.ref.get());
					if (key.type == ValueType::LIST || key.type == ValueType::SET || key.type == ValueType::DICT) {
						if (key.type == ValueType::DICT) throw TypeError("Dictionary cannot be used as a key", line, col);
						key = deepCopy(key);
						key.isConst = true;
					}
					dict->items[key] = val;
					break;
				}
				case OpCode::OP_LIST_TO_TUPLE: {
					Value v = pop();
					if (v.type != ValueType::LIST) throw TypeError("Expected list for tuple conversion", line, col);
					auto* list = static_cast<ListObject*>(v.ref.get());
					stack.push_back(Value::Tuple(list->elements));
					break;
				}
				case OpCode::OP_LIST_TO_VECTOR: {
					Value v = pop();
					if (v.type != ValueType::LIST) throw TypeError("Expected list for vector conversion", line, col);
					auto* list = static_cast<ListObject*>(v.ref.get());
					for (const auto& el : list->elements) if (!el.isNumber()) throw TypeError("Vector elements must be numbers", line, col);
					stack.push_back(Value::Vector(list->elements));
					break;
				}
				case OpCode::OP_BUILD_FSTRING: {
					uint8_t count = *ip++;
					int startPos = stack.size() - count;
					std::string finalStr = "";
					for (int i = 0; i < count; i++) {
						Value v = stack[startPos + i];
						finalStr += v.type == ValueType::STRING? v.asString() : valueToString(v);
					}
					for (int i = 0; i < count; i++) stack.pop_back();
					stack.push_back(Value::String(finalStr));
					break;
				}
				case OpCode::OP_GET_LOCAL: {
					uint8_t slot = *ip++;
					Value val = stack[frame->basePointer + slot];
					if (val.type == ValueType::REFERENCE) stack.push_back(*val.ptr);
					else stack.push_back(val);
					break;
				}
				case OpCode::OP_SET_LOCAL: {
					uint8_t slot = *ip++;
					Value& slotVal = stack[frame->basePointer + slot];
					Value newVal = stack.back();
					if (slotVal.type == ValueType::REFERENCE) {
						if (slotVal.isConst) throw OwnershipError("Cannot assign to const reference", line, col);
						Value* target = slotVal.ptr;
						if (target->isConst) throw ConstError("Cannot assign to const variable via reference", line, col);
						if (target->isLocked && target->type != newVal.type) {
							throw TypeError("Cannot change type of locked variable via reference", line, col);
						}
						bool wasConst = target->isConst;
						bool wasLocked = target->isLocked;
						*target = newVal;
						target->isConst = wasConst;
						target->isLocked = wasLocked;
					}
					else {
						if (slotVal.isConst) throw ConstError("Cannot assign to const variable", line, col);
						if (slotVal.isLocked && slotVal.type != newVal.type) {
							throw ConstError("Cannot change type of locked variable", line, col);
						}
						bool wasConst = slotVal.isConst;
						bool wasLocked = slotVal.isLocked;
						slotVal = newVal;
						slotVal.isConst = wasConst;
						slotVal.isLocked = wasLocked;
					}
					break;
				}
				case OpCode::OP_REF_LOCAL: {
					uint8_t slot = *ip++;
					Value* ptr = &stack[frame->basePointer + slot];
					if (ptr->type == ValueType::REFERENCE) ptr = ptr->ptr;
					stack.push_back(Value::Reference(ptr));
					break;
				}
				case OpCode::OP_REF_VAR: {
					uint8_t nameIndex = *ip++;
					string name = currentChunk->constants[nameIndex].asString();
					Var& v = globals->lookup(name);
					Value* ptr = v.alias ? v.alias : &v.value;
					stack.push_back(Value::Reference(ptr));
					break;
				}
				case OpCode::OP_REF_INDEX: {
					Value index = pop();
					Value base = pop();
					Value* ptr = nullptr;
					if (base.type == ValueType::LIST) {
						auto* list = static_cast<ListObject*>(base.ref.get());
						long long idx = index.asInt();
						ptr = &list->elements[idx];
					}
					else if (base.type == ValueType::VECTOR) {
						auto* vec = static_cast<VectorObject*>(base.ref.get());
						long long idx = index.asInt();
						ptr = &vec->elements[idx];
					}
					else throw OwnershipError("Cannot take reference of this type", line, col);
					stack.push_back(Value::Reference(ptr));
					break;
				}
				case OpCode::OP_INC_LOCAL: {
					uint8_t slot = *ip++;
					int idx = frame->basePointer + slot;
					if (stack[idx].type == ValueType::INT) stack[idx].iVal++;
					else if (stack[idx].type == ValueType::FLOAT) stack[idx].fVal++;
					else stack[slot].fVal++;
					break;
				}
				case OpCode::OP_JUMP_IF_NOT_LT: {
					uint8_t slot = *ip++;
					uint8_t constIdx = *ip++;
					uint16_t offset = (ip[0] << 8) | ip[1];
					ip += 2;
					int idx = frame->basePointer + slot;
					if (stack[idx].type == ValueType::INT) {
						 long long localVal = stack[idx].iVal;
						 long long constVal = currentChunk->constants[constIdx].iVal;
						 if (localVal >= constVal) ip += offset;
					}
					else throw TypeError("Optimized loop requires integer", line, col);
					break;
				}
				case OpCode::OP_SWITCH_TABLE: {
					Value val = stack.back();
					uint8_t minIdx = *ip++;
					uint8_t count = *ip++;
					if (val.type == ValueType::INT) {
						long long minVal = currentChunk->constants[minIdx].iVal;
						long long jumpIdx = val.iVal - minVal;
						if (jumpIdx >= 0 && jumpIdx < count) {
							uint8_t* tableEntry = ip + (jumpIdx * 2);
							uint16_t offset = (tableEntry[0] << 8) | tableEntry[1];
							ip = tableEntry + 2 + offset;
						}
						else ip += (count * 2);
					}
					else ip += (count * 2);
					break;
				}
				case OpCode::OP_CONSTANT: {
					uint8_t index = *ip++;
					stack.push_back(currentChunk->constants[index]);
					break;
				}
				case OpCode::OP_TRUE:  stack.push_back(Value::Bool(true)); break;
				case OpCode::OP_FALSE: stack.push_back(Value::Bool(false)); break;
				case OpCode::OP_NONE:  stack.push_back(Value::None()); break;
				case OpCode::OP_NOTYPE:  stack.push_back(Value::NoType()); break;
				case OpCode::OP_POP: {
					if (!stack.empty()) stack.pop_back();
					break;
				}
				case OpCode::OP_ADD: {
					Value b = pop();
					Value a = pop();
					if (a.type == ValueType::INT && b.type == ValueType::INT) {
						long long res = a.iVal + b.iVal;
						bool overflow = ((a.iVal ^ res) & (b.iVal ^ res)) < 0;
						if (overflow) stack.push_back(BigIntObject::add(Value::BigInt(a.iVal), Value::BigInt(b.iVal)));
						else stack.push_back(Value::Int(res));
					}
					else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
						stack.push_back(BigIntObject::add(a, b));
					}
					else if (a.type == ValueType::STRING || b.type == ValueType::STRING) {
						stack.push_back(Value::String(valueToString(a) + valueToString(b)));
					}
					else if (a.type == ValueType::VECTOR && b.type == ValueType::VECTOR) {
						auto* v1 = static_cast<VectorObject*>(a.ref.get());
						auto* v2 = static_cast<VectorObject*>(b.ref.get());
						if (v1->elements.size() != v2->elements.size()) throw ValueError("Vector dimension mismatch", line, 0);
						vector<Value> res;
						res.reserve(v1->elements.size());
						for (size_t i = 0; i < v1->elements.size(); i++) {
							Value x = v1->elements[i];
							Value y = v2->elements[i];
							if (x.type == ValueType::INT && y.type == ValueType::INT) {
								long long r = x.iVal + y.iVal;
								bool ovf = ((x.iVal ^ r) & (y.iVal ^ r)) < 0;
								if (ovf) res.push_back(BigIntObject::add(Value::BigInt(x.iVal), Value::BigInt(y.iVal)));
								else res.push_back(Value::Int(r));
							}
							else if (x.type == ValueType::BIGINT || y.type == ValueType::BIGINT) {
								res.push_back(BigIntObject::add(x, y));
							}
							else res.push_back(Value::Float(x.asFloat() + y.asFloat()));
						}
						stack.push_back(Value::Vector(res));
					}
					else stack.push_back(Value::Float(a.asFloat() + b.asFloat()));
					break;
				}
				case OpCode::OP_SUB: {
					Value b = pop();
					Value a = pop();
					if (a.type == ValueType::INT && b.type == ValueType::INT) {
						long long res = a.iVal - b.iVal;
						bool overflow = ((a.iVal ^ b.iVal) & (a.iVal ^ res)) < 0;
						if (overflow) stack.push_back(BigIntObject::sub(Value::BigInt(a.iVal), Value::BigInt(b.iVal)));
						else stack.push_back(Value::Int(res));
					}
					else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) stack.push_back(BigIntObject::sub(a, b));
					else if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) stack.push_back(Value::Float(a.asFloat() - b.asFloat()));
					else if (a.type == ValueType::VECTOR && b.type == ValueType::VECTOR) {
						auto* v1 = static_cast<VectorObject*>(a.ref.get());
						auto* v2 = static_cast<VectorObject*>(b.ref.get());
						if (v1->elements.size() != v2->elements.size()) throw ValueError("Vector dimension mismatch", line, col);
						vector<Value> res;
						res.reserve(v1->elements.size());
						for (size_t i = 0; i < v1->elements.size(); i++) {
							Value x = v1->elements[i];
							Value y = v2->elements[i];
							if (x.type == ValueType::INT && y.type == ValueType::INT) {
								long long r = x.iVal - y.iVal;
								bool ovf = ((x.iVal ^ y.iVal) & (x.iVal ^ r)) < 0;
								if (ovf) res.push_back(BigIntObject::sub(Value::BigInt(x.iVal), Value::BigInt(y.iVal)));
								else res.push_back(Value::Int(r));
							}
							else if (x.type == ValueType::BIGINT || y.type == ValueType::BIGINT) {
								res.push_back(BigIntObject::sub(x, y));
							}
							else res.push_back(Value::Float(x.asFloat() - y.asFloat()));
						}
						stack.push_back(Value::Vector(res));
					}
					else stack.push_back(Value::Float(a.asFloat() - b.asFloat()));
					break;
				}
				case OpCode::OP_DIV: {
					Value b = pop();
					Value a = pop();
					if (a.type == ValueType::VECTOR) {
						if (!b.isNumber()) throw TypeError("Vector can only be divided by a number", line, col);
						if (b.asFloat() == 0) throw DivisionByZeroError("Vector division by zero", line, col);
						auto* v = static_cast<VectorObject*>(a.ref.get());
						vector<Value> res;
						res.reserve(v->elements.size());
						double s = b.asFloat();
						for (const auto& elem : v->elements) res.push_back(Value::Float(elem.asFloat() / s));
						stack.push_back(Value::Vector(res));
					}
					else if (b.type == ValueType::VECTOR) throw TypeError("Cannot divide by a vector", line, col);
					else {
						double db = b.asFloat();
						if (db == 0) throw DivisionByZeroError("Division by zero", line, col);
						stack.push_back(Value::Float(a.asFloat() / db));
					}
					break;
				}
				case OpCode::OP_MUL: {
					Value b = pop();
					Value a = pop();
					if (a.type == ValueType::STRING && b.type == ValueType::INT) {
						string res = "";
						string base = a.asString();
						long long count = b.asInt();
						if (count < 0) count = 0;
						if (count > 1000000) throw MemoryError("String repetition too large", line, col);
						for (long long i = 0; i < count; i++) res += base;
						stack.push_back(Value::String(res));
					}
					else if (a.type == ValueType::LIST && b.type == ValueType::INT) {
						auto* listObj = static_cast<ListObject*>(a.ref.get());
						long long count = b.asInt();
						vector<Value> res;
						if (count > 0) {
							if (listObj->elements.size() * count > 1000000)
								throw MemoryError("List repetition too large", line, col);
							res.reserve(listObj->elements.size() * count);
							for (int i = 0; i < count; i++) {
								for (const auto& elem : listObj->elements) res.push_back(deepCopy(elem));
							}
						}
						stack.push_back(Value::List(res));
					}
					else if (a.type == ValueType::VECTOR && b.type == ValueType::VECTOR) {
						auto* v1 = static_cast<VectorObject*>(a.ref.get());
						auto* v2 = static_cast<VectorObject*>(b.ref.get());
						if (v1->elements.size() != v2->elements.size()) throw ValueError("Vector dimension mismatch", line, col);
						Value dot = Value::Int(0);
						for (size_t i = 0; i < v1->elements.size(); i++) {
							Value x = v1->elements[i];
							Value y = v2->elements[i];
							Value prod;
							if (x.type == ValueType::INT && y.type == ValueType::INT) {
								long long r = x.iVal * y.iVal;
								bool ovf = (x.iVal != 0 && r / x.iVal != y.iVal);
								if (ovf) prod = BigIntObject::mul(Value::BigInt(x.iVal), Value::BigInt(y.iVal));
								else prod = Value::Int(r);
							}
							else if (x.type == ValueType::BIGINT || y.type == ValueType::BIGINT) prod = BigIntObject::mul(x, y);
							else prod = Value::Float(x.asFloat() * y.asFloat());
							if (dot.type == ValueType::INT && prod.type == ValueType::INT) {
								long long r = dot.iVal + prod.iVal;
								bool ovf = ((dot.iVal ^ r) & (prod.iVal ^ r)) < 0;
								if (ovf) dot = BigIntObject::add(Value::BigInt(dot.iVal), Value::BigInt(prod.iVal));
								else dot = Value::Int(r);
							}
							else if (dot.type == ValueType::BIGINT || prod.type == ValueType::BIGINT) dot = BigIntObject::add(dot, prod);
							else dot = Value::Float(dot.asFloat() + prod.asFloat());
						}
						stack.push_back(dot);
					}
					else if ((a.type == ValueType::VECTOR && b.isNumber()) || (a.isNumber() && b.type == ValueType::VECTOR)) {
						VectorObject* vec = (a.type == ValueType::VECTOR) ? static_cast<VectorObject*>(a.ref.get()) : static_cast<VectorObject*>(b.ref.get());
						Value scalar = (a.type == ValueType::VECTOR) ? b : a;
						vector<Value> res;
						res.reserve(vec->elements.size());
						for (const auto& elem : vec->elements) {
							if (elem.type == ValueType::INT && scalar.type == ValueType::INT) {
								long long r = elem.iVal * scalar.iVal;
								bool ovf = (elem.iVal != 0 && r / elem.iVal != scalar.iVal);
								if (ovf) res.push_back(BigIntObject::mul(Value::BigInt(elem.iVal), Value::BigInt(scalar.iVal)));
								else res.push_back(Value::Int(r));
							}
							else if (elem.type == ValueType::BIGINT || scalar.type == ValueType::BIGINT) {
								res.push_back(BigIntObject::mul(elem, scalar));
							}
							else res.push_back(Value::Float(elem.asFloat() * scalar.asFloat()));
						}
						stack.push_back(Value::Vector(res));
					}
					else if ((a.type == ValueType::FLOAT && b.type == ValueType::BIGINT) || (a.type == ValueType::BIGINT && b.type == ValueType::FLOAT)) {
						Value fVal = (a.type == ValueType::FLOAT) ? a : b;
						Value intVal = (a.type == ValueType::FLOAT) ? b : a;
						auto* bigObj = static_cast<BigIntObject*>(intVal.ref.get());
						std::vector<uint32_t> tempChunks = bigObj->chunks;
						double floatBase = fVal.asFloat();
						double result = 0.0;
						double powerOf10 = 1.0;
						bool overflow = false;
						if (tempChunks.size() > 40) overflow = true;
						else {
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
							if (bigObj->isNegative) result = -result;
							stack.push_back(Value::Float(result));
						}
						else {
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
								for (int i = 0; i < power; i++) divisor = divisor * ten;
								product = product / divisor;
							}
							bool resultNeg = (bigObj->isNegative) != (floatBase < 0);
							product.isNegative = resultNeg;
							stack.push_back(Value::BigInt(std::make_shared<BigIntObject>(product)));
						}
					}
					else if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
						stack.push_back(Value::Float(a.asFloat() * b.asFloat()));
					}
					else if (a.type == ValueType::INT && b.type == ValueType::INT) {
						long long res = a.iVal * b.iVal;
						bool overflow = (a.iVal != 0 && res / a.iVal != b.iVal);
						if (overflow) stack.push_back(BigIntObject::mul(Value::BigInt(a.iVal), Value::BigInt(b.iVal)));
						else stack.push_back(Value::Int(res));
					}
					else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) stack.push_back(BigIntObject::mul(a, b));
					else stack.push_back(Value::Float(a.asFloat() * b.asFloat()));
					break;
				}
				case OpCode::OP_FLOOR_DIV: {
					Value b = pop();
					Value a = pop();
					if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
						if (b.asFloat() == 0) throw DivisionByZeroError("Division by zero", line, col);
						stack.push_back(BigIntObject::div(a, b));
					}
					else {
						double db = b.asFloat();
						if (db == 0) throw DivisionByZeroError("Division by zero", line, col);
						stack.push_back(Value::Int((long long)(a.asFloat() / db)));
					}
					break;
				}
				case OpCode::OP_MOD: {
					Value b = pop();
					Value a = pop();
					if (a.type == ValueType::INT && b.type == ValueType::INT) {
						if (b.iVal == 0) throw DivisionByZeroError("Modulo by zero", line, col);
						stack.push_back(Value::Int(a.iVal % b.iVal));
					}
					else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) stack.push_back(BigIntObject::mod(a, b));
					else {
						if (b.asFloat() == 0) throw DivisionByZeroError("Modulo by zero", line, col);
						stack.push_back(Value::Float(fmod(a.asFloat(), b.asFloat())));
					}
					break;
				}
				case OpCode::OP_POW: {
					Value b = pop(); 
					Value a = pop();
					if ((a.type == ValueType::INT || a.type == ValueType::BIGINT) &&
						(b.type == ValueType::INT || b.type == ValueType::BIGINT)) {
						stack.push_back(BigIntObject::pow(a, b));
					}
					else stack.push_back(Value::Float(pow(a.asFloat(), b.asFloat())));
					break;
				}
				// --- Comparisons ---
				case OpCode::OP_GT: { Value b = pop(); Value a = pop(); stack.push_back(Value::Bool(a.asFloat() > b.asFloat())); break; }
				case OpCode::OP_GTE: { Value b = pop(); Value a = pop(); stack.push_back(Value::Bool(a.asFloat() >= b.asFloat())); break; }
				case OpCode::OP_LT: { Value b = pop(); Value a = pop(); stack.push_back(Value::Bool(a.asFloat() < b.asFloat())); break; }
				case OpCode::OP_LTE: { Value b = pop(); Value a = pop(); stack.push_back(Value::Bool(a.asFloat() <= b.asFloat())); break; }
				case OpCode::OP_EQ: { Value b = pop(); Value a = pop(); stack.push_back(Value::Bool(a.looseEquals(b))); break; }
				case OpCode::OP_NEQ: { Value b = pop(); Value a = pop(); stack.push_back(Value::Bool(!(a.looseEquals(b)))); break; }
				case OpCode::OP_STRICT_EQ: { Value b = pop(); Value a = pop(); stack.push_back(Value::Bool(a.strictEquals(b))); break; }
				case OpCode::OP_STRICT_NEQ: { Value b = pop(); Value a = pop(); stack.push_back(Value::Bool(!(a.strictEquals(b)))); break; }
				case OpCode::OP_IS: {
					Value b = pop(); Value a = pop();
					const Value& valA = (a.type == ValueType::REFERENCE && a.ptr) ? *a.ptr : a;
					const Value& valB = (b.type == ValueType::REFERENCE && b.ptr) ? *b.ptr : b;
					bool same = false;
					if (valA.type == valB.type) same = (valA.ref.get() == valB.ref.get());
					stack.push_back(Value::Bool(same));
					break;
				}
				case OpCode::OP_IS_NOT: {
					Value b = pop(); 
					Value a = pop();
					const Value& valA = (a.type == ValueType::REFERENCE && a.ptr) ? *a.ptr : a;
					const Value& valB = (b.type == ValueType::REFERENCE && b.ptr) ? *b.ptr : b;
					bool same = false;
					if (valA.type == valB.type) same = (valA.ref.get() == valB.ref.get());
					stack.push_back(Value::Bool(!same));
					break;
				}
				case OpCode::OP_XOR: {
					Value b = pop(); Value a = pop();
					stack.push_back(Value::Bool(a.isTruthy() != b.isTruthy()));
					break;
				}
				case OpCode::OP_NXOR: {
					Value b = pop(); Value a = pop();
					stack.push_back(Value::Bool(a.isTruthy() == b.isTruthy()));
					break;
				}
				case OpCode::OP_NAND: {
					Value b = pop(); Value a = pop();
					stack.push_back(Value::Bool(!(a.isTruthy() && b.isTruthy())));
					break;
				}
				case OpCode::OP_NOR: {
					Value b = pop(); Value a = pop();
					stack.push_back(Value::Bool(!(a.isTruthy() || b.isTruthy())));
					break;
				}
				case OpCode::OP_NOT: {
					Value v = pop();
					stack.push_back(Value::Bool(!v.isTruthy()));
					break;
				}
				case OpCode::OP_NEGATE: {
					Value v = pop();
					if (v.type == ValueType::INT) {
						if (v.iVal == LLONG_MIN) stack.push_back(BigIntObject::mul(Value::BigInt(v.iVal), Value::BigInt(-1)));
						else stack.push_back(Value::Int(-v.iVal));
					}
					else if (v.type == ValueType::BIGINT) {
						stack.push_back(BigIntObject::mul(v, Value::BigInt(-1)));
					}
					else if (v.type == ValueType::VECTOR) {
						auto* vec = static_cast<VectorObject*>(v.ref.get());
						vector<Value> res;
						res.reserve(vec->elements.size());
						for (const auto& el : vec->elements) {
							if (el.type == ValueType::INT) {
								if (el.iVal == LLONG_MIN) res.push_back(BigIntObject::mul(Value::BigInt(el.iVal), Value::BigInt(-1)));
								else res.push_back(Value::Int(-el.iVal));
							}
							else if (el.type == ValueType::BIGINT) {
								res.push_back(BigIntObject::mul(el, Value::BigInt(-1)));
							}
							else {
								res.push_back(Value::Float(-el.asFloat()));
							}
						}
						stack.push_back(Value::Vector(res));
					}
					else stack.push_back(Value::Float(-v.asFloat()));
					break;
				}
				case OpCode::OP_IS_IN: 
				case OpCode::OP_IS_NOT_IN: {
					Value rhs = pop();
					Value lhs = pop();
					bool found = false;
					if (rhs.type == ValueType::STRING) {
						if (lhs.type == ValueType::STRING) {
							found = rhs.asString().find(lhs.asString()) != string::npos;
						}
					}
					else if (rhs.type == ValueType::LIST) {
						auto* list = static_cast<ListObject*>(rhs.ref.get());
						for (const auto& item : list->elements) {
							if (item.strictEquals(lhs)) { found = true; break; }
						}
					}
					else if (rhs.type == ValueType::DICT) {
						auto* d = static_cast<DictObject*>(rhs.ref.get());
						found = d->items.count(lhs) > 0;
					}
					else if (rhs.type == ValueType::SET) {
						auto* s = static_cast<SetObject*>(rhs.ref.get());
						for (const auto& item : s->elements) {
							if (item.strictEquals(lhs)) { found = true; break; }
						}
					}
					else if (rhs.type == ValueType::RANGE) {
						auto* rng = static_cast<RangeObject*>(rhs.ref.get());
						if (lhs.isNumber()) {
							double val = lhs.asFloat();
							bool inBounds = (rng->step > 0) ?
							(val >= rng->start && (rng->endInclusive ? val <= rng->end : val < rng->end)) :
							(val <= rng->start && (rng->endInclusive ? val >= rng->end : val > rng->end));
							if (inBounds && !rng->isFloat && lhs.type == ValueType::INT) {
								found = ((long long)(val - rng->start) % (long long)rng->step == 0);
							}
							else {
								found = inBounds;
							}
						}
					}
					bool finalResult = (instruction == OpCode::OP_IS_IN) ? found : !found;
					stack.push_back(Value::Bool(finalResult));
					break;
				}
				case OpCode::OP_LOOP: {
					uint8_t hi = *ip++;
					uint8_t lo = *ip++;
					uint16_t offset = (hi << 8) | lo;
					ip -= offset;
					break;
				}
				case OpCode::OP_DEFINE_VAR: {
					uint8_t nameIndex = *ip++;
					uint8_t flags = *ip++;
					string name = currentChunk->constants[nameIndex].asString();
					Value val = pop();
					if (DEBUGGER_MODE_IS_ENABLED) val.__DEBUGGING__NAME__=name;
					bool isConst = (flags & 0x01) != 0;
					bool isLocked = (flags & 0x02) != 0;
					globals->set(name, val, isLocked, isConst);
					break;
				}
				case OpCode::OP_GET_VAR: {
					uint8_t nameIndex = *ip++;
					string name = currentChunk->constants[nameIndex].asString();
					if (!globals->exists(name)) throw NameError("Undefined variable '" + name + "'", line, col);
					Var& v = globals->lookup(name);
					if (v.alias) stack.push_back(*v.alias);
					else stack.push_back(v.value);
					break;
				}
				case OpCode::OP_JUMP_IF_FALSE: {
					uint8_t hi = *ip++;
					uint8_t lo = *ip++;
					uint16_t offset = (hi << 8) | lo;
					if (!stack.back().isTruthy()) {
						ip += offset;
					}
					break;
				}
				case OpCode::OP_JUMP: {
					uint8_t hi = *ip++;
					uint8_t lo = *ip++;
					uint16_t offset = (hi << 8) | lo;
					ip += offset;
					break;
				}
				case OpCode::OP_CALL: {
					uint8_t argCount = *ip++;
					Value callee = pop();
					if (callee.type == ValueType::NATIVE_FUNCTION) {
						vector<Value> args;
						for (int i = 0; i < argCount; i++) {
							args.insert(args.begin(), pop());
						}
						auto native = static_cast<NativeFunctionObject*>(callee.ref.get());
						Value result = native->func(args, line, 0);
							stack.push_back(result);
					}
					else callValue(callee, argCount, line, col);
					break;
				}
				case OpCode::OP_UNPACK_DICT: {
					Value v = pop();
					if (v.type != ValueType::DICT) {
						throw TypeError("Cannot unpack non-dictionary", line, col);
					}
					auto dict = static_cast<DictObject*>(v.ref.get());
					auto keys = std::make_shared<ListObject>();
					auto vals = std::make_shared<ListObject>();
					for (const auto& pair : dict->items) {
						keys->elements.push_back(pair.first);
						vals->elements.push_back(pair.second);
					}
					stack.push_back(Value::List(keys->elements));
					stack.push_back(Value::List(vals->elements));
					break;
				}
				case OpCode::OP_SET_VAR: {
					uint8_t nameIndex = *ip++;
					string name = currentChunk->constants[nameIndex].asString();
					Value val = stack.back();
					if (DEBUGGER_MODE_IS_ENABLED) val.__DEBUGGING__NAME__=name;
					if (!globals->exists(name)) throw NameError("Undefined variable '" + name + "'", line, col);
					Var& v = globals->lookup(name);
					if (v.alias) {
						Value* target = v.alias;
						if (target->isConst) throw ConstError("Cannot assign to const variable '" + name + "' via reference", line, col);
						if (target->isLocked && target->type != val.type) {
							throw TypeError("Cannot change type of locked variable '" + name + "' via reference", line, col);
						}
						bool wasConst = target->isConst;
						bool wasLocked = target->isLocked;
						*target = val;
						target->isConst = wasConst;
						target->isLocked = wasLocked;
					}
					else {
						if (v.isConst) throw ConstError("Cannot assign to const variable '" + name + "'", line, col);
						if (v.isLocked && v.value.type != val.type) throw TypeError("Cannot change type of locked variable '" + name + "'", line, col);
						v.value = val;
					}
					break;
				}
				case OpCode::OP_DEFINE_REF: {
					uint8_t nameIdx = *ip++;
					string newVarName = currentChunk->constants[nameIdx].asString();
					OpCode subOp = static_cast<OpCode>(*ip++);
					if (subOp == OpCode::OP_REF_VAR) {
						string targetName = currentChunk->constants[*ip++].asString();
						Var& targetVar = globals->lookup(targetName);
						Var aliasVar;
						aliasVar.alias = targetVar.alias ? targetVar.alias : &targetVar.value;
						globals->vars[newVarName] = aliasVar;
					}
					else if (subOp == OpCode::OP_REF_INDEX) {
						Value index = pop();
						Value base = pop();
						if (base.type == ValueType::LIST) {
							auto* list = static_cast<ListObject*>(base.ref.get());
							int i = (int)index.asInt();
							Var aliasVar;
							aliasVar.alias = &list->elements[i];
							globals->vars[newVarName] = aliasVar;
						}
					}
					break;
				}
				case OpCode::OP_SET_REF: {
					uint8_t nameIdx = *ip++;
					string varName = currentChunk->constants[nameIdx].asString();
					if (!globals->exists(varName)) throw NameError("Undefined variable"+ varName, line, col);
					OpCode subOp = static_cast<OpCode>(*ip++);
					if (subOp == OpCode::OP_REF_VAR) {
						string target = currentChunk->constants[*ip++].asString();
						Var& targetVar = globals->lookup(target);
						globals->vars[varName].alias = targetVar.alias ? targetVar.alias : &targetVar.value;
					}
					break;
				}
				case OpCode::OP_COLON: {
					Value v = pop();
					Value k = pop();
					vector<std::pair<Value, Value>> p;
					p.push_back({ k, v });
					stack.push_back(Value::Paired(p));
						break;
				}
				case OpCode::OP_TO_STREAM: {
					Value v = pop();
					stack.push_back(prepareIterable(v, line, col));
					break;
				}
				case OpCode::OP_FOR_ITER: {
					uint8_t* jumpOffsetAddr = ip;
					uint16_t offset = (jumpOffsetAddr[0] << 8) | jumpOffsetAddr[1];
					ip += 2;
					uint8_t count = *ip++;
					Value& indexVal = stack.back();
					long long stepCount = indexVal.asInt();
					bool valid = true;
					vector<Value> nextValues;
					for (int i = 0; i < count; i++) {
						int stackPos = stack.size() - 1 - count + i;
						Value& stream = stack[stackPos];
						if (stream.type == ValueType::LIST) {
							auto* list = static_cast<ListObject*>(stream.ref.get());
							if (stepCount >= (long long)list->elements.size()) { valid = false; break; }
							nextValues.push_back(Value::Reference(&list->elements[stepCount]));
						}
						else if (stream.type == ValueType::TUPLE) {
							auto* tuple = static_cast<TupleObject*>(stream.ref.get());
							if (stepCount >= (long long)tuple->elements.size()) { valid = false; break; }
							nextValues.push_back(tuple->elements[stepCount]);
						}
						else if (stream.type == ValueType::SET) {
							auto* s = static_cast<SetObject*>(stream.ref.get());
							if (stepCount>= (long long)s->elements.size()) {valid = false; break;}
							nextValues.push_back(s->elements[stepCount]);
						}
						else if (stream.type == ValueType::STRING) {
							string s = stream.asString();
							if (stepCount >= (long long)s.length()) { valid = false; break; }
							nextValues.push_back(Value::String(string(1, s[stepCount])));
						}
						else if (stream.type == ValueType::VECTOR) {
							auto* vec = static_cast<VectorObject*>(stream.ref.get());
							if (stepCount >= (long long)vec->elements.size()) { valid = false; break; }
							nextValues.push_back(Value::Reference(&vec->elements[stepCount]));
						}
						else if (stream.type == ValueType::RANGE) {
							auto* r = static_cast<RangeObject*>(stream.ref.get());
							double current = r->start + (r->step * stepCount);
							bool inBounds = (r->step > 0) ?(r->endInclusive ? current <= r->end : current < r->end) :
								(r->endInclusive ? current >= r->end : current > r->end);
							if (!inBounds) { valid = false; break; }
							if (r->isFloat) nextValues.push_back(Value::Float(current));
							else nextValues.push_back(Value::Int((long long)current));
						}
						else throw TypeError("Unsupported stream type in iterator", line, col);
					}
					if (valid) {
						for (const auto& val : nextValues) stack.push_back(val);
						stack[stack.size() - 1 - count].iVal++;
					}
					else ip = jumpOffsetAddr + 2 + offset;
					break;
				}
				case OpCode::OP_SKIP_ITER: {
					uint8_t slot = *ip++;
					Value amount = pop();
					if (!amount.isNumber()) throw TypeError("skip amount must be a number", line, col);
					long long skipN = amount.asInt();
					int absoluteSlot = frame->basePointer + slot;
					if (absoluteSlot >= stack.size()) throw IndexError("Skip iterator slot out of bounds", line, col);
					stack[absoluteSlot].iVal += skipN;
					break;
				}
				case OpCode::OP_BUILD_SLICE: {
					Value step = pop();
					Value end = pop();
					Value start = pop();
					auto slice = std::make_shared<SliceObject>(start, end, step);
					Value v;
					v.type = ValueType::SLICE;
					v.ref = slice;
					stack.push_back(v);
					break;
				}
				//CONTAINERS
				case OpCode::OP_BUILD_LIST: {
					uint8_t count = *ip++;
					auto list = std::make_shared<ListObject>();
					if (stack.size() < count) throw EmptyContainerError("Stack underflow during list build", line, col);
					list->elements.resize(count);
					for (int i = count - 1; i >= 0; i--) {
						list->elements[i] = pop();
					}
					stack.push_back(Value::List(list->elements));
					break;
				}
				case OpCode::OP_BUILD_DICT: {
					uint8_t count = *ip++;
					auto dict = std::make_shared<DictObject>();
					for (int i = 0; i < count; i++) {
						Value pairVal = pop();
						if (pairVal.type == ValueType::PAIRED) {
							auto* pObj = static_cast<PairedObject*>(pairVal.ref.get());
							for (const auto& entry : pObj->pairs) {
								Value key = entry.first;
								if (key.type == ValueType::LIST || key.type == ValueType::SET) {
									key = deepCopy(key);
									key.isConst = true;
								}
								dict->items[key] = entry.second;
							}
						}
					}
					stack.push_back(Value::Dict(dict->items));
					break;
				}
				case OpCode::OP_BUILD_SET: {
					uint8_t count = *ip++;
					auto set = std::make_shared<SetObject>();
					for (int i = 0; i < count; i++) setAdd(set->elements, pop());
					stack.push_back(Value::Set(set->elements));
					break;
				}
				case OpCode::OP_BUILD_TUPLE: {
					uint8_t count = *ip++;
					vector<Value> elems(count);
					for (int i = count - 1; i >= 0; i--) elems[i] = pop();
					stack.push_back(Value::Tuple(elems));
					break;
				}
				case OpCode::OP_BUILD_VECTOR: {
					uint8_t count = *ip++;
					vector<Value> elems;
					elems.resize(count);
					for (int i = count - 1; i >= 0; i--) {
						Value v = pop();
						if (!v.isNumber()) throw TypeError("Vector elements must be numbers", line, col);
						elems[i] = v;
					}
					stack.push_back(Value::Vector(elems));
					break;
				}
				case OpCode::OP_BUILD_RANGE: {
					Value step = pop();
					Value end = pop();
					Value start = pop();
					bool isFloat = (start.type == ValueType::FLOAT || end.type == ValueType::FLOAT || step.type == ValueType::FLOAT);
					stack.push_back(Value::Range(start.asFloat(), end.asFloat(), step.asFloat(), true, false, isFloat));
					break;
				}
				case OpCode::OP_SHALLOW_COPY: {
					Value v = pop();
					Value res = shallowCopy(v);
					res.isConst = false;
					res.isLocked = false;
					stack.push_back(res);
					break;
				}
				case OpCode::OP_DEEP_COPY: {
					Value v = pop();
					Value res = deepCopy(v);
					res.isConst = false;
					res.isLocked = false;
					stack.push_back(res);
					break;
				}
				case OpCode::OP_GET_INDEX: {
					Value index = pop();
					Value base = pop();
					auto getSliceIndices = [&](size_t rawLen) -> std::vector<long long> {
						auto* s = static_cast<SliceObject*>(index.ref.get());
						long long len = (long long)rawLen;
						long long step = 1;
						if (s->step.type == ValueType::INT) step = s->step.asInt();
						if (step == 0) throw ValueError("Slice step cannot be zero", line, col);
						long long start, end;
						if (step > 0) {
							start = (s->start.type == ValueType::INT) ? s->start.asInt() : 0;
							end = (s->end.type == ValueType::INT) ? s->end.asInt() : len;
						}
						else {
							start = (s->start.type == ValueType::INT) ? s->start.asInt() : len - 1;
							end = (s->end.type == ValueType::INT) ? s->end.asInt() : -1;
						}
						if (s->start.type == ValueType::INT && start < 0) start += len;
						if (s->end.type == ValueType::INT && end < 0) end += len;
						if (step > 0) {
							if (start < 0) start = 0;
							if (end < 0) end = 0;
							if (start > len) start = len;
							if (end > len) end = len;
						}
						else {
							if (start > len - 1) start = len - 1;
							if (end > len - 1) end = len - 1;
							if (start < -1) start = -1;
							if (end < -1) end = -1;
						}
						std::vector<long long> result;
						if (step > 0) {
							for (long long i = start; i < end; i += step)
								if (i >= 0 && i < len) result.push_back(i);
						}
						else {
							for (long long i = start; i > end; i += step)
								if (i >= 0 && i < len) result.push_back(i);
						}
						return result;
					};
					switch (base.type){
						case ValueType::LIST: {
							auto* list = static_cast<ListObject*>(base.ref.get());
							if (index.type == ValueType::SLICE) {
								auto indices = getSliceIndices(list->elements.size());
								auto newList = std::make_shared<ListObject>();
								newList->elements.reserve(indices.size());
								for (long long i : indices) newList->elements.push_back(list->elements[i]);
								stack.push_back(Value::List(newList->elements));
							}
							else {
								if (!index.isNumber()) throw TypeError("List index must be int or slice", line, col);
								long long idx = index.asInt();
								if (idx < 0) idx += list->elements.size();
								if (idx < 0 || idx >= (long long)list->elements.size()) throw IndexError("List index out of range", line, col);
								stack.push_back(list->elements[idx]);
							}
							break;
						}
						case ValueType::SET: {
							auto* s = static_cast<SetObject*>(base.ref.get());
							if (index.type == ValueType::SLICE) {
								auto indices = getSliceIndices(s->elements.size());
								auto newSet = std::make_shared<SetObject>();
								for (long long i : indices) newSet->elements.push_back(s->elements[i]);
								stack.push_back(Value::Set(newSet->elements));
							}
							else {
								if (!index.isNumber()) throw TypeError("Set index must be int or slice", line, col);
								long long idx = index.asInt();
								if (idx < 0) idx += s->elements.size();
								if (idx < 0 || idx >= (long long)s->elements.size()) throw IndexError("Set index out of range", line, col);
								stack.push_back(s->elements[idx]);
							}
							break;
						}
						case ValueType::VECTOR: {
							auto* vec = static_cast<VectorObject*>(base.ref.get());
							if (index.type == ValueType::SLICE) {
								auto indices = getSliceIndices(vec->elements.size());
								std::vector<Value> newElems;
								newElems.reserve(indices.size());
								for (long long i : indices) newElems.push_back(vec->elements[i]);
								stack.push_back(Value::Vector(newElems));
							}
							else {
								if (!index.isNumber()) throw TypeError("Vector index must be int or slice", line, col);
								long long idx = index.asInt();
								if (idx < 0) idx += vec->elements.size();
								if (idx < 0 || idx >= (long long)vec->elements.size()) throw IndexError("Vector index out of range", line, col);
								stack.push_back(vec->elements[idx]);
							}
							break;
						}
						case ValueType::TUPLE: {
							auto* tuple = static_cast<TupleObject*>(base.ref.get());
							if (index.type == ValueType::SLICE) {
								auto indices = getSliceIndices(tuple->elements.size());
								std::vector<Value> newElems;
								newElems.reserve(indices.size());
								for (long long i : indices) newElems.push_back(tuple->elements[i]);
								stack.push_back(Value::Tuple(newElems));
							}
							else {
								if (!index.isNumber()) throw TypeError("Tuple index must be int or slice", line, col);
								long long idx = index.asInt();
								if (idx < 0) idx += tuple->elements.size();
								if (idx < 0 || idx >= (long long)tuple->elements.size()) throw IndexError("Tuple index out of range", line, col);
								stack.push_back(tuple->elements[idx]);
							}
							break;
						}
						case ValueType::DICT: {
							auto* dict = static_cast<DictObject*>(base.ref.get());
							if (dict->items.count(index) == 0) throw KeyError("Key not found: " + valueToString(index), line, 0);
							stack.push_back(dict->items.at(index));
							break;
						}
						case ValueType::STRING: {
							string s = base.asString();
							if (index.type == ValueType::SLICE) {
								auto indices = getSliceIndices(s.length());
								string newStr = "";
								newStr.reserve(indices.size());
								for (long long i : indices) newStr += s[i];
								stack.push_back(Value::String(newStr));
							}
							else {
								if (!index.isNumber()) throw TypeError("String index must be int or slice", line, col);
								long long idx = index.asInt();
								if (idx < 0) idx += s.length();
								if (idx < 0 || idx >= (long long)s.length()) throw IndexError("String index out of range", line, col);
								stack.push_back(Value::String(string(1, s[idx])));
							}
							break;
						}
						case ValueType::RANGE: {
							auto* rng = static_cast<RangeObject*>(base.ref.get());
							if (!index.isNumber()) throw TypeError("Range index must be a number", line, col);
							long long idx = index.asInt();
							long long len = (long long)((rng->end - rng->start) / rng->step);
							if (!rng->endInclusive) {
								if ((rng->step > 0 && rng->end > rng->start) || (rng->step < 0 && rng->end < rng->start))
									len = (long long)ceil((rng->end - rng->start) / rng->step);
								else len = 0;
							}
							else len++;
							if (idx < 0) throw IndexError("Range index cannot be negative", line, col);
							double val = rng->start + (idx * rng->step);
							if (rng->step > 0 && val >= rng->end && !rng->endInclusive) throw IndexError("Range index out of range", line, col);
							stack.push_back(rng->isFloat ? Value::Float(val) : Value::Int((long long)val));
							break;
						}
						default: throw TypeError("Object is not subscriptable", line, col); break;
					}
					break;
				}
				case OpCode::OP_SET_INDEX: {
					Value val = pop();
					Value index = pop();
					Value base = pop();
					if (base.type == ValueType::LIST) {
						auto* list = static_cast<ListObject*>(base.ref.get());
						if (!index.isNumber()) throw TypeError("List index must be a number", line, col);
						long long idx = index.asInt();
						if (idx < 0) idx += list->elements.size();
						if (idx < 0 || idx >= (long long)list->elements.size()) throw IndexError("List assignment index out of range", line, col);
						list->elements[idx] = val;
					}
					else if (base.type == ValueType::DICT) {
						auto* dict = static_cast<DictObject*>(base.ref.get());
						if (index.type == ValueType::LIST || index.type == ValueType::SET) {
							index = deepCopy(index);
							index.isConst = true;
						}
						dict->items[index] = val;
					}
					else if (base.type == ValueType::STRING) {
						auto* str = static_cast<StringObject*>(base.ref.get());
						if (!index.isNumber()) throw TypeError("String index must be a number", line, col);
						long long idx = index.asInt();
						if (idx < 0) idx += str->value.size();
						if (idx < 0 || idx >= (long long)str->value.size()) throw IndexError("String assignment index out of range", line, col);
						str->value[idx] = val.asString()[0];
					}
					else if (base.type == ValueType::TUPLE) 
					throw MutationError("Tuple object does not support item assignment", line, col);
					else throw MutationError("Object does not support item assignment", line, col);
					stack.push_back(val);
					break;
				}
				// ENVOKE
				case OpCode::OP_INVOKE: {
					uint8_t nameIdx = *ip++;
					uint8_t argCount = *ip++;
					string methodName = currentChunk->constants[nameIdx].asString();
					vector<Expr*> dummyArgs(argCount);
					for (int i = argCount - 1; i >= 0; i--) dummyArgs[i] = new ValueExpr(pop());
					Value targetVal = pop();
					Expr* dummyObject = new ValueExpr(targetVal);
					MethodCallExpr mockAST(dummyObject, methodName, dummyArgs);
					mockAST.line = line;
					if (!methodResolver) throw EnvironmentError("VM methodResolver bridge not initialized.", line, col);
					Value result = methodResolver(&mockAST);
					delete dummyObject;
					for (auto* a : dummyArgs) delete a;
					stack.push_back(result);
					break;
				}
				case OpCode::OP_TRY_ENTER: {
					uint8_t cHi = *ip++; uint8_t cLo = *ip++;
					uint8_t fHi = *ip++; uint8_t fLo = *ip++;
					int catchOffset = (cHi << 8) | cLo;
					int finallyOffset = (fHi << 8) | fLo;
					ExceptionHandler h;
					h.stackDepth = stack.size();
					int currentOffset = (int)(ip - 4 - currentChunk->code.data());
					h.catchAddress = currentOffset + catchOffset;
					h.finallyAddress = (finallyOffset == 0) ? -1 : currentOffset + finallyOffset;
					frame->handlerStack.push_back(h);
					break;
				}

				case OpCode::OP_TRY_EXIT: {
					if (!frame->handlerStack.empty()) frame->handlerStack.pop_back();
					break;
				}
				case OpCode::OP_THROW: {
					Value typeVal = pop();
					Value msgVal = pop();
					string t = typeVal.asString();
					string m = msgVal.asString();
					static const std::unordered_map<std::string, std::function<void(string, int, int)>> errorFactory = {
						// Base Categories
						{"InternalError", [](string m, int l, int c) { throw InternalError(m, l, c); }},
						{"ControlFlowError", [](string m, int l, int c) { throw ControlFlowError(m, l, c); }},
						{"ParseError", [](string m, int l, int c) { throw ParseError(m, l, c); }},
						{"RuntimeError", [](string m, int l, int c) { throw RuntimeError(m, l, c); }},
						{"Warning", [](string m, int l, int c) { throw Warning(m, l, c); }},

						// Runtime Errors
						{"NameError", [](string m, int l, int c) { throw NameError(m, l, c); }},
						{"AttributeError", [](string m, int l, int c) { throw AttributeError(m, l, c); }},
						{"TypeError", [](string m, int l, int c) { throw TypeError(m, l, c); }},
						{"ArgumentError", [](string m, int l, int c) { throw ArgumentError(m, l, c); }},
						{"ValueError", [](string m, int l, int c) { throw ValueError(m, l, c); }},
						{"ConstError", [](string m, int l, int c) { throw ConstError(m, l, c); }},
						{"OwnershipError", [](string m, int l, int c) { throw OwnershipError(m, l, c); }},
						{"IndexError", [](string m, int l, int c) { throw IndexError(m, l, c); }},
						{"KeyError", [](string m, int l, int c) { throw KeyError(m, l, c); }},
						{"RangeError", [](string m, int l, int c) { throw RangeError(m, l, c); }},
						{"AssertionError", [](string m, int l, int c) { throw AssertionError(m, l, c); }},
						{"RecursionError", [](string m, int l, int c) { throw RecursionError(m, l, c); }},
						{"ImportError", [](string m, int l, int c) { throw ImportError(m, l, c); }},
						{"IOError", [](string m, int l, int c) { throw IOError(m, l, c); }},
						{"MathError", [](string m, int l, int c) { throw MathError(m, l, c); }},
						{"CastError", [](string m, int l, int c) { throw CastError(m, l, c); }},
						{"IteratorError", [](string m, int l, int c) { throw IteratorError(m, l, c); }},
						{"TimeoutError", [](string m, int l, int c) { throw TimeoutError(m, l, c); }},
						{"MemoryError", [](string m, int l, int c) { throw MemoryError(m, l, c); }},
						{"SystemError", [](string m, int l, int c) { throw SystemError(m, l, c); }},

						// Specific Subtypes
						{"EmptyContainerError", [](string m, int l, int c) { throw EmptyContainerError(m, l, c); }},
						{"MutationError", [](string m, int l, int c) { throw MutationError(m, l, c); }},
						{"ModuleNotFoundError", [](string m, int l, int c) { throw ModuleNotFoundError(m, l, c); }},
						{"CircularImportError", [](string m, int l, int c) { throw CircularImportError(m, l, c); }},
						{"InvalidImportError", [](string m, int l, int c) { throw InvalidImportError(m, l, c); }},
						{"FileNotFoundError", [](string m, int l, int c) { throw FileNotFoundError(m, l, c); }},
						{"PermissionError", [](string m, int l, int c) { throw PermissionError(m, l, c); }},
						{"EOFError", [](string m, int l, int c) { throw EOFError(m, l, c); }},
						{"FileClosedError", [](string m, int l, int c) { throw FileClosedError(m, l, c); }},
						{"DivisionByZeroError", [](string m, int l, int c) { throw DivisionByZeroError(m, l, c); }},
						{"OverflowError", [](string m, int l, int c) { throw OverflowError(m, l, c); }},
						{"UnderflowError", [](string m, int l, int c) { throw UnderflowError(m, l, c); }},
						{"DomainError", [](string m, int l, int c) { throw DomainError(m, l, c); }},
						{"OSError", [](string m, int l, int c) { throw OSError(m, l, c); }},
						{"EnvironmentError", [](string m, int l, int c) { throw EnvironmentError(m, l, c); }},
						{"SignalError", [](string m, int l, int c) { throw SignalError(m, l, c); }},
						{"DeprecationWarning", [](string m, int l, int c) { throw DeprecationWarning(m, l, c); }},
						{"RuntimeWarning", [](string m, int l, int c) { throw RuntimeWarning(m, l, c); }},
						{"ImportWarning", [](string m, int l, int c) { throw ImportWarning(m, l, c); }}
					};
					auto it = errorFactory.find(t);
					if (it != errorFactory.end()) it->second(m, line, col);
					else throw LangError(t, m, -1, line, col);
					break;
				}
				case OpCode::OP_CATCH: {
					uint8_t count = *ip++;
					Value errorVal = stack.back();
					bool match = false;
					static const std::unordered_map<std::string, std::string> parentMap = {
						// MathError Subtree
						{"DivisionByZeroError", "MathError"}, {"OverflowError", "MathError"},
						{"UnderflowError", "MathError"}, {"DomainError", "MathError"},
						{"MathError", "RuntimeError"},

						// IOError Subtree
						{"FileNotFoundError", "IOError"}, {"PermissionError", "IOError"},
						{"EOFError", "IOError"}, {"FileClosedError", "IOError"},
						{"IOError", "RuntimeError"},

						// ImportError Subtree
						{"ModuleNotFoundError", "ImportError"}, {"CircularImportError", "ImportError"},
						{"InvalidImportError", "ImportError"}, {"ImportError", "RuntimeError"},

						// ValueError/ConstError Subtrees
						{"EmptyContainerError", "ValueError"}, {"ValueError", "RuntimeError"},
						{"MutationError", "ConstError"}, {"ConstError", "RuntimeError"},

						// SystemError Subtree
						{"OSError", "SystemError"}, {"EnvironmentError", "SystemError"},
						{"SignalError", "SystemError"}, {"SystemError", "RuntimeError"},

						// ParseError Subtree
						{"SyntaxError", "ParseError"}, {"IndentationError", "ParseError"},
						{"UnexpectedTokenError", "ParseError"}, {"UnterminatedLiteralError", "ParseError"},
						{"ParseError", "Error"},

						// ControlFlowError Subtree
						{"ReturnSignal", "ControlFlowError"}, {"BreakSignal", "ControlFlowError"},
						{"ContinueSignal", "ControlFlowError"}, {"ControlFlowError", "Error"},

						// Warning Subtree
						{"DeprecationWarning", "Warning"}, {"RuntimeWarning", "Warning"},
						{"ImportWarning", "Warning"}, {"Warning", "Error"},

						// Direct RuntimeError Children
						{"NameError", "RuntimeError"}, {"AttributeError", "RuntimeError"},
						{"TypeError", "RuntimeError"}, {"ArgumentError", "RuntimeError"},
						{"OwnershipError", "RuntimeError"}, {"IndexError", "RuntimeError"},
						{"KeyError", "RuntimeError"}, {"RangeError", "RuntimeError"},
						{"AssertionError", "RuntimeError"}, {"RecursionError", "RuntimeError"},
						{"CastError", "RuntimeError"}, {"IteratorError", "RuntimeError"},
						{"TimeoutError", "RuntimeError"}, {"MemoryError", "RuntimeError"},

						// Roots
						{"RuntimeError", "Error"}, {"InternalError", "Error"}
					};
					if (errorVal.type == ValueType::ERROR) {
						auto* errObj = static_cast<ErrorObject*>(errorVal.ref.get());
						for (int i = 0; i < count; i++) {
							uint8_t typeIdx = *ip++;
							if (match) continue;
							string catchType = currentChunk->constants[typeIdx].asString();
							string currentType = errObj->errType;
							while (true) {
								if (currentType == catchType) { match = true; break; }
								if (catchType == "Error") { match = true; break; }
								auto it = parentMap.find(currentType);
								if (it == parentMap.end()) break;
								currentType = it->second;
							}
						}
					}
					else ip += count;
					stack.push_back(Value::Bool(match));
					break;
				}
				case OpCode::OP_RETHROW: {
					Value errorVal = pop();
					if (errorVal.type == ValueType::ERROR) {
						auto* errObj = static_cast<ErrorObject*>(errorVal.ref.get());
						throw LangError(errObj->errType, errObj->message, errObj->code, errObj->line, errObj->col);
					}
					break;
				}
				case OpCode::OP_END_FINALLY: {
					if (isHandlingError) {
						isHandlingError = false;
						auto* errObj = static_cast<ErrorObject*>(pendingError.ref.get());
						throw LangError(errObj->errType, errObj->message, errObj->code, errObj->line, errObj->col);
					}
					if (isReturning) {
						isReturning = false;
						stack.push_back(pendingReturn);
						goto execute_return;
					}
					break;
				}
				case OpCode::OP_RETURN: {
				execute_return:
					if (!frame->handlerStack.empty()) {
						ExceptionHandler& h = frame->handlerStack.back();
						if (h.finallyAddress != -1 && !h.isInsideFinally) {
							isReturning = true;
							pendingReturn = pop();
							h.isInsideFinally = true;
							ip = currentChunk->code.data() + h.finallyAddress;
							break;
						}
					}
					Value result = pop();
					FunctionObject* func = frame->function;
					if (func) {
						if (result.type == ValueType::NOTYPE) {
							if (!func->defaultRetArgs.empty()) {
								result = executeDefault(func->defaultRetArgs[0], line);
							}
							else if (func->returnType != ValueType::NOTYPE) {
								result = Value::None();
							}
							else {
								result = Value::None();
							}
						}
						if (func->returnType != ValueType::NOTYPE && result.type != func->returnType) {
							bool converted = false;
							if (func->returnType == ValueType::FLOAT && result.type == ValueType::INT) {
								result = Value::Float((double)result.asInt());
								converted = true;
							}
							else if (func->returnType == ValueType::INT && result.type == ValueType::FLOAT) {
								result = Value::Int((long long)result.asFloat());
								converted = true;
							}
							else if (func->returnType == ValueType::INT && result.type == ValueType::BIGINT) {
								converted = true;
							}
							else if (func->returnType == ValueType::BIGINT && result.type == ValueType::INT) {
								result = Value::BigInt(result.asInt());
								converted = true;
							}
							if (!converted) {
								string typeName = "";
								switch (func->returnType) {
								case ValueType::INT:    typeName = "int"; break;
								case ValueType::FLOAT:  typeName = "float"; break;
								case ValueType::BOOL:   typeName = "bool"; break;
								case ValueType::STRING: typeName = "string"; break;
								case ValueType::LIST:   typeName = "list"; break;
								case ValueType::SET:    typeName = "set"; break;
								case ValueType::DICT:   typeName = "dict"; break;
								case ValueType::TUPLE:  typeName = "tuple"; break;
								case ValueType::VECTOR: typeName = "vector"; break;
								case ValueType::RANGE:  typeName = "range"; break;
								default: break;
								}
								if (!typeName.empty() && globals->exists(typeName)) {
									Value ctor = globals->get(typeName);
									vector<Value> args;
									if (result.type != ValueType::NONE && result.type != ValueType::NOTYPE) {
										args.push_back(result);
									}
									try {
										if (ctor.type == ValueType::NATIVE_FUNCTION) {
											auto* nat = static_cast<NativeFunctionObject*>(ctor.ref.get());
											result = nat->func(args, line, 0);
											converted = true;
										}
										else if (ctor.type == ValueType::OVERLOAD) {
											auto* ov = static_cast<OverloadObject*>(ctor.ref.get());
											for (const auto& v : ov->overloads) {
												if (v.type == ValueType::NATIVE_FUNCTION) {
													auto* nat = static_cast<NativeFunctionObject*>(v.ref.get());
													try {
														result = nat->func(args, line, 0);
														converted = true;
														break;
													}
													catch (...) {}
												}
											}
										}
									}
									catch (...) {
									
									}
								}
							}
							if (!converted && result.type != ValueType::NONE) {
								throw TypeError("Return type mismatch. Expected " + std::to_string((int)func->returnType) +
									" Got " + std::to_string((int)result.type), line, col);
							}
						}
						if (func->returnsConst) result.isConst = true;
						if (func->isCached) func->cache[frame->cacheKey] = result;
					}
					int returnSlot = frame->basePointer;
					frames.pop_back();
					if (frames.empty()) {
						stack.push_back(result);
						return;
					}
					frame = &frames.back();
					ip = frame->ip;
					stack.resize(returnSlot);
					stack.push_back(result);
					break;
				}
				default: throw UnexpectedTokenError("Unknown OpCode encountered", line, col);
				}
			}
			catch (const LangError& e) {
				if (frame->handlerStack.empty()) {
					std::cerr << "Uncaught " << e.type << ": " << e.message << "\n";
					throw;
				}
				ExceptionHandler h = frame->handlerStack.back();
				frame->handlerStack.pop_back();
				while ((int)stack.size() > h.stackDepth) stack.pop_back();
				auto errObj = std::make_shared<ErrorObject>(e.type, e.message, e.code, e.line, e.col);
				Value errVal = Value::Error(errObj);
				if (h.catchAddress != -1) {
					stack.push_back(errVal);
					Chunk* currentChunk = frame->function ? frame->function->chunk : &chunk;
					ip = currentChunk->code.data() + h.catchAddress;
				}
				else if (h.finallyAddress != -1) {
					pendingError = errVal;
					isHandlingError = true;
					Chunk* currentChunk = frame->function ? frame->function->chunk : &chunk;
					ip = currentChunk->code.data() + h.finallyAddress;
				}
				else {
					throw;
				}
			}
		}
	}
private:
	void callValue(Value callee, int argCount, int line, int col) {
		FunctionObject* function = nullptr;
		NativeFunctionObject* nativeObj = nullptr;
		if (callee.type == ValueType::FUNCTION) {
			function = static_cast<FunctionObject*>(callee.ref.get());
		}
		else if (callee.type == ValueType::NATIVE_FUNCTION) {
			nativeObj = static_cast<NativeFunctionObject*>(callee.ref.get());
		}
		else if (callee.type == ValueType::OVERLOAD) {
			// [KEEP YOUR EXACT OVERLOAD LOGIC HERE]
			// You need to peek at the stack args without popping them yet.
			// Use stack[stack.size() - argCount + i] to access args.
			// ... (Copy your overload selection loop) ...
			// function = candidate;
		}
		else throw TypeError("Object is not callable", line, col);
		if (nativeObj) {
			vector<Value> args;
			for (int i = 0; i < argCount; i++) args.push_back(pop());
			std::reverse(args.begin(), args.end());
			stack.push_back(nativeObj->func(args, line, 0));
			return;
		}
		
		vector<Value> providedArgs;
		providedArgs.resize(argCount);
		for (int i = argCount - 1; i >= 0; i--) providedArgs[i] = pop();
		// Debugging: Verify we have params
		//std::cout << "DEBUG: Call " << argCount << " args. Func expects " << function->params.size() << "\n";
		vector<Value> finalArgs;
		finalArgs.resize(function->params.size());
		size_t argIndex = 0;
		for (size_t i = 0; i < function->params.size(); i++) {
			const ParamSpec& p = function->params[i];
			Value argVal = Value::None();
			if (p.isKwargs) {
				auto dict = std::make_shared<DictObject>();
				while (argIndex < providedArgs.size()) {
					Value v = providedArgs[argIndex];
					if (v.type != ValueType::PAIRED) throw SyntaxError("Positional argument after keyword args", line, col);
					auto* pairObj = static_cast<PairedObject*>(v.ref.get());
					for (const auto& entry : pairObj->pairs) {
						Value key = entry.first;
						Value val = entry.second;
						if (p.mode == CopyMode::DEEP) {
							key = deepCopy(key);
							val = deepCopy(val);
						}
						dict->items[key] = val;
					}
					argIndex++;
				}
				argVal = Value::Dict(dict->items);
				if (p.isConst) argVal.isConst = true;
			}
			else if (p.isVariadic) {
				vector<Value> tupleItems;
				while (argIndex < providedArgs.size()) {
					if (providedArgs[argIndex].type == ValueType::PAIRED) break;
					Value v = providedArgs[argIndex];
					if (p.mode == CopyMode::DEEP) v = deepCopy(v);
					tupleItems.push_back(v);
					argIndex++;
				}
				argVal = Value::Tuple(tupleItems);
			}
			else {
				bool foundValue = false;
				if (argIndex < providedArgs.size() && providedArgs[argIndex].type != ValueType::PAIRED) {
					if (providedArgs[argIndex].type == ValueType::OMIT_MARKER) {
						if (p.defaultValue != nullptr) {
							argVal = executeDefault(p.defaultValue, line); // <--- EXECUTE AST
							foundValue = true;
						}
						else throw ArgumentError("Argument '" + p.name + "' cannot be omitted.", line, col);
					}
					else {
						argVal = providedArgs[argIndex];
						if (p.mode == CopyMode::DEEP) argVal = deepCopy(argVal);
						else if (p.mode == CopyMode::SHALLOW) argVal = shallowCopy(argVal);
						foundValue = true;
					}
					argIndex++;
				}
				if (!foundValue) {
					if (p.defaultValue != nullptr) argVal = executeDefault(p.defaultValue, line);
					else throw ArgumentError("Missing required argument '" + p.name + "'", line, col);
				}
				if (p.type != ValueType::NOTYPE && argVal.type != p.type) {
					bool mismatch = true;
					if (p.type == ValueType::FLOAT && argVal.type == ValueType::INT){ 
						argVal = Value::Float((double)argVal.asInt());
						mismatch = false;
					}
					else if (p.type == ValueType::INT && argVal.type == ValueType::FLOAT){
						argVal = Value::Int((long long)argVal.asFloat());
						mismatch = false;
					}
					else if (p.type == ValueType::INT && argVal.type == ValueType::BIGINT) {
						mismatch = false;
					}
					else if (p.type == ValueType::BIGINT && argVal.type == ValueType::INT) {
						argVal = Value::BigInt(argVal.asInt());
						mismatch = false;
					}
					else if (argVal.type == ValueType::OMIT_MARKER) {
						mismatch = false;
					}
					if (mismatch) {
						throw TypeError("Type mismatch for '" + p.name + "'. Expected " + std::to_string((int)p.type) + " got " +
						std::to_string((int)argVal.type), line, col);
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
		if (argIndex < providedArgs.size()) {
			bool acceptsMore = !function->params.empty() && (function->params.back().isVariadic || function->params.back().isKwargs);
			if (!acceptsMore) throw ArgumentError("Too many arguments passed to function.", line, col);
		}
		if (frame) frame->ip = ip;
		CallFrame newFrame;
		newFrame.function = function;
		newFrame.ip = function->chunk->code.data();
		newFrame.basePointer = stack.size();
		if (function->isCached) newFrame.cacheKey = finalArgs;
		for (const auto& v : finalArgs) stack.push_back(v);
		frames.push_back(newFrame);
		frame = &frames.back();
		ip = frame->ip;
	}
	Value executeDefault(Expr* expr, int line) {
		if (!expr) return Value::None();
		Chunk tempChunk;
		ByteCodeCompiler compiler(&tempChunk);
		compiler.compile(expr);
		compiler.emitByte(OpCode::OP_RETURN, line, 0);
		VM tempVM;
		tempVM.globals = this->globals;
		tempVM.methodResolver = this->methodResolver;
		tempVM.run(tempChunk);
		if (tempVM.stack.empty()) return Value::None();
		return tempVM.stack.back();
	}
	Value prepareIterable(Value collection, int line, int col) {
		if (collection.type == ValueType::LIST || collection.type == ValueType::SET ||
			collection.type == ValueType::TUPLE ||collection.type == ValueType::STRING ||
			collection.type == ValueType::VECTOR ||collection.type == ValueType::RANGE) {
			return collection;
		}
		if (collection.type == ValueType::REFERENCE) {
			return *collection.ptr;
		}
		auto list = std::make_shared<ListObject>();
		if (collection.type == ValueType::DICT) {
			auto* dict = static_cast<DictObject*>(collection.ref.get());
			for (const auto& pair : dict->items) list->elements.push_back(pair.first);
		}
		else throw TypeError("Object is not iterable", line, col);
		return Value::List(list->elements);
	}
	Value pop() {
		if (stack.empty()) throw UnderflowError("Stack underflow", 0, 0);
		Value v = stack.back();
		stack.pop_back();
		return v;
	}
};