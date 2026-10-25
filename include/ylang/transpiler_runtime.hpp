#pragma once

#include "ylang/objects.hpp"
#include "ylang/vm.hpp"
#include "ylang/ast.hpp"
#include <cmath>
#include <vector>
#include <unordered_map>
#include <string>
#include <type_traits>
#include <utility>
#include <stdexcept>
#include <memory>

// --- Value Constructor / Identity Helpers ---
inline Value __y_val(const Value& v) { return v; }
inline Value __y_val(Value&& v) { return std::move(v); }
inline Value __y_val(long long v) { return Value::Int(v); }
inline Value __y_val(unsigned long long v) { return Value::Int((long long)v); }
inline Value __y_val(int v) { return Value::Int((long long)v); }
inline Value __y_val(double v) { return Value::Float(v); }
inline Value __y_val(bool v) { return Value::Bool(v); }
inline Value __y_val(const std::string& v) { return Value::String(v); }
inline Value __y_val(const char* v) { return Value::String(std::string(v)); }

// --- Generic Template Cast Utility ---
template <typename Target, typename Source>
[[nodiscard]] inline constexpr auto __y_as(Source&& val) {
    using CleanTarget = std::decay_t<Target>;
    using CleanSource = std::decay_t<Source>;

    if constexpr (std::is_same_v<CleanTarget, CleanSource>) {
        return std::forward<Source>(val);
    } else if constexpr (std::is_same_v<CleanSource, Value>) {
        if constexpr (std::is_same_v<CleanTarget, long long>) return val.asInt();
        else if constexpr (std::is_same_v<CleanTarget, double>) return val.asFloat();
        else if constexpr (std::is_same_v<CleanTarget, bool>) return val.isTruthy();
        else if constexpr (std::is_same_v<CleanTarget, std::string>) return val.asString();
        else if constexpr (std::is_same_v<CleanTarget, Value>) return val;
    } else if constexpr (std::is_arithmetic_v<CleanSource> && std::is_arithmetic_v<CleanTarget>) {
        return static_cast<CleanTarget>(val);
    } else {
        return CleanTarget(std::forward<Source>(val));
    }
}

// --- Dynamic Conversion Helpers ---
inline long long __y_to_int(long long v) { return v; }
inline long long __y_to_int(double v) { return (long long)v; }
inline long long __y_to_int(bool v) { return v ? 1LL : 0LL; }
inline long long __y_to_int(const Value& v) { return v.asInt(); }

inline double __y_to_float(double v) { return v; }
inline double __y_to_float(long long v) { return (double)v; }
inline double __y_to_float(const Value& v) { return v.asFloat(); }

inline bool __y_to_bool(bool v) { return v; }
inline bool __y_to_bool(long long v) { return v != 0; }
inline bool __y_to_bool(const Value& v) { return v.isTruthy(); }

inline std::string __y_to_str(const std::string& v) { return v; }
inline std::string __y_to_str(const char* v) { return std::string(v); }
inline std::string __y_to_str(const Value& v) { return v.asString(); }

// --- Environment Reference Lookups ---
inline Value __y_make_ref(const std::shared_ptr<Env>& env, const std::string& name) {
    auto& var = env->lookup(name);
    if (var.alias) return Value::Reference(var.alias, nullptr);
    if (var.value.type == ValueType::REFERENCE && var.value.get_ptr_safe()) return var.value;
    return Value::FromExisting(std::make_shared<VarRefObject>(env, name), ValueType::REFERENCE);
}

