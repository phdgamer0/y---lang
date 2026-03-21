#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <climits>
#include <cmath>
#include <optional>
#include <cctype>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <numeric>

using namespace std;

/**
 * Capitalizes the first character of the string.
 * Optimization: Uses const reference to avoid copy. Returns modified copy.
 * Time Complexity: O(1) for access, O(N) for copy.
 */
inline string capitalize(const string& s) {
	if (s.empty()) return "";
	string res = s;
	if (res[0] >= 'a' && res[0] <= 'z') res[0] = ::toupper(res[0]);
	return res;
}


inline std::vector<std::string> chars(const std::string& s, int begin = 0, int end = -1) {
	std::vector<std::string> result;
	int len = (int)s.length();

	// 1. Handle Negative Start
	// If begin is -1, it means the last character. -2 is second to last, etc.
	if (begin < 0) begin += len;

	// Clamp Start
	if (begin < 0) begin = 0;
	if (begin >= len) return result; // Start is out of bounds

	// 2. Handle End
	// Special case: -1 as 'end' defaults to the actual end of the string
	if (end == -1) end = len;
	// Handle other negative end indices (e.g. chars(s, 0, -2) removes last 2 chars)
	else if (end < 0) end += len;

	// Clamp End
	if (end > len) end = len;
	if (end <= begin) return result; // Empty range

	// 3. Extract Characters
	result.reserve(end - begin);
	for (int i = begin; i < end; i++) {
		result.push_back(std::string(1, s[i]));
	}

	return result;
}

/**
 * Converts entire string to lowercase (Caseless matching).
 * Optimization: Single pass, modifies copy in place.
 * Time Complexity: O(N)
 */
inline string casefold(const string& s) {
	string res = s;
	for (char& c : res) c = ::tolower(c);
	return res;
}

/**
 * Centers string 's' within 'length', padded by 'character'.
 * Optimization: Pre-calculates padding size and reserves memory to prevent reallocation.
 * Time Complexity: O(N)
 */
inline string center(const string& s, int length, char character = ' ') {
	if ((int)s.size() >= length) return s;

	int pad = length - (int)s.size();
	int left = pad / 2;
	int right = pad - left; // Handles odd padding correctly

	string res;
	res.reserve(length); // Critical optimization: Allocates memory once
	res.append(left, character);
	res.append(s);
	res.append(right, character);
	return res;
}

/**
 * Counts occurrences of 'target' within 's'.
 * Optimization: Uses std::string::find (SIMD accelerated) instead of manual loops.
 * Time Complexity: O(N/M) where M is target length.
 */
inline int count(const string& s, const string& target, size_t start = 0, size_t end = string::npos) {
	if (target.empty()) return 0; // Standard Python behavior for empty target is complex, simpler to return 0 or N+1
	if (end > s.size()) end = s.size();
	if (start >= end) return 0;

	int count = 0;
	// Optimization: find() is much faster than manual iteration
	while ((start = s.find(target, start)) != string::npos) {
		if (start + target.size() > end) break;
		count++;
		start += target.size(); // Move past the match to avoid overlapping counts
	}
	return count;
}

/**
 * Checks if string ends with suffix.
 * Optimization: Uses direct pointer arithmetic/iterators or std::equal logic.
 * Time Complexity: O(K) where K is target length.
 */
inline bool endswith(const string& s, const string& target) {
	if (s.size() < target.size()) return false;
	// Optimization: Compare only the tail bytes directly
	return s.compare(s.size() - target.size(), target.size(), target) == 0;
}

/**
 * Expands tabs to spaces.
 * Optimization: Uses reserve() and simple arithmetic.
 * Time Complexity: O(N)
 */
inline string expandtabs(const string& s, int tabsize = 4) {
	if (tabsize <= 0) return s; // Sanity check
	string res;
	res.reserve(s.size() * 2); // Heuristic reservation

	int col = 0;
	for (char c : s) {
		if (c == '\t') {
			int spaces = tabsize - (col % tabsize);
			res.append(spaces, ' ');
			col += spaces;
		}
		else {
			res += c;
			col++;
			if (c == '\n' || c == '\r') col = 0;
		}
	}
	return res;
}

/**
 * Finds index of target substring. Returns -1 if not found.
 * Optimization: Wraps std::string::find.
 * Time Complexity: Average O(N).
 */
inline int index(const string& s, const string& target, size_t start = 0, size_t end = string::npos) {
	if (end > s.size()) end = s.size();
	size_t pos = s.find(target, start);
	if (pos != string::npos && pos + target.size() <= end) {
		return (int)pos;
	}
	return -1;
}

// Character checks (optimized standard wrappers)
inline bool isalnum_str(const string& s) {
	if (s.empty()) return false;
	return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isalnum(c); });
}
inline bool isalpha_str(const string& s) {
	if (s.empty()) return false;
	return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isalpha(c); });
}
inline bool isdecimal_str(const string& s) {
	if (s.empty()) return false;
	size_t i = 0;
	if (s[i] == '-' || s[i] == '+') i++;
	if (i == s.size()) return false;
	bool has_dot = false;
	bool has_digit = false;
	for (; i < s.size(); ++i) {
		if (std::isdigit(static_cast<unsigned char>(s[i]))) has_digit = true;
		else if (s[i] == '.') {
			if (has_dot) return false;
			has_dot = true;
		}
		else return false;
	}
	return has_digit;
}
inline bool islower_str(const string& s) {
	bool has_cased = false;
	for (unsigned char c : s) {
		if (std::isupper(c)) return false;
		if (std::islower(c)) has_cased = true;
	}
	return has_cased;
}
inline bool isupper_str(const string& s) {
	bool has_cased = false;
	for (unsigned char c : s) {
		if (std::islower(c)) return false;
		if (std::isupper(c)) has_cased = true;
	}
	return has_cased;
}

