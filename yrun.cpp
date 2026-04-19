#include "old.h"
#include "pystring.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
void printErrorContext(const std::string &, int, int, const std::filesystem::path &);
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
	/*
	=====================================================================================================================================
	|	Change the path to point to your target .ymm file.                                  															|
	|	Compile & Link: Compile yrun.cpp ensuring Raylib is linked.                    															      |
	|	g++ -w -O3 -flto yrun.cpp qrcodegen.cpp -o y_lang -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lncurses -lssl -lcrypto -lasmjit  |
	|	if you see this: "Choose Compiler: AST:0 VM:1 DEBUG_VM:2 :" ALWAYS CHOOSE 1         															|
	=====================================================================================================================================
	*/
	// -------------------------------------------------------------------------------
	if (argc < 2) {
		std::cerr << "FATAL ERROR: No input file provided.\n";
		std::cerr << "Usage: " << argv[0] << " <file.ymm>\n";
		return 1;
	}
	// Read the path from the first command-line argument
	std::string path = argv[1];
	std::filesystem::path p(path);
	if (p.extension() != ".ymm") {
		std::string filename = p.filename().string();
		std::cerr << "FATAL ERROR: " << filename << " must end with .ymm extension.\n";
		std::cerr << "Usage: " << p.stem().string() << ".ymm\n";
		std::cerr << "Interpretation aborted...\n";
		return 1;
	}
	// -------------------------------------------------------------------------------
	// string path = "/home/phd/Desktop/CppStuff/Projects/y--lang/y---lang/test.ymm";
	std::ifstream file(path);
	if (!file) {
		std::cerr << "FATAL ERROR: Cannot open file at path:" << path << "\n";
		return 1;
	}
	std::stringstream ss;
	ss << file.rdbuf();
	string code = ss.str();
	DEBUGGER_MODE_IS_ENABLED = false;
	std::chrono::time_point<std::chrono::_V2::system_clock, std::chrono::_V2::system_clock::duration> start;
	std::chrono::time_point<std::chrono::_V2::system_clock, std::chrono::_V2::system_clock::duration> end;
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
#endif
		useVM = 1;
		Interpreter interp;
		start = std::chrono::high_resolution_clock::now();
		if (!useVM) {
			interp.interpret(program);
			end = std::chrono::high_resolution_clock::now();
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
			for (Stmt *stmt : program)
				compiler.compileStmt(stmt);
			int line = program.empty() ? 0 : program.back()->line;
			int col = program.empty() ? 0 : program.back()->col;
			compiler.emitByte(OpCode::OP_NOTYPE, line, col);
			compiler.emitByte(OpCode::OP_RETURN, line, col);
			VM vm;
			vm.globals = interp.env;
			vm.methodResolver = [&](MethodCallExpr *m) {
				return interp.Resolve_methods(m);
			};
			vm.importResolver = [&](std::string libName, std::vector<std::string> symbols) {
				if (interp.modules.find(libName) != interp.modules.end())
					interp.modules[libName](interp.env, symbols);
				else
					throw ImportError("Unknown module '" + libName + "'", 0, 0);
			};
			vm.run(chunk);
			if (vm.globals->exists("main")) {
				Value mainVal = vm.globals->get("main");
				auto *funcObj = static_cast<FunctionObject *>(mainVal.ref.get());
				if (mainVal.type == ValueType::FUNCTION) {
					Chunk bootChunk;
					ByteCodeCompiler bootCompiler(&bootChunk);
					bootCompiler.emitIdentifier(OpCode::OP_GET_VAR, "main", 0, 0);
					bootCompiler.emitByte(OpCode::OP_CALL, 0, 0);
					bootCompiler.chunk->write((uint8_t)0, 0, 0);
					bootCompiler.emitByte(OpCode::OP_POP, 0, 0);
					bootCompiler.emitByte(OpCode::OP_NOTYPE, 0, 0);
					bootCompiler.emitByte(OpCode::OP_RETURN, 0, 0);
					vm.run(bootChunk);
				}
				end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> elapsed = end - start;
				std::cout << "Program finished successfully in " << elapsed.count() << " seconds " << "with code 0";
				vm.globals->clear();
			}
			if (!vm.stack.empty()) {
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