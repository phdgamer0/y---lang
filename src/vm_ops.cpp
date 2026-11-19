#include "ylang/vm.hpp"

void VM::markObject(HeapObject *obj) {
		if (!obj || obj->gc_marked)
			return;
		obj->gc_marked = true;
		switch (obj->type) {
		case ValueType::LIST: {
			for (auto &v : static_cast<ListObject *>(obj)->elements)
				markValue(v);
			break;
		}
		case ValueType::TUPLE: {
			for (auto &v : static_cast<TupleObject *>(obj)->elements)
				markValue(v);
			break;
		}
		case ValueType::SET: {
			for (auto &v : static_cast<SetObject *>(obj)->elements)
				markValue(const_cast<Value &>(v));
			break;
		}
		case ValueType::DICT: {
			for (auto &kv : static_cast<DictObject *>(obj)->items) {
				markValue(const_cast<Value &>(kv.first));
				markValue(kv.second);
			}
			break;
		}
		case ValueType::PAIRED: {
			for (auto &kv : static_cast<PairedObject *>(obj)->pairs) {
				markValue(kv.first);
				markValue(kv.second);
			}
			break;
		}
		case ValueType::VECTOR: {
			for (auto &v : static_cast<VectorObject *>(obj)->elements)
				markValue(v);
			break;
		}
		case ValueType::FUNCTION: {
			auto *f = static_cast<FunctionObject *>(obj);
			if (f->closure)
				markEnv(f->closure.get());
			for (auto &kv : f->cache) {
				for (auto &v : kv.first)
					markValue(const_cast<Value &>(v));
				markValue(const_cast<Value &>(kv.second));
			}
			for (auto &kv : f->singleArgCache) {
				markValue(const_cast<Value &>(kv.first));
				markValue(kv.second);
			}
			break;
		}
		case ValueType::CLASS: {
			auto *c = static_cast<ClassObject *>(obj);
			for (auto &v : c->parents)
				markValue(v);
			for (auto &kv : c->staticFields)
				markValue(kv.second);
			for (auto &kv : c->methods)
				markValue(kv.second.func);
			break;
		}
		case ValueType::INSTANCE: {
			auto *inst = static_cast<InstanceObject *>(obj);
			if (inst->klass)
				markObject(inst->klass);
			for (auto &kv : inst->fields)
				markValue(kv.second);
			break;
		}
		case ValueType::SUPER: {
			auto *s = static_cast<SuperObject *>(obj);
			markValue(s->instance);
			if (s->startClass)
				markObject(s->startClass);
			break;
		}
		case ValueType::OVERLOAD: {
			auto *o = static_cast<OverloadObject *>(obj);
			for (auto &v : o->overloads)
				markValue(v);
			for (auto &kv : o->signatureCache)
				markValue(kv.second);
			break;
		}
		case ValueType::REFERENCE: {
			if (auto *lr = dynamic_cast<ListRefObject *>(obj)) {
				if (lr->list)
					markObject(lr->list.get());
			} else if (auto *pr = dynamic_cast<PropRefObject *>(obj)) {
				if (pr->inst)
					markObject(pr->inst.get());
			} else if (auto *vr = dynamic_cast<VarRefObject *>(obj)) {
				if (vr->env)
					markEnv(vr->env.get());
			}
			break;
		}
		default:
			break;
		}
	}

void VM::gc_collect() {
		HeapObject *curr = gc_head;
		while (curr) {
			curr->gc_marked = false;
			curr = curr->gc_next;
		}
		for (VM *vm : activeVMs) {
			for (auto &v : vm->stack)
				markValue(v);
			if (vm->globals)
				markEnv(vm->globals.get());
			for (auto &f : vm->frames) {
				if (f.function)
					markObject(f.function);
				for (auto &v : f.cacheKey)
					markValue(v);
				markValue(f.singleCacheKey);
			}
		}
		curr = gc_head;
		while (curr) {
			if (!curr->gc_marked) {
				int old_count = gc_delete_count;
				curr->breakCycles();
				if (gc_delete_count != old_count) {
					curr = gc_head;
					continue;
				}
			}
			curr = curr->gc_next;
		}
	}

