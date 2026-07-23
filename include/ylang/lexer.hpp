#pragma once
#ifndef YLANG_LEXER_HPP
#define YLANG_LEXER_HPP
#include "types.hpp"
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
	BITWISE_AND,
	BITWISE_OR,
	BITWISE_XOR,
	BITWISE_NOT,
	BITWISE_SHL,
	BITWISE_SHR,
	BITWISE_AND_EQ,
	BITWISE_OR_EQ,
	BITWISE_XOR_EQ,
	BITWISE_NOT_EQ,
	BITWISE_SHL_EQ,
	BITWISE_SHR_EQ,
	PLUS_PLUS,
	MINUS_MINUS,
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
inline vector<Token> tokenize(const string &code, std::unordered_map<std::string, std::string>& macros) {
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
		if (c == '<' && i + 11 <= code.size() && code.compare(i, 11, "<<replace>>") == 0) {
			i += 11;
			col += 11;
			auto skipSpace = [&]() {
				while (i < code.size() && isspace(code[i])) {
					if (code[i] == '\n') {
						line++;
						col = 1;
					} else {
						col++;
					}
					i++;
				}
			};
			skipSpace();
			if (i < code.size() && code[i] == '"') {
				i++;
				col++;
				string target = "";
				while (i < code.size() && code[i] != '"') {
					target += code[i++];
					col++;
				}
				i++;
				col++;
				skipSpace();
				if (i + 4 < code.size() && code.compare(i, 4, "with") == 0) {
					i += 4;
					col += 4;
					skipSpace();
					if (i < code.size() && code[i] == '"') {
						i++;
						col++;
						string replacement = "";
						while (i < code.size() && code[i] != '"') {
							replacement += code[i++];
							col++;
						}
						i++;
						col++;
						macros[target] = replacement;
						continue;
					}
				}
			}
		}
		if (isspace(c)) {
			i++;
			col++;
			continue;
		}
		if ((c == 'f' || c == 'F') && i + 1 < code.size() && (code[i + 1] == '"' || code[i + 1] == '\'')) {
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
			if (auto it = macros.find(word); it != macros.end()) {
				vector<Token> expandedTokens = tokenize(it->second, macros);
				if (!expandedTokens.empty() && expandedTokens.back().type == TokenType::END) {
					expandedTokens.pop_back();
				}
				for (auto &t : expandedTokens) {
					t.line = line;
					t.col = startCol;
				}
				tokens.insert(tokens.end(), expandedTokens.begin(), expandedTokens.end());
			}
			else {
				tokens.push_back({keywordType(word), word, line, startCol});
			}
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
		CHECK_OP("&&=", TokenType::BITWISE_AND_EQ);
		CHECK_OP("||=", TokenType::BITWISE_OR_EQ);
		CHECK_OP("^^=", TokenType::BITWISE_XOR_EQ);
		CHECK_OP("!!=", TokenType::BITWISE_NOT_EQ);
		CHECK_OP("<<=", TokenType::BITWISE_SHL_EQ);
		CHECK_OP(">>=", TokenType::BITWISE_SHR_EQ);
		CHECK_OP("&&", TokenType::BITWISE_AND);
		CHECK_OP("||", TokenType::BITWISE_OR);
		CHECK_OP("^^", TokenType::BITWISE_XOR);
		CHECK_OP("!!", TokenType::BITWISE_NOT);
		CHECK_OP("<<", TokenType::BITWISE_SHL);
		CHECK_OP(">>", TokenType::BITWISE_SHR);
		CHECK_OP("++", TokenType::PLUS_PLUS);
		CHECK_OP("--", TokenType::MINUS_MINUS);
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
		case '!':
			pushToken(TokenType::NOT, "!");
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
inline vector<Token> tokenize(const string &code) {
   std::unordered_map<std::string, std::string> macros;
   return tokenize(code, macros);
}
// -------------------- ERROR SYSTEM --------------------

#endif // YLANG_LEXER_HPP
