
#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include "ylang/old_copy.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <map>
#include <algorithm>

inline std::string opToMnemonic(OpCode op) {
    switch (op) {
        case OpCode::OP_CONSTANT: return "CONS";
        case OpCode::OP_CONSTANT_LONG: return "CL";
        case OpCode::OP_TRUE: return "TRUE";
        case OpCode::OP_FALSE: return "FALS";
        case OpCode::OP_NONE: return "NONE";
        case OpCode::OP_NOTYPE: return "NOTY";
        case OpCode::OP_DEFINE_VAR: return "DV";
        case OpCode::OP_GET_VAR: return "GVAR";
        case OpCode::OP_SET_VAR: return "SVAR";
        case OpCode::OP_DEEP_COPY: return "DC";
        case OpCode::OP_REF_LOCAL: return "RLOC";
        case OpCode::OP_DEFINE_REF: return "DR";
        case OpCode::OP_REF_VAR: return "RVAR";
        case OpCode::OP_REF_INDEX: return "RIND";
        case OpCode::OP_SET_REF: return "SREF";
        case OpCode::OP_SHALLOW_COPY: return "SC";
        case OpCode::OP_MULTI_SET: return "MS";
        case OpCode::OP_GET_LOCAL: return "GLOC";
        case OpCode::OP_SET_LOCAL: return "SLOC";
        case OpCode::OP_INC_LOCAL: return "IL";
        case OpCode::OP_SET_FLAGS: return "SFLA";
        case OpCode::OP_REF_PROPERTY: return "RPRO";
        case OpCode::OP_DELETE: return "DELE";
        case OpCode::OP_ADD: return "ADD";
        case OpCode::OP_FAST_IADD: return "FI";
        case OpCode::OP_FAST_ISUB: return "FI1";
        case OpCode::OP_FAST_IMUL: return "FI2";
        case OpCode::OP_FAST_IDIV: return "FI3";
        case OpCode::OP_FAST_FADD: return "FF";
        case OpCode::OP_FAST_FSUB: return "FF1";
        case OpCode::OP_FAST_FMUL: return "FF2";
        case OpCode::OP_FAST_FDIV: return "FF3";
        case OpCode::OP_FAST_SADD: return "FS";
        case OpCode::OP_CTLE_TICK: return "CT";
        case OpCode::OP_SUB: return "SUB";
        case OpCode::OP_MUL: return "MUL";
        case OpCode::OP_DIV: return "DIV";
        case OpCode::OP_FLOOR_DIV: return "FD";
        case OpCode::OP_MOD: return "MOD";
        case OpCode::OP_POW: return "POW";
        case OpCode::OP_IADD: return "IADD";
        case OpCode::OP_ISUB: return "ISUB";
        case OpCode::OP_IMUL: return "IMUL";
        case OpCode::OP_IDIV: return "IDIV";
        case OpCode::OP_IFLOOR_DIV: return "ID";
        case OpCode::OP_IMOD: return "IMOD";
        case OpCode::OP_IPOW: return "IPOW";
        case OpCode::OP_DUP: return "DUP";
        case OpCode::OP_DUP_2: return "D2";
        case OpCode::OP_EQ: return "EQ";
        case OpCode::OP_NEQ: return "NEQ";
        case OpCode::OP_LT: return "LT";
        case OpCode::OP_GT: return "GT";
        case OpCode::OP_LTE: return "LTE";
        case OpCode::OP_GTE: return "GTE";
        case OpCode::OP_COLON: return "COLO";
        case OpCode::OP_STRICT_NEQ: return "SN";
        case OpCode::OP_NOT: return "NOT";
        case OpCode::OP_AND: return "AND";
        case OpCode::OP_OR: return "OR";
        case OpCode::OP_XOR: return "XOR";
        case OpCode::OP_IS: return "IS";
        case OpCode::OP_IN: return "IN";
        case OpCode::OP_IS_NOT: return "IN1";
        case OpCode::OP_STRICT_EQ: return "SE";
        case OpCode::OP_IS_IN: return "II";
        case OpCode::OP_IS_NOT_IN: return "INI";
        case OpCode::OP_NXOR: return "NXOR";
        case OpCode::OP_NAND: return "NAND";
        case OpCode::OP_NOR: return "NOR";
        case OpCode::OP_NEGATE: return "NEGA";
        case OpCode::OP_INCREMENT: return "INCR";
        case OpCode::OP_DECREMENT: return "DECR";
        case OpCode::OP_BITWISE_AND: return "BA";
        case OpCode::OP_BITWISE_OR: return "BO";
        case OpCode::OP_BITWISE_XOR: return "BX";
        case OpCode::OP_BITWISE_NOT: return "BN";
        case OpCode::OP_BITWISE_SHL: return "BS";
        case OpCode::OP_BITWISE_SHR: return "BS1";
        case OpCode::OP_PRE_INCREMENT: return "PI";
        case OpCode::OP_PRE_DECREMENT: return "PD";
        case OpCode::OP_BUILD_LIST: return "BLIS";
        case OpCode::OP_BUILD_TUPLE: return "BTUP";
        case OpCode::OP_BUILD_SET: return "BSET";
        case OpCode::OP_BUILD_DICT: return "BDIC";
        case OpCode::OP_UNPACK_DICT: return "UD";
        case OpCode::OP_BUILD_RANGE: return "BRAN";
        case OpCode::OP_BUILD_VECTOR: return "BVEC";
        case OpCode::OP_BUILD_FSTRING: return "BFST";
        case OpCode::OP_BUILD_FILE: return "BFIL";
        case OpCode::OP_BUILD_SLICE: return "BSLI";
        case OpCode::OP_CLASS: return "CLAS";
        case OpCode::OP_METHOD: return "METH";
        case OpCode::OP_GET_PROPERTY: return "GPRO";
        case OpCode::OP_SET_PROPERTY: return "SPRO";
        case OpCode::OP_CLASS_FIELD: return "CF";
        case OpCode::OP_SUPER: return "SUPE";
        case OpCode::OP_LIST_APPEND: return "LA";
        case OpCode::OP_SET_ADD: return "SADD";
        case OpCode::OP_DICT_SET: return "DS";
        case OpCode::OP_LIST_TO_TUPLE: return "LTT";
        case OpCode::OP_LIST_TO_VECTOR: return "LTV";
        case OpCode::OP_GET_INDEX: return "GIND";
        case OpCode::OP_SET_INDEX: return "SIND";
        case OpCode::OP_INVOKE: return "INVO";
        case OpCode::OP_CALL: return "CALL";
        case OpCode::OP_JUMP: return "JUMP";
        case OpCode::OP_JUMP_IF_FALSE: return "JMPF";
        case OpCode::OP_LOOP: return "LOOP";
        case OpCode::OP_RETURN: return "RETU";
        case OpCode::OP_TO_STREAM: return "TS";
        case OpCode::OP_JUMP_IF_NOT_LT: return "JMPN";
        case OpCode::OP_BREAK: return "BREA";
        case OpCode::OP_CONTINUE: return "CONT";
        case OpCode::OP_SKIP: return "SKIP";
        case OpCode::OP_OMIT: return "OMIT";
        case OpCode::OP_FOR_ITER: return "FI4";
        case OpCode::OP_SKIP_ITER: return "SI";
        case OpCode::OP_SWITCH_TABLE: return "ST";
        case OpCode::OP_THROW: return "THRO";
        case OpCode::OP_ASSERT: return "ASSE";
        case OpCode::OP_IMPORT: return "IMPO";
        case OpCode::OP_POP: return "POP";
        case OpCode::OP_DEBUG_NAME: return "DN";
        case OpCode::OP_REPL_PRINT: return "RP";
        case OpCode::OP_TRY_ENTER: return "TE";
        case OpCode::OP_TRY_EXIT: return "TE1";
        case OpCode::OP_CATCH: return "CATC";
        case OpCode::OP_RETHROW: return "RETH";
        case OpCode::OP_END_FINALLY: return "EF";
        default: return "UNKN";
    }
}

