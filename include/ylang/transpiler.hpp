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
};

struct NativeVar {
	std::string cppType;
	bool isRef;
	bool isConst;
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
				return &found->second;
		}
		return nullptr;
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
