#include "ylang/objects.hpp"

void enableColors() {
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	if (GetConsoleMode(hOut, &dwMode)) {
		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(hOut, dwMode);
	}
#endif // _WIN32
}


Value shallowCopy(const Value &v) {
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


Value deepCopy(const Value &v) {
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


Value applyCopy(const Value &v, CopyMode mode) {
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


void printValue(const Value &v, std::unordered_set<const HeapObject *> &seen, bool quoteStrings, Inspector *inspect) {
	bool isContainer =
		(v.type == ValueType::LIST || v.type == ValueType::SET ||
			v.type == ValueType::VECTOR || v.type == ValueType::DICT);
	if (isContainer && v.ref && seen.count(v.ref.get())) {
		std::cout << "<self>";
		return;
	}
	if (v.type == ValueType::INSTANCE && inspect) {
		std::string custom = (*inspect)(v);
		if (!custom.empty()) {
			std::cout << custom;
			return;
		}
	}
	if (isContainer && v.ref)
		seen.insert(v.ref.get());
	switch (v.type) {
	case ValueType::INT:
		std::cout << v.asInt();
		break;
	case ValueType::FLOAT:
		std::cout << v.asFloat();
		break;
	case ValueType::BOOL:
		std::cout << (v.asBool() ? "true" : "false");
		break;
	case ValueType::NONE:
		std::cout << "None";
		break;
	case ValueType::NOTYPE:
		std::cout << "NoType";
		break;
	case ValueType::CLASS:
		std::cout << "class: ";
#ifdef VM_DEBUG_MODE
		std::cout << v.__DEBUGGING__NAME__;
#endif
		break;
	case ValueType::STRING:
		if (quoteStrings)
			std::cout << "\"" << v.asString() << "\"";
		else
			std::cout << v.asString();
		break;
	case ValueType::LIST: {
		auto *list = static_cast<ListObject *>(v.ref.get());
		std::cout << "[";
		for (size_t i = 0; i < list->elements.size(); i++) {
			printValue(list->elements[i], seen, true, inspect);
			if (i + 1 < list->elements.size())
				std::cout << ", ";
		}
		std::cout << "]";
		break;
	}
	case ValueType::RANGE: {
		auto *r = static_cast<RangeObject *>(v.ref.get());
		if (!r->isValid) {
			std::cout << "(...)";
			break;
		}
		std::cout << (r->startInclusive ? "[" : "(")
					 << (r->isFloat ? std::to_string(r->start)
										 : std::to_string((long long)r->start))
					 << ".."
					 << (r->isFloat ? std::to_string(r->end)
										 : std::to_string((long long)r->end));
		if (r->step != 1)
			std::cout << ".."
						 << (r->isFloat ? std::to_string(r->step)
											 : std::to_string((long long)r->step));
		std::cout << (r->endInclusive ? "]" : ")");
		break;
	}
	case ValueType::SET: {
		auto *s = static_cast<SetObject *>(v.ref.get());
		if (s->elements.empty()) {
			std::cout << "{,}";
			break;
		}
		std::cout << "{";
		size_t i = 0;
		for (const auto &key : s->elements) {
			printValue(key, seen, true, inspect);
			if (i + 1 < s->elements.size())
				std::cout << ", ";
			i++;
		}
		std::cout << "}";
		break;
	}
	case ValueType::TUPLE: {
		auto *t = static_cast<TupleObject *>(v.ref.get());
		if (t->elements.empty()) {
			std::cout << "(,)";
			break;
		}
		std::cout << "(";
		for (size_t i = 0; i < t->elements.size(); i++) {
			printValue(t->elements[i], seen, true, inspect);
			if (i + 1 < t->elements.size())
				std::cout << ", ";
			else if (t->elements.size() == 1)
				std::cout << ",";
		}
		std::cout << ")";
		break;
	}
	case ValueType::DICT: {
		auto *dict = static_cast<DictObject *>(v.ref.get());
		if (dict->items.empty()) {
			std::cout << "{:}";
			break;
		}
		std::cout << "{";
		size_t i = 0;
		for (const auto &[key, val] : dict->items) {
			printValue(key, seen, true, inspect);
			std::cout << " : ";
			printValue(val, seen, true, inspect);
			if (i + 1 < dict->items.size())
				std::cout << ", ";
			i++;
		}
		std::cout << "}";
		break;
	}
	case ValueType::PAIRED: {
		auto *pair = static_cast<PairedObject *>(v.ref.get());
		std::cout << "<Paired Object at " << &pair << ">";
		break;
	}
	case ValueType::FILE: {
		auto *f = static_cast<FileObject *>(v.ref.get());
		std::cout << "<File at path: '" << f->path << "' >";
		break;
	}
	case ValueType::VECTOR: {
		auto *vec = static_cast<VectorObject *>(v.ref.get());
		std::cout << valueToString(v);
		break;
	}
	case ValueType::BIGINT: {
		auto *big = static_cast<BigIntObject *>(v.ref.get());
		std::cout << bigIntToString(big);
		break;
	}
	case ValueType::REFERENCE: {
		std::cout << valueToString(*(v.get_ptr_safe()));
		break;
	}

	default:
		std::cout << "<"
					 << (v.type == ValueType::INSTANCE
								 ? static_cast<InstanceObject *>(v.ref.get())->name
								 : "")
					 << "Object at " << v.ref << ">";
		break;
	}
}


void printValue(const Value &v) {
	std::unordered_set<const HeapObject *> seen;
	printValue(v, seen, false);
}


BigIntObject::BigIntObject(long long n) : HeapObject(ValueType::BIGINT) {
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

bool BigIntObject::operator<(const BigIntObject &other) const {
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

BigIntObject BigIntObject::absAdd(const BigIntObject &other) const {
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

BigIntObject BigIntObject::absSub(const BigIntObject &other) const {
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

BigIntObject BigIntObject::operator+(const BigIntObject &other) const {
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

BigIntObject BigIntObject::operator-(const BigIntObject &other) const {
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

std::vector<uint32_t> BigIntObject::karatsubaMul(const std::vector<uint32_t> &A, const std::vector<uint32_t> &B) {
		if (A.size() < 32 || B.size() < 32) {
			size_t n = A.size(), m = B.size();
			if (n == 0 || m == 0)
				return {0};
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
			while (res.size() > 1 && res.back() == 0)
				res.pop_back();
			return res;
		}

		size_t m = std::max(A.size(), B.size());
		size_t k = m / 2;

		std::vector<uint32_t> A_0(A.begin(), A.begin() + std::min(A.size(), k));
		std::vector<uint32_t> A_1(A.begin() + std::min(A.size(), k), A.end());
		if (A_1.empty())
			A_1.push_back(0);
		if (A_0.empty())
			A_0.push_back(0);

		std::vector<uint32_t> B_0(B.begin(), B.begin() + std::min(B.size(), k));
		std::vector<uint32_t> B_1(B.begin() + std::min(B.size(), k), B.end());
		if (B_1.empty())
			B_1.push_back(0);
		if (B_0.empty())
			B_0.push_back(0);

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

std::pair<BigIntObject, BigIntObject> BigIntObject::divMod(const BigIntObject &other) const {
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

void BigIntObject::lshift(int shift) {
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

BigIntObject BigIntObject::operator&(const BigIntObject &other) const {
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

BigIntObject BigIntObject::operator|(const BigIntObject &other) const {
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

BigIntObject BigIntObject::operator^(const BigIntObject &other) const {
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

BigIntObject BigIntObject::operator<<(long long shift) const {
		if (shift < 0)
			return operator>>(-shift);
		if (shift == 0 || chunks.empty())
			return *this;
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

BigIntObject BigIntObject::operator>>(long long shift) const {
		if (shift < 0)
			return operator<<(-shift);
		if (shift == 0 || chunks.empty())
			return *this;
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

Value BigIntObject::pow(Value a, Value b) {
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

void ClassObject::computeMRO() {
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

Value Value::BigInt(const std::string &str) {
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
		if (str[i] < '0' || str[i] > '9')
			continue;
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

Value Value::Range(double s, double e, double st, bool si, bool ei, bool f, bool locked, bool isConst) {
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

Value Value::Instance(Value classDef) {
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

bool Value::isTruthy() const {
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

bool Value::strictEquals(const Value &other) const {
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
		if (inst1 == inst2)
			return true;
		if (inst1->klass->name != inst2->klass->name)
			return false;
		if (inst1->fields.size() != inst2->fields.size())
			return false;
		for (const auto &[k, v] : inst1->fields) {
			if (inst2->fields.find(k) == inst2->fields.end() || !v.strictEquals(inst2->fields.at(k)))
				return false;
		}
		return true;
	}
	}
	return false;
}

std::size_t ValueHash::operator()(const Value &v) const {
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

Value defaultOf(ValueType t) {
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

void Env::set(const string &n, Value v, bool locked, bool isConstVar) {
#ifdef VM_DEBUG_MODE
		v.__DEBUGGING__NAME__ = n;
#endif
		auto it = vars.find(n);
		if (it != vars.end()) {
			Var &existing = it->second;
			if (v.type == ValueType::REFERENCE && v.get_ptr_safe()) {
				existing.alias = v.get_ptr_safe();
				existing.value = v;
				existing.isLocked = locked;
				existing.isConst = isConstVar;
				return;
			}
			if (existing.alias) {
				if (existing.isConst) {
					throw RuntimeError("Cannot reassign a constant variable '" + n + "'", 0, 0);
				}
				if (existing.isLocked && existing.alias->type != v.type) {
					throw RuntimeError("Type mismatch: variable '" + n + "' is type-locked.", 0, 0);
				}
				bool wasConst = existing.alias->isConst;
				bool wasLocked = existing.alias->isLocked;
				*existing.alias = v;
				existing.alias->isConst = wasConst;
				existing.alias->isLocked = wasLocked;
				return;
			}
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
		Value *target = (v.type == ValueType::REFERENCE && v.get_ptr_safe()) ? v.get_ptr_safe() : nullptr;
		vars[n] = Var{v, target, isConstVar, locked};
	}

Value Env::get(const string &n) {
		Var &var = lookup(n);
		Value val;
		if (var.alias) {
			val = *var.alias;
		} else if (var.value.type == ValueType::REFERENCE && var.value.get_ptr_safe()) {
			val = *var.value.get_ptr_safe();
		} else {
			val = var.value;
		}
		val.isConst = var.isConst;
		val.isLocked = var.isLocked;
		return val;
	}

void Env::assign(const string &n, Value v) {
		Var &var = lookup(n);

		if (var.isConst) {
			throw RuntimeError("Cannot reassign a constant variable '" + n + "'", 0, 0);
		}

		Value *target = var.alias ? var.alias : ((var.value.type == ValueType::REFERENCE) ? var.value.get_ptr_safe() : nullptr);
		if (target) {
			if (target->isConst) {
				throw RuntimeError("Cannot assign to const variable via reference '" + n + "'", 0, 0);
			}
			if (var.isLocked && target->type != v.type) {
				throw RuntimeError("Type mismatch: variable '" + n + "' is type-locked.", 0, 0);
			}
			bool wasConst = target->isConst;
			bool wasLocked = target->isLocked;
			*target = v;
			target->isConst = wasConst;
			target->isLocked = wasLocked;
			return;
		}

		if (var.isLocked && var.value.type != v.type) {
			throw RuntimeError("Type mismatch: variable '" + n + "' is type-locked.", 0, 0);
		}

		bool wasConst = var.value.isConst;
		bool wasLocked = var.value.isLocked;
		var.value = v;
		var.value.isConst = wasConst;
		var.value.isLocked = wasLocked;
	}

std::string bigIntToString(BigIntObject *big) {
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
	if (final_res.empty())
		return "0";
	return final_res;
}

std::string valueToString(const Value &v, int line, int col) {
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

std::string PrintStackForDebug(const std::vector<Value> &stack) {
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

std::string magic_methods_to_string(Magic_Methods Magic_method) {
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

std::string OpCodeToString(OpCode num) {
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
	case OpCode::OP_FAST_IADD:
		return "OP_FAST_IADD";
	case OpCode::OP_FAST_ISUB:
		return "OP_FAST_ISUB";
	case OpCode::OP_FAST_IMUL:
		return "OP_FAST_IMUL";
	case OpCode::OP_FAST_IDIV:
		return "OP_FAST_IDIV";
	case OpCode::OP_FAST_FADD:
		return "OP_FAST_FADD";
	case OpCode::OP_FAST_FSUB:
		return "OP_FAST_FSUB";
	case OpCode::OP_FAST_FMUL:
		return "OP_FAST_FMUL";
	case OpCode::OP_FAST_FDIV:
		return "OP_FAST_FDIV";
	case OpCode::OP_FAST_SADD:
		return "OP_FAST_SADD";
	case OpCode::OP_CTLE_TICK:
		return "OP_CTLE_TICK";
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
