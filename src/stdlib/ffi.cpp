#include "ylang/vm.hpp"
#include "objects.hpp"
#include "types.hpp"

#ifdef _WIN32
#include <shellapi.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif

void Interpreter::registerFfi() {
	env->set("cppCompile", Value::Native([this](const vector<Value> &args, int l, int c) {
		if (args.size() < 3 || args[0].type != ValueType::STRING || args[2].type != ValueType::DICT) {
			throw TypeError("cppCompile expects (String code, Value return_type, Dict args, String path_to_cpp_compile)", l, c);
		}
		std::string raw_user_code = args[0].asString();
		std::string includes = "";
		std::string logic = "";
		std::istringstream stream(raw_user_code);
		std::string line;
		while (std::getline(stream, line)) {
			size_t start = line.find_first_not_of(" \t");
			if (start != std::string::npos && line.compare(start, 8, "#include") == 0) {
				includes += line + "\n";
			} else {
				logic += line + "\n";
			}
		}
		std::string user_code = logic;
		std::string cache_key = raw_user_code;
		auto *dict = static_cast<DictObject *>(args[2].ref.get());
		std::vector<std::string> sorted_keys;
		for (const auto &[key, val] : dict->items) {
			sorted_keys.push_back(key.asString());
		}
		std::sort(sorted_keys.begin(), sorted_keys.end());
		std::function<std::string(const Value &, int, int)> getDeepCppType = [&](const Value &v, int l, int c) -> std::string {
			if (v.type == ValueType::INT)
				return "long long";
			if (v.type == ValueType::FLOAT)
				return "double";
			if (v.type == ValueType::BOOL)
				return "bool";
			if (v.type == ValueType::STRING)
				return "std::string";
			if (v.type == ValueType::LIST || v.type == ValueType::VECTOR || v.type == ValueType::TUPLE) {
				const auto &elems = (v.type == ValueType::LIST) ? static_cast<ListObject *>(v.ref.get())->elements : (v.type == ValueType::VECTOR) ? static_cast<VectorObject *>(v.ref.get())->elements
																																															  : static_cast<TupleObject *>(v.ref.get())->elements;
				if (elems.empty())
					throw TypeError("Cannot deduce deep C++ type from empty container", l, c);
				return "std::vector<" + getDeepCppType(elems[0], l, c) + ">";
			}
			if (v.type == ValueType::DICT) {
				auto *dictObj = static_cast<DictObject *>(v.ref.get());
				if (dictObj->items.empty())
					throw TypeError("Cannot deduce deep C++ type from empty Dict", l, c);
				auto first_pair = dictObj->items.begin();
				return "std::unordered_map<" + getDeepCppType(first_pair->first, l, c) + ", " + getDeepCppType(first_pair->second, l, c) + ">";
			}
			if (v.type == ValueType::SET) {
				auto *setObj = static_cast<SetObject *>(v.ref.get());
				if (setObj->elements.empty())
					throw TypeError("Cannot deduce deep C++ type from empty Set", l, c);
				auto first_it = setObj->elements.begin();
				return "std::unordered_set<" + getDeepCppType(*first_it, l, c) + ">";
			}
			throw TypeError("Unsupported nested type for C++ FFI translation", l, c);
		};

		for (const auto &k : sorted_keys) {
			Value val = dict->items.at(Value::String(k));
			cache_key += "|" + k + ":" + getDeepCppType(val, l, c);
		}
		static std::unordered_map<std::string, void *> ffi_cache;
		typedef Value (*MacroFunc)(Value *);
		MacroFunc exec_func = nullptr;
		if (ffi_cache.find(cache_key) != ffi_cache.end()) {
			// CACHED
			exec_func = (MacroFunc)ffi_cache[cache_key];
		} else {
			// --- NOT CACHED: MUST COMPILE ---
			std::stringstream cpp;
			cpp << "#include <" << args[3].asString() << ">\n";
			cpp << "#include <string>\\n";
			cpp << "#include <type_traits>\n\n";
			cpp << includes << "\n";
			// 1. Unpackers: y-- Value -> Raw C++ (Using Struct Specialization for safe recursion)
			cpp << "template<typename T> struct Unboxer;\n";
			cpp << "template<> struct Unboxer<long long> { static long long get(const Value& v) { return v.asInt(); } };\n";
			cpp << "template<> struct Unboxer<int> { static int get(const Value& v) { return (int)v.asInt(); } };\n";
			cpp << "template<> struct Unboxer<size_t> { static int get(const Value& v) { return (size_t)v.asInt(); } };\n";
			cpp << "template<> struct Unboxer<double> { static double get(const Value& v) { return v.asFloat(); } };\n";
			cpp << "template<> struct Unboxer<float> { static float get(const Value& v) { return (float)v.asFloat(); } };\n";
			cpp << "template<> struct Unboxer<bool> { static bool get(const Value& v) { return v.asBool(); } };\n";
			cpp << "template<> struct Unboxer<std::string> { static std::string get(const Value& v) { return v.asString(); } };\n\n";

			cpp << "template<typename T> struct Unboxer<std::vector<T>> {\n";
			cpp << "    static std::vector<T> get(const Value& v) {\n";
			cpp << "        std::vector<T> res;\n";
			cpp << "        if (v.type == ValueType::LIST) {\n";
			cpp << "            for(const auto& e : static_cast<ListObject*>(v.ref.get())->elements) res.push_back(Unboxer<T>::get(e));\n";
			cpp << "        } else if (v.type == ValueType::VECTOR) {\n";
			cpp << "            for(const auto& e : static_cast<VectorObject*>(v.ref.get())->elements) res.push_back(Unboxer<T>::get(e));\n";
			cpp << "        } else if (v.type == ValueType::TUPLE) {\n";
			cpp << "            for(const auto& e : static_cast<TupleObject*>(v.ref.get())->elements) res.push_back(Unboxer<T>::get(e));\n";
			cpp << "        }\n";
			cpp << "        return res;\n";
			cpp << "    }\n";
			cpp << "};\n";
			cpp << "template<typename T> T ValueToCpp(const Value& v) { return Unboxer<T>::get(v); }\n\n";

			cpp << "template<typename K, typename V> struct Unboxer<std::unordered_map<K, V>> {\n";
			cpp << "    static std::unordered_map<K, V> get(const Value& v) {\n";
			cpp << "        std::unordered_map<K, V> res;\n";
			cpp << "        for(const auto& [key, val] : static_cast<DictObject*>(v.ref.get())->items) {\n";
			cpp << "            res[Unboxer<K>::get(key)] = Unboxer<V>::get(val);\n";
			cpp << "        }\n";
			cpp << "        return res;\n";
			cpp << "    }\n";
			cpp << "};\n";

			cpp << "template<typename T> struct Unboxer<std::unordered_set<T>> {\n";
			cpp << "    static std::unordered_set<T> get(const Value& v) {\n";
			cpp << "        std::unordered_set<T> res;\n";
			cpp << "        for(const auto& e : static_cast<SetObject*>(v.ref.get())->elements) {\n";
			cpp << "            res.insert(Unboxer<T>::get(e));\n";
			cpp << "        }\n";
			cpp << "        return res;\n";
			cpp << "    }\n";
			cpp << "};\n";

			// Raw C++ -> y-- Value
			cpp << "template<typename T> Value CppToValue(T val);\n";
			cpp << "template<> Value CppToValue(long long v) { return Value::Int(v); }\n";
			cpp << "template<> Value CppToValue(int v) { return Value::Int(v); }\n";
			cpp << "template<> Value CppToValue(size_t v) { return Value::Int(v); }\n";
			cpp << "template<> Value CppToValue(double v) { return Value::Float(v); }\n";
			cpp << "template<> Value CppToValue(float v) { return Value::Float(v); }\n";
			cpp << "template<> Value CppToValue(bool v) { return Value::Bool(v); }\n";
			cpp << "template<> Value CppToValue(std::string v) { return Value::String(v); }\n";
			cpp << "template<> Value CppToValue(const char* v) { return Value::String(std::string(v)); }\n\n";

			cpp << "template<typename T> Value CppToValue(const std::vector<T>& vec) {\n";
			cpp << "    std::vector<Value> res;\n";
			cpp << "    res.reserve(vec.size());\n";
			cpp << "    for(const auto& e : vec) res.push_back(CppToValue(e));\n";
			cpp << "    return Value::List(res);\n";
			cpp << "}\n\n";

			cpp << "template<typename K, typename V> Value CppToValue(const std::unordered_map<K, V>& m) {\n";
			cpp << "    std::unordered_map<Value, Value, ValueHash, ValueEqual> res;\n";
			cpp << "    for(const auto& [k, v] : m) res[CppToValue(k)] = CppToValue(v);\n";
			cpp << "    return Value::Dict(res);\n";
			cpp << "}\n\n";

			cpp << "template<typename T> Value CppToValue(const std::unordered_set<T>& s) {\n";
			cpp << "    std::unordered_set<Value, ValueHash, ValueEqual> res;\n";
			cpp << "    for(const auto& e : s) setAdd(res, CppToValue(e));\n";
			cpp << "    return Value::Set(res);\n";
			cpp << "}\n\n";

			cpp << "template<typename T1, typename T2> Value CppToValue(const std::pair<T1, T2>& p) {\n";
			cpp << "    return Value::Tuple({CppToValue(p.first), CppToValue(p.second)});\n";
			cpp << "}\n\n";

			cpp << "extern \"C\" {\n";
			cpp << "#ifdef _WIN32\n__declspec(dllexport)\n#endif\n";
			cpp << "Value y_macro_exec(Value* args_array) {\n";
			// RECURSIVE Type Sniffer
			// Unpack variables based on SORTED order
			for (size_t i = 0; i < sorted_keys.size(); i++) {
				std::string var_name = sorted_keys[i];
				Value val = dict->items.at(Value::String(var_name));
				// Sniff the deep type signature
				std::string cpp_type = getDeepCppType(val, l, c);
				// Emit exactly ONE C++ line to invoke the recursive Unboxer template
				cpp << "    " << cpp_type << " " << var_name << " = ValueToCpp<" << cpp_type << ">(args_array[" << i << "]);\n";
			}
			cpp << "\n    auto user_logic = [&]() {\n";
			cpp << "        " << user_code << "\n";
			cpp << "    };\n\n";
			cpp << "    using RetType = decltype(user_logic());\n";
			cpp << "    if constexpr (std::is_same_v<RetType, void>) {\n";
			cpp << "        user_logic();\n";
			cpp << "        return Value::None();\n";
			cpp << "    } else {\n";
			cpp << "        return CppToValue(user_logic());\n";
			cpp << "    }\n";
			cpp << "}\n}\n";
			std::string current_dir = std::filesystem::current_path().string();
			std::string hash_str = std::to_string(std::hash<std::string>{}(cache_key));
			std::string debug_flag = "";
#ifdef VM_DEBUG_MODE // NEVER USE
			debug_flag = " -DVM_DEBUG_MODE ";
#endif
#ifdef _WIN32
			char exePathStr[MAX_PATH];
			GetModuleFileNameA(NULL, exePathStr, MAX_PATH);
			std::filesystem::path exePath(exePathStr);
			std::string exeDir = exePath.parent_path().string();
			std::string cpp_file = current_dir + "\\y_macro_" + hash_str + ".cpp";
			std::string lib_file = current_dir + "\\y_macro_" + hash_str + ".dll";
			std::string implib_file = exeDir + "\\y_lang.a";
			std::string cmd = "g++ -std=c++17 -shared -O3 -fopenmp " + debug_flag +
									"-I\"" + current_dir + "\" " +
									"-I/c/raylib-src/raylib-5.5/src " +
									"\"" + cpp_file + "\" -o \"" + lib_file + "\" " +
									"\"" + implib_file + "\" " +
									"-Wl,--allow-multiple-definition 2>&1";
#else
			std::string cpp_file = "/tmp/y_macro_" + hash_str + ".cpp";
			std::string lib_file = "/tmp/y_macro_" + hash_str + ".so";
			std::string cmd = "g++ -std=c++17 -shared -fPIC -O3 -fopenmp " + debug_flag + "-I\"" + current_dir + "\" " + cpp_file + " -o " + lib_file + " -lraylib 2>&1";
#endif
			std::ofstream out(cpp_file);
			out << cpp.str();
			out.flush();
			out.close();
			if (system(cmd.c_str()) != 0)
				throw RuntimeError("C++ FFI Compilation failed!", l, c);
#ifdef _WIN32
			SetDllDirectoryA(exeDir.c_str());
			HINSTANCE handle = LoadLibraryA(lib_file.c_str());
			SetDllDirectoryA(NULL); // Reset immediately for security

			if (!handle) {
				DWORD err = GetLastError();
				throw RuntimeError("Failed to load DLL. Windows Error Code: " + std::to_string(err), l, c);
			}
			exec_func = (MacroFunc)GetProcAddress(handle, "y_macro_exec");
#else
			void *handle = dlopen(lib_file.c_str(), RTLD_NOW);
			if (!handle)
				throw RuntimeError(std::string("Failed to load SO: ") + dlerror(), l, c);
			exec_func = (MacroFunc)dlsym(handle, "y_macro_exec");
#endif
			if (!exec_func)
				throw RuntimeError("Failed to find execution hook", l, c);
			ffi_cache[cache_key] = (void *)exec_func;
		}
		std::vector<Value> packed_args;
		for (const auto &k : sorted_keys) {
			packed_args.push_back(dict->items.at(Value::String(k)));
		}
		Value raw_result = exec_func(packed_args.data());
		ValueType expected = args[1].type;
		if (expected == ValueType::NONE || expected == ValueType::NOTYPE || expected == raw_result.type) {
			return raw_result;
		}
		if (expected == ValueType::VECTOR && raw_result.type == ValueType::LIST) {
			return Value::Vector(static_cast<ListObject *>(raw_result.ref.get())->elements);
		}
		if (expected == ValueType::LIST && raw_result.type == ValueType::VECTOR) {
			return Value::List(static_cast<VectorObject *>(raw_result.ref.get())->elements);
		}
		if (expected == ValueType::SET && raw_result.type == ValueType::LIST) {
			auto &elems = static_cast<ListObject *>(raw_result.ref.get())->elements;
			std::unordered_set<Value, ValueHash, ValueEqual> s(elems.begin(), elems.end());
			return Value::Set(s);
		}
		if (expected == ValueType::TUPLE && raw_result.type == ValueType::LIST) {
			return Value::Tuple(static_cast<ListObject *>(raw_result.ref.get())->elements);
		}
		return raw_result;
	}),
		false);
}