void VM::callValue(Value callee, int argCount, int line, int col) {
		FunctionObject *function = nullptr;
		NativeFunctionObject *nativeObj = nullptr;
		if (callee.type == ValueType::FUNCTION) {
			function = static_cast<FunctionObject *>(callee.ref.get());
		} else if (callee.type == ValueType::NATIVE_FUNCTION) {
			nativeObj = static_cast<NativeFunctionObject *>(callee.ref.get());
		} else if (callee.type == ValueType::OVERLOAD) {
			auto *ov = static_cast<OverloadObject *>(callee.ref.get());
			std::size_t sigHash = argCount;
			for (int i = 0; i < argCount; i++) {
				Value argVal = stack[stack.size() - argCount + i];
				ValueType checkType = argVal.type;
				if (argVal.type == ValueType::REFERENCE && argVal.get_ptr_safe() != nullptr) {
					checkType = argVal.get_ptr_safe()->type;
				}
				sigHash ^= std::hash<int>{}((int)checkType) + 0x9e3779b9 + (sigHash << 6) + (sigHash >> 2);
			}
			bool found = false;
			if (ov->signatureCache.count(sigHash)) {
				Value cachedTarget = ov->signatureCache[sigHash];
				if (cachedTarget.type == ValueType::FUNCTION) {
					function = static_cast<FunctionObject *>(cachedTarget.ref.get());
					found = true;
				} else if (cachedTarget.type == ValueType::NATIVE_FUNCTION) {
					nativeObj = static_cast<NativeFunctionObject *>(cachedTarget.ref.get());
					found = true;
				}
			}
			if (!found) {
				FunctionObject *bestFunc = nullptr;
				NativeFunctionObject *bestNative = nullptr;
				int bestScore = -1;
				for (int i = (int)ov->overloads.size() - 1; i >= 0; i--) {
					Value candVal = ov->overloads[i];
					if (candVal.type == ValueType::FUNCTION) {
						auto *candidate =
							static_cast<FunctionObject *>(candVal.ref.get());
						size_t minArgs = 0;
						bool isVariadic = false;
						for (const auto &p : candidate->params) {
							if (p.isVariadic || p.isKwargs)
								isVariadic = true;
							else if (p.defaultValue == nullptr)
								minArgs++;
						}
						if (argCount < minArgs && !isVariadic)
							continue;
						if (!isVariadic && argCount > candidate->params.size())
							continue;
						bool typesMatch = true;
						int currentScore = 0;
						size_t paramIdx = 0;
						for (int argIdx = 0; argIdx < argCount; argIdx++) {
							if (paramIdx >= candidate->params.size()) {
								if (!isVariadic) {
									typesMatch = false;
									break;
								}
								currentScore++;
								break;
							}
							const ParamSpec &p = candidate->params[paramIdx];
							if (p.isVariadic || p.isKwargs)
								break;
							Value argVal = stack[stack.size() - argCount + argIdx];
							ValueType checkType = argVal.type;
							if (argVal.type == ValueType::REFERENCE && argVal.get_ptr_safe() != nullptr) {
								checkType = argVal.get_ptr_safe()->type;
							}
							if (p.type != ValueType::NOTYPE && checkType != p.type) {
								if (p.type == ValueType::FLOAT && checkType == ValueType::INT) {
									currentScore += 5;
								} else {
									typesMatch = false;
									break;
								}
							} else if (p.type != ValueType::NOTYPE && checkType == p.type) {
								currentScore += 10;
							} else if (p.type == ValueType::NOTYPE) {
								currentScore += 2;
							}
							paramIdx++;
						}
						if (typesMatch && currentScore > bestScore) {
							bestScore = currentScore;
							bestFunc = candidate;
							bestNative = nullptr;
						}
					} else if (candVal.type == ValueType::NATIVE_FUNCTION) {
						if (0 > bestScore) {
							bestScore = 0;
							bestFunc = nullptr;
							bestNative = static_cast<NativeFunctionObject *>(candVal.ref.get());
						}
					}
				}
				if (bestFunc) {
					function = bestFunc;
					found = true;
					Value cachedTarget;
					cachedTarget.type = ValueType::FUNCTION;
					cachedTarget.ref = std::shared_ptr<HeapObject>(bestFunc, [](HeapObject *) {});
					ov->signatureCache[sigHash] = cachedTarget;
				} else if (bestNative) {
					nativeObj = bestNative;
					found = true;
					Value cachedTarget;
					cachedTarget.type = ValueType::NATIVE_FUNCTION;
					cachedTarget.ref = std::shared_ptr<HeapObject>(bestNative, [](HeapObject *) {});
					ov->signatureCache[sigHash] = cachedTarget;
				}
				if (!found)
					throw TypeError(
						"No matching overload found with provided arguments", line,
						col);
			}
		} else
			throw TypeError("Object is not callable", line, col);
		if (nativeObj) {
			vector<Value> args(stack.end() - argCount, stack.end());
			stack.erase(stack.end() - argCount, stack.end());
			stack.push_back(nativeObj->func(args, line, 0));
			return;
		}
		vector<Value> providedArgs;
		providedArgs.resize(argCount);
		for (int i = argCount - 1; i >= 0; i--)
			providedArgs[i] = pop();
		// Debugging: Verify we have params
		// std::cout << "DEBUG: Call " << argCount << " args. Func expects " <<
		// function->params.size() << "\n";
		vector<Value> finalArgs;
		finalArgs.resize(function->params.size());
		size_t argIndex = 0;
		std::vector<bool> consumed(providedArgs.size(), false);
		for (size_t i = 0; i < function->params.size(); i++) {
			const ParamSpec &p = function->params[i];
			Value argVal = Value::None();
			if (p.isKwargs) {
				auto dict = std::make_shared<DictObject>();
				while (argIndex < providedArgs.size()) {
					Value v = providedArgs[argIndex];
					if (v.type != ValueType::PAIRED)
						throw SyntaxError("Positional argument after keyword args",
							line, col);
					auto *pairObj = static_cast<PairedObject *>(v.ref.get());
					for (const auto &entry : pairObj->pairs) {
						Value key = entry.first;
						Value val = entry.second;
						if (p.mode == CopyMode::DEEP) {
							key = deepCopy(key);
							val = deepCopy(val);
						}
						dict->items[key] = val;
					}
					consumed[argIndex] = true;
					argIndex++;
				}
				argVal = Value::Dict(dict->items);
				if (p.isConst)
					argVal.isConst = true;
			} else if (p.isVariadic) {
				vector<Value> tupleItems;
				while (argIndex < providedArgs.size()) {
					if (providedArgs[argIndex].type == ValueType::PAIRED)
						break;
					Value v = providedArgs[argIndex];
					if (p.mode == CopyMode::DEEP)
						v = deepCopy(v);
					tupleItems.push_back(v);
					consumed[argIndex] = true;
					argIndex++;
				}
				argVal = Value::Tuple(tupleItems);
			} else {
				bool foundValue = false;
				if (argIndex < providedArgs.size() &&
					 providedArgs[argIndex].type != ValueType::PAIRED) {
					if (providedArgs[argIndex].type == ValueType::OMIT_MARKER) {
						if (p.defaultChunk != nullptr) {
							argVal = executeDefault(p.defaultChunk,
								line); // <--- EXECUTE AST
							foundValue = true;
						} else
							throw ArgumentError(
								"Argument '" + p.name + "' cannot be omitted.", line,
								col);
					} else {
						argVal = providedArgs[argIndex];
						if (p.mode == CopyMode::DEEP)
							argVal = deepCopy(argVal);
						else if (p.mode == CopyMode::SHALLOW)
							argVal = shallowCopy(argVal);
						foundValue = true;
					}
					consumed[argIndex] = true;
					argIndex++;
				}
				if (!foundValue) {
					for (size_t k = argIndex; k < providedArgs.size(); k++) {
						if (!consumed[k] && providedArgs[k].type == ValueType::PAIRED) {
							auto *pairObj = static_cast<PairedObject *>(providedArgs[k].ref.get());
							for (const auto &entry : pairObj->pairs) {
								if (entry.first.asString() == p.name) {
									argVal = entry.second;
									if (p.mode == CopyMode::DEEP)
										argVal = deepCopy(argVal);
									else if (p.mode == CopyMode::SHALLOW)
										argVal = shallowCopy(argVal);
									foundValue = true;
									consumed[k] = true;
									break;
								}
							}
						}
						if (foundValue)
							break;
					}
				}
				if (!foundValue) {
					if (p.defaultChunk != nullptr)
						argVal = executeDefault(p.defaultChunk, line);
					else
						throw ArgumentError(
							"Missing required argument '" + p.name + "'", line, col);
				}
				if (p.type != ValueType::NOTYPE && argVal.type != p.type) {
					bool mismatch = true;
					if (p.type == ValueType::FLOAT &&
						 argVal.type == ValueType::INT) {
						argVal = Value::Float((double)argVal.asInt());
						mismatch = false;
					} else if (p.type == ValueType::INT &&
								  argVal.type == ValueType::FLOAT) {
						argVal = Value::Int((long long)argVal.asFloat());
						mismatch = false;
					} else if (p.type == ValueType::INT &&
								  argVal.type == ValueType::BIGINT) {
						mismatch = false;
					} else if (p.type == ValueType::BIGINT &&
								  argVal.type == ValueType::INT) {
						argVal = Value::BigInt(argVal.asInt());
						mismatch = false;
					} else if (argVal.type == ValueType::REFERENCE && argVal.get_ptr_safe() != nullptr && argVal.get_ptr_safe()->type == p.type) {
						mismatch = false;
					} else if (argVal.type == ValueType::OMIT_MARKER) {
						mismatch = false;
					}
					if (mismatch) {
						throw TypeError("Type mismatch for '" + p.name +
												 "'. Expected " +
												 std::to_string((int)p.type) + " got " +
												 std::to_string((int)argVal.type),
							line, col);
					}
				}
			}
			if (p.isConst)
				argVal.isConst = true;
			if (p.type != ValueType::NOTYPE)
				argVal.isLocked = true;
			finalArgs[i] = argVal;
		}
		if (function->isCached) {
			if (finalArgs.size() == 1) {
				if (function->singleArgCache.count(finalArgs[0])) {
					stack.push_back(function->singleArgCache[finalArgs[0]]);
					return;
				}
			} else {
				if (function->cache.count(finalArgs)) {
					stack.push_back(function->cache[finalArgs]);
					return;
				}
			}
		}
		for (size_t k = 0; k < providedArgs.size(); k++) {
			if (!consumed[k]) {
				throw ArgumentError("Too many arguments or unexpected keyword argument passed.", line, col);
			}
		}
		if (frame)
			frame->ip = ip;
		CallFrame newFrame;
		newFrame.function = function;
		newFrame.ip = function->chunk->code.data();
		newFrame.basePointer = stack.size();
		if (function->isCached) {
			if (finalArgs.size() == 1) {
				newFrame.singleCacheKey = finalArgs[0];
			} else {
				newFrame.cacheKey = finalArgs;
			}
		}
		for (const auto &v : finalArgs)
			stack.push_back(v);
		frames.push_back(newFrame);
		frame = &frames.back();
		ip = frame->ip;
	}

