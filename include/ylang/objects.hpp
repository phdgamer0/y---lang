#pragma once
#ifndef YLANG_OBJECTS_HPP
#define YLANG_OBJECTS_HPP
#include "types.hpp"
#include "errors.hpp"
#include "lexer.hpp"
#include "ast.hpp"
#include "parser.hpp"
struct OptimizerHack : public opennn::Optimizer {
	static void set_epochs(opennn::Optimizer *opt, long epochs) {
		static_cast<OptimizerHack *>(opt)->maximum_epochs = (Index)epochs;
	}
};
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
	static Value BigInt(const std::string& str);
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

	Value* get_ptr_safe() const;
};
inline HeapObject* gc_head = nullptr;
inline int gc_alloc_count = 0;
inline int gc_delete_count = 0;

struct HeapObject {
	std::string name = "";
	ValueType type;
	bool typeLocked = false;
	bool gc_marked = false;
	HeapObject* gc_next = nullptr;
	HeapObject* gc_prev = nullptr;

	HeapObject(ValueType t, bool locked = false) : type(t), typeLocked(locked) {
		if (gc_head) {
			gc_head->gc_prev = this;
			this->gc_next = gc_head;
		}
		gc_head = this;
		gc_alloc_count++;
	}

	HeapObject(const HeapObject& other) : name(other.name), type(other.type), typeLocked(other.typeLocked) {
		if (gc_head) {
			gc_head->gc_prev = this;
			this->gc_next = gc_head;
		}
		gc_head = this;
		gc_alloc_count++;
	}

	HeapObject(HeapObject&& other) noexcept : name(std::move(other.name)), type(other.type), typeLocked(other.typeLocked) {
		if (gc_head) {
			gc_head->gc_prev = this;
			this->gc_next = gc_head;
		}
		gc_head = this;
		gc_alloc_count++;
	}

	HeapObject& operator=(const HeapObject& other) {
		if (this != &other) {
			name = other.name;
			type = other.type;
			typeLocked = other.typeLocked;
		}
		return *this;
	}

	HeapObject& operator=(HeapObject&& other) noexcept {
		if (this != &other) {
			name = std::move(other.name);
			type = other.type;
			typeLocked = other.typeLocked;
		}
		return *this;
	}

