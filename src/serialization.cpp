#include "ylang/serialization.hpp"

void saveString(const std::string &str, std::ostream &os) {
	uint32_t len = str.length();
	os.write(reinterpret_cast<const char *>(&len), sizeof(len));
	if (len > 0) {
		os.write(str.c_str(), len);
	}
}

std::string loadString(std::istream &is) {
	uint32_t len = 0;
	is.read(reinterpret_cast<char *>(&len), sizeof(len));
	if (len == 0)
		return "";
	std::string str(len, '\0');
	is.read(&str[0], len);
	return str;
}

void saveChunk(Chunk *chunk, std::ostream &os) {
	if (!chunk) {
		uint32_t zero = 0;
		os.write(reinterpret_cast<const char *>(&zero), sizeof(zero));
		return;
	}
	uint32_t codeSize = chunk->code.size();
	os.write(reinterpret_cast<const char *>(&codeSize), sizeof(codeSize));
	if (codeSize > 0)
		os.write(reinterpret_cast<const char *>(chunk->code.data()), codeSize);

	uint32_t constSize = chunk->constants.size();
	os.write(reinterpret_cast<const char *>(&constSize), sizeof(constSize));
	for (const Value &v : chunk->constants) {
		saveValue(v, os);
	}

	uint32_t linesSize = chunk->lines.size();
	os.write(reinterpret_cast<const char *>(&linesSize), sizeof(linesSize));
	if (linesSize > 0)
		os.write(reinterpret_cast<const char *>(chunk->lines.data()), linesSize * sizeof(int));

	uint32_t colsSize = chunk->columns.size();
	os.write(reinterpret_cast<const char *>(&colsSize), sizeof(colsSize));
	if (colsSize > 0)
		os.write(reinterpret_cast<const char *>(chunk->columns.data()), colsSize * sizeof(int));
}

Chunk *loadChunk(std::istream &is) {
	uint32_t codeSize = 0;
	is.read(reinterpret_cast<char *>(&codeSize), sizeof(codeSize));

	Chunk *chunk = new Chunk();
	if (codeSize > 0) {
		chunk->code.resize(codeSize);
		is.read(reinterpret_cast<char *>(chunk->code.data()), codeSize);
	}

	uint32_t constSize = 0;
	is.read(reinterpret_cast<char *>(&constSize), sizeof(constSize));
	for (uint32_t i = 0; i < constSize; ++i) {
		chunk->constants.push_back(loadValue(is));
	}

	uint32_t linesSize = 0;
	is.read(reinterpret_cast<char *>(&linesSize), sizeof(linesSize));
	if (linesSize > 0) {
		chunk->lines.resize(linesSize);
		is.read(reinterpret_cast<char *>(chunk->lines.data()), linesSize * sizeof(int));
	}

	uint32_t colsSize = 0;
	is.read(reinterpret_cast<char *>(&colsSize), sizeof(colsSize));
	if (colsSize > 0) {
		chunk->columns.resize(colsSize);
		is.read(reinterpret_cast<char *>(chunk->columns.data()), colsSize * sizeof(int));
	}
	return chunk;
}

void saveFunction(FunctionObject *func, std::ostream &os) {
	uint32_t numParams = func->params.size();
	os.write(reinterpret_cast<const char *>(&numParams), sizeof(numParams));
	for (auto &p : func->params) {
		saveString(p.name, os);
		os.write(reinterpret_cast<const char *>(&p.mode), sizeof(p.mode));
		os.write(reinterpret_cast<const char *>(&p.type), sizeof(p.type));
		os.write(reinterpret_cast<const char *>(&p.isConst), sizeof(p.isConst));
		os.write(reinterpret_cast<const char *>(&p.isVariadic), sizeof(p.isVariadic));
		os.write(reinterpret_cast<const char *>(&p.isKwargs), sizeof(p.isKwargs));
		bool hasDefault = p.defaultChunk != nullptr;
		os.write(reinterpret_cast<const char *>(&hasDefault), sizeof(hasDefault));
		if (hasDefault)
			saveChunk(p.defaultChunk, os);
	}
	os.write(reinterpret_cast<const char *>(&func->returnType), sizeof(func->returnType));

	uint32_t numRetChunks = func->defaultRetChunks.size();
	os.write(reinterpret_cast<const char *>(&numRetChunks), sizeof(numRetChunks));
	for (auto *rc : func->defaultRetChunks) {
		saveChunk(rc, os);
	}

	os.write(reinterpret_cast<const char *>(&func->returnsConst), sizeof(func->returnsConst));
	os.write(reinterpret_cast<const char *>(&func->isCached), sizeof(func->isCached));
	os.write(reinterpret_cast<const char *>(&func->returnsRef), sizeof(func->returnsRef));
	saveString(func->name, os);

	bool hasChunk = func->chunk != nullptr;
	os.write(reinterpret_cast<const char *>(&hasChunk), sizeof(hasChunk));
	if (hasChunk)
		saveChunk(func->chunk, os);
}