Value VM::prepareIterable(Value collection, int line, int col) {
		if (collection.type == ValueType::LIST ||
			 collection.type == ValueType::SET ||
			 collection.type == ValueType::TUPLE ||
			 collection.type == ValueType::STRING ||
			 collection.type == ValueType::VECTOR ||
			 collection.type == ValueType::RANGE) {
			return collection;
		}
		if (collection.type == ValueType::REFERENCE) {
			return *(collection.get_ptr_safe());
		}
		auto list = std::make_shared<ListObject>();
		if (collection.type == ValueType::DICT) {
			auto *dict = static_cast<DictObject *>(collection.ref.get());
			for (const auto &pair : dict->items)
				list->elements.push_back(pair.first);
		} else
			throw TypeError("Object is not iterable", line, col);
		return Value::List(list->elements);
	}

std::pair<bool, Value> tryCastDunder(Value v, const std::string &dunderName, int l, int c, std::shared_ptr<Env> globals, std::function<Value(MethodCallExpr *)> methodResolver, std::function<void(std::string, std::vector<std::string>, std::shared_ptr<Env>)> importResolver) {
	while (v.type == ValueType::REFERENCE) {
		if (!v.get_ptr_safe())
			throw RuntimeError("Attempted to dereference a null pointer!", l, c);
		v = *(v.get_ptr_safe());
	}

	if (v.type == ValueType::INSTANCE) {
		auto *inst = static_cast<InstanceObject *>(v.ref.get());
		ClassObject *cls = inst->klass;
		ClassObject::MethodInfo *dunderMethod = nullptr;
		ClassObject *methodOwner = nullptr;

		for (auto *ancestor : cls->mro) {
			if (ancestor->methods.count(dunderName)) {
				dunderMethod = &ancestor->methods[dunderName];
				methodOwner = ancestor;
				break;
			}
		}

		if (dunderMethod) {
			Value classObjVal;
			classObjVal.type = ValueType::CLASS;
			classObjVal.ref = std::shared_ptr<HeapObject>(methodOwner, [](HeapObject *) {});

			if (dunderMethod->func.type == ValueType::NATIVE_FUNCTION) {
				auto *nat = static_cast<NativeFunctionObject *>(dunderMethod->func.ref.get());
				std::vector<Value> args = {v, classObjVal};
				return {true, nat->func(args, l, c)};
			} else {
				// ---> INJECT THE CONTEXT INTO THE TEMP VM <---
				VM tempVM;
				tempVM.globals = globals;
				tempVM.methodResolver = methodResolver;
				tempVM.importResolver = importResolver;

				Chunk tempChunk;
				int selfIdx = tempChunk.addConstant(v);
				tempChunk.write(OpCode::OP_CONSTANT, l, c);
				tempChunk.write((uint8_t)selfIdx, l, c);

				int objIdx = tempChunk.addConstant(classObjVal);
				tempChunk.write(OpCode::OP_CONSTANT, l, c);
				tempChunk.write((uint8_t)objIdx, l, c);

				int methIdx = tempChunk.addConstant(dunderMethod->func);
				tempChunk.write(OpCode::OP_CONSTANT, l, c);
				tempChunk.write((uint8_t)methIdx, l, c);

				tempChunk.write(OpCode::OP_CALL, l, c);
				tempChunk.write((uint8_t)2, l, c);
				tempChunk.write(OpCode::OP_RETURN, l, c);

				try {
					tempVM.run(tempChunk);
					if (!tempVM.stack.empty())
						return {true, tempVM.stack.back()};
				} catch (...) {
					throw;
				}
				return {true, Value::None()};
			}
		}
	}
	return {false, v};
}