inline OpCode mnemonicToOp(const std::string& mn) {
    if (mn == "CONS") return OpCode::OP_CONSTANT;
    if (mn == "CL") return OpCode::OP_CONSTANT_LONG;
    if (mn == "TRUE") return OpCode::OP_TRUE;
    if (mn == "FALS") return OpCode::OP_FALSE;
    if (mn == "NONE") return OpCode::OP_NONE;
    if (mn == "NOTY") return OpCode::OP_NOTYPE;
    if (mn == "DV") return OpCode::OP_DEFINE_VAR;
    if (mn == "GVAR") return OpCode::OP_GET_VAR;
    if (mn == "SVAR") return OpCode::OP_SET_VAR;
    if (mn == "DC") return OpCode::OP_DEEP_COPY;
    if (mn == "RLOC") return OpCode::OP_REF_LOCAL;
    if (mn == "DR") return OpCode::OP_DEFINE_REF;
    if (mn == "RVAR") return OpCode::OP_REF_VAR;
    if (mn == "RIND") return OpCode::OP_REF_INDEX;
    if (mn == "SREF") return OpCode::OP_SET_REF;
    if (mn == "SC") return OpCode::OP_SHALLOW_COPY;
    if (mn == "MS") return OpCode::OP_MULTI_SET;
    if (mn == "GLOC") return OpCode::OP_GET_LOCAL;
    if (mn == "SLOC") return OpCode::OP_SET_LOCAL;
    if (mn == "IL") return OpCode::OP_INC_LOCAL;
    if (mn == "SFLA") return OpCode::OP_SET_FLAGS;
    if (mn == "RPRO") return OpCode::OP_REF_PROPERTY;
    if (mn == "DELE") return OpCode::OP_DELETE;
    if (mn == "ADD") return OpCode::OP_ADD;
    if (mn == "FI") return OpCode::OP_FAST_IADD;
    if (mn == "FI1") return OpCode::OP_FAST_ISUB;
    if (mn == "FI2") return OpCode::OP_FAST_IMUL;
    if (mn == "FI3") return OpCode::OP_FAST_IDIV;
    if (mn == "FF") return OpCode::OP_FAST_FADD;
    if (mn == "FF1") return OpCode::OP_FAST_FSUB;
    if (mn == "FF2") return OpCode::OP_FAST_FMUL;
    if (mn == "FF3") return OpCode::OP_FAST_FDIV;
    if (mn == "FS") return OpCode::OP_FAST_SADD;
    if (mn == "CT") return OpCode::OP_CTLE_TICK;
    if (mn == "SUB") return OpCode::OP_SUB;
    if (mn == "MUL") return OpCode::OP_MUL;
    if (mn == "DIV") return OpCode::OP_DIV;
    if (mn == "FD") return OpCode::OP_FLOOR_DIV;
    if (mn == "MOD") return OpCode::OP_MOD;
    if (mn == "POW") return OpCode::OP_POW;
    if (mn == "IADD") return OpCode::OP_IADD;
    if (mn == "ISUB") return OpCode::OP_ISUB;
    if (mn == "IMUL") return OpCode::OP_IMUL;
    if (mn == "IDIV") return OpCode::OP_IDIV;
    if (mn == "ID") return OpCode::OP_IFLOOR_DIV;
    if (mn == "IMOD") return OpCode::OP_IMOD;
    if (mn == "IPOW") return OpCode::OP_IPOW;
    if (mn == "DUP") return OpCode::OP_DUP;
    if (mn == "D2") return OpCode::OP_DUP_2;
    if (mn == "EQ") return OpCode::OP_EQ;
    if (mn == "NEQ") return OpCode::OP_NEQ;
    if (mn == "LT") return OpCode::OP_LT;
    if (mn == "GT") return OpCode::OP_GT;
    if (mn == "LTE") return OpCode::OP_LTE;
    if (mn == "GTE") return OpCode::OP_GTE;
    if (mn == "COLO") return OpCode::OP_COLON;
    if (mn == "SN") return OpCode::OP_STRICT_NEQ;
    if (mn == "NOT") return OpCode::OP_NOT;
    if (mn == "AND") return OpCode::OP_AND;
    if (mn == "OR") return OpCode::OP_OR;
    if (mn == "XOR") return OpCode::OP_XOR;
    if (mn == "IS") return OpCode::OP_IS;
    if (mn == "IN") return OpCode::OP_IN;
    if (mn == "IN1") return OpCode::OP_IS_NOT;
    if (mn == "SE") return OpCode::OP_STRICT_EQ;
    if (mn == "II") return OpCode::OP_IS_IN;
    if (mn == "INI") return OpCode::OP_IS_NOT_IN;
    if (mn == "NXOR") return OpCode::OP_NXOR;
    if (mn == "NAND") return OpCode::OP_NAND;
    if (mn == "NOR") return OpCode::OP_NOR;
    if (mn == "NEGA") return OpCode::OP_NEGATE;
    if (mn == "INCR") return OpCode::OP_INCREMENT;
    if (mn == "DECR") return OpCode::OP_DECREMENT;
    if (mn == "BA") return OpCode::OP_BITWISE_AND;
    if (mn == "BO") return OpCode::OP_BITWISE_OR;
    if (mn == "BX") return OpCode::OP_BITWISE_XOR;
    if (mn == "BN") return OpCode::OP_BITWISE_NOT;
    if (mn == "BS") return OpCode::OP_BITWISE_SHL;
    if (mn == "BS1") return OpCode::OP_BITWISE_SHR;
    if (mn == "PI") return OpCode::OP_PRE_INCREMENT;
    if (mn == "PD") return OpCode::OP_PRE_DECREMENT;
    if (mn == "BLIS") return OpCode::OP_BUILD_LIST;
    if (mn == "BTUP") return OpCode::OP_BUILD_TUPLE;
    if (mn == "BSET") return OpCode::OP_BUILD_SET;
    if (mn == "BDIC") return OpCode::OP_BUILD_DICT;
    if (mn == "UD") return OpCode::OP_UNPACK_DICT;
    if (mn == "BRAN") return OpCode::OP_BUILD_RANGE;
    if (mn == "BVEC") return OpCode::OP_BUILD_VECTOR;
    if (mn == "BFST") return OpCode::OP_BUILD_FSTRING;
    if (mn == "BFIL") return OpCode::OP_BUILD_FILE;
    if (mn == "BSLI") return OpCode::OP_BUILD_SLICE;
    if (mn == "CLAS") return OpCode::OP_CLASS;
    if (mn == "METH") return OpCode::OP_METHOD;
    if (mn == "GPRO") return OpCode::OP_GET_PROPERTY;
    if (mn == "SPRO") return OpCode::OP_SET_PROPERTY;
    if (mn == "CF") return OpCode::OP_CLASS_FIELD;
    if (mn == "SUPE") return OpCode::OP_SUPER;
    if (mn == "LA") return OpCode::OP_LIST_APPEND;
    if (mn == "SADD") return OpCode::OP_SET_ADD;
    if (mn == "DS") return OpCode::OP_DICT_SET;
    if (mn == "LTT") return OpCode::OP_LIST_TO_TUPLE;
    if (mn == "LTV") return OpCode::OP_LIST_TO_VECTOR;
    if (mn == "GIND") return OpCode::OP_GET_INDEX;
    if (mn == "SIND") return OpCode::OP_SET_INDEX;
    if (mn == "INVO") return OpCode::OP_INVOKE;
    if (mn == "CALL") return OpCode::OP_CALL;
    if (mn == "JUMP") return OpCode::OP_JUMP;
    if (mn == "JMPF") return OpCode::OP_JUMP_IF_FALSE;
    if (mn == "LOOP") return OpCode::OP_LOOP;
    if (mn == "RETU") return OpCode::OP_RETURN;
    if (mn == "TS") return OpCode::OP_TO_STREAM;
    if (mn == "JMPN") return OpCode::OP_JUMP_IF_NOT_LT;
    if (mn == "BREA") return OpCode::OP_BREAK;
    if (mn == "CONT") return OpCode::OP_CONTINUE;
    if (mn == "SKIP") return OpCode::OP_SKIP;
    if (mn == "OMIT") return OpCode::OP_OMIT;
    if (mn == "FI4") return OpCode::OP_FOR_ITER;
    if (mn == "SI") return OpCode::OP_SKIP_ITER;
    if (mn == "ST") return OpCode::OP_SWITCH_TABLE;
    if (mn == "THRO") return OpCode::OP_THROW;
    if (mn == "ASSE") return OpCode::OP_ASSERT;
    if (mn == "IMPO") return OpCode::OP_IMPORT;
    if (mn == "POP") return OpCode::OP_POP;
    if (mn == "DN") return OpCode::OP_DEBUG_NAME;
    if (mn == "RP") return OpCode::OP_REPL_PRINT;
    if (mn == "TE") return OpCode::OP_TRY_ENTER;
    if (mn == "TE1") return OpCode::OP_TRY_EXIT;
    if (mn == "CATC") return OpCode::OP_CATCH;
    if (mn == "RETH") return OpCode::OP_RETHROW;
    if (mn == "EF") return OpCode::OP_END_FINALLY;
    return OpCode::OP_NOTYPE;
}