inline long long& __y_env_int_ref(const std::shared_ptr<Env>& env, const std::string& name) {
    auto& var = env->lookup(name);
    if (var.alias) return var.alias->iVal;
    if (var.value.type == ValueType::REFERENCE && var.value.get_ptr_safe()) return var.value.get_ptr_safe()->iVal;
    return var.value.iVal;
}
inline double& __y_env_float_ref(const std::shared_ptr<Env>& env, const std::string& name) {
    auto& var = env->lookup(name);
    if (var.alias) return var.alias->fVal;
    if (var.value.type == ValueType::REFERENCE && var.value.get_ptr_safe()) return var.value.get_ptr_safe()->fVal;
    return var.value.fVal;
}
inline bool& __y_env_bool_ref(const std::shared_ptr<Env>& env, const std::string& name) {
    auto& var = env->lookup(name);
    if (var.alias) return var.alias->bVal;
    if (var.value.type == ValueType::REFERENCE && var.value.get_ptr_safe()) return var.value.get_ptr_safe()->bVal;
    return var.value.bVal;
}
inline Value& __y_env_val_ref(const std::shared_ptr<Env>& env, const std::string& name) {
    auto& var = env->lookup(name);
    if (var.alias) return *var.alias;
    if (var.value.type == ValueType::REFERENCE && var.value.get_ptr_safe()) return *var.value.get_ptr_safe();
    return var.value;
}
inline Value* __y_env_ptr(const std::shared_ptr<Env>& env, const std::string& name) {
    auto& var = env->lookup(name);
    if (var.alias) return var.alias;
    if (var.value.type == ValueType::REFERENCE && var.value.get_ptr_safe()) return var.value.get_ptr_safe();
    return &var.value;
}

// --- Truthiness ---
inline bool __y_is_truthy(bool b) { return b; }
inline bool __y_is_truthy(long long n) { return n != 0; }
inline bool __y_is_truthy(double d) { return d != 0.0; }
inline bool __y_is_truthy(const std::string& s) { return !s.empty(); }
inline bool __y_is_truthy(const Value& v) { return v.isTruthy(); }

// --- Comparisons ---
inline bool __y_lt(long long a, long long b) { return a < b; }
inline bool __y_lt(double a, double b) { return a < b; }
inline bool __y_lt(const Value& a, const Value& b) {
    if (a.type == ValueType::INT && b.type == ValueType::INT) return a.iVal < b.iVal;
    if (a.type == ValueType::FLOAT && b.type == ValueType::FLOAT) return a.fVal < b.fVal;
    if (a.isNumber() && b.isNumber()) return a.asFloat() < b.asFloat();
    return EvaluateConstBinary(TokenType::LT, a, b).isTruthy();
}
inline bool __y_lt(const Value& a, long long b) {
    if (a.type == ValueType::INT) return a.iVal < b;
    if (a.isNumber()) return a.asFloat() < (double)b;
    return EvaluateConstBinary(TokenType::LT, a, Value::Int(b)).isTruthy();
}
inline bool __y_lt(long long a, const Value& b) {
    if (b.type == ValueType::INT) return a < b.iVal;
    if (b.isNumber()) return (double)a < b.asFloat();
    return EvaluateConstBinary(TokenType::LT, Value::Int(a), b).isTruthy();
}

inline bool __y_lte(long long a, long long b) { return a <= b; }
inline bool __y_lte(double a, double b) { return a <= b; }
inline bool __y_lte(const Value& a, const Value& b) {
    if (a.type == ValueType::INT && b.type == ValueType::INT) return a.iVal <= b.iVal;
    if (a.type == ValueType::FLOAT && b.type == ValueType::FLOAT) return a.fVal <= b.fVal;
    if (a.isNumber() && b.isNumber()) return a.asFloat() <= b.asFloat();
    return EvaluateConstBinary(TokenType::LTE, a, b).isTruthy();
}
inline bool __y_lte(const Value& a, long long b) {
    if (a.type == ValueType::INT) return a.iVal <= b;
    if (a.isNumber()) return a.asFloat() <= (double)b;
    return EvaluateConstBinary(TokenType::LTE, a, Value::Int(b)).isTruthy();
}
inline bool __y_lte(long long a, const Value& b) {
    if (b.type == ValueType::INT) return a <= b.iVal;
    if (b.isNumber()) return (double)a <= b.asFloat();
    return EvaluateConstBinary(TokenType::LTE, Value::Int(a), b).isTruthy();
}

