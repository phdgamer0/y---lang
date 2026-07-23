#include "ylang/compiler.hpp"
#include "ylang/vm.hpp"

	void ByteCodeCompiler::compileStmt(Stmt *s) {
		if (!s)
			return;
		switch (s->type) {
		case StmtType::CLASS: {
			auto c = static_cast<ClassStmt *>(s);
			for (const auto &p : c->parents) {
				int arg = resolveLocal(p);
				if (arg != -1) {
					emitByte(OpCode::OP_GET_LOCAL, c->line, 0);
					chunk->write((uint8_t)arg, c->line, 0);
				} else
					emitIdentifier(OpCode::OP_GET_VAR, p, c->line, 0);
			}
			emitConstant(Value::String(c->name), c->line, 0);
			emitByte(OpCode::OP_CLASS, c->line, 0);
			chunk->write((uint8_t)c->parents.size(), c->line, 0);
			vector<Stmt *> instanceFields;
			auto collectFields = [&](const vector<Stmt *> &body) {
				for (auto *stmt : body) {
					if (stmt->type == StmtType::LET) {
						auto let = static_cast<LetStmt *>(stmt);
						if (let->name.rfind("self.", 0) == 0)
							instanceFields.push_back(let);
					}
				}
			};
			collectFields(c->publicBody);
			collectFields(c->privateBody);
			collectFields(c->protectedBody);
			auto compileClassBody = [&](const vector<Stmt *> &body,
												AccessLevel access) {
				for (auto *stmt : body) {
					if (stmt->type == StmtType::FUNC) {
						auto *func = static_cast<FuncStmt *>(stmt);
						if (func->name == "__construct__")
							emitFunction(func, true, instanceFields);
						else
							emitFunction(func, true);
						emitByte(OpCode::OP_METHOD, c->line, 0);
						chunk->write((uint8_t)access, c->line, 0);
					} else if (stmt->type == StmtType::LET) {
						auto let = static_cast<LetStmt *>(stmt);
						string propName = let->name;
						bool isInstance = false;
						if (propName.rfind("self.", 0) == 0) {
							propName = propName.substr(5);
							isInstance = true;
						} else if (propName.rfind("obj.", 0) == 0)
							propName = propName.substr(4);
						if (isInstance)
							emitByte(OpCode::OP_NOTYPE, let->line, let->col);
						else if (let->value)
							compile(let->value);
						else
							emitByte(OpCode::OP_NOTYPE, let->line, let->col);
						emitByte(OpCode::OP_CLASS_FIELD, let->line, let->col);
						chunk->write((uint8_t)access, let->line, let->col);
						int nameIdx = chunk->addConstant(Value::String(propName));
						chunk->write((uint8_t)nameIdx, let->line, let->col);
					} else if (stmt->type == StmtType::MULTI_LET) {
						auto mlet = static_cast<MultiLetStmt *>(stmt);
						for (size_t i = 0; i < mlet->names.size(); i++) {
							string propName = mlet->names[i];
							bool isInstance = false;
							if (propName.rfind("self.", 0) == 0) {
								propName = propName.substr(5);
								isInstance = true;
							} else if (propName.rfind("obj.", 0) == 0)
								propName = propName.substr(4);
							if (isInstance)
								emitByte(OpCode::OP_NOTYPE, mlet->line, 0);
							else if (mlet->values[i])
								compile(mlet->values[i]);
							else
								emitByte(OpCode::OP_NOTYPE, mlet->line, 0);
							emitByte(OpCode::OP_CLASS_FIELD, mlet->line, 0);
							chunk->write((uint8_t)access, mlet->line, 0);
							int nameIdx =
								chunk->addConstant(Value::String(propName));
							chunk->write((uint8_t)nameIdx, mlet->line, 0);
						}
					}
				}
			};
			compileClassBody(c->publicBody, AccessLevel::PUBLIC);
			compileClassBody(c->privateBody, AccessLevel::PRIVATE);
			compileClassBody(c->protectedBody, AccessLevel::PROTECTED);
			if (scopeDepth > 0) {
				addLocal(c->name);
				emitByte(OpCode::OP_SET_LOCAL, c->line, 0);
				chunk->write((uint8_t)(locals.size() - 1), c->line, 0);
			} else {
				emitIdentifier(OpCode::OP_DEFINE_VAR, c->name, c->line, 0);
				chunk->write((uint8_t)0, c->line, 0);
			}
			break;
		}
		case StmtType::DELETE: {
			auto delStmt = static_cast<DeleteStmt *>(s);
			if (auto v = dynamic_cast<VarExpr *>(delStmt->target)) {
				int local = resolveLocal(v->name);
				if (local != -1) {
					emitByte(OpCode::OP_REF_LOCAL, delStmt->line, delStmt->col);
					chunk->write((uint8_t)local, delStmt->line, delStmt->col);
				} else
					emitIdentifier(OpCode::OP_REF_VAR, v->name, delStmt->line,
						delStmt->col);
			} else if (auto idx = dynamic_cast<IndexExpr *>(delStmt->target)) {
				compile(idx->base);
				compile(idx->index);
				emitByte(OpCode::OP_REF_INDEX, delStmt->line, delStmt->col);
			}
			/*else if (auto own = dynamic_cast<OwnershipExpr*>(delStmt->target))
			{ compile(own->expr); emitByte(OpCode::OP_)
			}*/
			else
				throw SyntaxError("Can only delete variables or indices",
					delStmt->line, delStmt->col);
			emitByte(OpCode::OP_DELETE, delStmt->line, delStmt->col);
			emitByte(OpCode::OP_POP, delStmt->line, delStmt->col);
			break;
		}
		case StmtType::EXPR: {
			auto es = static_cast<ExprStmt *>(s);
			compile(es->expr);
			if (isReplMode && scopeDepth == 0) {
				emitByte(OpCode::OP_REPL_PRINT, es->line, es->col);
			} else {
				emitByte(OpCode::OP_POP, es->line, es->col);
			}
			break;
		}
		case StmtType::ASSERT: {
			auto as = static_cast<AssertStmt *>(s);
			compile(as->condition);
			int failJump = emitJump(OpCode::OP_JUMP_IF_FALSE, as->line, 0);
			emitByte(OpCode::OP_POP, as->line, 0);
			int successJump = emitJump(OpCode::OP_JUMP, as->line, 0);
			patchJump(failJump);
			emitByte(OpCode::OP_POP, as->line, 0);
			if (as->message)
				compile(as->message);
			else {
				std::string msg =
					"Assertion failed on line " + std::to_string(as->line);
				emitConstant(Value::String(msg), as->line, 0);
			}
			emitConstant(Value::String("AssertionError"), as->line, 0);
			emitByte(OpCode::OP_THROW, as->line, 0);
			patchJump(successJump);
			break;
		}
		case StmtType::THROW: {
			auto ts = static_cast<ThrowStmt *>(s);
			compile(ts->message);
			emitConstant(Value::String(ts->errorType), ts->line, 0);
			emitByte(OpCode::OP_THROW, ts->line, 0);
			break;
		}
		case StmtType::TRY: {
			auto t = static_cast<TryStmt *>(s);
			emitByte(OpCode::OP_TRY_ENTER, t->line, 0);
			int tryEnterAddr = (int)chunk->code.size();
			chunk->write(0xff, t->line, 0);
			chunk->write(0xff, t->line, 0);
			chunk->write(0xff, t->line, 0);
			chunk->write(0xff, t->line, 0);
			for (auto st : t->tryBlock)
				compileStmt(st);
			emitByte(OpCode::OP_TRY_EXIT, t->line, 0);
			int skipCatchJump = emitJump(OpCode::OP_JUMP, t->line, 0);
			int catchStartAddr = (int)chunk->code.size();
			int catchOffset = catchStartAddr - tryEnterAddr;
			chunk->code[tryEnterAddr] = (catchOffset >> 8) & 0xff;
			chunk->code[tryEnterAddr + 1] = catchOffset & 0xff;
			vector<int> exitJumps;
			for (auto &catchBlock : t->catches) {
				emitByte(OpCode::OP_CATCH, t->line, 0);
				chunk->write((uint8_t)catchBlock.types.size(), t->line, 0);
				for (const auto &typeName : catchBlock.types) {
					int idx = chunk->addConstant(Value::String(typeName));
					chunk->write((uint8_t)idx, t->line, 0);
				}
				int nextCatchJump =
					emitJump(OpCode::OP_JUMP_IF_FALSE, t->line, 0);
				emitByte(OpCode::OP_POP, t->line, 0);
				emitByte(OpCode::OP_POP, t->line, 0);
				beginScope();
				for (auto st : catchBlock.body)
					compileStmt(st);
				endScope(t->line, 0);
				exitJumps.push_back(emitJump(OpCode::OP_JUMP, t->line, 0));
				patchJump(nextCatchJump);
				emitByte(OpCode::OP_POP, t->line, 0);
			}
			emitByte(OpCode::OP_RETHROW, t->line, 0);
			patchJump(skipCatchJump);
			for (int j : exitJumps)
				patchJump(j);
			int finallyStartAddr = (int)chunk->code.size();
			if (!t->finallyBlock.empty()) {
				int finallyOffset = finallyStartAddr - tryEnterAddr;
				chunk->code[tryEnterAddr + 2] = (finallyOffset >> 8) & 0xff;
				chunk->code[tryEnterAddr + 3] = finallyOffset & 0xff;
				beginScope();
				for (auto st : t->finallyBlock)
					compileStmt(st);
				endScope(t->line, 0);
				emitByte(OpCode::OP_END_FINALLY, t->line, 0);
			} else {
				chunk->code[tryEnterAddr + 2] = 0;
				chunk->code[tryEnterAddr + 3] = 0;
			}
			break;
		}
		case StmtType::IMPORT: {
			auto imp = static_cast<ImportStmt *>(s);
			emitConstant(Value::String(imp->libName), imp->line, 0);
			for (const auto &sym : imp->symbols)
				emitConstant(Value::String(sym), imp->line, 0);
			emitByte(OpCode::OP_IMPORT, imp->line, 0);
			chunk->write((uint8_t)imp->symbols.size(), imp->line, 0);
			if (imp->symbols.empty()) {
				auto getModuleName = [](const std::string& path) {
					size_t slash = path.find_last_of("/\\");
					std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
					if (name.length() > 4 && name.substr(name.length() - 4) == ".ymm") {
						name = name.substr(0, name.length() - 4);
					}
					return name;
				};
				std::string modName = getModuleName(imp->libName);
				if (scopeDepth > 0) {
					addLocal(modName);
				} else {
					emitIdentifier(OpCode::OP_DEFINE_VAR, modName, imp->line, 0);
					chunk->write(0, imp->line, 0);
				}
				if (isCompileOnly && interp) {
					if (interp->modules.find(imp->libName) != interp->modules.end()) {
						interp->modules[imp->libName](interp->env, {});
					} else if (interp->modules.find(modName) != interp->modules.end()) {
						interp->modules[modName](interp->env, {});
					} else {
						// To avoid throwing false positives if it's a valid local .ymm file import, we just define it as a dummy NoType.
						interp->env->set(modName, Value::NoType(), false);
					}
				}
			} else {
				if (scopeDepth > 0) {
					for (const auto &sym : imp->symbols) {
						addLocal(sym);
					}
				} else {
					for (int i = (int)imp->symbols.size() - 1; i >= 0; i--) {
						emitIdentifier(OpCode::OP_DEFINE_VAR, imp->symbols[i], imp->line, 0);
						chunk->write(0, imp->line, 0);
					}
				}
				if (isCompileOnly && interp) {
					if (interp->modules.find(imp->libName) != interp->modules.end()) {
						interp->modules[imp->libName](interp->env, imp->symbols);
					} else {
						// If we can't statically find the module, just register the symbols so they don't throw NameError.
						for (const auto &sym : imp->symbols) {
							interp->env->set(sym, Value::NoType(), false);
						}
					}
				}
			}
			break;
		}
		case StmtType::LET: {
			auto let = static_cast<LetStmt *>(s);
			bool isSelf = (let->name.rfind("self.", 0) == 0);
			bool isObj = (let->name.rfind("obj.", 0) == 0);
			if (isSelf || isObj) {
				string propName = let->name.substr(isSelf ? 5 : 4);
				int objSlot = resolveLocal(isSelf ? "self" : "obj");
				if (objSlot == -1)
					throw NameError("Cannot use '" + string(isSelf ? "self" : "obj") + "' outside of a method.", let->line, let->col);
				emitByte(OpCode::OP_GET_LOCAL, let->line, let->col);
				chunk->write((uint8_t)objSlot, let->line, let->col);
				if (let->value)
					compile(let->value);
				else
					emitByte(OpCode::OP_NOTYPE, let->line, let->col);
				emitIdentifier(OpCode::OP_SET_PROPERTY, propName, let->line, let->col);
				chunk->write((uint8_t)let->access, let->line, let->col);
				emitByte(OpCode::OP_POP, let->line, let->col);
				break;
			}
			if (scopeDepth == 0 && let->value &&
				 let->value->type == ExprType::OWNERSHIP) {
				auto o = static_cast<OwnershipExpr *>(let->value);
				if (o->mode == CopyMode::REF) {
					emitIdentifier(OpCode::OP_DEFINE_REF, let->name, let->line,
						let->col);
					if (auto v = dynamic_cast<VarExpr *>(o->expr))
						emitIdentifier(OpCode::OP_REF_VAR, v->name, let->line,
							let->col);
					else if (auto idx = dynamic_cast<IndexExpr *>(o->expr)) {
						compile(idx->base);
						compile(idx->index);
						emitByte(OpCode::OP_REF_INDEX, let->line, let->col);
					}
					break;
				}
			}
			if (let->value) {
				compile(let->value);
				if (let->value->type != ExprType::OWNERSHIP &&
					 let->value->type != ExprType::LIST &&
					 let->value->type != ExprType::DICT &&
					 let->value->type != ExprType::SET &&
					 let->value->type != ExprType::TUPLE &&
					 let->value->type != ExprType::VECTOR &&
					 let->value->type != ExprType::LAMBDA &&
					 let->value->type != ExprType::NUMBER &&
					 let->value->type != ExprType::STRING &&
					 let->value->type != ExprType::BOOL) {
					emitByte(OpCode::OP_SHALLOW_COPY, let->line, let->col);
				}
			} else
				emitByte(OpCode::OP_NOTYPE, let->line, let->col);
			if (scopeDepth > 0) {
				if (let->isConst || let->isLocked) {
					emitByte(OpCode::OP_SET_FLAGS, let->line, let->col);
					uint8_t flags = 0;
					if (let->isConst)
						flags |= 0x01;
					if (let->isLocked)
						flags |= 0x02;
					chunk->write(flags, let->line, let->col);
				}
				addLocal(let->name);
				if (let->value) {
					// We no longer set hasKnownValue for non-const locals here!
					// Because otherwise variables like loop counters (j=0) will be wrongly folded as constants
				}
				if (let->isLocked && let->value) {
					ValueType vt = tryExtractType(let->value);
					if (vt != ValueType::NOTYPE) {
						locals.back().hasKnownType = true;
						locals.back().knownType = vt;
					}
				}
				if (let->isConst && let->value) {
					Value constVal;
					if (tryExtractConstant(let->value, constVal)) {
						locals.back().isConst = true;
						locals.back().hasKnownValue = true;
						locals.back().knownValue = constVal;
					}
				}
				if (DEBUGGER_MODE_IS_ENABLED)
					emitIdentifier(OpCode::OP_DEBUG_NAME, let->name, let->line,
						let->col);
			} else {
				emitIdentifier(OpCode::OP_DEFINE_VAR, let->name, let->line,
					let->col);
				uint8_t flags = 0;
				if (let->isConst)
					flags |= 0x01;
				if (let->isLocked)
					flags |= 0x02;
				chunk->write(flags, let->line, let->col);
				if (let->isConst && let->value) {
					Value constVal;
					if (tryExtractConstant(let->value, constVal)) {
						knownGlobals[let->name] = constVal;
					}
				}
			}
			break;
		}
		case StmtType::ASSIGN: {
			auto as = static_cast<AssignStmt *>(s);
			if (auto get = dynamic_cast<GetExpr *>(as->target)) {
				compile(get->object);
				if (as->op != TokenType::ASSIGN) {
					emitByte(OpCode::OP_DUP, as->line, as->col);
					emitIdentifier(OpCode::OP_GET_PROPERTY, get->name, as->line,
						as->col);
					compile(as->value);
					switch (as->op) {
					case TokenType::PLUS_EQ:
						emitByte(OpCode::OP_IADD, as->line, as->col);
						break;
					case TokenType::MINUS_EQ:
						emitByte(OpCode::OP_ISUB, as->line, as->col);
						break;
					case TokenType::STAR_EQ:
						emitByte(OpCode::OP_IMUL, as->line, as->col);
						break;
					case TokenType::DIV_EQ:
						emitByte(OpCode::OP_IDIV, as->line, as->col);
						break;
					case TokenType::FLOOR_DIV_EQ:
						emitByte(OpCode::OP_IFLOOR_DIV, as->line, as->col);
						break;
					case TokenType::MOD_EQ:
						emitByte(OpCode::OP_IMOD, as->line, as->col);
						break;
					case TokenType::POW_EQ:
						emitByte(OpCode::OP_IPOW, as->line, as->col);
						break;
					case TokenType::BITWISE_AND_EQ:
						emitByte(OpCode::OP_BITWISE_AND, as->line, as->col);
						break;
					case TokenType::BITWISE_OR_EQ:
						emitByte(OpCode::OP_BITWISE_OR, as->line, as->col);
						break;
					case TokenType::BITWISE_XOR_EQ:
						emitByte(OpCode::OP_BITWISE_XOR, as->line, as->col);
						break;
					case TokenType::BITWISE_NOT_EQ:
						emitByte(OpCode::OP_BITWISE_NOT, as->line, as->col);
						break;
					case TokenType::BITWISE_SHL_EQ:
						emitByte(OpCode::OP_BITWISE_SHL, as->line, as->col);
						break;
					case TokenType::BITWISE_SHR_EQ:
						emitByte(OpCode::OP_BITWISE_SHR, as->line, as->col);
						break;

					default:
						throw SyntaxError(
							"Unknown/Unsupported augmented assignment operator",
							as->line, as->col);
					}
				} else
					compile(as->value); // Stack: [obj, val]
				emitIdentifier(OpCode::OP_SET_PROPERTY, get->name, as->line, as->col);
				chunk->write((uint8_t)AccessLevel::PUBLIC, as->line, as->col);
				emitByte(OpCode::OP_POP, as->line, as->col);
				break;
			}
			if (auto idx = dynamic_cast<IndexExpr *>(as->target)) {
				if (as->op != TokenType::ASSIGN) {
					compile(idx->base);
					compile(idx->index);
					emitByte(OpCode::OP_DUP_2, as->line, as->col);
					emitByte(OpCode::OP_GET_INDEX, as->line, as->col);
					compile(as->value);
					switch (as->op) {
					case TokenType::PLUS_EQ:
						emitByte(OpCode::OP_IADD, as->line, as->col);
						break;
					case TokenType::MINUS_EQ:
						emitByte(OpCode::OP_ISUB, as->line, as->col);
						break;
					case TokenType::STAR_EQ:
						emitByte(OpCode::OP_IMUL, as->line, as->col);
						break;
					case TokenType::DIV_EQ:
						emitByte(OpCode::OP_IDIV, as->line, as->col);
						break;
					case TokenType::FLOOR_DIV_EQ:
						emitByte(OpCode::OP_IFLOOR_DIV, as->line, as->col);
						break;
					case TokenType::MOD_EQ:
						emitByte(OpCode::OP_IMOD, as->line, as->col);
						break;
					case TokenType::POW_EQ:
						emitByte(OpCode::OP_IPOW, as->line, as->col);
						break;
					case TokenType::BITWISE_AND_EQ:
						emitByte(OpCode::OP_BITWISE_AND, as->line, as->col);
						break;
					case TokenType::BITWISE_OR_EQ:
						emitByte(OpCode::OP_BITWISE_OR, as->line, as->col);
						break;
					case TokenType::BITWISE_XOR_EQ:
						emitByte(OpCode::OP_BITWISE_XOR, as->line, as->col);
						break;
					case TokenType::BITWISE_NOT_EQ:
						emitByte(OpCode::OP_BITWISE_NOT, as->line, as->col);
						break;
					case TokenType::BITWISE_SHL_EQ:
						emitByte(OpCode::OP_BITWISE_SHL, as->line, as->col);
						break;
					case TokenType::BITWISE_SHR_EQ:
						emitByte(OpCode::OP_BITWISE_SHR, as->line, as->col);
						break;
					default:
						throw SyntaxError("Unknown augmented assignment",
							as->line, as->col);
					}
					emitByte(OpCode::OP_SET_INDEX, as->line, as->col);
					emitByte(OpCode::OP_POP, as->line, as->col);
					break;
				}
				compile(idx->base);
				compile(idx->index);
				compile(as->value);
				emitByte(OpCode::OP_SET_INDEX, as->line, as->col);
				emitByte(OpCode::OP_POP, as->line, as->col);
				break;
			}
			auto v = static_cast<VarExpr *>(as->target);
			if (auto o = dynamic_cast<OwnershipExpr *>(as->value)) {
				if (scopeDepth == 0 && o->mode == CopyMode::REF) {
					emitIdentifier(OpCode::OP_SET_REF, v->name, as->line,
						as->col);
					if (auto targetVar = dynamic_cast<VarExpr *>(o->expr)) {
						emitIdentifier(OpCode::OP_REF_VAR, targetVar->name,
							as->line, as->col);
					} else if (auto idx = dynamic_cast<IndexExpr *>(o->expr)) {
						compile(idx->base);
						compile(idx->index);
						emitByte(OpCode::OP_REF_INDEX, as->line, as->col);
					}
					break;
				}
			}
			int arg = resolveLocal(v->name);
			if (arg != -1) {
				bool isInc = (as->op == TokenType::PLUS_EQ);
				if (isInc) {
					if (auto num = dynamic_cast<NumberExpr *>(as->value)) {
						if (num->val == 1) {
							emitByte(OpCode::OP_INC_LOCAL, as->line, as->col);
							chunk->write((uint8_t)arg, as->line, as->col);
							break;
						}
					}
				}
			}
			if (as->op == TokenType::ASSIGN)
				compileWithMode(as->value, as->line, as->col);
			else {
				if (arg != -1) {
					emitByte(OpCode::OP_GET_LOCAL, as->line, as->col);
					chunk->write((uint8_t)arg, as->line, as->col);
				} else
					emitIdentifier(OpCode::OP_GET_VAR, v->name, as->line,
						as->col);
				compile(as->value);
				switch (as->op) {
				case TokenType::PLUS_EQ:
					emitByte(OpCode::OP_IADD, as->line, as->col);
					break;
				case TokenType::MINUS_EQ:
					emitByte(OpCode::OP_ISUB, as->line, as->col);
					break;
				case TokenType::STAR_EQ:
					emitByte(OpCode::OP_IMUL, as->line, as->col);
					break;
				case TokenType::DIV_EQ:
					emitByte(OpCode::OP_IDIV, as->line, as->col);
					break;
				case TokenType::FLOOR_DIV_EQ:
					emitByte(OpCode::OP_IFLOOR_DIV, as->line, as->col);
					break;
				case TokenType::POW_EQ:
					emitByte(OpCode::OP_IPOW, as->line, as->col);
					break;
				case TokenType::MOD_EQ:
					emitByte(OpCode::OP_IMOD, as->line, as->col);
					break;
				case TokenType::BITWISE_AND_EQ:
					emitByte(OpCode::OP_BITWISE_AND, as->line, as->col);
					break;
				case TokenType::BITWISE_OR_EQ:
					emitByte(OpCode::OP_BITWISE_OR, as->line, as->col);
					break;
				case TokenType::BITWISE_XOR_EQ:
					emitByte(OpCode::OP_BITWISE_XOR, as->line, as->col);
					break;
				case TokenType::BITWISE_NOT_EQ:
					emitByte(OpCode::OP_BITWISE_NOT, as->line, as->col);
					break;
				case TokenType::BITWISE_SHL_EQ:
					emitByte(OpCode::OP_BITWISE_SHL, as->line, as->col);
					break;
				case TokenType::BITWISE_SHR_EQ:
					emitByte(OpCode::OP_BITWISE_SHR, as->line, as->col);
					break;
				}
			}
			if (arg != -1) {
				emitByte(OpCode::OP_SET_LOCAL, as->line, as->col);
				chunk->write((uint8_t)arg, as->line, as->col);
			} else
				emitIdentifier(OpCode::OP_SET_VAR, v->name, as->line, as->col);
			emitByte(OpCode::OP_POP, as->line, as->col);
			break;
		}
		case StmtType::IF: {
			auto ifs = static_cast<IfStmt *>(s);
			vector<int> exitJumps;
			compile(ifs->condition);
			int jumpToNext =
				emitJump(OpCode::OP_JUMP_IF_FALSE, ifs->line, ifs->col);
			emitByte(OpCode::OP_POP, ifs->line, ifs->col);
			beginScope();
			for (auto stmt : ifs->body)
				compileStmt(stmt);
			endScope(ifs->line, ifs->col);
			exitJumps.push_back(emitJump(OpCode::OP_JUMP, ifs->line, ifs->col));
			patchJump(jumpToNext);
			emitByte(OpCode::OP_POP, ifs->line, ifs->col);
			for (const auto &elif : ifs->elifs) {
				compile(elif.first);
				jumpToNext =
					emitJump(OpCode::OP_JUMP_IF_FALSE, ifs->line, ifs->col);
				emitByte(OpCode::OP_POP, ifs->line, ifs->col);
				beginScope();
				for (auto stmt : elif.second)
					compileStmt(stmt);
				endScope(ifs->line, ifs->col);
				exitJumps.push_back(
					emitJump(OpCode::OP_JUMP, ifs->line, ifs->col));
				patchJump(jumpToNext);
				emitByte(OpCode::OP_POP, ifs->line, ifs->col);
			}
			if (!ifs->elseBody.empty()) {
				beginScope();
				for (auto stmt : ifs->elseBody)
					compileStmt(stmt);
				endScope(ifs->line, ifs->col);
			}
			for (int offset : exitJumps) {
				patchJump(offset);
			}
			break;
		}
		case StmtType::SWITCH: {
			auto sw = static_cast<SwitchStmt *>(s);
			int startAddr = (int)chunk->code.size();
			beginScope();
			compile(sw->target);
			addLocal("");
			LoopContext switchLoop = {startAddr, -1, {}, {}, false, (int)locals.size(), -1};
			loopStack.push_back(switchLoop);
			bool useTable = true;
			vector<long long> tableKeys;
			for (const auto &c : sw->cases) {
				if (c.value->type == ExprType::NUMBER) {
					auto num = static_cast<NumberExpr *>(c.value);
					if (!num->isFloat)
						tableKeys.push_back((long long)num->val);
					else {
						useTable = false;
						break;
					}
				} else {
					useTable = false;
					break;
				}
			}
			if (useTable && !tableKeys.empty()) {
				auto minVal =
					*std::min_element(tableKeys.begin(), tableKeys.end());
				auto maxVal =
					*std::max_element(tableKeys.begin(), tableKeys.end());
				if ((maxVal - minVal) > 256)
					useTable = false;
			} else
				useTable = false;
			// JUMP TABLE (Fast)
			if (useTable && !tableKeys.empty()) {
				long long minVal =
					*std::min_element(tableKeys.begin(), tableKeys.end());
				long long maxVal =
					*std::max_element(tableKeys.begin(), tableKeys.end());
				long long count = maxVal - minVal + 1;
				emitByte(OpCode::OP_SWITCH_TABLE, sw->line, sw->col);
				int minIdx = chunk->addConstant(Value::Int(minVal));
				chunk->write((uint8_t)minIdx, sw->line, sw->col);
				chunk->write((uint8_t)count, sw->line, sw->col);
				int tableStart = (int)chunk->code.size();
				for (int i = 0; i < count; i++) {
					chunk->write(0xff, sw->line, sw->col);
					chunk->write(0xff, sw->line, sw->col);
				}
				auto compileBodyWithLoop = [&](const vector<Stmt *> &stmts) {
					beginScope();
					for (auto stmt : stmts)
						compileStmt(stmt);
					endScope(sw->line, sw->col);
					emitByte(OpCode::OP_POP, sw->line, sw->col);
					emitLoop(startAddr, sw->line, sw->col);
				};
				int defaultAddr = (int)chunk->code.size();
				compileBodyWithLoop(sw->defaultBody);
				std::map<long long, int> caseAddresses;
				for (size_t i = 0; i < sw->cases.size(); i++) {
					caseAddresses[(long long)((NumberExpr *)sw->cases[i].value)
							->val] = (int)chunk->code.size();
					compileBodyWithLoop(sw->cases[i].body);
				}
				int currentPos = tableStart;
				for (long long i = 0; i < count; i++) {
					long long val = minVal + i;
					int target = caseAddresses.count(val) ? caseAddresses[val]
																	  : defaultAddr;
					int offset = target - currentPos - 2;
					chunk->code[currentPos] = (offset >> 8) & 0xff;
					chunk->code[currentPos + 1] = offset & 0xff;
					currentPos += 2;
				}
			}
			// LINEAR SCAN (Compatible)
			else {
				vector<int> nextCaseJumps;
				for (const auto &c : sw->cases) {
					emitByte(OpCode::OP_GET_LOCAL, sw->line, sw->col);
					chunk->write((uint8_t)(locals.size() - 1), sw->line, sw->col);
					compile(c.value);
					emitByte(OpCode::OP_STRICT_EQ, sw->line, sw->col);
					int nextJump =
						emitJump(OpCode::OP_JUMP_IF_FALSE, sw->line, sw->col);
					emitByte(OpCode::OP_POP, sw->line, sw->col);
					beginScope();
					for (auto stmt : c.body)
						compileStmt(stmt);
					endScope(sw->line, sw->col);
					emitByte(OpCode::OP_POP, sw->line, sw->col);
					emitLoop(startAddr, sw->line, sw->col);
					patchJump(nextJump);
					emitByte(OpCode::OP_POP, sw->line, sw->col);
				}
				beginScope();
				for (auto stmt : sw->defaultBody)
					compileStmt(stmt);
				endScope(sw->line, sw->col);
				emitByte(OpCode::OP_POP, sw->line, sw->col);
				emitLoop(startAddr, sw->line, sw->col);
			}
			for (int b : loopStack.back().breakJumps)
				patchJump(b);
			loopStack.pop_back();
			endScope(sw->line, sw->col);
			break;
		}
		/*
		case StmtType::FUNC: {
				  auto f = static_cast<FuncStmt*>(s);
				  Chunk* funcChunk = new Chunk();
				  ByteCodeCompiler subCompiler(funcChunk);
				  subCompiler.beginScope();
				  for (const auto& param : f->params)
		subCompiler.addLocal(param.name); for (auto bodyStmt : f->body)
		subCompiler.compileStmt(bodyStmt);
		subCompiler.emitByte(OpCode::OP_NOTYPE, f->line, f->col);
		subCompiler.emitByte(OpCode::OP_RETURN, f->line, f->col); auto* funcObj
		= new FunctionObject(f->params, f->returnType, f->defaultRetArgs,
		f->returnsConst, f->body, nullptr, f->isCached, funcChunk);
		funcObj->name= f->name; Value funcVal; funcVal.type =
		ValueType::FUNCTION; if (DEBUGGER_MODE_IS_ENABLED)
		funcVal.__DEBUGGING__NAME__=funcObj->name; funcVal.ref =
		std::shared_ptr<HeapObject>(funcObj); emitConstant(funcVal, f->line,
		f->col); emitIdentifier(OpCode::OP_DEFINE_VAR, f->name, f->line,
		f->col); chunk->write((uint8_t)0, f->line, f->col); break;
		}
		*/
		case StmtType::FUNC: {
			auto f = static_cast<FuncStmt *>(s);
			emitFunction(f);
			emitIdentifier(OpCode::OP_DEFINE_VAR, f->name, f->line, f->col);
			chunk->write((uint8_t)0, f->line, f->col);
			break;
		}
		case StmtType::RETURN: {
			auto r = static_cast<ReturnStmt *>(s);
			if (r->value)
				compile(r->value);
			else
				emitByte(OpCode::OP_NOTYPE, r->line, r->col);
			emitByte(OpCode::OP_RETURN, r->line, r->col);
			break;
		}
		case StmtType::MULTI_LET: {
			auto m = static_cast<MultiLetStmt *>(s);
			for (auto val : m->values) {
				if (val)
					compileWithMode(val, val->line, val->col);
				else
					emitByte(OpCode::OP_NOTYPE, m->line, m->col);
			}
			if (scopeDepth > 0) {
				for (size_t i = 0; i < m->names.size(); i++) {
					addLocal(m->names[i]);
				}
			} else {
				for (int i = (int)m->names.size() - 1; i >= 0; i--) {
					Expr *valExpr = m->values[i];
					if (valExpr && valExpr->type == ExprType::OWNERSHIP &&
						 static_cast<OwnershipExpr *>(valExpr)->mode ==
							 CopyMode::REF) {
						auto o = static_cast<OwnershipExpr *>(valExpr);
						emitIdentifier(OpCode::OP_DEFINE_REF, m->names[i], m->line,
							m->col);
						if (auto v = dynamic_cast<VarExpr *>(o->expr))
							emitIdentifier(OpCode::OP_REF_VAR, v->name, m->line,
								m->col);
						else if (auto idx = dynamic_cast<IndexExpr *>(o->expr)) {
							compile(idx->base);
							compile(idx->index);
							emitByte(OpCode::OP_REF_INDEX, m->line, m->col);
						}
					} else {
						emitIdentifier(OpCode::OP_DEFINE_VAR, m->names[i], m->line,
							m->col);
						uint8_t flags = 0;
						if (m->isConsts[i])
							flags |= 0x01;
						if (m->isLocked)
							flags |= 0x02;
						chunk->write(flags, m->line, m->col);
					}
				}
			}
			break;
		}
		case StmtType::MULTI_ASSIGN: {
			auto ma = static_cast<MultiAssignStmt *>(s);
			for (auto val : ma->values)
				compileWithMode(val, val->line, val->col);
			for (int i = (int)ma->targets.size() - 1; i >= 0; i--) {
				auto v = static_cast<VarExpr *>(ma->targets[i]);
				int arg = resolveLocal(v->name);
				if (arg != -1) {
					emitByte(OpCode::OP_SET_LOCAL, ma->line, ma->col);
					chunk->write((uint8_t)arg, ma->line, ma->col);
				} else
					emitIdentifier(OpCode::OP_SET_VAR, v->name, ma->line,
						ma->col);
				emitByte(OpCode::OP_POP, ma->line, ma->col);
			}
			break;
		}
		case StmtType::WHILE: {
			if (!isCTLE && tryEvaluateCompileTimeLoop(s)) {
				break;
			}
			beginScope();
			auto w = static_cast<WhileStmt *>(s);
			int startAddr = (int)chunk->code.size();
			LoopContext loop = {startAddr, startAddr, {}, {}, false, (int)locals.size(), -1};
			loopStack.push_back(loop);
			bool optimized = false;
			int exitJump = -1;
			if (w->condition->type == ExprType::BINARY) {
				auto bin = static_cast<BinExpr *>(w->condition);
				if (bin->op == TokenType::LT) {
					if (bin->left->type == ExprType::VAR) {
						auto var = static_cast<VarExpr *>(bin->left);
						int localSlot = resolveLocal(var->name);
						if (localSlot != -1 &&
							 bin->right->type == ExprType::NUMBER) {
							auto num = static_cast<NumberExpr *>(bin->right);
							optimized = true;
							emitByte(OpCode::OP_JUMP_IF_NOT_LT, w->line, w->col);
							chunk->write((uint8_t)localSlot, w->line, w->col);
							int constIdx =
								chunk->addConstant(Value::Int((long long)num->val));
							chunk->write((uint8_t)constIdx, w->line, w->col);
							exitJump = (int)chunk->code.size();
							chunk->write(0xff, w->line, w->col);
							chunk->write(0xff, w->line, w->col);
						}
					}
				}
			}
			if (!optimized) {
				compile(w->condition);
				exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE, w->line, w->col);
				emitByte(OpCode::OP_POP, w->line, w->col);
			}
			beginScope();
			if (isCTLE) emitByte(OpCode::OP_CTLE_TICK, w->line, w->col);
			for (auto stmt : w->body)
				compileStmt(stmt);
			endScope(w->line, w->col);
			emitLoop(startAddr, w->line, w->col);
			patchJump(exitJump);
			if (!optimized)
				emitByte(OpCode::OP_POP, w->line, w->col);
			for (int b : loopStack.back().breakJumps)
				patchJump(b);
			loopStack.pop_back();
			endScope(w->line, w->col);
			break;
		}
		case StmtType::DO_WHILE: {
			if (!isCTLE && tryEvaluateCompileTimeLoop(s)) {
				break;
			}
			auto dw = static_cast<DoWhileStmt *>(s);
			int startAddr = (int)chunk->code.size();
			if (isCTLE) emitByte(OpCode::OP_CTLE_TICK, dw->line, dw->col);
			beginScope();
			LoopContext loop = {startAddr, -1, {}, {}, false, (int)locals.size(), -1};
			loopStack.push_back(loop);
			for (auto stmt : dw->body)
				compileStmt(stmt);
			for (int jump : loopStack.back().continueJumps)
				patchJump(jump);
			compile(dw->condition);
			int exitJump =
				emitJump(OpCode::OP_JUMP_IF_FALSE, dw->line, dw->col);
			emitByte(OpCode::OP_POP, dw->line, dw->col);
			emitLoop(startAddr, dw->line, dw->col);
			patchJump(exitJump);
			emitByte(OpCode::OP_POP, dw->line, dw->col);
			for (int b : loopStack.back().breakJumps)
				patchJump(b);
			loopStack.pop_back();
			endScope(dw->line, dw->col);
			break;
		}
		case StmtType::FOR: {
			if (!isCTLE && tryEvaluateCompileTimeLoop(s)) {
				break;
			}
			beginScope();
			auto f = static_cast<ForStmt *>(s);
			for (auto init : f->inits)
				compileStmt(init);
			int condAddr = (int)chunk->code.size();
			int exitJump = -1;
			if (f->condition) {
				compile(f->condition);
				exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE, f->line, f->col);
				emitByte(OpCode::OP_POP, f->line, f->col);
			}
			LoopContext loop = {condAddr, -1, {}, {}, false, (int)locals.size(), -1};
			loopStack.push_back(loop);
			beginScope();
			if (isCTLE) emitByte(OpCode::OP_CTLE_TICK, f->line, f->col);
			for (auto stmt : f->body)
				compileStmt(stmt);
			endScope(f->line, f->col);
			int stepStart = (int)chunk->code.size();
			for (int jump : loopStack.back().continueJumps) {
				patchJump(jump);
			}
			loopStack.back().stepAddress = stepStart;
			for (auto step : f->steps)
				compileStmt(step);
			emitLoop(condAddr, f->line, f->col);
			if (exitJump != -1) {
				patchJump(exitJump);
				emitByte(OpCode::OP_POP, f->line, f->col);
			}
			for (int b : loopStack.back().breakJumps)
				patchJump(b);
			loopStack.pop_back();
			endScope(f->line, f->col);
			break;
		}
		case StmtType::FOR_EACH: {
			auto fe = static_cast<ForEachStmt *>(s);
			beginScope();
			size_t colCount = fe->collections.size();
			size_t varCount = fe->loopVars.size();
			bool isDictUnpack = (colCount == 1 && varCount == 2);
			bool isOneToOne = (colCount == varCount);
			if (!isDictUnpack && !isOneToOne) {
				throw ValueError(
					"Mismatch between loop variables and collections.", fe->line,
					fe->col);
			}
			int streamCount = isDictUnpack ? 2 : (int)colCount;
			if (isDictUnpack) {
				compile(fe->collections[0]);
				emitByte(OpCode::OP_UNPACK_DICT, fe->line, fe->col);
			} else {
				for (auto col : fe->collections) {
					compile(col);
					emitByte(OpCode::OP_TO_STREAM, fe->line, fe->col);
				}
			}
			for (int i = 0; i < streamCount; i++)
				addLocal("");
			emitConstant(Value::Int(0), fe->line, fe->col);
			addLocal("");
			int startAddr = (int)chunk->code.size();
			emitByte(OpCode::OP_FOR_ITER, fe->line, fe->col);
			int exitJump = (int)chunk->code.size();
			chunk->write(0xff, fe->line, fe->col);
			chunk->write(0xff, fe->line, fe->col);
			chunk->write((uint8_t)streamCount, fe->line, fe->col);
			beginScope();
			int iterSlot = (int)locals.size() - 1;
			if (scopeDepth > 0) {
				for (size_t i = 0; i < fe->loopVars.size(); i++)
					addLocal(fe->loopVars[i]);
			} else {
				for (int i = (int)fe->loopVars.size() - 1; i >= 0; i--) {
					emitIdentifier(OpCode::OP_DEFINE_VAR, fe->loopVars[i],
						fe->line, fe->col);
					chunk->write(0, fe->line, fe->col);
				}
			}
			loopStack.push_back({startAddr,
				startAddr,
				{},
				{},
				true,
				(int)locals.size() - streamCount,
				iterSlot});
			beginScope();
			for (auto stmt : fe->body)
				compileStmt(stmt);
			endScope(fe->line, fe->col);
			for (int i = 0; i < streamCount; i++) {
				emitByte(OpCode::OP_POP, fe->line, fe->col);
			}
			for (int jump : loopStack.back().continueJumps)
				patchJump(jump);
			emitLoop(startAddr, fe->line, fe->col);
			patchJump(exitJump);
			scopeDepth--;
			while (locals.size() > 0 && locals.back().depth > scopeDepth)
				locals.pop_back();
			emitByte(OpCode::OP_POP, fe->line, fe->col);
			if (locals.size() > 0)
				locals.pop_back();
			for (int i = 0; i < streamCount; i++) {
				emitByte(OpCode::OP_POP, fe->line, fe->col);
				if (locals.size() > 0)
					locals.pop_back();
			}
			for (int b : loopStack.back().breakJumps)
				patchJump(b);
			loopStack.pop_back();
			scopeDepth--;
			break;
		}
		case StmtType::BREAK: {
			if (loopStack.empty())
				throw ControlFlowError("break outside of loop", s->line, s->col);
			int localsToPop =
				(int)locals.size() - loopStack.back().startLocalCount;
			for (int i = 0; i < localsToPop; i++)
				emitByte(OpCode::OP_POP, s->line, s->col);
			loopStack.back().breakJumps.push_back(
				emitJump(OpCode::OP_JUMP, s->line, s->col));
			break;
		}
		case StmtType::CONTINUE: {
			if (loopStack.empty())
				throw ControlFlowError("continue outside of loop", s->line,
					s->col);
			int localsToPop =
				(int)locals.size() - loopStack.back().startLocalCount;
			for (int i = 0; i < localsToPop; i++)
				emitByte(OpCode::OP_POP, s->line, s->col);
			int target = loopStack.back().stepAddress;
			if (target == -1) {
				int jump = emitJump(OpCode::OP_JUMP, s->line, s->col);
				loopStack.back().continueJumps.push_back(jump);
			} else
				emitLoop(target, s->line, s->col);
			break;
		}
		case StmtType::SKIP: {
			auto sk = static_cast<SkipStmt *>(s);
			if (loopStack.empty())
				throw ControlFlowError("skip statement outside of loop", s->line,
					s->col);
			if (!loopStack.back().isForEach) {
				throw ControlFlowError(
					"skip statement is only valid in for-each loops.", s->line,
					s->col);
			}
			compile(sk->count);
			emitByte(OpCode::OP_SKIP_ITER, s->line, s->col);
			int slot = loopStack.back().iteratorSlot;
			chunk->write((uint8_t)slot, s->line, s->col);
			break;
		}
		default:
			break;
		}
	}


