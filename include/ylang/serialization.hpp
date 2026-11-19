
#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP

#include "ylang/objects.hpp"
#include <iostream>
#include <string>

// Forward declarations
void saveValue(const Value &v, std::ostream &os);
Value loadValue(std::istream &is);
void saveChunk(Chunk *chunk, std::ostream &os);
Chunk *loadChunk(std::istream &is);

void saveString(const std::string &str, std::ostream &os);

std::string loadString(std::istream &is);

void saveChunk(Chunk *chunk, std::ostream &os);

Chunk *loadChunk(std::istream &is);

void saveFunction(FunctionObject *func, std::ostream &os);

FunctionObject *loadFunction(std::istream &is);

void saveValue(const Value &v, std::ostream &os);

Value loadValue(std::istream &is);

#endif // SERIALIZATION_HPP
