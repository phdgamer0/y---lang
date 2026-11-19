#pragma once
#ifndef YLANG_OBJECTS_HPP
#define YLANG_OBJECTS_HPP
#include "ast.hpp"
#include "errors.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "types.hpp"

struct Env;
struct HeapObject;
struct BigIntObject;
struct ErrorObject;
struct ClassObject;
struct InstanceObject;
using NativeFunc = std::function<Value(const std::vector<Value> &, int, int)>;
using Inspector = std::function<std::string(const Value &)>;
struct ValueHash {
	std::size_t operator()(const Value &v) const;
};
struct ValueEqual {
	bool operator()(const Value &a, const Value &b) const;
};
struct Value {
	ValueType type = ValueType::NOTYPE;
	bool isConst = false;
	bool isLocked = false;
#ifdef VM_DEBUG_MODE
	std::string __DEBUGGING__NAME__ = "";
#endif
	union {
		long long iVal;
		double fVal;
		bool bVal;
	};
	union {
		Value *ptr;
		void *adress;
	};
	std::shared_ptr<HeapObject> ref;
	static Value Reference(Value *p, std::shared_ptr<HeapObject> owner);
	static Value Int(long long v, bool locked = false, bool isConst = false);
	static Value pInt(void *v, bool locked = false, bool isConst = false);
	static Value BigInt(long long n);
	static Value BigInt(std::vector<uint32_t> chunks, bool isNegative);
	static Value BigInt(std::shared_ptr<BigIntObject> obj);
	static Value BigInt(const std::string &str);
	static Value Float(double v, bool locked = false, bool isConst = false);
	static Value Bool(bool v, bool locked = false, bool isConst = false);
	static Value String(const string &v, bool locked = false,
		bool isConst = false);
	static Value None();
	static Value NoType();
	static Value List();
	static Value List(const std::vector<Value> &elems, bool locked = false,
		bool isConst = false);
	static Value Range(double s, double e, double st, bool si, bool ei, bool f,
		bool locked = false, bool isConst = false);
	static Value Set();
	static Value Set(const std::unordered_set<Value, ValueHash, ValueEqual> &elems, bool locked = false);
	static Value Tuple(const std::vector<Value> &elems, bool locked = false);
	static Value Dict(const std::unordered_map<Value, Value, ValueHash, ValueEqual> &m, bool locked = false);
	static Value Paired(const std::vector<std::pair<Value, Value>> &p);
	static Value Native(NativeFunc f);
	static Value Overload(const Value &first);
	static Value Omit();
	static Value File(const string &path);
	static Value Slice(Value s, Value e, Value p);
	static Value Vector(const std::vector<Value> &elems);
	static Value Error(std::shared_ptr<ErrorObject> e);
	static Value Class(const string &name);
	static Value Instance(Value classObj);
	static Value FromExisting(std::shared_ptr<HeapObject> existingRef, ValueType type);
	bool isTruthy() const;
	bool strictEquals(const Value &other) const;
	bool looseEquals(const Value &other) const;
	bool isNumber() const;
	long long asInt() const;
	void *aspInt() const;
	double asFloat() const;
	bool asBool() const;
	const string &asString() const;
	bool sameType(const Value &other) const { return type == other.type; }

	Value *get_ptr_safe() const;
};
inline HeapObject *gc_head = nullptr;
inline int gc_alloc_count = 0;
inline int gc_delete_count = 0;

struct HeapObject {
	std::string name = "";
	ValueType type;
	bool typeLocked = false;
	bool gc_marked = false;
	HeapObject *gc_next = nullptr;
	HeapObject *gc_prev = nullptr;

	HeapObject(ValueType t, bool locked = false) : type(t), typeLocked(locked) {
		if (gc_head) {
			gc_head->gc_prev = this;
			this->gc_next = gc_head;
		}
		gc_head = this;
		gc_alloc_count++;
	}

	HeapObject(const HeapObject &other) : name(other.name), type(other.type), typeLocked(other.typeLocked) {
		if (gc_head) {
			gc_head->gc_prev = this;
			this->gc_next = gc_head;
		}
		gc_head = this;
		gc_alloc_count++;
	}

	HeapObject(HeapObject &&other) noexcept : name(std::move(other.name)), type(other.type), typeLocked(other.typeLocked) {
		if (gc_head) {
			gc_head->gc_prev = this;
			this->gc_next = gc_head;
		}
		gc_head = this;
		gc_alloc_count++;
	}

	HeapObject &operator=(const HeapObject &other) {
		if (this != &other) {
			name = other.name;
			type = other.type;
			typeLocked = other.typeLocked;
		}
		return *this;
	}

	HeapObject &operator=(HeapObject &&other) noexcept {
		if (this != &other) {
			name = std::move(other.name);
			type = other.type;
			typeLocked = other.typeLocked;
		}
		return *this;
	}