bool ByteCodeCompiler::tryEvaluateCompileTimeLoop(Stmt *s) {
	try {
		Chunk tempChunk;
		ByteCodeCompiler tempComp(&tempChunk);
		tempComp.isCTLE = true;
		tempComp.isCompileOnly = this->isCompileOnly;
		tempComp.interp = this->interp;
		tempComp.locals = this->locals; // pass current locals state
		tempComp.knownGlobals = this->knownGlobals;
		
		// 1. Prepend stack allocation (OP_NONE x locals.size)
		for (size_t i = 0; i < this->locals.size(); i++) {
			tempChunk.write(OpCode::OP_NONE, 0, 0);
		}
		
		// 2. Initialize the local variables
		for (size_t i = 0; i < this->locals.size(); i++) {
			if (this->locals[i].hasKnownValue) {
				int constIdx = tempChunk.addConstant(this->locals[i].knownValue);
				tempChunk.write(OpCode::OP_CONSTANT, 0, 0);
				tempChunk.write((uint8_t)constIdx, 0, 0);
			} else {
				tempChunk.write(OpCode::OP_NOTYPE, 0, 0);
			}
			tempChunk.write(OpCode::OP_SET_LOCAL, 0, 0);
			tempChunk.write((uint8_t)i, 0, 0);
			tempChunk.write(OpCode::OP_POP, 0, 0);
		}
		
		// 3. Compile the loop body (and inject OP_CTLE_TICK at the start of loop)
		tempComp.compileStmt(s);
		
		// 4. Prepend the extractor (returns a List containing all modified locals)
		for (size_t i = 0; i < this->locals.size(); i++) {
			tempChunk.write(OpCode::OP_GET_LOCAL, 0, 0);
			tempChunk.write((uint8_t)i, 0, 0);
		}
		tempChunk.write(OpCode::OP_BUILD_LIST, 0, 0);
		tempChunk.write((uint8_t)this->locals.size(), 0, 0); 
		tempChunk.write(OpCode::OP_RETURN, 0, 0);
		
		// 5. Run sandbox
		VM tempVM;
		try {
			printf("Running tempVM...\n"); tempVM.run(tempChunk); printf("tempVM finished\n");
			if (tempVM.stack.empty()) { return false; }
			Value res = tempVM.stack.back();
			if (res.type != ValueType::LIST) { return false; }
			
			auto *resList = static_cast<ListObject*>(res.ref.get());
			if (!resList) { return false; }
			
			// If we got here, loop finished successfully!
			// We emit the new constant values directly into our REAL chunk!
			for (size_t i = 0; i < resList->elements.size(); i++) {
				Value finalVal = resList->elements[i];
				this->locals[i].hasKnownValue = true;
				this->locals[i].knownValue = finalVal;
				
				int cIdx = this->chunk->addConstant(finalVal);
				this->chunk->write(OpCode::OP_CONSTANT, s->line, s->col);
				this->chunk->write((uint8_t)cIdx, s->line, s->col);
				this->chunk->write(OpCode::OP_SET_LOCAL, s->line, s->col);
				this->chunk->write((uint8_t)i, s->line, s->col);
				this->chunk->write(OpCode::OP_POP, s->line, s->col);
			}
			return true; // We successfully evaluated and erased the loop!
		} catch (CTLEAbort&) {
			return false;
		} catch (const std::exception& e) {
			return false;
		}
	} catch (CTLEAbort&) {
		// Unknown variables or restricted ops
		return false;
	}
}