inline bool __y_gt(long long a, long long b) { return a > b; }
inline bool __y_gt(double a, double b) { return a > b; }
inline bool __y_gt(const Value& a, const Value& b) {
    if (a.type == ValueType::INT && b.type == ValueType::INT) return a.iVal > b.iVal;
    if (a.type == ValueType::FLOAT && b.type == ValueType::FLOAT) return a.fVal > b.fVal;
    if (a.isNumber() && b.isNumber()) return a.asFloat() > b.asFloat();
    return EvaluateConstBinary(TokenType::GT, a, b).isTruthy();
}
inline bool __y_gt(const Value& a, long long b) {
    if (a.type == ValueType::INT) return a.iVal > b;
    if (a.isNumber()) return a.asFloat() > (double)b;
    return EvaluateConstBinary(TokenType::GT, a, Value::Int(b)).isTruthy();
}
inline bool __y_gt(long long a, const Value& b) {
    if (b.type == ValueType::INT) return a > b.iVal;
    if (b.isNumber()) return (double)a > b.asFloat();
    return EvaluateConstBinary(TokenType::GT, Value::Int(a), b).isTruthy();
}

inline bool __y_gte(long long a, long long b) { return a >= b; }
inline bool __y_gte(double a, double b) { return a >= b; }
inline bool __y_gte(const Value& a, const Value& b) {
    if (a.type == ValueType::INT && b.type == ValueType::INT) return a.iVal >= b.iVal;
    if (a.type == ValueType::FLOAT && b.type == ValueType::FLOAT) return a.fVal >= b.fVal;
    if (a.isNumber() && b.isNumber()) return a.asFloat() >= b.asFloat();
    return EvaluateConstBinary(TokenType::GTE, a, b).isTruthy();
}
inline bool __y_gte(const Value& a, long long b) {
    if (a.type == ValueType::INT) return a.iVal >= b;
    if (a.isNumber()) return a.asFloat() >= (double)b;
    return EvaluateConstBinary(TokenType::GTE, a, Value::Int(b)).isTruthy();
}
inline bool __y_gte(long long a, const Value& b) {
    if (b.type == ValueType::INT) return a >= b.iVal;
    if (b.isNumber()) return (double)a >= b.asFloat();
    return EvaluateConstBinary(TokenType::GTE, Value::Int(a), b).isTruthy();
}

inline bool __y_eq(long long a, long long b) { return a == b; }
inline bool __y_eq(double a, double b) { return a == b; }
inline bool __y_eq(const Value& a, const Value& b) {
    if (a.type == b.type) return a.strictEquals(b);
    if (a.isNumber() && b.isNumber()) return a.asFloat() == b.asFloat();
    return a.looseEquals(b);
}
inline bool __y_eq(const Value& a, long long b) {
    if (a.type == ValueType::INT) return a.iVal == b;
    if (a.isNumber()) return a.asFloat() == (double)b;
    return false;
}
inline bool __y_eq(long long a, const Value& b) { return __y_eq(b, a); }

inline bool __y_neq(long long a, long long b) { return a != b; }
inline bool __y_neq(double a, double b) { return a != b; }
inline bool __y_neq(const Value& a, const Value& b) { return !__y_eq(a, b); }

// --- Strict Equality Helpers ---
inline bool __y_strict_eq(const Value& a, const Value& b) { return a.strictEquals(b); }
inline bool __y_strict_eq(long long a, long long b) { return a == b; }
inline bool __y_strict_eq(double a, double b) { return a == b; }
inline bool __y_strict_eq(bool a, bool b) { return a == b; }
inline bool __y_strict_eq(const std::string& a, const std::string& b) { return a == b; }
inline bool __y_strict_eq(const char* a, const std::string& b) { return std::string(a) == b; }
inline bool __y_strict_eq(const std::string& a, const char* b) { return a == std::string(b); }