	virtual ~HeapObject() {
		if (gc_prev)
			gc_prev->gc_next = gc_next;
		if (gc_next)
			gc_next->gc_prev = gc_prev;
		if (gc_head == this)
			gc_head = gc_next;
		gc_delete_count++;
	}
	virtual void breakCycles() {}
};
struct StringObject : HeapObject {
	string value;
	StringObject(const string &v, bool locked = false)
		 : HeapObject(ValueType::STRING, locked), value(v) {}
};
struct ListObject : HeapObject {
	std::vector<Value> elements;
	ListObject() : HeapObject(ValueType::LIST) {}
	ListObject(const std::vector<Value> &elems, bool locked = false)
		 : HeapObject(ValueType::LIST, locked), elements(elems) {}
	void breakCycles() override { elements.clear(); }
};
struct RangeObject : HeapObject {
	double start, end, step;
	bool startInclusive, endInclusive, isFloat, isValid;
	RangeObject(double s, double e, double st, bool si, bool ei, bool f,
		bool valid, bool locked = false)
		 : HeapObject(ValueType::RANGE, locked),
			start(s),
			end(e),
			step(st),
			startInclusive(si),
			endInclusive(ei),
			isFloat(f),
			isValid(valid) {}
};
struct SetObject : HeapObject {
	std::unordered_set<Value, ValueHash, ValueEqual> elements;
	SetObject() : HeapObject(ValueType::SET) {}
	SetObject(const std::unordered_set<Value, ValueHash, ValueEqual> &e,
		bool locked = false)
		 : HeapObject(ValueType::SET, locked), elements(e) {}
	void breakCycles() override { elements.clear(); }
};
struct TupleObject : HeapObject {
	std::vector<Value> elements;
	TupleObject() : HeapObject(ValueType::TUPLE) {}
	TupleObject(const std::vector<Value> &elems, bool locked = false)
		 : HeapObject(ValueType::TUPLE, locked), elements(elems) {}
	void breakCycles() override { elements.clear(); }
};
struct DictObject : HeapObject {
	std::unordered_map<Value, Value, ValueHash, ValueEqual> items;
	DictObject() : HeapObject(ValueType::DICT) {}
	DictObject(const std::unordered_map<Value, Value, ValueHash, ValueEqual> &m,
		bool locked = false)
		 : HeapObject(ValueType::DICT, locked), items(m) {}
	void breakCycles() override { items.clear(); }
};
struct PairedObject : HeapObject {
	std::vector<std::pair<Value, Value>> pairs;
	PairedObject(const std::vector<std::pair<Value, Value>> &p)
		 : HeapObject(ValueType::PAIRED), pairs(p) {}
	void breakCycles() override { pairs.clear(); }
};
struct NativeFunctionObject : HeapObject {
	NativeFunc func;
	NativeFunctionObject(NativeFunc f)
		 : HeapObject(ValueType::NATIVE_FUNCTION), func(f) {}
};
struct OverloadObject : HeapObject {
	std::vector<Value> overloads;
	std::unordered_map<std::size_t, Value> signatureCache;
	OverloadObject(const Value &v) : HeapObject(ValueType::OVERLOAD) {
		overloads.push_back(v);
	}
	void breakCycles() override {
		overloads.clear();
		signatureCache.clear();
	}
};
struct FileObject : HeapObject {
	std::fstream stream;
	string path;
	bool isOpen;
	FileObject(const string &p) : HeapObject(ValueType::FILE), path(p) {
		stream.open(p, std::ios::in | std::ios::out | std::ios::binary);
		isOpen = stream.is_open();
	}
	void Reset() {
		stream.close();
		stream.open(path, std::ios::in | std::ios::out | std::ios::binary |
									std::ios::trunc);
		isOpen = stream.is_open();
	}
	~FileObject() {
		if (isOpen)
			stream.close();
	}
};
struct SliceObject : HeapObject {
	Value start;
	Value end;
	Value step;
	SliceObject(Value s, Value e, Value p)
		 : HeapObject(ValueType::SLICE), start(s), end(e), step(p) {}
};
struct VectorObject : HeapObject {
	vector<Value> elements;
	VectorObject(const vector<Value> &e)
		 : HeapObject(ValueType::VECTOR), elements(e) {}
	void breakCycles() override { elements.clear(); }
};
struct BigIntObject : HeapObject {
	bool isNegative;
	std::vector<uint32_t> chunks;
	BigIntObject(long long n);
	BigIntObject(std::vector<uint32_t> c, bool neg)
		 : HeapObject(ValueType::BIGINT), chunks(c), isNegative(neg) {
		trim();
	}
	void trim() {
		while (chunks.size() > 1 && chunks.back() == 0)
			chunks.pop_back();
		if (chunks.size() == 1 && chunks[0] == 0)
			isNegative = false;
	}
	bool operator==(const BigIntObject &other) const {
		return isNegative == other.isNegative && chunks == other.chunks;
	}
	bool operator<(const BigIntObject &other) const;
	bool operator>(const BigIntObject &other) const { return other < *this; }
	bool absLess(const BigIntObject &other) const {
		if (chunks.size() != other.chunks.size())
			return chunks.size() < other.chunks.size();
		for (int i = chunks.size() - 1; i >= 0; i--)
			if (chunks[i] != other.chunks[i])
				return chunks[i] < other.chunks[i];
		return false;
	}
	BigIntObject absAdd(const BigIntObject &other) const;
	BigIntObject absSub(const BigIntObject &other) const;
	BigIntObject operator+(const BigIntObject &other) const;
	BigIntObject operator-(const BigIntObject &other) const;
	static std::vector<uint32_t> karatsubaMul(const std::vector<uint32_t> &A, const std::vector<uint32_t> &B);