inline std::string escapeString(const std::string& s) {
    std::string r;
    for (char c : s) {
        if (c == '\n') r += "\\n";
        else if (c == '\r') r += "\\r";
        else if (c == '\t') r += "\\t";
        else if (c == '\\') r += "\\\\";
        else if (c == '\"') r += "\\\"";
        else r += c;
    }
    return r;
}

inline std::string unescapeString(const std::string& s) {
    std::string r;
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] == '\\' && i + 1 < s.length()) {
            char n = s[i+1];
            if (n == 'n') r += '\n';
            else if (n == 'r') r += '\r';
            else if (n == 't') r += '\t';
            else if (n == '\\') r += '\\';
            else if (n == '"') r += '"';
            else r += n;
            i++;
        } else {
            r += s[i];
        }
    }
    return r;
}

struct AssemblyState {
    std::map<FunctionObject*, int> funcToId;
    std::vector<FunctionObject*> funcs;
    
    int getId(FunctionObject* f) {
        if (funcToId.find(f) == funcToId.end()) {
            int id = funcs.size();
            funcs.push_back(f);
            funcToId[f] = id;
            
            if (f->chunk) collect(f->chunk);
            for (auto& p : f->params) if (p.defaultChunk) collect(p.defaultChunk);
            for (auto* rc : f->defaultRetChunks) if (rc) collect(rc);
        }
        return funcToId[f];
    }
    
