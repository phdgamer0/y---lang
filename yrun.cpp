#include "old.h"
#include "pystring.h"
#include <fstream>
#include <sstream>
#include <iostream>
int main() {
	string path = "C:\\Users\\Windows\\source\\repos\\phdgamer0\\y---lang\\test.ymm";
	std::ifstream file(path);
	if (!file) {
		std::cerr << "FATAL ERROR: Cannot open file at path:" << path << "\n";
		return 1;
	}
	std::stringstream ss;
	ss << file.rdbuf();
	string code = ss.str();
	try {
		auto tokens = tokenize(code);
		Parser parser(tokens);
		vector<Stmt*> program;
		while (!parser.isAtEnd()) program.push_back(parser.parseStmt());
		int useVM;
		cout<<"Choose Compiler: AST:0 VM:1 :";
		cin>>useVM;
		Interpreter interp;
		if (!useVM) {
			interp.interpret(program);
			std::cout<<"Program finished successfully with code 0";
		}
		else {
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
				//std::cout << "\n--- Final Stack Top ---\n";
				//std::cout << valueToString(vm.stack.back()) << "\n";
			}
		}
	}
	catch (const LangError& e) {
		std::cerr << e.what() << "\n";
	}
	catch (const std::exception& e) {
		std::cerr << "Internal C++ Error: " << e.what() << "\n";
	}
	return 0;
}