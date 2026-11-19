#include "ylang/assembler.hpp"

// Mnemonics are the opcode names without "OP_", so every opcode has a unique one.
std::string opToMnemonic(OpCode op) {
	return OpCodeToString(op).substr(3);
}

OpCode mnemonicToOp(const std::string &mn) {
	static const std::unordered_map<std::string, OpCode> table = [] {
		std::unordered_map<std::string, OpCode> t;
		for (int i = 0; i <= (int)OpCode::OP_END_FINALLY; i++)
			t[opToMnemonic((OpCode)i)] = (OpCode)i;
		return t;
	}();
	auto it = table.find(mn);
	if (it == table.end())
		throw std::runtime_error("unknown instruction '" + mn + "' in .yb file");
	return it->second;
}

// Number of operand bytes after the opcode at code[i]. Must match how VM::run decodes each instruction.
static size_t operandBytes(const std::vector<uint8_t> &code, size_t i) {
	auto at = [&](size_t k) -> size_t { return k < code.size() ? code[k] : 0; };
	switch ((OpCode)code[i]) {
	case OpCode::OP_BUILD_DICT:
	case OpCode::OP_BUILD_FSTRING:
	case OpCode::OP_BUILD_LIST:
	case OpCode::OP_BUILD_RANGE:
	case OpCode::OP_BUILD_SET:
	case OpCode::OP_BUILD_TUPLE:
	case OpCode::OP_BUILD_VECTOR:
	case OpCode::OP_CALL:
	case OpCode::OP_CLASS:
	case OpCode::OP_CONSTANT:
	case OpCode::OP_DEBUG_NAME:
	case OpCode::OP_DICT_SET:
	case OpCode::OP_GET_LOCAL:
	case OpCode::OP_GET_PROPERTY:
	case OpCode::OP_GET_VAR:
	case OpCode::OP_IMPORT:
	case OpCode::OP_INC_LOCAL:
	case OpCode::OP_LIST_APPEND:
	case OpCode::OP_METHOD:
	case OpCode::OP_REF_LOCAL:
	case OpCode::OP_REF_PROPERTY:
	case OpCode::OP_REF_VAR:
	case OpCode::OP_SET_ADD:
	case OpCode::OP_SET_FLAGS:
	case OpCode::OP_SET_LOCAL:
	case OpCode::OP_SET_VAR:
	case OpCode::OP_SKIP_ITER:
		return 1;
	case OpCode::OP_CLASS_FIELD:
	case OpCode::OP_CONSTANT_LONG:
	case OpCode::OP_DEFINE_VAR:
	case OpCode::OP_INC_INDEX:
	case OpCode::OP_INVOKE:
	case OpCode::OP_JUMP:
	case OpCode::OP_JUMP_IF_FALSE:
	case OpCode::OP_LOOP:
	case OpCode::OP_SET_PROPERTY:
		return 2;
	case OpCode::OP_FOR_ITER:
	case OpCode::OP_INC_PROPERTY:
		return 3;
	case OpCode::OP_JUMP_IF_NOT_LT:
	case OpCode::OP_TRY_ENTER:
		return 4;
	case OpCode::OP_CATCH: // count, then one constant index per caught type
		return 1 + at(i + 1);
	case OpCode::OP_SWITCH_TABLE: // min constant, count, then a 2-byte offset per entry
		return 2 + 2 * at(i + 2);
	case OpCode::OP_DEFINE_REF: // name, sub-op, and a target name only for variable refs
	case OpCode::OP_SET_REF:
		return 2 + ((OpCode)at(i + 2) == OpCode::OP_REF_VAR ? 1 : 0);
	default:
		return 0;
	}
}

std::string escapeString(const std::string &s) {
	std::string r;
	for (char c : s) {
		if (c == '\n')
			r += "\\n";
		else if (c == '\r')
			r += "\\r";
		else if (c == '\t')
			r += "\\t";
		else if (c == '\\')
			r += "\\\\";
		else if (c == '\"')
			r += "\\\"";
		else
			r += c;
	}
	return r;
}

