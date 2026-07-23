#pragma once
#ifndef YLANG_ERRORS_HPP
#define YLANG_ERRORS_HPP
#include <exception>
#include <string>
using std::string;
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

#endif // YLANG_ERRORS_HPP
