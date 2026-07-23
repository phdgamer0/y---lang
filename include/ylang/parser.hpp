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
		Expr *left = parseBitwiseOr();
		while (peek().type == TokenType::AND || peek().type == TokenType::NAND) {
			Token opToken = advance();
			Expr *right = parseBitwiseOr();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
		}
		return left;
	}
	Expr *parseBitwiseOr() {
		Expr *left = parseBitwiseXor();
		while (peek().type == TokenType::BITWISE_OR) {
			Token opToken = advance();
			Expr *right = parseBitwiseXor();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
		}
		return left;
	}
	Expr *parseBitwiseXor() {
		Expr *left = parseBitwiseAnd();
		while (peek().type == TokenType::BITWISE_XOR) {
			Token opToken = advance();
			Expr *right = parseBitwiseAnd();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
		}
		return left;
	}
	Expr *parseBitwiseAnd() {
		Expr *left = parseCompare();
		while (peek().type == TokenType::BITWISE_AND) {
			Token opToken = advance();
			Expr *right = parseCompare();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
		}
		return left;
	}
	Expr *parseCompare() {
		Expr *left = parseShift();
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
	Expr *parseShift() {
		Expr *left = parseAdd();
		while (peek().type == TokenType::BITWISE_SHL || peek().type == TokenType::BITWISE_SHR) {
			Token opToken = advance();
			Expr *right = parseAdd();
			left = setPos(new BinExpr(left, right, opToken.type), opToken);
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
		if (match(TokenType::BITWISE_NOT)) {
			Token opToken = tokens[pos - 1];
			Expr *right = parseUnary();
			return setPos(new BinExpr(nullptr, right, TokenType::BITWISE_NOT), opToken);
		}
		if (match(TokenType::PLUS_PLUS)) {
			Token opToken = tokens[pos - 1];
			Expr *right = parseUnary();
			return setPos(new IncDecExpr(right, true, true), opToken);
		}
		if (match(TokenType::MINUS_MINUS)) {
			Token opToken = tokens[pos - 1];
			Expr *right = parseUnary();
			return setPos(new IncDecExpr(right, false, true), opToken);
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
					params.push_back({pname, mode, ptype, defaultExpr, nullptr, isConst,
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
			double parsedVal = 0.0;
			try {
				parsedVal = std::stod(v);
			} catch (...) {
				// Too large for double, will be handled by BigInt logic later
			}
			expr = setPos(new NumberExpr(parsedVal, isFloat, v), tokens[pos - 1]);
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
			} else if (match(TokenType::PLUS_PLUS)) {
				Token opToken = tokens[pos - 1];
				expr = setPos(new IncDecExpr(expr, true, false), opToken);
			} else if (match(TokenType::MINUS_MINUS)) {
				Token opToken = tokens[pos - 1];
				expr = setPos(new IncDecExpr(expr, false, false), opToken);
			} else
				break;
		}
		return expr;
	}
	Stmt *parseForStep() {
		Expr *e = parseExpr();
		if (match(TokenType::ASSIGN) || match(TokenType::COLON_EQ)) {
			Token op = tokens[pos - 1];
			Expr *val = parseExpr();
			return setPos(new AssignStmt(e, TokenType::ASSIGN, val), op);
		}
		TokenType type = peek().type;
		if (type == TokenType::PLUS_EQ || type == TokenType::MINUS_EQ ||
			 type == TokenType::STAR_EQ || type == TokenType::DIV_EQ ||
			 type == TokenType::MOD_EQ || type == TokenType::POW_EQ ||
			 type == TokenType::FLOOR_DIV_EQ || type == TokenType::AND_EQ ||
			 type == TokenType::OR_EQ || type == TokenType::XOR_EQ ||
			 type == TokenType::BITWISE_AND_EQ || type == TokenType::BITWISE_OR_EQ ||
			 type == TokenType::BITWISE_XOR_EQ || type == TokenType::BITWISE_NOT_EQ ||
			 type == TokenType::BITWISE_SHL_EQ || type == TokenType::BITWISE_SHR_EQ) {
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
		if (!isReplMode && scopeDepth == 0 && classDepth == 0 && !isAtEnd()) {
			TokenType t = peek().type;
			if (t != TokenType::IMPORT && t != TokenType::LET && t != TokenType::DEFINE && t != TokenType::CLASS) {
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
					params.push_back({pname, mode, ptype, defaultExpr, nullptr, isConst,
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
						bool isLocked = false;
						if (match(TokenType::COLON_EQ)) {
							isLocked = true;
						} else {
							consume(TokenType::ASSIGN, "Expected '=' or ':=' after variable names");
						}
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
								new LetStmt(names[k], values[k], false, isLocked, AccessLevel::PUBLIC));
						}
					} else {
						do {
							consume(TokenType::IDENTIFIER, "Expected variable name");
							string name = tokens[pos - 1].value;
							Expr *val = nullptr;
							bool isLocked = false;
							if (match(TokenType::COLON_EQ)) {
								isLocked = true;
								val = parseExpr();
							} else if (match(TokenType::ASSIGN)) {
								val = parseExpr();
							} else {
								val = new NumberExpr(0, false);
							}
							inits.push_back(new LetStmt(name, val, false, isLocked, AccessLevel::PUBLIC));
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
		if (match(TokenType::COLON_EQ) || match(TokenType::ASSIGN) || match(TokenType::PLUS_EQ) ||
			 match(TokenType::MINUS_EQ) || match(TokenType::STAR_EQ) ||
			 match(TokenType::DIV_EQ) || match(TokenType::MOD_EQ) ||
			 match(TokenType::FLOOR_DIV_EQ) || match(TokenType::POW_EQ) ||
			 match(TokenType::AND_EQ) || match(TokenType::OR_EQ) ||
			 match(TokenType::XOR_EQ) || match(TokenType::BITWISE_AND_EQ) ||
			 match(TokenType::BITWISE_OR_EQ) || match(TokenType::BITWISE_XOR_EQ) ||
			 match(TokenType::BITWISE_NOT_EQ) || match(TokenType::BITWISE_SHL_EQ) ||
			 match(TokenType::BITWISE_SHR_EQ)) {
			Token op = tokens[pos - 1];
			Expr *rhs = parseExpr();
			if (op.type == TokenType::COLON_EQ) op.type = TokenType::ASSIGN;
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

#endif // YLANG_PARSER_HPP