bool lessValue(const Value &a, const Value &b, std::shared_ptr<Env> globals, std::function<Value(MethodCallExpr *)> methodResolver, std::function<void(std::string, std::vector<std::string>, std::shared_ptr<Env>)> importResolver) {
	if (a.type == ValueType::REFERENCE) {
		if (b.type == ValueType::REFERENCE) {
			return lessValue(*(a.get_ptr_safe()), *(b.get_ptr_safe()), globals, methodResolver, importResolver);
		} else {
			return lessValue(*(a.get_ptr_safe()), b, globals, methodResolver, importResolver);
		}
	} else if (b.type == ValueType::REFERENCE) {
		return lessValue(a, *(b.get_ptr_safe()), globals, methodResolver, importResolver);
	}
	if (a.type == ValueType::INSTANCE) {
		auto *instA = static_cast<InstanceObject *>(a.ref.get());
		ClassObject *clsA = instA->klass;
		ClassObject::MethodInfo *lessMethod = nullptr;
		ClassObject *methodOwner = nullptr;

		for (auto *ancestor : clsA->mro) {
			if (ancestor->methods.count("__less__")) {
				lessMethod = &ancestor->methods["__less__"];
				methodOwner = ancestor;
				break;
			}
		}

		if (lessMethod) {
			Value classObjVal;
			classObjVal.type = ValueType::CLASS;
			classObjVal.ref = std::shared_ptr<HeapObject>(methodOwner, [](HeapObject *) {});

			// PATH A: Native C++ Dunder
			if (lessMethod->func.type == ValueType::NATIVE_FUNCTION) {
				auto *nat = static_cast<NativeFunctionObject *>(lessMethod->func.ref.get());
				std::vector<Value> args = {a, classObjVal, b};
				return nat->func(args, 0, 0).isTruthy();
			}
			// PATH B: y-- Bytecode Dunder (Temp VM)
			else {
				VM tempVM;
				tempVM.globals = globals;
				tempVM.methodResolver = methodResolver;
				tempVM.importResolver = importResolver;

				Chunk tempChunk;

				// Push self
				int selfIdx = tempChunk.addConstant(a);
				tempChunk.write(OpCode::OP_CONSTANT, 0, 0);
				tempChunk.write((uint8_t)selfIdx, 0, 0);

				// Push classObj
				int objIdx = tempChunk.addConstant(classObjVal);
				tempChunk.write(OpCode::OP_CONSTANT, 0, 0);
				tempChunk.write((uint8_t)objIdx, 0, 0);

				// Push other (b)
				int otherIdx = tempChunk.addConstant(b);
				tempChunk.write(OpCode::OP_CONSTANT, 0, 0);
				tempChunk.write((uint8_t)otherIdx, 0, 0);

				// Push method
				int methIdx = tempChunk.addConstant(lessMethod->func);
				tempChunk.write(OpCode::OP_CONSTANT, 0, 0);
				tempChunk.write((uint8_t)methIdx, 0, 0);

				// Call (self, class, other)
				tempChunk.write(OpCode::OP_CALL, 0, 0);
				tempChunk.write((uint8_t)3, 0, 0);
				tempChunk.write(OpCode::OP_RETURN, 0, 0);

				try {
					tempVM.run(tempChunk);
					if (!tempVM.stack.empty())
						return tempVM.stack.back().isTruthy();
				} catch (...) {
					throw;
				} // Let user crashes propagate cleanly
				return false;
			}
		}
	}
	if (a.type != b.type) {
		bool aIsNum = (a.type == ValueType::INT || a.type == ValueType::FLOAT || a.type == ValueType::BIGINT);
		bool bIsNum = (b.type == ValueType::INT || b.type == ValueType::FLOAT || b.type == ValueType::BIGINT);
		if (aIsNum && bIsNum) {
			if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
				return a.asFloat() < b.asFloat();
			}
			BigIntObject tempA(0), tempB(0);
			BigIntObject *ba = (a.type == ValueType::BIGINT) ? static_cast<BigIntObject *>(a.ref.get()) : &(tempA = BigIntObject(a.asInt()));
			BigIntObject *bb = (b.type == ValueType::BIGINT) ? static_cast<BigIntObject *>(b.ref.get()) : &(tempB = BigIntObject(b.asInt()));
			return *ba < *bb;
		}
		return a.type < b.type;
	}
	switch (a.type) {
	case ValueType::INT:
		return a.asInt() < b.asInt();
	case ValueType::FLOAT:
		return a.asFloat() < b.asFloat();
	case ValueType::STRING:
		return a.asString() < b.asString();
	case ValueType::BOOL:
		return a.asBool() < b.asBool();
	case ValueType::BIGINT: {
		auto *b1 = static_cast<BigIntObject *>(a.ref.get());
		auto *b2 = static_cast<BigIntObject *>(b.ref.get());
		return *b1 < *b2;
	}
	case ValueType::TUPLE: {
		auto *t1 = static_cast<TupleObject *>(a.ref.get());
		auto *t2 = static_cast<TupleObject *>(b.ref.get());
		if (t1->elements.empty() && t2->elements.empty())
			return false;
		if (t1->elements.empty())
			return true;
		if (t2->elements.empty())
			return false;
		return lessValue(t1->elements[0], t2->elements[0], globals, methodResolver, importResolver);
	}
	case ValueType::VECTOR: {
		auto *v1 = static_cast<VectorObject *>(a.ref.get());
		auto *v2 = static_cast<VectorObject *>(b.ref.get());
		bool hasFloat = false;
		for (const auto &val : v1->elements)
			if (val.type == ValueType::FLOAT)
				hasFloat = true;
		for (const auto &val : v2->elements)
			if (val.type == ValueType::FLOAT)
				hasFloat = true;
		if (hasFloat) {
			double mag1 = 0.0, mag2 = 0.0;
			for (const auto &val : v1->elements) {
				double v = val.asFloat();
				mag1 += (v * v);
			}
			for (const auto &val : v2->elements) {
				double v = val.asFloat();
				mag2 += (v * v);
			}
			return mag1 < mag2;
		} else {
			Value mag1 = Value::Int(0);
			for (const auto &val : v1->elements) {
				Value sq = BigIntObject::mul(val, val);
				mag1 = BigIntObject::add(mag1, sq);
			}
			Value mag2 = Value::Int(0);
			for (const auto &val : v2->elements) {
				Value sq = BigIntObject::mul(val, val);
				mag2 = BigIntObject::add(mag2, sq);
			}
			return lessValue(mag1, mag2, globals, methodResolver, importResolver);
		}
	}
	case ValueType::LIST: {
		auto *l1 = static_cast<ListObject *>(a.ref.get());
		auto *l2 = static_cast<ListObject *>(b.ref.get());
		return std::lexicographical_compare(
			l1->elements.begin(), l1->elements.end(), l2->elements.begin(),
			l2->elements.end(),
			// Recursive call for elements
			[&](const Value &x, const Value &y) { return lessValue(x, y, globals, methodResolver, importResolver); });
	}
	default:
		return false;
	}
}