inline bool __y_strict_eq(long long a, const Value& b) { return b.type == ValueType::INT && a == b.iVal; }
inline bool __y_strict_eq(const Value& a, long long b) { return a.type == ValueType::INT && a.iVal == b; }
inline bool __y_strict_eq(int a, const Value& b) { return b.type == ValueType::INT && (long long)a == b.iVal; }
inline bool __y_strict_eq(const Value& a, int b) { return a.type == ValueType::INT && a.iVal == (long long)b; }
inline bool __y_strict_eq(double a, const Value& b) { return b.type == ValueType::FLOAT && a == b.fVal; }
inline bool __y_strict_eq(const Value& a, double b) { return a.type == ValueType::FLOAT && a.fVal == b; }
inline bool __y_strict_eq(bool a, const Value& b) { return b.type == ValueType::BOOL && a == b.bVal; }
inline bool __y_strict_eq(const Value& a, bool b) { return a.type == ValueType::BOOL && a.bVal == b; }
inline bool __y_strict_eq(const std::string& a, const Value& b) { return b.type == ValueType::STRING && a == b.asString(); }
inline bool __y_strict_eq(const Value& a, const std::string& b) { return a.type == ValueType::STRING && a.asString() == b; }
inline bool __y_strict_eq(const char* a, const Value& b) { return b.type == ValueType::STRING && std::string(a) == b.asString(); }
inline bool __y_strict_eq(const Value& a, const char* b) { return a.type == ValueType::STRING && a.asString() == std::string(b); }

template <typename T1, typename T2>
inline bool __y_strict_eq(const T1& a, const T2& b) {
    if constexpr (std::is_same_v<T1, T2>) return a == b;
    else return false;
}

template <typename T1, typename T2>
inline bool __y_strict_neq(const T1& a, const T2& b) {
    return !__y_strict_eq(a, b);
}

// --- Membership Helpers ---
inline bool __y_is_in(const Value& lhs, const Value& rhs) {
    if (rhs.type == ValueType::STRING) {
        if (lhs.type != ValueType::STRING) return false;
        return rhs.asString().find(lhs.asString()) != std::string::npos;
    }
    if (rhs.type == ValueType::LIST) {
        auto* list = static_cast<ListObject*>(rhs.ref.get());
        for (const auto& item : list->elements) {
            if (item.strictEquals(lhs)) return true;
        }
        return false;
    }
    if (rhs.type == ValueType::TUPLE) {
        auto* tup = static_cast<TupleObject*>(rhs.ref.get());
        for (const auto& item : tup->elements) {
            if (item.strictEquals(lhs)) return true;
        }
        return false;
    }
    if (rhs.type == ValueType::DICT) {
        auto* d = static_cast<DictObject*>(rhs.ref.get());
        return d->items.count(lhs) > 0;
    }
    if (rhs.type == ValueType::SET) {
        auto* s = static_cast<SetObject*>(rhs.ref.get());
        return s->elements.count(lhs) > 0;
    }
    if (rhs.type == ValueType::RANGE) {
        auto* rng = static_cast<RangeObject*>(rhs.ref.get());
        if (!lhs.isNumber()) return false;
        double val = lhs.asFloat();
        if (!rng->isValid) return false;
        bool inBounds = (rng->step > 0)
            ? (val >= rng->start && (rng->endInclusive ? val <= rng->end : val < rng->end))
            : (val <= rng->start && (rng->endInclusive ? val >= rng->end : val > rng->end));
        if (!inBounds) return false;
        if (!rng->isFloat && lhs.type == ValueType::INT) {
            return ((long long)(val - rng->start) % (long long)rng->step == 0);
        }
        return inBounds;
    }
    return false;
}
inline bool __y_is_not_in(const Value& lhs, const Value& rhs) {
    return !__y_is_in(lhs, rhs);
}

