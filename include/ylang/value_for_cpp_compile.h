#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using std::make_shared;
using std::string;
using std::to_string;
using std::unordered_map;
using std::unordered_set;
using std::vector;
enum class AccessLevel {
	PUBLIC,
	PRIVATE,
	PROTECTED
};
enum class ValueType {
	NOTYPE,
	NONE,
	INT,
	FLOAT,
	STRING,
	BOOL,
	LIST,
	VECTOR,
	DICT,
	SLICE,
	BIGINT,
	REFERENCE,
	PAIRED,
	SUPER,
	RANGE,
	TUPLE,
	SET,
	FUNCTION,
	NATIVE_FUNCTION,
	FILE,
	OVERLOAD,
	OMIT_MARKER,
	ERROR,
	CLASS,
	INSTANCE
};
enum class CopyMode { SHALLOW,
	DEEP,
	REF };
class RuntimeError : public std::runtime_error {
 public:
	RuntimeError(const string &msg, int line, int col) : std::runtime_error(msg) {}
};
class TypeError : public std::runtime_error {
 public:
	TypeError(const string &msg, int line, int col) : std::runtime_error(msg) {}
};
struct Value;
struct HeapObject;
struct BigIntObject;
struct ErrorObject;
struct ClassObject;
struct InstanceObject;
using JitFunc = void *;
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
	static Value Reference(Value *p);
	static Value Int(long long v, bool locked = false, bool isConst = false);
	static Value pInt(void *v, bool locked = false, bool isConst = false);
	static Value BigInt(long long n);
	static Value BigInt(std::vector<uint32_t> chunks, bool isNegative);
	static Value BigInt(std::shared_ptr<BigIntObject> obj);
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
};
struct HeapObject {
	std::string name = "";
	ValueType type;
	bool typeLocked = false;
	HeapObject(ValueType t, bool locked = false) : type(t), typeLocked(locked) {}
	virtual ~HeapObject() = default;
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
	SetObject(const std::unordered_set<Value, ValueHash, ValueEqual> &e, bool locked = false)
		 : HeapObject(ValueType::SET, locked), elements(e) {}
};
struct TupleObject : HeapObject {
	std::vector<Value> elements;
	TupleObject() : HeapObject(ValueType::TUPLE) {}
	TupleObject(const std::vector<Value> &elems, bool locked = false)
		 : HeapObject(ValueType::TUPLE, locked), elements(elems) {}
};
struct DictObject : HeapObject {
	std::unordered_map<Value, Value, ValueHash, ValueEqual> items;
	DictObject() : HeapObject(ValueType::DICT) {}
	DictObject(const std::unordered_map<Value, Value, ValueHash, ValueEqual> &m,
		bool locked = false)
		 : HeapObject(ValueType::DICT, locked), items(m) {}
};
struct PairedObject : HeapObject {
	std::vector<std::pair<Value, Value>> pairs;
	PairedObject(const std::vector<std::pair<Value, Value>> &p)
		 : HeapObject(ValueType::PAIRED), pairs(p) {}
};
struct NativeFunctionObject : HeapObject {
	NativeFunc func;
	NativeFunctionObject(NativeFunc f)
		 : HeapObject(ValueType::NATIVE_FUNCTION), func(f) {}
};
struct OverloadObject : HeapObject {
	std::vector<Value> overloads;
	OverloadObject(const Value &v) : HeapObject(ValueType::OVERLOAD) {
		overloads.push_back(v);
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
};
struct BigIntObject : HeapObject {
	bool isNegative;
	std::vector<uint32_t> chunks;
	BigIntObject(long long n) : HeapObject(ValueType::BIGINT) {
		if (n < 0) {
			isNegative = true;
			n = -n;
		} else
			isNegative = false;
		if (n == 0)
			chunks.push_back(0);
		while (n > 0) {
			chunks.push_back((uint32_t)(n & 0xFFFFFFFF));
			n >>= 32;
		}
	}
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
	bool operator<(const BigIntObject &other) const {
		if (isNegative != other.isNegative)
			return isNegative;
		if (chunks.size() != other.chunks.size())
			return isNegative ? chunks.size() > other.chunks.size()
									: chunks.size() < other.chunks.size();
		for (int i = chunks.size() - 1; i >= 0; i--) {
			if (chunks[i] != other.chunks[i])
				return isNegative ? chunks[i] > other.chunks[i]
										: chunks[i] < other.chunks[i];
		}
		return false;
	}
	bool operator>(const BigIntObject &other) const { return other < *this; }
	bool absLess(const BigIntObject &other) const {
		if (chunks.size() != other.chunks.size())
			return chunks.size() < other.chunks.size();
		for (int i = chunks.size() - 1; i >= 0; i--)
			if (chunks[i] != other.chunks[i])
				return chunks[i] < other.chunks[i];
		return false;
	}
	BigIntObject absAdd(const BigIntObject &other) const {
		std::vector<uint32_t> res;
		uint64_t carry = 0;
		size_t n = std::max(chunks.size(), other.chunks.size());
		res.reserve(n + 1);
		for (size_t i = 0; i < n || carry; i++) {
			uint64_t sum = carry + (i < chunks.size() ? chunks[i] : 0) +
								(i < other.chunks.size() ? other.chunks[i] : 0);
			res.push_back((uint32_t)(sum & 0xFFFFFFFF));
			carry = sum >> 32;
		}
		return BigIntObject(res, false);
	}
	BigIntObject absSub(const BigIntObject &other) const {
		std::vector<uint32_t> res;
		int64_t borrow = 0;
		size_t n = chunks.size();
		res.reserve(n);
		for (size_t i = 0; i < n; i++) {
			int64_t sub = (int64_t)chunks[i] -
							  (i < other.chunks.size() ? other.chunks[i] : 0) - borrow;
			if (sub < 0) {
				sub += 4294967296LL;
				borrow = 1;
			} else
				borrow = 0;
			res.push_back((uint32_t)sub);
		}
		return BigIntObject(res, false);
	}
	BigIntObject operator+(const BigIntObject &other) const {
		if (isNegative == other.isNegative) {
			BigIntObject res = absAdd(other);
			res.isNegative = isNegative;
			return res;
		} else {
			if (absLess(other)) {
				BigIntObject res = other.absSub(*this);
				res.isNegative = other.isNegative;
				return res;
			} else {
				BigIntObject res = absSub(other);
				res.isNegative = isNegative;
				return res;
			}
		}
	}
	BigIntObject operator-(const BigIntObject &other) const {
		if (isNegative != other.isNegative) {
			BigIntObject res = absAdd(other);
			res.isNegative = isNegative;
			return res;
		} else {
			if (absLess(other)) {
				BigIntObject res = other.absSub(*this);
				res.isNegative = !isNegative;
				return res;
			} else {
				BigIntObject res = absSub(other);
				res.isNegative = isNegative;
				return res;
			}
		}
	}
	BigIntObject operator*(const BigIntObject &other) const {
		size_t n = chunks.size(), m = other.chunks.size();
		std::vector<uint32_t> res(n + m, 0);
		for (size_t i = 0; i < n; i++) {
			uint64_t carry = 0;
			for (size_t j = 0; j < m; j++) {
				uint64_t prod =
					(uint64_t)chunks[i] * other.chunks[j] + res[i + j] + carry;
				res[i + j] = (uint32_t)(prod & 0xFFFFFFFF);
				carry = prod >> 32;
			}
			res[i + m] += (uint32_t)carry;
		}
		return BigIntObject(res, isNegative != other.isNegative);
	}
	std::pair<BigIntObject, BigIntObject> divMod(const BigIntObject &other) const {
		if (other.chunks.size() == 1 && other.chunks[0] == 0)
			throw std::runtime_error("Divide by zero");
		BigIntObject dividend = *this;
		dividend.isNegative = false;
		BigIntObject divisor = other;
		divisor.isNegative = false;
		BigIntObject quotient(0);
		BigIntObject remainder(0);
		if (dividend < divisor)
			return {BigIntObject(0), *this};
		for (int i = dividend.chunks.size() * 32 - 1; i >= 0; i--) {
			remainder.lshift(1);
			int chunkIdx = i / 32;
			int bitIdx = i % 32;
			if ((dividend.chunks[chunkIdx] >> bitIdx) & 1) {
				remainder.chunks[0] |= 1;
			}
			if (!remainder.absLess(divisor)) {
				remainder = remainder.absSub(divisor);
				quotient.setBit(i);
			}
		}
		quotient.isNegative = isNegative != other.isNegative;
		remainder.isNegative = isNegative;
		return {quotient, remainder};
	}
	BigIntObject operator/(const BigIntObject &other) const {
		return divMod(other).first;
	}
	BigIntObject operator%(const BigIntObject &other) const {
		return divMod(other).second;
	}
	void lshift(int shift) {
		if (shift == 0)
			return;
		int chunkShift = shift / 32;
		int bitShift = shift % 32;

		std::vector<uint32_t> newChunks(chunks.size() + chunkShift, 0);

		uint32_t carry = 0;
		for (size_t i = 0; i < chunks.size(); i++) {
			uint64_t val = chunks[i];
			newChunks[i + chunkShift] = (val << bitShift) | carry;
			carry = (bitShift == 0) ? 0 : (val >> (32 - bitShift));
		}
		if (carry)
			newChunks.push_back(carry);
		chunks = std::move(newChunks);
		trim();
	}
	void setBit(int n) {
		int chunkIdx = n / 32;
		int bitIdx = n % 32;
		if (chunkIdx >= chunks.size())
			chunks.resize(chunkIdx + 1, 0);
		chunks[chunkIdx] |= (1U << bitIdx);
	}
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
	static Value pow(Value a, Value b) {
		BigIntObject tempA(0), tempB(0);
		BigIntObject *ba = (a.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(a.ref.get())
									 : &(tempA = BigIntObject(a.asInt()));
		BigIntObject *bb = (b.type == ValueType::BIGINT)
									 ? static_cast<BigIntObject *>(b.ref.get())
									 : &(tempB = BigIntObject(b.asInt()));
		if (bb->isNegative)
			return Value::Int(0);
		BigIntObject base = *ba;
		BigIntObject exp = *bb;
		BigIntObject res(1);
		while (!exp.absLess(BigIntObject(1)) &&
				 !(exp.chunks.size() == 1 && exp.chunks[0] == 0)) {
			if (exp.chunks[0] & 1)
				res = res * base;
			uint32_t carry = 0;
			for (int i = exp.chunks.size() - 1; i >= 0; i--) {
				uint32_t nextCarry = (exp.chunks[i] & 1) << 31;
				exp.chunks[i] = (exp.chunks[i] >> 1) | carry;
				carry = nextCarry;
			}
			exp.trim();
			base = base * base;
		}
		return Value::BigInt(std::make_shared<BigIntObject>(res));
	}
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
	void computeMRO() {
		mro.clear();
		mro.push_back(this);
		std::vector<std::vector<ClassObject *>> lists;
		for (auto &pVal : parents) {
			if (pVal.type == ValueType::CLASS) {
				auto *pClass = static_cast<ClassObject *>(pVal.ref.get());
				lists.push_back(pClass->mro);
			}
		}
		std::vector<ClassObject *> parentsList;
		for (auto &pVal : parents)
			if (pVal.type == ValueType::CLASS)
				parentsList.push_back(static_cast<ClassObject *>(pVal.ref.get()));
		if (!parentsList.empty())
			lists.push_back(parentsList);
		while (true) {
			for (size_t i = 0; i < lists.size();) {
				if (lists[i].empty())
					lists.erase(lists.begin() + i);
				else
					i++;
			}
			if (lists.empty())
				break;
			ClassObject *candidate = nullptr;
			for (size_t i = 0; i < lists.size(); i++) {
				ClassObject *head = lists[i][0];
				bool valid = true;
				for (size_t j = 0; j < lists.size(); j++) {
					for (size_t k = 1; k < lists[j].size(); k++) {
						if (lists[j][k] == head) {
							valid = false;
							break;
						}
					}
					if (!valid)
						break;
				}
				if (valid) {
					candidate = head;
					break;
				}
			}
			if (!candidate)
				break;
			mro.push_back(candidate);
			for (size_t i = 0; i < lists.size(); i++)
				if (lists[i].front() == candidate)
					lists[i].erase(lists[i].begin());
		}
	}
	string toString() const { return "<class '" + name + "'>"; }
};
struct InstanceObject : HeapObject {
	ClassObject *klass;
	unordered_map<string, Value> fields;
	InstanceObject(ClassObject *k) : HeapObject(ValueType::INSTANCE), klass(k) {}
	string toString() const { return "<instance of '" + klass->name + "'>"; }
};
struct SuperObject : HeapObject {
	Value instance;
	ClassObject *startClass;
	SuperObject(Value inst, ClassObject *start)
		 : HeapObject(ValueType::SUPER), instance(inst), startClass(start) {}
	string toString() const { return "<super: " + startClass->name + ">"; }
};
inline Value Value::Reference(Value *p) {
	Value v;
	v.type = ValueType::REFERENCE;
	v.ptr = p;
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
inline Value Value::Range(double s, double e, double st, bool si, bool ei, bool f, bool locked, bool isConst) {
	bool isInvalid = false;
	if (st == 0)
		isInvalid = true;
	else if (st > 0) {
		if (s > e)
			isInvalid = true;
		else if (s == e && (!si || !ei))
			isInvalid = true;
	} else {
		if (s < e)
			isInvalid = true;
		else if (s == e && (!si || !ei))
			isInvalid = true;
	}
	if (isInvalid) {
		s = 0;
		e = 0;
		st = 1;
		si = false;
		ei = false;
		f = false;
	}
	Value x;
	x.type = ValueType::RANGE;
	x.ref =
		std::make_shared<RangeObject>(s, e, st, si, ei, f, !isInvalid, locked);
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
	return v;
}
inline Value Value::Class(const string &name) {
	Value v;
	v.type = ValueType::CLASS;
	v.ref = make_shared<ClassObject>(name);
	return v;
}
inline Value Value::Instance(Value classDef) {
	if (classDef.type != ValueType::CLASS)
		throw RuntimeError("Cannot create instance from non-class type", 0, 0);
	Value v;
	v.type = ValueType::INSTANCE;
	auto *clsPtr = static_cast<ClassObject *>(classDef.ref.get());
	v.ref = make_shared<InstanceObject>(clsPtr);
	return v;
}
inline bool Value::isTruthy() const {
	if (type == ValueType::NONE || type == ValueType::NOTYPE)
		return false;
	if (type == ValueType::BOOL)
		return bVal;
	if (type == ValueType::INT)
		return iVal != 0;
	if (type == ValueType::FLOAT)
		return fVal != 0.0;
	if (type == ValueType::STRING)
		return !static_cast<StringObject *>(ref.get())->value.empty();
	if (type == ValueType::LIST)
		return !static_cast<ListObject *>(ref.get())->elements.empty();
	if (type == ValueType::SET)
		return !static_cast<SetObject *>(ref.get())->elements.empty();
	if (type == ValueType::TUPLE)
		return !static_cast<TupleObject *>(ref.get())->elements.empty();
	if (type == ValueType::DICT)
		return !static_cast<DictObject *>(ref.get())->items.empty();
	if (type == ValueType::RANGE) {
		auto *r = static_cast<RangeObject *>(ref.get());
		if (!r->isValid || r->step == 0)
			return false;
		double s = r->start;
		if (!r->startInclusive)
			s += r->step;
		if (r->step > 0)
			return (s < r->end) || (r->endInclusive && s == r->end);
		else
			return (s > r->end) || (r->endInclusive && s == r->end);
	}
	return false;
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
inline bool Value::strictEquals(const Value &other) const {
	if (type != other.type)
		return false;
	switch (type) {
	case ValueType::INT:
		return iVal == other.iVal;
	case ValueType::FLOAT:
		return fVal == other.fVal;
	case ValueType::BOOL:
		return bVal == other.bVal;
	case ValueType::STRING:
		return asString() == other.asString();
	case ValueType::NONE:
	case ValueType::NOTYPE:
		return true;
	case ValueType::BIGINT: {
		return *static_cast<BigIntObject *>(ref.get()) ==
				 *static_cast<BigIntObject *>(other.ref.get());
	}
	case ValueType::LIST: {
		auto *l1 = static_cast<ListObject *>(ref.get());
		auto *l2 = static_cast<ListObject *>(other.ref.get());
		if (l1->elements.size() != l2->elements.size())
			return false;
		for (size_t i = 0; i < l1->elements.size(); ++i)
			if (!l1->elements[i].strictEquals(l2->elements[i]))
				return false;
		return true;
	}
	case ValueType::SET: {
		auto *s1 = static_cast<SetObject *>(ref.get());
		auto *s2 = static_cast<SetObject *>(other.ref.get());
		if (s1->elements.size() != s2->elements.size())
			return false;
		for (const auto &v1 : s1->elements) {
			bool found = false;
			for (const auto &v2 : s2->elements)
				if (v1.strictEquals(v2)) {
					found = true;
					break;
				}
			if (!found)
				return false;
		}
		return true;
	}
	case ValueType::RANGE: {
		auto *r1 = static_cast<RangeObject *>(ref.get());
		auto *r2 = static_cast<RangeObject *>(other.ref.get());
		return r1->start == r2->start && r1->end == r2->end &&
				 r1->step == r2->step &&
				 r1->startInclusive == r2->startInclusive &&
				 r1->endInclusive == r2->endInclusive;
	}
	case ValueType::TUPLE: {
		auto *t1 = static_cast<TupleObject *>(ref.get());
		auto *t2 = static_cast<TupleObject *>(other.ref.get());
		if (t1->elements.size() != t2->elements.size())
			return false;
		for (size_t i = 0; i < t1->elements.size(); ++i)
			if (!t1->elements[i].strictEquals(t2->elements[i]))
				return false;
		return true;
	}
	case ValueType::DICT: {
		auto *d1 = static_cast<DictObject *>(ref.get());
		auto *d2 = static_cast<DictObject *>(other.ref.get());
		if (d1->items.size() != d2->items.size())
			return false;
		for (const auto &[k, v] : d1->items) {
			if (d2->items.find(k) == d2->items.end())
				return false;
			if (!v.strictEquals(d2->items.at(k)))
				return false;
		}
		return true;
	}
	case ValueType::PAIRED:
		return false;
	case ValueType::VECTOR: {
		auto *v1 = static_cast<VectorObject *>(ref.get());
		auto *v2 = static_cast<VectorObject *>(other.ref.get());
		if (v1->elements.size() != v2->elements.size())
			return false;
		for (size_t i = 0; i < v1->elements.size(); i++) {
			if (std::abs(v1->elements[i].asInt() - v2->elements[i].asInt()) >
				 1e-9)
				return false;
		}
		return true;
	}
	}
	return false;
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
inline std::size_t ValueHash::operator()(const Value &v) const {
	size_t seed = 0;
	auto hash_combine = [&](size_t hash) {
		seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	};
	switch (v.type) {
	case ValueType::INT:
		hash_combine(std::hash<long long>{}(v.asInt()));
		break;
	case ValueType::FLOAT:
		hash_combine(std::hash<double>{}(v.asFloat()));
		break;
	case ValueType::BOOL:
		hash_combine(std::hash<bool>{}(v.asBool()));
		break;
	case ValueType::STRING:
		hash_combine(std::hash<string>{}(v.asString()));
		break;
	case ValueType::NONE:
		hash_combine(0);
		break;
	case ValueType::INSTANCE: {
		auto *inst = static_cast<InstanceObject *>(v.ref.get());
		hash_combine(std::hash<string>{}(inst->klass->name));
		size_t fieldsXor = 0;
		for (const auto &[key, val] : inst->fields) {
			size_t pairHash = std::hash<string>{}(key);
			size_t valHash = ValueHash{}(val);
			pairHash ^=
				valHash + 0x9e3779b9 + (pairHash << 6) + (pairHash >> 2);
			fieldsXor ^= pairHash;
		}
		hash_combine(fieldsXor);
		break;
	}
	case ValueType::TUPLE: {
		auto *t = static_cast<TupleObject *>(v.ref.get());
		for (const auto &elem : t->elements)
			hash_combine(ValueHash{}(elem));
		break;
	}
	case ValueType::LIST: {
		auto *l = static_cast<ListObject *>(v.ref.get());
		for (const auto &elem : l->elements)
			hash_combine(ValueHash{}(elem));
		break;
	}
	case ValueType::SET: {
		auto *s = static_cast<SetObject *>(v.ref.get());
		size_t xorSum = 0;
		for (const auto &elem : s->elements)
			xorSum ^= ValueHash{}(elem);
		hash_combine(xorSum);
		break;
	}
	case ValueType::VECTOR: {
		auto *vec = static_cast<VectorObject *>(v.ref.get());
		hash_combine(vec->elements.size());
		for (auto &elem : vec->elements) {
			hash_combine(ValueHash{}(elem));
		}
		break;
	}
	default:
		hash_combine(std::hash<int>{}((int)v.type));
	}
	return seed;
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
inline Value shallowCopy(const Value &v) {
	switch (v.type) {
	case ValueType::LIST: {
		auto *oldObj = static_cast<ListObject *>(v.ref.get());
		return Value::List(oldObj->elements);
	}
	case ValueType::SET: {
		auto *oldObj = static_cast<SetObject *>(v.ref.get());
		return Value::Set(oldObj->elements);
	}
	case ValueType::TUPLE: {
		auto *oldObj = static_cast<TupleObject *>(v.ref.get());
		return Value::Tuple(oldObj->elements);
	}
	case ValueType::DICT: {
		auto *oldObj = static_cast<DictObject *>(v.ref.get());
		return Value::Dict(oldObj->items, v.ref->typeLocked);
	}
	default:
		return v;
	}
}
inline Value deepCopy(const Value &v) {
	Value out;
	if (v.type == ValueType::INT || v.type == ValueType::FLOAT ||
		 v.type == ValueType::BOOL) {
		return v;
	}
	switch (v.type) {
	case ValueType::STRING:
		out = Value::String(static_cast<StringObject *>(v.ref.get())->value);
		break;
	case ValueType::LIST: {
		auto *oldList = static_cast<ListObject *>(v.ref.get());
		std::vector<Value> copied;
		for (const auto &el : oldList->elements)
			copied.push_back(deepCopy(el));
		out = Value::List(copied);
		break;
	}
	case ValueType::BIGINT: {
		auto *oldBig = static_cast<BigIntObject *>(v.ref.get());
		out = Value::BigInt(oldBig->chunks, oldBig->isNegative);
		break;
	}
	case ValueType::SET: {
		auto *oldSet = static_cast<SetObject *>(v.ref.get());
		std::unordered_set<Value, ValueHash, ValueEqual> copied;
		for (const auto &el : oldSet->elements)
			copied.insert(deepCopy(el));
		out = Value::Set(copied);
		break;
	}
	case ValueType::DICT: {
		auto *oldDict = static_cast<DictObject *>(v.ref.get());
		std::unordered_map<Value, Value, ValueHash, ValueEqual> copied;
		for (const auto &[key, val] : oldDict->items)
			copied[deepCopy(key)] = deepCopy(val);
		out = Value::Dict(copied, v.ref->typeLocked);
		break;
	}
	case ValueType::VECTOR: {
		auto *vec = static_cast<VectorObject *>(v.ref.get());
		std::vector<Value> copied;
		copied.reserve(vec->elements.size());
		for (const auto &el : vec->elements)
			copied.push_back(deepCopy(el));
		out = Value::Vector(copied);
		break;
	}
	default:
		out = v;
		break;
	}
	return out;
}
inline Value applyCopy(const Value &v, CopyMode mode) {
	switch (mode) {
	case CopyMode::REF:
		return v;
	case CopyMode::DEEP:
		return deepCopy(v);
	case CopyMode::SHALLOW:
	default:
		return shallowCopy(v);
	}
}
static Value defaultOf(ValueType t) {
	switch (t) {
	case ValueType::INT:
		return Value::Int(0);
	case ValueType::FLOAT:
		return Value::Float(0.0);
	case ValueType::BOOL:
		return Value::Bool(false);
	case ValueType::STRING:
		return Value::String("");
	case ValueType::NONE:
		return Value::None();
	case ValueType::NOTYPE:
		return Value::NoType();
	case ValueType::LIST:
		return Value::List({});
	case ValueType::DICT:
		return Value::Dict({});
	case ValueType::SET:
		return Value::Set({});
	case ValueType::TUPLE:
		return Value::Tuple({});
	}
	return Value::NoType();
}
static inline int divMod10(std::vector<uint32_t> &chunks) {
	uint64_t remainder = 0;
	for (int i = chunks.size() - 1; i >= 0; i--) {
		uint64_t combined = (remainder << 32) | chunks[i];
		chunks[i] = (uint32_t)(combined / 10);
		remainder = combined % 10;
	}
	while (chunks.size() > 1 && chunks.back() == 0)
		chunks.pop_back();
	return (int)remainder;
}
static inline std::string bigIntToString(BigIntObject *big) {
	if (big->chunks.empty())
		return "0";
	if (big->chunks.size() == 1 && big->chunks[0] == 0)
		return "0";
	std::vector<uint32_t> temp = big->chunks;
	std::string res = "";
	while (temp.size() > 1 || temp[0] > 0) {
		int digit = divMod10(temp);
		res += std::to_string(digit);
	}
	if (res.empty())
		return "0";
	if (big->isNegative)
		res += "-";
	std::reverse(res.begin(), res.end());
	return res;
}
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
static inline std::string valueToString(const Value &v, int line = 0, int col = 0) {
	switch (v.type) {
	case ValueType::BOOL:
		return v.asBool() ? "true" : "false";
	case ValueType::NONE:
		return "None";
	case ValueType::NOTYPE:
		return "Notype";
	case ValueType::FLOAT:
		return formatNumber(v.asFloat());
	case ValueType::INT:
		return std::to_string(v.asInt());
	case ValueType::STRING:
		return v.asString();
	case ValueType::FUNCTION:
		return "<function>";
	case ValueType::NATIVE_FUNCTION:
		return "<native_function>";
	case ValueType::CLASS:
		return "<class>";
	case ValueType::INSTANCE:
		return "<class-instance>";
	case ValueType::ERROR: {
		auto *err = static_cast<ErrorObject *>(v.ref.get());
		return "<Error: " + err->errType + ": " + err->message + ">";
	}
	case ValueType::SET: {
		auto *s = static_cast<SetObject *>(v.ref.get());
		string str = "{";
		size_t i = 0;
		for (const auto &key : s->elements) {
			str += valueToString(key, line, col);
			if (i + 1 < s->elements.size())
				str += ", ";
			i++;
		}
		str += "}";
		return str;
	}
	case ValueType::LIST: {
		auto *s = static_cast<ListObject *>(v.ref.get());
		string str = "[";
		for (size_t i = 0; i < s->elements.size(); i++) {
			str += valueToString(s->elements[i], line, col);
			if (i + 1 < s->elements.size())
				str += ", ";
		}
		str += "]";
		return str;
	}
	case ValueType::RANGE: {
		auto *r = static_cast<RangeObject *>(v.ref.get());
		if (!r->isValid)
			return "(...)";
		string s = "";
		auto fmt = [](double d, bool isF) {
			return isF ? std::to_string(d) : std::to_string((long long)d);
		};
		if (r->start > r->end) {
			s += (r->endInclusive ? "[" : "(") + fmt(r->end, r->isFloat);
			s += ", " + fmt(r->start, r->isFloat) +
				  (r->startInclusive ? "]" : ")");
		} else {
			s += (r->startInclusive ? "[" : "(") + fmt(r->start, r->isFloat);
			s += ", " + fmt(r->end, r->isFloat) + (r->endInclusive ? "]" : ")");
		}
		if (r->step != 1)
			s += ".." + fmt(r->step, r->isFloat);
		return s;
	}
	case ValueType::TUPLE: {
		auto *list = static_cast<TupleObject *>(v.ref.get());
		if (list->elements.empty())
			return "(,)";
		string str = "(";
		for (size_t i = 0; i < list->elements.size(); i++) {
			str += valueToString(list->elements[i], line, col);
			if (i + 1 < list->elements.size())
				str += ", ";
			else if (list->elements.size() == 1)
				str += ",";
		}
		str += ")";
		return str;
	}
	case ValueType::DICT: {
		auto *d = static_cast<DictObject *>(v.ref.get());
		string s = "{";
		size_t count = 0;
		for (const auto &[key, val] : d->items) {
			s += valueToString(key, line, col) + ": " +
				  valueToString(val, line, col);
			if (count++ < d->items.size() - 1)
				s += ", ";
		}
		s += "}";
		return s;
	}
	case ValueType::VECTOR: {
		auto *vec = static_cast<VectorObject *>(v.ref.get());
		string s = "<";
		for (size_t i = 0; i < vec->elements.size(); i++) {
			s += valueToString(vec->elements[i]);
			if (i < vec->elements.size() - 1)
				s += ", ";
		}
		s += ">";
		return s;
	}
	case ValueType::BIGINT: {
		auto *big = static_cast<BigIntObject *>(v.ref.get());
		return bigIntToString(big);
	}
	case ValueType::REFERENCE: {
		return valueToString(*v.ptr);
	}
	case ValueType::SUPER: {
		auto *super = static_cast<SuperObject *>(v.ref.get());
		return "<super" + (super->name.empty() ? "" : super->name) + ">";
	}
	case ValueType::PAIRED: {
		auto *pair = static_cast<PairedObject *>(v.ref.get());
		return "<Paired Object>";
	}
	case ValueType::OMIT_MARKER: {
		return "Omit_marker";
	}
	case ValueType::FILE: {
		auto *file = static_cast<FileObject *>(v.ref.get());
		return "<File: " + file->name + "at path: " + file->path;
	}
	default:
		throw TypeError("Cannot implicitly convert this type to string " +
								 to_string((int)v.type),
			line, col);
	}
}
static inline bool lessValue(const Value &a, const Value &b) {
	if (a.type != b.type) {
		bool aIsNum = (a.type == ValueType::INT || a.type == ValueType::FLOAT || a.type == ValueType::BIGINT);
		bool bIsNum = (b.type == ValueType::INT || b.type == ValueType::FLOAT || b.type == ValueType::BIGINT);
		if (aIsNum && bIsNum) {
			if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
				return a.asFloat() < b.asFloat();
			}
			BigIntObject tempA(0), tempB(0);
			BigIntObject *ba = (a.type == ValueType::BIGINT) ? static_cast<BigIntObject *>(a.ref.get()) : &(tempA = BigIntObject(a.asInt()));
			BigIntObject *bb = (b.type == ValueType::BIGINT) ? static_cast<BigIntObject *>(b.ref.get()) : &(tempB = BigIntObject(b.asInt()));
			return *ba < *bb;
		}
		return a.type < b.type;
	}
	if (a.type == ValueType::REFERENCE) {
		if (b.type == ValueType::REFERENCE) {
			return lessValue(*a.ptr, *b.ptr);
		} else {
			return lessValue(*a.ptr, b);
		}
	} else if (b.type == ValueType::REFERENCE) {
		return lessValue(a, *b.ptr);
	}
	switch (a.type) {
	case ValueType::INT:
		return a.asInt() < b.asInt();
	case ValueType::FLOAT:
		return a.asFloat() < b.asFloat();
	case ValueType::STRING:
		return a.asString() < b.asString();
	case ValueType::BOOL:
		return a.asBool() < b.asBool();
	case ValueType::BIGINT: {
		auto *b1 = static_cast<BigIntObject *>(a.ref.get());
		auto *b2 = static_cast<BigIntObject *>(b.ref.get());
		return *b1 < *b2;
	}
	case ValueType::TUPLE: {
		auto *t1 = static_cast<TupleObject *>(a.ref.get());
		auto *t2 = static_cast<TupleObject *>(b.ref.get());
		if (t1->elements.empty() && t2->elements.empty())
			return false;
		if (t1->elements.empty())
			return true;
		if (t2->elements.empty())
			return false;
		return lessValue(t1->elements[0], t2->elements[0]);
	}
	case ValueType::VECTOR: {
		auto *v1 = static_cast<VectorObject *>(a.ref.get());
		auto *v2 = static_cast<VectorObject *>(b.ref.get());
		bool hasFloat = false;
		for (const auto &val : v1->elements)
			if (val.type == ValueType::FLOAT)
				hasFloat = true;
		for (const auto &val : v2->elements)
			if (val.type == ValueType::FLOAT)
				hasFloat = true;
		if (hasFloat) {
			double mag1 = 0.0, mag2 = 0.0;
			for (const auto &val : v1->elements) {
				double v = val.asFloat();
				mag1 += (v * v);
			}
			for (const auto &val : v2->elements) {
				double v = val.asFloat();
				mag2 += (v * v);
			}
			return mag1 < mag2;
		} else {
			Value mag1 = Value::Int(0);
			for (const auto &val : v1->elements) {
				Value sq = BigIntObject::mul(val, val);
				mag1 = BigIntObject::add(mag1, sq);
			}
			Value mag2 = Value::Int(0);
			for (const auto &val : v2->elements) {
				Value sq = BigIntObject::mul(val, val);
				mag2 = BigIntObject::add(mag2, sq);
			}
			return lessValue(mag1, mag2);
		}
	}
	case ValueType::LIST: {
		auto *l1 = static_cast<ListObject *>(a.ref.get());
		auto *l2 = static_cast<ListObject *>(b.ref.get());
		return std::lexicographical_compare(
			l1->elements.begin(), l1->elements.end(), l2->elements.begin(),
			l2->elements.end(),
			// Recursive call for elements
			[](const Value &x, const Value &y) { return lessValue(x, y); });
	}
	default:
		return false;
	}
}
static void setAdd(std::unordered_set<Value, ValueHash, ValueEqual> &elems, const Value &v) {
	Value finalVal = deepCopy(v);
	finalVal.isConst = true;
	elems.insert(finalVal);
}