Value EvaluateConstBinary(TokenType op, const Value &a, const Value &b) {
	// operands like `@a + 1`: operate on the referenced value, not the reference
	if (a.type == ValueType::REFERENCE && a.get_ptr_safe())
		return EvaluateConstBinary(op, *a.get_ptr_safe(), b);
	if (b.type == ValueType::REFERENCE && b.get_ptr_safe())
		return EvaluateConstBinary(op, a, *b.get_ptr_safe());
	if (op == TokenType::PLUS) {
		if (a.type == ValueType::LIST && b.type == ValueType::LIST) {
			// new list; elements are shared like any other list copy
			const auto &x = static_cast<ListObject *>(a.ref.get())->elements;
			const auto &y = static_cast<ListObject *>(b.ref.get())->elements;
			std::vector<Value> out;
			out.reserve(x.size() + y.size());
			out.insert(out.end(), x.begin(), x.end());
			out.insert(out.end(), y.begin(), y.end());
			return Value::List(out);
		}
		if (a.type == ValueType::INT && b.type == ValueType::INT) {
			long long res = a.iVal + b.iVal;
			bool overflow = ((a.iVal ^ res) & (b.iVal ^ res)) < 0;
			if (overflow)
				return BigIntObject::add(Value::BigInt(a.iVal), Value::BigInt(b.iVal));
			else
				return Value::Int(res);
		} else if (a.type == ValueType::STRING || b.type == ValueType::STRING) {
			return Value::String(valueToString(a) + valueToString(b));
		} else if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
			return Value::Float(a.asFloat() + b.asFloat());
		} else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
			return BigIntObject::add(a, b);
		} else if (a.type == ValueType::VECTOR && b.type == ValueType::VECTOR) {
			auto *v1 = static_cast<VectorObject *>(a.ref.get());
			auto *v2 = static_cast<VectorObject *>(b.ref.get());
			if (v1->elements.size() != v2->elements.size())
				throw std::runtime_error("Vector dimension mismatch");
			std::vector<Value> res;
			res.reserve(v1->elements.size());
			for (size_t i = 0; i < v1->elements.size(); i++) {
				Value x = v1->elements[i];
				Value y = v2->elements[i];
				if (x.type == ValueType::INT && y.type == ValueType::INT) {
					long long r = x.iVal + y.iVal;
					bool ovf = ((x.iVal ^ r) & (y.iVal ^ r)) < 0;
					if (ovf)
						res.push_back(BigIntObject::add(Value::BigInt(x.iVal), Value::BigInt(y.iVal)));
					else
						res.push_back(Value::Int(r));
				} else if (x.type == ValueType::BIGINT || y.type == ValueType::BIGINT) {
					res.push_back(BigIntObject::add(x, y));
				} else {
					res.push_back(Value::Float(x.asFloat() + y.asFloat()));
				}
			}
			return Value::Vector(res);
		} else if (a.isNumber() || b.isNumber()) {
			return Value::Float(a.asFloat() + b.asFloat());
		}
	} else if (op == TokenType::MINUS) {
		if (a.type == ValueType::INT && b.type == ValueType::INT) {
			long long res = a.iVal - b.iVal;
			bool overflow = ((a.iVal ^ b.iVal) & (a.iVal ^ res)) < 0;
			if (overflow)
				return BigIntObject::sub(Value::BigInt(a.iVal), Value::BigInt(b.iVal));
			else
				return Value::Int(res);
		} else if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
			return Value::Float(a.asFloat() - b.asFloat());
		} else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
			return BigIntObject::sub(a, b);
		} else if (a.type == ValueType::VECTOR && b.type == ValueType::VECTOR) {
			auto *v1 = static_cast<VectorObject *>(a.ref.get());
			auto *v2 = static_cast<VectorObject *>(b.ref.get());
			if (v1->elements.size() != v2->elements.size())
				throw std::runtime_error("Vector dimension mismatch");
			std::vector<Value> res;
			res.reserve(v1->elements.size());
			for (size_t i = 0; i < v1->elements.size(); i++) {
				Value x = v1->elements[i];
				Value y = v2->elements[i];
				if (x.type == ValueType::INT && y.type == ValueType::INT) {
					long long r = x.iVal - y.iVal;
					bool ovf = ((x.iVal ^ y.iVal) & (x.iVal ^ r)) < 0;
					if (ovf)
						res.push_back(BigIntObject::sub(Value::BigInt(x.iVal), Value::BigInt(y.iVal)));
					else
						res.push_back(Value::Int(r));
				} else if (x.type == ValueType::BIGINT || y.type == ValueType::BIGINT) {
					res.push_back(BigIntObject::sub(x, y));
				} else {
					res.push_back(Value::Float(x.asFloat() - y.asFloat()));
				}
			}
			return Value::Vector(res);
		} else if (a.isNumber() || b.isNumber()) {
			return Value::Float(a.asFloat() - b.asFloat());
		}
	} else if (op == TokenType::SLASH) {
		if (a.type == ValueType::VECTOR) {
			if (!b.isNumber())
				throw std::runtime_error("Vector can only be divided by a number");
			double s = b.asFloat();
			if (s == 0.0)
				throw std::runtime_error("Vector division by zero");
			auto *v = static_cast<VectorObject *>(a.ref.get());
			std::vector<Value> res;
			res.reserve(v->elements.size());
			for (const auto &elem : v->elements) {
				res.push_back(Value::Float(elem.asFloat() / s));
			}
			return Value::Vector(res);
		} else if (b.type == ValueType::VECTOR) {
			throw std::runtime_error("Cannot divide by a vector");
		} else if (a.isNumber() && b.isNumber()) {
			double db = b.asFloat();
			if (db == 0.0)
				throw std::runtime_error("Division by zero");
			return Value::Float(a.asFloat() / db);
		}
	} else if (op == TokenType::STAR) {
		if (a.type == ValueType::STRING && b.type == ValueType::INT) {
			std::string res = "";
			std::string base = a.asString();
			long long count = b.asInt();
			if (count < 0)
				count = 0;
			if (count > 1000000)
				throw std::runtime_error("String repetition too large");
			for (long long i = 0; i < count; i++)
				res += base;
			return Value::String(res);
		} else if (a.type == ValueType::VECTOR && b.type == ValueType::VECTOR) {
			auto *v1 = static_cast<VectorObject *>(a.ref.get());
			auto *v2 = static_cast<VectorObject *>(b.ref.get());
			if (v1->elements.size() != v2->elements.size())
				throw std::runtime_error("Vector dimension mismatch");
			Value dot = Value::Int(0);
			for (size_t i = 0; i < v1->elements.size(); i++) {
				Value x = v1->elements[i];
				Value y = v2->elements[i];
				Value prod;
				if (x.type == ValueType::INT && y.type == ValueType::INT) {
					long long r;
					bool ovf = __builtin_mul_overflow(x.iVal, y.iVal, &r);
					if (ovf)
						prod = BigIntObject::mul(Value::BigInt(x.iVal), Value::BigInt(y.iVal));
					else
						prod = Value::Int(r);
				} else if (x.type == ValueType::BIGINT || y.type == ValueType::BIGINT) {
					prod = BigIntObject::mul(x, y);
				} else {
					prod = Value::Float(x.asFloat() * y.asFloat());
				}
				if (dot.type == ValueType::INT && prod.type == ValueType::INT) {
					long long r = dot.iVal + prod.iVal;
					bool ovf = ((dot.iVal ^ r) & (prod.iVal ^ r)) < 0;
					if (ovf)
						dot = BigIntObject::add(Value::BigInt(dot.iVal), Value::BigInt(prod.iVal));
					else
						dot = Value::Int(r);
				} else if (dot.type == ValueType::BIGINT || prod.type == ValueType::BIGINT) {
					dot = BigIntObject::add(dot, prod);
				} else {
					dot = Value::Float(dot.asFloat() + prod.asFloat());
				}
			}
			return dot;
		} else if ((a.type == ValueType::VECTOR && b.isNumber()) || (a.isNumber() && b.type == ValueType::VECTOR)) {
			VectorObject *vec = (a.type == ValueType::VECTOR) ? static_cast<VectorObject *>(a.ref.get()) : static_cast<VectorObject *>(b.ref.get());
			Value scalar = (a.type == ValueType::VECTOR) ? b : a;
			std::vector<Value> res;
			res.reserve(vec->elements.size());
			for (const auto &elem : vec->elements) {
				if (elem.type == ValueType::INT && scalar.type == ValueType::INT) {
					long long r;
					bool ovf = __builtin_mul_overflow(elem.iVal, scalar.iVal, &r);
					if (ovf)
						res.push_back(BigIntObject::mul(Value::BigInt(elem.iVal), Value::BigInt(scalar.iVal)));
					else
						res.push_back(Value::Int(r));
				} else if (elem.type == ValueType::BIGINT || scalar.type == ValueType::BIGINT) {
					res.push_back(BigIntObject::mul(elem, scalar));
				} else {
					res.push_back(Value::Float(elem.asFloat() * scalar.asFloat()));
				}
			}
			return Value::Vector(res);
		} else if (a.type == ValueType::INT && b.type == ValueType::INT) {
			long long res;
			bool overflow = __builtin_mul_overflow(a.iVal, b.iVal, &res);
			if (overflow)
				return BigIntObject::mul(Value::BigInt(a.iVal), Value::BigInt(b.iVal));
			else
				return Value::Int(res);
		} else if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
			return Value::Float(a.asFloat() * b.asFloat());
		} else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
			return BigIntObject::mul(a, b);
		} else if (a.isNumber() && b.isNumber()) {
			return Value::Float(a.asFloat() * b.asFloat());
		}
	} else if (op == TokenType::FLOOR_DIV) {
		if (a.type == ValueType::VECTOR) {
			if (!b.isNumber())
				throw std::runtime_error("Vector can only be floor-divided by a number");
			if (b.asFloat() == 0.0)
				throw std::runtime_error("Vector floor division by zero");
			auto *v = static_cast<VectorObject *>(a.ref.get());
			std::vector<Value> res;
			res.reserve(v->elements.size());
			double db = b.asFloat();
			for (const auto &elem : v->elements) {
				if (elem.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
					res.push_back(BigIntObject::div(elem, b));
				} else {
					res.push_back(Value::Int((long long)std::floor(elem.asFloat() / db)));
				}
			}
			return Value::Vector(res);
		} else if (b.type == ValueType::VECTOR) {
			throw std::runtime_error("Cannot floor-divide by a vector");
		} else if (a.type == ValueType::INT && b.type == ValueType::INT) {
			if (b.iVal == 0)
				throw std::runtime_error("Division by zero");
			return Value::Int(a.iVal / b.iVal);
		} else if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
			double db = b.asFloat();
			if (db == 0.0)
				throw std::runtime_error("Division by zero");
			return Value::Int((long long)std::floor(a.asFloat() / db));
		} else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
			if (b.asFloat() == 0.0)
				throw std::runtime_error("Division by zero");
			return BigIntObject::div(a, b);
		} else if (a.isNumber() && b.isNumber()) {
			double db = b.asFloat();
			if (db == 0.0)
				throw std::runtime_error("Division by zero");
			return Value::Int((long long)std::floor(a.asFloat() / db));
		}
	} else if (op == TokenType::MOD) {
		if (a.type == ValueType::INT && b.type == ValueType::INT) {
			if (b.iVal == 0)
				throw std::runtime_error("Modulo by zero");
			return Value::Int(a.iVal % b.iVal);
		} else if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
			double db = b.asFloat();
			if (db == 0.0)
				throw std::runtime_error("Modulo by zero");
			return Value::Float(std::fmod(a.asFloat(), db));
		} else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
			return BigIntObject::mod(a, b);
		} else if (a.isNumber() && b.isNumber()) {
			if (b.asFloat() == 0)
				throw std::runtime_error("Modulo by zero");
			return Value::Float(std::fmod(a.asFloat(), b.asFloat()));
		}
	} else if (op == TokenType::BITWISE_AND) {
		if (a.type == ValueType::INT && b.type == ValueType::INT)
			return Value::Int(a.iVal & b.iVal);
		else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT)
			return BigIntObject::bit_and(a, b);
	} else if (op == TokenType::BITWISE_OR) {
		if (a.type == ValueType::INT && b.type == ValueType::INT)
			return Value::Int(a.iVal | b.iVal);
		else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT)
			return BigIntObject::bit_or(a, b);
	} else if (op == TokenType::BITWISE_XOR) {
		if (a.type == ValueType::INT && b.type == ValueType::INT)
			return Value::Int(a.iVal ^ b.iVal);
		else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT)
			return BigIntObject::bit_xor(a, b);
	} else if (op == TokenType::BITWISE_SHL) {
		if (a.type == ValueType::INT && b.type == ValueType::INT) {
			if (b.iVal < 0)
				throw std::runtime_error("Negative shift count");
			long long r;
			if (a.iVal == 0)
				return Value::Int(0);
			if (b.iVal >= 63 || __builtin_mul_overflow(a.iVal, 1LL << b.iVal, &r))
				return BigIntObject::bit_shl(Value::BigInt(a.iVal), b);
			return Value::Int(r);
		}
		else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT)
			return BigIntObject::bit_shl(a, b);
	} else if (op == TokenType::BITWISE_SHR) {
		if (a.type == ValueType::INT && b.type == ValueType::INT) {
			if (b.iVal < 0)
				throw std::runtime_error("Negative shift count");
			return Value::Int(b.iVal >= 63 ? (a.iVal < 0 ? -1 : 0) : a.iVal >> b.iVal);
		}
		else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT)
			return BigIntObject::bit_shr(a, b);
	} else if (op == TokenType::POW) {
		if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
			return Value::Float(std::pow(a.asFloat(), b.asFloat()));
		} else if (a.type == ValueType::BIGINT || b.type == ValueType::BIGINT) {
			return BigIntObject::pow(a, b);
		} else if (a.type == ValueType::INT && b.type == ValueType::INT) {
			if (b.iVal < 0) {
				if (a.iVal == 0)
					throw std::runtime_error("Division by zero: 0 cannot be raised to a negative power");
				// integer semantics: 1 ** -n == 1, (-1) ** -n == +-1, anything else truncates to 0
				if (a.iVal == 1)
					return Value::Int(1);
				if (a.iVal == -1)
					return Value::Int((b.iVal & 1) ? -1 : 1);
				return Value::Int(0);
			}
			// exact exponentiation by squaring; std::pow is a double and is wrong above 2^53
			long long r = 1, base = a.iVal, e = b.iVal;
			while (e) {
				if ((e & 1) && __builtin_mul_overflow(r, base, &r))
					return BigIntObject::pow(a, b);
				e >>= 1;
				if (e && __builtin_mul_overflow(base, base, &base))
					return BigIntObject::pow(a, b);
			}
			return Value::Int(r);
		} else if (a.isNumber() && b.isNumber()) {
			return Value::Float(std::pow(a.asFloat(), b.asFloat()));
		}
	} else if (op == TokenType::LT) {
		if (a.type == ValueType::INT && b.type == ValueType::INT)
			return Value::Bool(a.iVal < b.iVal);
		if (a.isNumber() && b.isNumber())
			return Value::Bool(a.asFloat() < b.asFloat());
		if (a.type == ValueType::STRING && b.type == ValueType::STRING)
			return Value::Bool(valueToString(a) < valueToString(b));
	} else if (op == TokenType::LTE) {
		if (a.type == ValueType::INT && b.type == ValueType::INT)
			return Value::Bool(a.iVal <= b.iVal);
		if (a.isNumber() && b.isNumber())
			return Value::Bool(a.asFloat() <= b.asFloat());
		if (a.type == ValueType::STRING && b.type == ValueType::STRING)
			return Value::Bool(valueToString(a) <= valueToString(b));
	} else if (op == TokenType::GT) {
		if (a.type == ValueType::INT && b.type == ValueType::INT)
			return Value::Bool(a.iVal > b.iVal);
		if (a.isNumber() && b.isNumber())
			return Value::Bool(a.asFloat() > b.asFloat());
		if (a.type == ValueType::STRING && b.type == ValueType::STRING)
			return Value::Bool(valueToString(a) > valueToString(b));
	} else if (op == TokenType::GTE) {
		if (a.type == ValueType::INT && b.type == ValueType::INT)
			return Value::Bool(a.iVal >= b.iVal);
		if (a.isNumber() && b.isNumber())
			return Value::Bool(a.asFloat() >= b.asFloat());
		if (a.type == ValueType::STRING && b.type == ValueType::STRING)
			return Value::Bool(valueToString(a) >= valueToString(b));
	} else if (op == TokenType::EQ) {
		return Value::Bool(a.looseEquals(b));
	} else if (op == TokenType::NEQ) {
		return Value::Bool(!a.looseEquals(b));
	} else if (op == TokenType::STRICT_EQ) {
		return Value::Bool(a.strictEquals(b));
	} else if (op == TokenType::STRICT_NEQ) {
		return Value::Bool(!a.strictEquals(b));
	} else if (op == TokenType::AND) {
		return Value::Bool(a.isTruthy() && b.isTruthy());
	} else if (op == TokenType::OR) {
		return Value::Bool(a.isTruthy() || b.isTruthy());
	}
	throw std::runtime_error("Unsupported operand types for this operator");
}

