
#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include "objects.hpp"
#include <iostream>
#include <sstream>
#include <string>

std::string opToMnemonic(OpCode op);

OpCode mnemonicToOp(const std::string &mn);

std::string escapeString(const std::string &s);

std::string unescapeString(const std::string &s);

struct AssemblyState {
	std::map<FunctionObject *, int> funcToId;
	std::vector<FunctionObject *> funcs;

	int getId(FunctionObject *f);

	void collect(Chunk *c);
};

void saveChunkAssembly(Chunk *chunk, std::ostream &os, AssemblyState &state, std::string indent = "");

void saveAssembly(Chunk *mainChunk, std::ostream &os);

Chunk *loadChunkAssembly(std::istream &is, std::map<int, FunctionObject *> &funcs);

Chunk *loadAssembly(std::istream &is);

#endif // ASSEMBLER_HPP
