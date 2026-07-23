
#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP

#include "ylang/old_copy.hpp"
#include <iostream>
#include <vector>
#include <string>

// Forward declarations
void saveValue(const Value& v, std::ostream& os);
Value loadValue(std::istream& is);
void saveChunk(Chunk* chunk, std::ostream& os);
Chunk* loadChunk(std::istream& is);

inline void saveString(const std::string& str, std::ostream& os) {
    uint32_t len = str.length();
    os.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0) {
        os.write(str.c_str(), len);
    }
}

inline std::string loadString(std::istream& is) {
    uint32_t len = 0;
    is.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len == 0) return "";
    std::string str(len, '\0');
    is.read(&str[0], len);
    return str;
}

inline void saveChunk(Chunk* chunk, std::ostream& os) {
    if (!chunk) {
        uint32_t zero = 0;
        os.write(reinterpret_cast<const char*>(&zero), sizeof(zero)); 
        return;
    }
    uint32_t codeSize = chunk->code.size();
    os.write(reinterpret_cast<const char*>(&codeSize), sizeof(codeSize));
    if (codeSize > 0) os.write(reinterpret_cast<const char*>(chunk->code.data()), codeSize);

    uint32_t constSize = chunk->constants.size();
    os.write(reinterpret_cast<const char*>(&constSize), sizeof(constSize));
    for (const Value& v : chunk->constants) {
        saveValue(v, os);
    }

    uint32_t linesSize = chunk->lines.size();
    os.write(reinterpret_cast<const char*>(&linesSize), sizeof(linesSize));
    if (linesSize > 0) os.write(reinterpret_cast<const char*>(chunk->lines.data()), linesSize * sizeof(int));

    uint32_t colsSize = chunk->columns.size();
    os.write(reinterpret_cast<const char*>(&colsSize), sizeof(colsSize));
    if (colsSize > 0) os.write(reinterpret_cast<const char*>(chunk->columns.data()), colsSize * sizeof(int));
}

inline Chunk* loadChunk(std::istream& is) {
    uint32_t codeSize = 0;
    is.read(reinterpret_cast<char*>(&codeSize), sizeof(codeSize));
    
    Chunk* chunk = new Chunk();
    if (codeSize > 0) {
        chunk->code.resize(codeSize);
        is.read(reinterpret_cast<char*>(chunk->code.data()), codeSize);
    }

    uint32_t constSize = 0;
    is.read(reinterpret_cast<char*>(&constSize), sizeof(constSize));
    for (uint32_t i = 0; i < constSize; ++i) {
        chunk->constants.push_back(loadValue(is));
    }

    uint32_t linesSize = 0;
    is.read(reinterpret_cast<char*>(&linesSize), sizeof(linesSize));
    if (linesSize > 0) {
        chunk->lines.resize(linesSize);
        is.read(reinterpret_cast<char*>(chunk->lines.data()), linesSize * sizeof(int));
    }

    uint32_t colsSize = 0;
    is.read(reinterpret_cast<char*>(&colsSize), sizeof(colsSize));
    if (colsSize > 0) {
        chunk->columns.resize(colsSize);
        is.read(reinterpret_cast<char*>(chunk->columns.data()), colsSize * sizeof(int));
    }
    return chunk;
}

inline void saveFunction(FunctionObject* func, std::ostream& os) {
    uint32_t numParams = func->params.size();
    os.write(reinterpret_cast<const char*>(&numParams), sizeof(numParams));
    for (auto& p : func->params) {
        saveString(p.name, os);
        os.write(reinterpret_cast<const char*>(&p.mode), sizeof(p.mode));
        os.write(reinterpret_cast<const char*>(&p.type), sizeof(p.type));
        os.write(reinterpret_cast<const char*>(&p.isConst), sizeof(p.isConst));
        os.write(reinterpret_cast<const char*>(&p.isVariadic), sizeof(p.isVariadic));
        os.write(reinterpret_cast<const char*>(&p.isKwargs), sizeof(p.isKwargs));
        bool hasDefault = p.defaultChunk != nullptr;
        os.write(reinterpret_cast<const char*>(&hasDefault), sizeof(hasDefault));
        if (hasDefault) saveChunk(p.defaultChunk, os);
    }
    os.write(reinterpret_cast<const char*>(&func->returnType), sizeof(func->returnType));
    
    uint32_t numRetChunks = func->defaultRetChunks.size();
    os.write(reinterpret_cast<const char*>(&numRetChunks), sizeof(numRetChunks));
    for (auto* rc : func->defaultRetChunks) {
        saveChunk(rc, os);
    }

    os.write(reinterpret_cast<const char*>(&func->returnsConst), sizeof(func->returnsConst));
    os.write(reinterpret_cast<const char*>(&func->isCached), sizeof(func->isCached));
    saveString(func->name, os);
    
    bool hasChunk = func->chunk != nullptr;
    os.write(reinterpret_cast<const char*>(&hasChunk), sizeof(hasChunk));
    if (hasChunk) saveChunk(func->chunk, os);
}

