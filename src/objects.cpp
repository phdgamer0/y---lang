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