std::string unescapeString(const std::string &s) {
	std::string r;
	for (size_t i = 0; i < s.length(); i++) {
		if (s[i] == '\\' && i + 1 < s.length()) {
			char n = s[i + 1];
			if (n == 'n')
				r += '\n';
			else if (n == 'r')
				r += '\r';
			else if (n == 't')
				r += '\t';
			else if (n == '\\')
				r += '\\';
			else if (n == '"')
				r += '"';
			else
				r += n;
			i++;
		} else {
			r += s[i];
		}
	}
	return r;
}

int AssemblyState::getId(FunctionObject *f) {
		if (funcToId.find(f) == funcToId.end()) {
			int id = funcs.size();
			funcs.push_back(f);
			funcToId[f] = id;

			if (f->chunk)
				collect(f->chunk);
			for (auto &p : f->params)
				if (p.defaultChunk)
					collect(p.defaultChunk);
			for (auto *rc : f->defaultRetChunks)
				if (rc)
					collect(rc);
		}
		return funcToId[f];
	}

void AssemblyState::collect(Chunk *c) {
		for (const Value &v : c->constants) {
			if (v.type == ValueType::FUNCTION && v.ref) {
				getId(static_cast<FunctionObject *>(v.ref.get()));
			}
		}
	}

static void writeConst(std::ostream &os, const Value &v, AssemblyState &state) {
	auto seq = [&](const char *tag, const std::vector<Value> &el) {
		os << tag << " " << el.size();
		for (const auto &e : el) {
			os << " ";
			writeConst(os, e, state);
		}
	};
	switch (v.type) {
	case ValueType::INT:
		os << "INT " << v.iVal;
		break;
	case ValueType::FLOAT:
		if (std::isfinite(v.fVal)) {
			char buf[40];
			std::snprintf(buf, sizeof buf, "%.17g", v.fVal); // exact round trip
			os << "FLOAT " << buf;
		} else
			os << "FLOAT " << (std::isnan(v.fVal) ? "nan" : (v.fVal > 0 ? "inf" : "-inf"));
		break;
	case ValueType::BOOL:
		os << "BOOL " << (v.bVal ? "true" : "false");
		break;
	case ValueType::STRING:
		os << "STR \"" << escapeString(v.asString()) << "\"";
		break;
	case ValueType::BIGINT:
		os << "BIG " << bigIntToString(static_cast<BigIntObject *>(v.ref.get()));
		break;
	case ValueType::NONE:
		os << "NONE";
		break;
	case ValueType::NOTYPE:
		os << "NOTY";
		break;
	case ValueType::OMIT_MARKER:
		os << "OMIT";
		break;
	case ValueType::FUNCTION:
		os << "FUNC " << state.getId(static_cast<FunctionObject *>(v.ref.get()));
		break;
	case ValueType::LIST:
		seq("LIST", static_cast<ListObject *>(v.ref.get())->elements);
		break;
	case ValueType::TUPLE:
		seq("TUPLE", static_cast<TupleObject *>(v.ref.get())->elements);
		break;
	case ValueType::VECTOR:
		seq("VEC", static_cast<VectorObject *>(v.ref.get())->elements);
		break;
	case ValueType::SET: {
		auto &el = static_cast<SetObject *>(v.ref.get())->elements;
		seq("SET", std::vector<Value>(el.begin(), el.end()));
		break;
	}
	case ValueType::DICT: {
		std::vector<Value> flat;
		for (const auto &kv : static_cast<DictObject *>(v.ref.get())->items) {
			flat.push_back(kv.first);
			flat.push_back(kv.second);
		}
		seq("DICT", flat);
		break;
	}
	default:
		throw std::runtime_error("cannot write a constant of type " + std::to_string((int)v.type) + " to .yb");
	}
}

