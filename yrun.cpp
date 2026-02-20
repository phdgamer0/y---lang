#include "old.h"
#include "pystring.h"
#include <fstream>
#include <sstream>
#include <iostream>
void printErrorContext(const std::string&, int, int);
int main() {
	/*
	====================================================================================
	|	Change the path to point to your target .ymm file.                              |
	|	Compile & Link: Compile yrun.cpp ensuring Raylib is linked.                     |
	|	g++ yrun.cpp -o y_lang -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 && ./y_lang  |
	|	if you see this: "Choose Compiler: AST:0 VM:1 DEBUG_VM:2 :" ALWAYS CHOOSE 1     |
	====================================================================================
	*/
	// -------------------------------------------------------------------------------
	string path = "C:\\Users\\Windows\\source\\repos\\phdgamer0\\y---lang\\test.ymm"; // <-- CHANGE THIS TO YOUR PATH!
	// -------------------------------------------------------------------------------
	std::ifstream file(path);
	if (!file) {
		std::cerr << "FATAL ERROR: Cannot open file at path:" << path << "\n";
		return 1;
	}
	std::stringstream ss;
	ss << file.rdbuf();
	string code = ss.str();
	DEBUGGER_MODE_IS_ENABLED = false;
	try {
		auto tokens = tokenize(code);
		Parser parser(tokens);
		vector<Stmt*> program;
		while (!parser.isAtEnd()) program.push_back(parser.parseStmt());
		int useVM;
		cout<<"Choose Compiler: AST:0 VM:1 DEBUG_VM:2 :";
		cin>>useVM;
		Interpreter interp;
		if (!useVM) {
			interp.interpret(program);
			std::cout<<"Program finished successfully with code 0";
		}
		else {
			if (useVM == 2) DEBUGGER_MODE_IS_ENABLED = true;
			Chunk chunk;
			ByteCodeCompiler compiler(&chunk);
			for (Stmt* stmt : program) compiler.compileStmt(stmt);
			int line = program.empty() ? 0 : program.back()->line;
			int col = program.empty() ? 0 : program.back()->col;
			compiler.emitByte(OpCode::OP_NOTYPE, line, col);
			compiler.emitByte(OpCode::OP_RETURN, line, col);
			VM vm;
			vm.globals = interp.env;
			vm.methodResolver = [&](MethodCallExpr* m) {
				return interp.Resolve_methods(m);
			};
			vm.importResolver = [&](std::string libName, std::vector<std::string> symbols) {
				if (interp.modules.find(libName) != interp.modules.end()) interp.modules[libName](interp.env, symbols);
				else throw ImportError("Unknown module '" + libName + "'", 0, 0);
			};
			vm.run(chunk);
			if (vm.globals->exists("main")) {
				Value mainVal = vm.globals->get("main");
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
				std::cout << "Program finished successfully with code 0";
			}
			if (!vm.stack.empty()) {
				//for debugging only!
				//std::cout << "\n--- Final Stack Top ---\n";
				//std::cout << valueToString(vm.stack.back()) << "\n";
			}
		}
	}
	catch (const LangError& e) {
		printErrorContext(code, e.line, e.col);
		std::cerr << "\033[1;31m" << e.type << ": " << e.message << "\033[0m\n";
		std::cerr << "Program crashed with exit code " << e.code;
	}
	catch (const std::exception& e) {
		std::cerr << "Internal C++ Error: " << e.what() << "\n";
	}
	return 0;
}
void printErrorContext(const string& source, int line, int col) {
	if (line <= 0) return;
	std::stringstream ss(source);
	string currentLineText;
	int currentLineNum = 1;
	while (std::getline(ss, currentLineText)) {
		if (currentLineNum == line) {
			if (!currentLineText.empty() && currentLineText.back() == '\r') currentLineText.pop_back();
			std::cerr << "Traceback (most recent call last):";
			std::cerr << " File \"test.ymm\", [line: " << line << " | col: " << col << "]:" << "\n";
			std::cerr << ::strip(currentLineText,"\t") << "\n";
			for (int i = 0; i < col; i++) {
				if (i < currentLineText.size() && currentLineText[i] == '\t') continue;
				else std::cerr << " ";
			}
			std::cerr << "\033[1;31m^\033[0m\n";
			break;
		}
		currentLineNum++;
	}
}