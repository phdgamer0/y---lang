#pragma once
#ifndef YLANG_COMPILER_HPP
#define YLANG_COMPILER_HPP
#include "objects.hpp"
struct ByteCodeCompiler {
#define DEBUG_TRACE_EXECUTION

	// ByteCodeCompiler logic
	bool isCTLE = false;
	bool tryEvaluateCompileTimeLoop(Stmt *s);
	Chunk *chunk;
	vector<LoopContext> loopStack;
	vector<Local> locals;
	int scopeDepth = 0;
	bool isReplMode = false;
	Interpreter *interp = nullptr;
	bool isCompileOnly = false;
	bool returnsRef = false;
	bool returnsConst = false;
	std::map<string, Value> knownGlobals;

	// exact integer from the literal text; n->val is a double and loses precision above 2^53
	static Value numberLiteralValue(NumberExpr *n);
	ValueType tryExtractType(Expr *e);
	ByteCodeCompiler(Chunk *c);
	int resolveLocal(string name);
	void addLocal(string name);
	void beginScope();
	void endScope(int line, int col);
	void compile(Expr *e);
	void compileBinary(BinExpr *b);
	void compileLogical(BinExpr *l);
	void emitByte(OpCode op, int line, int col);
	void emitConstant(Value v, int line, int col);
	int emitJump(OpCode instruction, int line, int col);
	void patchJump(int offset);
	void compileWithMode(Expr *expr, int line, int col);
	void compileStmt(Stmt *s);
	void emitIdentifier(OpCode op, const string &name, int line, int col);
	void emitLoop(int loopStart, int line, int col);
	void emitFunction(FuncStmt *f, bool isMethod = false, const vector<Stmt *> &fieldInits = {});
	bool tryExtractConstant(Expr *e, Value &outVal);
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
		if (gc_alloc_count > 50000) {                                     \
			gc_collect();                                                  \
			gc_alloc_count = 0;                                            \
		}                                                                 \
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
std::pair<bool, Value> tryCastDunder(Value v, const std::string &dunderName, int l, int c, std::shared_ptr<Env> globals, std::function<Value(MethodCallExpr *)> methodResolver, std::function<void(std::string, std::vector<std::string>, std::shared_ptr<Env>)> importResolver);

#endif // YLANG_COMPILER_HPP