    void collect(Chunk* c) {
        for (const Value& v : c->constants) {
            if (v.type == ValueType::FUNCTION && v.ref) {
                getId(static_cast<FunctionObject*>(v.ref.get()));
            }
        }
    }
};

inline void saveChunkAssembly(Chunk* chunk, std::ostream& os, AssemblyState& state, std::string indent = "") {
    if (!chunk) {
        os << indent << ".nullchunk\n";
        return;
    }
    os << indent << ".const\n";
    for (size_t i = 0; i < chunk->constants.size(); i++) {
        os << indent << std::left << std::setw(4) << i << ": ";
        const Value& v = chunk->constants[i];
        if (v.type == ValueType::INT) os << std::setw(8) << "INT" << v.iVal << "\n";
        else if (v.type == ValueType::FLOAT) os << std::setw(8) << "FLOAT" << v.fVal << "\n";
        else if (v.type == ValueType::BOOL) os << std::setw(8) << "BOOL" << (v.bVal ? "true" : "false") << "\n";
        else if (v.type == ValueType::STRING) os << std::setw(8) << "STR" << "\"" << escapeString(v.asString()) << "\"\n";
        else if (v.type == ValueType::NONE) os << std::setw(8) << "NONE\n";
        else if (v.type == ValueType::FUNCTION && v.ref) {
            os << std::setw(8) << "FUNC" << state.funcToId[static_cast<FunctionObject*>(v.ref.get())] << "\n";
        } else if (v.type == ValueType::OMIT_MARKER) {
            os << std::setw(8) << "OMIT\n";
        } else os << std::setw(8) << "UNKN" << "\n";
    }
    os << indent << ".code\n";
    for (size_t i = 0; i < chunk->code.size(); ) {
        uint8_t instruction = chunk->code[i];
        OpCode op = static_cast<OpCode>(instruction);
        os << indent << std::setw(4) << std::setfill('0') << i << "    " << std::setfill(' ');
        os << std::left << std::setw(8) << opToMnemonic(op);
        int args = 0;
        switch (op) {
            case OpCode::OP_CONSTANT: case OpCode::OP_GET_VAR: case OpCode::OP_SET_VAR: case OpCode::OP_DEFINE_VAR:
            case OpCode::OP_GET_LOCAL: case OpCode::OP_SET_LOCAL: case OpCode::OP_CALL: case OpCode::OP_BUILD_LIST:
            case OpCode::OP_BUILD_TUPLE: case OpCode::OP_BUILD_SET: case OpCode::OP_BUILD_DICT: case OpCode::OP_IMPORT:
                if (i + 1 < chunk->code.size()) { os << (int)chunk->code[i+1]; args = 1; }
                break;
            case OpCode::OP_JUMP: case OpCode::OP_JUMP_IF_FALSE: case OpCode::OP_LOOP:
                if (i + 2 < chunk->code.size()) {
                    uint16_t jmp = (chunk->code[i+1] << 8) | chunk->code[i+2];
                    os << jmp; args = 2;
                }
                break;
            default: break;
        }
        os << "\n"; i += 1 + args;
    }
    os << indent << ".endchunk\n";
}

