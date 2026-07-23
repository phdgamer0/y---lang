#pragma once
#ifndef YLANG_AST_HPP
#define YLANG_AST_HPP
#include "types.hpp"
#include "lexer.hpp"
#include "errors.hpp"
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
	string raw;
	NumberExpr(double v, bool f, const string& r = "") : Expr(ExprType::NUMBER), val(v), isFloat(f), raw(r) {}
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
struct IncDecExpr : Expr {
	Expr *expr;
	bool isIncrement;
	bool isPrefix;
	IncDecExpr(Expr *e, bool isInc, bool isPref)
		 : Expr(ExprType::INC_DEC), expr(e), isIncrement(isInc), isPrefix(isPref) {}
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
struct Chunk;
struct ParamSpec {
	string name;
	CopyMode mode = CopyMode::SHALLOW;
	ValueType type;
	Expr *defaultValue = nullptr;
	Chunk *defaultChunk = nullptr;
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

#endif // YLANG_AST_HPP