	BigIntObject operator*(const BigIntObject &other) const {
		return BigIntObject(karatsubaMul(chunks, other.chunks), isNegative != other.isNegative);
	}
	std::pair<BigIntObject, BigIntObject> divMod(const BigIntObject &other) const;
	BigIntObject operator/(const BigIntObject &other) const {
		return divMod(other).first;
	}
	BigIntObject operator%(const BigIntObject &other) const {
		return divMod(other).second;
	}
	void lshift(int shift);
	void setBit(int n) {
		int chunkIdx = n / 32;
		int bitIdx = n % 32;
		if (chunkIdx >= chunks.size())
			chunks.resize(chunkIdx + 1, 0);
		chunks[chunkIdx] |= (1U << bitIdx);
	}
	BigIntObject operator&(const BigIntObject &other) const;
	BigIntObject operator|(const BigIntObject &other) const;
	BigIntObject operator^(const BigIntObject &other) const;
	BigIntObject operator<<(long long shift) const;
	BigIntObject operator>>(long long shift) const;
	static Value add(Value a, Value b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(a.ref.get())
									 : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(b.ref.get())
									 : &(tempB = BigIntObject(b.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba + *bb));
	}
	static Value sub(Value a, Value b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(a.ref.get())
									 : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(b.ref.get())
									 : &(tempB = BigIntObject(b.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba - *bb));
	}
	static Value mul(Value a, Value b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(a.ref.get())
									 : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(b.ref.get())
									 : &(tempB = BigIntObject(b.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba * *bb));
	}
	static Value div(Value a, Value b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(a.ref.get())
									 : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(b.ref.get())
									 : &(tempB = BigIntObject(b.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba / *bb));
	}
	static Value mod(Value a, Value b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(a.ref.get())
									 : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(b.ref.get())
									 : &(tempB = BigIntObject(b.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba % *bb));
	}
	static Value bit_and(const Value &a, const Value &b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT) ? static_cast<BigIntObject *>(a.ref.get()) : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT) ? static_cast<BigIntObject *>(b.ref.get()) : &(tempB = BigIntObject(b.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba & *bb));
	}
	static Value bit_or(const Value &a, const Value &b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT) ? static_cast<BigIntObject *>(a.ref.get()) : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT) ? static_cast<BigIntObject *>(b.ref.get()) : &(tempB = BigIntObject(b.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba | *bb));
	}
	static Value bit_xor(const Value &a, const Value &b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT) ? static_cast<BigIntObject *>(a.ref.get()) : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT) ? static_cast<BigIntObject *>(b.ref.get()) : &(tempB = BigIntObject(b.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba ^ *bb));
	}
	static Value bit_shl(const Value &a, const Value &b) {
		BigIntObject tempA(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT) ? static_cast<BigIntObject *>(a.ref.get()) : &(tempA = BigIntObject(a.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba << b.asInt()));
	}
	static Value bit_shr(const Value &a, const Value &b) {
		BigIntObject tempA(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT) ? static_cast<BigIntObject *>(a.ref.get()) : &(tempA = BigIntObject(a.asInt()));
		return Value::BigInt(std::make_shared<BigIntObject>(*ba >> b.asInt()));
	}
	static Value pow(Value a, Value b);
};
struct ErrorObject : HeapObject {
	string message;
	string errType;
	long long code;
	int line, col;
	ErrorObject(string type, string msg, long long co, int l, int c)
		 : HeapObject(ValueType::ERROR),
			errType(type),
			message(msg),
			code(co),
			line(l),
			col(c) {}
	string toString() const { return errType + ": " + message; }
};
struct ClassObject : HeapObject {
	string name;
	vector<Value> parents;
	vector<ClassObject *> mro;
	unordered_map<string, Value> staticFields;
	unordered_map<std::string, AccessLevel> fieldAccess;
	struct MethodInfo {
		Value func;
		AccessLevel access;
	};
	unordered_map<string, MethodInfo> methods;
	ClassObject(const string &n) : HeapObject(ValueType::CLASS), name(n) {
		mro.push_back(this);
	}
	void computeMRO();
	string toString() const { return "<class '" + name + "'>"; }
	void breakCycles() override {
		parents.clear();
		mro.clear();
		staticFields.clear();
		methods.clear();
	}
};
struct InstanceObject : HeapObject {
	ClassObject *klass;
	unordered_map<string, Value> fields;
	InstanceObject(ClassObject *k) : HeapObject(ValueType::INSTANCE), klass(k) {}
	void breakCycles() override { fields.clear(); }
	string toString() const { return "<instance of '" + klass->name + "'>"; }
};
struct RefObject : HeapObject {
	RefObject() : HeapObject(ValueType::REFERENCE) {}
	virtual Value *get_ptr() = 0;
};
struct ListRefObject : RefObject {
	std::shared_ptr<ListObject> list;
	int index;
	ListRefObject(std::shared_ptr<ListObject> l, int i)
		 : list(l), index(i) {}
	Value *get_ptr() override {
		if (index < 0 || index >= (int)list->elements.size())
			throw IndexError("Reference to list element out of range", 0, 0);
		return &list->elements[index];
	}
	void breakCycles() override { list.reset(); }
};
struct VarRefObject : RefObject {
	std::shared_ptr<Env> env;
	std::string name;
	VarRefObject(std::shared_ptr<Env> e, const std::string &n)
		 : env(e), name(n) {}
	Value *get_ptr() override;
	void breakCycles() override { env.reset(); }
};
struct PropRefObject : RefObject {
	std::shared_ptr<InstanceObject> inst;
	std::string name;
	PropRefObject(std::shared_ptr<InstanceObject> i, const std::string &n)
		 : inst(i), name(n) {}
	Value *get_ptr() override { return &inst->fields[name]; }
	void breakCycles() override { inst.reset(); }
};
struct DictRefObject : RefObject {
	std::shared_ptr<DictObject> dict;
	Value key;
	DictRefObject(std::shared_ptr<DictObject> d, const Value &k)
		 : dict(d), key(k) {}
	Value *get_ptr() override { return &dict->items[key]; }
	void breakCycles() override { dict.reset(); }
};
struct SuperObject : HeapObject {
	Value instance;
	ClassObject *startClass;
	SuperObject(Value inst, ClassObject *start)
		 : HeapObject(ValueType::SUPER), instance(inst), startClass(start) {}
	string toString() const { return "<super: " + startClass->name + ">"; }
};
inline Value Value::Reference(Value *p, std::shared_ptr<HeapObject> owner) {
	Value v;
	v.type = ValueType::REFERENCE;
	v.ptr = p;
	v.ref = owner;
	return v;
}
inline Value Value::Int(long long v, bool locked, bool isConst) {
	Value x;
	x.type = ValueType::INT;
	x.iVal = v;
	x.isConst = isConst;
	return x;
}
inline Value Value::pInt(void *v, bool locked, bool isConst) {
	Value x;
	x.type = ValueType::INT;
	x.adress = v;
	x.iVal = (long long)v;
	x.isConst = isConst;
	return x;
}
inline Value Value::BigInt(long long n) {
	Value v;
	v.type = ValueType::BIGINT;
	v.ref = std::make_shared<BigIntObject>(n);
	return v;
}
inline Value Value::BigInt(std::vector<uint32_t> chunks, bool isNegative) {
	Value v;
	v.type = ValueType::BIGINT;
	v.ref = std::make_shared<BigIntObject>(chunks, isNegative);
	return v;
}
inline Value Value::BigInt(std::shared_ptr<BigIntObject> obj) {
	Value v;
	v.type = ValueType::BIGINT;
	v.ref = obj;
	return v;
}

