#ifndef TEST_H
#define TEST_H

// A very small test framework, so the tests need nothing but the compiler.
//
// TEST(name) { ... } defines a test; it is found and run automatically.
// CHECK(condition) and CHECK_EQ(actual, expected) record a failure and carry on.
// REQUIRE(condition) records a failure and abandons the current test, for when
// the rest of the test would make no sense (or would crash).

#include <cstdio>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace test {

struct Abandon {}; // thrown by REQUIRE

struct Case {
	const char *name;
	void (*run)(void);
};

std::vector<Case> &cases(void);
int register_case(const char *name, void (*run)(void));
void fail(const char *file, int line, const std::string &message);

std::string show_text(const std::string &s);

inline std::string show(const std::string &v) { return show_text(v); }
inline std::string show(const char *v) { return v ? show_text(v) : "NULL"; }
inline std::string show(char *v) { return v ? show_text(v) : "NULL"; }
inline std::string show(bool v) { return v ? "true" : "false"; }

template <typename T> std::string show(const T &v) {
	if constexpr (std::is_enum_v<T>) {
		return std::to_string((long long)v);
	}
	else if constexpr (std::is_integral_v<T>) {
		return std::to_string(v); // shows uint8_t as a number, not as a character
	}
	else {
		std::ostringstream ss;
		ss << v;
		return ss.str();
	}
}

// C strings and char buffers are compared as text, not as addresses
inline std::string value(const char *v) { return v ? v : "(null)"; }
template <typename T> const T &value(const T &v) { return v; }

template <typename A, typename E>
void check_eq(const char *file, int line, const char *actual_text, const char *expected_text, const A &actual, const E &expected) {
	if (!(actual == expected)) {
		fail(file, line, std::string("CHECK_EQ(") + actual_text + ", " + expected_text + ")\n"
			"      actual:   " + show(actual) + "\n"
			"      expected: " + show(expected));
	}
}

// The repository root, the example project and the test data, with native separators
std::string join(const std::string &dir, const std::string &relative);
std::string as_project_dir(const std::string &dir); // with the trailing separator the program expects
std::string example_path(const std::string &relative);
std::string data_path(const std::string &relative);

// A new, empty directory that belongs to the running test
std::string temp_dir(void);

std::string read_file(const std::string &path);
void write_file(const std::string &path, const std::string &bytes);
void copy_file(const std::string &from, const std::string &to);
void make_dirs(const std::string &path);
bool exists(const std::string &path);

// Makes a file look as if another program had changed it after it was loaded
void make_newer(const std::string &path);

}

#define TEST(name) \
	static void name(void); \
	static int registered_##name = test::register_case(#name, name); \
	static void name(void)

#define CHECK(condition) \
	do { if (!(condition)) { test::fail(__FILE__, __LINE__, "CHECK(" #condition ")"); } } while (0)

#define CHECK_EQ(actual, expected) \
	test::check_eq(__FILE__, __LINE__, #actual, #expected, test::value(actual), test::value(expected))

#define REQUIRE(condition) \
	do { if (!(condition)) { test::fail(__FILE__, __LINE__, "REQUIRE(" #condition ")"); throw test::Abandon(); } } while (0)

#endif