FunctionObject *loadFunction(std::istream &is) {
	uint32_t numParams = 0;
	is.read(reinterpret_cast<char *>(&numParams), sizeof(numParams));
	std::vector<ParamSpec> params;
	for (uint32_t i = 0; i < numParams; ++i) {
		ParamSpec p;
		p.name = loadString(is);
		is.read(reinterpret_cast<char *>(&p.mode), sizeof(p.mode));
		is.read(reinterpret_cast<char *>(&p.type), sizeof(p.type));
		is.read(reinterpret_cast<char *>(&p.isConst), sizeof(p.isConst));
		is.read(reinterpret_cast<char *>(&p.isVariadic), sizeof(p.isVariadic));
		is.read(reinterpret_cast<char *>(&p.isKwargs), sizeof(p.isKwargs));
		bool hasDefault = false;
		is.read(reinterpret_cast<char *>(&hasDefault), sizeof(hasDefault));
		if (hasDefault)
			p.defaultChunk = loadChunk(is);
		else
			p.defaultChunk = nullptr;
		p.defaultValue = nullptr;
		params.push_back(p);
	}

	ValueType returnType;
	is.read(reinterpret_cast<char *>(&returnType), sizeof(returnType));

	uint32_t numRetChunks = 0;
	is.read(reinterpret_cast<char *>(&numRetChunks), sizeof(numRetChunks));
	std::vector<Chunk *> defaultRetChunks;
	for (uint32_t i = 0; i < numRetChunks; ++i) {
		defaultRetChunks.push_back(loadChunk(is));
	}

	bool returnsConst;
	is.read(reinterpret_cast<char *>(&returnsConst), sizeof(returnsConst));
	bool isCached;
	is.read(reinterpret_cast<char *>(&isCached), sizeof(isCached));
	bool returnsRef = false;
	is.read(reinterpret_cast<char *>(&returnsRef), sizeof(returnsRef));
	std::string name = loadString(is);

	bool hasChunk = false;
	is.read(reinterpret_cast<char *>(&hasChunk), sizeof(hasChunk));
	Chunk *chunk = nullptr;
	if (hasChunk)
		chunk = loadChunk(is);

	FunctionObject *func = new FunctionObject(params, returnType, std::vector<Expr *>(), returnsConst, std::vector<Stmt *>(), nullptr, isCached, chunk, returnsRef);
	func->defaultRetChunks = defaultRetChunks;
	func->name = name;
	return func;
}

void saveValue(const Value &v, std::ostream &os) {
	uint8_t type = static_cast<uint8_t>(v.type);
	os.write(reinterpret_cast<const char *>(&type), sizeof(type));
	os.write(reinterpret_cast<const char *>(&v.isConst), sizeof(v.isConst));

	switch (v.type) {
	case ValueType::INT:
		os.write(reinterpret_cast<const char *>(&v.iVal), sizeof(v.iVal));
		break;
	case ValueType::FLOAT:
		os.write(reinterpret_cast<const char *>(&v.fVal), sizeof(v.fVal));
		break;
	case ValueType::BOOL:
		os.write(reinterpret_cast<const char *>(&v.bVal), sizeof(v.bVal));
		break;
	case ValueType::STRING:
		saveString(v.asString(), os);
		break;
	case ValueType::FUNCTION:
		saveFunction(static_cast<FunctionObject *>(v.ref.get()), os);
		break;
	case ValueType::BIGINT: {
		auto *big = static_cast<BigIntObject *>(v.ref.get());
		os.write(reinterpret_cast<const char *>(&big->isNegative), sizeof(big->isNegative));
		uint32_t n = big->chunks.size();
		os.write(reinterpret_cast<const char *>(&n), sizeof(n));
		os.write(reinterpret_cast<const char *>(big->chunks.data()), n * sizeof(uint32_t));
		break;
	}
	case ValueType::LIST:
		saveValues(static_cast<ListObject *>(v.ref.get())->elements, os);
		break;
	case ValueType::TUPLE:
		saveValues(static_cast<TupleObject *>(v.ref.get())->elements, os);
		break;
	case ValueType::VECTOR:
		saveValues(static_cast<VectorObject *>(v.ref.get())->elements, os);
		break;
	case ValueType::SET: {
		auto &el = static_cast<SetObject *>(v.ref.get())->elements;
		saveValues(std::vector<Value>(el.begin(), el.end()), os);
		break;
	}
	case ValueType::DICT: {
		std::vector<Value> flat;
		for (const auto &kv : static_cast<DictObject *>(v.ref.get())->items) {
			flat.push_back(kv.first);
			flat.push_back(kv.second);
		}
		saveValues(flat, os);
		break;
	}
	case ValueType::NOTYPE:
	case ValueType::NONE:
	case ValueType::OMIT_MARKER:
		break;
	default:
		// a silently skipped constant would corrupt the file; fail the compile instead
		throw std::runtime_error("cannot save a constant of type " + std::to_string((int)v.type) + " to bytecode");
	}
}