// --- Arithmetic Helpers ---
inline Value __y_add(const Value& a, const Value& b);
inline Value __y_sub(const Value& a, const Value& b);
inline Value __y_mul(const Value& a, const Value& b);
inline Value __y_div(const Value& a, const Value& b);

inline Value __y_add(long long a, long long b) {
    long long res;
    if (!__builtin_add_overflow(a, b, &res)) return Value::Int(res);
    return BigIntObject::add(Value::BigInt(a), Value::BigInt(b));
}
inline Value __y_add(long long a, const Value& b) {
    if (b.type == ValueType::INT) {
        long long res;
        if (!__builtin_add_overflow(a, b.iVal, &res)) return Value::Int(res);
        return BigIntObject::add(Value::BigInt(a), Value::BigInt(b.iVal));
    }
    return __y_add(Value::Int(a), b);
}
inline Value __y_add(const Value& a, long long b) {
    if (a.type == ValueType::INT) {
        long long res;
        if (!__builtin_add_overflow(a.iVal, b, &res)) return Value::Int(res);
        return BigIntObject::add(Value::BigInt(a.iVal), Value::BigInt(b));
    }
    return __y_add(a, Value::Int(b));
}
inline Value __y_add(double a, double b) { return Value::Float(a + b); }
inline Value __y_add(const Value& a, double b) { return __y_add(a, Value::Float(b)); }
inline Value __y_add(double a, const Value& b) { return __y_add(Value::Float(a), b); }

inline Value __y_add(const Value& a, const Value& b) {
    if (a.type == ValueType::INT && b.type == ValueType::INT) {
        long long res;
        if (!__builtin_add_overflow(a.iVal, b.iVal, &res)) return Value::Int(res);
        return BigIntObject::add(Value::BigInt(a.iVal), Value::BigInt(b.iVal));
    }
    if (a.type == ValueType::FLOAT && b.type == ValueType::FLOAT) {
        return Value::Float(a.fVal + b.fVal);
    }
    return EvaluateConstBinary(TokenType::PLUS, a, b);
}

inline Value __y_sub(long long a, long long b) {
    long long res;
    if (!__builtin_sub_overflow(a, b, &res)) return Value::Int(res);
    return BigIntObject::sub(Value::BigInt(a), Value::BigInt(b));
}
inline Value __y_sub(long long a, const Value& b) {
    if (b.type == ValueType::INT) {
        long long res;
        if (!__builtin_sub_overflow(a, b.iVal, &res)) return Value::Int(res);
        return BigIntObject::sub(Value::BigInt(a), Value::BigInt(b.iVal));
    }
    return __y_sub(Value::Int(a), b);
}
inline Value __y_sub(const Value& a, long long b) {
    if (a.type == ValueType::INT) {
        long long res;
        if (!__builtin_sub_overflow(a.iVal, b, &res)) return Value::Int(res);
        return BigIntObject::sub(Value::BigInt(a.iVal), Value::BigInt(b));
    }
    return __y_sub(a, Value::Int(b));
}
inline Value __y_sub(double a, double b) { return Value::Float(a - b); }
inline Value __y_sub(const Value& a, double b) { return __y_sub(a, Value::Float(b)); }
inline Value __y_sub(double a, const Value& b) { return __y_sub(Value::Float(a), b); }

inline Value __y_sub(const Value& a, const Value& b) {
    if (a.type == ValueType::INT && b.type == ValueType::INT) {
        long long res;
        if (!__builtin_sub_overflow(a.iVal, b.iVal, &res)) return Value::Int(res);
        return BigIntObject::sub(Value::BigInt(a.iVal), Value::BigInt(b.iVal));
    }
    if (a.type == ValueType::FLOAT && b.type == ValueType::FLOAT) {
        return Value::Float(a.fVal - b.fVal);
    }
    return EvaluateConstBinary(TokenType::MINUS, a, b);
}