inline Value Value::Float(double v, bool locked, bool isConst) {
	Value x;
	x.type = ValueType::FLOAT;
	x.fVal = v;
	x.isConst = isConst;
	return x;
}
inline Value Value::Bool(bool v, bool locked, bool isConst) {
	Value x;
	x.type = ValueType::BOOL;
	x.bVal = v;
	x.isConst = isConst;
	return x;
}
inline Value Value::String(const string &v, bool locked, bool isConst) {
	Value x;
	x.type = ValueType::STRING;
	x.ref = std::make_shared<StringObject>(v, locked);
	return x;
}
inline Value Value::None() {
	Value x;
	x.type = ValueType::NONE;
	return x;
}
inline Value Value::NoType() {
	Value x;
	x.type = ValueType::NOTYPE;
	return x;
}
inline Value Value::List() {
	Value x;
	x.type = ValueType::LIST;
	x.ref = std::make_shared<ListObject>();
	return x;
}
inline Value Value::List(const std::vector<Value> &elems, bool locked, bool isConst) {
	Value x;
	x.type = ValueType::LIST;
	x.ref = std::make_shared<ListObject>(elems, locked);
	return x;
}
inline Value Value::Set() {
	Value x;
	x.type = ValueType::SET;
	x.ref = std::make_shared<SetObject>();
	return x;
}
inline Value Value::Set(const std::unordered_set<Value, ValueHash, ValueEqual> &elems, bool locked) {
	Value x;
	x.type = ValueType::SET;
	x.ref = std::make_shared<SetObject>(elems, locked);
	return x;
}
inline Value Value::Tuple(const std::vector<Value> &elems, bool locked) {
	Value x;
	x.type = ValueType::TUPLE;
	std::vector<Value> constElems = elems;
	for (auto &e : constElems)
		e.isConst = true;
	x.ref = std::make_shared<TupleObject>(constElems, locked);
	return x;
}
inline Value Value::Dict(const std::unordered_map<Value, Value, ValueHash, ValueEqual> &m, bool locked) {
	Value x;
	x.type = ValueType::DICT;
	x.ref = std::make_shared<DictObject>(m, locked);
	return x;
}
inline Value Value::Paired(const std::vector<std::pair<Value, Value>> &p) {
	Value x;
	x.type = ValueType::PAIRED;
	x.ref = std::make_shared<PairedObject>(p);
	return x;
}
inline Value Value::Native(NativeFunc f) {
	Value x;
	x.type = ValueType::NATIVE_FUNCTION;
	x.ref = std::make_shared<NativeFunctionObject>(f);
	return x;
}
inline Value Value::Overload(const Value &first) {
	Value x;
	x.type = ValueType::OVERLOAD;
	x.ref = std::make_shared<OverloadObject>(first);
	return x;
}
inline Value Value::Omit() {
	Value v;
	v.type = ValueType::OMIT_MARKER;
	return v;
}
inline Value Value::File(const string &path) {
	Value x;
	x.type = ValueType::FILE;
	x.ref = std::make_shared<FileObject>(path);
	return x;
}

inline Value Value::Slice(Value s, Value e, Value p) {
	Value x;
	x.type = ValueType::SLICE;
	x.ref = std::make_shared<SliceObject>(s, e, p);
	return x;
}
inline Value Value::Vector(const std::vector<Value> &elems) {
	Value x;
	x.type = ValueType::VECTOR;
	x.ref = std::make_shared<VectorObject>(elems);
	return x;
}
inline Value Value::Error(std::shared_ptr<ErrorObject> e) {
	Value v;
	v.type = ValueType::ERROR;
	v.ref = e;
#ifdef VM_DEBUG_MODE
	v.__DEBUGGING__NAME__ = e->errType;
#endif
	return v;
}
inline Value Value::Class(const string &name) {
	Value v;
#ifdef VM_DEBUG_MODE
	v.__DEBUGGING__NAME__ = name;
#endif
	v.type = ValueType::CLASS;
	v.ref = make_shared<ClassObject>(name);
	return v;
}