void saveValues(const std::vector<Value> &vals, std::ostream &os) {
	uint32_t n = vals.size();
	os.write(reinterpret_cast<const char *>(&n), sizeof(n));
	for (const auto &e : vals)
		saveValue(e, os);
}

std::vector<Value> loadValues(std::istream &is) {
	uint32_t n = 0;
	is.read(reinterpret_cast<char *>(&n), sizeof(n));
	std::vector<Value> vals;
	for (uint32_t i = 0; i < n && is; i++)
		vals.push_back(loadValue(is));
	return vals;
}

Value loadValue(std::istream &is) {
	uint8_t typeByte;
	is.read(reinterpret_cast<char *>(&typeByte), sizeof(typeByte));
	ValueType type = static_cast<ValueType>(typeByte);
	bool isConst;
	is.read(reinterpret_cast<char *>(&isConst), sizeof(isConst));

	Value v;
	switch (type) {
	case ValueType::INT: {
		long long i;
		is.read(reinterpret_cast<char *>(&i), sizeof(i));
		v = Value::Int(i);
		break;
	}
	case ValueType::FLOAT: {
		double d;
		is.read(reinterpret_cast<char *>(&d), sizeof(d));
		v = Value::Float(d);
		break;
	}
	case ValueType::BOOL: {
		bool b;
		is.read(reinterpret_cast<char *>(&b), sizeof(b));
		v = Value::Bool(b);
		break;
	}
	case ValueType::STRING: {
		v = Value::String(loadString(is));
		break;
	}
	case ValueType::FUNCTION: {
		FunctionObject *func = loadFunction(is);
		v.type = ValueType::FUNCTION;
		v.ref = std::shared_ptr<HeapObject>(func);
		break;
	}
	case ValueType::NOTYPE:
		v = Value::NoType();
		break;
	case ValueType::NONE:
		v = Value::None();
		break;
	case ValueType::OMIT_MARKER:
		v.type = ValueType::OMIT_MARKER;
		break;
	case ValueType::BIGINT: {
		bool neg = false;
		uint32_t n = 0;
		is.read(reinterpret_cast<char *>(&neg), sizeof(neg));
		is.read(reinterpret_cast<char *>(&n), sizeof(n));
		std::vector<uint32_t> chunks(n);
		is.read(reinterpret_cast<char *>(chunks.data()), n * sizeof(uint32_t));
		v = Value::BigInt(chunks, neg);
		break;
	}
	case ValueType::LIST:
		v = Value::List(loadValues(is));
		break;
	case ValueType::TUPLE:
		v = Value::Tuple(loadValues(is));
		break;
	case ValueType::VECTOR:
		v = Value::Vector(loadValues(is));
		break;
	case ValueType::SET: {
		auto el = loadValues(is);
		v = Value::Set(std::unordered_set<Value, ValueHash, ValueEqual>(el.begin(), el.end()));
		break;
	}
	case ValueType::DICT: {
		auto flat = loadValues(is);
		std::unordered_map<Value, Value, ValueHash, ValueEqual> m;
		for (size_t i = 0; i + 1 < flat.size(); i += 2)
			m[flat[i]] = flat[i + 1];
		v = Value::Dict(m);
		break;
	}
	default:
		throw std::runtime_error("corrupt or unsupported bytecode file (value type " + std::to_string((int)type) + ")");
	}
	v.isConst = isConst;
	return v;
}