inline Value __y_mul(long long a, long long b) {
    long long res;
    if (!__builtin_mul_overflow(a, b, &res)) return Value::Int(res);
    return BigIntObject::mul(Value::BigInt(a), Value::BigInt(b));
}
inline Value __y_mul(long long a, const Value& b) {
    if (b.type == ValueType::INT) {
        long long res;
        if (!__builtin_mul_overflow(a, b.iVal, &res)) return Value::Int(res);
        return BigIntObject::mul(Value::BigInt(a), Value::BigInt(b.iVal));
    }
    return __y_mul(Value::Int(a), b);
}
inline Value __y_mul(const Value& a, long long b) {
    if (a.type == ValueType::INT) {
        long long res;
        if (!__builtin_mul_overflow(a.iVal, b, &res)) return Value::Int(res);
        return BigIntObject::mul(Value::BigInt(a.iVal), Value::BigInt(b));
    }
    return __y_mul(a, Value::Int(b));
}
inline Value __y_mul(double a, double b) { return Value::Float(a * b); }
inline Value __y_mul(const Value& a, double b) { return __y_mul(a, Value::Float(b)); }
inline Value __y_mul(double a, const Value& b) { return __y_mul(Value::Float(a), b); }

inline Value __y_mul(const Value& a, const Value& b) {
    if (a.type == ValueType::INT && b.type == ValueType::INT) {
        long long res;
        if (!__builtin_mul_overflow(a.iVal, b.iVal, &res)) return Value::Int(res);
        return BigIntObject::mul(Value::BigInt(a.iVal), Value::BigInt(b.iVal));
    }
    if (a.type == ValueType::FLOAT && b.type == ValueType::FLOAT) {
        return Value::Float(a.fVal * b.fVal);
    }
    return EvaluateConstBinary(TokenType::STAR, a, b);
}

inline Value __y_div(long long a, long long b) {
    if (b == 0) throw std::runtime_error("Division by zero");
    return Value::Float((double)a / (double)b);
}
inline Value __y_div(long long a, const Value& b) { return __y_div(Value::Int(a), b); }
inline Value __y_div(const Value& a, long long b) {
    if (b == 0) throw std::runtime_error("Division by zero");
    return __y_div(a, Value::Float((double)b));
}
inline Value __y_div(double a, double b) {
    if (b == 0.0) throw std::runtime_error("Division by zero");
    return Value::Float(a / b);
}
inline Value __y_div(const Value& a, double b) {
    if (b == 0.0) throw std::runtime_error("Division by zero");
    return __y_div(a, Value::Float(b));
}
inline Value __y_div(double a, const Value& b) { return __y_div(Value::Float(a), b); }

inline Value __y_div(const Value& a, const Value& b) {
    if (a.isNumber() && b.isNumber()) {
        double db = b.asFloat();
        if (db == 0.0) throw std::runtime_error("Division by zero");
        return Value::Float(a.asFloat() / db);
    }
    return EvaluateConstBinary(TokenType::SLASH, a, b);
}

// --- Zero-Overhead Container Accessors ---
inline std::vector<Value>* __y_get_vector_ptr(Value& val) {
    if (val.type == ValueType::REFERENCE && val.get_ptr_safe()) {
        return __y_get_vector_ptr(*val.get_ptr_safe());
    }
    if (val.type == ValueType::LIST) {
        return &static_cast<ListObject*>(val.ref.get())->elements;
    } else if (val.type == ValueType::VECTOR) {
        return &static_cast<VectorObject*>(val.ref.get())->elements;
    } else if (val.type == ValueType::TUPLE) {
        return &static_cast<TupleObject*>(val.ref.get())->elements;
    }
    return nullptr;
}