inline void saveAssembly(Chunk* mainChunk, std::ostream& os) {
    AssemblyState state;
    state.collect(mainChunk);
    
    os << "; Generated by y--lang Assembler\n\n";
    for (size_t i = 0; i < state.funcs.size(); i++) {
        FunctionObject* f = state.funcs[i];
        os << ".function " << i << "\n";
        os << "  .name \"" << escapeString(f->name) << "\"\n";
        os << "  .params " << f->params.size() << "\n";
        for (auto& p : f->params) {
            os << "    \"" << escapeString(p.name) << "\" " << (int)p.mode << " " << (int)p.type << " " << p.isConst << " " << p.isVariadic << " " << p.isKwargs << "\n";
            os << "    .default\n";
            saveChunkAssembly(p.defaultChunk, os, state, "      ");
        }
        os << "  .returnType " << (int)f->returnType << "\n";
        os << "  .returnsConst " << f->returnsConst << "\n";
        os << "  .isCached " << f->isCached << "\n";
        os << "  .retChunks " << f->defaultRetChunks.size() << "\n";
        for (auto* rc : f->defaultRetChunks) {
            os << "    .retchunk\n";
            saveChunkAssembly(rc, os, state, "      ");
        }
        os << "  .body\n";
        saveChunkAssembly(f->chunk, os, state, "    ");
        os << "\n";
    }
    
    os << ".main\n";
    saveChunkAssembly(mainChunk, os, state, "  ");
}