inline long long Value::asInt() const {
	if (type == ValueType::INT)
		return iVal;
	if (type == ValueType::FLOAT)
		return (long long)fVal;
	if (type == ValueType::BOOL)
		return bVal ? 1 : 0;
	return 0;
}
inline void *Value::aspInt() const { return adress; }
inline double Value::asFloat() const {
	if (type == ValueType::FLOAT)
		return fVal;
	if (type == ValueType::INT)
		return (double)iVal;
	if (type == ValueType::BOOL)
		return bVal ? 1.0 : 0.0;
	return 0.0;
}
inline bool Value::asBool() const {
	if (type == ValueType::BOOL)
		return bVal;
	if (type == ValueType::INT)
		return iVal != 0;
	if (type == ValueType::FLOAT)
		return fVal != 0.0;
	return isTruthy();
}
inline const string &Value::asString() const {
	static string empty = "";
	if (type == ValueType::STRING)
		return static_cast<StringObject *>(ref.get())->value;
	return empty;
}
inline bool Value::isNumber() const {
	return type == ValueType::INT || type == ValueType::FLOAT ||
			 type == ValueType::BIGINT;
}

inline bool Value::looseEquals(const Value &other) const {
	return isTruthy() == other.isTruthy();
}
inline Value Value::FromExisting(std::shared_ptr<HeapObject> existingRef, ValueType type) {
	Value v;
	v.type = type;
	v.ref = std::move(existingRef);
	return v;
}

inline bool ValueEqual::operator()(const Value &a, const Value &b) const {
	return a.strictEquals(b);
}
struct VectorHash {
	std::size_t operator()(const std::vector<Value> &vec) const {
		std::size_t seed = vec.size();
		ValueHash hasher;
		for (const auto &v : vec)
			seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};
struct VectorEqual {
	bool operator()(const std::vector<Value> &a,
		const std::vector<Value> &b) const {
		if (a.size() != b.size())
			return false;
		for (size_t i = 0; i < a.size(); ++i)
			if (!(a[i].strictEquals(b[i])))
				return false;
		return true;
	}
};
static void setAdd(std::unordered_set<Value, ValueHash, ValueEqual> &elems, const Value &v);
Value shallowCopy(const Value &val);
Value deepCopy(const Value &val);
Value applyCopy(const Value &v, CopyMode mode);
Value defaultOf(ValueType t);
inline int getGlobalVarId(const std::string &name) {
	static std::unordered_map<std::string, int> pool;
	static int nextId = 0;
	if (pool.find(name) == pool.end())
		pool[name] = nextId++;
	return pool[name];
}
struct Var {
	Value value;
	Value *alias = nullptr;
	bool isConst = false;
	bool isLocked = false;
};
struct LValue {
	Value *ref = nullptr;
	bool isRefTarget = false;
	bool isConstView = false;
	bool isLocked = false;
	LValue() = default;
	LValue(Value *r, bool i, bool c = false, bool l = false)
		 : ref(r), isRefTarget(i), isConstView(c), isLocked(l) {}
};
struct CallArg {
	Value value;
	LValue lvalue;
	bool hasLValue = false;
};
struct Env {
	unordered_map<string, Var> vars;
	std::shared_ptr<Env> parent = nullptr;
	void clear() {
		vars.clear();
	}
	Var &lookup(const string &n) {
		auto it = vars.find(n);
		if (it != vars.end())
			return it->second;
		if (parent)
			return parent->lookup(n);
		static Var nullVar;
		return nullVar;
	}
	bool existsLocal(const string &n) { return vars.find(n) != vars.end(); }
	void set(const string &n, Value v, bool locked, bool isConstVar = false);
	Value get(const string &n);
	void setAlias(const string &n, Value *target, bool locked = false, bool isConstVar = false) {
		Var v;
		v.alias = target;
		v.isConst = isConstVar;
		v.isLocked = locked;
		vars[n] = v;
	}
	void assign(const string &n, Value v);
	bool exists(const string &n) {
		if (vars.count(n))
			return true;
		if (parent)
			return parent->exists(n); // <--- Recursively check parent
		return false;
	}
};

inline Value *VarRefObject::get_ptr() {
	auto &var = env->lookup(name);
	if (var.alias)
		return var.alias;
	if (var.value.type == ValueType::REFERENCE && var.value.get_ptr_safe())
		return var.value.get_ptr_safe();
	return &var.value;
}

inline Value *Value::get_ptr_safe() const {
	if (ref && ref->type == ValueType::REFERENCE)
		return static_cast<RefObject *>(ref.get())->get_ptr();
	return ptr;
}
static inline uint32_t divMod1e9(std::vector<uint32_t> &chunks) {
	uint64_t remainder = 0;
	for (int i = chunks.size() - 1; i >= 0; i--) {
		uint64_t combined = (remainder << 32) | chunks[i];
		chunks[i] = (uint32_t)(combined / 1000000000ULL);
		remainder = combined % 1000000000ULL;
	}
	while (chunks.size() > 1 && chunks.back() == 0)
		chunks.pop_back();
	return (uint32_t)remainder;
}
std::string bigIntToString(BigIntObject *big);
static inline std::string formatNumber(double val) {
	std::string s = std::to_string(val);
	s.erase(s.find_last_not_of('0') + 1, std::string::npos);
	if (s.back() == '.')
		s.pop_back();
	return s;
}
static inline std::string ptr_to_string(void *p) {
	std::ostringstream oss;
	oss << p;
	return oss.str();
}
std::string valueToString(const Value &v, int line = 0, int col = 0);
static inline std::string PrintProperty(const Value &v) {
	if (v.isConst && v.isLocked)
		return "Locked Const ";
	else if (v.isConst)
		return "Const ";
	else if (v.isLocked)
		return "Locked ";
	else
		return "";
	return "failure";
}
std::string PrintStackForDebug(const std::vector<Value> &stack);
bool lessValue(const Value &a, const Value &b, std::shared_ptr<Env> globals, std::function<Value(MethodCallExpr *)> methodResolver, std::function<void(std::string, std::vector<std::string>, std::shared_ptr<Env>)> importResolver);
struct CacheKeyCmp {
	bool operator()(const vector<Value> &a, const vector<Value> &b, std::shared_ptr<Env> globals, std::function<Value(MethodCallExpr *)> methodResolver, std::function<void(std::string, std::vector<std::string>, std::shared_ptr<Env>)> importResolver) const {
		return std::lexicographical_compare(
			a.begin(), a.end(), b.begin(), b.end(),
			[&](const Value &x, const Value &y) { return lessValue(x, y, globals, methodResolver, importResolver); });
	}
};
struct FuncVal {
	FuncStmt *stmt;
	std::map<vector<Value>, Value, CacheKeyCmp> cache;
	FuncVal(FuncStmt *s) : stmt(s) {}
};
struct Chunk;
struct FunctionObject : HeapObject {
	vector<ParamSpec> params;
	ValueType returnType;
	vector<Expr *> defaultRetArgs;
	vector<Chunk *> defaultRetChunks;
	bool returnsConst;
	bool returnsRef = false;
	vector<Stmt *> body;
	std::shared_ptr<Env> closure;
	bool isCached;
	ClassObject *owner = nullptr;
	Chunk *chunk;
	std::unordered_map<vector<Value>, Value, VectorHash, VectorEqual> cache;
	std::unordered_map<Value, Value, ValueHash, ValueEqual> singleArgCache;
	FunctionObject(const vector<ParamSpec> &p, ValueType rt, vector<Expr *> dra,
		bool rc, const vector<Stmt *> &b, std::shared_ptr<Env> c,
		bool cached, Chunk *ch = nullptr, bool rr = false)
		 : HeapObject(ValueType::FUNCTION),
			params(p),
			returnType(rt),
			defaultRetArgs(dra),
			returnsConst(rc),
			returnsRef(rr),
			body(b),
			closure(c),
			isCached(cached),
			chunk(ch) {}
	~FunctionObject();
	void breakCycles() override {
		closure = nullptr;
		cache.clear();
		singleArgCache.clear();
	}
};
static void setAdd(std::unordered_set<Value, ValueHash, ValueEqual> &elems, const Value &v) {
	Value finalVal = deepCopy(v);
	finalVal.isConst = true;
	elems.insert(finalVal);
}
void enableColors();
void printValue(const Value &v, std::unordered_set<const HeapObject *> &seen, bool quoteStrings, Inspector *inspect = nullptr);
void printValue(const Value &v);
struct ValueExpr : public Expr {
	Value val;
	Value *sourcePtr = nullptr;
	ValueExpr(Value v, Value *src = nullptr)
		 : Expr(ExprType::NUMBER), val(v), sourcePtr(src) {}
};
enum class Magic_Methods : uint8_t {

