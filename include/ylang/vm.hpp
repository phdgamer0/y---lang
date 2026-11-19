#pragma once
#ifndef YLANG_VM_HPP
#define YLANG_VM_HPP
#include "compiler.hpp"
#include "parser.hpp"
#include <algorithm>
struct VM;
inline std::vector<VM *> activeVMs;

struct VM {
	VM(const VM &) = delete;
	VM &operator=(const VM &) = delete;
	VM(VM &&) = delete;
	VM &operator=(VM &&) = delete;
	int ctleTicks = 0;
	bool isCTLE = false;
	std::vector<Value> stack;
	std::shared_ptr<Env> globals;
	std::function<Value(MethodCallExpr *)> methodResolver;
	std::vector<CallFrame> frames;
	std::unordered_set<std::string> importStack;
	std::function<void(std::string, std::vector<std::string>, std::shared_ptr<Env>)> importResolver;
	CallFrame *frame;
	uint8_t *ip;
	bool printOutput;
	VM() {
		activeVMs.push_back(this);
		globals = std::make_shared<Env>();
		stack.reserve(65536);
		frame = nullptr;
		ip = nullptr;
		printOutput = false;
	}
	~VM() {
		activeVMs.erase(std::remove(activeVMs.begin(), activeVMs.end(), this), activeVMs.end());
	}
	void markObject(HeapObject *obj);
	void markValue(Value &v, int depth = 0) {
		if (v.ref)
			markObject(v.ref.get());
		if (v.type == ValueType::REFERENCE && depth < 100) {
			Value *p = v.get_ptr_safe();
			if (p && p != &v)
				markValue(*p, depth + 1);
		}
	}
	void markEnv(Env *env) {
		if (!env)
			return;
		for (auto &kv : env->vars) {
			markValue(kv.second.value);
			if (kv.second.alias)
				markValue(*kv.second.alias);
		}
		if (env->parent)
			markEnv(env->parent.get());
	}
	void gc_collect();
	void run(Chunk &chunk);

 public:
	Value prepareIterable(Value collection, int line, int col); // also used by transpiled comprehensions

 private:
	void callValue(Value callee, int argCount, int line, int col);
	Value executeDefault(Chunk *defaultChunk, int line) {
		if (!defaultChunk)
			return Value::None();
		VM tempVM;
		tempVM.globals = this->globals;
		tempVM.methodResolver = this->methodResolver;
		tempVM.run(*defaultChunk);
		if (tempVM.stack.empty())
			return Value::None();
		return tempVM.stack.back();
	}
	inline Value pop() {
		// if (stack.empty()) throw UnderflowError("Stack underflow", 0, 0);
		Value v = stack.back();
		stack.pop_back();
		return v;
	}
};

std::pair<bool, Value> tryCastDunder(Value v, const std::string &dunderName, int l, int c, std::shared_ptr<Env> globals, std::function<Value(MethodCallExpr *)> methodResolver, std::function<void(std::string, std::vector<std::string>, std::shared_ptr<Env>)> importResolver);
bool lessValue(const Value &a, const Value &b, std::shared_ptr<Env> globals, std::function<Value(MethodCallExpr *)> methodResolver, std::function<void(std::string, std::vector<std::string>, std::shared_ptr<Env>)> importResolver);
Value EvaluateConstBinary(TokenType op, const Value &a, const Value &b);

Value indexValue(Value base, const Value &index, int line, int col);
bool errorTypeMatches(const std::string &errType, const std::string &catchType);
#endif // YLANG_VM_HPP
