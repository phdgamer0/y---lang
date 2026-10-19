#pragma once
#ifndef YLANG_TRANSPILER_HPP
#define YLANG_TRANSPILER_HPP

#include "objects.hpp"
#include "ast.hpp"
#include "assembler.hpp"
#include <string>
#include <vector>
#include <sstream>

struct CppTranspiler {
    Chunk* chunk;
    int scopeDepth = 0;
    int varCounter = 0;
    int loopIdCounter = 0;
    std::vector<std::pair<int, bool>> loopStack;
    std::string output = "";
    
    // Store generated C++ constants block
    std::string constantsBlock = "";

    CppTranspiler(Chunk* c) : chunk(c) {}

    std::string getUniqueVar(const std::string& base) {
        return base + "_" + std::to_string(varCounter++);
    }

    // AST Traversals
    std::string transpileExpr(Expr* e);
    void transpileStmt(Stmt* s);

    // Constant Pooling
    void generateConstants();

    // Final Assembly
    std::string finalize(const std::string& scriptName);
};

#endif
