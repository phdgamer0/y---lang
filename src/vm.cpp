#include "ylang/vm.hpp"
#include "objects.hpp"
#include "types.hpp"

#ifdef _WIN32
#include <shellapi.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif

void VM::run(Chunk &chunk) {
	CallFrame mainFrame;
	mainFrame.function = nullptr;
	mainFrame.ip = chunk.code.data();
	mainFrame.basePointer = stack.size();
	frames.push_back(mainFrame);
	frame = &frames.back();
	ip = frame->ip;
	Value pendingReturn = Value::NoType();
	bool isReturning = false;
	Value pendingError = Value::NoType();
	bool isHandlingError = false;
	int line = 0;
	int col = 0;
#ifdef USE_COMPUTED_GOTOS
	static void *dispatch_table[] = {
		&&TARGET_OP_CONSTANT, &&TARGET_OP_CONSTANT_LONG, &&TARGET_OP_TRUE,
		&&TARGET_OP_FALSE, &&TARGET_OP_NONE, &&TARGET_OP_NOTYPE,

		// Variables & Scope
		&&TARGET_OP_DEFINE_VAR, &&TARGET_OP_GET_VAR, &&TARGET_OP_SET_VAR,
		&&TARGET_OP_DEEP_COPY, &&TARGET_OP_REF_LOCAL, &&TARGET_OP_DEFINE_REF,
		&&TARGET_OP_REF_VAR, &&TARGET_OP_REF_INDEX, &&TARGET_OP_SET_REF,
		&&TARGET_OP_SHALLOW_COPY, &&TARGET_OP_MULTI_SET,
		&&TARGET_OP_GET_LOCAL, &&TARGET_OP_SET_LOCAL, &&TARGET_OP_INC_LOCAL,
		&&TARGET_OP_SET_FLAGS, &&TARGET_OP_REF_PROPERTY, &&TARGET_OP_DELETE,

		// Arithmetic & Logic
		&&TARGET_OP_ADD, &&TARGET_OP_SUB, &&TARGET_OP_MUL, &&TARGET_OP_DIV, &&TARGET_OP_FAST_IADD, &&TARGET_OP_FAST_ISUB, &&TARGET_OP_FAST_IMUL, &&TARGET_OP_FAST_IDIV, &&TARGET_OP_FAST_FADD, &&TARGET_OP_FAST_FSUB, &&TARGET_OP_FAST_FMUL, &&TARGET_OP_FAST_FDIV, &&TARGET_OP_FAST_SADD, &&TARGET_OP_CTLE_TICK,
		&&TARGET_OP_FLOOR_DIV, &&TARGET_OP_MOD, &&TARGET_OP_POW,
		&&TARGET_OP_IADD, &&TARGET_OP_ISUB, &&TARGET_OP_IMUL,
		&&TARGET_OP_IDIV, &&TARGET_OP_IFLOOR_DIV, &&TARGET_OP_IMOD,
		&&TARGET_OP_IPOW, &&TARGET_OP_DUP, &&TARGET_OP_DUP_2, &&TARGET_OP_EQ,
		&&TARGET_OP_NEQ, &&TARGET_OP_LT, &&TARGET_OP_GT, &&TARGET_OP_LTE,
		&&TARGET_OP_GTE, &&TARGET_OP_COLON, &&TARGET_OP_STRICT_NEQ,
		&&TARGET_OP_NOT, &&TARGET_OP_AND, &&TARGET_OP_OR, &&TARGET_OP_XOR,
		&&TARGET_OP_IS, &&TARGET_OP_IN, &&TARGET_OP_IS_NOT,
		&&TARGET_OP_STRICT_EQ, &&TARGET_OP_IS_IN, &&TARGET_OP_IS_NOT_IN,
		&&TARGET_OP_NXOR, &&TARGET_OP_NAND, &&TARGET_OP_NOR,
		&&TARGET_OP_NEGATE, &&TARGET_OP_INCREMENT, &&TARGET_OP_DECREMENT,
		&&TARGET_OP_BITWISE_AND, &&TARGET_OP_BITWISE_OR, &&TARGET_OP_BITWISE_XOR,
		&&TARGET_OP_BITWISE_NOT, &&TARGET_OP_BITWISE_SHL, &&TARGET_OP_BITWISE_SHR,
		&&TARGET_OP_PRE_INCREMENT, &&TARGET_OP_PRE_DECREMENT,

		// Containers
		&&TARGET_OP_BUILD_LIST, &&TARGET_OP_BUILD_TUPLE,
		&&TARGET_OP_BUILD_SET, &&TARGET_OP_BUILD_DICT,
		&&TARGET_OP_UNPACK_DICT, &&TARGET_OP_BUILD_RANGE,
		&&TARGET_OP_BUILD_VECTOR, &&TARGET_OP_BUILD_FSTRING,
		&&TARGET_OP_BUILD_FILE, &&TARGET_OP_BUILD_SLICE,

		// OOP
		&&TARGET_OP_CLASS, &&TARGET_OP_METHOD, &&TARGET_OP_GET_PROPERTY,
		&&TARGET_OP_SET_PROPERTY, &&TARGET_OP_INC_PROPERTY, &&TARGET_OP_CLASS_FIELD, &&TARGET_OP_SUPER,

		// Comprehension
		&&TARGET_OP_LIST_APPEND, &&TARGET_OP_SET_ADD, &&TARGET_OP_DICT_SET,
		&&TARGET_OP_LIST_TO_TUPLE, &&TARGET_OP_LIST_TO_VECTOR,

		// Access & Calls
		&&TARGET_OP_GET_INDEX, &&TARGET_OP_SET_INDEX, &&TARGET_OP_INC_INDEX, &&TARGET_OP_INVOKE,
		&&TARGET_OP_CALL,

		// Control Flow
		&&TARGET_OP_JUMP, &&TARGET_OP_JUMP_IF_FALSE, &&TARGET_OP_LOOP,
		&&TARGET_OP_RETURN, &&TARGET_OP_TO_STREAM, &&TARGET_OP_JUMP_IF_NOT_LT,
		&&TARGET_OP_BREAK, &&TARGET_OP_CONTINUE, &&TARGET_OP_SKIP,
		&&TARGET_OP_OMIT, &&TARGET_OP_FOR_ITER, &&TARGET_OP_SKIP_ITER,
		&&TARGET_OP_SWITCH_TABLE,

		// Errors & Systems
		&&TARGET_OP_THROW, &&TARGET_OP_ASSERT, &&TARGET_OP_IMPORT,
		&&TARGET_OP_POP, &&TARGET_OP_DEBUG_NAME, &&TARGET_OP_REPL_PRINT, &&TARGET_OP_TRY_ENTER,
		&&TARGET_OP_TRY_EXIT, &&TARGET_OP_CATCH, &&TARGET_OP_RETHROW,
		&&TARGET_OP_END_FINALLY};
#endif
	auto invokeBinaryDunder = [&](Value a, Value b, const string &leftOp, const string &rightOp, int line, int col) -> bool {
		if (a.type == ValueType::INSTANCE) {
			auto *instanceA = static_cast<InstanceObject *>(a.ref.get());
			ClassObject *clsA = instanceA->klass;
			ClassObject::MethodInfo *methodA = nullptr;
			for (auto *ancestor : clsA->mro) {
				if (ancestor->methods.count(leftOp)) {
					methodA = &ancestor->methods[leftOp];
					break;
				}
			}
			if (methodA) {
				stack.push_back(a);
				Value objVal;
				objVal.type = ValueType::CLASS;
				objVal.ref =
					std::shared_ptr<HeapObject>(clsA, [](HeapObject *) {});
				stack.push_back(objVal);
				stack.push_back(b);
				callValue(methodA->func, 3, line, col);
				return true;
			}
		}
		if (b.type == ValueType::INSTANCE) {
			auto *instanceB = static_cast<InstanceObject *>(b.ref.get());
			ClassObject *clsB = instanceB->klass;
			ClassObject::MethodInfo *methodB = nullptr;
			for (auto *ancestor : clsB->mro) {
				if (ancestor->methods.count(rightOp)) {
					methodB = &ancestor->methods[rightOp];
					break;
				}
			}
			if (methodB) {
				stack.push_back(b);
				Value objVal;
				objVal.type = ValueType::CLASS;
				objVal.ref =
					std::shared_ptr<HeapObject>(clsB, [](HeapObject *) {});
				stack.push_back(objVal);
				stack.push_back(a);
				callValue(methodB->func, 3, line, col);
				return true;
			}
		}
		return false;
	};
	auto checkFieldAccess = [&](ClassObject *targetClass, const string &name, int line, int col) {
		if (targetClass->fieldAccess.count(name)) {
			AccessLevel access = targetClass->fieldAccess[name];
			if (access != AccessLevel::PUBLIC) {
				bool allowed = false;
				if (frame->function && frame->function->owner) {
					ClassObject *callerCls = frame->function->owner;
					if (access == AccessLevel::PRIVATE &&
						 callerCls == targetClass)
						allowed = true;
					else if (access == AccessLevel::PROTECTED) {
						for (auto *ancestor : callerCls->mro) {
							if (ancestor == targetClass) {
								allowed = true;
								break;
							}
						}
					}
				}
				if (!allowed)
					throw RuntimeError(
						"Cannot access private/protected field '" + name + "'",
						line, col);
			}
		}
	};
	auto invokeUnaryDunder = [&](Value a, const string &opName, int line, int col) -> bool {
		if (a.type == ValueType::INSTANCE) {
			auto *instanceA = static_cast<InstanceObject *>(a.ref.get());
			ClassObject *clsA = instanceA->klass;
			for (auto *ancestor : clsA->mro) {
				if (ancestor->methods.count(opName)) {
					stack.push_back(a);
					Value objVal;
					objVal.type = ValueType::CLASS;
					objVal.ref = std::shared_ptr<HeapObject>(clsA, [](HeapObject *) {});
					stack.push_back(objVal);
					callValue(ancestor->methods[opName].func, 2, line, col);
					return true;
				}
			}
		}
		return false;
	};
	while (true) {
		Chunk *currentChunk = frame->function ? frame->function->chunk : &chunk;
		try {
			OpCode instruction;
#ifdef USE_COMPUTED_GOTOS
			DISPATCH();
#else
		loop_start:
			if (gc_alloc_count > 50000) {
				gc_collect();
				gc_alloc_count = 0;
			}
			currentChunk = frame->function ? frame->function->chunk : &chunk;
			RUN_DEBUGGER();
			instruction = static_cast<OpCode>(*ip++);
			int offset = (int)(ip - currentChunk->code.data());
			line = currentChunk->lines[offset - 1];
			col = currentChunk->columns[offset - 1];
			switch (instruction)
#endif
			{
				OP(OP_REPL_PRINT) : {
					{
						Value v = pop();
						if (printOutput /*v.type != ValueType::NOTYPE && v.type != ValueType::NONE*/) {
							std::cout << "-> " << valueToString(v) << "\n";
						}
					}
					DISPATCH();
				}
				OP(OP_CLASS) : {
					{
						uint8_t parentCount = *ip++;
						string name = pop().asString();
						Value classVal = Value::Class(name);
						auto *newClassObj =
							static_cast<ClassObject *>(classVal.ref.get());
						for (int i = 0; i < parentCount; i++) {
							Value pVal = pop();
							if (pVal.type != ValueType::CLASS)
								throw RuntimeError("Superclass must be a class", line,
									col);
							newClassObj->parents.push_back(pVal);
						}
						std::reverse(newClassObj->parents.begin(),
							newClassObj->parents.end());
						newClassObj->computeMRO();
						stack.push_back(classVal);
					}
					DISPATCH();
				}
				OP(OP_SUPER) : {
					{
						Value self = stack[frame->basePointer];
						if (self.type != ValueType::INSTANCE)
							throw RuntimeError("super() must be called on an instance",
								line, col);
						FunctionObject *currentFunc = frame->function;
						if (!currentFunc || !currentFunc->owner) {
							throw RuntimeError(
								"super() used in a function that is not a method.",
								line, col);
						}
						ClassObject *definingClass = currentFunc->owner;
						auto *instance = static_cast<InstanceObject *>(self.ref.get());
						ClassObject *trueClass = instance->klass;
						ClassObject *superTarget = nullptr;
						bool foundDefining = false;
						for (auto *ancestor : trueClass->mro) {
							if (foundDefining) {
								superTarget = ancestor;
								break;
							}
							if (ancestor == definingClass)
								foundDefining = true;
						}
						if (!superTarget)
							throw RuntimeError(
								"super(): No superclass found (reached top of MRO).",
								line, col);
						Value v;
						v.type = ValueType::SUPER;
						v.ref = make_shared<SuperObject>(self, superTarget);
						stack.push_back(v);
					}
					DISPATCH();
				}
				OP(OP_METHOD) : {
					{
						uint8_t accessByte = *ip++;
						AccessLevel access = (AccessLevel)accessByte;
						Value funcVal = pop();
						Value classVal = stack.back();
						if (classVal.type != ValueType::CLASS)
							throw RuntimeError("Cannot define method on non-class",
								line, col);
						auto *cls = static_cast<ClassObject *>(classVal.ref.get());
						auto *func = static_cast<FunctionObject *>(funcVal.ref.get());
						func->owner = cls;
						if (cls->methods.count(func->name)) {
							Value existing = cls->methods[func->name].func;
							if (existing.type == ValueType::FUNCTION || existing.type == ValueType::NATIVE_FUNCTION) {
								auto ovObj = std::make_shared<OverloadObject>(existing);
								ovObj->overloads.push_back(funcVal);
								Value ovVal;
								ovVal.type = ValueType::OVERLOAD;
								ovVal.ref = ovObj;
								cls->methods[func->name] = {ovVal, access};
							} else if (existing.type == ValueType::OVERLOAD) {
								auto ovObj = std::make_shared<OverloadObject>(existing);
								ovObj->overloads.push_back(funcVal);
							} else {
								cls->methods[func->name] = {funcVal, access};
							}
						} else {
							cls->methods[func->name] = {funcVal, access};
						}
					}
					DISPATCH();
				}
				OP(OP_GET_PROPERTY) : {
					{
						uint8_t nameIdx = *ip++;
						string name = currentChunk->constants[nameIdx].asString();
						Value obj = pop();
						if (name == "__mro__" || name == "__var_count__" || name == "__var_names__" ||
							 name == "__function_count__" || name == "__function_names__" || name == "__var_reverse_pairs__" ||
							 name == "__all_count__" || name == "__all_names__" || name == "__var_values__" || name == "__var_pairs__") {
							ClassObject *cls = nullptr;
							InstanceObject *inst = nullptr;
							if (obj.type == ValueType::INSTANCE) {
								inst = static_cast<InstanceObject *>(obj.ref.get());
								cls = inst->klass;
							} else if (obj.type == ValueType::CLASS) {
								cls = static_cast<ClassObject *>(obj.ref.get());
							}
							if (name == "__mro__") {
								auto mroList = std::make_shared<ListObject>();
								if (cls) {
									for (auto *ancestor : cls->mro) {
										mroList->elements.push_back(Value::String(ancestor->name));
									}
								}
								stack.push_back(Value::List(mroList->elements));
							} else {
								bool wantsNames = (name.find("_names__") != string::npos);
								bool wantsValues = (name.find("_values__") != string::npos);
								bool wantsPairs = (name.find("_pairs__") != string::npos);
								bool wantsReverse = (name.find("_reverse_") != string::npos);
								bool wantsVars = (name.find("__var_") != string::npos || name.find("__all_") != string::npos);
								bool wantsFuncs = (name.find("__function_") != string::npos || name.find("__all_") != string::npos);
								std::unordered_set<std::string> seen;
								std::unordered_set<Value, ValueHash, ValueEqual> seenVal;
								auto nameList = std::make_shared<SetObject>();
								auto valueList = std::make_shared<SetObject>();
								auto pairDict = std::make_shared<DictObject>();
								auto addName = [&](const std::string &n) {
									if (seen.insert(n).second && wantsNames) {
										nameList->elements.insert(Value::String(n));
									}
								};
								auto addVal = [&](const Value &n) {
									if (seenVal.insert(n).second && wantsValues) {
										valueList->elements.insert(n);
									}
								};
								auto addPair = [&](const std::string &n, const Value &v) {
									if (seen.insert(n).second && seenVal.insert(v).second && wantsPairs) {
										pairDict->items[Value::String(n)] = v;
									}
								};
								auto addReversePair = [&](const std::string &n, const Value &v) {
									if (seen.insert(n).second && seenVal.insert(v).second && wantsPairs && wantsReverse) {
										pairDict->items[v] = Value::String(n);
									}
								};
								if (cls) {
									if (wantsVars) {
										if (inst) {
											for (const auto &pair : inst->fields) {
												AccessLevel acc = cls->fieldAccess.count(pair.first) ? cls->fieldAccess[pair.first] : AccessLevel::PUBLIC;
												if (acc == AccessLevel::PUBLIC) {
													if (wantsValues) {
														addVal(pair.second);
													} else if (wantsPairs) {
														if (wantsReverse)
															addReversePair(pair.first, pair.second);
														else
															addPair(pair.first, pair.second);
													} else {
														addName(pair.first);
													}
												}
											}
										}
										for (auto *ancestor : cls->mro) {
											for (const auto &pair : ancestor->staticFields) {
												AccessLevel acc = ancestor->fieldAccess.count(pair.first) ? ancestor->fieldAccess[pair.first] : AccessLevel::PUBLIC;
												if (acc == AccessLevel::PUBLIC) {
													if (wantsValues) {
														addVal(pair.second);
													} else if (wantsPairs) {
														if (wantsReverse)
															addReversePair(pair.first, pair.second);
														else
															addPair(pair.first, pair.second);
													} else {
														addName(pair.first);
													}
												}
											}
										}
									}
									if (wantsFuncs) {
										for (auto *ancestor : cls->mro) {
											for (const auto &pair : ancestor->methods) {
												if (pair.second.access == AccessLevel::PUBLIC) {
													addName(pair.first);
												}
											}
										}
									}
								}
								if (wantsNames) {
									stack.push_back(Value::Set(nameList->elements));
								} else if (wantsValues) {
									stack.push_back(Value::Set(valueList->elements));
								} else if (wantsPairs) {
									stack.push_back(Value::Dict(pairDict->items));
								} else {
									stack.push_back(Value::Int(seen.size()));
								}
							}
						} else {
							std::shared_ptr<HeapObject> lifeline = obj.ref;
							if (obj.type == ValueType::REFERENCE) {
								if (obj.get_ptr_safe() == nullptr) {
									throw RuntimeError("Null pointer dereference.", line, col);
								}
								obj = *(obj.get_ptr_safe());
								if (obj.type == ValueType::INSTANCE)
									lifeline = obj.ref;
							}
							if (obj.type == ValueType::INSTANCE) {
								auto *instance =
									static_cast<InstanceObject *>(obj.ref.get());
								checkFieldAccess(instance->klass, name, line, col);
								if (instance->fields.count(name))
									stack.push_back(instance->fields[name]);
								else {
									ClassObject *cls = instance->klass;
									bool found = false;
									for (auto *ancestor : cls->mro) {
										if (ancestor->methods.count(name)) {
											stack.push_back(ancestor->methods[name].func);
											found = true;
											break;
										}
										if (ancestor->staticFields.count(name)) {
											stack.push_back(ancestor->staticFields[name]);
											found = true;
											break;
										}
									}
									if (!found)
										throw AttributeError("Instance of '" + cls->name +
																		"' has no attribute '" +
																		name + "'",
											line, col);
								}
							} else if (obj.type == ValueType::CLASS) {
								auto *cls = static_cast<ClassObject *>(obj.ref.get());
								bool found = false;
								for (auto *ancestor : cls->mro) {
									if (ancestor->staticFields.count(name)) {
										stack.push_back(ancestor->staticFields[name]);
										found = true;
										break;
									}
									if (ancestor->methods.count(name)) {
										stack.push_back(ancestor->methods[name].func);
										found = true;
										break;
									}
								}
								if (!found)
									throw AttributeError("Class '" + cls->name +
																	"' has no attribute '" + name +
																	"'",
										line, col);
							} else {
								bool foundItem = false;
								if (obj.type == ValueType::DICT) {
									auto *dictObj = static_cast<DictObject *>(obj.ref.get());
									Value key = Value::String(name);
									if (dictObj->items.count(key)) {
										stack.push_back(dictObj->items[key]);
										foundItem = true;
									}
								}

								if (!foundItem) {
									Value boundObj = obj;
									std::string boundName = name;
									auto *vm = this;
									auto boundFunc = [boundObj, boundName, vm](const std::vector<Value> &args, int l, int c) -> Value {
										std::vector<ValueExpr *> dummyArgs(args.size());
										for (size_t i = 0; i < args.size(); i++) {
											dummyArgs[i] = new ValueExpr(args[i]);
										}
										ValueExpr *dummyObject = new ValueExpr(boundObj);
										std::vector<Expr *> exprArgs(dummyArgs.begin(), dummyArgs.end());
										MethodCallExpr mockAST(dummyObject, boundName, exprArgs);
										mockAST.line = l;
										mockAST.col = c;

										if (!vm->methodResolver) {
											delete dummyObject;
											for (auto *a : dummyArgs)
												delete a;
											throw EnvironmentError("VM methodResolver bridge not initialized.", l, c);
										}

										Value result;
										try {
											result = vm->methodResolver(&mockAST);
										} catch (...) {
											delete dummyObject;
											for (auto *a : dummyArgs)
												delete a;
											throw;
										}

										delete dummyObject;
										for (auto *a : dummyArgs)
											delete a;
										return result;
									};
									stack.push_back(Value::Native(boundFunc));
								}
							}
						}
					}
					DISPATCH();
				}
				OP(OP_REF_PROPERTY) : {
					{
						uint8_t nameIdx = *ip++;
						string name = currentChunk->constants[nameIdx].asString();
						Value obj = pop();
						std::shared_ptr<HeapObject> lifeline = obj.ref;
						if (obj.type == ValueType::REFERENCE) {
							if (obj.get_ptr_safe() == nullptr) {
								throw RuntimeError("Null pointer dereference.", line, col);
							}
							obj = *(obj.get_ptr_safe());
							if (obj.type == ValueType::INSTANCE)
								lifeline = obj.ref;
						}
						if (obj.type == ValueType::INSTANCE) {
							auto *instance = static_cast<InstanceObject *>(obj.ref.get());
							checkFieldAccess(instance->klass, name, line, col);
							if (instance->fields.find(name) == instance->fields.end()) {
								instance->fields[name] = Value::None();
							}
							stack.push_back(Value::FromExisting(std::make_shared<PropRefObject>(std::static_pointer_cast<InstanceObject>(obj.ref), name), ValueType::REFERENCE));
						} else if (obj.type == ValueType::CLASS) {
							auto *cls = static_cast<ClassObject *>(obj.ref.get());
							Value *ptr = &cls->staticFields[name];
							stack.push_back(Value::Reference(ptr, obj.ref));
						} else if (obj.type == ValueType::DICT) {
							Value key = Value::String(name);
							stack.push_back(Value::FromExisting(std::make_shared<DictRefObject>(std::static_pointer_cast<DictObject>(obj.ref), key), ValueType::REFERENCE));
						} else {
							throw TypeError("Cannot take reference of property on non-object", line, col);
						}
					}
					DISPATCH();
				}
				OP(OP_SET_PROPERTY) : {
					{
						uint8_t nameIdx = *ip++;
						string name = currentChunk->constants[nameIdx].asString();
						uint8_t accessByte = *ip++;
						AccessLevel access = (AccessLevel)accessByte;
						Value val = pop();
						Value obj = pop();
						std::shared_ptr<HeapObject> lifeline = obj.ref;
						if (obj.type == ValueType::REFERENCE) {
							if (obj.get_ptr_safe() == nullptr) {
								throw RuntimeError("Null pointer dereference.", line, col);
							}
							obj = *(obj.get_ptr_safe());
							if (obj.type == ValueType::INSTANCE)
								lifeline = obj.ref;
						}
						if (obj.type == ValueType::INSTANCE) {
							auto *instance = static_cast<InstanceObject *>(obj.ref.get());
							checkFieldAccess(instance->klass, name, line, col);
							instance->fields[name] = val;
							instance->klass->fieldAccess[name] = access;
						} else if (obj.type == ValueType::CLASS) {
							auto *cls = static_cast<ClassObject *>(obj.ref.get());
							cls->staticFields[name] = val;
							cls->fieldAccess[name] = access;
						} else if (obj.type == ValueType::DICT) {
							auto *dictObj = static_cast<DictObject *>(obj.ref.get());
							Value key = Value::String(name);
							dictObj->items[key] = val;
						} else
							throw AttributeError("Cannot set property on non-object",
								line, col);
						stack.push_back(val);
					}
					DISPATCH();
				}
				OP(OP_INC_PROPERTY) : {
					{
						uint8_t nameIdx = *ip++;
						string name = currentChunk->constants[nameIdx].asString();
						uint8_t isInc = *ip++;
						uint8_t isPrefix = *ip++;

						Value obj = pop();
						std::shared_ptr<HeapObject> lifeline = obj.ref;
						if (obj.type == ValueType::REFERENCE) {
							if (obj.get_ptr_safe() == nullptr)
								throw RuntimeError("Null pointer dereference.", line, col);
							obj = *(obj.get_ptr_safe());
							if (obj.type == ValueType::INSTANCE)
								lifeline = obj.ref;
						}

						Value oldVal;
						if (obj.type == ValueType::INSTANCE) {
							auto *instance = static_cast<InstanceObject *>(obj.ref.get());
							if (instance->fields.find(name) != instance->fields.end()) {
								oldVal = instance->fields[name];
							} else {
								throw AttributeError("Instance has no property '" + name + "'", line, col);
							}
						} else if (obj.type == ValueType::DICT) {
							auto *dictObj = static_cast<DictObject *>(obj.ref.get());
							Value key = Value::String(name);
							if (dictObj->items.find(key) != dictObj->items.end()) {
								oldVal = dictObj->items[key];
							} else {
								throw AttributeError("Dictionary has no property '" + name + "'", line, col);
							}
						} else {
							throw AttributeError("Cannot increment property on this object type", line, col);
						}

						Value newVal;
						if (oldVal.type == ValueType::INT) {
							newVal = Value::Int(oldVal.iVal + (isInc ? 1 : -1));
						} else if (oldVal.type == ValueType::FLOAT) {
							newVal = Value::Float(oldVal.fVal + (isInc ? 1.0 : -1.0));
						} else {
							throw TypeError("Increment/Decrement requires numeric property", line, col);
						}

						if (obj.type == ValueType::INSTANCE) {
							auto *instance = static_cast<InstanceObject *>(obj.ref.get());
							instance->fields[name] = newVal;
						} else if (obj.type == ValueType::DICT) {
							auto *dictObj = static_cast<DictObject *>(obj.ref.get());
							Value key = Value::String(name);
							dictObj->items[key] = newVal;
						}

						stack.push_back(isPrefix ? newVal : oldVal);
					}
					DISPATCH();
				}
				OP(OP_CLASS_FIELD) : {
					{
						uint8_t accessByte = *ip++;
						AccessLevel access = (AccessLevel)accessByte;
						uint8_t nameIdx = *ip++;
						string name = currentChunk->constants[nameIdx].asString();
						Value val = pop();
						Value classVal = stack.back();
						if (classVal.type != ValueType::CLASS)
							throw RuntimeError(
								"Cannot define static field on non-class", line, col);
						auto *cls = static_cast<ClassObject *>(classVal.ref.get());
						cls->fieldAccess[name] = access;
						if (val.type != ValueType::NOTYPE)
							cls->staticFields[name] = val;
					}
					DISPATCH();
				}
				OP(OP_DUP) : {
					{
						stack.push_back(stack.back());
					}
					DISPATCH();
				}
				OP(OP_DUP_2) : {
					{
						if (stack.size() < 2)
							throw UnderflowError("Stack underflow for DUP_2", line,
								col);
						Value top = stack.back();
						Value under = stack[stack.size() - 2];
						stack.push_back(under);
						stack.push_back(top);
					}
					DISPATCH();
				}
				OP(OP_CALL) : {
					{
						if (stack.size() > 65000)
							throw RecursionError(
								"Stack Overflow Maximum call stack size exceeded.",
								line, col);
						uint8_t argCount = *ip++;
						Value callee = pop();
						if (callee.type == ValueType::NATIVE_FUNCTION) {
							vector<Value> args(stack.end() - argCount, stack.end());
							stack.erase(stack.end() - argCount, stack.end());
							auto native =
								static_cast<NativeFunctionObject *>(callee.ref.get());
							Value result = native->func(args, line, 0);
							stack.push_back(result);
						} else if (callee.type == ValueType::CLASS) {
							Value instance = Value::Instance(callee);
							auto *cls = static_cast<ClassObject *>(callee.ref.get());
							if (cls->methods.count(magic_methods_to_string(
									 Magic_Methods::__construct__))) {
								Value initMethod =
									cls->methods[magic_methods_to_string(
														 Magic_Methods::__construct__)]
										.func;
								vector<Value> args;
								for (int i = 0; i < argCount; i++)
									args.push_back(pop());
								std::reverse(args.begin(), args.end());
								stack.push_back(instance);
								stack.push_back(callee);
								for (auto &a : args)
									stack.push_back(a);
								callValue(initMethod, argCount + 2, line, col);
							} else {
								for (int i = 0; i < argCount; i++)
									pop();
								stack.push_back(instance);
							}
						} else if (callee.type == ValueType::INSTANCE) {
							auto *inst = static_cast<InstanceObject *>(callee.ref.get());
							ClassObject *cls = inst->klass;
							ClassObject::MethodInfo *callMethod = nullptr;
							ClassObject *methodOwner = nullptr;
							for (auto *ancestor : cls->mro) {
								if (ancestor->methods.count("__call__")) {
									callMethod = &ancestor->methods["__call__"];
									methodOwner = ancestor;
									break;
								}
							}
							if (callMethod) {
								Value classObjVal;
								classObjVal.type = ValueType::CLASS;
								classObjVal.ref = std::shared_ptr<HeapObject>(methodOwner, [](HeapObject *) {});
								vector<Value> args;
								for (int i = 0; i < argCount; i++)
									args.push_back(pop());
								std::reverse(args.begin(), args.end());
								stack.push_back(callee);
								stack.push_back(classObjVal);
								for (auto &a : args)
									stack.push_back(a);
								callValue(callMethod->func, argCount + 2, line, col);
							} else {
								throw TypeError("Instance object is not callable", line, col);
							}
						} else
							callValue(callee, argCount, line, col);
					}
					DISPATCH();
				}
				OP(OP_IMPORT) : {
					{
						uint8_t count = *ip++;
						std::vector<std::string> symbols;
						for (int i = 0; i < count; i++)
							symbols.push_back(pop().asString());
						std::reverse(symbols.begin(), symbols.end());
						std::string libName = pop().asString();
						Value moduleResult;
						bool isStar = (count == 1 && symbols[0] == "*");
						if (libName.length() > 4 && libName.substr(libName.length() - 4) == ".ymm") {
							namespace fs = std::filesystem;
							fs::path p(libName);
							if (!fs::exists(p))
								throw ImportError("Module file not found: " + libName, line, col);
							std::error_code ec;
							std::string absPath = fs::absolute(p, ec).string();

							// Check if already loaded in globals
							// We can check if a special dictionary or variable exists
							// Since this is VM::run, we need to know if the module was loaded
							if (globals->exists(absPath)) {
								moduleResult = globals->get(absPath);
							} else {
								if (importStack.count(absPath))
									throw CircularImportError("Circular import detected: " + libName, line, col);
								std::ifstream file(absPath);
								if (!file)
									throw FileNotFoundError("Unable to read module: " + libName, line, col);
								std::stringstream buffer;
								buffer << file.rdbuf();
								std::string source = buffer.str();
								importStack.insert(absPath);
								try {
									auto tokens = tokenize(source);
									Parser parser(tokens);
									Chunk moduleChunk;
									ByteCodeCompiler moduleCompiler(&moduleChunk);
									while (!parser.isAtEnd()) {
										Stmt *stmt = parser.parseStmt();
										moduleCompiler.compileStmt(stmt);
									}
									moduleCompiler.emitByte(OpCode::OP_NONE, 0, 0);
									moduleCompiler.emitByte(OpCode::OP_RETURN, 0, 0);
									VM moduleVM;
									moduleVM.globals->set("None", Value::None(), true);
									moduleVM.methodResolver = this->methodResolver;
									moduleVM.importResolver = this->importResolver;
									moduleVM.run(moduleChunk);
									auto exportDict = std::make_shared<DictObject>();
									for (const auto &[key, var] : moduleVM.globals->vars) {
										if (key == "None")
											continue;
										exportDict->items[Value::String(key)] = var.value;
										if (isStar) {
											if (!globals->exists(key)) {
												globals->set(key, var.value, var.isLocked,
													var.isConst);
											}
										}
									}
									moduleResult = Value::Dict(exportDict->items);
									// Cache the module result using the absolute path
									globals->set(absPath, moduleResult, false, false);
								} catch (...) {
									importStack.erase(absPath);
									throw;
								}
								importStack.erase(absPath);
							}
						} else {
							if (this->importResolver) {
								this->importResolver(libName, symbols, globals);
								if (count == 0) {
									if (globals->exists(libName))
										moduleResult = globals->get(libName);
									else
										moduleResult = Value::None();
								} else
									moduleResult = Value::None();
							} else
								throw EnvironmentError("Import resolver not linked!", line, col);
						}
						if (count == 0)
							stack.push_back(moduleResult);
						else {
							if (isStar) {
								stack.push_back(Value::None());
							} else {
								auto findVal = [&](std::string key) -> Value {
									if (moduleResult.type == ValueType::DICT) {
										auto *d = static_cast<DictObject *>(moduleResult.ref.get());
										Value k = Value::String(key);
										if (d->items.count(k))
											return d->items.at(k);
									}
									if (globals->exists(key))
										return globals->get(key);
									throw InvalidImportError("Module '" + libName + "' does not export '" + key + "'", line, col);
								};
								for (const auto &sym : symbols)
									stack.push_back(findVal(sym));
							}
						}
					}
					DISPATCH();
				}
				OP(OP_DEBUG_NAME) : {
					{
						uint8_t nameIndex = *ip++;
						string name = currentChunk->constants[nameIndex].asString();

#ifdef VM_DEBUG_MODE
						stack.back().__DEBUGGING__NAME__ = name;
#endif
					}
					DISPATCH();
				}
				OP(OP_SET_FLAGS) : {
					{
						uint8_t flags = *ip++;
						if (stack.empty())
							throw UnderflowError("Stack underflow", line, col);
						stack.back().isConst = (flags & 0x01);
						stack.back().isLocked = (flags & 0x02);
					}
					DISPATCH();
				}
				OP(OP_LIST_APPEND) : {
					{
						uint8_t slot = *ip++;
						Value val = pop();
						Value &listVal = stack[stack.size() - 4];
						if (listVal.type != ValueType::LIST)
							throw TypeError("Append target is not a list", line, col);
						auto *list = static_cast<ListObject *>(listVal.ref.get());
						list->elements.push_back(val);
					}
					DISPATCH();
				}
				OP(OP_SET_ADD) : {
					{
						uint8_t slot = *ip++;
						Value val = pop();
						Value &setVal = stack[stack.size() - 4];
						if (setVal.type != ValueType::SET)
							throw TypeError("Add target is not a set", line, col);
						auto *set = static_cast<SetObject *>(setVal.ref.get());
						setAdd(set->elements, val);
					}
					DISPATCH();
				}
				OP(OP_DICT_SET) : {
					{
						uint8_t slot = *ip++;
						Value val = pop();
						Value key = pop();
						Value &dictVal = stack[stack.size() - 4];
						if (dictVal.type != ValueType::DICT)
							throw TypeError("Target is not a dict", line, col);
						auto *dict = static_cast<DictObject *>(dictVal.ref.get());
						if (key.type == ValueType::LIST ||
							 key.type == ValueType::SET ||
							 key.type == ValueType::DICT) {
							if (key.type == ValueType::DICT)
								throw TypeError("Dictionary cannot be used as a key",
									line, col);
							key = deepCopy(key);
							key.isConst = true;
						}
						dict->items[key] = val;
					}
					DISPATCH();
				}
				OP(OP_LIST_TO_TUPLE) : {
					{
						Value v = pop();
						if (v.type != ValueType::LIST)
							throw TypeError("Expected list for tuple conversion", line,
								col);
						auto *list = static_cast<ListObject *>(v.ref.get());
						stack.push_back(Value::Tuple(list->elements));
					}
					DISPATCH();
				}
				OP(OP_LIST_TO_VECTOR) : {
					{
						Value v = pop();
						if (v.type != ValueType::LIST)
							throw TypeError("Expected list for vector conversion",
								line, col);
						auto *list = static_cast<ListObject *>(v.ref.get());
						for (const auto &el : list->elements)
							if (!el.isNumber())
								throw TypeError("Vector elements must be numbers", line,
									col);
						stack.push_back(Value::Vector(list->elements));
					}
					DISPATCH();
				}
				OP(OP_BUILD_FSTRING) : {
					{
						uint8_t count = *ip++;
						int startPos = stack.size() - count;
						std::string finalStr = "";
						for (int i = 0; i < count; i++) {
							Value v = stack[startPos + i];
							finalStr += v.type == ValueType::STRING ? v.asString()
																				 : valueToString(v);
						}
						for (int i = 0; i < count; i++)
							stack.pop_back();
						stack.push_back(Value::String(finalStr));
					}
					DISPATCH();
				}
				OP(OP_GET_LOCAL) : {
					{
						uint8_t slot = *ip++;
						const Value &val = stack[frame->basePointer + slot];
						if (isCTLE && val.type == ValueType::NOTYPE) {
							throw CTLEAbort();
						}
						if (val.type == ValueType::REFERENCE)
							stack.push_back(*(val.get_ptr_safe()));
						else
							stack.push_back(val);
					}
					DISPATCH();
				}
				OP(OP_SET_LOCAL) : {
					{
						uint8_t slot = *ip++;
						Value &slotVal = stack[frame->basePointer + slot];
						Value newVal = stack.back();
						if (newVal.type == ValueType::REFERENCE) {
							bool wasConst = slotVal.isConst;
							bool wasLocked = slotVal.isLocked;
							slotVal = newVal;
							slotVal.isConst = wasConst;
							slotVal.isLocked = wasLocked;
						} else if (slotVal.type == ValueType::REFERENCE) {
							if (slotVal.isConst)
								throw OwnershipError("Cannot assign to const reference",
									line, col);
							Value *target = slotVal.get_ptr_safe();
							if (!target)
								throw RuntimeError("Null pointer dereference.", line, col);
							if (target->isConst)
								throw ConstError(
									"Cannot assign to const variable via reference",
									line, col);
							if (target->isLocked && target->type != newVal.type) {
								throw TypeError(
									"Cannot change type of locked variable via "
									"reference",
									line, col);
							}
							bool wasConst = target->isConst;
							bool wasLocked = target->isLocked;
							*target = newVal;
							target->isConst = wasConst;
							target->isLocked = wasLocked;
						} else {
							if (slotVal.isConst)
								throw ConstError("Cannot assign to const variable",
									line, col);
							if (slotVal.isLocked && slotVal.type != newVal.type) {
								throw ConstError(
									"Cannot change type of locked variable", line, col);
							}
							bool wasConst = slotVal.isConst;
							bool wasLocked = slotVal.isLocked;
							slotVal = newVal;
							slotVal.isConst = wasConst;
							slotVal.isLocked = wasLocked;
						}
					}
					DISPATCH();
				}
				OP(OP_REF_LOCAL) : {
					{
						uint8_t slot = *ip++;
						Value *ptr = &stack[frame->basePointer + slot];
						if (!invokeUnaryDunder(*ptr, "__ref__", line, col)) {
							std::shared_ptr<HeapObject> owner = nullptr;
							if (ptr->type == ValueType::REFERENCE) {
								owner = ptr->ref;
								ptr = ptr->ptr;
							}
							stack.push_back(Value::Reference(ptr, owner));
						}
					}
					DISPATCH();
				}
				OP(OP_REF_VAR) : {
					{
						uint8_t nameIndex = *ip++;
						string name = currentChunk->constants[nameIndex].asString();
						if (!globals->exists(name))
							throw NameError("Undefined variable '" + name + "'", line, col);
						Var &v = globals->lookup(name);
						Value *ptr = v.alias ? v.alias : &v.value;
						if (!invokeUnaryDunder(*ptr, "__ref__", line, col)) {
							stack.push_back(Value::FromExisting(std::make_shared<VarRefObject>(globals, name), ValueType::REFERENCE));
						}
					}
					DISPATCH();
				}
				OP(OP_REF_INDEX) : {
					{
						Value index = pop();
						Value base = pop();
						Value *ptr = nullptr;
						std::shared_ptr<HeapObject> owner = base.ref;
						if (base.type == ValueType::LIST) {
							auto *list = static_cast<ListObject *>(base.ref.get());
							long long idx = index.asInt();
							if (idx < 0)
								idx += (long long)list->elements.size();
							if (idx < 0 || idx >= (long long)list->elements.size())
								throw IndexError("List index out of range", line, col);
							stack.push_back(Value::FromExisting(std::make_shared<ListRefObject>(std::static_pointer_cast<ListObject>(base.ref), idx), ValueType::REFERENCE));
						} else if (base.type == ValueType::VECTOR) {
							auto *vec = static_cast<VectorObject *>(base.ref.get());
							long long idx = index.asInt();
							if (idx < 0)
								idx += (long long)vec->elements.size();
							if (idx < 0 || idx >= (long long)vec->elements.size())
								throw IndexError("Vector index out of range", line, col);
							ptr = &vec->elements[idx];
							stack.push_back(Value::Reference(ptr, owner));
						} else {
							throw OwnershipError("Cannot take reference of this type", line, col);
						}
					}
					DISPATCH();
				}
				OP(OP_INC_LOCAL) : {
					{
						uint8_t slot = *ip++;
						int idx = frame->basePointer + slot;
						if (isCTLE && stack[idx].type == ValueType::NOTYPE) {
							throw CTLEAbort();
						}
						Value *target = &stack[idx];
						if (target->type == ValueType::REFERENCE) {
							if (target->isConst)
								throw OwnershipError("Cannot assign to const reference", line, col);
							target = target->get_ptr_safe();
						}
						if (target->isConst)
							throw ConstError("Cannot assign to const variable", line, col);
						if (target->type == ValueType::INT)
							target->iVal++;
						else if (target->type == ValueType::FLOAT)
							target->fVal++;
						else
							target->fVal++;
					}
					DISPATCH();
				}
				OP(OP_JUMP_IF_NOT_LT) : {
					{
						uint8_t slot = *ip++;
						uint8_t constIdx = *ip++;
						uint16_t offset = (ip[0] << 8) | ip[1];
						ip += 2;
						int idx = frame->basePointer + slot;
						if (isCTLE && stack[idx].type == ValueType::NOTYPE) {
							throw CTLEAbort();
						}
						if (stack[idx].type == ValueType::INT) {
							long long localVal = stack[idx].iVal;
							long long constVal =
								currentChunk->constants[constIdx].iVal;
							if (localVal >= constVal)
								ip += offset;
						} else
							throw TypeError("Optimized loop requires integer", line,
								col);
					}
					DISPATCH();
				}
				OP(OP_SWITCH_TABLE) : {
					{
						Value val = stack.back();
						uint8_t minIdx = *ip++;
						uint8_t count = *ip++;
						if (val.type == ValueType::INT) {
							long long minVal = currentChunk->constants[minIdx].iVal;
							long long jumpIdx = val.iVal - minVal;
							if (jumpIdx >= 0 && jumpIdx < count) {
								uint8_t *tableEntry = ip + (jumpIdx * 2);
								uint16_t offset = (tableEntry[0] << 8) | tableEntry[1];
								ip = tableEntry + 2 + offset;
							} else
								ip += (count * 2);
						} else
							ip += (count * 2);
					}
					DISPATCH();
				}
				OP(OP_CONSTANT) : {
					{
						uint8_t index = *ip++;
						stack.push_back(currentChunk->constants[index]);
					}
					DISPATCH();
				}
				OP(OP_CONSTANT_LONG) : {
					{
						uint8_t hi = *ip++;
						uint8_t lo = *ip++;
						uint16_t index = (hi << 8) | lo;
						stack.push_back(currentChunk->constants[index]);
					}
					DISPATCH();
				}
				OP(OP_TRUE) : {
					{
						stack.push_back(Value::Bool(true));
					}
					DISPATCH();
				}
				OP(OP_FALSE) : {
					{
						stack.push_back(Value::Bool(false));
					}
					DISPATCH();
				}
				OP(OP_NONE) : {
					{
						stack.push_back(Value::None());
					}
					DISPATCH();
				}
				OP(OP_NOTYPE) : {
					{
						stack.push_back(Value::NoType());
					}
					DISPATCH();
				}
				OP(OP_POP) : {
					{
						if (!stack.empty())
							stack.pop_back();
					}
					DISPATCH();
				}
				OP(OP_IADD) : {
					bool goToAdd = false;
					{
						Value b = pop();
						Value a = pop();
						// `s += t`: append in place when only the target and this copy own the string (O(1) amortized instead of O(n))
						if (a.type == ValueType::STRING && b.type == ValueType::STRING && !a.isConst && a.ref.use_count() == 2) {
							static_cast<StringObject *>(a.ref.get())->value += b.asString();
							stack.push_back(a);
						} else if (!invokeBinaryDunder(a, b, "__plus_eq__", "", line, col)) {
							stack.push_back(a);
							stack.push_back(b);
							goToAdd = true;
						}
					}
					if (goToAdd) {
						goto execute_op_add;
					}
					DISPATCH();
				}
				OP(OP_FAST_IADD) : {
					{
						Value b = pop();
						Value a = pop();
						long long res;
						if (__builtin_add_overflow(a.iVal, b.iVal, &res)) {
							stack.push_back(BigIntObject::add(Value::BigInt(a.iVal), Value::BigInt(b.iVal)));
						} else {
							stack.push_back(Value::Int(res));
						}
					}
					DISPATCH();
				}
				OP(OP_FAST_ISUB) : {
					{
						Value b = pop();
						Value a = pop();
						long long res;
						if (__builtin_sub_overflow(a.iVal, b.iVal, &res)) {
							stack.push_back(BigIntObject::sub(Value::BigInt(a.iVal), Value::BigInt(b.iVal)));
						} else {
							stack.push_back(Value::Int(res));
						}
					}
					DISPATCH();
				}
				OP(OP_FAST_IMUL) : {
					{
						Value b = pop();
						Value a = pop();
						long long res;
						if (__builtin_mul_overflow(a.iVal, b.iVal, &res)) {
							stack.push_back(BigIntObject::mul(Value::BigInt(a.iVal), Value::BigInt(b.iVal)));
						} else {
							stack.push_back(Value::Int(res));
						}
					}
					DISPATCH();
				}
				OP(OP_FAST_IDIV) : {
					{
						Value b = pop();
						Value a = pop();
						if (b.iVal == 0) {
							throw ValueError("Division by zero", line, col);
						}
						stack.push_back(Value::Int(a.iVal / b.iVal));
					}
					DISPATCH();
				}
				OP(OP_FAST_FADD) : {
					{
						Value b = pop();
						Value a = pop();
						stack.push_back(Value::Float(a.fVal + b.fVal));
					}
					DISPATCH();
				}
				OP(OP_FAST_FSUB) : {
					{
						Value b = pop();
						Value a = pop();
						stack.push_back(Value::Float(a.fVal - b.fVal));
					}
					DISPATCH();
				}
				OP(OP_FAST_FMUL) : {
					{
						Value b = pop();
						Value a = pop();
						stack.push_back(Value::Float(a.fVal * b.fVal));
					}
					DISPATCH();
				}
				OP(OP_FAST_FDIV) : {
					{
						Value b = pop();
						Value a = pop();
						if (b.fVal == 0.0) {
							throw ValueError("Division by zero", line, col);
						}
						stack.push_back(Value::Float(a.fVal / b.fVal));
					}
					DISPATCH();
				}
				OP(OP_FAST_SADD) : {
					{
						Value b = pop();
						Value a = pop();
						stack.push_back(Value::String(a.asString() + b.asString()));
					}
					DISPATCH();
				}
				OP(OP_CTLE_TICK) : {
					{
						ctleTicks++;
						if (ctleTicks > 50000000) {
							throw CTLEAbort();
						}
					}
					DISPATCH();
				}
				OP(OP_ADD) : {
				execute_op_add:
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__plus__", "__r_plus__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::PLUS, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
					}
					DISPATCH();
				}
				OP(OP_ISUB) : {
					bool goToSub = false;
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__minus_eq__", "", line, col)) {
							stack.push_back(a);
							stack.push_back(b);
							goToSub = true;
						}
					}
					if (goToSub) {
						goto execute_op_sub;
					}
					DISPATCH();
				}
				OP(OP_SUB) : {
				execute_op_sub:
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__minus__", "__r_minus__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::MINUS, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
					}
					DISPATCH();
				}
				OP(OP_IDIV) : {
					bool goToDiv = false;
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__divide_eq__", "", line, col)) {
							stack.push_back(a);
							stack.push_back(b);
							goToDiv = true;
						}
					}
					if (goToDiv) {
						goto execute_op_div;
					}
					DISPATCH();
				}
				OP(OP_DIV) : {
				execute_op_div:
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__divide__", "__r_divide__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::SLASH, a, b));
							} catch (const std::runtime_error &e) {
								std::string msg = e.what();
								if (msg.find("zero") != std::string::npos) {
									throw DivisionByZeroError(msg, line, col);
								} else if (msg.find("Vector") != std::string::npos || msg.find("divide by") != std::string::npos) {
									throw TypeError(msg, line, col);
								} else {
									throw ValueError(msg, line, col);
								}
							}
						}
					}
					DISPATCH();
				}
				OP(OP_IMUL) : {
					bool goToMul = false;
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__times_eq__", "", line, col)) {
							stack.push_back(a);
							stack.push_back(b);
							goToMul = true;
						}
					}
					if (goToMul) {
						goto execute_op_mul;
					}
					DISPATCH();
				}
				OP(OP_MUL) : {
				execute_op_mul:
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__times__", "__r_times__", line, col)) {
							if (a.type == ValueType::LIST && b.type == ValueType::INT) {
								auto *listObj = static_cast<ListObject *>(a.ref.get());
								long long count = b.asInt();
								vector<Value> res;
								if (count > 0) {
									if (listObj->elements.size() * count > 1000000)
										throw MemoryError("List repetition too large", line, col);
									res.reserve(listObj->elements.size() * count);
									for (int i = 0; i < count; i++) {
										for (const auto &elem : listObj->elements)
											res.push_back(deepCopy(elem));
									}
								}
								stack.push_back(Value::List(res));
							} else if ((a.type == ValueType::FLOAT && b.type == ValueType::BIGINT) ||
										  (a.type == ValueType::BIGINT && b.type == ValueType::FLOAT)) {
								Value fVal = (a.type == ValueType::FLOAT) ? a : b;
								Value intVal = (a.type == ValueType::FLOAT) ? b : a;
								auto *bigObj = static_cast<BigIntObject *>(intVal.ref.get());
								std::vector<uint32_t> tempChunks = bigObj->chunks;
								double floatBase = fVal.asFloat();
								double result = 0.0;
								double powerOf10 = 1.0;
								bool overflow = false;
								if (tempChunks.size() > 40) {
									overflow = true;
								} else {
									while (!tempChunks.empty() && !(tempChunks.size() == 1 && tempChunks[0] == 0)) {
										uint32_t rem = divMod1e9(tempChunks);
										result += (floatBase * rem * powerOf10);
										powerOf10 *= 1e9;
										if (std::isinf(result) || std::isinf(powerOf10)) {
											overflow = true;
											break;
										}
									}
								}
								if (!overflow) {
									if (bigObj->isNegative)
										result = -result;
									stack.push_back(Value::Float(result));
								} else {
									std::ostringstream ss;
									ss << std::fixed << std::setprecision(6) << std::abs(floatBase);
									std::string s = ss.str();
									size_t decimalPos = s.find('.');
									long long power = 0;
									if (decimalPos != std::string::npos) {
										power = s.length() - decimalPos - 1;
										s.erase(decimalPos, 1);
									}
									BigIntObject mantissa(0);
									BigIntObject ten(10);
									for (char c : s) {
										BigIntObject digit(c - '0');
										mantissa = (mantissa * ten) + digit;
									}
									BigIntObject product = (*bigObj) * mantissa;
									if (power > 0) {
										BigIntObject divisor(1);
										for (int i = 0; i < power; i++)
											divisor = divisor * ten;
										product = product / divisor;
									}
									bool resultNeg = (bigObj->isNegative) != (floatBase < 0);
									product.isNegative = resultNeg;
									stack.push_back(Value::BigInt(std::make_shared<BigIntObject>(product)));
								}
							} else {
								try {
									stack.push_back(EvaluateConstBinary(TokenType::STAR, a, b));
								} catch (const std::runtime_error &e) {
									std::string msg = e.what();
									if (msg.find("Memory") != std::string::npos || msg.find("large") != std::string::npos) {
										throw MemoryError(msg, line, col);
									} else {
										throw ValueError(msg, line, col);
									}
								}
							}
						}
					}
					DISPATCH();
				}
				OP(OP_IFLOOR_DIV) : {
					bool goToFloorDiv = false;
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__int_divide_eq__", "", line, col)) {
							stack.push_back(a);
							stack.push_back(b);
							goToFloorDiv = true;
						}
					}
					if (goToFloorDiv) {
						goto execute_op_int_div;
					}
					DISPATCH();
				}
				OP(OP_FLOOR_DIV) : {
				execute_op_int_div:
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__int_divide__", "__r_int_divide__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::FLOOR_DIV, a, b));
							} catch (const std::runtime_error &e) {
								std::string msg = e.what();
								if (msg.find("zero") != std::string::npos) {
									throw DivisionByZeroError(msg, line, col);
								} else if (msg.find("Vector") != std::string::npos || msg.find("floor-divide by") != std::string::npos) {
									throw TypeError(msg, line, col);
								} else {
									throw ValueError(msg, line, col);
								}
							}
						}
					}
					DISPATCH();
				}
				OP(OP_IMOD) : {
					bool goToMod = false;
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__modulo_eq__", "", line, col)) {
							stack.push_back(a);
							stack.push_back(b);
							goToMod = true;
						}
					}
					if (goToMod) {
						goto execute_op_mod;
					}
					DISPATCH();
				}
				OP(OP_MOD) : {
				execute_op_mod:
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__modulo__", "__r_modulo__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::MOD, a, b));
							} catch (const std::runtime_error &e) {
								std::string msg = e.what();
								if (msg.find("zero") != std::string::npos) {
									throw DivisionByZeroError(msg, line, col);
								} else {
									throw ValueError(msg, line, col);
								}
							}
						}
					}
					DISPATCH();
				}
				OP(OP_IPOW) : {
					bool goToPow = false;
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__power_eq__", "", line, col)) {
							stack.push_back(a);
							stack.push_back(b);
							goToPow = true;
						}
					}
					if (goToPow) {
						goto execute_op_pow;
					}
					DISPATCH();
				}
				OP(OP_POW) : {
				execute_op_pow:
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__power__", "__r_power__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::POW, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
					}
					DISPATCH();
				}
				OP(OP_BITWISE_AND) : {
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__bit_and__", "__r_bit_and__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::BITWISE_AND, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
					}
					DISPATCH();
				}
				OP(OP_BITWISE_OR) : {
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__bit_or__", "__r_bit_or__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::BITWISE_OR, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
					}
					DISPATCH();
				}
				OP(OP_BITWISE_XOR) : {
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__bit_xor__", "__r_bit_xor__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::BITWISE_XOR, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
					}
					DISPATCH();
				}
				OP(OP_BITWISE_SHL) : {
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__bit_shl__", "__r_bit_shl__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::BITWISE_SHL, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
					}
					DISPATCH();
				}
				OP(OP_BITWISE_SHR) : {
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__bit_shr__", "__r_bit_shr__", line, col)) {
							try {
								stack.push_back(EvaluateConstBinary(TokenType::BITWISE_SHR, a, b));
							} catch (const std::runtime_error &e) {
								throw ValueError(e.what(), line, col);
							}
						}
					}
					DISPATCH();
				}
				// --- Comparisons ---
				OP(OP_GT) : {
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__greater__", "__less__", line, col)) {
							bool result = lessValue(b, a, this->globals, this->methodResolver, this->importResolver);
							stack.push_back(Value::Bool(result));
						}
					}
					DISPATCH();
				}
				OP(OP_GTE) : {
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__greater_eq__", "__less_eq__", line, col)) {
							bool result = !lessValue(a, b, this->globals, this->methodResolver, this->importResolver);
							stack.push_back(Value::Bool(result));
						}
					}
					DISPATCH();
				}
				OP(OP_LT) : {
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__less__", "__greater__", line, col)) {
							bool result = lessValue(a, b, this->globals, this->methodResolver, this->importResolver);
							stack.push_back(Value::Bool(result));
						}
					}
					DISPATCH();
				}
				OP(OP_LTE) : {
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__less_eq__", "__greater_eq__", line, col)) {
							bool result = !lessValue(b, a, this->globals, this->methodResolver, this->importResolver);
							stack.push_back(Value::Bool(result));
						}
					}
					DISPATCH();
				}
				OP(OP_EQ) : {
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__equals__", "__equals__", line, col)) {
							stack.push_back(Value::Bool(a.looseEquals(b)));
						}
					}
					DISPATCH();
				}
				OP(OP_NEQ) : {
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__differs__", "__differs__", line, col)) {
							stack.push_back(Value::Bool(!(a.looseEquals(b))));
						}
					}
					DISPATCH();
				}
				OP(OP_STRICT_EQ) : {
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__identical__", "__identical__", line, col)) {
							stack.push_back(Value::Bool(a.strictEquals(b)));
						}
					}
					DISPATCH();
				}
				OP(OP_STRICT_NEQ) : {
					{
						Value b = pop();
						Value a = pop();
						if (!invokeBinaryDunder(a, b, "__distinct__", "__distinct__", line, col)) {
							stack.push_back(Value::Bool(!(a.strictEquals(b))));
						}
					}
					DISPATCH();
				}
				OP(OP_IS) : {
					{
						Value b = pop();
						Value a = pop();
						const Value &valA = (a.type == ValueType::REFERENCE && a.get_ptr_safe()) ? *(a.get_ptr_safe()) : a;
						const Value &valB = (b.type == ValueType::REFERENCE && b.get_ptr_safe()) ? *(b.get_ptr_safe()) : b;
						bool same = false;
						if (valA.type == valB.type) {
							if (valA.ref != nullptr || valB.ref != nullptr) {
								same = (valA.ref.get() == valB.ref.get());
							} else {
								same = valA.strictEquals(valB);
							}
						}
						stack.push_back(Value::Bool(same));
					}
					DISPATCH();
				}
				OP(OP_IS_NOT) : {
					{
						Value b = pop();
						Value a = pop();
						const Value &valA = (a.type == ValueType::REFERENCE && a.get_ptr_safe()) ? *(a.get_ptr_safe()) : a;
						const Value &valB = (b.type == ValueType::REFERENCE && b.get_ptr_safe()) ? *(b.get_ptr_safe()) : b;
						bool same = false;
						if (valA.type == valB.type) {
							if (valA.ref != nullptr || valB.ref != nullptr) {
								same = (valA.ref.get() == valB.ref.get());
							} else {
								same = valA.strictEquals(valB);
							}
						}
						stack.push_back(Value::Bool(!same));
					}
					DISPATCH();
				}
				OP(OP_XOR) : {
					{
						Value b = pop();
						Value a = pop();
						stack.push_back(Value::Bool(a.isTruthy() != b.isTruthy()));
					}
					DISPATCH();
				}
				OP(OP_NXOR) : {
					{
						Value b = pop();
						Value a = pop();
						stack.push_back(Value::Bool(a.isTruthy() == b.isTruthy()));
					}
					DISPATCH();
				}
				OP(OP_NAND) : {
					{
						Value b = pop();
						Value a = pop();
						stack.push_back(Value::Bool(!(a.isTruthy() && b.isTruthy())));
					}
					DISPATCH();
				}
				OP(OP_NOR) : {
					{
						Value b = pop();
						Value a = pop();
						stack.push_back(Value::Bool(!(a.isTruthy() || b.isTruthy())));
					}
					DISPATCH();
				}
				OP(OP_NOT) : {
					{
						Value v = pop();
						stack.push_back(Value::Bool(!v.isTruthy()));
					}
					DISPATCH();
				}
				OP(OP_BITWISE_NOT) : {
					{
						Value v = pop();
						if (v.type == ValueType::INT) {
							stack.push_back(Value::Int(~v.iVal));
						} else if (v.type == ValueType::BIGINT) {
							throw std::runtime_error("Bitwise NOT on BigInt is not supported");
						} else {
							throw TypeError("Bitwise NOT requires an integer", line, col);
						}
					}
					DISPATCH();
				}
				OP(OP_NEGATE) : {
					{
						Value v = pop();
						if (v.type == ValueType::REFERENCE && v.get_ptr_safe())
							v = *v.get_ptr_safe();
						if (v.type == ValueType::INT) {
							if (v.iVal == LLONG_MIN)
								stack.push_back(BigIntObject::mul(Value::BigInt(v.iVal),
									Value::BigInt(-1)));
							else
								stack.push_back(Value::Int(-v.iVal));
						} else if (v.type == ValueType::BIGINT) {
							stack.push_back(BigIntObject::mul(v, Value::BigInt(-1)));
						} else if (v.type == ValueType::VECTOR) {
							auto *vec = static_cast<VectorObject *>(v.ref.get());
							vector<Value> res;
							res.reserve(vec->elements.size());
							for (const auto &el : vec->elements) {
								if (el.type == ValueType::INT) {
									if (el.iVal == LLONG_MIN)
										res.push_back(BigIntObject::mul(
											Value::BigInt(el.iVal), Value::BigInt(-1)));
									else
										res.push_back(Value::Int(-el.iVal));
								} else if (el.type == ValueType::BIGINT) {
									res.push_back(
										BigIntObject::mul(el, Value::BigInt(-1)));
								} else {
									res.push_back(Value::Float(-el.asFloat()));
								}
							}
							stack.push_back(Value::Vector(res));
						} else
							stack.push_back(Value::Float(-v.asFloat()));
					}
					DISPATCH();
				}
				OP(OP_IS_IN) : {
					{
						Value rhs = pop(); // The container
						Value lhs = pop(); // The item
						if (!invokeBinaryDunder(rhs, lhs, "__has__", "__missing__", line, col)) {
							bool found = false;
							if (rhs.type == ValueType::STRING) {
								if (lhs.type == ValueType::STRING) {
									found = rhs.asString().find(lhs.asString()) != string::npos;
								}
							} else if (rhs.type == ValueType::LIST) {
								auto *list = static_cast<ListObject *>(rhs.ref.get());
								for (const auto &item : list->elements) {
									if (item.strictEquals(lhs)) {
										found = true;
										break;
									}
								}
							} else if (rhs.type == ValueType::DICT) {
								auto *d = static_cast<DictObject *>(rhs.ref.get());
								found = d->items.count(lhs) > 0;
							} else if (rhs.type == ValueType::SET) {
								auto *s = static_cast<SetObject *>(rhs.ref.get());
								found = s->elements.count(lhs) > 0;
							} else if (rhs.type == ValueType::RANGE) {
								auto *rng = static_cast<RangeObject *>(rhs.ref.get());
								if (lhs.isNumber()) {
									double val = lhs.asFloat();
									bool inBounds =
										(rng->step > 0)
											? (val >= rng->start &&
												  (rng->endInclusive ? val <= rng->end : val < rng->end))
											: (val <= rng->start &&
												  (rng->endInclusive ? val >= rng->end : val > rng->end));
									if (inBounds && !rng->isFloat && lhs.type == ValueType::INT) {
										found = ((long long)(val - rng->start) % (long long)rng->step == 0);
									} else {
										found = inBounds;
									}
								}
							}
							stack.push_back(Value::Bool(found));
						}
					}
					DISPATCH();
				}
				OP(OP_IS_NOT_IN) : {
					{
						Value rhs = pop(); // The container
						Value lhs = pop(); // The item
						if (!invokeBinaryDunder(rhs, lhs, "__lacks__", "__not_missing__", line, col)) {
							bool found = false;
							if (rhs.type == ValueType::STRING) {
								if (lhs.type == ValueType::STRING) {
									found = rhs.asString().find(lhs.asString()) != string::npos;
								}
							} else if (rhs.type == ValueType::LIST) {
								auto *list = static_cast<ListObject *>(rhs.ref.get());
								for (const auto &item : list->elements) {
									if (item.strictEquals(lhs)) {
										found = true;
										break;
									}
								}
							} else if (rhs.type == ValueType::DICT) {
								auto *d = static_cast<DictObject *>(rhs.ref.get());
								found = d->items.count(lhs) > 0;
							} else if (rhs.type == ValueType::SET) {
								auto *s = static_cast<SetObject *>(rhs.ref.get());
								found = s->elements.count(lhs) > 0;
							} else if (rhs.type == ValueType::RANGE) {
								auto *rng = static_cast<RangeObject *>(rhs.ref.get());
								if (lhs.isNumber()) {
									double val = lhs.asFloat();
									bool inBounds =
										(rng->step > 0)
											? (val >= rng->start &&
												  (rng->endInclusive ? val <= rng->end : val < rng->end))
											: (val <= rng->start &&
												  (rng->endInclusive ? val >= rng->end : val > rng->end));
									if (inBounds && !rng->isFloat && lhs.type == ValueType::INT) {
										found = ((long long)(val - rng->start) % (long long)rng->step == 0);
									} else {
										found = inBounds;
									}
								}
							}
							stack.push_back(Value::Bool(!found));
						}
					}
					DISPATCH();
				}
				OP(OP_LOOP) : {
					{
						uint8_t hi = *ip++;
						uint8_t lo = *ip++;
						uint16_t offset = (hi << 8) | lo;
						ip -= offset;
					}
					DISPATCH();
				}
				OP(OP_DEFINE_VAR) : {
					{
						uint8_t nameIndex = *ip++;
						uint8_t flags = *ip++;
						string name = currentChunk->constants[nameIndex].asString();
						Value val = pop();
#ifdef VM_DEBUG_MODE
						if (DEBUGGER_MODE_IS_ENABLED)
							val.__DEBUGGING__NAME__ = name;
#endif
						bool isConst = (flags & 0x01) != 0;
						bool isLocked = (flags & 0x02) != 0;
						if (globals->exists(name)) {
							Value existing = globals->get(name);
							bool identical = false;
							if (existing.ref != nullptr && val.ref != nullptr) {
								identical = (existing.ref.get() == val.ref.get());
							} else if (existing.type == val.type) {
								if (existing.type == ValueType::FLOAT)
									identical = (existing.fVal == val.fVal);
								else if (existing.type == ValueType::INT)
									identical = (existing.iVal == val.iVal);
								else if (existing.type == ValueType::BOOL)
									identical = (existing.bVal == val.bVal);
								else if (existing.type == ValueType::NONE)
									identical = true;
							}
							if (identical) {
								goto skip_define;
							}
						}
						if (val.type == ValueType::FUNCTION || val.type == ValueType::NATIVE_FUNCTION) {
							if (globals->exists(name)) {
								Value existing = globals->get(name);
								if (existing.type == ValueType::FUNCTION || existing.type == ValueType::NATIVE_FUNCTION) {
									auto ovObj = std::make_shared<OverloadObject>(existing);
									ovObj->overloads.push_back(val);
									Value ovVal;
									ovVal.type = ValueType::OVERLOAD;
									ovVal.ref = ovObj;
									globals->set(name, ovVal, isLocked, isConst);
								} else if (existing.type == ValueType::OVERLOAD) {
									auto *ov = static_cast<OverloadObject *>(existing.ref.get());
									ov->overloads.push_back(val);
								} else {
									globals->set(name, val, isLocked, isConst);
								}
							} else {
								globals->set(name, val, isLocked, isConst);
							}
						} else {
							globals->set(name, val, isLocked, isConst);
						}
					}
				skip_define:;
					DISPATCH();
				}
				OP(OP_GET_VAR) : {
					{
						uint8_t nameIndex = *ip++;
						string name = currentChunk->constants[nameIndex].asString();
						if (!globals->exists(name))
							throw NameError("Undefined variable '" + name + "'", line,
								col);
						Var &v = globals->lookup(name);
						if (v.alias)
							stack.push_back(*v.alias);
						else
							stack.push_back(v.value);
					}
					DISPATCH();
				}
				OP(OP_JUMP_IF_FALSE) : {
					{
						uint8_t hi = *ip++;
						uint8_t lo = *ip++;
						uint16_t offset = (hi << 8) | lo;
						Value val = stack.back();
						bool truthy = val.isTruthy();
						if (val.type == ValueType::INSTANCE) {
							auto dunder = tryCastDunder(val, "__to_bool__", line, col, globals, methodResolver, importResolver);
							if (dunder.first) {
								truthy = dunder.second.isTruthy();
							}
						}
						if (!truthy) {
							ip += offset;
						}
					}
					DISPATCH();
				}
				OP(OP_JUMP) : {
					{
						uint8_t hi = *ip++;
						uint8_t lo = *ip++;
						uint16_t offset = (hi << 8) | lo;
						ip += offset;
					}
					DISPATCH();
				}
				OP(OP_UNPACK_DICT) : {
					{
						Value v = pop();
						if (v.type != ValueType::DICT) {
							throw TypeError("Cannot unpack non-dictionary", line, col);
						}
						auto dict = static_cast<DictObject *>(v.ref.get());
						auto keys = std::make_shared<ListObject>();
						auto vals = std::make_shared<ListObject>();
						for (const auto &pair : dict->items) {
							keys->elements.push_back(pair.first);
							vals->elements.push_back(pair.second);
						}
						stack.push_back(Value::List(keys->elements));
						stack.push_back(Value::List(vals->elements));
					}
					DISPATCH();
				}
				OP(OP_SET_VAR) : {
					{
						uint8_t nameIndex = *ip++;
						string name = currentChunk->constants[nameIndex].asString();
						Value val = stack.back();
#ifdef VM_DEBUG_MODE
						if (DEBUGGER_MODE_IS_ENABLED)
							val.__DEBUGGING__NAME__ = name;
#endif
						if (!globals->exists(name))
							throw NameError("Undefined variable '" + name + "'", line,
								col);
						Var &v = globals->lookup(name);
						if (val.type == ValueType::REFERENCE) {
							v.alias = val.ptr;
							v.value.ref = val.ref;
						} else if (v.alias) {
							Value *target = v.alias;
							if (target->isConst)
								throw ConstError("Cannot assign to const variable '" +
														  name + "' via reference",
									line, col);
							if (target->isLocked && target->type != val.type) {
								throw TypeError(
									"Cannot change type of locked variable '" + name +
										"' via reference",
									line, col);
							}
							bool wasConst = target->isConst;
							bool wasLocked = target->isLocked;
							*target = val;
							target->isConst = wasConst;
							target->isLocked = wasLocked;
						} else {
							if (v.isConst)
								throw ConstError(
									"Cannot assign to const variable '" + name + "'",
									line, col);
							if (v.isLocked && v.value.type != val.type)
								throw TypeError(
									"Cannot change type of locked variable '" + name +
										"'",
									line, col);
							v.value = val;
						}
					}
					DISPATCH();
				}
				OP(OP_DEFINE_REF) : {
					{
						uint8_t nameIdx = *ip++;
						string newVarName =
							currentChunk->constants[nameIdx].asString();
						OpCode subOp = static_cast<OpCode>(*ip++);
						if (subOp == OpCode::OP_REF_VAR) {
							string targetName =
								currentChunk->constants[*ip++].asString();
							if (!globals->exists(targetName))
								throw NameError("Undefined variable '" + targetName + "'", line, col);
							Var &targetVar = globals->lookup(targetName);
							Var aliasVar;
							aliasVar.alias =
								targetVar.alias ? targetVar.alias : &targetVar.value;
							globals->vars[newVarName] = aliasVar;
						} else if (subOp == OpCode::OP_REF_INDEX) {
							Value index = pop();
							Value base = pop();
							if (base.type == ValueType::LIST) {
								auto *list = static_cast<ListObject *>(base.ref.get());
								long long i = index.asInt();
								if (i < 0)
									i += (long long)list->elements.size();
								if (i < 0 || i >= (long long)list->elements.size())
									throw IndexError("List index out of range", line, col);
								// RefObject instead of raw pointer: survives list reallocation
								Var aliasVar;
								aliasVar.value = Value::FromExisting(std::make_shared<ListRefObject>(std::static_pointer_cast<ListObject>(base.ref), (int)i), ValueType::REFERENCE);
								globals->vars[newVarName] = aliasVar;
							}
						}
					}
					DISPATCH();
				}
				OP(OP_SET_REF) : {
					{
						uint8_t nameIdx = *ip++;
						string varName = currentChunk->constants[nameIdx].asString();
						if (!globals->exists(varName))
							throw NameError("Undefined variable '" + varName + "'", line, col);
						OpCode subOp = static_cast<OpCode>(*ip++);
						if (subOp == OpCode::OP_REF_VAR) {
							string target = currentChunk->constants[*ip++].asString();
							if (!globals->exists(target))
								throw NameError("Undefined variable '" + target + "'", line, col);
							Var &targetVar = globals->lookup(target);
							globals->vars[varName].alias =
								targetVar.alias ? targetVar.alias : &targetVar.value;
						}
					}
					DISPATCH();
				}
				OP(OP_COLON) : {
					{
						Value v = pop();
						Value k = pop();
						vector<std::pair<Value, Value>> p;
						p.push_back({k, v});
						stack.push_back(Value::Paired(p));
					}
					DISPATCH();
				}
				OP(OP_TO_STREAM) : {
					{
						Value v = pop();
						if (v.type == ValueType::INSTANCE) {
							auto dunder = tryCastDunder(v, "__traverse__", line, col, this->globals, this->methodResolver, this->importResolver);
							if (dunder.first) {
								stack.push_back(dunder.second);
							} else {
								stack.push_back(prepareIterable(v, line, col));
							}
						} else {
							stack.push_back(prepareIterable(v, line, col));
						}
					}
					DISPATCH();
				}
				OP(OP_FOR_ITER) : {
					{
						uint8_t *jumpOffsetAddr = ip;
						uint16_t offset = (jumpOffsetAddr[0] << 8) | jumpOffsetAddr[1];
						ip += 2;
						uint8_t count = *ip++;
						Value &indexVal = stack.back();
						long long stepCount = indexVal.asInt();
						bool valid = true;
						vector<Value> nextValues;
						for (int i = 0; i < count; i++) {
							int stackPos = stack.size() - 1 - count + i;
							Value &stream = stack[stackPos];
							std::shared_ptr<HeapObject> owner = stream.ref;
							if (stream.type == ValueType::LIST) {
								auto *list = static_cast<ListObject *>(stream.ref.get());
								if (stepCount >= (long long)list->elements.size()) {
									valid = false;
									break;
								}
								nextValues.push_back(Value::Reference(&list->elements[stepCount], owner));
							} else if (stream.type == ValueType::TUPLE) {
								auto *tuple = static_cast<TupleObject *>(stream.ref.get());
								if (stepCount >= (long long)tuple->elements.size()) {
									valid = false;
									break;
								}
								nextValues.push_back(tuple->elements[stepCount]);
							} else if (stream.type == ValueType::SET) {
								auto *s = static_cast<SetObject *>(stream.ref.get());
								std::vector<Value> snapshot(s->elements.begin(), s->elements.end());
								stream = Value::List(snapshot);
								owner = stream.ref;
								auto *list = static_cast<ListObject *>(stream.ref.get());
								if (stepCount >= (long long)list->elements.size()) {
									valid = false;
									break;
								}
								nextValues.push_back(Value::Reference(&list->elements[stepCount], owner));
							} else if (stream.type == ValueType::STRING) {
								string s = stream.asString();
								if (stepCount >= (long long)s.length()) {
									valid = false;
									break;
								}
								nextValues.push_back(Value::String(string(1, s[stepCount])));
							} else if (stream.type == ValueType::VECTOR) {
								auto *vec = static_cast<VectorObject *>(stream.ref.get());
								if (stepCount >= (long long)vec->elements.size()) {
									valid = false;
									break;
								}
								nextValues.push_back(Value::Reference(&vec->elements[stepCount], owner));
							} else if (stream.type == ValueType::RANGE) {
								auto *r = static_cast<RangeObject *>(stream.ref.get());
								double current = r->start + (r->step * stepCount);
								bool inBounds = (r->step > 0)
														 ? (r->endInclusive ? current <= r->end : current < r->end)
														 : (r->endInclusive ? current >= r->end : current > r->end);
								if (!inBounds) {
									valid = false;
									break;
								}
								if (r->isFloat)
									nextValues.push_back(Value::Float(current));
								else
									nextValues.push_back(Value::Int((long long)current));
							} else if (stream.type == ValueType::INSTANCE) {
								auto *inst = static_cast<InstanceObject *>(stream.ref.get());
								ClassObject *cls = inst->klass;
								ClassObject::MethodInfo *advMethod = nullptr;
								ClassObject *methodOwner = nullptr;
								for (auto *ancestor : cls->mro) {
									if (ancestor->methods.count("__advance__")) {
										advMethod = &ancestor->methods["__advance__"];
										methodOwner = ancestor;
										break;
									}
								}
								if (advMethod) {
									Value classObjVal;
									classObjVal.type = ValueType::CLASS;
									classObjVal.ref = std::shared_ptr<HeapObject>(methodOwner, [](HeapObject *) {});
									VM tempVM;
									tempVM.globals = this->globals;
									tempVM.methodResolver = this->methodResolver;
									tempVM.importResolver = this->importResolver;
									Chunk tempChunk;
									int selfIdx = tempChunk.addConstant(stream);
									tempChunk.write(OpCode::OP_CONSTANT, line, col);
									tempChunk.write((uint8_t)selfIdx, line, col);
									int objIdx = tempChunk.addConstant(classObjVal);
									tempChunk.write(OpCode::OP_CONSTANT, line, col);
									tempChunk.write((uint8_t)objIdx, line, col);
									int stepIdx = tempChunk.addConstant(Value::Int(stepCount));
									tempChunk.write(OpCode::OP_CONSTANT, line, col);
									tempChunk.write((uint8_t)stepIdx, line, col);
									int methIdx = tempChunk.addConstant(advMethod->func);
									tempChunk.write(OpCode::OP_CONSTANT, line, col);
									tempChunk.write((uint8_t)methIdx, line, col);
									tempChunk.write(OpCode::OP_CALL, line, col);
									tempChunk.write((uint8_t)3, line, col);
									tempChunk.write(OpCode::OP_RETURN, line, col);
									try {
										tempVM.run(tempChunk);
										if (!tempVM.stack.empty()) {
											nextValues.push_back(tempVM.stack.back());
										} else {
											nextValues.push_back(Value::None());
										}
									} catch (const LangError &e) {
										if (e.type == "IteratorError") {
											valid = false;
										} else {
											throw;
										}
									}
								} else {
									throw TypeError("Iterator object missing '__advance__' method", line, col);
								}
							} else {
								throw TypeError("Unsupported stream type in iterator", line, col);
							}
						}
						if (valid) {
							for (const auto &val : nextValues)
								stack.push_back(val);
							stack[stack.size() - 1 - count].iVal++;
						} else {
							ip = jumpOffsetAddr + 2 + offset;
						}
					}
					DISPATCH();
				}
				OP(OP_SKIP_ITER) : {
					{
						uint8_t slot = *ip++;
						Value amount = pop();
						if (!amount.isNumber())
							throw TypeError("skip amount must be a number", line, col);
						long long skipN = amount.asInt();
						int absoluteSlot = frame->basePointer + slot;
						if (absoluteSlot >= stack.size())
							throw IndexError("Skip iterator slot out of bounds", line,
								col);
						stack[absoluteSlot].iVal += skipN;
					}
					DISPATCH();
				}
				OP(OP_BUILD_SLICE) : {
					{
						Value step = pop();
						Value end = pop();
						Value start = pop();
						auto slice = std::make_shared<SliceObject>(start, end, step);
						Value v;
						v.type = ValueType::SLICE;
						v.ref = slice;
						stack.push_back(v);
					}
					DISPATCH();
				}
				// CONTAINERS
				OP(OP_BUILD_LIST) : {
					{
						uint8_t count = *ip++;
						auto list = std::make_shared<ListObject>();
						if (stack.size() < count)
							throw EmptyContainerError(
								"Stack underflow during list build", line, col);
						list->elements.resize(count);
						for (int i = count - 1; i >= 0; i--) {
							list->elements[i] = pop();
						}
						stack.push_back(Value::FromExisting(list, ValueType::LIST));
					}
					DISPATCH();
				}
				OP(OP_BUILD_DICT) : {
					{
						uint8_t count = *ip++;
						auto dict = std::make_shared<DictObject>();
						for (int i = 0; i < count; i++) {
							Value pairVal = pop();
							if (pairVal.type == ValueType::PAIRED) {
								auto *pObj =
									static_cast<PairedObject *>(pairVal.ref.get());
								for (const auto &entry : pObj->pairs) {
									Value key = entry.first;
									if (key.type == ValueType::LIST ||
										 key.type == ValueType::SET) {
										key = deepCopy(key);
										key.isConst = true;
									}
									dict->items[key] = entry.second;
								}
							}
						}
						stack.push_back(Value::Dict(dict->items));
					}
					DISPATCH();
				}
				OP(OP_BUILD_SET) : {
					{
						uint8_t count = *ip++;
						auto set = std::make_shared<SetObject>();
						for (int i = 0; i < count; i++)
							setAdd(set->elements, pop());
						stack.push_back(Value::Set(set->elements));
					}
					DISPATCH();
				}
				OP(OP_BUILD_TUPLE) : {
					{
						uint8_t count = *ip++;
						vector<Value> elems(count);
						for (int i = count - 1; i >= 0; i--)
							elems[i] = pop();
						stack.push_back(Value::Tuple(elems));
					}
					DISPATCH();
				}
				OP(OP_BUILD_VECTOR) : {
					{
						uint8_t count = *ip++;
						vector<Value> elems;
						elems.resize(count);
						for (int i = count - 1; i >= 0; i--) {
							Value v = pop();
							if (!v.isNumber())
								throw TypeError("Vector elements must be numbers", line,
									col);
							elems[i] = v;
						}
						stack.push_back(Value::Vector(elems));
					}
					DISPATCH();
				}
				OP(OP_BUILD_RANGE) : {
					{
						uint8_t flags = *ip++;
						bool si = (flags & 0x01) != 0;
						bool ei = (flags & 0x02) != 0;
						Value step = pop();
						Value end = pop();
						Value start = pop();
						bool isFloat = (start.type == ValueType::FLOAT || end.type == ValueType::FLOAT || step.type == ValueType::FLOAT);
						stack.push_back(Value::Range(start.asFloat(), end.asFloat(),
							step.asFloat(), si, ei,
							isFloat));
					}
					DISPATCH();
				}
				OP(OP_SHALLOW_COPY) : {
					{
						Value v = pop();
						if (!invokeUnaryDunder(v, "__copy__", line, col)) {
							if (v.type == ValueType::INSTANCE ||
								 v.type == ValueType::CLASS ||
								 v.type == ValueType::REFERENCE) {
								stack.push_back(v);
							} else {
								Value res = shallowCopy(v);
								res.isConst = false;
								res.isLocked = false;
								stack.push_back(res);
							}
						}
					}
					DISPATCH();
				}
				OP(OP_DEEP_COPY) : {
					{
						Value v = pop();
						if (!invokeUnaryDunder(v, "__clone__", line, col)) {
							Value res = deepCopy(v);
							res.isConst = false;
							res.isLocked = false;
							stack.push_back(res);
						}
					}
					DISPATCH();
				}
				OP(OP_GET_INDEX) : {
					{
						Value index = pop();
						Value base = pop();
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
						while (base.type == ValueType::REFERENCE) {
							if (!base.get_ptr_safe())
								throw RuntimeError("Dead-end reference", line, col);
							base = *(base.get_ptr_safe());
						}
						if (!invokeBinaryDunder(base, index, "__at__", "", line, col)) {
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
									stack.push_back(Value::List(newList->elements));
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
									stack.push_back(list->elements[idx]);
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
									stack.push_back(Value::Vector(newElems));
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
									stack.push_back(vec->elements[idx]);
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
									stack.push_back(Value::Tuple(newElems));
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
									stack.push_back(tuple->elements[idx]);
								}
								break;
							}
							case ValueType::DICT: {
								auto *dict = static_cast<DictObject *>(base.ref.get());
								if (dict->items.count(index) == 0)
									throw KeyError(
										"Key not found: " + valueToString(index), line,
										0);
								stack.push_back(dict->items.at(index));
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
									stack.push_back(Value::String(newStr));
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
									stack.push_back(Value::String(string(1, s[idx])));
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
								stack.push_back(rng->isFloat
														 ? Value::Float(val)
														 : Value::Int((long long)val));
								break;
							}
							default:
								throw TypeError("Object is not subscriptable", line, col);
								break;
							}
						}
					}
					DISPATCH();
				}
				OP(OP_SET_INDEX) : {
					{
						Value val = pop();
						Value index = pop();
						Value base = pop();
						bool dunderFound = false;
						if (base.type == ValueType::INSTANCE) {
							auto *inst = static_cast<InstanceObject *>(base.ref.get());
							ClassObject *cls = inst->klass;
							ClassObject::MethodInfo *putMethod = nullptr;
							ClassObject *methodOwner = nullptr;
							for (auto *ancestor : cls->mro) {
								if (ancestor->methods.count("__put__")) {
									putMethod = &ancestor->methods["__put__"];
									methodOwner = ancestor;
									break;
								}
							}
							if (putMethod) {
								dunderFound = true;
								Value classObjVal;
								classObjVal.type = ValueType::CLASS;
								classObjVal.ref = std::shared_ptr<HeapObject>(methodOwner, [](HeapObject *) {});
								if (putMethod->func.type == ValueType::NATIVE_FUNCTION) {
									auto *nat = static_cast<NativeFunctionObject *>(putMethod->func.ref.get());
									std::vector<Value> args = {base, classObjVal, index, val};
									nat->func(args, line, col);
								} else {
									VM tempVM;
									tempVM.globals = this->globals;
									tempVM.methodResolver = this->methodResolver;
									tempVM.importResolver = this->importResolver;
									Chunk tempChunk;
									int selfIdx = tempChunk.addConstant(base);
									tempChunk.write(OpCode::OP_CONSTANT, line, col);
									tempChunk.write((uint8_t)selfIdx, line, col);
									int objIdx = tempChunk.addConstant(classObjVal);
									tempChunk.write(OpCode::OP_CONSTANT, line, col);
									tempChunk.write((uint8_t)objIdx, line, col);
									int keyIdx = tempChunk.addConstant(index);
									tempChunk.write(OpCode::OP_CONSTANT, line, col);
									tempChunk.write((uint8_t)keyIdx, line, col);
									int valIdx = tempChunk.addConstant(val);
									tempChunk.write(OpCode::OP_CONSTANT, line, col);
									tempChunk.write((uint8_t)valIdx, line, col);
									int methIdx = tempChunk.addConstant(putMethod->func);
									tempChunk.write(OpCode::OP_CONSTANT, line, col);
									tempChunk.write((uint8_t)methIdx, line, col);
									tempChunk.write(OpCode::OP_CALL, line, col);
									tempChunk.write((uint8_t)4, line, col);
									tempChunk.write(OpCode::OP_RETURN, line, col);
									try {
										tempVM.run(tempChunk);
									} catch (...) {
										throw;
									}
								}
								stack.push_back(val);
							}
						}
						if (!dunderFound) {
							if (base.type == ValueType::LIST) {
								auto *list = static_cast<ListObject *>(base.ref.get());
								if (!index.isNumber())
									throw TypeError("List index must be a number", line,
										col);
								long long idx = index.asInt();
								if (idx < 0)
									idx += list->elements.size();
								if (idx < 0 || idx >= (long long)list->elements.size())
									throw IndexError("List assignment index out of range",
										line, col);
								list->elements[idx] = val;
							} else if (base.type == ValueType::DICT) {
								auto *dict = static_cast<DictObject *>(base.ref.get());
								if (index.type == ValueType::LIST ||
									 index.type == ValueType::SET) {
									index = deepCopy(index);
									index.isConst = true;
								}
								dict->items[index] = val;
							} else if (base.type == ValueType::STRING) {
								auto *str = static_cast<StringObject *>(base.ref.get());
								if (!index.isNumber())
									throw TypeError("String index must be a number", line,
										col);
								long long idx = index.asInt();
								if (idx < 0)
									idx += str->value.size();
								if (idx < 0 || idx >= (long long)str->value.size())
									throw IndexError("String assignment index out of range",
										line, col);
								str->value[idx] = val.asString()[0];
							} else if (base.type == ValueType::VECTOR) {
								auto *vec = static_cast<VectorObject *>(base.ref.get());
								if (!index.isNumber())
									throw TypeError("Vector index must be a number", line,
										col);
								long long idx = index.asInt();
								if (idx < 0)
									idx += vec->elements.size();
								if (idx < 0 || idx >= (long long)vec->elements.size())
									throw IndexError("Vector assignment index out of range",
										line, col);
								vec->elements[idx] = val;
							} else if (base.type == ValueType::SET) {
								throw TypeError("Sets are unordered collections and do not support indexing or slicing.", line, col);
							} else if (base.type == ValueType::TUPLE)
								throw MutationError("Tuple object does not support item assignment", line, col);
							else
								throw MutationError("Object does not support item assignment", line, col);
							stack.push_back(val);
						}
					}
					DISPATCH();
				}
				OP(OP_INC_INDEX) : {
					{
						uint8_t isInc = *ip++;
						uint8_t isPrefix = *ip++;
						Value index = pop();
						Value obj = pop();

						if (obj.type == ValueType::REFERENCE) {
							if (obj.get_ptr_safe() == nullptr)
								throw RuntimeError("Null pointer dereference.", line, col);
							obj = *(obj.get_ptr_safe());
						}

						Value oldVal;
						if (obj.type == ValueType::DICT) {
							auto *dictObj = static_cast<DictObject *>(obj.ref.get());
							if (dictObj->items.find(index) != dictObj->items.end()) {
								oldVal = dictObj->items[index];
							} else {
								throw KeyError("Key not found in dict", line, col);
							}
						} else if (obj.type == ValueType::LIST) {
							auto *listObj = static_cast<ListObject *>(obj.ref.get());
							if (index.type != ValueType::INT)
								throw TypeError("List indices must be integers", line, col);
							long long i = index.iVal;
							if (i < 0)
								i += listObj->elements.size();
							if (i < 0 || i >= listObj->elements.size())
								throw IndexError("List index out of bounds", line, col);
							oldVal = listObj->elements[i];
						} else {
							throw TypeError("Cannot increment index on this object type", line, col);
						}

						Value newVal;
						if (oldVal.type == ValueType::INT) {
							newVal = Value::Int(oldVal.iVal + (isInc ? 1 : -1));
						} else if (oldVal.type == ValueType::FLOAT) {
							newVal = Value::Float(oldVal.fVal + (isInc ? 1.0 : -1.0));
						} else {
							throw TypeError("Increment/Decrement requires numeric property", line, col);
						}

						if (obj.type == ValueType::DICT) {
							auto *dictObj = static_cast<DictObject *>(obj.ref.get());
							dictObj->items[index] = newVal;
						} else if (obj.type == ValueType::LIST) {
							auto *listObj = static_cast<ListObject *>(obj.ref.get());
							long long i = index.iVal;
							if (i < 0)
								i += listObj->elements.size();
							listObj->elements[i] = newVal;
						}

						stack.push_back(isPrefix ? newVal : oldVal);
					}
					DISPATCH();
				}
				OP(OP_INVOKE) : {
					{
						uint8_t nameIdx = *ip++;
						uint8_t argCount = *ip++;
						string methodName = currentChunk->constants[nameIdx].asString();
						Value receiver = stack[stack.size() - 1 - argCount];
						bool shouldDispatch = false;

						if (receiver.type == ValueType::INSTANCE ||
							 receiver.type == ValueType::CLASS ||
							 receiver.type == ValueType::SUPER) {

							Value methodToCall;
							Value selfVal = Value::None();
							Value objVal = Value::None();
							ClassObject *startClass = nullptr;

							if (receiver.type == ValueType::SUPER) {
								auto *superObj = static_cast<SuperObject *>(receiver.ref.get());
								startClass = superObj->startClass;
								selfVal = superObj->instance;
							} else if (receiver.type == ValueType::INSTANCE) {
								auto *instance = static_cast<InstanceObject *>(receiver.ref.get());
								startClass = instance->klass;
								selfVal = receiver;
							} else {
								startClass = static_cast<ClassObject *>(receiver.ref.get());
								selfVal = Value::None();
							}

							ClassObject *methodOwner = nullptr;
							ClassObject::MethodInfo *methodInfo = nullptr;

							if (receiver.type == ValueType::INSTANCE) {
								auto *inst = static_cast<InstanceObject *>(receiver.ref.get());
								if (inst->fields.count(methodName)) {
									Value dynamicMethod = inst->fields[methodName];
									vector<Value> args(argCount);
									for (int i = argCount - 1; i >= 0; i--)
										args[i] = pop();
									pop();
									for (const auto &v : args)
										stack.push_back(v);
									callValue(dynamicMethod, argCount, line, col);
									shouldDispatch = true;
								}
							}

							if (!shouldDispatch) {
								for (auto *ancestor : startClass->mro) {
									if (ancestor->methods.count(methodName)) {
										methodOwner = ancestor;
										methodInfo = &ancestor->methods[methodName];
										break;
									}
								}

								if (!methodOwner || !methodInfo) {
									bool foundStatic = false;
									for (auto *ancestor : startClass->mro) {
										if (ancestor->staticFields.count(methodName)) {
											Value staticCallable = ancestor->staticFields[methodName];
											vector<Value> tempArgs(argCount);
											for (int i = argCount - 1; i >= 0; i--)
												tempArgs[i] = pop();
											pop();
											for (const auto &v : tempArgs)
												stack.push_back(v);
											callValue(staticCallable, argCount, line, col);
											foundStatic = true;
											shouldDispatch = true;
											break;
										}
									}
									if (!foundStatic)
										throw AttributeError("'" + startClass->name + "' object has no attribute '" + methodName + "'", line, col);
								}

								if (!shouldDispatch) {
									if (methodInfo->access != AccessLevel::PUBLIC) {
										bool allowed = false;
										if (frame->basePointer + 1 < stack.size()) {
											Value potentialCaller = stack[frame->basePointer + 1];
											if (potentialCaller.type == ValueType::CLASS) {
												auto *callerPtr = static_cast<ClassObject *>(potentialCaller.ref.get());
												if (methodInfo->access == AccessLevel::PRIVATE) {
													if (callerPtr == methodOwner)
														allowed = true;
												} else if (methodInfo->access == AccessLevel::PROTECTED) {
													for (auto *ancestor : callerPtr->mro) {
														if (ancestor == methodOwner) {
															allowed = true;
															break;
														}
													}
												}
											}
										}
										if (!allowed) {
											throw RuntimeError("Cannot access method '" + methodName + "' from this context.", line, col);
										}
									}

									methodToCall = methodInfo->func;
									objVal.type = ValueType::CLASS;
									objVal.ref = std::shared_ptr<HeapObject>(methodOwner, [](HeapObject *) {});

									vector<Value> args(argCount);
									for (int i = argCount - 1; i >= 0; i--)
										args[i] = pop();
									pop();
									stack.push_back(selfVal);
									stack.push_back(objVal);
									for (const auto &v : args)
										stack.push_back(v);

									callValue(methodToCall, argCount + 2, line, col);
									shouldDispatch = true;
								}
							}
						}

						if (!shouldDispatch) {
							// Fallback: Dummy AST execution
							vector<ValueExpr *> dummyArgs(argCount);
							for (int i = argCount - 1; i >= 0; i--)
								dummyArgs[i] = new ValueExpr(pop());
							Value targetVal = pop();
							ValueExpr *dummyObject = new ValueExpr(targetVal);

							vector<Expr *> exprArgs(dummyArgs.begin(), dummyArgs.end());
							MethodCallExpr mockAST(dummyObject, methodName, exprArgs);
							mockAST.line = line;

							if (!methodResolver) {
								delete dummyObject;
								for (auto *a : dummyArgs)
									delete a;
								throw EnvironmentError("VM methodResolver bridge not initialized.", line, col);
							}

							Value result;
							try {
								result = methodResolver(&mockAST);
							} catch (...) {
								delete dummyObject;
								for (auto *a : dummyArgs)
									delete a;
								throw;
							}

							delete dummyObject;
							for (auto *a : dummyArgs)
								delete a;

							stack.push_back(result);
						}
					}
					DISPATCH();
				}
				OP(OP_TRY_ENTER) : {
					{
						uint8_t cHi = *ip++;
						uint8_t cLo = *ip++;
						uint8_t fHi = *ip++;
						uint8_t fLo = *ip++;
						int catchOffset = (cHi << 8) | cLo;
						int finallyOffset = (fHi << 8) | fLo;
						ExceptionHandler h;
						h.stackDepth = stack.size();
						int currentOffset = (int)(ip - 4 - currentChunk->code.data());
						h.catchAddress = currentOffset + catchOffset;
						h.finallyAddress =
							(finallyOffset == 0) ? -1 : currentOffset + finallyOffset;
						frame->handlerStack.push_back(h);
					}
					DISPATCH();
				}

				OP(OP_TRY_EXIT) : {
					{
						if (!frame->handlerStack.empty())
							frame->handlerStack.pop_back();
					}
					DISPATCH();
				}
				OP(OP_THROW) : {
					{
						Value typeVal = pop();
						Value msgVal = pop();
						string t = typeVal.asString();
						string m = msgVal.asString();
						static const std::unordered_map<
							std::string, std::function<void(string, int, int)>>
							errorFactory = {
								// Base Categories
								{"InternalError",
									[](string m, int l, int c) {
										throw InternalError(m, l, c);
									}},
								{"ControlFlowError",
									[](string m, int l, int c) {
										throw ControlFlowError(m, l, c);
									}},
								{"ParseError",
									[](string m, int l, int c) {
										throw ParseError(m, l, c);
									}},
								{"RuntimeError",
									[](string m, int l, int c) {
										throw RuntimeError(m, l, c);
									}},
								{"Warning", [](string m, int l,
													int c) { throw Warning(m, l, c); }},

								// Runtime Errors
								{"NameError",
									[](string m, int l, int c) {
										throw NameError(m, l, c);
									}},
								{"AttributeError",
									[](string m, int l, int c) {
										throw AttributeError(m, l, c);
									}},
								{"TypeError",
									[](string m, int l, int c) {
										throw TypeError(m, l, c);
									}},
								{"ArgumentError",
									[](string m, int l, int c) {
										throw ArgumentError(m, l, c);
									}},
								{"ValueError",
									[](string m, int l, int c) {
										throw ValueError(m, l, c);
									}},
								{"ConstError",
									[](string m, int l, int c) {
										throw ConstError(m, l, c);
									}},
								{"OwnershipError",
									[](string m, int l, int c) {
										throw OwnershipError(m, l, c);
									}},
								{"IndexError",
									[](string m, int l, int c) {
										throw IndexError(m, l, c);
									}},
								{"KeyError", [](string m, int l,
													 int c) { throw KeyError(m, l, c); }},
								{"RangeError",
									[](string m, int l, int c) {
										throw RangeError(m, l, c);
									}},
								{"AssertionError",
									[](string m, int l, int c) {
										throw AssertionError(m, l, c);
									}},
								{"RecursionError",
									[](string m, int l, int c) {
										throw RecursionError(m, l, c);
									}},
								{"ImportError",
									[](string m, int l, int c) {
										throw ImportError(m, l, c);
									}},
								{"IOError", [](string m, int l,
													int c) { throw IOError(m, l, c); }},
								{"MathError",
									[](string m, int l, int c) {
										throw MathError(m, l, c);
									}},
								{"CastError",
									[](string m, int l, int c) {
										throw CastError(m, l, c);
									}},
								{"IteratorError",
									[](string m, int l, int c) {
										throw IteratorError(m, l, c);
									}},
								{"TimeoutError",
									[](string m, int l, int c) {
										throw TimeoutError(m, l, c);
									}},
								{"MemoryError",
									[](string m, int l, int c) {
										throw MemoryError(m, l, c);
									}},
								{"SystemError",
									[](string m, int l, int c) {
										throw SystemError(m, l, c);
									}},

								// Specific Subtypes
								{"EmptyContainerError",
									[](string m, int l, int c) {
										throw EmptyContainerError(m, l, c);
									}},
								{"MutationError",
									[](string m, int l, int c) {
										throw MutationError(m, l, c);
									}},
								{"ModuleNotFoundError",
									[](string m, int l, int c) {
										throw ModuleNotFoundError(m, l, c);
									}},
								{"CircularImportError",
									[](string m, int l, int c) {
										throw CircularImportError(m, l, c);
									}},
								{"InvalidImportError",
									[](string m, int l, int c) {
										throw InvalidImportError(m, l, c);
									}},
								{"FileNotFoundError",
									[](string m, int l, int c) {
										throw FileNotFoundError(m, l, c);
									}},
								{"PermissionError",
									[](string m, int l, int c) {
										throw PermissionError(m, l, c);
									}},
								{"EOFError", [](string m, int l,
													 int c) { throw EOFError(m, l, c); }},
								{"FileClosedError",
									[](string m, int l, int c) {
										throw FileClosedError(m, l, c);
									}},
								{"DivisionByZeroError",
									[](string m, int l, int c) {
										throw DivisionByZeroError(m, l, c);
									}},
								{"OverflowError",
									[](string m, int l, int c) {
										throw OverflowError(m, l, c);
									}},
								{"UnderflowError",
									[](string m, int l, int c) {
										throw UnderflowError(m, l, c);
									}},
								{"DomainError",
									[](string m, int l, int c) {
										throw DomainError(m, l, c);
									}},
								{"OSError", [](string m, int l,
													int c) { throw OSError(m, l, c); }},
								{"EnvironmentError",
									[](string m, int l, int c) {
										throw EnvironmentError(m, l, c);
									}},
								{"SignalError",
									[](string m, int l, int c) {
										throw SignalError(m, l, c);
									}},
								{"DeprecationWarning",
									[](string m, int l,
										int c) { throw DeprecationWarning(m, l, c); }},
								{"RuntimeWarning",
									[](string m, int l,
										int c) { throw RuntimeWarning(m, l, c); }},
								{"ImportWarning", [](string m, int l, int c) {
									 throw ImportWarning(m, l, c);
								 }}};
						auto it = errorFactory.find(t);
						if (it != errorFactory.end())
							it->second(m, line, col);
						else
							throw LangError(t, m, -1, line, col);
					}
					DISPATCH();
				}
				OP(OP_CATCH) : {
					{
						uint8_t count = *ip++;
						Value errorVal = stack.back();
						bool match = false;
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
						if (errorVal.type == ValueType::ERROR) {
							auto *errObj =
								static_cast<ErrorObject *>(errorVal.ref.get());
							for (int i = 0; i < count; i++) {
								uint8_t typeIdx = *ip++;
								if (match)
									continue;
								string catchType =
									currentChunk->constants[typeIdx].asString();
								string currentType = errObj->errType;
								while (true) {
									if (currentType == catchType) {
										match = true;
										break;
									}
									if (catchType == "Error") {
										match = true;
										break;
									}
									auto it = parentMap.find(currentType);
									if (it == parentMap.end())
										break;
									currentType = it->second;
								}
							}
						} else
							ip += count;
						stack.push_back(Value::Bool(match));
					}
					DISPATCH();
				}
				OP(OP_RETHROW) : {
					{
						Value errorVal = pop();
						if (errorVal.type == ValueType::ERROR) {
							auto *errObj =
								static_cast<ErrorObject *>(errorVal.ref.get());
							throw LangError(errObj->errType, errObj->message,
								errObj->code, errObj->line, errObj->col);
						}
					}
					DISPATCH();
				}
				OP(OP_END_FINALLY) : {
					bool goToReturn = false;
					{
						if (isHandlingError) {
							isHandlingError = false;
							auto *errObj =
								static_cast<ErrorObject *>(pendingError.ref.get());
							throw LangError(errObj->errType, errObj->message,
								errObj->code, errObj->line, errObj->col);
						}
						if (isReturning) {
							isReturning = false;
							stack.push_back(pendingReturn);
							goToReturn = true;
						}
					}
					if (goToReturn) {
						goto execute_return;
					}
					DISPATCH();
				}
				OP(OP_DELETE) : {
					{
						Value refVal = pop();
						if (refVal.type != ValueType::REFERENCE || !refVal.get_ptr_safe())
							throw RuntimeError("Cannot delete a non-reference value",
								line, col);
						Value *target = refVal.get_ptr_safe();
						Value actualVal = *target;
						*target = Value::NoType();
						if (actualVal.type == ValueType::INSTANCE) {
							auto *instance =
								static_cast<InstanceObject *>(actualVal.ref.get());
							ClassObject *cls = instance->klass;
							ClassObject::MethodInfo *delMethod = nullptr;
							for (auto *ancestor : cls->mro) {
								if (ancestor->methods.count("__destruct__")) {
									delMethod = &ancestor->methods["__destruct__"];
									break;
								}
							}
							if (delMethod) {
								stack.push_back(actualVal);
								Value objVal;
								objVal.type = ValueType::CLASS;
								objVal.ref = std::shared_ptr<HeapObject>(
									cls, [](HeapObject *) {});
								stack.push_back(objVal);
								callValue(delMethod->func, 2, line, col);
							} else {
								stack.push_back(Value::None());
							}
						} else {
							stack.push_back(Value::None());
						}
					}
					DISPATCH();
				}
				OP(OP_RETURN) : {
				execute_return: // <--- Safely outside the Scope Lock!
					bool shouldReturnFromVM = false;
					{
						bool interceptedByFinally = false;
						if (!frame->handlerStack.empty()) {
							ExceptionHandler &h = frame->handlerStack.back();
							if (h.finallyAddress != -1 && !h.isInsideFinally) {
								isReturning = true;
								pendingReturn = pop();
								h.isInsideFinally = true;
								ip = currentChunk->code.data() + h.finallyAddress;
								interceptedByFinally = true; // Triggers dispatch instead of return logic
							}
						}
						if (!interceptedByFinally) {
							Value result = pop();
							if (result.type == ValueType::REFERENCE && !result.ref && result.ptr &&
								 result.ptr >= stack.data() + frame->basePointer &&
								 result.ptr < stack.data() + stack.capacity())
								throw OwnershipError("Cannot return a reference to a local variable", line, col);
							FunctionObject *func = frame->function;
							if (func) {
								if (func->name == magic_methods_to_string(
															Magic_Methods::__construct__)) {
									result = stack[frame->basePointer];
								} else {
									if (result.type == ValueType::NOTYPE) {
										if (!func->defaultRetChunks.empty()) { // chunks, not AST: survive .ymc/.yb
											result =
												executeDefault(func->defaultRetChunks.empty() ? nullptr : func->defaultRetChunks[0], line);
										} else if (func->returnType != ValueType::NOTYPE) {
											result = Value::None();
										} else {
											result = Value::None();
										}
									}
								}
								if (func->returnType != ValueType::NOTYPE &&
									 result.type != func->returnType) {
									if (result.type == ValueType::REFERENCE && result.get_ptr_safe() && result.get_ptr_safe()->type == func->returnType) {
										// Target matches returnType, keep reference!
									} else {
										bool converted = false;
										if (func->returnType == ValueType::FLOAT &&
											 result.type == ValueType::INT) {
											result = Value::Float((double)result.asInt());
											converted = true;
										} else if (func->returnType == ValueType::INT &&
													  result.type == ValueType::FLOAT) {
											result = Value::Int((long long)result.asFloat());
											converted = true;
										} else if (func->returnType == ValueType::INT &&
													  result.type == ValueType::BIGINT) {
											converted = true;
										} else if (func->returnType == ValueType::BIGINT &&
													  result.type == ValueType::INT) {
											result = Value::BigInt(result.asInt());
											converted = true;
										}
										if (!converted) {
											string typeName = "";
											switch (func->returnType) {
											case ValueType::INT:
												typeName = "int";
												break;
											case ValueType::FLOAT:
												typeName = "float";
												break;
											case ValueType::BOOL:
												typeName = "bool";
												break;
											case ValueType::STRING:
												typeName = "string";
												break;
											case ValueType::LIST:
												typeName = "list";
												break;
											case ValueType::SET:
												typeName = "set";
												break;
											case ValueType::DICT:
												typeName = "dict";
												break;
											case ValueType::TUPLE:
												typeName = "tuple";
												break;
											case ValueType::VECTOR:
												typeName = "vector";
												break;
											case ValueType::RANGE:
												typeName = "range";
												break;
											default:
												break;
											}
											if (!typeName.empty() && globals->exists(typeName)) {
												Value ctor = globals->get(typeName);
												vector<Value> args;
												if (result.type != ValueType::NONE &&
													 result.type != ValueType::NOTYPE) {
													args.push_back(result);
												}
												try {
													if (ctor.type == ValueType::NATIVE_FUNCTION) {
														auto *nat =
															static_cast<NativeFunctionObject *>(
																ctor.ref.get());
														result = nat->func(args, line, 0);
														converted = true;
													} else if (ctor.type == ValueType::OVERLOAD) {
														auto *ov = static_cast<OverloadObject *>(
															ctor.ref.get());
														for (const auto &v : ov->overloads) {
															if (v.type ==
																 ValueType::NATIVE_FUNCTION) {
																auto *nat = static_cast<
																	NativeFunctionObject *>(
																	v.ref.get());
																try {
																	result = nat->func(args, line, 0);
																	converted = true;
																	break;
																} catch (...) {
																}
															}
														}
													}
												} catch (...) {
												}
											}
										}
										if (!converted && result.type != ValueType::NONE) {
											throw TypeError(
												"Return type mismatch. Expected " +
													std::to_string((int)func->returnType) +
													" Got " + std::to_string((int)result.type),
												line, col);
										}
									}
								}
								if (func->returnsConst)
									result.isConst = true;
								if (func->isCached) {
									if (frame->singleCacheKey.type != ValueType::NOTYPE) {
										func->singleArgCache[frame->singleCacheKey] = result;
									} else {
										func->cache[frame->cacheKey] = result;
									}
								}
							}
							int returnSlot = frame->basePointer;
							frames.pop_back();
							if (frames.empty()) {
								stack.push_back(result);
								shouldReturnFromVM = true;
							} else {
								frame = &frames.back();
								ip = frame->ip;
								stack.resize(returnSlot);
								stack.push_back(result);
							}
						}
					}
					if (shouldReturnFromVM) {
						return;
					}
					DISPATCH();
				}
				OP(OP_MULTI_SET) : OP(OP_AND) : OP(OP_OR) : OP(OP_IN) : OP(OP_INCREMENT) : OP(OP_DECREMENT) : OP(OP_PRE_INCREMENT) : OP(OP_PRE_DECREMENT) : OP(OP_BUILD_FILE) : OP(OP_BREAK) : OP(OP_CONTINUE) : OP(OP_SKIP) : OP(OP_OMIT) : OP(OP_ASSERT) : {
					throw InternalError("Optimized or unimplemented OpCode was executed!", line, col);
					DISPATCH();
				}