void saveChunkAssembly(Chunk *chunk, std::ostream &os, AssemblyState &state, std::string indent) {
	if (!chunk) {
		os << indent << ".nullchunk\n";
		return;
	}
	os << indent << ".const\n";
	for (size_t i = 0; i < chunk->constants.size(); i++) {
		os << indent << i << " : ";
		writeConst(os, chunk->constants[i], state);
		os << "\n";
	}
	// one instruction per line: address, mnemonic, raw operand bytes, @source-line, ; note
	os << indent << ".code\n";
	const auto &code = chunk->code;
	for (size_t i = 0; i < code.size();) {
		OpCode op = static_cast<OpCode>(code[i]);
		size_t n = operandBytes(code, i);
		os << indent << std::right << std::setw(4) << std::setfill('0') << i << std::setfill(' ') << "  " << opToMnemonic(op);
		for (size_t k = 1; k <= n && i + k < code.size(); k++)
			os << " " << (int)code[i + k];
		if (i < chunk->lines.size())
			os << " @" << chunk->lines[i];
		if (n >= 2 && (op == OpCode::OP_JUMP || op == OpCode::OP_JUMP_IF_FALSE || op == OpCode::OP_LOOP)) {
			size_t off = (code[i + 1] << 8) | code[i + 2];
			os << "  ; -> " << (op == OpCode::OP_LOOP ? i + 3 - off : i + 3 + off);
		} else if (n >= 1 && (op == OpCode::OP_CONSTANT || op == OpCode::OP_GET_VAR || op == OpCode::OP_SET_VAR || op == OpCode::OP_DEFINE_VAR ||
										op == OpCode::OP_GET_PROPERTY || op == OpCode::OP_SET_PROPERTY || op == OpCode::OP_INVOKE || op == OpCode::OP_IMPORT)) {
			size_t k = code[i + 1];
			if (k < chunk->constants.size() && chunk->constants[k].type == ValueType::STRING)
				os << "  ; " << escapeString(chunk->constants[k].asString());
		}
		os << "\n";
		i += 1 + n;
	}
	os << indent << ".endchunk\n";
}