// `catch X` matches X and every error type below it (also used by transpiled code)
bool errorTypeMatches(const std::string &errType, const std::string &catchType) {
	static const std::unordered_map<std::string, std::string>
		parentMap = {// MathError Subtree
			{"DivisionByZeroError", "MathError"},
			{"OverflowError", "MathError"},
			{"UnderflowError", "MathError"},
			{"DomainError", "MathError"},
			{"MathError", "RuntimeError"},
	
			// IOError Subtree
			{"FileNotFoundError", "IOError"},
			{"PermissionError", "IOError"},
			{"EOFError", "IOError"},
			{"FileClosedError", "IOError"},
			{"IOError", "RuntimeError"},
	
			// ImportError Subtree
			{"ModuleNotFoundError", "ImportError"},
			{"CircularImportError", "ImportError"},
			{"InvalidImportError", "ImportError"},
			{"ImportError", "RuntimeError"},
	
			// ValueError/ConstError Subtrees
			{"EmptyContainerError", "ValueError"},
			{"ValueError", "RuntimeError"},
			{"MutationError", "ConstError"},
			{"ConstError", "RuntimeError"},
	
			// SystemError Subtree
			{"OSError", "SystemError"},
			{"EnvironmentError", "SystemError"},
			{"SignalError", "SystemError"},
			{"SystemError", "RuntimeError"},
	
			// ParseError Subtree
			{"SyntaxError", "ParseError"},
			{"IndentationError", "ParseError"},
			{"UnexpectedTokenError", "ParseError"},
			{"UnterminatedLiteralError", "ParseError"},
			{"ParseError", "Error"},
	
			// ControlFlowError Subtree
			{"ReturnSignal", "ControlFlowError"},
			{"BreakSignal", "ControlFlowError"},
			{"ContinueSignal", "ControlFlowError"},
			{"ControlFlowError", "Error"},
	
			// Warning Subtree
			{"DeprecationWarning", "Warning"},
			{"RuntimeWarning", "Warning"},
			{"ImportWarning", "Warning"},
			{"Warning", "Error"},
	
			// Direct RuntimeError Children
			{"NameError", "RuntimeError"},
			{"AttributeError", "RuntimeError"},
			{"TypeError", "RuntimeError"},
			{"ArgumentError", "RuntimeError"},
			{"OwnershipError", "RuntimeError"},
			{"IndexError", "RuntimeError"},
			{"KeyError", "RuntimeError"},
			{"RangeError", "RuntimeError"},
			{"AssertionError", "RuntimeError"},
			{"RecursionError", "RuntimeError"},
			{"CastError", "RuntimeError"},
			{"IteratorError", "RuntimeError"},
			{"TimeoutError", "RuntimeError"},
			{"MemoryError", "RuntimeError"},
	
			// Roots
			{"RuntimeError", "Error"},
			{"InternalError", "Error"}};
	if (catchType == "Error")
		return true;
	for (std::string cur = errType;;) {
		if (cur == catchType)
			return true;
		auto it = parentMap.find(cur);
		if (it == parentMap.end())
			return false;
		cur = it->second;
	}
}