	/*
	================================================================
	1. LIFECYCLE & IDENTITY
	Distinct from Python's "init/del", using Constructor terminology
	================================================================
	*/
	__construct__, // (was __init__)   Called on creation
	__destruct__,	// (was __del__)    Called on cleanup
	__copy__,		// (was __scopy__)  Shallow copy
	__clone__,		// (was __dcopy__)  Deep copy
	__ref__,			// (was __rcopy__)  Reference copy
	__mro__,			// Method Resolution Order

	/*
	================================================================
			  2. REPRESENTATION & DEBUGGING
			  Focus on "Displaying" vs "Inspecting"
	================================================================
	*/
	__display__, // (was __show__)   User-friendly string representation
	__inspect__, // (was __debug__)  Programmer-focused raw representation

	/*
	================================================================
			  3. ARITHMETIC (VERBS OVER ABBREVIATIONS)
			  Using full words makes it feel less like C macros
	================================================================
	*/
	// Standard           // Reverse (Right-hand side)
	__plus__,
	__r_plus__, // (was __add__)
	__minus__,
	__r_minus__, // (was __sub__)
	__times__,
	__r_times__, // (was __mul__)
	__divide__,
	__r_divide__, // (was __div__)
	__int_divide__,
	__r_int_divide__, // (was __div__)
	__power__,
	__r_power__, // (was __pow__)
	__modulo__,
	__r_modulo__, // (was __mod__ - added this, usually needed)

	/*
	================================================================
			  3.5 inplace assignment for arithmetic operators
			  Using full words makes it feel less like C macros
	================================================================
	*/
	__plus_eq__,		 // (was __iadd__)    obj1 += obj2
	__minus_eq__,		 // (was __isub__)    obj1 -= obj2
	__times_eq__,		 // (was __imul__)    obj1 *= obj2
	__divide_eq__,		 // (was __idiv__)    obj1 /= obj2
	__int_divide_eq__, // (was __idiv__)    obj1 //= obj2
	__power_eq__,		 // (was __ipow__)    obj1 **= obj
	__modulo_eq__,		 // (was __imod__)    obj1 %= obj2