void saveAssembly(Chunk *mainChunk, std::ostream &os) {
	AssemblyState state;
	state.collect(mainChunk);

	os << "; Generated by y--lang Assembler\n\n";
	for (size_t i = 0; i < state.funcs.size(); i++) {
		FunctionObject *f = state.funcs[i];
		os << ".function " << i << "\n";
		os << "  .name \"" << escapeString(f->name) << "\"\n";
		os << "  .params " << f->params.size() << "\n";
		for (auto &p : f->params) {
			os << "    \"" << escapeString(p.name) << "\" " << (int)p.mode << " " << (int)p.type << " " << p.isConst << " " << p.isVariadic << " " << p.isKwargs << "\n";
			os << "    .default\n";
			saveChunkAssembly(p.defaultChunk, os, state, "      ");
		}
		os << "  .returnType " << (int)f->returnType << "\n";
		os << "  .returnsConst " << f->returnsConst << "\n";
		os << "  .isCached " << f->isCached << "\n";
		os << "  .returnsRef " << f->returnsRef << "\n";
		os << "  .retChunks " << f->defaultRetChunks.size() << "\n";
		for (auto *rc : f->defaultRetChunks) {
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

// Drops a trailing "; comment" but keeps ';' that are inside string literals.
static std::string stripComment(const std::string &line) {
	bool inStr = false;
	for (size_t i = 0; i < line.size(); i++) {
		if (inStr && line[i] == '\\')
			i++;
		else if (line[i] == '"')
			inStr = !inStr;
		else if (!inStr && line[i] == ';')
			return line.substr(0, i);
	}
	return line;
}

static std::string trim(std::string line) {
	line.erase(0, line.find_first_not_of(" \t\r\n"));
	line.erase(line.find_last_not_of(" \t\r\n") + 1);
	return line;
}

// Splits on spaces; a "quoted string" (with escapes) is one token, returned unescaped and marked by a leading '"'.
static std::vector<std::string> tokens(const std::string &s) {
	std::vector<std::string> out;
	for (size_t i = 0; i < s.size();) {
		if (isspace((unsigned char)s[i])) {
			i++;
		} else if (s[i] == '"') {
			std::string raw;
			for (i++; i < s.size() && s[i] != '"'; i++) {
				if (s[i] == '\\' && i + 1 < s.size())
					raw += s[i++];
				raw += s[i];
			}
			i++;
			out.push_back("\"" + unescapeString(raw));
		} else {
			size_t j = s.find_first_of(" \t", i);
			if (j == std::string::npos)
				j = s.size();
			out.push_back(s.substr(i, j - i));
			i = j;
		}
	}
	return out;
}

static Value readConst(const std::vector<std::string> &t, size_t &k, std::map<int, FunctionObject *> &funcs) {
	if (k >= t.size())
		throw std::runtime_error("truncated constant in .yb file");
	const std::string tag = t[k++];
	auto next = [&]() -> const std::string & {
		if (k >= t.size())
			throw std::runtime_error("truncated constant in .yb file");
		return t[k++];
	};
	auto seq = [&]() {
		size_t n = std::stoul(next());
		std::vector<Value> el;
		for (size_t i = 0; i < n; i++)
			el.push_back(readConst(t, k, funcs));
		return el;
	};
	if (tag == "INT")
		return Value::Int(std::stoll(next()));
	if (tag == "FLOAT")
		return Value::Float(std::strtod(next().c_str(), nullptr));
	if (tag == "BOOL")
		return Value::Bool(next() == "true");
	if (tag == "STR") {
		const std::string &s = next();
		return Value::String(s.empty() ? s : s.substr(1));
	}
	if (tag == "BIG")
		return Value::BigInt(next());
	if (tag == "NONE")
		return Value::None();
	if (tag == "NOTY")
		return Value::NoType();
	if (tag == "OMIT") {
		Value v;
		v.type = ValueType::OMIT_MARKER;
		return v;
	}
	if (tag == "FUNC") {
		int id = std::stoi(next());
		if (!funcs.count(id))
			throw std::runtime_error("unknown function id " + std::to_string(id) + " in .yb file");
		Value v;
		v.type = ValueType::FUNCTION;
		v.ref = std::shared_ptr<HeapObject>(funcs[id]);
		return v;
	}
	if (tag == "LIST")
		return Value::List(seq());
	if (tag == "TUPLE")
		return Value::Tuple(seq());
	if (tag == "VEC")
		return Value::Vector(seq());
	if (tag == "SET") {
		auto el = seq();
		return Value::Set(std::unordered_set<Value, ValueHash, ValueEqual>(el.begin(), el.end()));
	}
	if (tag == "DICT") {
		auto flat = seq();
		std::unordered_map<Value, Value, ValueHash, ValueEqual> m;
		for (size_t i = 0; i + 1 < flat.size(); i += 2)
			m[flat[i]] = flat[i + 1];
		return Value::Dict(m);
	}
	throw std::runtime_error("unknown constant type '" + tag + "' in .yb file");
}

Chunk *loadChunkAssembly(std::istream &is, std::map<int, FunctionObject *> &funcs) {
	Chunk *chunk = new Chunk();
	std::string line;
	bool inConst = false, inCode = false;

	while (std::getline(is, line)) {
		line = trim(stripComment(line));
		if (line.empty())
			continue;
		if (line == ".nullchunk") {
			delete chunk;
			return nullptr;
		}
		if (line == ".endchunk")
			break;
		if (line == ".const" || line == ".code") {
			inConst = line == ".const";
			inCode = !inConst;
			continue;
		}
		if (inConst) {
			size_t colon = line.find(':');
			if (colon == std::string::npos)
				continue;
			auto t = tokens(line.substr(colon + 1));
			size_t k = 0;
			chunk->constants.push_back(readConst(t, k, funcs));
		} else if (inCode) {
			// "addr MNEMONIC b1 b2 ... @line": every number after the mnemonic is one operand byte
			auto t = tokens(line);
			if (t.size() < 2)
				throw std::runtime_error("bad instruction line in .yb file: " + line);
			int srcLine = 0;
			std::vector<uint8_t> bytes = {static_cast<uint8_t>(mnemonicToOp(t[1]))};
			for (size_t k = 2; k < t.size(); k++) {
				if (t[k][0] == '@') {
					srcLine = std::stoi(t[k].substr(1));
					continue;
				}
				int b = std::stoi(t[k]);
				if (b < 0 || b > 255)
					throw std::runtime_error("operand out of byte range in .yb file: " + line);
				bytes.push_back((uint8_t)b);
			}
			for (uint8_t b : bytes)
				chunk->write(b, srcLine, 0);
		}
	}
	return chunk;
}

Chunk *loadAssembly(std::istream &is) {
	std::map<int, FunctionObject *> funcs;
	std::string line;

	// First pass: create empty functions so references resolve correctly
	std::streampos startPos = is.tellg();
	while (std::getline(is, line)) {
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		if (line.substr(0, 9) == ".function") {
			int id;
			std::stringstream ss(line.substr(9));
			ss >> id;
			funcs[id] = new FunctionObject(std::vector<ParamSpec>(), ValueType::NOTYPE, std::vector<Expr *>(), false, std::vector<Stmt *>(), nullptr, false, nullptr);
		}
	}
	is.clear();
	is.seekg(startPos);

	// Second pass: parse everything
	Chunk *mainChunk = nullptr;
	while (std::getline(is, line)) {
		line = trim(stripComment(line));
		if (line.empty())
			continue;

		if (line.substr(0, 9) == ".function") {
			int id;
			std::stringstream ss(line.substr(9));
			ss >> id;
			FunctionObject *f = funcs[id];

			while (std::getline(is, line)) {
				line = trim(stripComment(line));
				if (line.empty())
					continue;

				if (line.substr(0, 5) == ".name") {
					std::string n = line.substr(5);
					n.erase(0, n.find_first_not_of(" \t\""));
					if (!n.empty() && n.back() == '"')
						n.pop_back();
					f->name = unescapeString(n);
				} else if (line.substr(0, 7) == ".params") {
					int pCount;
					std::stringstream pss(line.substr(7));
					pss >> pCount;
					for (int i = 0; i < pCount; i++) {
						std::getline(is, line);
						line.erase(0, line.find_first_not_of(" \t\r\n"));
						std::stringstream argss(line);
						ParamSpec p;
						std::string n;
						argss >> n;
						n.erase(0, n.find_first_not_of(" \t\""));
						if (!n.empty() && n.back() == '"')
							n.pop_back();
						p.name = unescapeString(n);
						int m, t;
						bool ic, iv, ik;
						argss >> m >> t >> ic >> iv >> ik;
						p.mode = static_cast<CopyMode>(m);
						p.type = static_cast<ValueType>(t);
						p.isConst = ic;
						p.isVariadic = iv;
						p.isKwargs = ik;
						p.defaultValue = nullptr;

						std::getline(is, line); // .default
						p.defaultChunk = loadChunkAssembly(is, funcs);
						f->params.push_back(p);
					}
				} else if (line.substr(0, 11) == ".returnType") {
					int v;
					std::stringstream pss(line.substr(11));
					pss >> v;
					f->returnType = static_cast<ValueType>(v);
				} else if (line.substr(0, 13) == ".returnsConst") {
					int v;
					std::stringstream pss(line.substr(13));
					pss >> v;
					f->returnsConst = v;
				} else if (line.substr(0, 11) == ".returnsRef") {
					f->returnsRef = std::stoi(line.substr(11));
				} else if (line.substr(0, 9) == ".isCached") {
					int v;
					std::stringstream pss(line.substr(9));
					pss >> v;
					f->isCached = v;
				} else if (line.substr(0, 10) == ".retChunks") {
					int v;
					std::stringstream pss(line.substr(10));
					pss >> v;
					for (int i = 0; i < v; i++) {
						std::getline(is, line); // .retchunk
						f->defaultRetChunks.push_back(loadChunkAssembly(is, funcs));
					}
				} else if (line.substr(0, 5) == ".body") {
					f->chunk = loadChunkAssembly(is, funcs);
					break;
				}
			}
		} else if (line == ".main") {
			mainChunk = loadChunkAssembly(is, funcs);
		}
	}
	return mainChunk;
}