#ifndef USE_COMPUTED_GOTOS
			default:
				throw UnexpectedTokenError("Unknown OpCode encountered", line, col);
#endif
			}
		} catch (const LangError &e) {
			if (e.type == "Warning" || e.type == "ImportWarning" || e.type == "RuntimeWarning" || e.type == "DeprecationWarning") {
				std::cerr << "\033[1;36m" << e.type << ": " << e.message << "\033[0m\n";
			} else {
				if (frame->handlerStack.empty()) {
					// CTLE failures are expected; the compiler falls back to runtime evaluation
					if (e.type != "IteratorError" && !isCTLE) {
						std::cerr << "Uncaught " << e.type << ": " << e.message << "\n";
					}
					throw;
				}
				ExceptionHandler h = frame->handlerStack.back();
				frame->handlerStack.pop_back();
				while ((int)stack.size() > h.stackDepth)
					stack.pop_back();
				auto errObj = std::make_shared<ErrorObject>(
					e.type, e.message, e.code, e.line, e.col);
				Value errVal = Value::Error(errObj);
				if (h.catchAddress != -1) {
					stack.push_back(errVal);
					Chunk *currentChunk =
						frame->function ? frame->function->chunk : &chunk;
					ip = currentChunk->code.data() + h.catchAddress;
				} else if (h.finallyAddress != -1) {
					pendingError = errVal;
					isHandlingError = true;
					Chunk *currentChunk =
						frame->function ? frame->function->chunk : &chunk;
					ip = currentChunk->code.data() + h.finallyAddress;
				} else {
					throw;
				}
			}
		}
	}
}