inline Chunk* loadChunkAssembly(std::istream& is, std::map<int, FunctionObject*>& funcs) {
    Chunk* chunk = new Chunk();
    std::string line;
    bool inConst = false, inCode = false;

    while (std::getline(is, line)) {
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty()) continue;

        if (line == ".endchunk" || line == ".nullchunk") {
            if (line == ".nullchunk") { delete chunk; return nullptr; }
            break;
        }
        if (line == ".const") { inConst = true; inCode = false; continue; }
        if (line == ".code") { inConst = false; inCode = true; continue; }

        if (inConst) {
            size_t colon = line.find(':');
            if (colon == std::string::npos) continue;
            std::stringstream ss(line.substr(colon + 1));
            std::string typeStr; ss >> typeStr;
            
            if (typeStr == "INT") { long long v; ss >> v; chunk->constants.push_back(Value::Int(v)); }
            else if (typeStr == "FLOAT") { double v; ss >> v; chunk->constants.push_back(Value::Float(v)); }
            else if (typeStr == "BOOL") { std::string v; ss >> v; chunk->constants.push_back(Value::Bool(v == "true")); }
            else if (typeStr == "STR") {
                std::string v; std::getline(ss, v);
                v.erase(0, v.find_first_not_of(" \t\""));
                if (!v.empty() && v.back() == '"') v.pop_back();
                chunk->constants.push_back(Value::String(unescapeString(v)));
            }
            else if (typeStr == "NONE") chunk->constants.push_back(Value::None());
            else if (typeStr == "OMIT") {
                Value v; v.type = ValueType::OMIT_MARKER;
                chunk->constants.push_back(v);
            }
            else if (typeStr == "FUNC") {
                int id; ss >> id;
                Value v; v.type = ValueType::FUNCTION;
                v.ref = std::shared_ptr<HeapObject>(funcs[id]);
                chunk->constants.push_back(v);
            }
        } else if (inCode) {
            std::stringstream ss(line);
            std::string addr, mn; ss >> addr >> mn;
            OpCode op = mnemonicToOp(mn);
            chunk->write(static_cast<uint8_t>(op), 0, 0);
            
            int args = 0;
            switch (op) {
                case OpCode::OP_CONSTANT: case OpCode::OP_GET_VAR: case OpCode::OP_SET_VAR: case OpCode::OP_DEFINE_VAR:
                case OpCode::OP_GET_LOCAL: case OpCode::OP_SET_LOCAL: case OpCode::OP_CALL: case OpCode::OP_BUILD_LIST:
                case OpCode::OP_BUILD_TUPLE: case OpCode::OP_BUILD_SET: case OpCode::OP_BUILD_DICT: case OpCode::OP_IMPORT:
                    args = 1; break;
                case OpCode::OP_JUMP: case OpCode::OP_JUMP_IF_FALSE: case OpCode::OP_LOOP:
                    args = 2; break;
                default: break;
            }
            if (args == 1) { int v; if(ss>>v) chunk->write(v,0,0); else chunk->write(0,0,0); }
            else if (args == 2) {
                int v;
                if(ss>>v) { chunk->write((v>>8)&0xFF,0,0); chunk->write(v&0xFF,0,0); }
                else { chunk->write(0,0,0); chunk->write(0,0,0); }
            }
        }
    }
    return chunk;
}

