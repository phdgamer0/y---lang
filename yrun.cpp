#include "old.h"
#include "pystring.h"
#include <fstream>
#include <sstream>
#include <iostream>

int main() {
   string path = "C:\\Users\\Windows\\source\\repos\\y--\\test.ymm";
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
      while (!parser.isAtEnd()) {
         program.push_back(parser.parseStmt());
      }

      bool useVM = true; // Toggle this to true to use your new VM!

      if (!useVM) {
         Interpreter interp;
         interp.interpret(program);
         std::cout<<"Program finished successfully with code 0";
      }
      else {
         Chunk chunk;
         ByteCodeCompiler compiler(&chunk);

         // 1. Compile all statements in the program 
         for (Stmt* stmt : program) {
            compiler.compileStmt(stmt);
         }

         // 2. Always end the chunk with a RETURN to stop the VM loop 
         compiler.emitByte(OpCode::OP_RETURN, program.empty() ? 0 : program.back()->line);

         // 3. Run the VM 
         VM vm;
         vm.run(chunk);
         if (vm.globals->exists("main")) {
            Value mainVal = vm.globals->get("main");
            if (mainVal.type == ValueType::FUNCTION) {
               // Get the actual function object from your existing system
               auto* func = static_cast<FunctionObject*>(mainVal.ref.get());

               // 1. Create a fresh chunk for the function body
               Chunk mainChunk;
               ByteCodeCompiler mainCompiler(&mainChunk);

               // 2. Compile every statement inside the 'main' function
               for (Stmt* s : func->body) {
                  mainCompiler.compileStmt(s);
               }

               // 3. Add a return at the end
               mainCompiler.emitByte(OpCode::OP_RETURN, 0);

               // 4. Run the VM on this specific function body
               vm.run(mainChunk);
            }
         }
         // 4. Optional: If the last statement was an expression, show the result
         if (!vm.stack.empty()) {
            std::cout << "\n--- Final Stack Top ---\n";
            std::cout << valueToString(vm.stack.back()) << "\n";
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