// base[index] without dunders: the VM's OP_GET_INDEX, also called by transpiled code
Value indexValue(Value base, const Value &index, int line, int col) {
	while (base.type == ValueType::REFERENCE) {
		if (!base.get_ptr_safe())
			throw RuntimeError("Dead-end reference", line, col);
		base = *(base.get_ptr_safe());
	}
	auto getSliceIndices = [&](size_t rawLen) -> std::vector<long long> {
		auto *s = static_cast<SliceObject *>(index.ref.get());
		long long len = (long long)rawLen;
		long long step = 1;
		if (s->step.type == ValueType::INT)
			step = s->step.asInt();
		if (step == 0)
			throw ValueError("Slice step cannot be zero", line,
				col);
		long long start, end;
		if (step > 0) {
			start = (s->start.type == ValueType::INT)
						  ? s->start.asInt()
						  : 0;
			end = (s->end.type == ValueType::INT) ? s->end.asInt()
															  : len;
		} else {
			start = (s->start.type == ValueType::INT)
						  ? s->start.asInt()
						  : len - 1;
			end = (s->end.type == ValueType::INT) ? s->end.asInt()
															  : -1;
		}
		if (s->start.type == ValueType::INT && start < 0)
			start += len;
		if (s->end.type == ValueType::INT && end < 0)
			end += len;
		if (step > 0) {
			if (start < 0)
				start = 0;
			if (end < 0)
				end = 0;
			if (start > len)
				start = len;
			if (end > len)
				end = len;
		} else {
			if (start > len - 1)
				start = len - 1;
			if (end > len - 1)
				end = len - 1;
			if (start < -1)
				start = -1;
			if (end < -1)
				end = -1;
		}
		std::vector<long long> result;
		if (step > 0) {
			for (long long i = start; i < end; i += step)
				if (i >= 0 && i < len)
					result.push_back(i);
		} else {
			for (long long i = start; i > end; i += step)
				if (i >= 0 && i < len)
					result.push_back(i);
		}
		return result;
	};
		switch (base.type) {
		case ValueType::LIST: {
			auto *list = static_cast<ListObject *>(base.ref.get());
			if (index.type == ValueType::SLICE) {
				auto indices =
					getSliceIndices(list->elements.size());
				auto newList = std::make_shared<ListObject>();
				newList->elements.reserve(indices.size());
				for (long long i : indices)
					newList->elements.push_back(list->elements[i]);
				return (Value::List(newList->elements));
			} else {
				if (!index.isNumber())
					throw TypeError("List index must be int or slice",
						line, col);
				long long idx = index.asInt();
				if (idx < 0)
					idx += list->elements.size();
				if (idx < 0 ||
					 idx >= (long long)list->elements.size())
					throw IndexError("List index out of range", line,
						col);
				return (list->elements[idx]);
			}
			break;
		}
		case ValueType::SET: {
			throw TypeError("Sets are unordered collections and do not support indexing or slicing.", line, col);
		}
		case ValueType::VECTOR: {
			auto *vec = static_cast<VectorObject *>(base.ref.get());
			if (index.type == ValueType::SLICE) {
				auto indices = getSliceIndices(vec->elements.size());
				std::vector<Value> newElems;
				newElems.reserve(indices.size());
				for (long long i : indices)
					newElems.push_back(vec->elements[i]);
				return (Value::Vector(newElems));
			} else {
				if (!index.isNumber())
					throw TypeError(
						"Vector index must be int or slice", line,
						col);
				long long idx = index.asInt();
				if (idx < 0)
					idx += vec->elements.size();
				if (idx < 0 ||
					 idx >= (long long)vec->elements.size())
					throw IndexError("Vector index out of range",
						line, col);
				return (vec->elements[idx]);
			}
			break;
		}
		case ValueType::TUPLE: {
			auto *tuple = static_cast<TupleObject *>(base.ref.get());
			if (index.type == ValueType::SLICE) {
				auto indices =
					getSliceIndices(tuple->elements.size());
				std::vector<Value> newElems;
				newElems.reserve(indices.size());
				for (long long i : indices)
					newElems.push_back(tuple->elements[i]);
				return (Value::Tuple(newElems));
			} else {
				if (!index.isNumber())
					throw TypeError(
						"Tuple index must be int or slice", line,
						col);
				long long idx = index.asInt();
				if (idx < 0)
					idx += tuple->elements.size();
				if (idx < 0 ||
					 idx >= (long long)tuple->elements.size())
					throw IndexError("Tuple index out of range", line,
						col);
				return (tuple->elements[idx]);
			}
			break;
		}
		case ValueType::DICT: {
			auto *dict = static_cast<DictObject *>(base.ref.get());
			if (dict->items.count(index) == 0)
				throw KeyError(
					"Key not found: " + valueToString(index), line,
					0);
			return (dict->items.at(index));
			break;
		}
		case ValueType::STRING: {
			string s = base.asString();
			if (index.type == ValueType::SLICE) {
				auto indices = getSliceIndices(s.length());
				string newStr = "";
				newStr.reserve(indices.size());
				for (long long i : indices)
					newStr += s[i];
				return (Value::String(newStr));
			} else {
				if (!index.isNumber())
					throw TypeError(
						"String index must be int or slice", line,
						col);
				long long idx = index.asInt();
				if (idx < 0)
					idx += s.length();
				if (idx < 0 || idx >= (long long)s.length())
					throw IndexError("String index out of range",
						line, col);
				return (Value::String(string(1, s[idx])));
			}
			break;
		}
		case ValueType::RANGE: {
			auto *rng = static_cast<RangeObject *>(base.ref.get());
			if (!index.isNumber())
				throw TypeError("Range index must be a number", line,
					col);
			long long idx = index.asInt();
			long long len =
				(long long)((rng->end - rng->start) / rng->step);
			if (!rng->endInclusive) {
				if ((rng->step > 0 && rng->end > rng->start) ||
					 (rng->step < 0 && rng->end < rng->start))
					len = (long long)ceil((rng->end - rng->start) /
												 rng->step);
				else
					len = 0;
			} else
				len++;
			if (idx < 0)
				throw IndexError("Range index cannot be negative",
					line, col);
			double val = rng->start + (idx * rng->step);
			if (rng->step > 0 && val >= rng->end &&
				 !rng->endInclusive)
				throw IndexError("Range index out of range", line,
					col);
			return (rng->isFloat
									 ? Value::Float(val)
									 : Value::Int((long long)val));
			break;
		}
		default:
			throw TypeError("Object is not subscriptable", line, col);
			break;
		}
}