bool ByteCodeCompiler::tryExtractConstant(Expr *e, Value &outVal) {
	try {
		std::function<Value(Expr *)> extract = [&](Expr *expr) -> Value {
			if (expr->type == ExprType::NUMBER) {
				auto n = static_cast<NumberExpr *>(expr);
				return n->isFloat ? Value::Float(n->val) : Value::Int((long long)n->val);
			}
			if (expr->type == ExprType::STRING)
				return Value::String(static_cast<StringExpr *>(expr)->val);
			if (expr->type == ExprType::BOOL)
				return Value::Bool(static_cast<BoolExpr *>(expr)->value);
			if (expr->type == ExprType::VECTOR) {
				auto ve = static_cast<VectorExpr *>(expr);
				std::vector<Value> elems;
				for (auto *el : ve->elements)
					elems.push_back(extract(el));
				return Value::Vector(elems);
			}
			if (expr->type == ExprType::BINARY) {
				auto bin = static_cast<BinExpr *>(expr);
				return EvaluateConstBinary(bin->op, extract(bin->left), extract(bin->right));
			}
			if (expr->type == ExprType::VAR) {
				auto v = static_cast<VarExpr *>(expr);
				int arg = resolveLocal(v->name);
				if (arg != -1 && locals[arg].hasKnownValue) {
					return locals[arg].knownValue; // INJECT THE VALUE!
				}
				else if (knownGlobals.count(v->name)) return knownGlobals[v->name];

			}
			if (expr->type == ExprType::CALL) {
				auto c = static_cast<CallExpr *>(expr);
				if (knownGlobals.count(c->name) && knownGlobals[c->name].type == ValueType::FUNCTION) {
					std::vector<Value> argVals;
					for (auto arg : c->args) {
						argVals.push_back(extract(arg));
					}
					
					Chunk tempChunk;
					ByteCodeCompiler tempComp(&tempChunk);
					tempComp.isCTLE = true;
					tempComp.knownGlobals = knownGlobals;
					
					int funcIdx = tempChunk.addConstant(knownGlobals[c->name]);
					tempChunk.write(OpCode::OP_CONSTANT, c->line, c->col);
					tempChunk.write((uint8_t)funcIdx, c->line, c->col);
					
					for (auto v : argVals) {
						int argIdx = tempChunk.addConstant(v);
						tempChunk.write(OpCode::OP_CONSTANT, c->line, c->col);
						tempChunk.write((uint8_t)argIdx, c->line, c->col);
					}
					
					tempChunk.write(OpCode::OP_CALL, c->line, c->col);
					tempChunk.write((uint8_t)argVals.size(), c->line, c->col);
					tempChunk.write(OpCode::OP_RETURN, c->line, c->col);
					
					VM tempVM;
					tempVM.globals = std::make_shared<Env>();
					for (auto& pair : knownGlobals) {
						tempVM.globals->set(pair.first, pair.second, false, true);
					}
					printf("Running tempVM...\n"); tempVM.run(tempChunk); printf("tempVM finished\n");
					if (!tempVM.stack.empty()) {
						return tempVM.stack.back();
					}
				}
			}
			throw std::runtime_error("Not a constant");
		};
		outVal = extract(e);
		return true;
	} catch (...) {
		return false;
	}
}

