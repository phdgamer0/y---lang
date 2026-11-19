#pragma once
#ifndef YLANG_TRANSPILER_HPP
#define YLANG_TRANSPILER_HPP

#include "ast.hpp"
#include "objects.hpp"
#include <string>
#include <vector>

enum class ConstType {
	INT,
	FLOAT,
	BOOL,
	STRING,
	VALUE
};

struct ConstSymbol {
	std::string cppName;
	ConstType type;
	bool isConstexpr;
};

struct UserFunctionInfo {
	std::string name;
	std::string cppName;
	std::string returnTypeStr;
	ValueType returnType;
	bool returnsRef;
	bool returnsConst;
	std::vector<ParamSpec> params;
	std::vector<std::string> paramTypeStrs;
	int variadicIdx = -1;
	int kwargsIdx = -1;
};

struct NativeVar {
	std::string cppType; // "" marks a name kept in current_env that shadows outer locals
	bool isRef;
	bool isConst;
	std::string cppName = ""; // C++ identifier; each `let` gets its own (x_3), so a redeclaration can change type
};

// one declaration (param or `let`) inside the function being transpiled
struct LocalDecl {
	enum Kind { ENV, NATIVE, VALUE, CONST } kind = ENV;
	std::string type = "?"; // "?" not known yet, "long long" / "double" / "bool", "Value" = dynamic
	bool locked = false, escapes = false, fixed = false, incDec = false;
	Expr *init = nullptr;
	std::vector<struct AssignStmt *> stores;
};

struct CppTranspiler {
	Chunk *chunk;
	int scopeDepth = 0;
	int varCounter = 0;
	int loopIdCounter = 0;
	std::vector<std::pair<int, bool>> loopStack;
	std::string output = "";

	std::vector<std::unordered_map<std::string, ConstSymbol>> constScopes;
	bool currentFuncReturnsConst = false;
	bool currentFuncReturnsRef = false;
	std::string currentFuncNativeReturnType = "";
	Expr *currentFuncDefaultRet = nullptr; // `-> int(5)`: returned by a bare return or falling off the end

	std::unordered_map<std::string, UserFunctionInfo> userFunctions;
	std::string userFunctionDecls = "";
	std::string userFunctionDefs = "";
	std::string userFunctionRegistrations = "";
	bool hasUserMain = false;

	std::vector<std::unordered_map<std::string, NativeVar>> nativeScopes;

	// Store generated C++ constants block
	std::string constantsBlock = "";

	CppTranspiler(Chunk *c) : chunk(c) {
		constScopes.push_back({});
	}

	const ConstSymbol *lookupConst(const std::string &name) const {
		for (auto it = constScopes.rbegin(); it != constScopes.rend(); ++it) {
			auto found = it->find(name);
			if (found != it->end())
				return &found->second;
		}
		return nullptr;
	}

	const NativeVar *lookupNativeVar(const std::string &name) const {
		for (auto it = nativeScopes.rbegin(); it != nativeScopes.rend(); ++it) {
			auto found = it->find(name);
			if (found != it->end())
				return found->second.cppType.empty() ? nullptr : &found->second;
		}
		return nullptr;
	}

	// type inference for function locals (see analyzeFunction)
	std::vector<LocalDecl> decls;
	std::unordered_map<const Stmt *, int> letDecl;
	std::unordered_map<const Expr *, int> varDecl;
	std::vector<int> paramDecl;
	void analyzeFunction(struct FuncStmt *func);
	std::string exprType(Expr *e, bool checked);
	std::string emitNative(Expr *e, bool checked);
	std::string operand(Expr *e);

	// block EnvRestorers are emitted with a marker and dropped in finalize when the block declares nothing in current_env
	std::vector<int> envScopeStack;
	std::vector<char> envNeeded;
	std::string beginScope(const std::string &restorerVar);
	void endScope(bool hadRestorer);
	void markEnvDecl() {
		if (!envScopeStack.empty())
			envNeeded[envScopeStack.back()] = 1;
	}
	void shadowWithEnv(const std::string &name) {
		markEnvDecl();
		if (!nativeScopes.empty())
			nativeScopes.back()[name] = {"", false, false};
	}

	std::string getUniqueVar(const std::string &base) {
		return base + "_" + std::to_string(varCounter++);
	}

	void prePass(const std::vector<Stmt *> &program);
	std::string getCppType(ValueType type, CopyMode mode, bool isConst, bool isReturn, bool isRef);

	// AST Traversals
	std::string transpileExpr(Expr *e);
	std::string transpileCond(Expr *e);
	void transpileStmt(Stmt *s);

	// Constant Pooling
	void generateConstants();

	// Final Assembly
	std::string finalize(const std::string &scriptName);
};

#endif
