#include "ylang/old_copy.hpp"
#include "ylang/pystring.h"
#include "ylang/serialization.hpp"
#include "ylang/assembler.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

void printErrorContext(const std::string &, int, int, const std::filesystem::path &);
void ymm_highlighter(std::string const &input, replxx::Replxx::colors_t &colors);
void runREPL();
struct ASTCleaner {
	std::vector<Stmt *> &program;
	~ASTCleaner() {
		for (Stmt *stmt : program) {
			delete stmt;
		}
		program.clear();
	}
};

int main(int argc, char *argv[]) {
	if (argc == 1) {
		runREPL();
		return 0;
	}

	bool compileOnly = false;
	bool humanReadable = false;
	std::vector<std::string> scriptArgs;

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "-c" || arg == "--compile") {
			compileOnly = true;
		} else if (arg == "-h" || arg == "--humanreadable") {
			humanReadable = true;
		} else {
			scriptArgs.push_back(arg);
		}
	}

	if (scriptArgs.empty()) {
		std::cerr << "FATAL ERROR: No input file provided.\n";
		std::cerr << "Usage: yrun [flags] <file.ymm> [args...]\n";
		return 1;
	}

	std::string path = scriptArgs[0];
	std::filesystem::path p(path);
	std::string ext = p.extension().string();

	if (ext != ".ymm" && ext != ".ymc" && ext != ".yb") {
		std::string filename = p.filename().string();
		std::cerr << "FATAL ERROR: " << filename << " must end with .ymm, .ymc, or .yb extension.\n";
		std::cerr << "Interpretation aborted...\n";
		return 1;
	}

	std::ifstream file(path, std::ios::binary);
	if (!file) {
		std::cerr << "FATAL ERROR: Cannot open file at path:" << path << "\n";
		return 1;
	}

	DEBUGGER_MODE_IS_ENABLED = false;
	enableColors();
	std::chrono::steady_clock::time_point start;
	std::chrono::steady_clock::time_point end;

    if (ext == ".ymc") {
        Chunk* chunk = loadChunk(file);
        VM vm;
        Interpreter interp;
        vm.globals = interp.env;
        vm.methodResolver = [&](MethodCallExpr *m) { return interp.Resolve_methods(m); };
        vm.importResolver = [&](std::string libName, std::vector<std::string> symbols, std::shared_ptr<Env> targetEnv) {
            if (interp.modules.find(libName) != interp.modules.end()) interp.modules[libName](targetEnv, symbols);
            else throw ImportError("Unknown module '" + libName + "'", 0, 0);
        };
        start = std::chrono::steady_clock::now();
        vm.run(*chunk);
        
        if (vm.globals->exists("main")) {
            Value mainVal = vm.globals->get("main");
            if (mainVal.type == ValueType::FUNCTION) {
                Chunk bootChunk;
                ByteCodeCompiler bootCompiler(&bootChunk);
                int passedArgs = 0;
                for (size_t i = 1; i < scriptArgs.size(); i++) {
                    bootCompiler.emitConstant(Value::String(scriptArgs[i]), 0, 0);
                    passedArgs++;
                }
                bootCompiler.emitIdentifier(OpCode::OP_GET_VAR, "main", 0, 0);
                bootCompiler.emitByte(OpCode::OP_CALL, 0, 0);
                bootCompiler.chunk->write((uint8_t)passedArgs, 0, 0);
                bootCompiler.emitByte(OpCode::OP_POP, 0, 0);
                bootCompiler.emitByte(OpCode::OP_NOTYPE, 0, 0);
                bootCompiler.emitByte(OpCode::OP_RETURN, 0, 0);
                vm.run(bootChunk);
            }
        }
        end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Program finished successfully in " << elapsed.count() << " seconds with code 0\n";
        delete chunk;
        return 0;
    } else if (ext == ".yb") {
        Chunk* chunk = loadAssembly(file);
        VM vm;
        Interpreter interp;
        vm.globals = interp.env;
        vm.methodResolver = [&](MethodCallExpr *m) { return interp.Resolve_methods(m); };
        vm.importResolver = [&](std::string libName, std::vector<std::string> symbols, std::shared_ptr<Env> targetEnv) {
            if (interp.modules.find(libName) != interp.modules.end()) interp.modules[libName](targetEnv, symbols);
            else throw ImportError("Unknown module '" + libName + "'", 0, 0);
        };
        start = std::chrono::steady_clock::now();
        vm.run(*chunk);
        
        if (vm.globals->exists("main")) {
            Value mainVal = vm.globals->get("main");
            if (mainVal.type == ValueType::FUNCTION) {
                Chunk bootChunk;
                ByteCodeCompiler bootCompiler(&bootChunk);
                int passedArgs = 0;
                for (size_t i = 1; i < scriptArgs.size(); i++) {
                    bootCompiler.emitConstant(Value::String(scriptArgs[i]), 0, 0);
                    passedArgs++;
                }
                bootCompiler.emitIdentifier(OpCode::OP_GET_VAR, "main", 0, 0);
                bootCompiler.emitByte(OpCode::OP_CALL, 0, 0);
                bootCompiler.chunk->write((uint8_t)passedArgs, 0, 0);
                bootCompiler.emitByte(OpCode::OP_POP, 0, 0);
                bootCompiler.emitByte(OpCode::OP_NOTYPE, 0, 0);
                bootCompiler.emitByte(OpCode::OP_RETURN, 0, 0);
                vm.run(bootChunk);
            }
        }
        end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Program finished successfully in " << elapsed.count() << " seconds with code 0\n";
        delete chunk;
        return 0;
    }

	std::stringstream ss;
	ss << file.rdbuf();
	std::string code = ss.str();

	try {
		auto tokens = tokenize(code);
		Parser parser(tokens);
		vector<Stmt *> program;
		ASTCleaner cleanup_guard{program};
		bool hadError = false;
		while (!parser.isAtEnd()) {
			try {
				program.push_back(parser.parseStmt());
			} catch (const LangError &e) {
				hadError = true;
				printErrorContext(code, e.line, e.col, p);
				std::cerr << "\033[1;31m" << e.type << ": " << e.message << "\033[0m\n\n";
				parser.synchronize();
			}
		}
		if (hadError) {
			std::cerr << "\033[1;31mCompilation aborted due to syntax errors.\033[0m\n";
			return 1;
		}
		int useVM;
#ifdef VM_DEBUG_MODE
		cout << "Choose Compiler: AST:0 VM:1 ";
		std::cout << "DEBUG_VM:2 ";
		cin >> useVM;
#else
		useVM = 1;
#endif
		Interpreter interp;
		start = std::chrono::steady_clock::now();
		if (!useVM) {
			interp.interpret(program);
			end = std::chrono::steady_clock::now();
			std::cout << "Program finished successfully with code 0";
		} else {
#ifndef VM_DEBUG_MODE
			if (useVM == 2) {
				std::cout << "\n[Notice: Fast Release Build active. Debugging disabled. Running standard VM]\n";
				useVM = 1;
			}
#endif
			if (useVM == 2) {
				DEBUGGER_MODE_IS_ENABLED = true;
			}
			Chunk chunk;
			ByteCodeCompiler compiler(&chunk);
			if (compileOnly) {
				compiler.isCompileOnly = true;
				compiler.interp = &interp;
			}
			for (Stmt *stmt : program) {
				compiler.compileStmt(stmt);
			}
			int line = program.empty() ? 0 : program.back()->line;
			int col = program.empty() ? 0 : program.back()->col;
			compiler.emitByte(OpCode::OP_NOTYPE, line, col);
			compiler.emitByte(OpCode::OP_RETURN, line, col);
			VM vm;
			vm.globals = interp.env;
			vm.methodResolver = [&](MethodCallExpr *m) {
				return interp.Resolve_methods(m);
			};
			vm.importResolver = [&](std::string libName, std::vector<std::string> symbols, std::shared_ptr<Env> targetEnv) {
				if (interp.modules.find(libName) != interp.modules.end())
					interp.modules[libName](targetEnv, symbols);
				else
					throw ImportError("Unknown module '" + libName + "'", 0, 0);
			};
			
			if (compileOnly) {
				if (!humanReadable) {
					std::string outPath = p.stem().string() + ".ymc";
					std::ofstream outFile(outPath, std::ios::binary);
					saveChunk(&chunk, outFile);
					std::cout << "Successfully compiled to " << outPath << "\n";
				} else {
					std::string outPath = p.stem().string() + ".yb";
					std::ofstream outFile(outPath);
					saveAssembly(&chunk, outFile);
					std::cout << "Successfully compiled to " << outPath << "\n";
				}
				return 0;
			}
			vm.run(chunk);
			if (vm.globals->exists("main")) {
				Value mainVal = vm.globals->get("main");
				if (mainVal.type == ValueType::FUNCTION) {
					Chunk bootChunk;
					ByteCodeCompiler bootCompiler(&bootChunk);
					int passedArgs = 0;
					for (size_t i = 1; i < scriptArgs.size(); i++) {
						bootCompiler.emitConstant(Value::String(scriptArgs[i]), 0, 0);
						passedArgs++;
					}
					bootCompiler.emitIdentifier(OpCode::OP_GET_VAR, "main", 0, 0);
					bootCompiler.emitByte(OpCode::OP_CALL, 0, 0);
					bootCompiler.chunk->write((uint8_t)passedArgs, 0, 0);
					bootCompiler.emitByte(OpCode::OP_POP, 0, 0);
					bootCompiler.emitByte(OpCode::OP_NOTYPE, 0, 0);
					bootCompiler.emitByte(OpCode::OP_RETURN, 0, 0);
					vm.run(bootChunk);
				}
				end = std::chrono::steady_clock::now();
				std::chrono::duration<double> elapsed = end - start;
				std::cout << "Program finished successfully in " << elapsed.count() << " seconds " << "with code 0\n";
				vm.globals->clear();
			}

			if (!vm.stack.empty()) {
				vm.stack.clear();
				// for debugging only!
				// std::cout << "\n--- Final Stack Top ---\n";
				// std::cout << valueToString(vm.stack.back()) << "\n";
			}
		}
	} catch (const LangError &e) {
		printErrorContext(code, e.line, e.col, p);
		std::cerr << "\033[1;31m" << e.type << ": " << e.message << "\033[0m\n";
		std::cerr << "Program crashed with exit code " << e.code;
	} catch (const std::exception &e) {
		std::cerr << "Internal C++ Error: " << e.what() << "\n";
	}
	return 0;
}
void printErrorContext(const string &source, int line, int col, const std::filesystem::path &p) {
	if (line <= 0)
		return;
	std::stringstream ss(source);
	string currentLineText;
	int currentLineNum = 1;
	while (std::getline(ss, currentLineText)) {
		if (currentLineNum == line) {
			if (!currentLineText.empty() && currentLineText.back() == '\r')
				currentLineText.pop_back();
			std::cerr << "Traceback (most recent call last):";
			std::cerr << " File " << p.filename().string() << ", [line: " << line << " | col: " << col << "]:" << "\n";
			std::cerr << ::strip(currentLineText, "\t") << "\n";
			for (int i = 0; i < col; i++) {
				if (i < currentLineText.size() && currentLineText[i] == '\t')
					continue;
				else
					std::cerr << " ";
			}
			std::cerr << "\033[1;31m^\033[0m\n";
			break;
		}
		currentLineNum++;
	}
}
void runREPL() {
   std::cout << "y-- 1.0.0 Interactive Shell\n";
   std::cout << "Type 'exit' to quit.\n";
   std::cout << "Type 'ts' to toggle output supression.\n";
   std::cout << "Type 'clear' to clear the screen.\n";
   VM vm;
   vm.printOutput = true;
   Interpreter interp;
   vm.globals = interp.env;
   vm.methodResolver = [&](MethodCallExpr *m) { return interp.Resolve_methods(m); };
   vm.importResolver = [&](std::string libName, std::vector<std::string> symbols, std::shared_ptr<Env> targetEnv) {
      if (interp.modules.find(libName) != interp.modules.end())
         interp.modules[libName](targetEnv, symbols);
      else
         throw ImportError("Unknown module '" + libName + "'", 0, 0);
   };
   replxx::Replxx rx;
   rx.install_window_change_handler();
   rx.set_highlighter_callback(ymm_highlighter);
   std::string codeBuffer = "";
   int openBraces = 0;
   int openParens = 0;
   int openBrackets = 0;
   while (true) {
		std::string prompt = codeBuffer.empty() ? "\033[38;2;100;180;255m>>>\033[38;2;0m " : "\033[38;2;100;180;255m...\033[38;2;0m ";
      char const* c_line = rx.input(prompt);
      if (!c_line) break;
      std::string line(c_line);
      if (codeBuffer.empty()) {
         if (line == "exit" || line == "quit") {
            std::cout << "Exiting the interactive shell...\n";
            break;
         }
         if (line == "ts"){
            vm.printOutput = !vm.printOutput;
            std::cout << (vm.printOutput ? "output supression disabled" : "output supression enabled") << "\n";
            rx.history_add(line);
            continue;
         }
         if (line == "clear") {
            rx.clear_screen();
            rx.history_add(line);
            continue;
         }
         if (line.empty()) continue;
      }
      if (!line.empty()) {
         rx.history_add(line);
      }
      codeBuffer += line + "\n";
      for (char c : line) {
         if (c == '{') openBraces++;
         if (c == '}') openBraces--;
         if (c == '(') openParens++;
         if (c == ')') openParens--;
         if (c == '[') openBrackets++;
         if (c == ']') openBrackets--;
      }
      if (openBraces > 0 || openParens > 0 || openBrackets > 0) {
         continue;
      }
      try {
         auto tokens = tokenize(codeBuffer);
         Parser parser(tokens);
         parser.isReplMode = true;
         vector<Stmt *> program;
         ASTCleaner cleanup_guard{program};
         while (!parser.isAtEnd()) {
            program.push_back(parser.parseStmt());
         }
         Chunk chunk;
         ByteCodeCompiler compiler(&chunk);
         compiler.isReplMode = true;
         for (Stmt *stmt : program) {
            compiler.compileStmt(stmt);
         }
         compiler.emitByte(OpCode::OP_NOTYPE, 0, 0);
         compiler.emitByte(OpCode::OP_RETURN, 0, 0);
         vm.run(chunk);
         vm.stack.clear();
         vm.frames.clear();
      } catch (const LangError &e) {
         std::cerr << "\033[1;31m" << e.type << ": " << e.message << "\033[0m\n";
         vm.stack.clear();
         vm.frames.clear();
      } catch (const std::exception &e) {
         std::cerr << "Internal C++ Error: " << e.what() << "\n";
         vm.stack.clear();
         vm.frames.clear();
      }
      codeBuffer = "";
      openBraces = 0;
      openParens = 0;
      openBrackets = 0;
   }
}
void ymm_highlighter(std::string const &input, replxx::Replxx::colors_t &colors) {
	try {
		auto tokens = tokenize(input);
		size_t charIndex = 0;
		for (const auto &token : tokens) {
			replxx::Replxx::Color c = replxx::Replxx::Color::DEFAULT;
			switch (token.type) {
			case TokenType::LET:
			case TokenType::DEFINE:
			case TokenType::FUNCTION:
			case TokenType::RETURN:
			case TokenType::IF:
			case TokenType::ELSE_IF:
			case TokenType::ELSE:
			case TokenType::FOR:
			case TokenType::WHILE:
			case TokenType::DO:
			case TokenType::THEN:
			case TokenType::BREAK:
			case TokenType::CONTINUE:
			case TokenType::SKIP:
			case TokenType::DELETE:
			case TokenType::CONST:
			case TokenType::ASSERT:
			case TokenType::SWITCH:
			case TokenType::CASE:
			case TokenType::DEFAULT:
			case TokenType::TRY:
			case TokenType::THROW:
			case TokenType::CATCH:
			case TokenType::FINALLY:
			case TokenType::IMPORT:
			case TokenType::FROM:
			case TokenType::LAMBDA:
			case TokenType::OMIT:
			case TokenType::CLASS:
			case TokenType::INHERITS:
			case TokenType::PUBLIC:
			case TokenType::PRIVATE:
			case TokenType::PROTECTED:
			case TokenType::CACHED:
			case TokenType::AND:
			case TokenType::OR:
			case TokenType::NOT:
			case TokenType::XOR:
			case TokenType::NAND:
			case TokenType::NOR:
			case TokenType::NXOR:
			case TokenType::IS:
			case TokenType::IN:
			case TokenType::IS_IN:
			case TokenType::IS_NOT:
			case TokenType::IS_NOT_IN:
				c = replxx::Replxx::Color::MAGENTA;
				break;
			case TokenType::TRUE:
			case TokenType::FALSE:
			case TokenType::NUMBER:
				c = replxx::Replxx::Color::YELLOW;
				break;
			case TokenType::STRING:
			case TokenType::FSTRING:
				c = replxx::Replxx::Color::GREEN;
				break;
			case TokenType::ASSIGN:
			case TokenType::PLUS:
			case TokenType::MINUS:
			case TokenType::STAR:
			case TokenType::SLASH:
			case TokenType::PLUS_EQ:
			case TokenType::MINUS_EQ:
			case TokenType::STAR_EQ:
			case TokenType::DIV_EQ:
			case TokenType::FLOOR_DIV:
			case TokenType::FLOOR_DIV_EQ:
			case TokenType::MOD:
			case TokenType::MOD_EQ:
			case TokenType::POW:
			case TokenType::POW_EQ:
			case TokenType::INCREMENT:
			case TokenType::DECREMENT:
			case TokenType::ARROW:
			case TokenType::EQ:
			case TokenType::STRICT_EQ:
			case TokenType::NEQ:
			case TokenType::STRICT_NEQ:
			case TokenType::GT:
			case TokenType::LT:
			case TokenType::GTE:
			case TokenType::LTE:
			case TokenType::AND_EQ:
			case TokenType::OR_EQ:
			case TokenType::XOR_EQ:
			case TokenType::COLON_EQ:
				c = replxx::Replxx::Color::CYAN;
				break;
			case TokenType::IDENTIFIER: {
				const string &lex = token.value;
				if (lex == "int" || lex == "float" || lex == "string" || lex == "bool" || lex == "list" || lex == "set" || lex == "dict" || lex == "tuple" || lex == "vector" || lex == "range") {
					c = replxx::Replxx::Color::CYAN;
				}
				else if (!lex.empty() && std::isupper(lex[0])) {
					c = replxx::Replxx::Color::YELLOW;
				}
				break;
			}
			default:
				c = replxx::Replxx::Color::DEFAULT;
				break;
			}
			size_t pos = input.find(token.value, charIndex);
			if (pos != std::string::npos) {
				charIndex = pos;
				for (size_t i = 0; i < token.value.length(); i++) {
					if (charIndex < colors.size()) {
						colors[charIndex] = c;
					}
					charIndex++;
				}
			}
		}
	} catch (...) {
	}
}