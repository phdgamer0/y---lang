#include "ymm.h"
#include "pystring.h"
#include <fstream>
#include <sstream>
#include <iostream>

int main() {
	string path = "C:\\Users\\Windows\\source\\repos\\y--\\test.ymm";
	std::ifstream file(path);
	if (!file) {
		std::cerr << "FATAL ERROR: Cannot open file at path:" << path << "\n";
		std::cerr << "Check if the file exists.";
		std::cout << "Press Enter to exit...";
		return 1;
	}
	std::stringstream ss;
	ss << file.rdbuf();
	string code = ss.str();
	try {
		auto tokens = tokenize(code);
		Parser parser(tokens);
		vector<Stmt*> program;
		while (parser.peek().type != TokenType::END) {
			program.push_back(parser.parseStmt());
		}
		Interpreter interp;
		interp.interpret(program);
		std::cout << "Program finished successfully with code 0";
	}
	catch (const LangError& e) {
		std::cerr << e.what();
		return 1;
	}
	catch (const std::exception& e) {
		std::cerr << "INTERNAL ERROR: " << e.what() << "\n";
		return 1;
	}
	return 0;
}