	virtual ~HeapObject() {
		if (gc_prev) gc_prev->gc_next = gc_next;
		if (gc_next) gc_next->gc_prev = gc_prev;
		if (gc_head == this) gc_head = gc_next;
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
	static std::vector<uint32_t> karatsubaMul(const std::vector<uint32_t>& A, const std::vector<uint32_t>& B) {
		if (A.size() < 32 || B.size() < 32) {
			size_t n = A.size(), m = B.size();
			if (n == 0 || m == 0) return {0};
			std::vector<uint32_t> res(n + m, 0);
			for (size_t i = 0; i < n; i++) {
				uint64_t carry = 0;
				for (size_t j = 0; j < m; j++) {
					uint64_t prod = (uint64_t)A[i] * B[j] + res[i + j] + carry;
					res[i + j] = (uint32_t)(prod & 0xFFFFFFFF);
					carry = prod >> 32;
				}
				res[i + m] += (uint32_t)carry;
			}
			while(res.size() > 1 && res.back() == 0) res.pop_back();
			return res;
		}
		
		size_t m = std::max(A.size(), B.size());
		size_t k = m / 2;
		
		std::vector<uint32_t> A_0(A.begin(), A.begin() + std::min(A.size(), k));
		std::vector<uint32_t> A_1(A.begin() + std::min(A.size(), k), A.end());
		if (A_1.empty()) A_1.push_back(0);
		if (A_0.empty()) A_0.push_back(0);
		
		std::vector<uint32_t> B_0(B.begin(), B.begin() + std::min(B.size(), k));
		std::vector<uint32_t> B_1(B.begin() + std::min(B.size(), k), B.end());
		if (B_1.empty()) B_1.push_back(0);
		if (B_0.empty()) B_0.push_back(0);
		
		std::vector<uint32_t> Z_2 = karatsubaMul(A_1, B_1);
		std::vector<uint32_t> Z_0 = karatsubaMul(A_0, B_0);
		
		BigIntObject a0(A_0, false), a1(A_1, false);
		BigIntObject b0(B_0, false), b1(B_1, false);
		BigIntObject sumA = a0.absAdd(a1);
		BigIntObject sumB = b0.absAdd(b1);
		
		std::vector<uint32_t> Z_1 = karatsubaMul(sumA.chunks, sumB.chunks);
		
		BigIntObject z1(Z_1, false), z2(Z_2, false), z0(Z_0, false);
		z1 = z1.absSub(z2).absSub(z0);
		
		BigIntObject res(Z_0, false);
		BigIntObject z1_shifted = z1;
		if (!(z1_shifted.chunks.size() == 1 && z1_shifted.chunks[0] == 0)) {
			z1_shifted.chunks.insert(z1_shifted.chunks.begin(), k, 0);
			res = res.absAdd(z1_shifted);
		}
		
		BigIntObject z2_shifted = z2;
		if (!(z2_shifted.chunks.size() == 1 && z2_shifted.chunks[0] == 0)) {
			z2_shifted.chunks.insert(z2_shifted.chunks.begin(), 2 * k, 0);
			res = res.absAdd(z2_shifted);
		}
		
		return res.chunks;
	}

	BigIntObject operator*(const BigIntObject &other) const {
		return BigIntObject(karatsubaMul(chunks, other.chunks), isNegative != other.isNegative);
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
	BigIntObject operator&(const BigIntObject &other) const {
		BigIntObject result(0);
		size_t maxLen = std::max(chunks.size(), other.chunks.size());
		result.chunks.resize(maxLen, 0);
		for (size_t i = 0; i < maxLen; ++i) {
			uint32_t a = (i < chunks.size()) ? chunks[i] : (isNegative ? 0xFFFFFFFF : 0);
			uint32_t b = (i < other.chunks.size()) ? other.chunks[i] : (other.isNegative ? 0xFFFFFFFF : 0);
			result.chunks[i] = a & b;
		}
		result.isNegative = isNegative & other.isNegative;
		result.trim();
		return result;
	}
	BigIntObject operator|(const BigIntObject &other) const {
		BigIntObject result(0);
		size_t maxLen = std::max(chunks.size(), other.chunks.size());
		result.chunks.resize(maxLen, 0);
		for (size_t i = 0; i < maxLen; ++i) {
			uint32_t a = (i < chunks.size()) ? chunks[i] : (isNegative ? 0xFFFFFFFF : 0);
			uint32_t b = (i < other.chunks.size()) ? other.chunks[i] : (other.isNegative ? 0xFFFFFFFF : 0);
			result.chunks[i] = a | b;
		}
		result.isNegative = isNegative | other.isNegative;
		result.trim();
		return result;
	}
	BigIntObject operator^(const BigIntObject &other) const {
		BigIntObject result(0);
		size_t maxLen = std::max(chunks.size(), other.chunks.size());
		result.chunks.resize(maxLen, 0);
		for (size_t i = 0; i < maxLen; ++i) {
			uint32_t a = (i < chunks.size()) ? chunks[i] : (isNegative ? 0xFFFFFFFF : 0);
			uint32_t b = (i < other.chunks.size()) ? other.chunks[i] : (other.isNegative ? 0xFFFFFFFF : 0);
			result.chunks[i] = a ^ b;
		}
		result.isNegative = isNegative ^ other.isNegative;
		result.trim();
		return result;
	}
	BigIntObject operator<<(long long shift) const {
		if (shift < 0) return operator>>(-shift);
		if (shift == 0 || chunks.empty()) return *this;
		BigIntObject result(0);
		size_t chunkShift = shift / 32;
		size_t bitShift = shift % 32;
		result.chunks.resize(chunks.size() + chunkShift + 1, 0);
		uint32_t carry = 0;
		for (size_t i = 0; i < chunks.size(); ++i) {
			result.chunks[i + chunkShift] = (chunks[i] << bitShift) | carry;
			carry = (bitShift == 0) ? 0 : (chunks[i] >> (32 - bitShift));
		}
		result.chunks[chunks.size() + chunkShift] = carry;
		result.isNegative = isNegative;
		result.trim();
		return result;
	}
	BigIntObject operator>>(long long shift) const {
		if (shift < 0) return operator<<(-shift);
		if (shift == 0 || chunks.empty()) return *this;
		BigIntObject result(0);
		size_t chunkShift = shift / 32;
		size_t bitShift = shift % 32;
		if (chunkShift >= chunks.size()) {
			return BigIntObject(isNegative ? -1 : 0);
		}
		result.chunks.resize(chunks.size() - chunkShift, 0);
		uint32_t carry = isNegative ? (0xFFFFFFFF << (32 - bitShift)) : 0;
		for (long long i = chunks.size() - 1; i >= (long long)chunkShift; --i) {
			result.chunks[i - chunkShift] = (chunks[i] >> bitShift) | carry;
			carry = (bitShift == 0) ? 0 : (chunks[i] << (32 - bitShift));
		}
		result.isNegative = isNegative;
		result.trim();
		return result;
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
	virtual Value* get_ptr() = 0;
};
struct ListRefObject : RefObject {
	std::shared_ptr<ListObject> list;
	int index;
	ListRefObject(std::shared_ptr<ListObject> l, int i)
		 : list(l), index(i) {}
	Value* get_ptr() override { return &list->elements[index]; }
	void breakCycles() override { list.reset(); }
};
struct VarRefObject : RefObject {
	std::shared_ptr<Env> env;
	std::string name;
	VarRefObject(std::shared_ptr<Env> e, const std::string &n)
		 : env(e), name(n) {}
	Value* get_ptr() override;
	void breakCycles() override { env.reset(); }
};
struct PropRefObject : RefObject {
	std::shared_ptr<InstanceObject> inst;
	std::string name;
	PropRefObject(std::shared_ptr<InstanceObject> i, const std::string &n)
		 : inst(i), name(n) {}
	Value* get_ptr() override { return &inst->fields[name]; }
	void breakCycles() override { inst.reset(); }
};
struct DictRefObject : RefObject {
	std::shared_ptr<DictObject> dict;
	Value key;
	DictRefObject(std::shared_ptr<DictObject> d, const Value &k)
		 : dict(d), key(k) {}
	Value* get_ptr() override { return &dict->items[key]; }
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
inline Value Value::BigInt(const std::string& str) {
	Value v;
	v.type = ValueType::BIGINT;
	std::vector<uint32_t> chunks;
	bool isNegative = false;
	size_t start = 0;
	if (str.length() > 0 && str[0] == '-') {
		isNegative = true;
		start = 1;
	} else if (str.length() > 0 && str[0] == '+') {
		start = 1;
	}
	
	// Parse base 10
	chunks.push_back(0);
	for (size_t i = start; i < str.length(); i++) {
		if (str[i] < '0' || str[i] > '9') continue;
		uint32_t digit = str[i] - '0';
		
		// Multiply chunks by 10 and add digit
		uint64_t carry = digit;
		for (size_t j = 0; j < chunks.size(); j++) {
			uint64_t p = (uint64_t)chunks[j] * 10 + carry;
			chunks[j] = (uint32_t)(p & 0xFFFFFFFF);
			carry = p >> 32;
		}
		if (carry > 0) {
			chunks.push_back((uint32_t)carry);
		}
	}
	v.ref = std::make_shared<BigIntObject>(chunks, isNegative);
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
inline Value Value::Instance(Value classDef) {
	if (classDef.type != ValueType::CLASS)
		throw RuntimeError("Cannot create instance from non-class type", 0, 0);
	Value v;
	v.type = ValueType::INSTANCE;
	auto *clsPtr = static_cast<ClassObject *>(classDef.ref.get());
#ifdef VM_DEBUG_MODE
	v.__DEBUGGING__NAME__ = clsPtr->name;
#endif
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
	if (type == ValueType::VECTOR)
		return !static_cast<VectorObject *>(ref.get())->elements.empty();
	if (type == ValueType::BIGINT) {
		auto *b = static_cast<BigIntObject *>(ref.get());
		return b->chunks.size() > 1 || (b->chunks.size() == 1 && b->chunks[0] != 0);
	}
	if (type == ValueType::NONE || type == ValueType::NOTYPE)
		return false;
	return true;
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
	case ValueType::INSTANCE: {
      auto *inst1 = static_cast<InstanceObject *>(ref.get());
      auto *inst2 = static_cast<InstanceObject *>(other.ref.get());
      if (inst1 == inst2) return true;
      if (inst1->klass->name != inst2->klass->name) return false;
      if (inst1->fields.size() != inst2->fields.size()) return false;
      for (const auto &[k, v] : inst1->fields) {
         if (inst2->fields.find(k) == inst2->fields.end() || !v.strictEquals(inst2->fields.at(k))) return false;
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
Value shallowCopy(const Value &val);
Value deepCopy(const Value &val);
Value applyCopy(const Value &v, CopyMode mode);
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
	void set(const string &n, Value v, bool locked, bool isConstVar = false) {
#ifdef VM_DEBUG_MODE
		v.__DEBUGGING__NAME__ = n;
#endif
		auto it = vars.find(n);
		if (it != vars.end()) {
			Var &existing = it->second;
			if (existing.isLocked && existing.value.type != v.type) {
				throw RuntimeError(
					"Type mismatch: variable '" + n + "' is type-locked.", 0, 0);
			}

			if (existing.isConst) {
				if (existing.value.type == v.type && existing.value.ref == v.ref) {
					return; // Already set to the exact same constant reference, safely skip!
				}
				throw RuntimeError("Cannot reassign a constant variable '" + n + "'", 0, 0);
			}

			existing.value = v;
			v.isLocked = existing.isLocked;
			v.isConst = existing.isConst;
			return;
		}
		v.isLocked = locked;
		v.isConst = isConstVar;
		if (locked && v.ref)
			v.ref->typeLocked = true;
		vars[n] = Var{v, nullptr, isConstVar, locked};
	}
	Value get(const string &n) {
		Var &var = lookup(n);
		Value val = var.alias ? *var.alias : var.value;
		val.isConst = var.isConst;
		val.isLocked = var.isLocked;
		return val;
	}
	void assign(const string &n, Value v) {
		Var &var = lookup(n);

		if (var.alias) {
			*var.alias = v;
			return;
		}

		var.value = v;
	}
	bool exists(const string &n) {
		if (vars.count(n))
			return true;
		if (parent)
			return parent->exists(n); // <--- Recursively check parent
		return false;
	}
};

inline Value* VarRefObject::get_ptr() {
	return &env->lookup(name).value;
}

inline Value* Value::get_ptr_safe() const {
	if (ref && ref->type == ValueType::REFERENCE) return static_cast<RefObject*>(ref.get())->get_ptr();
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
static inline std::string bigIntToString(BigIntObject *big) {
	if (big->chunks.empty())
		return "0";
	if (big->chunks.size() == 1 && big->chunks[0] == 0)
		return "0";
	std::vector<uint32_t> temp = big->chunks;
	std::string res = "";
	while (temp.size() > 1 || temp[0] > 0) {
		uint32_t rem = divMod1e9(temp);
		std::string part = std::to_string(rem);
		if (temp.size() > 1 || temp[0] > 0) {
			// Pad with zeros up to 9 digits
			res += std::string(9 - part.length(), '0') + part;
		} else {
			res += part;
		}
	}
	if (res.empty())
		return "0";
	std::string final_res = "";
	if (big->isNegative)
		final_res += "-";
	
	// Reverse 9-digit chunks by reversing the whole string then reversing each chunk?
	// Wait, we append chunks in reverse order (least significant first).
	// So we should build a vector of strings or insert at front.
	// Since string prepend is slow, let's collect chunks.
	std::vector<std::string> parts;
	temp = big->chunks;
	while (temp.size() > 1 || temp[0] > 0) {
		uint32_t rem = divMod1e9(temp);
		std::string part = std::to_string(rem);
		if (temp.size() > 1 || temp[0] > 0) {
			part = std::string(9 - part.length(), '0') + part;
		}
		parts.push_back(part);
	}
	for (int i = parts.size() - 1; i >= 0; i--) {
		final_res += parts[i];
	}
	if (final_res.empty()) return "0";
	return final_res;
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
		return "NoType";
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
		return valueToString(*(v.get_ptr_safe()));
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
static inline std::string PrintStackForDebug(const std::vector<Value> &stack) {
	std::string result = "start -> [";
	for (auto val = stack.begin(); val < stack.end(); val++) {
		result += PrintProperty(*val);
		switch (val->type) {
		case ValueType::NOTYPE:
			result += "NoType";
			break;
		case ValueType::NONE:
			result += "None";
			break;
		case ValueType::INT:
			result += "Int";
			break;
		case ValueType::FLOAT:
			result += "Float";
			break;
		case ValueType::STRING:
			result += "String";
			break;
		case ValueType::BOOL:
			result += "Bool";
			break;
		case ValueType::LIST:
			result += "List";
			break;
		case ValueType::VECTOR:
			result += "Vector";
			break;
		case ValueType::DICT:
			result += "Dict";
			break;
		case ValueType::SLICE:
			result += "Slice";
			break;
		case ValueType::BIGINT:
			result += "BigInt";
			break;
		case ValueType::PAIRED:
			result += "Paired";
			break;
		case ValueType::RANGE:
			result += "Range";
			break;
		case ValueType::TUPLE:
			result += "Tuple";
			break;
		case ValueType::SET:
			result += "Set";
			break;
		case ValueType::FUNCTION:
			result += "Func";
			break;
		case ValueType::NATIVE_FUNCTION:
			result += "NFunc";
			break;
		case ValueType::FILE:
			result += "File";
			break;
		case ValueType::OVERLOAD:
			result += "Overload";
			break;
		case ValueType::OMIT_MARKER:
			result += "OmitMarker";
			break;
		case ValueType::REFERENCE:
			result += "Refrance";
			break;
		case ValueType::ERROR:
			result += "Error";
			break;
		case ValueType::CLASS:
			result += "Class";
			break;
		case ValueType::INSTANCE:
			result += "Instance";
			break;
		case ValueType::SUPER:
			result += "Super";
			break;
		default:
			result += "Unknown";
			break;
		}
		if (DEBUGGER_MODE_IS_ENABLED) {
			result += " ";
#ifdef VM_DEBUG_MODE
			result += val->__DEBUGGING__NAME__;
#endif
			result += (" " + valueToString(*val));
		}
		if (val != stack.end() - 1)
			result += ", ";
	}
	return result + "] <- end";
}
static inline bool lessValue(const Value &a, const Value &b, std::shared_ptr<Env> globals, std::function<Value(MethodCallExpr *)> methodResolver, std::function<void(std::string, std::vector<std::string>, std::shared_ptr<Env>)> importResolver);
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
	vector<Stmt *> body;
	std::shared_ptr<Env> closure;
	bool isCached;
	ClassObject *owner = nullptr;
	Chunk *chunk;
	std::unordered_map<vector<Value>, Value, VectorHash, VectorEqual> cache;
	std::unordered_map<Value, Value, ValueHash, ValueEqual> singleArgCache;
	FunctionObject(const vector<ParamSpec> &p, ValueType rt, vector<Expr *> dra,
		bool rc, const vector<Stmt *> &b, std::shared_ptr<Env> c,
		bool cached, Chunk *ch = nullptr)
		 : HeapObject(ValueType::FUNCTION),
			params(p),
			returnType(rt),
			defaultRetArgs(dra),
			returnsConst(rc),
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
	__count__, // (was __len__)    Size of container
	__at__,	  // (was __get__)    val = obj[key]
	__put__,	  // (was __set__)    obj[key] = val
	__call__, // () operator to call like a function
	__has__, // (was __contains__) if x is in obj
	__lacks__,  // if x is not in obj
	__missing__, // if obj is not in x
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
static inline std::string magic_methods_to_string(Magic_Methods Magic_method) {
	switch (Magic_method) {
	case Magic_Methods::__construct__: // creates the object and returns the
												  // instance
		return "__construct__";
	case Magic_Methods::__destruct__: // deletes the instance and returns
												 // None
		return "__destruct__";
	case Magic_Methods::__copy__: // shallowley copies the instance
		return "__copy__";
	case Magic_Methods::__clone__: // deeply copies the instance
		return "__clone__";
	case Magic_Methods::__ref__: // creates an alias to the instance
		return "__ref__";
	case Magic_Methods::__mro__: // returns a list of meethod resolution
										  // order
		return "__mro__";
	case Magic_Methods::__display__: // called by the print() function
		return "__display__";
	case Magic_Methods::__inspect__: // used for debugging and printing
												// nested objects
		return "__inspect__";
	case Magic_Methods::__plus__: // + operator overload for normal addition
											// obj1 + obj2
		return "__plus__";
	case Magic_Methods::__r_plus__: // + opertaor overload for reverse
											  // addition obj2 + obj1
		return "__r_plus__";
	case Magic_Methods::__minus__: // - operator overload for normal
											 // subtraction obj1 - obj2
		return "__minus__";
	case Magic_Methods::__r_minus__: // - operator overload for reverse
												// subtraction obj2 - obj1
		return "__r_minus__";
	case Magic_Methods::__times__: // * operator overload for normal
											 // multiplication obj1 * obj2
		return "__times__";
	case Magic_Methods::__r_times__: // * operator overload for reverse
												// multiplication obj2 * obj1
		return "__r_times__";
	case Magic_Methods::__divide__: // / operator overload for normal
											  // division obj1 / obj2
		return "__divide__";
	case Magic_Methods::__r_divide__: // / operator overload for reverse
												 // division obj2 / obj1
		return "__r_divide__";
	case Magic_Methods::__int_divide__: // // operator overload for integer
													// division obj1 // obj2
		return "__int_divide__";
	case Magic_Methods::__r_int_divide__: // // operator overload for reverse
													  // integre division obj2 // obj1
		return "__r_int_divide__";
	case Magic_Methods::__power__: // ** operator overload for normal
											 // exponentiation obj1 ** obj2
		return "__power__";
	case Magic_Methods::__r_power__: // ** operator overload for reverse
												// exponentiation obj2 ** obj1
		return "__r_power__";
	case Magic_Methods::__modulo__: // % operator overlaod for normal
											  // modulation obj1 % obj2
		return "__modulo__";
	case Magic_Methods::__r_modulo__: // % operator overload for reverse
												 // modulation obj2 % obj1
		return "__r_modulo__";
	case Magic_Methods::__plus_eq__: // += operator overload for addition
												// assignment obj1 += obj2
		return "__plus_eq__";
	case Magic_Methods::__minus_eq__: // -= operator overload for subtraction
												 // assignment obj1 -= obj2
		return "__minus_eq__";
	case Magic_Methods::__times_eq__: // *= operator overload for
												 // multiplication assignment obj1 *=
												 // obj2
		return "__times_eq__";
	case Magic_Methods::__divide_eq__: // /= operator overload for division
												  // assignment obj1 /= obj2
		return "__divide_eq__";
	case Magic_Methods::__int_divide_eq__: // //= operator overload for
														// integer division assignment
														// obj1 //= obj2
		return "__int_divide_eq__";
	case Magic_Methods::__power_eq__: // **= operator overload for
												 // exponentiation assignment obj1 **=
												 // obj2
		return "__power_eq__";
	case Magic_Methods::__modulo_eq__: // %= operator overload for modulation
												  // assignment obj1 %= obj2
		return "__modulo_eq__";
	case Magic_Methods::__positive__: // + operator overload for unary
												 // positive +obj
		return "__positive__";
	case Magic_Methods::__negative__: // - operator overload for unary
												 // negative -obj
		return "__negative__";
	case Magic_Methods::__increment__: // ++ operator overload for unary
												  // increment obj++
		return "__increment__";
	case Magic_Methods::__decrement__: // -- operator overload for unary
												  // decrement obj--
		return "__decrement__";
	case Magic_Methods::__invert__: // ~ operator overload for unary bitwise
											  // not ~obj
		return "__invert__";
	case Magic_Methods::__and__: // and operator overload for normal
										  // bitwise and obj1 and obj2
		return "__bit_and__";
	case Magic_Methods::__r_and__: // and operator overload for reverse
											 // bitwise and obj2 and obj1
		return "__r_bit_and__";
	case Magic_Methods::__or__: // or operator overload for normal
										 // bitwise or obj1 or obj2
		return "__bit_or__";
	case Magic_Methods::__r_or__: // or operator overload for reverse
											// bitwise or obj2 or obj1
		return "__r_bit_or__";
	case Magic_Methods::__xor__: // xor operator overload for normal
										  // bitwise xor obj1 xor obj2
		return "__bit_xor__";
	case Magic_Methods::__r_xor__: // xor operator overload for reverse
											 // bitwise xor obj2 xor obj1
		return "__r_bit_xor__";
	case Magic_Methods::__nand__: // nand operator overload for normal
											// bitwise nand obj1 nand obj2
		return "__bit_nand__";
	case Magic_Methods::__r_nand__: // nand operator overload for reverse
											  // bitwise nand obj2 nand obj1
		return "__r_bit_nand__";
	case Magic_Methods::__nor__: // nor operator overload for normal
										  // bitwise nor obj1 nor obj2
		return "__bit_nor__";
	case Magic_Methods::__r_nor__: // nor operator overload for reverse
											 // bitwise nor obj2 nor obj1
		return "__r_bit_nor__";
	case Magic_Methods::__nxor__: // nxor operator overload for normal
											// bitwise nxor obj1 nxor obj2
		return "__bit_nxor__";
	case Magic_Methods::__r_nxor__: // nxor operator overload for reverse
											  // bitwise nxor obj2 nxor obj1
		return "__r_bit_nxor__";
	case Magic_Methods::__and_equals__: // &= operator overload for bitwise
													// and assignment obj1 &= obj2
		return "__and_equals__";
	case Magic_Methods::__or_equals__: // |= operator overload for bitwise or
												  // assignment obj1 |= obj2
		return "__or_equals__";
	case Magic_Methods::__xor_equals__: // ^= operator overload for bitwise
													// xor assignment obj1 ^= obj2
		return "__xor_equals__";
	case Magic_Methods::__equals__: // == operator overload for equality obj1
											  // == obj2
		return "__equals__";
	case Magic_Methods::__differs__: // != operator overload for inequality
												// obj1 != obj2
		return "__differs__";
	case Magic_Methods::__identical__: // === operator overload for strict
												  // equality obj1 === obj2
		return "__identical__";
	case Magic_Methods::__distinct__: // !== operator overload for strict
												 // inequality obj1 !== obj2
		return "__distinct__";
	case Magic_Methods::__less__: // < operator overload for less than obj1 <
											// obj2
		return "__less__";
	case Magic_Methods::__less_eq__: // <= operator overload for less than or
												// equal to obj1 <= obj2
		return "__less_eq__";
	case Magic_Methods::__greater__: // > operator overload for greater than
												// obj1 > obj2
		return "__greater__";
	case Magic_Methods::__greater_eq__: // >= operator overload for greater
													// than or equal to obj1 >= obj2
		return "__greater_eq__";
	case Magic_Methods::__count__: // length() operator overload for counting
											 // items in a container length(obj)
		return "__count__";
	case Magic_Methods::__at__: // [] operator overload for getting an item
										 // from a container obj[key]
		return "__at__";
	case Magic_Methods::__put__: // [] operator overload for setting an item
										  // in a container obj[key] = val
		return "__put__";
	case Magic_Methods::__has__: // is in operator overload for checking if a
										  // container has an item if x is in obj
		return "__has__";
	case Magic_Methods::__missing__: // is not in operator overload for
												// checking if a container is missing an
												// item if x is not in obj
		return "__missing__";
	case Magic_Methods::__assign__: // . operator overload for setting an
											  // attribute obj.attr = val
		return "__assign__";
	case Magic_Methods::__traverse__: // iter() operator overload for getting
												 // an iterator from a container
												 // iter(obj)
		return "__traverse__";
	case Magic_Methods::__advance__: // next() operator overload for
												// advancing an iterator to the next
												// item next(obj)
		return "__advance__";
	case Magic_Methods::__to_int__: // int() operator overload for converting
											  // an object to an integer int(obj)
		return "__to_int__";
	case Magic_Methods::__to_string__: // string() operator overload for
												  // converting an object to a string
												  // string(obj)
		return "__to_string__";
	case Magic_Methods::__to_bool__: // bool() operator overload for
												// converting an object to a boolean
												// bool(obj)
		return "__to_bool__";
	case Magic_Methods::__to_list__: // list() operator overload for
												// converting an object to a list
												// list(obj)
		return "__to_list__";
	case Magic_Methods::__to_set__: // set() operator overload for converting
											  // an object to a set set(obj)
		return "__to_set__";
	case Magic_Methods::__to_dict__: // dict() operator overload for
												// converting an object to a dictionary
												// dict(obj)
		return "__to_dict__";
	case Magic_Methods::__to_tuple__: // tuple() operator overload for
												 // converting an object to a tuple
												 // tuple(obj)
		return "__to_tuple__";
	case Magic_Methods::__to_vector__: // vector() operator overload for
												  // converting an object to a vector
												  // vector(obj)
		return "__to_vector__";
	case Magic_Methods::__var_count__: // returns the number of variables in
												  // the class
		return "__var_count__";
	case Magic_Methods::__var_names__: // returns a list of variable names in
												  // the class
		return "__var_names__";
	case Magic_Methods::__function_count__: // returns the number of
														 // functions in the class
		return "__function_count__";
	case Magic_Methods::__function_names__: // returns a list of function
														 // names in the class
		return "__function_names__";
	case Magic_Methods::__all_count__: // returns the total number of
												  // variables and functions in the
												  // class
		return "__all_count__";
	case Magic_Methods::__all_names__: // returns a list of all variable and
												  // function names in the class
		return "__all_names__";
	default: // Should never happen, but just in case
		return "_";
	}
}
static inline Color ValueToColor(const Value &v, int l, int c) {
	if (v.type != ValueType::INSTANCE)
		throw TypeError("Expected Color object", l, c);
	auto *inst = static_cast<InstanceObject *>(v.ref.get());
	if (inst->klass->name != "Color")
		throw TypeError("Expected Color object", l, c);
	return Color{(unsigned char)inst->fields["r"].asInt(),
		(unsigned char)inst->fields["g"].asInt(),
		(unsigned char)inst->fields["b"].asInt(),
		(unsigned char)inst->fields["a"].asInt()};
}
static inline Rectangle ValueToRect(const Value &v, int l, int c) {
	if (v.type != ValueType::INSTANCE)
		throw TypeError("Expected Rectangle object", l, c);
	auto *inst = static_cast<InstanceObject *>(v.ref.get());
	if (inst->klass->name != "Rectangle")
		throw TypeError("Expected Rectangle object", l, c);
	return Rectangle{(float)inst->fields["x"].asInt(),
		(float)inst->fields["y"].asInt(),
		(float)inst->fields["width"].asInt(),
		(float)inst->fields["height"].asInt()};
}
static inline ::Vector2 ValueToVector2(const Value &v, int l, int c) {
	if (v.type != ValueType::VECTOR)
		throw TypeError("Expected Vector object", l, c);
	auto *vec = static_cast<VectorObject *>(v.ref.get());
	if (vec->elements.size() < 2)
		throw ValueError("Vector must have at least 2 elements for Vector2", l,
			c);
	return ::Vector2{(float)vec->elements[0].asFloat(),
		(float)vec->elements[1].asFloat()};
}
static inline auto Vector2ToValue = [](::Vector2 v) -> Value {
	std::vector<Value> elems;
	elems.reserve(2);
	elems.push_back(Value::Float(v.x));
	elems.push_back(Value::Float(v.y));
	return Value::Vector(elems);
};
static inline std::vector<::Vector2> ValueToVectorList(const Value &v, int l, int c) {
	if (v.type != ValueType::LIST)
		throw TypeError("Expected List of Vectors", l, c);
	auto *list = static_cast<ListObject *>(v.ref.get());
	std::vector<::Vector2> points;
	points.reserve(list->elements.size());
	for (const auto &el : list->elements)
		points.push_back(ValueToVector2(el, l, c));
	return points;
}
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
static inline std::string OpCodeToString(OpCode num) {
	switch (num) {
	case OpCode::OP_CONSTANT:
		return "OP_CONSTANT";
	case OpCode::OP_CONSTANT_LONG:
		return "OP_CONSTANT_LONG";
	case OpCode::OP_TRUE:
		return "OP_TRUE";
	case OpCode::OP_FALSE:
		return "OP_FALSE";
	case OpCode::OP_NONE:
		return "OP_NONE";
	case OpCode::OP_NOTYPE:
		return "OP_NOTYPE";
	case OpCode::OP_DEFINE_VAR:
		return "OP_DEFINE_VAR";
	case OpCode::OP_GET_VAR:
		return "OP_GET_VAR";
	case OpCode::OP_SET_VAR:
		return "OP_SET_VAR";
	case OpCode::OP_DEEP_COPY:
		return "OP_DEEP_COPY";
	case OpCode::OP_REF_LOCAL:
		return "OP_REF_LOCAL";
	case OpCode::OP_DEFINE_REF:
		return "OP_DEFINE_REF";
	case OpCode::OP_REF_VAR:
		return "OP_REF_VAR";
	case OpCode::OP_REF_INDEX:
		return "OP_REF_INDEX";
	case OpCode::OP_SET_REF:
		return "OP_SET_REF";
	case OpCode::OP_SHALLOW_COPY:
		return "OP_SHALLOW_COPY";
	case OpCode::OP_MULTI_SET:
		return "OP_MULTI_LET";
	case OpCode::OP_GET_LOCAL:
		return "OP_GET_LOCAL";
	case OpCode::OP_SET_LOCAL:
		return "OP_SET_LOCAL";
	case OpCode::OP_INC_LOCAL:
		return "OP_INC_LOCAL";
	case OpCode::OP_SET_FLAGS:
		return "OP_SET_FLAGS";
	case OpCode::OP_REF_PROPERTY:
		return "OP_REF_PROPERTY";
	case OpCode::OP_DELETE:
		return "OP_DELETE";
	case OpCode::OP_ADD:
		return "OP_ADD";
	case OpCode::OP_FAST_IADD: return "OP_FAST_IADD";
	case OpCode::OP_FAST_ISUB: return "OP_FAST_ISUB";
	case OpCode::OP_FAST_IMUL: return "OP_FAST_IMUL";
	case OpCode::OP_FAST_IDIV: return "OP_FAST_IDIV";
	case OpCode::OP_FAST_FADD: return "OP_FAST_FADD";
	case OpCode::OP_FAST_FSUB: return "OP_FAST_FSUB";
	case OpCode::OP_FAST_FMUL: return "OP_FAST_FMUL";
	case OpCode::OP_FAST_FDIV: return "OP_FAST_FDIV";
	case OpCode::OP_FAST_SADD: return "OP_FAST_SADD";
	case OpCode::OP_CTLE_TICK: return "OP_CTLE_TICK";
	case OpCode::OP_SUB:
		return "OP_SUB";
	case OpCode::OP_MUL:
		return "OP_MUL";
	case OpCode::OP_DIV:
		return "OP_DIV";
	case OpCode::OP_FLOOR_DIV:
		return "OP_FLOOR_DIV";
	case OpCode::OP_MOD:
		return "OP_MOD";
	case OpCode::OP_POW:
		return "OP_POW";
	case OpCode::OP_IADD:
		return "OP_IADD";
	case OpCode::OP_ISUB:
		return "OP_ISUB";
	case OpCode::OP_IMUL:
		return "OP_IMUL";
	case OpCode::OP_IDIV:
		return "OP_IDIV";
	case OpCode::OP_IFLOOR_DIV:
		return "OP_IFLOOR_DIV";
	case OpCode::OP_IMOD:
		return "OP_IMOD";
	case OpCode::OP_IPOW:
		return "OP_IPOW";
	case OpCode::OP_DUP:
		return "OP_DUP";
	case OpCode::OP_DUP_2:
		return "OP_DUP_2";
	case OpCode::OP_EQ:
		return "OP_EQ";
	case OpCode::OP_NEQ:
		return "OP_NEQ";
	case OpCode::OP_LT:
		return "OP_LT";
	case OpCode::OP_GT:
		return "OP_GT";
	case OpCode::OP_LTE:
		return "OP_LTE";
	case OpCode::OP_GTE:
		return "OP_GTE";
	case OpCode::OP_COLON:
		return "OP_COLON";
	case OpCode::OP_STRICT_NEQ:
		return "OP_STRICT_NEQ";
	case OpCode::OP_NOT:
		return "OP_NOT";
	case OpCode::OP_AND:
		return "OP_AND";
	case OpCode::OP_OR:
		return "OP_OR";
	case OpCode::OP_XOR:
		return "OP_XOR";
	case OpCode::OP_IS:
		return "OP_IS";
	case OpCode::OP_IN:
		return "OP_IN";
	case OpCode::OP_IS_NOT:
		return "OP_IS_NOT";
	case OpCode::OP_STRICT_EQ:
		return "OP_STRICT_EQ";
	case OpCode::OP_IS_IN:
		return "OP_IS_IN";
	case OpCode::OP_IS_NOT_IN:
		return "OP_IS_NOT_IN";
	case OpCode::OP_NXOR:
		return "OP_NXOR";
	case OpCode::OP_NAND:
		return "OP_NAND";
	case OpCode::OP_NOR:
		return "OP_NOR";
	case OpCode::OP_NEGATE:
		return "OP_NEGATE";
	case OpCode::OP_INCREMENT:
		return "OP_INCREMENT";
	case OpCode::OP_DECREMENT:
		return "OP_DECREMENT";
	case OpCode::OP_BITWISE_AND:
		return "OP_BITWISE_AND";
	case OpCode::OP_BITWISE_OR:
		return "OP_BITWISE_OR";
	case OpCode::OP_BITWISE_XOR:
		return "OP_BITWISE_XOR";
	case OpCode::OP_BITWISE_NOT:
		return "OP_BITWISE_NOT";
	case OpCode::OP_BITWISE_SHL:
		return "OP_BITWISE_SHL";
	case OpCode::OP_BITWISE_SHR:
		return "OP_BITWISE_SHR";
	case OpCode::OP_PRE_INCREMENT:
		return "OP_PRE_INCREMENT";
	case OpCode::OP_PRE_DECREMENT:
		return "OP_PRE_DECREMENT";
	case OpCode::OP_BUILD_LIST:
		return "OP_BUILD_LIST";
	case OpCode::OP_BUILD_TUPLE:
		return "OP_BUILD_TUPLE";
	case OpCode::OP_BUILD_SET:
		return "OP_BUILD_SET";
	case OpCode::OP_BUILD_DICT:
		return "OP_BUILD_DICT";
	case OpCode::OP_UNPACK_DICT:
		return "OP_UNPACK_DICT";
	case OpCode::OP_BUILD_RANGE:
		return "OP_BUILD_RANGE";
	case OpCode::OP_BUILD_VECTOR:
		return "OP_BUILD_VECTOR";
	case OpCode::OP_BUILD_FSTRING:
		return "OP_BUILD_FSTRING";
	case OpCode::OP_BUILD_FILE:
		return "OP_BUILD_FILE";
	case OpCode::OP_BUILD_SLICE:
		return "OP_BUILD_SLICE";
	case OpCode::OP_CLASS:
		return "OP_CLASS";
	case OpCode::OP_METHOD:
		return "OP_METHOD";
	case OpCode::OP_GET_PROPERTY:
		return "OP_GET_PROPERTY";
	case OpCode::OP_SET_PROPERTY:
		return "OP_SET_PROPERTY";
	case OpCode::OP_INC_PROPERTY:
		return "OP_INC_PROPERTY";
	case OpCode::OP_CLASS_FIELD:
		return "OP_CLASS_FIELD";
	case OpCode::OP_SUPER:
		return "OP_SUPER";
	case OpCode::OP_LIST_APPEND:
		return "OP_LIST_APPEND";
	case OpCode::OP_SET_ADD:
		return "OP_SET_ADD";
	case OpCode::OP_DICT_SET:
		return "OP_DICT_SET";
	case OpCode::OP_LIST_TO_TUPLE:
		return "OP_LIST_TO_TUPLE";
	case OpCode::OP_LIST_TO_VECTOR:
		return "OP_LIST_TO_VECTOR";
	case OpCode::OP_GET_INDEX:
		return "OP_GET_INDEX";
	case OpCode::OP_SET_INDEX:
		return "OP_SET_INDEX";
	case OpCode::OP_INC_INDEX:
		return "OP_INC_INDEX";
	case OpCode::OP_INVOKE:
		return "OP_INVOKE";
	case OpCode::OP_CALL:
		return "OP_CALL";
	case OpCode::OP_JUMP:
		return "OP_JUMP";
	case OpCode::OP_JUMP_IF_FALSE:
		return "OP_JUMP_IF_FALSE";
	case OpCode::OP_LOOP:
		return "OP_LOOP";
	case OpCode::OP_RETURN:
		return "OP_RETURN";
	case OpCode::OP_TO_STREAM:
		return "OP_TO_STREAM";
	case OpCode::OP_JUMP_IF_NOT_LT:
		return "OP_JUMP_IF_NOT_LT";
	case OpCode::OP_BREAK:
		return "OP_BREAK";
	case OpCode::OP_CONTINUE:
		return "OP_CONTINUE";
	case OpCode::OP_SKIP:
		return "OP_SKIP";
	case OpCode::OP_OMIT:
		return "OP_OMIT";
	case OpCode::OP_FOR_ITER:
		return "OP_FOR_ITER";
	case OpCode::OP_SKIP_ITER:
		return "OP_SKIP_ITER";
	case OpCode::OP_SWITCH_TABLE:
		return "OP_SWITCH_TABLE";
	case OpCode::OP_THROW:
		return "OP_THROW";
	case OpCode::OP_ASSERT:
		return "OP_ASSERT";
	case OpCode::OP_IMPORT:
		return "OP_IMPORT";
	case OpCode::OP_POP:
		return "OP_POP";
	case OpCode::OP_DEBUG_NAME:
		return "OP_DEBUG_NAME";
	case OpCode::OP_REPL_PRINT:
		return "OP_REPL_PRINT";
	case OpCode::OP_TRY_ENTER:
		return "OP_TRY_ENTER";
	case OpCode::OP_TRY_EXIT:
		return "OP_TRY_EXIT";
	case OpCode::OP_CATCH:
		return "OP_CATCH";
	case OpCode::OP_RETHROW:
		return "OP_RETHROW";
	case OpCode::OP_END_FINALLY:
		return "OP_END_FINALLY";
	default:
		return "UNKNOWN";
	}
}
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
		if (p.defaultChunk) delete p.defaultChunk;
	}
	for (auto *c : defaultRetChunks) {
		delete c;
	}
	if (chunk) delete chunk;
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
inline Value EvaluateConstBinary(TokenType op, const Value &a, const Value &b);
// ------------ AST WALKER -------------
struct Interpreter {
	std::shared_ptr<Env> env;
	unordered_map<string, FuncVal *> funcs;
	using ModuleLoader =
		std::function<void(std::shared_ptr<Env>, const vector<string> &)>;
	std::unordered_map<string, ModuleLoader> modules;
	bool returning = false;
	Value returnValue;
	bool breaking = false;
	bool continuing = false;
	int skipCount = 0;
	bool vectorEnabled = false;
	std::unordered_set<string> importStack;
	Interpreter() {
		enableColors();
		env = std::make_shared<Env>();
		env->set("None", Value::None(), true);
		registerStdLib();
	}
	Value nativePrint(const vector<Value> &args, int l, int c);
	void registerStdLib();
	LValue resolveLValue(Expr *e) {
		if (ValueExpr *ve = dynamic_cast<ValueExpr *>(e)) {
			if (ve->sourcePtr)
				return LValue(ve->sourcePtr, true);
			return LValue(&ve->val, false);
		}
		if (auto v = dynamic_cast<VarExpr *>(e)) {
			if (!env->exists(v->name))
				throw NameError("Undefined variable '" + v->name + "'", e->line,
					e->col);
			Var &var = env->lookup(v->name);
			if (var.alias)
				return LValue(var.alias, true, var.isConst, var.isLocked);
			return LValue(&var.value, false, var.isConst, var.isLocked);
		}
		if (auto idx = dynamic_cast<IndexExpr *>(e)) {
			LValue base = resolveLValue(idx->base);
			Value &container = *base.ref;
			int i = eval(idx->index).asInt();
			if (container.type == ValueType::LIST) {
				auto *listObj = static_cast<ListObject *>(container.ref.get());
				if (i < 0 || i > (int)listObj->elements.size())
					throw IndexError("List index out of range", e->line, e->col);
				if (i == (int)listObj->elements.size())
					listObj->elements.push_back(Value::None());

				return LValue(&listObj->elements[i], true,
					base.isConstView || container.isConst);
			}
			if (container.type == ValueType::STRING)
				return LValue(&container, true);

			throw TypeError("Type is not indexable or mutable", e->line, e->col);
		}
		throw TypeError("Expression is not assignable", e->line, e->col);
	}
	Value Resolve_methods(MethodCallExpr *m);
	bool matchesError(const LangError &e, const string &typeName) {
		// --- ROOT ---
		if (typeName == "Error")
			return true;
		// --- LEVEL 1: BASE CATEGORIES ---
		if (typeName == "InternalError")
			return dynamic_cast<const InternalError *>(&e);
		if (typeName == "ControlFlowError")
			return dynamic_cast<const ControlFlowError *>(&e);
		if (typeName == "ParseError")
			return dynamic_cast<const ParseError *>(&e);
		if (typeName == "RuntimeError")
			return dynamic_cast<const RuntimeError *>(&e);
		if (typeName == "Warning")
			return dynamic_cast<const Warning *>(&e);
		// --- LEVEL 2: RUNTIME ERROR SUBTREE ---
		if (typeName == "NameError")
			return dynamic_cast<const NameError *>(&e);
		if (typeName == "AttributeError")
			return dynamic_cast<const AttributeError *>(&e);
		if (typeName == "TypeError")
			return dynamic_cast<const TypeError *>(&e);
		if (typeName == "ArgumentError")
			return dynamic_cast<const ArgumentError *>(&e);
		if (typeName == "ValueError")
			return dynamic_cast<const ValueError *>(&e);
		if (typeName == "ConstError")
			return dynamic_cast<const ConstError *>(&e);
		if (typeName == "OwnershipError")
			return dynamic_cast<const OwnershipError *>(&e);
		if (typeName == "IndexError")
			return dynamic_cast<const IndexError *>(&e);
		if (typeName == "KeyError")
			return dynamic_cast<const KeyError *>(&e);
		if (typeName == "RangeError")
			return dynamic_cast<const RangeError *>(&e);
		if (typeName == "AssertionError")
			return dynamic_cast<const AssertionError *>(&e);
		if (typeName == "RecursionError")
			return dynamic_cast<const RecursionError *>(&e);
		if (typeName == "ImportError")
			return dynamic_cast<const ImportError *>(&e);
		if (typeName == "IOError")
			return dynamic_cast<const IOError *>(&e);
		if (typeName == "MathError")
			return dynamic_cast<const MathError *>(&e);
		if (typeName == "CastError")
			return dynamic_cast<const CastError *>(&e);
		if (typeName == "IteratorError")
			return dynamic_cast<const IteratorError *>(&e);
		if (typeName == "TimeoutError")
			return dynamic_cast<const TimeoutError *>(&e);
		if (typeName == "MemoryError")
			return dynamic_cast<const MemoryError *>(&e);
		if (typeName == "SystemError")
			return dynamic_cast<const SystemError *>(&e);
		// Children of ValueError
		if (typeName == "EmptyContainerError")
			return dynamic_cast<const EmptyContainerError *>(&e);
		// Children of ConstError
		if (typeName == "MutationError")
			return dynamic_cast<const MutationError *>(&e);
		// Children of ImportError
		if (typeName == "ModuleNotFoundError")
			return dynamic_cast<const ModuleNotFoundError *>(&e);
		if (typeName == "CircularImportError")
			return dynamic_cast<const CircularImportError *>(&e);
		if (typeName == "InvalidImportError")
			return dynamic_cast<const InvalidImportError *>(&e);
		// Children of IOError
		if (typeName == "FileNotFoundError")
			return dynamic_cast<const FileNotFoundError *>(&e);
		if (typeName == "PermissionError")
			return dynamic_cast<const PermissionError *>(&e);
		if (typeName == "EOFError")
			return dynamic_cast<const EOFError *>(&e);
		if (typeName == "FileClosedError")
			return dynamic_cast<const FileClosedError *>(&e);
		// Children of MathError
		if (typeName == "DivisionByZeroError")
			return dynamic_cast<const DivisionByZeroError *>(&e);
		if (typeName == "OverflowError")
			return dynamic_cast<const OverflowError *>(&e);
		if (typeName == "UnderflowError")
			return dynamic_cast<const UnderflowError *>(&e);
		if (typeName == "DomainError")
			return dynamic_cast<const DomainError *>(&e);
		// Children of SystemError
		if (typeName == "OSError")
			return dynamic_cast<const OSError *>(&e);
		if (typeName == "EnvironmentError")
			return dynamic_cast<const EnvironmentError *>(&e);
		if (typeName == "SignalError")
			return dynamic_cast<const SignalError *>(&e);
		// Children of ControlFlowError
		if (typeName == "ReturnSignal")
			return dynamic_cast<const ReturnSignal *>(&e);
		if (typeName == "BreakSignal")
			return dynamic_cast<const BreakSignal *>(&e);
		if (typeName == "ContinueSignal")
			return dynamic_cast<const ContinueSignal *>(&e);
		// Children of ParseError
		if (typeName == "SyntaxError")
			return dynamic_cast<const SyntaxError *>(&e);
		if (typeName == "IndentationError")
			return dynamic_cast<const IndentationError *>(&e);
		if (typeName == "UnexpectedTokenError")
			return dynamic_cast<const UnexpectedTokenError *>(&e);
		if (typeName == "UnterminatedLiteralError")
			return dynamic_cast<const UnterminatedLiteralError *>(&e);
		// Children of Warning
		if (typeName == "DeprecationWarning")
			return dynamic_cast<const DeprecationWarning *>(&e);
		if (typeName == "RuntimeWarning")
			return dynamic_cast<const RuntimeWarning *>(&e);
		if (typeName == "ImportWarning")
			return dynamic_cast<const ImportWarning *>(&e);
		// Fallback: Direct String Match (For exact matches or user-defined types)
		return e.type == typeName;
	}
	Value eval(Expr *e) {
		if (!e)
			return Value::None();
		if (ValueExpr *ve = dynamic_cast<ValueExpr *>(e))
			return ve->val;
		switch (e->type) {
		case ExprType::FSTRING: {
			auto fs = static_cast<FStringExpr *>(e);
			string res = "";
			for (auto *part : fs->parts) {
				Value v = eval(part);
				if (v.type == ValueType::STRING)
					res += v.asString();
				else
					res += valueToString(v);
			}
			return Value::String(res);
		}
		case ExprType::OWNERSHIP: {
			auto o = static_cast<OwnershipExpr *>(e);
			if (o->mode == CopyMode::REF) {
				LValue lv = resolveLValue(o->expr);
				return *lv.ref;
			}
			Value v = eval(o->expr);
			return applyCopy(v, o->mode);
		}
		case ExprType::BOOL: {
			return Value::Bool(static_cast<BoolExpr *>(e)->value);
		}
		case ExprType::NUMBER: {
			auto n = static_cast<NumberExpr *>(e);
			return n->isFloat ? Value::Float(n->val)
									: Value::Int((long long)n->val);
		}
		case ExprType::STRING: {
			return Value::String(static_cast<StringExpr *>(e)->val);
		}
		case ExprType::VAR: {
			auto *v = static_cast<VarExpr *>(e);
			if (v->cachedGlobal)
				return *v->cachedGlobal;
			if (!env->exists(v->name))
				throw NameError("Undefined variable '" + v->name + "'", e->line,
					e->col);
			Var &var = env->lookup(v->name);
			if (env->parent == nullptr)
				v->cachedGlobal = &var.value;
			if (var.alias)
				return *var.alias;
			return var.value;
		}
		case ExprType::LIST: {
			auto l = static_cast<ListExpr *>(e);
			vector<Value> vals;
			for (auto el : l->elements)
				vals.push_back(eval(el));
			return Value::List(vals);
		}
		case ExprType::BINARY: {
			auto b = static_cast<BinExpr *>(e);
			if (b->left && b->right && b->left->type == ExprType::NUMBER &&
				 b->right->type == ExprType::NUMBER) {
				double l = static_cast<NumberExpr *>(b->left)->val;
				double r = static_cast<NumberExpr *>(b->right)->val;
				switch (b->op) {
				case TokenType::PLUS:
					return Value::Float(l + r);
				case TokenType::MINUS:
					return Value::Float(l - r);
				case TokenType::STAR:
					return Value::Float(l * r);
				case TokenType::SLASH:
					return r != 0 ? Value::Float(l / r) : Value::None();
				case TokenType::POW:
					return Value::Float(pow(l, r));
				}
			}
			if (b->op == TokenType::AND) {
				Value l = eval(b->left);
				if (!l.isTruthy())
					return Value::Bool(false);
				return Value::Bool(eval(b->right).isTruthy());
			}
			if (b->op == TokenType::OR) {
				Value l = eval(b->left);
				if (l.isTruthy())
					return Value::Bool(true);
				return Value::Bool(eval(b->right).isTruthy());
			}
			Value l = b->left ? eval(b->left) : Value::None();
			Value r = eval(b->right);
			switch (b->op) {
			case TokenType::PLUS:
				if (l.type == ValueType::STRING ||
					 r.type == ValueType::STRING)
					return Value::String(
						(l.type == ValueType::STRING
								? l.asString()
								: (l.type == ValueType::FLOAT
										  ? std::to_string(l.asFloat())
										  : std::to_string(l.asInt()))) +
						(r.type == ValueType::STRING
								? r.asString()
								: (r.type == ValueType::FLOAT
										  ? std::to_string(r.asFloat())
										  : std::to_string(r.asInt()))));
				if (l.type == ValueType::INT && r.type == ValueType::INT) {
					long long a = l.asInt();
					long long b = r.asInt();
					if ((b > 0 && a > LLONG_MAX - b) ||
						 (b < 0 && a < LLONG_MIN - b))
						return Value::Float((double)a + (double)b);
					return Value::Int(a + b);
				}
				if (l.isNumber() && r.isNumber()) {
					return Value::Float(l.asFloat() + r.asFloat());
				}
				if (l.type == ValueType::VECTOR &&
					 r.type == ValueType::VECTOR) {
					auto *v1 = static_cast<VectorObject *>(l.ref.get());
					auto *v2 = static_cast<VectorObject *>(r.ref.get());
					if (v1->elements.size() != v2->elements.size())
						throw ValueError(
							"Vector dimension mismatch in addition", e->line,
							e->col);
					vector<Value> res;
					for (size_t i = 0; i < v1->elements.size(); i++)
						res.push_back(Value::Float(v1->elements[i].asFloat() +
															v2->elements[i].asFloat()));
					return Value::Vector(res);
				}
				throw TypeError("Invalid operands for +", e->line, e->col);
			case TokenType::FLOOR_DIV:
				if (r.asFloat() == 0)
					throw DivisionByZeroError("Division by zero", e->line,
						e->col);
				if (l.isNumber() && r.isNumber())
					return Value::Int(
						(long long)std::floor(l.asFloat() / r.asFloat()));
				break;
			case TokenType::MOD:
				if (r.asInt() == 0)
					throw DivisionByZeroError("Modulo by zero", e->line,
						e->col);
				if (l.type == ValueType::INT && r.type == ValueType::INT)
					return Value::Int(l.asInt() % r.asInt());
				if (l.isNumber() && r.isNumber()) {
					return Value::Float(std::fmod(l.asFloat(), r.asFloat()));
				}
				break;
			case TokenType::POW:
				if (l.isNumber() && r.isNumber())
					return Value::Float(std::pow(l.asFloat(), r.asFloat()));
				break;
			case TokenType::MINUS:
				if (l.isNumber() && r.isNumber()) {
					if (l.type == ValueType::FLOAT ||
						 r.type == ValueType::FLOAT)
						return Value::Float(l.asFloat() - r.asFloat());
					return Value::Int(l.asInt() - r.asInt());
				}
				if (l.type == ValueType::VECTOR &&
					 r.type == ValueType::VECTOR) {
					auto *v1 = static_cast<VectorObject *>(l.ref.get());
					auto *v2 = static_cast<VectorObject *>(r.ref.get());
					if (v1->elements.size() != v2->elements.size())
						throw ValueError(
							"Vector dimension mismatch in subtraction", e->line,
							e->col);
					vector<Value> res;
					for (size_t i = 0; i < v1->elements.size(); i++)
						res.push_back(Value::Float(v1->elements[i].asFloat() -
															v2->elements[i].asFloat()));
					return Value::Vector(res);
				}
				break;
			case TokenType::STAR:
				if (l.type == ValueType::INT && r.type == ValueType::INT) {
					long long a = l.asInt();
					long long b = r.asInt();
					if (a != 0 &&
						 (b > LLONG_MAX / abs(a) || b < LLONG_MIN / abs(a)))
						return Value::Float((double)a * (double)b);
					return Value::Int(a * b);
				}
				if (l.type == ValueType::STRING && r.type == ValueType::INT) {
					string res = "";
					string base = l.asString();
					long long count = r.asInt();
					if (count < 0)
						count = 0;
					if (count > 1000000)
						throw MemoryError("String repetition too large",
							e->line, e->col);
					for (long long i = 0; i < count; i++)
						res += base;
					return Value::String(res);
				}
				if (l.type == ValueType::LIST && r.type == ValueType::INT) {
					auto *listObj = static_cast<ListObject *>(l.ref.get());
					long long count = r.asInt();
					vector<Value> res;
					if (count > 0) {
						if (listObj->elements.size() * count > 1000000)
							throw MemoryError("List repetition too large",
								e->line, e->col);
						res.reserve(listObj->elements.size() * count);
						for (int i = 0; i < count; i++)
							for (const auto &elem : listObj->elements)
								res.push_back(deepCopy(elem));
					}
					return Value::List(res);
				}
				if (l.isNumber() && r.isNumber()) {
					return Value::Float(l.asFloat() * r.asFloat());
				}
				if (l.type == ValueType::VECTOR && r.isNumber()) {
					auto *v = static_cast<VectorObject *>(l.ref.get());
					double s = r.asFloat();
					vector<Value> res;
					for (auto val : v->elements)
						res.push_back(Value::Float(val.asFloat() * s));
					return Value::Vector(res);
				}
				if (l.isNumber() && r.type == ValueType::VECTOR) {
					auto *v = static_cast<VectorObject *>(r.ref.get());
					double s = l.asFloat();
					vector<Value> res;
					for (auto val : v->elements)
						res.push_back(Value::Float(val.asFloat() * s));
					return Value::Vector(res);
				}
				if (l.type == ValueType::VECTOR &&
					 r.type == ValueType::VECTOR) {
					auto *v1 = static_cast<VectorObject *>(l.ref.get());
					auto *v2 = static_cast<VectorObject *>(r.ref.get());
					if (v1->elements.size() != v2->elements.size())
						throw ValueError(
							"Vector dimension mismatch in multiplication",
							e->line, e->col);
					vector<Value> res;
					for (size_t i = 0; i < v1->elements.size(); i++)
						res.push_back(Value::Float(v1->elements[i].asFloat() *
															v2->elements[i].asFloat()));
					return Value::Vector(res);
				}
				break;
			case TokenType::SLASH:
				if (r.asFloat() == 0)
					throw DivisionByZeroError("Division by zero", e->line,
						e->col);
				if (l.isNumber() && r.isNumber())
					return Value::Float(l.asFloat() / r.asFloat());
				if (l.type == ValueType::VECTOR ||
					 r.type == ValueType::VECTOR)
					throw TypeError("Vector division is not supported",
						e->line, e->col);
				break;
			case TokenType::GT:
				return Value::Bool(l.asFloat() > r.asFloat());
			case TokenType::LT:
				return Value::Bool(l.asFloat() < r.asFloat());
			case TokenType::GTE:
				return Value::Bool(l.asFloat() >= r.asFloat());
			case TokenType::LTE:
				return Value::Bool(l.asFloat() <= r.asFloat());
			case TokenType::EQ:
				return Value::Bool(l.looseEquals(r));
			case TokenType::STRICT_EQ:
				return Value::Bool(l.strictEquals(r));
			case TokenType::STRICT_NEQ:
				return Value::Bool(!l.strictEquals(r));
			case TokenType::NEQ:
				return Value::Bool(!l.looseEquals(r));
			case TokenType::AND:
				if (l.type == ValueType::SET && r.type == ValueType::SET)
					break;
				return Value::Bool(l.isTruthy() && r.isTruthy());
			case TokenType::OR:
				if (l.type == ValueType::SET && r.type == ValueType::SET)
					break;
				return Value::Bool(l.isTruthy() || r.isTruthy());
			case TokenType::XOR:
				if (l.type == ValueType::SET && r.type == ValueType::SET)
					break;
				return Value::Bool(l.isTruthy() != r.isTruthy());
			case TokenType::NAND:
				return Value::Bool(!(l.isTruthy() && r.isTruthy()));
			case TokenType::NOR:
				return Value::Bool(!(l.isTruthy() || r.isTruthy()));
			case TokenType::NXOR:
				return Value::Bool(l.isTruthy() == r.isTruthy());
			case TokenType::NOT:
				return Value::Bool(!r.isTruthy());
			case TokenType::IS: {
				if (l.ref && r.ref)
					return Value::Bool(l.ref.get() == r.ref.get());
				if (l.type == ValueType::NONE && r.type == ValueType::NONE)
					return Value::Bool(true);
				return Value::Bool(false);
			}
			case TokenType::IS_IN: {
				if (r.type == ValueType::STRING) {
					if (l.type != ValueType::STRING) {
						return Value::Bool(false);
					}
					return Value::Bool(r.asString().find(l.asString()) !=
											 string::npos);
				}
				if (r.type == ValueType::LIST) {
					auto *listObj = static_cast<ListObject *>(r.ref.get());
					for (const auto &item : listObj->elements) {
						if (item.strictEquals(l))
							return Value::Bool(true);
					}
					return Value::Bool(false);
				}
				if (r.type == ValueType::DICT) {
					auto *d = static_cast<DictObject *>(r.ref.get());
					return Value::Bool(d->items.count(l));
				}
				if (r.type == ValueType::SET) {
					auto *s = static_cast<SetObject *>(r.ref.get());
					for (const auto &item : s->elements) {
						if (item.strictEquals(l))
							return Value::Bool(true);
					}
					return Value::Bool(false);
				}
				if (r.type == ValueType::RANGE) {
					auto *rng = static_cast<RangeObject *>(r.ref.get());
					if (!l.isNumber())
						return Value::Bool(false);
					double val = l.asFloat();
					if (!rng->isValid)
						return Value::Bool(false);
					bool inBounds =
						(rng->step > 0)
							? (val >= rng->start &&
								  (rng->endInclusive ? val <= rng->end
															: val < rng->end))
							: (val <= rng->start &&
								  (rng->endInclusive ? val >= rng->end
															: val > rng->end));
					if (!inBounds)
						return Value::Bool(false);
					if (!rng->isFloat && l.type == ValueType::INT) {
						long long diff = (long long)(val - rng->start);
						if (diff % (long long)rng->step != 0)
							return Value::Bool(false);
					}
					return Value::Bool(true);
				}
				throw TypeError("RHS of 'in' must be iterable", e->line,
					e->col);
			}
			case TokenType::IS_NOT: {
				if (l.ref && r.ref)
					return Value::Bool(l.ref.get() != r.ref.get());
				if (l.type == ValueType::NONE && r.type == ValueType::NONE)
					return Value::Bool(false);
				return Value::Bool(true);
			}
			case TokenType::IS_NOT_IN: {
				if (r.type == ValueType::STRING) {
					if (l.type != ValueType::STRING)
						throw TypeError(
							"LHS of 'is not in' string must be a string",
							e->line, e->col);
					return Value::Bool(r.asString().find(l.asString()) ==
											 string::npos);
				} else if (r.type == ValueType::LIST) {
					auto *listObj = static_cast<ListObject *>(r.ref.get());
					for (const auto &item : listObj->elements)
						if (item.looseEquals(l))
							return Value::Bool(false);
					return Value::Bool(true);
				} else if (r.type == ValueType::RANGE) {
					auto *rng = static_cast<RangeObject *>(r.ref.get());
					if (!l.isNumber())
						return Value::Bool(true);
					double val = l.asFloat();
					if (!rng->isValid)
						return Value::Bool(true);
					bool inBounds =
						(rng->step > 0)
							? (val >= rng->start &&
								  (rng->endInclusive ? val <= rng->end
															: val < rng->end))
							: (val <= rng->start &&
								  (rng->endInclusive ? val >= rng->end
															: val > rng->end));
					if (!inBounds)
						return Value::Bool(true);
					if (!rng->isFloat && l.type == ValueType::INT) {
						long long diff = (long long)(val - rng->start);
						if (diff % (long long)rng->step != 0)
							return Value::Bool(true);
					}
					return Value::Bool(false);
				} else if (r.type == ValueType::SET) {
					auto *s = static_cast<SetObject *>(r.ref.get());
					for (const auto &el : s->elements)
						if (el.strictEquals(l))
							return Value::Bool(false);
					return Value::Bool(true);
				} else if (r.type == ValueType::TUPLE) {
					auto *t = static_cast<TupleObject *>(r.ref.get());
					for (const auto &item : t->elements)
						if (item.looseEquals(l))
							return Value::Bool(false);
					return Value::Bool(true);
				}
				throw TypeError("RHS of 'is not in' must be iterable",
					e->line, e->col);
			}
			case TokenType::COLON: {
				Value k = l;
				Value v = r;
				vector<std::pair<Value, Value>> p;
				p.push_back({k, v});
				return Value::Paired(p);
			}
			default:
				return Value::None();
			}
			if (l.type == ValueType::SET && r.type == ValueType::SET) {
				auto *s1 = static_cast<SetObject *>(l.ref.get());
				auto *s2 = static_cast<SetObject *>(r.ref.get());
				std::unordered_set<Value, ValueHash, ValueEqual> result;
				if (b->op == TokenType::OR) {
					result = s1->elements;
					for (auto &v : s2->elements)
						setAdd(result, v);
					return Value::Set(result);
				}
				if (b->op == TokenType::AND) {
					for (auto &v2 : s2->elements) {
						if (s1->elements.count(v2))
							setAdd(result, v2);
					}
					return Value::Set(result);
				}
				if (b->op == TokenType::MINUS || b->op == TokenType::SLASH) {
					for (auto &v1 : s1->elements) {
						if (s2->elements.count(v1))
							continue;
						setAdd(result, v1);
					}
					return Value::Set(result);
				}
				if (b->op == TokenType::XOR) {
					for (auto &v1 : s1->elements) {
						if (s2->elements.count(v1))
							continue;
						setAdd(result, v1);
					}
					for (auto &v2 : s2->elements) {
						if (s1->elements.count(v2))
							continue;
						setAdd(result, v2);
					}
					return Value::Set(result);
				}
			}
		}
		case ExprType::OMIT_MARKER_EXPR: {
			return Value::Omit();
		}
		case ExprType::CALL: {
			auto c = static_cast<CallExpr *>(e);
			Value callee;
			if (c->cachedFunc) {
				callee = *c->cachedFunc;
			} else {
				if (!env->exists(c->name))
					throw NameError("Undefined function '" + c->name + "'",
						c->line, c->col);
				Var &v = env->lookup(c->name);
				callee = v.value;
				if (env->parent == nullptr)
					c->cachedFunc = &v.value;
			}
			vector<CallArg> callArgs;
			for (size_t i = 0; i < c->args.size(); i++) {
				CallArg ca;
				if (c->modes[i] == CopyMode::REF) {
					LValue lv = resolveLValue(c->args[i]);
					ca.lvalue = lv;
					ca.hasLValue = true;
					ca.value = *lv.ref;
				} else
					ca.value = eval(c->args[i]);
				callArgs.push_back(ca);
			}
			return call(callee, callArgs, c->modes, c->line, c->col);
		}
		case ExprType::INDEX: {
			auto idx = static_cast<IndexExpr *>(e);
			Value base = eval(idx->base);
			Value index = eval(idx->index);
			auto normalize = [&](long long i, size_t len) {
				if (i < 0)
					i += len;
				if (i < 0 || i >= (long long)len)
					throw IndexError("Index out of bounds", idx->line, idx->col);
				return i;
			};
			auto getSliceIndices = [&](size_t len) -> vector<long long> {
				auto *s = static_cast<SliceObject *>(index.ref.get());
				long long step =
					(s->step.type == ValueType::INT) ? s->step.asInt() : 1;
				if (step == 0)
					throw ValueError("Slice step cannot be zero", idx->line,
						idx->col);
				long long start, end;
				if (step > 0) {
					start =
						(s->start.type == ValueType::INT) ? s->start.asInt() : 0;
					end = (s->end.type == ValueType::INT) ? s->end.asInt()
																	  : (long long)len;
				} else {
					start = (s->start.type == ValueType::INT)
								  ? s->start.asInt()
								  : (long long)len - 1;
					end = (s->end.type == ValueType::INT) ? s->end.asInt()
																	  : -1LL - 1LL;
				}
				auto normalizeSlice = [&](long long val) {
					return (val < 0) ? val + (long long)len : val;
				};
				if (s->start.type == ValueType::INT)
					start = normalizeSlice(start);
				if (s->end.type == ValueType::INT)
					end = normalizeSlice(end);
				auto clamp = [&](long long val) {
					if (val < 0)
						return (step > 0) ? 0LL : -1LL;
					if (val > (long long)len)
						return (long long)len;
					return val;
				};
				if (step > 0) {
					if (start < 0)
						start = 0;
					if (end < 0)
						end = 0;
					if (start > (long long)len)
						start = len;
					if (end > (long long)len)
						end = len;
				} else {
					if (start < -1)
						start = -1;
					if (end < -1)
						end = -1;
					if (start > (long long)len - 1)
						start = len - 1;
					if (end > (long long)len - 1)
						end = len - 1;
				}
				vector<long long> result;
				if (step > 0) {
					for (long long i = start; i < end; i += step) {
						if (i >= 0 && i < (long long)len)
							result.push_back(i);
					}
				} else {
					for (long long i = start; i > end; i += step) {
						if (i >= 0 && i < (long long)len)
							result.push_back(i);
					}
				}
				return result;
			};
			if (base.type == ValueType::DICT) {
				auto *dictObj = static_cast<DictObject *>(base.ref.get());
				if (dictObj->items.find(index) == dictObj->items.end()) {
					throw KeyError("Key not found in dictionary", idx->line,
						idx->col);
				}
				return dictObj->items.at(index);
			}
			if (base.type == ValueType::LIST) {
				auto *list = static_cast<ListObject *>(base.ref.get());
				size_t len = list->elements.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					Value el = list->elements[i];
					if (base.isConst)
						el.isConst = true;
					return el;
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					auto *newList = new ListObject();
					for (long long i : indices) {
						newList->elements.push_back(list->elements[i]);
					}
					Value ret;
					ret.type = ValueType::LIST;
					ret.ref = shared_ptr<HeapObject>(newList);
					ret.isConst = base.isConst;
					return ret;
				}
				throw TypeError("List index must be int or slice", idx->line,
					idx->col);
			}
			if (base.type == ValueType::STRING) {
				const string &s = base.asString();
				size_t len = s.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					return Value::String(string(1, s[i]));
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					string newStr = "";
					for (long long i : indices)
						newStr += s[i];
					return Value::String(newStr);
				}
				throw TypeError("String index must be int or slice", idx->line,
					idx->col);
			}
			if (base.type == ValueType::TUPLE) {
				auto *t = static_cast<TupleObject *>(base.ref.get());
				size_t len = t->elements.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					return t->elements[i];
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					auto *newTuple = new TupleObject();
					for (long long i : indices)
						newTuple->elements.push_back(t->elements[i]);
					Value ret;
					ret.type = ValueType::TUPLE;
					ret.ref = shared_ptr<HeapObject>(newTuple);
					ret.isConst = base.isConst;
					return ret;
				}
				throw TypeError("Tuple index must be int or slice", idx->line,
					idx->col);
			}
			if (base.type == ValueType::RANGE) {
				if (index.type != ValueType::INT)
					throw TypeError("slice index must be int", idx->line,
						idx->col);
				auto *r = static_cast<RangeObject *>(base.ref.get());
				long long i = index.asInt();
				double val = r->start;
				if (!r->startInclusive)
					val += r->step;
				val += (r->step * i);
				bool outOfBounds = false;
				if (r->step > 0) {
					if (r->endInclusive) {
						if (val > r->end)
							outOfBounds = true;
					} else {
						if (val >= r->end)
							outOfBounds = true;
					}
				} else {
					if (r->endInclusive) {
						if (val < r->end)
							outOfBounds = true;
					} else {
						if (val <= r->end)
							outOfBounds = true;
					}
				}
				if (outOfBounds || i < 0)
					throw IndexError("slice index out of bounds", idx->line,
						idx->col);
				return r->isFloat ? Value::Float(val)
										: Value::Int((long long)val);
			}
			if (base.type == ValueType::SET) {
				auto *setObj = static_cast<SetObject *>(base.ref.get());
				size_t len = setObj->elements.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					auto it = setObj->elements.begin();
					std::advance(it, i);
					Value val = *it;
					if (base.isConst)
						val.isConst = true;
					return val;
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					auto *newSet = new SetObject();
					for (long long idx_val : indices) {
						auto it = setObj->elements.begin();
						std::advance(it, idx_val);
						setAdd(newSet->elements, *it);
					}
					Value ret;
					ret.type = ValueType::SET;
					ret.ref = shared_ptr<HeapObject>(newSet);
					ret.isConst = base.isConst;
					return ret;
				}
				throw TypeError("Set index must be int or slice", idx->line,
					idx->col);
			}
			if (base.type == ValueType::VECTOR) {
				auto *vec = static_cast<VectorObject *>(base.ref.get());
				size_t len = vec->elements.size();
				if (index.type == ValueType::INT) {
					long long i = normalize(index.asInt(), len);
					return Value::Float(vec->elements[i].asFloat());
				} else if (index.type == ValueType::SLICE) {
					auto indices = getSliceIndices(len);
					vector<Value> newElems;
					for (long long i : indices) {
						newElems.push_back(vec->elements[i]);
					}
					return Value::Vector(newElems);
				}
				throw TypeError("Vector index must be int or slice", idx->line,
					idx->col);
			}
			throw TypeError("Indexing non-indexable type", idx->line, idx->col);
		}
		case ExprType::METHOD_CALL: {
			return Resolve_methods(static_cast<MethodCallExpr *>(e));
		}
		case ExprType::LAMBDA: {
			auto lam = static_cast<LambdaExpr *>(e);
			auto *funcObj = new FunctionObject(
				lam->params, lam->returnType, lam->defaultRetArgs,
				lam->returnsConst, lam->body, env, lam->isCached);
			Value v;
			v.type = ValueType::FUNCTION;
			v.ref = std::shared_ptr<HeapObject>(funcObj);
			return v;
		}
		case ExprType::RANGE: {
			auto r = static_cast<RangeExpr *>(e);
			Value s = eval(r->start);
			Value en = eval(r->end);
			if (!s.isNumber() || !en.isNumber())
				throw TypeError("Range start and end must be numbers", r->line,
					r->col);
			double startVal = s.asFloat();
			double endVal = en.asFloat();
			double stepVal = 1.0;
			bool isFloat =
				(s.type == ValueType::FLOAT || en.type == ValueType::FLOAT);
			if (r->step == nullptr) {
				if (startVal > endVal)
					stepVal = -1.0;
				else
					stepVal = 1.0;
			} else {
				Value st = eval(r->step);
				if (!st.isNumber())
					throw TypeError("Range step must be a number", r->line,
						r->col);
				stepVal = st.asFloat();
				if (st.type == ValueType::FLOAT)
					isFloat = true;
			}
			if (stepVal == 0)
				throw ValueError("Range step cannot be zero", r->line, r->col);
			return Value::Range(startVal, endVal, stepVal, r->startInclusive,
				r->endInclusive, isFloat);
		}
		case ExprType::SET: {
			auto s = static_cast<SetExpr *>(e);
			std::unordered_set<Value, ValueHash, ValueEqual> vals;
			for (auto el : s->elements)
				setAdd(vals, eval(el));
			return Value::Set(vals);
		}
		case ExprType::DICT: {
			auto d = static_cast<DictExpr *>(e);
			std::unordered_map<Value, Value, ValueHash, ValueEqual> map;
			for (auto &item : d->items) {
				Value key = eval(item.first);
				Value val = eval(item.second);
				if (key.type == ValueType::LIST || key.type == ValueType::SET) {
					key = deepCopy(key);
					key.isConst = true;
				}
				if (key.type == ValueType::DICT)
					throw TypeError("Dictionary cannot be used as a key", e->line,
						e->col);
				map[key] = val;
			}
			return Value::Dict(map);
		}
		case ExprType::TUPLE: {
			auto t = static_cast<TupleExpr *>(e);
			vector<Value> vals;
			for (auto el : t->elements)
				vals.push_back(eval(el));
			return Value::Tuple(vals);
		}
		case ExprType::TERNARY: {
			auto t = static_cast<TernaryExpr *>(e);
			Value cond = eval(t->condition);
			if (cond.isTruthy())
				return eval(t->trueBranch);
			else
				return eval(t->falseBranch);
		}
		case ExprType::COMPREHENSION: {
			auto comp = static_cast<CompExpr *>(e);
			Value collection = eval(comp->iterable);
			vector<Value> items;
			bool isSet = false;
			if (collection.type == ValueType::LIST)
				items = static_cast<ListObject *>(collection.ref.get())->elements;
			else if (collection.type == ValueType::SET) {
				auto *s = static_cast<SetObject *>(collection.ref.get());
				items.assign(s->elements.begin(), s->elements.end());
			} else if (collection.type == ValueType::TUPLE)
				items =
					static_cast<TupleObject *>(collection.ref.get())->elements;
			else if (collection.type == ValueType::STRING) {
				for (char c : collection.asString())
					items.push_back(Value::String(string(1, c)));
			} else if (collection.type == ValueType::RANGE) {
				auto *r = static_cast<RangeObject *>(collection.ref.get());
				double cur = r->start;
				if (!r->startInclusive)
					cur += r->step;
				while (true) {
					bool cond =
						(r->step > 0)
							? (r->endInclusive ? cur <= r->end : cur < r->end)
							: (r->endInclusive ? cur >= r->end : cur > r->end);
					if (!cond)
						break;
					items.push_back(r->isFloat ? Value::Float(cur)
														: Value::Int((long long)cur));
					cur += r->step;
				}
			} else if (collection.type == ValueType::DICT) {
				auto *d = static_cast<DictObject *>(collection.ref.get());
				for (auto &pair : d->items)
					items.push_back(pair.first);
			} else if (collection.type == ValueType::VECTOR) {
				auto *v = static_cast<VectorObject *>(collection.ref.get());
				for (auto d : v->elements)
					items.push_back(d);
			} else
				throw TypeError("Comprehension 'in' target must be iterable", comp->line, comp->col);
			vector<Value> listResults;
			std::unordered_set<Value, ValueHash, ValueEqual> setResults;
			std::unordered_map<Value, Value, ValueHash, ValueEqual> dictResults;
			bool isDict = (comp->typeToken == TokenType::LBRACE && comp->valueExpr != nullptr);
			bool isSetComp = (comp->typeToken == TokenType::LBRACE && comp->valueExpr == nullptr);
			std::shared_ptr<Env> prevEnv = env;
			for (const auto &item : items) {
				env = std::make_shared<Env>();
				env->parent = prevEnv;
				env->set(comp->varName, item, false, false);
				if (comp->filter) {
					Value cond = eval(comp->filter);
					if (!cond.isTruthy()) {
						continue;
					}
				}
				if (isDict) {
					Value k = eval(comp->expression);
					if (k.type == ValueType::LIST || k.type == ValueType::SET || k.type == ValueType::DICT) {
						k = deepCopy(k);
						k.isConst = true;
					}
					Value v = eval(comp->valueExpr);
					dictResults[k] = v;
				} else {
					Value v = eval(comp->expression);
					if (isSetComp) {
						if (v.type == ValueType::LIST || v.type == ValueType::SET || v.type == ValueType::DICT) {
							v = deepCopy(v);
							v.isConst = true;
						}
						setResults.insert(v);
					} else {
						listResults.push_back(v);
					}
				}
			}
			env = prevEnv;
			if (comp->typeToken == TokenType::LBRACKET)
				return Value::List(listResults);
			if (comp->typeToken == TokenType::LPAREN)
				return Value::Tuple(listResults);
			if (comp->typeToken == TokenType::LBRACE) {
				if (isDict)
					return Value::Dict(dictResults);
				else
					return Value::Set(setResults);
			}
			return Value::None();
		}
		case ExprType::SLICE: {
			auto sl = static_cast<SliceExpr *>(e);
			Value s = sl->start ? eval(sl->start) : Value::None();
			Value en = sl->end ? eval(sl->end) : Value::None();
			Value st = sl->step ? eval(sl->step) : Value::None();
			return Value::Slice(s, en, st);
		}
		case ExprType::VECTOR: {
			auto ve = static_cast<VectorExpr *>(e);
			if (!vectorEnabled)
				throw RuntimeError(
					"Vector syntax <...> requires 'import Vector'", e->line,
					e->col);
			vector<Value> elems;
			for (auto *el : ve->elements) {
				Value v = eval(el);
				if (!v.isNumber())
					throw TypeError("Vector elements must be numbers", e->line,
						e->col);
				elems.push_back(v);
			}
			return Value::Vector(elems);
		}
		default: {
			return Value::None();
		}
		}
	}
	Value exec(Stmt *s) {
		switch (s->type) {
		case StmtType::ASSIGN: {
			auto as = static_cast<AssignStmt *>(s);
			if (as->target->type == ExprType::INDEX) {
				auto idx = static_cast<IndexExpr *>(as->target);
				Value base = eval(idx->base);
				if (base.type == ValueType::STRING) {
					Value rhs = eval(as->value);
					if (rhs.type != ValueType::STRING)
						throw TypeError("Can only insert string into string",
							s->line, s->col);
					int i = eval(idx->index).asInt();
					auto *strObj = static_cast<StringObject *>(base.ref.get());
					string &str = strObj->value;
					if (i < 0 || i > (int)str.size())
						throw IndexError("String index out of bounds", s->line,
							s->col);
					if (i == (int)str.size())
						str += rhs.asString();
					else {
						const string &r = rhs.asString();
						int eraseLen =
							std::min((int)r.size(), (int)str.size() - i);
						str.erase(i, eraseLen);
						str.insert(i, r);
					}
					return Value::None();
				}
				if (base.type == ValueType::DICT) {
					if (base.isConst)
						throw ConstError("Cannot modify const dictionary", s->line,
							s->col);
					Value key = eval(idx->index);
					Value val = eval(as->value);
					if (key.type == ValueType::LIST ||
						 key.type == ValueType::SET) {
						key = deepCopy(key);
						key.isConst = true;
					}
					if (key.type == ValueType::DICT)
						throw TypeError("Dictionary cannot be used as a key",
							s->line, s->col);
					auto *dictObj = static_cast<DictObject *>(base.ref.get());
					dictObj->items[key] = val;
					return Value::None();
				}
			}
			LValue lv = resolveLValue(as->target);
			Value &cur = *lv.ref;
			if (cur.isConst || lv.isConstView)
				throw ConstError("Cannot assign to or modify const variable",
					s->line, s->col);
			Value rhs = eval(as->value);
			if (lv.isLocked && cur.type != rhs.type) {
				throw TypeError("Cannot change type of locked variable", s->line,
					s->col);
			}
			if (as->op == TokenType::ASSIGN) {
				if (lv.isRefTarget) {
					*lv.ref = rhs;
					return Value::None();
				}
				if (cur.ref && cur.ref->typeLocked && cur.type != rhs.type) {
					throw TypeError("Cannot change type of locked variable",
						s->line, s->col);
				}
				if (cur.ref && cur.type == rhs.type) {
					if (cur.type == ValueType::STRING)
						static_cast<StringObject *>(cur.ref.get())->value =
							rhs.asString();
					else
						cur = rhs;
				} else
					cur = rhs;
				return Value::None();
			}
			if (cur.type == ValueType::STRING) {
				if (rhs.type == ValueType::STRING &&
					 as->op == TokenType::PLUS_EQ) {
					static_cast<StringObject *>(cur.ref.get())->value +=
						rhs.asString();
					return Value::None();
				}
				if (rhs.type == ValueType::INT && as->op == TokenType::STAR_EQ) {
					string base =
						static_cast<StringObject *>(cur.ref.get())->value;
					long long count = rhs.asInt();
					if (count < 0)
						count = 0;
					string res = "";
					if (base.length() * count > 10000000)
						throw MemoryError("Resulting string too large", s->line,
							s->col);
					for (int k = 0; k < count; k++)
						res += base;
					static_cast<StringObject *>(cur.ref.get())->value = res;
					return Value::None();
				}
			}
			if (cur.type == ValueType::LIST && rhs.type == ValueType::INT &&
				 as->op == TokenType::STAR_EQ) {
				auto *listObj = static_cast<ListObject *>(cur.ref.get());
				int count = rhs.asInt();
				if (count <= 0)
					listObj->elements.clear();
				else if (count > 1) {
					vector<Value> orig = listObj->elements;
					if (orig.size() * count > 1000000)
						throw MemoryError("Resulting list too large", s->line,
							s->col);
					listObj->elements.reserve(orig.size() * count);
					for (int k = 1; k < count; k++)
						listObj->elements.insert(listObj->elements.end(),
							orig.begin(), orig.end());
				}
				return Value::None();
			}
			if (cur.type == ValueType::FLOAT && rhs.type == ValueType::FLOAT) {
				switch (as->op) {
				case TokenType::PLUS_EQ:
					cur.fVal += rhs.asFloat();
					break;
				case TokenType::MINUS_EQ:
					cur.fVal -= rhs.asFloat();
					break;
				case TokenType::STAR_EQ:
					cur.fVal *= rhs.asFloat();
					break;
				case TokenType::DIV_EQ:
					if (rhs.asFloat() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.fVal /= rhs.asFloat();
					break;
				case TokenType::POW_EQ:
					cur.fVal = std::pow(cur.fVal, rhs.asFloat());
					break;
				default:
					throw TypeError("Invalid float operation", s->line,
						s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::FLOAT && rhs.type == ValueType::INT) {
				switch (as->op) {
				case TokenType::PLUS_EQ:
					cur.fVal += rhs.asFloat();
					break;
				case TokenType::MINUS_EQ:
					cur.fVal -= rhs.asFloat();
					break;
				case TokenType::STAR_EQ:
					cur.fVal *= rhs.asFloat();
					break;
				case TokenType::DIV_EQ:
					if (rhs.asFloat() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.fVal /= rhs.asFloat();
					break;
				case TokenType::POW_EQ:
					cur.fVal = std::pow(cur.fVal, rhs.asFloat());
					break;
				default:
					throw TypeError("Invalid float operation", s->line,
						s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::INT && rhs.type == ValueType::INT) {
				switch (as->op) {
				case TokenType::PLUS_EQ:
					cur.iVal += rhs.asInt();
					break;
				case TokenType::MINUS_EQ:
					cur.iVal -= rhs.asInt();
					break;
				case TokenType::STAR_EQ:
					cur.iVal *= rhs.asInt();
					break;
				case TokenType::DIV_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.iVal /= rhs.asInt();
					break;
				case TokenType::MOD_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Modulo by zero", s->line,
							s->col);
					cur.iVal %= rhs.asInt();
					break;
				case TokenType::AND_EQ:
					cur.iVal &= rhs.asInt();
					break;
				case TokenType::OR_EQ:
					cur.iVal |= rhs.asInt();
					break;
				case TokenType::XOR_EQ:
					cur.iVal ^= rhs.asInt();
					break;
				case TokenType::FLOOR_DIV_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.iVal =
						(long long)std::floor((double)cur.iVal / rhs.asInt());
					break;
				case TokenType::POW_EQ:
					cur.iVal = (long long)std::pow(cur.iVal, rhs.asInt());
					break;
				default:
					throw TypeError("Invalid int operation", s->line, s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::INT && rhs.type == ValueType::FLOAT) {
				switch (as->op) {
				case TokenType::PLUS_EQ:
					cur.iVal += rhs.asInt();
					break;
				case TokenType::MINUS_EQ:
					cur.iVal -= rhs.asInt();
					break;
				case TokenType::STAR_EQ:
					cur.iVal *= rhs.asInt();
					break;
				case TokenType::DIV_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.iVal /= rhs.asInt();
					break;
				case TokenType::MOD_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Modulo by zero", s->line,
							s->col);
					cur.iVal %= rhs.asInt();
					break;
				case TokenType::AND_EQ:
					cur.iVal &= rhs.asInt();
					break;
				case TokenType::OR_EQ:
					cur.iVal |= rhs.asInt();
					break;
				case TokenType::XOR_EQ:
					cur.iVal ^= rhs.asInt();
					break;
				case TokenType::FLOOR_DIV_EQ:
					if (rhs.asInt() == 0)
						throw DivisionByZeroError("Division by zero", s->line,
							s->col);
					cur.iVal =
						(long long)std::floor((double)cur.iVal / rhs.asInt());
					break;
				case TokenType::POW_EQ:
					cur.iVal = (long long)std::pow(cur.iVal, rhs.asInt());
					break;
				default:
					throw TypeError("Invalid int operation", s->line, s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::BOOL && rhs.type == ValueType::BOOL) {
				switch (as->op) {
				case TokenType::AND_EQ:
					cur.bVal = cur.bVal & rhs.asBool();
					break;
				case TokenType::OR_EQ:
					cur.bVal = cur.bVal | rhs.asBool();
					break;
				case TokenType::XOR_EQ:
					cur.bVal = cur.bVal ^ rhs.asBool();
					break;
				default:
					throw TypeError("Invalid bool operation", s->line, s->col);
				}
				return Value::None();
			}
			if (cur.type == ValueType::SET && rhs.type == ValueType::SET) {
				auto *s1 = static_cast<SetObject *>(cur.ref.get());
				auto *s2 = static_cast<SetObject *>(rhs.ref.get());
				if (as->op == TokenType::OR_EQ) {
					s1->elements.insert(s2->elements.begin(), s2->elements.end());
					return Value::None();
				}
				if (as->op == TokenType::AND_EQ) {
					std::unordered_set<Value, ValueHash, ValueEqual> keep;
					for (const auto &v1 : s1->elements) {
						if (s2->elements.count(v1) > 0) {
							keep.insert(v1);
						}
					}
					s1->elements = std::move(keep);
					return Value::None();
				}
				if (as->op == TokenType::MINUS_EQ || as->op == TokenType::DIV_EQ) {
					for (const auto &v2 : s2->elements) {
						s1->elements.erase(v2);
					}
					return Value::None();
				}
				if (as->op == TokenType::XOR_EQ) {
					for (const auto &v2 : s2->elements) {
						if (s1->elements.count(v2) > 0) {
							s1->elements.erase(v2);
						} else {
							s1->elements.insert(v2);
						}
					}
					return Value::None();
				}
			}
			if (cur.type == ValueType::VECTOR) {
				auto *v = static_cast<VectorObject *>(cur.ref.get());
				if (as->op == TokenType::PLUS_EQ) {
					if (rhs.type != ValueType::VECTOR)
						throw TypeError("+= requires a vector", s->line, s->col);
					auto *v2 = static_cast<VectorObject *>(rhs.ref.get());
					if (v->elements.size() != v2->elements.size())
						throw ValueError("Dimension mismatch", s->line, s->col);
					for (size_t i = 0; i < v->elements.size();
						i++)
						; // v->elements[i].asFloat() +=
						  // v2->elements[i].asFloat();
					return Value::None();
				}
				if (as->op == TokenType::MINUS_EQ) {
					if (rhs.type != ValueType::VECTOR)
						throw TypeError("-= requires a vector", s->line, s->col);
					auto *v2 = static_cast<VectorObject *>(rhs.ref.get());
					if (v->elements.size() != v2->elements.size())
						throw ValueError("Dimension mismatch", s->line, s->col);
					for (size_t i = 0; i < v->elements.size();
						i++)
						; // v->elements[i].asFloat() -=
						  // v2->elements[i].asFloat();
					return Value::None();
				}
				if (as->op == TokenType::STAR_EQ) {
					if (rhs.isNumber()) {
						double scalar = rhs.asFloat();
						for (auto &val : v->elements)
							; // val.asFloat() *= scalar;
					} else if (rhs.type == ValueType::VECTOR) {
						auto *v2 = static_cast<VectorObject *>(rhs.ref.get());
						if (v->elements.size() != v2->elements.size())
							throw ValueError("Dimension mismatch", s->line, s->col);
						for (size_t i = 0; i < v->elements.size();
							i++)
							; // v->elements[i].asFloat() *=
							  // v2->elements[i].asFloat();
					} else
						throw TypeError("*= requires number or vector", s->line,
							s->col);
					return Value::None();
				}
				if (as->op == TokenType::DIV_EQ)
					throw TypeError("/= not supported for vectors", s->line,
						s->col);
			}
			throw TypeError("Invalid operands for compound assignment", s->line,
				s->col);
		}
		case StmtType::LET: {
			auto let = static_cast<LetStmt *>(s);
			if (env->existsLocal(let->name)) {
				throw RuntimeError("Variable '" + let->name +
											 "' is already defined in this scope.",
					s->line, s->col);
			}
			if (env->exists(let->name)) {
				Var &existing = env->lookup(let->name);
				if (existing.isConst) {
					throw ConstError(
						"Cannot shadow constant '" + let->name + "'.", s->line,
						s->col);
				}
			}
			if (!let->value) {
				env->set(let->name, Value::NoType(), false);
				return Value::None();
			}
			if (let->value->type == ExprType::OWNERSHIP) {
				auto o = static_cast<OwnershipExpr *>(let->value);
				if (o->mode == CopyMode::REF) {
					LValue lv = resolveLValue(o->expr);
					Var varStruct;
					varStruct.alias = lv.ref;
					varStruct.isConst = let->isConst;
					env->vars[let->name] = varStruct;
					return Value::None();
				}
			}
			Value v = eval(let->value);
			if (let->isConst)
				v.isConst = true;
			bool lock = false;
			if (auto call = dynamic_cast<CallExpr *>(let->value)) {
				if (call->name == "int" || call->name == "float" ||
					 call->name == "bool" || call->name == "tuple" ||
					 call->name == "string" || call->name == "list" ||
					 call->name == "range" || call->name == "set") {
					lock = true;
				}
			}
			env->set(let->name, v, let->isLocked, let->isConst);
			return Value::None();
		}
		case StmtType::RETURN: {
			auto ret = static_cast<ReturnStmt *>(s);
			returning = true;
			returnValue =
				(ret->value != nullptr) ? eval(ret->value) : Value::NoType();
			return returnValue;
		}
		case StmtType::FUNC: {
			auto f = static_cast<FuncStmt *>(s);
			auto *func =
				new FunctionObject(f->params, f->returnType, f->defaultRetArgs,
					f->returnsConst, f->body, env, f->isCached);
			Value funcVal;
			funcVal.type = ValueType::FUNCTION;
			funcVal.ref = std::shared_ptr<HeapObject>(func);
			if (env->existsLocal(f->name)) {
				Var &existing = env->lookup(f->name);
				if (existing.value.type == ValueType::FUNCTION) {
					Value group = Value::Overload(existing.value);
					static_cast<OverloadObject *>(group.ref.get())
						->overloads.push_back(funcVal);
					existing.value = group;
				} else if (existing.value.type == ValueType::OVERLOAD) {
					static_cast<OverloadObject *>(existing.value.ref.get())
						->overloads.push_back(funcVal);
				} else {
					Var v;
					v.value = funcVal;
					v.isConst = f->returnsConst;
					env->vars[f->name] = v;
				}
			} else {
				Var v;
				v.value = funcVal;
				v.isConst = f->returnsConst;
				env->vars[f->name] = v;
			}
			return Value::None();
		}
		case StmtType::IF: {
			auto ifs = static_cast<IfStmt *>(s);
			if (eval(ifs->condition).isTruthy()) {
				for (auto st : ifs->body) {
					exec(st);
					if (returning || breaking || continuing)
						break;
				}
				return returnValue;
			}
			for (auto &[cond, block] : ifs->elifs) {
				if (eval(cond).isTruthy()) {
					for (auto st : block) {
						exec(st);
						if (returning || breaking || continuing)
							break;
					}
					return returnValue;
				}
			}
			for (auto st : ifs->elseBody) {
				exec(st);
				if (returning || breaking || continuing)
					break;
			}
			return returnValue;
		}
		case StmtType::EXPR: {
			auto es = static_cast<ExprStmt *>(s);
			eval(es->expr);
			return Value::None();
		}
		case StmtType::BREAK: {
			breaking = true;
			return Value::None();
		}
		case StmtType::CONTINUE: {
			continuing = true;
			return Value::None();
		}
		case StmtType::SKIP: {
			auto sk = static_cast<SkipStmt *>(s);
			skipCount = eval(sk->count).asInt();
			return Value::None();
		}
		case StmtType::WHILE: {
			auto w = static_cast<WhileStmt *>(s);
			std::shared_ptr<Env> loopEnv = std::make_shared<Env>();
			loopEnv->parent = env;
			std::shared_ptr<Env> previousGlobal = env;
			if (canOptimizeScope(w->body)) {
				env = loopEnv;
				while (eval(w->condition).isTruthy()) {
					for (auto st : w->body) {
						exec(st);
						if (breaking || continuing || returning)
							break;
					}
					if (breaking) {
						breaking = false;
						break;
					}
					if (continuing) {
						continuing = false;
					}
					if (returning)
						break;
				}
				env = previousGlobal;
			} else {
				while (eval(w->condition).isTruthy()) {
					env = loopEnv;
					for (auto st : w->body) {
						exec(st);
						if (breaking || continuing || returning)
							break;
					}
					env = previousGlobal;
					if (breaking) {
						breaking = false;
						break;
					}
					if (continuing) {
						continuing = false;
					}
					if (returning)
						break;
					loopEnv = std::make_shared<Env>();
					loopEnv->parent = previousGlobal;
				}
			}
			return Value::None();
		}
		case StmtType::DO_WHILE: {
			auto d = static_cast<DoWhileStmt *>(s);
			do {
				std::shared_ptr<Env> prevEnv = env;
				env = std::make_shared<Env>();
				env->parent = prevEnv;
				for (auto st : d->body) {
					exec(st);
					if (breaking || continuing || returning)
						break;
				}
				env = prevEnv;
				if (breaking) {
					breaking = false;
					return Value::None();
				}
				if (continuing) {
					continuing = false;
				}
				if (returning)
					return returnValue;
			} while (eval(d->condition).isTruthy());
			return Value::None();
		}
		case StmtType::FOR: {
			auto f = static_cast<ForStmt *>(s);
			std::shared_ptr<Env> loopEnv = std::make_shared<Env>();
			loopEnv->parent = env;
			std::shared_ptr<Env> previousGlobal = env;
			env = loopEnv;
			for (auto *st : f->inits)
				exec(st);
			bool staticScope = true;
			for (auto *st : f->body) {
				if (st->type == StmtType::LET ||
					 st->type == StmtType::MULTI_LET ||
					 st->type == StmtType::FUNC || st->type == StmtType::IMPORT) {
					staticScope = false;
					break;
				}
			}
			Value *fastCounter = nullptr;
			if (f->steps.size() == 1 && f->steps[0]->type == StmtType::ASSIGN) {
				auto as = static_cast<AssignStmt *>(f->steps[0]);
				try {
					LValue lv = resolveLValue(as->target);
					if (lv.ref->type == ValueType::INT ||
						 lv.ref->type == ValueType::FLOAT)
						fastCounter = lv.ref;
				} catch (...) {
				}
			}
			double fastLimit = 0;
			TokenType fastOp = TokenType::LET;
			bool useFastCondition = false;
			if (fastCounter && f->condition->type == ExprType::BINARY) {
				auto be = static_cast<BinExpr *>(f->condition);
				if (be->left->type == ExprType::VAR &&
					 be->right->type == ExprType::NUMBER) {
					auto ve = static_cast<VarExpr *>(be->left);
					if (static_cast<AssignStmt *>(f->steps[0])->target->type ==
						 ExprType::VAR) {
						auto stepVar = static_cast<VarExpr *>(
							static_cast<AssignStmt *>(f->steps[0])->target);
						if (stepVar->name == ve->name) {
							Value limitVal = eval(be->right);
							fastLimit = limitVal.asFloat();
							fastOp = be->op;
							useFastCondition = true;
						}
					}
				}
			}
			Value *fastBodyTarget = nullptr;
			Value *fastBodyOperand = nullptr;
			bool fastBodyIsNumber = false;
			double fastBodyLiteral = 0;
			TokenType fastBodyOp = TokenType::LET;
			bool useFastBody = false;
			if (staticScope && f->body.size() == 1 &&
				 f->body[0]->type == StmtType::ASSIGN) {
				auto as = static_cast<AssignStmt *>(f->body[0]);
				if (as->op == TokenType::PLUS_EQ ||
					 as->op == TokenType::MINUS_EQ ||
					 as->op == TokenType::STAR_EQ ||
					 as->op == TokenType::DIV_EQ) {
					try {
						LValue lv = resolveLValue(as->target);
						if (lv.ref->type == ValueType::INT ||
							 lv.ref->type == ValueType::FLOAT) {
							fastBodyTarget = lv.ref;
							fastBodyOp = as->op;
							if (as->value->type == ExprType::VAR) {
								LValue opLv = resolveLValue(as->value);
								if (opLv.ref->type == ValueType::INT ||
									 opLv.ref->type == ValueType::FLOAT) {
									fastBodyOperand = opLv.ref;
									if (fastBodyTarget->type ==
										 fastBodyOperand->type) {
										useFastBody = true;
									}
								}
							} else if (as->value->type == ExprType::NUMBER) {
								fastBodyLiteral =
									static_cast<NumberExpr *>(as->value)->val;
								fastBodyIsNumber = true;
								useFastBody = true;
							}
						}
					} catch (...) {
					}
				}
			}
			if (staticScope) {
				while (true) {
					if (useFastCondition) {
						double current = (fastCounter->type == ValueType::INT)
												  ? (double)fastCounter->iVal
												  : fastCounter->fVal;
						bool cond = false;
						switch (fastOp) {
						case TokenType::LT:
							cond = current < fastLimit;
							break;
						case TokenType::LTE:
							cond = current <= fastLimit;
							break;
						case TokenType::GT:
							cond = current > fastLimit;
							break;
						case TokenType::GTE:
							cond = current >= fastLimit;
							break;
						default:
							cond = eval(f->condition).isTruthy();
							break;
						}
						if (!cond)
							break;
					} else {
						if (!eval(f->condition).isTruthy())
							break;
					}
					if (useFastBody) {
						double operand = 0;
						if (fastBodyIsNumber)
							operand = fastBodyLiteral;
						else
							operand = (fastBodyOperand->type == ValueType::INT)
											 ? (double)fastBodyOperand->iVal
											 : fastBodyOperand->fVal;
						if (fastBodyTarget->type == ValueType::INT) {
							long long opInt = (long long)operand;
							switch (fastBodyOp) {
							case TokenType::PLUS_EQ:
								fastBodyTarget->iVal += opInt;
								break;
							case TokenType::MINUS_EQ:
								fastBodyTarget->iVal -= opInt;
								break;
							case TokenType::STAR_EQ:
								fastBodyTarget->iVal *= opInt;
								break;
							case TokenType::DIV_EQ:
								if (opInt != 0)
									fastBodyTarget->iVal /= opInt;
								break;
							}
						} else {
							switch (fastBodyOp) {
							case TokenType::PLUS_EQ:
								fastBodyTarget->fVal += operand;
								break;
							case TokenType::MINUS_EQ:
								fastBodyTarget->fVal -= operand;
								break;
							case TokenType::STAR_EQ:
								fastBodyTarget->fVal *= operand;
								break;
							case TokenType::DIV_EQ:
								if (operand != 0)
									fastBodyTarget->fVal /= operand;
								break;
							}
						}
					} else {
						for (auto st : f->body) {
							exec(st);
							if (breaking || continuing || returning)
								break;
						}
						if (breaking) {
							breaking = false;
							break;
						}
						if (continuing) {
							continuing = false;
						}
						if (returning)
							break;
					}
					if (fastCounter) {
						if (fastCounter->type == ValueType::INT)
							fastCounter->iVal += 1;
						else
							fastCounter->fVal += 1.0;
					} else {
						for (auto *st : f->steps)
							exec(st);
					}
				}
			} else {
				env = previousGlobal;
				while (eval(f->condition).isTruthy()) {
					env = loopEnv;
					for (auto st : f->body)
						exec(st);
					env = previousGlobal;
					if (breaking) {
						breaking = false;
						break;
					}
					if (continuing) {
						continuing = false;
					}
					if (returning)
						break;
					env = loopEnv;
					for (auto *st : f->steps)
						exec(st);
					env = previousGlobal;
				}
			}
			env = previousGlobal;
			if (returning)
				return returnValue;
			return Value::None();
		}
		case StmtType::FOR_EACH: {
			auto fe = static_cast<ForEachStmt *>(s);
			vector<vector<Value>> allStreams;
			size_t minLen = (size_t)-1;
			bool dictUnpack =
				fe->collections.size() == 1 && fe->loopVars.size() == 2;
			for (size_t i = 0; i < fe->collections.size(); i++) {
				Value collection = eval(fe->collections[i]);
				vector<Value> currentStream;
				vector<Value> secondaryStream;
				if (collection.type == ValueType::DICT) {
					auto *dict = static_cast<DictObject *>(collection.ref.get());
					if (dictUnpack) {
						for (auto &pair : dict->items) {
							currentStream.push_back(pair.first);
							secondaryStream.push_back(pair.second);
						}
					} else {
						for (auto &pair : dict->items)
							currentStream.push_back(pair.first);
					}
				} else if (collection.type == ValueType::LIST) {
					auto *list = static_cast<ListObject *>(collection.ref.get());
					for (auto &v : list->elements)
						currentStream.push_back(v);
				} else if (collection.type == ValueType::SET) {
					auto *set = static_cast<SetObject *>(collection.ref.get());
					for (auto &v : set->elements)
						currentStream.push_back(v);
				} else if (collection.type == ValueType::TUPLE) {
					auto *tuple = static_cast<TupleObject *>(collection.ref.get());
					for (auto &v : tuple->elements)
						currentStream.push_back(v);
				} else if (collection.type == ValueType::STRING) {
					string str = collection.asString();
					for (char c : str)
						currentStream.push_back(Value::String(string(1, c)));
				} else if (collection.type == ValueType::RANGE) {
					auto *r = static_cast<RangeObject *>(collection.ref.get());
					double cur = r->start;
					if (!r->startInclusive)
						cur += r->step;
					while (true) {
						bool keep =
							(r->step > 0)
								? (r->endInclusive ? cur <= r->end : cur < r->end)
								: (r->endInclusive ? cur >= r->end : cur > r->end);
						if (!keep)
							break;
						currentStream.push_back(r->isFloat
															? Value::Float(cur)
															: Value::Int((long long)cur));
						cur += r->step;
					}
				} else if (collection.type == ValueType::VECTOR) {
					auto *v = static_cast<VectorObject *>(collection.ref.get());
					for (auto d : v->elements)
						currentStream.push_back(d);
				} else {
					throw TypeError("Object is not iterable", s->line, s->col);
				}
				if (currentStream.size() < minLen)
					minLen = currentStream.size();
				allStreams.push_back(currentStream);
				if (dictUnpack && !secondaryStream.empty()) {
					if (secondaryStream.size() < minLen)
						minLen = secondaryStream.size();
					allStreams.push_back(secondaryStream);
				}
			}
			if (minLen == (size_t)-1)
				minLen = 0;
			if (allStreams.size() != fe->loopVars.size()) {
				throw RuntimeError(
					"Mismatch between loop variables and collections", s->line,
					s->col);
			}
			std::shared_ptr<Env> prevEnv = env;
			for (size_t i = 0; i < minLen; i++) {
				if (skipCount > 0) {
					skipCount--;
					continue;
				}
				env = std::make_shared<Env>();
				env->parent = prevEnv;
				for (size_t v = 0; v < fe->loopVars.size(); v++) {
					if (i < allStreams[v].size()) {
						env->set(fe->loopVars[v], allStreams[v][i], false, false);
					}
				}
				for (auto stmt : fe->body) {
					exec(stmt);
					if (breaking || returning || continuing)
						break;
				}
				env = prevEnv;
				if (breaking) {
					breaking = false;
					break;
				}
				if (returning)
					break;
				if (continuing)
					continuing = false;
			}
			if (returning)
				return returnValue;
			return Value::None();
		}
		case StmtType::TRY: {
			auto t = static_cast<TryStmt *>(s);
			bool errorCaught = false;
			try {
				for (auto st : t->tryBlock) {
					exec(st);
					if (returning || breaking || continuing)
						break;
				}
			} catch (const LangError &e) {
				if (dynamic_cast<const ControlFlowError *>(&e)) {
					if (!t->finallyBlock.empty()) {
						bool r = returning;
						Value rv = returnValue;
						returning = false;
						try {
							for (auto st : t->finallyBlock)
								exec(st);
						} catch (...) {
							throw;
						}
						if (!returning && !breaking && !continuing) {
							returning = r;
							returnValue = rv;
						}
					}
					throw;
				}
				errorCaught = true;
				bool handled = false;
				for (auto &catchBlock : t->catches) {
					for (auto &typeName : catchBlock.types) {
						if (matchesError(e, typeName)) {
							try {
								for (auto st : catchBlock.body) {
									exec(st);
									if (returning || breaking || continuing)
										break;
								}
							} catch (...) {
								if (!t->finallyBlock.empty())
									for (auto st : t->finallyBlock)
										exec(st);
								throw;
							}
							handled = true;
							goto after_catches;
						}
					}
				}
			after_catches:
				if (!handled) {
					if (!t->finallyBlock.empty())
						for (auto st : t->finallyBlock)
							exec(st);
					throw;
				}
			}
			if (!errorCaught && !returning && !breaking && !continuing &&
				 !t->elseBlock.empty()) {
				for (auto st : t->elseBlock) {
					exec(st);
					if (returning || breaking || continuing)
						break;
				}
			}
			if (!t->finallyBlock.empty()) {
				bool oldRet = returning;
				Value oldVal = returnValue;
				bool oldBrk = breaking;
				bool oldCont = continuing;
				returning = false;
				breaking = false;
				continuing = false;
				try {
					for (auto st : t->finallyBlock) {
						exec(st);
						if (returning || breaking || continuing)
							break;
					}
				} catch (...) {
					throw;
				}
				if (!returning && !breaking && !continuing) {
					returning = oldRet;
					returnValue = oldVal;
					breaking = oldBrk;
					continuing = oldCont;
				}
			}
			return Value::None();
		}
		case StmtType::THROW: {
			auto ts = static_cast<ThrowStmt *>(s);
			Value msgVal = eval(ts->message);
			throw LangError(ts->errorType, valueToString(msgVal), -1000000,
				s->line, s->col);
		}
		case StmtType::ASSERT: {
			auto a = static_cast<AssertStmt *>(s);
			Value cond = eval(a->condition);
			if (!cond.isTruthy()) {
				string msg = "Assertion failed";
				if (a->message) {
					Value m = eval(a->message);
					msg += ": " + valueToString(m);
				}
				throw RuntimeError(msg, s->line, s->col);
			}
			return Value::None();
		}
		case StmtType::SWITCH: {
			auto sw = dynamic_cast<SwitchStmt *>(s);
			while (true) {
				Value targetVal = eval(sw->target);
				bool matchFound = false;
				for (auto &c : sw->cases) {
					Value caseVal = eval(c.value);
					if (targetVal.strictEquals(caseVal)) {
						matchFound = true;
						for (auto st : c.body) {
							exec(st);
							if (returning)
								return returnValue;
							if (continuing)
								goto exit_switch;
							if (breaking) {
								breaking = false;
								goto exit_switch;
							}
						}
						goto restart_loop;
					}
				}
				if (!matchFound) {
					for (auto st : sw->defaultBody) {
						exec(st);
						if (returning)
							return returnValue;
						if (continuing)
							goto exit_switch;
						if (breaking) {
							breaking = false;
							goto exit_switch;
						}
					}
					goto restart_loop;
				}
			restart_loop:;
			}
		exit_switch:
			return Value::None();
		}
		case StmtType::IMPORT: {
			auto imp = static_cast<ImportStmt *>(s);
			if (imp->libName.size() > 4 &&
				 imp->libName.substr(imp->libName.size() - 4) == ".ymm") {
				fs::path p(imp->libName);
				if (!fs::exists(p)) {
					throw InvalidImportError(
						"Module file not found: " + imp->libName, s->line,
						s->col);
				}
				std::error_code ec;
				string absPath = fs::absolute(p, ec).string();
				if (importStack.count(absPath)) {
					throw CircularImportError(
						"Circular import detected involving: " + imp->libName,
						s->line, s->col);
				}
				std::ifstream file(absPath);
				if (!file)
					throw InvalidImportError(
						"Unable to read module: " + imp->libName, s->line,
						s->col);
				std::stringstream buffer;
				buffer << file.rdbuf();
				string code = buffer.str();
				std::shared_ptr<Env> previousEnv = this->env;
				std::shared_ptr<Env> moduleEnv = std::make_shared<Env>();
				moduleEnv->set("None", Value::None(), true);
				importStack.insert(absPath);
				this->env = moduleEnv;
				try {
					auto tokens = tokenize(code);
					Parser parser(tokens);
					vector<Stmt *> moduleStmts;
					while (!parser.isAtEnd())
						moduleStmts.push_back(parser.parseStmt());
					for (Stmt *stmt : moduleStmts) {
						if (!isGlobalAllowed(stmt))
							throw RuntimeError(
								"Module code must only contain definitions (let, "
								"func, "
								"import).",
								stmt->line, stmt->col);
					}
					for (Stmt *ms : moduleStmts)
						exec(ms);
				} catch (...) {
					this->env = previousEnv;
					importStack.erase(absPath);
					throw;
				}
				this->env = previousEnv;
				importStack.erase(absPath);
				if (imp->symbols.empty()) {
					for (auto &[name, var] : moduleEnv->vars) {
						if (name == "None")
							continue;
						this->env->vars[name] = var;
					}
				} else {
					for (const string &sym : imp->symbols) {
						if (moduleEnv->vars.count(sym)) {
							this->env->vars[sym] = moduleEnv->vars[sym];
						} else {
							throw AttributeError("Module '" + imp->libName +
															"' does not export '" + sym +
															"'",
								s->line, s->col);
						}
					}
				}
				return Value::None();
			}
			if (modules.find(imp->libName) == modules.end()) {
				if (imp->libName.find('.') != string::npos) {
					throw InvalidImportError("Import filenames must end in .ymm",
						s->line, s->col);
				}
				throw ImportError("Unknown module '" + imp->libName + "'",
					s->line, s->col);
			}
			modules[imp->libName](this->env, imp->symbols);
			return Value::None();
		}
		case StmtType::MULTI_LET: {
			auto ml = static_cast<MultiLetStmt *>(s);
			vector<Value> results;
			for (auto *expr : ml->values)
				results.push_back(expr ? eval(expr) : Value::NoType());
			for (size_t i = 0; i < ml->names.size(); i++) {
				if (env->existsLocal(ml->names[i])) {
					throw RuntimeError("Variable '" + ml->names[i] +
												 "' is already defined in this scope.",
						s->line, s->col);
				}
				Value v = results[i];
				if (ml->isConsts[i])
					v.isConst = true;
				bool lock = false;
				env->set(ml->names[i], v, ml->isLocked, ml->isConsts[i]);
			}
			return Value::None();
		}
		case StmtType::MULTI_ASSIGN: {
			auto ma = static_cast<MultiAssignStmt *>(s);
			vector<LValue> lvals;
			for (auto *t : ma->targets)
				lvals.push_back(resolveLValue(t));
			vector<Value> rvals;
			for (auto *v : ma->values)
				rvals.push_back(eval(v));
			for (size_t i = 0; i < lvals.size(); i++) {
				Value &cur = *lvals[i].ref;
				if (cur.isConst || lvals[i].isConstView)
					throw ConstError("Cannot assign to const variable", s->line,
						s->col);
				if (cur.ref && cur.ref->typeLocked &&
					 cur.type != rvals[i].type) {
					throw TypeError("Cannot change type of locked variable",
						s->line, s->col);
				}
				if (lvals[i].isRefTarget)
					*lvals[i].ref = rvals[i];
				else if (cur.ref && cur.type == rvals[i].type) {
					if (cur.type == ValueType::STRING)
						static_cast<StringObject *>(cur.ref.get())->value =
							rvals[i].asString();
					else
						cur = rvals[i];
				} else
					cur = rvals[i];
			}
			return Value::None();
		}
		default: {
			return Value::None();
		}
		}
		return Value::None();
	}
	Value call(Value callee, const vector<CallArg> &args,
		const vector<CopyMode> &modes, int line = 0, int col = 0) {
		auto error = [&](const string &msg, const string &type = "RuntimeError") {
			if (type == "TypeError")
				throw TypeError(msg, line, col);
			if (type == "ValueError")
				throw ValueError(msg, line, col);
			if (type == "ArgumentError")
				throw ArgumentError(msg, line, col);
			if (type == "NameError")
				throw NameError(msg, line, col);
			throw RuntimeError(msg, line, col);
		};
		// ------------------ RESOLVE CALLEE OBJECT ------------------
		FunctionObject *lambdaObj = nullptr;
		NativeFunctionObject *nativeObj = nullptr;
		if (callee.type == ValueType::FUNCTION) {
			lambdaObj = static_cast<FunctionObject *>(callee.ref.get());
		} else if (callee.type == ValueType::NATIVE_FUNCTION) {
			nativeObj = static_cast<NativeFunctionObject *>(callee.ref.get());
		} else if (callee.type == ValueType::OVERLOAD) {
			auto *ov = static_cast<OverloadObject *>(callee.ref.get());
			bool found = false;
			for (int i = ov->overloads.size() - 1; i >= 0; i--) {
				Value candVal = ov->overloads[i];
				if (candVal.type != ValueType::FUNCTION)
					continue;
				auto *candidate = static_cast<FunctionObject *>(candVal.ref.get());
				size_t minArgs = 0;
				bool isVariadic = false;
				for (const auto &p : candidate->params) {
					if (p.isVariadic || p.isKwargs)
						isVariadic = true;
					else if (p.defaultValue == nullptr)
						minArgs++;
				}
				if (args.size() < minArgs && !isVariadic)
					continue;
				if (!isVariadic && args.size() > candidate->params.size())
					continue;
				bool typesMatch = true;
				size_t paramIdx = 0;
				for (size_t argIdx = 0; argIdx < args.size(); argIdx++) {
					if (paramIdx >= candidate->params.size()) {
						if (!isVariadic) {
							typesMatch = false;
							break;
						}
						break;
					}
					const ParamSpec &p = candidate->params[paramIdx];
					if (p.isVariadic || p.isKwargs)
						break;
					Value argVal = args[argIdx].value;
					if (p.type != ValueType::NOTYPE && argVal.type != p.type) {
						if (!(p.type == ValueType::FLOAT &&
								 argVal.type == ValueType::INT)) {
							typesMatch = false;
							break;
						}
					}
					paramIdx++;
				}
				if (typesMatch) {
					lambdaObj = candidate;
					found = true;
					break;
				}
			}
			if (!found)
				error("No matching overload found with provided arguments",
					"TypeError");
		} else {
			error("Object is not callable", "TypeError");
		}
		if (nativeObj) {
			vector<Value> argValues;
			for (const auto &ca : args)
				argValues.push_back(ca.value);
			return nativeObj->func(argValues, line, col);
		}
		if (!lambdaObj)
			error("Critical: Lambda Object is null", "RuntimeError");
		vector<Value> cacheKey;
		if (lambdaObj->isCached) {
			for (const auto &arg : args)
				cacheKey.push_back(arg.value);
			if (lambdaObj->cache.count(cacheKey))
				return lambdaObj->cache[cacheKey];
		}
		std::shared_ptr<Env> previous = env;
		std::shared_ptr<Env> newEnv = std::make_shared<Env>();
		newEnv->parent = lambdaObj->closure ? lambdaObj->closure : previous;
		env = newEnv;
		size_t argIndex = 0;
		for (size_t i = 0; i < lambdaObj->params.size(); i++) {
			const ParamSpec &p = lambdaObj->params[i];
			Value argVal = Value::None();
			if (p.isKwargs) {
				std::unordered_map<Value, Value, ValueHash, ValueEqual> dictItems;
				while (argIndex < args.size()) {
					Value v = args[argIndex].value;
					if (v.type != ValueType::PAIRED)
						throw RuntimeError(
							"Positional argument found after keyword arguments.",
							line, col);
					auto *pairObj = static_cast<PairedObject *>(v.ref.get());
					for (const auto &entry : pairObj->pairs) {
						Value key = entry.first;
						Value val = entry.second;
						if (p.mode == CopyMode::DEEP) {
							if (key.type == ValueType::LIST ||
								 key.type == ValueType::DICT)
								key = deepCopy(key);
							val = deepCopy(val);
						}
						dictItems[key] = val;
					}
					argIndex++;
				}
				Value dictVal = Value::Dict(dictItems, p.isConst);
				env->set(p.name, dictVal, false, p.isConst);
			} else if (p.isVariadic) {
				vector<Value> tupleItems;
				while (argIndex < args.size()) {
					if (args[argIndex].value.type == ValueType::PAIRED)
						break;
					Value v = args[argIndex].value;
					if (p.mode == CopyMode::DEEP)
						v = deepCopy(v);
					tupleItems.push_back(v);
					argIndex++;
				}
				env->set(p.name, Value::Tuple(tupleItems), false, true);
			} else {
				bool foundValue = false;
				if (argIndex < args.size() &&
					 args[argIndex].value.type != ValueType::PAIRED) {
					if (args[argIndex].value.type == ValueType::OMIT_MARKER) {
						if (p.defaultChunk != nullptr) {
							argVal = eval(p.defaultValue);
							foundValue = true;
						} else {
							env = previous;
							error("Argument '" + p.name +
										"' cannot be omitted (no default value).",
								"ArgumentError");
						}
					} else {
						argVal = args[argIndex].value;
						if (p.mode == CopyMode::DEEP)
							argVal = deepCopy(argVal);
						foundValue = true;
					}
					argIndex++;
				}
				if (!foundValue) {
					if (p.defaultChunk != nullptr)
						argVal = eval(p.defaultValue);
					else {
						env = previous;
						error("Missing required argument '" + p.name + "'",
							"ArgumentError");
					}
				}
				if (p.type != ValueType::NOTYPE && argVal.type != p.type) {
					if (p.type == ValueType::FLOAT && argVal.type == ValueType::INT)
						argVal = Value::Float((double)argVal.asInt());
					else {
						env = previous;
						error("Type mismatch for '" + p.name + "'", "TypeError");
					}
				}
				env->set(p.name, argVal, false, p.isConst);
			}
		}
		bool prevReturning = returning;
		Value prevReturnValue = returnValue;
		returning = false;
		for (auto stmt : lambdaObj->body) {
			exec(stmt);
			if (returning)
				break;
		}
		bool didReturn = returning;
		Value ret = returnValue;
		returning = prevReturning;
		returnValue = prevReturnValue;
		env = previous;
		auto generateDefault = [&]() -> Value {
			vector<CallArg> defArgs;
			vector<CopyMode> defModes;
			for (auto *e : lambdaObj->defaultRetArgs) {
				defArgs.push_back({eval(e)});
				defModes.push_back(CopyMode::SHALLOW);
			}
			string typeName = "";
			switch (lambdaObj->returnType) {
			case ValueType::INT:
				typeName = "int";
				break;
			case ValueType::FLOAT:
				typeName = "float";
				break;
			case ValueType::STRING:
				typeName = "string";
				break;
			case ValueType::BOOL:
				typeName = "bool";
				break;
			case ValueType::LIST:
				typeName = "list";
				break;
			case ValueType::DICT:
				typeName = "dict";
				break;
			case ValueType::SET:
				typeName = "set";
				break;
			case ValueType::TUPLE:
				typeName = "tuple";
				break;
			case ValueType::RANGE:
				typeName = "range";
				break;
			case ValueType::VECTOR:
				typeName = "vector";
				break;
			default:
				break;
			}
			if (!typeName.empty()) {
				if (env->exists(typeName)) {
					return call(env->get(typeName), defArgs, defModes, line, col);
				}
			}
			return Value::None();
		};
		Value finalResult;
		if (didReturn) {
			if (ret.type == ValueType::NOTYPE) {
				if (!lambdaObj->defaultRetArgs.empty())
					finalResult = generateDefault();
				else if (lambdaObj->returnType != ValueType::NOTYPE)
					finalResult = defaultOf(lambdaObj->returnType);
				else
					finalResult = Value::NoType();
			} else if (lambdaObj->returnType != ValueType::NOTYPE &&
						  ret.type != lambdaObj->returnType) {
				if (lambdaObj->returnType == ValueType::FLOAT &&
					 ret.type == ValueType::INT)
					finalResult = Value::Float((double)ret.asInt());
				else if (lambdaObj->returnType == ValueType::INT &&
							ret.type == ValueType::FLOAT)
					finalResult = Value::Int((long long)ret.fVal);
				else
					error("Return type mismatch", "TypeError");
			} else
				finalResult = ret;
		} else {
			if (!lambdaObj->defaultRetArgs.empty())
				finalResult = generateDefault();
			else
				finalResult = (lambdaObj->returnType != ValueType::NOTYPE)
									  ? defaultOf(lambdaObj->returnType)
									  : Value::NoType();
		}
		if (lambdaObj->returnsConst)
			finalResult.isConst = true;
		if (lambdaObj->isCached)
			lambdaObj->cache[cacheKey] = finalResult;
		return finalResult;
	}
	void interpret(const vector<Stmt *> &statements) {
		try {
			for (Stmt *stmt : statements) {
				if (!isGlobalAllowed(stmt))
					throw RuntimeError(
						"Executable code must be inside a 'main' function.",
						stmt->line, stmt->col);
			}

			for (Stmt *stmt : statements)
				exec(stmt);
			if (env->exists("main")) {
				Value mainVal = env->get("main");
				if (mainVal.type != ValueType::FUNCTION)
					throw RuntimeError("'main' is defined but is not a function.", 0,
						0);
				auto *func = static_cast<FunctionObject *>(mainVal.ref.get());
				if (func->params.size() > 0) {
					throw RuntimeError("'main' function cannot accept arguments.", 0,
						0);
				}
				std::shared_ptr<Env> globalEnv = this->env;
				this->env = std::make_shared<Env>();
				this->env->parent = func->closure;
				try {
					// Execute Main Body
					for (Stmt *bodyStmt : func->body) {
						exec(bodyStmt);
						if (returning) {
							returning = false;
							break;
						}
					}
				} catch (...) {
					this->env = globalEnv;
					throw;
				}
				this->env = globalEnv;
			} else
				throw RuntimeError(
					"No entry point found. Define a 'main' function.", 0, 0);
		} catch (...) {
			throw;
		}
	}

 private:
	bool canOptimizeScope(const vector<Stmt *> &body) {
		for (auto *st : body) {
			if (st->type == StmtType::LET || st->type == StmtType::MULTI_LET ||
				 st->type == StmtType::FUNC || st->type == StmtType::IMPORT ||
				 st->type == StmtType::MULTI_ASSIGN) {
				return false;
			}
		}
		return true;
	}
	bool isGlobalAllowed(Stmt *s) {
		if (dynamic_cast<LetStmt *>(s))
			return true;
		if (dynamic_cast<FuncStmt *>(s))
			return true;
		if (dynamic_cast<ImportStmt *>(s))
			return true;
		if (dynamic_cast<MultiLetStmt *>(s))
			return true;
		if (dynamic_cast<MultiAssignStmt *>(s))
			return true;
		return false;
	}
};
// ---------------- JIT ----------------

// ------------ BYTECODE VM ------------
struct CTLEAbort : public std::exception {};

#endif // YLANG_OBJECTS_HPP