inline const std::vector<Value>* __y_get_vector_ptr(const Value& val) {
    if (val.type == ValueType::REFERENCE && val.get_ptr_safe()) {
        return __y_get_vector_ptr(*val.get_ptr_safe());
    }
    if (val.type == ValueType::LIST) {
        return &static_cast<const ListObject*>(val.ref.get())->elements;
    } else if (val.type == ValueType::VECTOR) {
        return &static_cast<const VectorObject*>(val.ref.get())->elements;
    } else if (val.type == ValueType::TUPLE) {
        return &static_cast<const TupleObject*>(val.ref.get())->elements;
    }
    return nullptr;
}

inline std::unordered_map<Value, Value, ValueHash, ValueEqual>* __y_get_dict_ptr(Value& val) {
    if (val.type == ValueType::REFERENCE && val.get_ptr_safe()) {
        return __y_get_dict_ptr(*val.get_ptr_safe());
    }
    if (val.type == ValueType::DICT) {
        return &static_cast<DictObject*>(val.ref.get())->items;
    }
    return nullptr;
}

inline const std::unordered_map<Value, Value, ValueHash, ValueEqual>* __y_get_dict_ptr(const Value& val) {
    if (val.type == ValueType::REFERENCE && val.get_ptr_safe()) {
        return __y_get_dict_ptr(*val.get_ptr_safe());
    }
    if (val.type == ValueType::DICT) {
        return &static_cast<const DictObject*>(val.ref.get())->items;
    }
    return nullptr;
}

// --- Sequence Materialization Helper ---
inline std::vector<Value> __y_to_vector(const Value& val) {
    if (const auto* vec = __y_get_vector_ptr(val)) {
        return *vec;
    } else if (val.type == ValueType::STRING) {
        std::string s = val.asString();
        std::vector<Value> res;
        res.reserve(s.size());
        for (char c : s) res.push_back(Value::String(std::string(1, c)));
        return res;
    } else if (val.type == ValueType::SET) {
        auto* s = static_cast<SetObject*>(val.ref.get());
        std::vector<Value> res;
        res.reserve(s->elements.size());
        for (const auto& elem : s->elements) res.push_back(elem);
        return res;
    } else if (val.type == ValueType::RANGE) {
        auto* rng = static_cast<RangeObject*>(val.ref.get());
        std::vector<Value> res;
        if (rng && rng->isValid && rng->step != 0) {
            if (rng->step > 0) {
                for (double d = rng->start; rng->endInclusive ? (d <= rng->end) : (d < rng->end); d += rng->step) {
                    res.push_back(rng->isFloat ? Value::Float(d) : Value::Int((long long)d));
                }
            } else {
                for (double d = rng->start; rng->endInclusive ? (d >= rng->end) : (d > rng->end); d += rng->step) {
                    res.push_back(rng->isFloat ? Value::Float(d) : Value::Int((long long)d));
                }
            }
        }
        return res;
    }
    return {};
}

// --- Optimized Indexing ---
inline Value __y_get_index(const Value& base, long long idx) {
    if (const auto* vec = __y_get_vector_ptr(base)) {
        if (idx < 0) idx += (long long)vec->size();
        if (idx >= 0 && idx < (long long)vec->size()) return (*vec)[idx];
        return Value::None();
    }
    if (base.type == ValueType::STRING) {
        std::string s = base.asString();
        if (idx < 0) idx += (long long)s.size();
        if (idx >= 0 && idx < (long long)s.size()) return Value::String(std::string(1, s[idx]));
    }
    return Value::None();
}

inline Value __y_get_index(const Value& base, const Value& index) {
    if (index.type == ValueType::INT) {
        return __y_get_index(base, index.iVal);
    }
    if (const auto* dict = __y_get_dict_ptr(base)) {
        auto it = dict->find(index);
        if (it != dict->end()) return it->second;
        return Value::None();
    }
    if (const auto* vec = __y_get_vector_ptr(base)) {
        long long idx = index.asInt();
        if (idx < 0) idx += (long long)vec->size();
        if (idx >= 0 && idx < (long long)vec->size()) return (*vec)[idx];
        return Value::None();
    }
    if (base.type == ValueType::STRING) {
        std::string s = base.asString();
        long long idx = index.asInt();
        if (idx < 0) idx += (long long)s.size();
        if (idx >= 0 && idx < (long long)s.size()) return Value::String(std::string(1, s[idx]));
    }
    return Value::None();
}