inline FunctionObject* loadFunction(std::istream& is) {
    uint32_t numParams = 0;
    is.read(reinterpret_cast<char*>(&numParams), sizeof(numParams));
    std::vector<ParamSpec> params;
    for (uint32_t i = 0; i < numParams; ++i) {
        ParamSpec p;
        p.name = loadString(is);
        is.read(reinterpret_cast<char*>(&p.mode), sizeof(p.mode));
        is.read(reinterpret_cast<char*>(&p.type), sizeof(p.type));
        is.read(reinterpret_cast<char*>(&p.isConst), sizeof(p.isConst));
        is.read(reinterpret_cast<char*>(&p.isVariadic), sizeof(p.isVariadic));
        is.read(reinterpret_cast<char*>(&p.isKwargs), sizeof(p.isKwargs));
        bool hasDefault = false;
        is.read(reinterpret_cast<char*>(&hasDefault), sizeof(hasDefault));
        if (hasDefault) p.defaultChunk = loadChunk(is);
        else p.defaultChunk = nullptr;
        p.defaultValue = nullptr;
        params.push_back(p);
    }

    ValueType returnType;
    is.read(reinterpret_cast<char*>(&returnType), sizeof(returnType));

    uint32_t numRetChunks = 0;
    is.read(reinterpret_cast<char*>(&numRetChunks), sizeof(numRetChunks));
    std::vector<Chunk*> defaultRetChunks;
    for (uint32_t i = 0; i < numRetChunks; ++i) {
        defaultRetChunks.push_back(loadChunk(is));
    }

    bool returnsConst;
    is.read(reinterpret_cast<char*>(&returnsConst), sizeof(returnsConst));
    bool isCached;
    is.read(reinterpret_cast<char*>(&isCached), sizeof(isCached));
    std::string name = loadString(is);

    bool hasChunk = false;
    is.read(reinterpret_cast<char*>(&hasChunk), sizeof(hasChunk));
    Chunk* chunk = nullptr;
    if (hasChunk) chunk = loadChunk(is);

    FunctionObject* func = new FunctionObject(params, returnType, std::vector<Expr*>(), returnsConst, std::vector<Stmt*>(), nullptr, isCached, chunk);
    func->defaultRetChunks = defaultRetChunks;
    func->name = name;
    return func;
}

inline void saveValue(const Value& v, std::ostream& os) {
    uint8_t type = static_cast<uint8_t>(v.type);
    os.write(reinterpret_cast<const char*>(&type), sizeof(type));
    os.write(reinterpret_cast<const char*>(&v.isConst), sizeof(v.isConst));

    switch (v.type) {
        case ValueType::INT:
            os.write(reinterpret_cast<const char*>(&v.iVal), sizeof(v.iVal));
            break;
        case ValueType::FLOAT:
            os.write(reinterpret_cast<const char*>(&v.fVal), sizeof(v.fVal));
            break;
        case ValueType::BOOL:
            os.write(reinterpret_cast<const char*>(&v.bVal), sizeof(v.bVal));
            break;
        case ValueType::STRING:
            saveString(v.asString(), os);
            break;
        case ValueType::FUNCTION:
            saveFunction(static_cast<FunctionObject*>(v.ref.get()), os);
            break;
        case ValueType::NOTYPE:
        case ValueType::NONE:
        case ValueType::OMIT_MARKER:
            break; 
        default:
            std::cerr << "Warning: Serialization of ValueType " << (int)v.type << " is not supported.\n";
            break;
    }
}

inline Value loadValue(std::istream& is) {
    uint8_t typeByte;
    is.read(reinterpret_cast<char*>(&typeByte), sizeof(typeByte));
    ValueType type = static_cast<ValueType>(typeByte);
    bool isConst;
    is.read(reinterpret_cast<char*>(&isConst), sizeof(isConst));

    Value v;
    switch (type) {
        case ValueType::INT: {
            long long i;
            is.read(reinterpret_cast<char*>(&i), sizeof(i));
            v = Value::Int(i);
            break;
        }
        case ValueType::FLOAT: {
            double d;
            is.read(reinterpret_cast<char*>(&d), sizeof(d));
            v = Value::Float(d);
            break;
        }
        case ValueType::BOOL: {
            bool b;
            is.read(reinterpret_cast<char*>(&b), sizeof(b));
            v = Value::Bool(b);
            break;
        }
        case ValueType::STRING: {
            v = Value::String(loadString(is));
            break;
        }
        case ValueType::FUNCTION: {
            FunctionObject* func = loadFunction(is);
            v.type = ValueType::FUNCTION;
            v.ref = std::shared_ptr<HeapObject>(func);
            break;
        }
        case ValueType::NOTYPE: v = Value::NoType(); break;
        case ValueType::NONE: v = Value::None(); break;
        case ValueType::OMIT_MARKER: v.type = ValueType::OMIT_MARKER; break;
        default:
            v = Value::None();
            break;
    }
    v.isConst = isConst;
    return v;
}

#endif // SERIALIZATION_HPP