	/*
	================================================================
			  4. UNARY & STATE MODIFIERS
	================================================================
	*/
	__positive__,	// (was __pos__)    +x
	__negative__,	// (was __neg__)    -x
	__increment__, // (was __inc__)    x++
	__decrement__, // (was __dec__)    x--
	__invert__,		// (was __not__)    ~x (Bitwise not)

	/*
	================================================================
			  5. LOGIC
			  Explicit naming to Boolean logic
	================================================================
	*/
	// Standard            // Reverse
	__and__,
	__r_and__, // (was __and__)
	__or__,
	__r_or__, // (was __or__)
	__xor__,
	__r_xor__, // (was __xor__)
	__nand__,
	__r_nand__, // (was __nand__)
	__nor__,
	__r_nor__, // (was __nor__)
	__nxor__,
	__r_nxor__, // (was __nxor__)

	/*
	================================================================
			  5.5 inplace assignment for bitwise operators
			  Explicit naming to separate Boolean logic from Bitwise logic
	================================================================
	*/
	__and_equals__, // (was __iand__)    obj1 &= obj2
	__or_equals__,	 // (was __ior__)     obj1 |= obj2
	__xor_equals__, // (was __ixor__)    obj1 ^= obj2

	/*
	================================================================
			  6. COMPARISON (THE JUDGES)
			  Using "is" makes the intent readable
	================================================================
	*/
	__equals__,		 // (was __eq__)     ==
	__differs__,	 // (was __neq__)    !=
	__identical__,	 // (was __seq__)    === (Strict equality)
	__distinct__,	 // (was __sneq__)   !== (Strict inequality)
	__less__,		 // (was __lt__)     <
	__less_eq__,	 // (was __lte__)    <=
	__greater__,	 // (was __gt__)     >
	__greater_eq__, // (was __gte__)    >=

	/*
	================================================================
			  7. CONTAINER & ACCESS
			  Moving away from "getitem/setitem" to "at/put" style
	================================================================
	*/
	__count__,			// (was __len__)    Size of container
	__at__,				// (was __get__)    val = obj[key]
	__put__,				// (was __set__)    obj[key] = val
	__call__,			// () operator to call like a function
	__has__,				// (was __contains__) if x is in obj
	__lacks__,			// if x is not in obj
	__missing__,		// if obj is not in x
	__not__missing__, // (was __getattr__) if obj is not in x
	__assign__,			// (was __setattr__) Called when setting a property

	/*
	================================================================
			  8. ITERATION (FLOW)
	================================================================
	*/
	__traverse__, // (was __iter__)   Returns the iterator object
	__advance__,  // (was __next__)   Moves to next item

	/*
	================================================================
			  9. CASTING (TRANSFORMATION)
			  "to" prefix implies conversion
	================================================================
	*/
	__to_int__,		// (was __int__)
	__to_float__,	// (was __float__)
	__to_string__, // (was __string__)
	__to_bool__,	// (was __bool__)
	__to_list__,	// (was __list__)
	__to_set__,		// (was __set__)
	__to_dict__,	// (was __dict__)
	__to_tuple__,	// (was __tuple__)
	__to_vector__, // (was __vector__)
	__to_range__,	// (was __range__)

