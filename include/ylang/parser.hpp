#pragma once
#ifndef YLANG_PARSER_HPP
#define YLANG_PARSER_HPP
#include "ast.hpp"
class Parser {
	vector<Token> &tokens;
	size_t pos = 0;
	bool allowGT = true;
	int classDepth = 0;
	int scopeDepth = 0;

 public:
	bool isReplMode = false;
	void synchronize();

 private:
	[[noreturn]] void error(const string &message);
	Token consume(TokenType type, const string &message);
	template <typename T>
	T *setPos(T *node, const Token &t) {
		node->line = t.line;
		node->col = t.col;
		return node;
	}

 public:
	Parser(vector<Token> &t);
	bool isAtEnd();
	Token &peek();
	Token &advance();
	bool match(TokenType t);
	Expr *parseExpr();
	Expr *parseTernary();
	Expr *parseRange();
	vector<Stmt *> parseBlock();
	Expr *parseOr();
	Expr *parseXor();
	Expr *parseAnd();
	Expr *parseBitwiseOr();
	Expr *parseBitwiseXor();
	Expr *parseBitwiseAnd();
	Expr *parseCompare();
	Expr *parseShift();
	Expr *parseAdd();
	Expr *parseMul();
	Expr *parsePower();
	Expr *parseUnary();
	Expr *parseFString();
	Expr *parsePrimary();
	Stmt *parseForStep();
	Stmt *parseStmt();
};
// -------------------- RUNTIME -------------------

#endif // YLANG_PARSER_HPP