inline Chunk* loadAssembly(std::istream& is) {
    std::map<int, FunctionObject*> funcs;
    std::string line;
    
    // First pass: create empty functions so references resolve correctly
    std::streampos startPos = is.tellg();
    while (std::getline(is, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        if (line.substr(0, 9) == ".function") {
            int id;
            std::stringstream ss(line.substr(9)); ss >> id;
            funcs[id] = new FunctionObject(std::vector<ParamSpec>(), ValueType::NOTYPE, std::vector<Expr*>(), false, std::vector<Stmt*>(), nullptr, false, nullptr);
        }
    }
    is.clear();
    is.seekg(startPos);
    
    // Second pass: parse everything
    Chunk* mainChunk = nullptr;
    while (std::getline(is, line)) {
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty()) continue;

        if (line.substr(0, 9) == ".function") {
            int id; std::stringstream ss(line.substr(9)); ss >> id;
            FunctionObject* f = funcs[id];
            
            while (std::getline(is, line)) {
                size_t cPos = line.find(';'); if (cPos != std::string::npos) line = line.substr(0, cPos);
                line.erase(0, line.find_first_not_of(" \t\r\n")); line.erase(line.find_last_not_of(" \t\r\n") + 1);
                if (line.empty()) continue;
                
                if (line.substr(0, 5) == ".name") {
                    std::string n = line.substr(5);
                    n.erase(0, n.find_first_not_of(" \t\"")); if (!n.empty() && n.back() == '"') n.pop_back();
                    f->name = unescapeString(n);
                }
                else if (line.substr(0, 7) == ".params") {
                    int pCount; std::stringstream pss(line.substr(7)); pss >> pCount;
                    for (int i = 0; i < pCount; i++) {
                        std::getline(is, line);
                        line.erase(0, line.find_first_not_of(" \t\r\n"));
                        std::stringstream argss(line);
                        ParamSpec p;
                        std::string n; argss >> n;
                        n.erase(0, n.find_first_not_of(" \t\"")); if (!n.empty() && n.back() == '"') n.pop_back();
                        p.name = unescapeString(n);
                        int m, t; bool ic, iv, ik;
                        argss >> m >> t >> ic >> iv >> ik;
                        p.mode = static_cast<CopyMode>(m); p.type = static_cast<ValueType>(t);
                        p.isConst = ic; p.isVariadic = iv; p.isKwargs = ik; p.defaultValue = nullptr;
                        
                        std::getline(is, line); // .default
                        p.defaultChunk = loadChunkAssembly(is, funcs);
                        f->params.push_back(p);
                    }
                }
                else if (line.substr(0, 11) == ".returnType") { int v; std::stringstream pss(line.substr(11)); pss >> v; f->returnType = static_cast<ValueType>(v); }
                else if (line.substr(0, 13) == ".returnsConst") { int v; std::stringstream pss(line.substr(13)); pss >> v; f->returnsConst = v; }
                else if (line.substr(0, 9) == ".isCached") { int v; std::stringstream pss(line.substr(9)); pss >> v; f->isCached = v; }
                else if (line.substr(0, 10) == ".retChunks") {
                    int v; std::stringstream pss(line.substr(10)); pss >> v;
                    for(int i = 0; i < v; i++) {
                        std::getline(is, line); // .retchunk
                        f->defaultRetChunks.push_back(loadChunkAssembly(is, funcs));
                    }
                }
                else if (line.substr(0, 5) == ".body") {
                    f->chunk = loadChunkAssembly(is, funcs);
                    break;
                }
            }
        }
        else if (line == ".main") {
            mainChunk = loadChunkAssembly(is, funcs);
        }
    }
    return mainChunk;
}

#endif // ASSEMBLER_HPP