	/*
	================================================================
			  10. INFO
	================================================================
	*/
	__var_count__,
	__var_names__,
	__var_values__,
	__var_pairs__,
	__var_reverse_pairs__,
	__function_count__,
	__function_names__,
	__all_count__,
	__all_names__,
};
std::string magic_methods_to_string(Magic_Methods Magic_method);

enum class OpCode : uint8_t {
	// Literals & Constants
	OP_CONSTANT,
	OP_CONSTANT_LONG,
	OP_TRUE,
	OP_FALSE,
	OP_NONE,
	OP_NOTYPE,
	// Variables & Scope
	OP_DEFINE_VAR,
	OP_GET_VAR,
	OP_SET_VAR,
	OP_DEEP_COPY,
	OP_REF_LOCAL,
	OP_DEFINE_REF,
	OP_REF_VAR,
	OP_REF_INDEX,
	OP_SET_REF,
	OP_SHALLOW_COPY,
	OP_MULTI_SET,
	OP_GET_LOCAL,
	OP_SET_LOCAL,
	OP_INC_LOCAL,
	OP_SET_FLAGS,
	OP_REF_PROPERTY,
	OP_DELETE,
	// Arithmetic & Logic
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_FAST_IADD,
	OP_FAST_ISUB,
	OP_FAST_IMUL,
	OP_FAST_IDIV,
	OP_FAST_FADD,
	OP_FAST_FSUB,
	OP_FAST_FMUL,
	OP_FAST_FDIV,
	OP_FAST_SADD,
	OP_CTLE_TICK,
	OP_FLOOR_DIV,
	OP_MOD,
	OP_POW,
	OP_IADD,
	OP_ISUB,
	OP_IMUL,
	OP_IDIV,
	OP_IFLOOR_DIV,
	OP_IMOD,
	OP_IPOW,
	OP_DUP,
	OP_DUP_2,
	OP_EQ,
	OP_NEQ,
	OP_LT,
	OP_GT,
	OP_LTE,
	OP_GTE,
	OP_COLON,
	OP_STRICT_NEQ,
	OP_NOT,
	OP_AND,
	OP_OR,
	OP_XOR,
	OP_IS,
	OP_IN,
	OP_IS_NOT,
	OP_STRICT_EQ,
	OP_IS_IN,
	OP_IS_NOT_IN,
	OP_NXOR,
	OP_NAND,
	OP_NOR,
	OP_NEGATE,
	OP_INCREMENT,
	OP_DECREMENT,
	OP_BITWISE_AND,
	OP_BITWISE_OR,
	OP_BITWISE_XOR,
	OP_BITWISE_NOT,
	OP_BITWISE_SHL,
	OP_BITWISE_SHR,
	OP_PRE_INCREMENT,
	OP_PRE_DECREMENT,
	// Containers
	OP_BUILD_LIST,
	OP_BUILD_TUPLE,
	OP_BUILD_SET,
	OP_BUILD_DICT,
	OP_UNPACK_DICT,
	OP_BUILD_RANGE,
	OP_BUILD_VECTOR,
	OP_BUILD_FSTRING,
	OP_BUILD_FILE,
	OP_BUILD_SLICE,
	// OOP
	OP_CLASS,
	OP_METHOD,
	OP_GET_PROPERTY,
	OP_SET_PROPERTY,
	OP_INC_PROPERTY,
	OP_CLASS_FIELD,
	OP_SUPER,
	// Comprehension
	OP_LIST_APPEND,
	OP_SET_ADD,
	OP_DICT_SET,
	OP_LIST_TO_TUPLE,
	OP_LIST_TO_VECTOR,
	// Access & Calls
	OP_GET_INDEX,
	OP_SET_INDEX,
	OP_INC_INDEX,
	OP_INVOKE,
	OP_CALL,
	// Control Flow
	OP_JUMP,
	OP_JUMP_IF_FALSE,
	OP_LOOP,
	OP_RETURN,
	OP_TO_STREAM,
	OP_JUMP_IF_NOT_LT,
	OP_BREAK,
	OP_CONTINUE,
	OP_SKIP,
	OP_OMIT,
	OP_FOR_ITER,
	OP_SKIP_ITER,
	OP_SWITCH_TABLE,
	// Errors & Systems
	OP_THROW,
	OP_ASSERT,
	OP_IMPORT,
	OP_POP,
	OP_DEBUG_NAME,
	OP_REPL_PRINT,
	OP_TRY_ENTER,
	OP_TRY_EXIT,
	OP_CATCH,
	OP_RETHROW,
	OP_END_FINALLY
};
std::string OpCodeToString(OpCode num);
struct Chunk {
	vector<uint8_t> code;
	vector<Value> constants;
	vector<int> lines;
	vector<int> columns;
	void write(uint8_t byte, int line, int col) {
		code.push_back(byte);
		lines.push_back(line);
		columns.push_back(col);
	}
	void write(OpCode op, int line, int col) {
		write(static_cast<uint8_t>(op), line, col);
	}
	int addConstant(Value v) {
		for (size_t i = 0; i < constants.size(); i++) {
			if (constants[i].strictEquals(v)) {
				return static_cast<int>(i);
			}
		}
		constants.push_back(v);
		return static_cast<int>(constants.size() - 1);
	}
};

inline FunctionObject::~FunctionObject() {
	for (auto &p : params) {
		if (p.defaultChunk)
			delete p.defaultChunk;
	}
	for (auto *c : defaultRetChunks) {
		delete c;
	}
	if (chunk)
		delete chunk;
}
struct LoopContext {
	int startAddress;
	int stepAddress;
	vector<int> breakJumps;
	vector<int> continueJumps;
	bool isForEach;
	int startLocalCount;
	int iteratorSlot;
};
struct Local {
	string name;
	int depth;
	bool isConst = false;
	bool hasKnownValue = false;
	Value knownValue;
	bool hasKnownType = false;
	ValueType knownType = ValueType::NOTYPE;
};
struct ExceptionHandler {
	int catchAddress;
	int finallyAddress;
	int stackDepth;
	int scopeDepth;
	bool isInsideFinally = false;
};
struct CallFrame {
	FunctionObject *function;
	uint8_t *ip;
	int basePointer;
	vector<Value> cacheKey;
	Value singleCacheKey = Value::NoType();
	vector<ExceptionHandler> handlerStack;
};
Value EvaluateConstBinary(TokenType op, const Value &a, const Value &b);
// ------------ AST WALKER -------------
struct Interpreter {
	std::shared_ptr<Env> env;
	unordered_map<string, FuncVal *> funcs;
	using ModuleLoader =
		std::function<void(std::shared_ptr<Env>, const vector<string> &)>;
	std::unordered_map<string, ModuleLoader> modules;
	void resolveImport(std::string lib, std::vector<std::string> sym, std::shared_ptr<Env> targetEnv);
	bool returning = false;
	Value returnValue;
	bool breaking = false;
	bool continuing = false;
	int skipCount = 0;
	bool vectorEnabled = false;
	std::unordered_set<string> importStack;
	Interpreter();
	Value nativePrint(const vector<Value> &args, int l, int c);
	void registerStdLib();
	void registerIoModules();
	void registerTimeModule();
	void registerSerialModule();
	void registerEventsModule();
	void registerMathModules();
	void registerRegexModule();
	void registerBuiltins();
	void registerFfi();
	LValue resolveLValue(Expr *e);
	Value Resolve_methods(MethodCallExpr *m);
	bool matchesError(const LangError &e, const string &typeName);
	Value eval(Expr *e);
	Value exec(Stmt *s);
	Value call(Value callee, const vector<CallArg> &args,
		const vector<CopyMode> &modes, int line = 0, int col = 0);
	void interpret(const vector<Stmt *> &statements);

 private:
	bool canOptimizeScope(const vector<Stmt *> &body);
	bool isGlobalAllowed(Stmt *s);
};
// ---------------- JIT ----------------

// ------------ BYTECODE VM ------------
struct CTLEAbort : public std::exception {};

#endif // YLANG_OBJECTS_HPP