/**
 * Joins a list of strings with a delimiter.
 * Optimization: Calculates total required memory first, allocates ONCE, then copies.
 * Massive speedup for large lists.
 */
inline string join(const vector<string>& args, const string& bridge = "") {
	if (args.empty()) return "";

	// 1. Calculate total length
	size_t len = 0;
	for (const auto& s : args) len += s.size();
	len += bridge.size() * (args.size() - 1);

	// 2. Allocate once
	string res;
	res.reserve(len);

	// 3. Assemble
	for (size_t i = 0; i < args.size(); ++i) {
		if (i > 0) res += bridge;
		res += args[i];
	}
	return res;
}

/**
 * Left-justifies string.
 * Optimization: Uses reserve and append logic.
 */
inline string ljust(const string& s, int num, char fillchar = ' ') {
	if ((int)s.size() >= num) return s;
	string res = s;
	res.resize(num, fillchar); // Fastest way to pad
	return res;
}

/**
 * Converts string to lowercase.
 */
inline string lower(const string& s) {
	string res = s;
	std::transform(res.begin(), res.end(), res.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return res;
}

/**
 * Removes leading characters.
 * Optimization: Uses find_first_not_of to jump immediately to data.
 */
inline string lstrip(const string& s, const string& chars = " \t\n\r\v\f") {
	size_t start = s.find_first_not_of(chars);
	return (start == string::npos) ? "" : s.substr(start);
}

/**
 * Removes trailing characters.
 * Optimization: Uses find_last_not_of.
 */
inline string rstrip(const string& s, const string& chars = " \t\n\r\v\f") {
	size_t end = s.find_last_not_of(chars);
	return (end == string::npos) ? "" : s.substr(0, end + 1);
}

/**
 * Removes leading and trailing characters.
 * Optimization: Combination of lstrip/rstrip logic with substring.
 */
inline string strip(const string& s, const string& chars = " \t\n\r\v\f") {
	size_t start = s.find_first_not_of(chars);
	if (start == string::npos) return "";
	size_t end = s.find_last_not_of(chars);
	return s.substr(start, end - start + 1);
}

/**
 * Splits string into 3 parts: head, sep, tail.
 */
inline const vector<string> partition(const string& s, const string& target) {
	size_t pos = s.find(target);
	if (pos == string::npos) return { s, "", "" };
	return { s.substr(0, pos), target, s.substr(pos + target.size()) };
}

/**
 * Splits string by delimiter.
 * Optimization: Uses find() in a loop. Does NOT perform deep copies or strip implicitly.
 */
inline vector<string> split(const string& s, const string& delimiter = " ") {
	vector<string> tokens;
	if (delimiter.empty()) { tokens.push_back(s); return tokens; } // Safety

	size_t pos = 0;
	size_t found = 0;

	// Heuristic: Reserve space for at least a few tokens to avoid early vector resizing
	tokens.reserve(s.size() / delimiter.size() / 2);

	while ((found = s.find(delimiter, pos)) != string::npos) {
		tokens.push_back(s.substr(pos, found - pos));
		pos = found + delimiter.size();
	}
	tokens.push_back(s.substr(pos));
	return tokens;
}

/**
 * Replaces occurrences of target with replacement.
 * Optimization: Uses find() and append(). Much faster than split+join.
 */
inline string replace(const string& s, const string& target, const string& replacement, int count = -1) {
	if (target.empty()) return s;

	// Fast path: if target not found, return copy
	size_t first = s.find(target);
	if (first == string::npos) return s;

	string res;
	res.reserve(s.size()); // Optimistic reservation

	size_t pos = 0;
	int replacements = 0;

	while ((count < 0 || replacements < count) && (first = s.find(target, pos)) != string::npos) {
		res.append(s, pos, first - pos); // Copy text before match
		res.append(replacement);         // Copy replacement
		pos = first + target.size();     // Advance
		replacements++;
	}
	res.append(s, pos, string::npos); // Copy remainder
	return res;
}

/**
 * Right-justifies string.
 * Optimization: Uses reserve and append logic.
 */
inline std::string rjust(const std::string& s, int num, char fillchar = ' ') {
   int s_size = static_cast<int>(s.size());
   if (s_size >= num) return s;

   std::string res(num, fillchar);
   // Copy the original string into the end of the newly padded string
   res.replace(num - s_size, s_size, s);
   return res;
}

/**
 * Checks if string starts with prefix.
 */
inline bool startswith(const string& s, const string& value) {
	if (s.size() < value.size()) return false;
	return s.compare(0, value.size(), value) == 0;
}

inline string upper(const string& s) {
	string res = s;
	std::transform(res.begin(), res.end(), res.begin(),
		[](unsigned char c) { return std::toupper(c); });
	return res;
}
struct Ranges {
	int start{ 0 }, end{ 0 }, step{ 1 };
};

/**
 * Generates a range of integers.
 * Optimization: Reserves vector size immediately.
 */
inline vector<int> range(Ranges ra) {
	if (ra.step == 0) return {};

	// Calculate expected size to reserve memory
	int count = 0;
	if (ra.step > 0 && ra.end > ra.start)
		count = (ra.end - ra.start + ra.step - 1) / ra.step;
	else if (ra.step < 0 && ra.end < ra.start)
		count = (ra.start - ra.end - ra.step - 1) / (-ra.step);

	if (count <= 0) return {};

	vector<int> r;
	r.reserve(count);

	for (int i = 0; i < count; ++i) {
		r.push_back(ra.start + i * ra.step);
	}
	return r;
}