inline void __y_set_index(Value& base, long long idx, const Value& val) {
    if (auto* vec = __y_get_vector_ptr(base)) {
        if (idx < 0) idx += (long long)vec->size();
        if (idx >= 0 && idx < (long long)vec->size()) (*vec)[idx] = val;
        return;
    }
    if (auto* dict = __y_get_dict_ptr(base)) {
        (*dict)[Value::Int(idx)] = val;
        return;
    }
}

inline void __y_set_index(Value& base, const Value& index, const Value& val) {
    if (index.type == ValueType::INT) {
        __y_set_index(base, index.iVal, val);
        return;
    }
    if (auto* dict = __y_get_dict_ptr(base)) {
        (*dict)[index] = val;
        return;
    }
    if (auto* vec = __y_get_vector_ptr(base)) {
        long long idx = index.asInt();
        if (idx < 0) idx += (long long)vec->size();
        if (idx >= 0 && idx < (long long)vec->size()) (*vec)[idx] = val;
        return;
    }
}

// --- Dynamic Function Invocation ---
inline Value __y_call(Value callee, const std::vector<Value>& args) {
    if (callee.type == ValueType::REFERENCE && callee.get_ptr_safe()) callee = *callee.get_ptr_safe();
    if (callee.type == ValueType::NATIVE_FUNCTION) {
        auto* nf = static_cast<NativeFunctionObject*>(callee.ref.get());
        int pos_count = 0;
        while (pos_count < (int)args.size() && args[pos_count].type != ValueType::PAIRED) pos_count++;
        int kw_count = (int)args.size() - pos_count;
        return nf->func(args, pos_count, kw_count);
    } else if (callee.type == ValueType::CLASS) {
        auto* cls = static_cast<ClassObject*>(callee.ref.get());
        Value instance = Value::Instance(callee);
        if (cls->methods.count("__construct__")) {
            Value initMethod = cls->methods["__construct__"].func;
            if (initMethod.type == ValueType::NATIVE_FUNCTION) {
                int pos_count = 0;
                while (pos_count < (int)args.size() && args[pos_count].type != ValueType::PAIRED) pos_count++;
                int kw_count = (int)args.size() - pos_count;
                static_cast<NativeFunctionObject*>(initMethod.ref.get())->func(args, pos_count, kw_count);
            }
        }
        return instance;
    }
    return Value::None();
}

// --- Scope Restoration ---
struct EnvRestorer {
    std::shared_ptr<Env>& current_env;
    std::shared_ptr<Env> previous_env;
    EnvRestorer(std::shared_ptr<Env>& ce) : current_env(ce), previous_env(ce) {
        current_env = std::make_shared<Env>();
        current_env->parent = previous_env;
    }
    ~EnvRestorer() { current_env = previous_env; }
};

// --- Globals & Method Dispatch ---
inline std::shared_ptr<Env> y_transpile_globals = std::make_shared<Env>();
inline VM transpiled_vm;

inline Value __y_method_call(const std::string& method, Value obj, std::vector<Value> args) {
    if (obj.type == ValueType::REFERENCE && obj.get_ptr_safe()) obj = *obj.get_ptr_safe();
    std::vector<ValueExpr*> dummyArgs;
    for (auto& arg : args) dummyArgs.push_back(new ValueExpr(arg));
    ValueExpr* dummyObject = new ValueExpr(obj);
    std::vector<Expr*> exprArgs(dummyArgs.begin(), dummyArgs.end());
    MethodCallExpr mockAST(dummyObject, method, exprArgs);
    Value result = transpiled_vm.methodResolver(&mockAST);
    delete dummyObject;
    for (auto* a : dummyArgs) delete a;
    return result;
}
