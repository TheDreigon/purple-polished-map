// Runs the tests: purple-polished-map-tests [--root DIR] [--list] [filter...]
//
// --root DIR  the repository folder (default: found by walking up from the current folder)
// --list      print the test names and exit
// filter      run only the tests whose name contains one of the given words

#include <algorithm>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "config.h"
#include "test.h"

namespace fs = std::filesystem;

namespace test {

static fs::path root_dir, work_dir;
static const char *current_name = NULL;
static int current_failures = 0, current_temp_dirs = 0;

std::vector<Case> &cases() {
	static std::vector<Case> all;
	return all;
}

int register_case(const char *name, void (*run)(void)) {
	cases().push_back({name, run});
	return 0;
}

void fail(const char *file, int line, const std::string &message) {
	if (!current_failures) { printf("FAILED\n"); }
	current_failures++;
	printf("    %s:%d: %s\n", fs::path(file).filename().string().c_str(), line, message.c_str());
}

std::string show_text(const std::string &s) {
	std::string r("\"");
	for (unsigned char c : s) {
		switch (c) {
		case '\n': r += "\\n"; break;
		case '\r': r += "\\r"; break;
		case '\t': r += "\\t"; break;
		case '"': r += "\\\""; break;
		case '\\': r += "\\\\"; break;
		default:
			if (c < 0x20 || c == 0x7f) {
				char hex[8];
				snprintf(hex, sizeof(hex), "\\x%02x", c);
				r += hex;
			}
			else {
				r += (char)c;
			}
		}
	}
	return r + "\"";
}

std::string join(const std::string &dir, const std::string &relative) {
	fs::path p = fs::path(dir) / fs::path(relative);
	return p.make_preferred().string();
}

std::string as_project_dir(const std::string &dir) {
	std::string d = fs::path(dir).make_preferred().string();
	if (d.empty() || d.back() != fs::path::preferred_separator) {
		d += (char)fs::path::preferred_separator;
	}
	return d;
}

std::string example_path(const std::string &relative) {
	return join((root_dir / "example").string(), relative);
}

std::string data_path(const std::string &relative) {
	return join((root_dir / "tests" / "data").string(), relative);
}

std::string temp_dir() {
	fs::path dir = work_dir / (std::string(current_name) + "-" + std::to_string(++current_temp_dirs));
	fs::create_directories(dir);
	return dir.string();
}

std::string read_file(const std::string &path) {
	std::ifstream ifs(fs::path(path), std::ios::binary);
	if (!ifs.good()) {
		fail(__FILE__, __LINE__, "cannot read " + path);
		throw Abandon();
	}
	return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

void write_file(const std::string &path, const std::string &bytes) {
	fs::create_directories(fs::path(path).parent_path());
	std::ofstream ofs(fs::path(path), std::ios::binary);
	ofs.write(bytes.data(), (std::streamsize)bytes.size());
	if (!ofs.good()) {
		fail(__FILE__, __LINE__, "cannot write " + path);
		throw Abandon();
	}
}

void copy_file(const std::string &from, const std::string &to) {
	write_file(to, read_file(from));
}

void make_dirs(const std::string &path) {
	fs::create_directories(path);
}

bool exists(const std::string &path) {
	return fs::exists(path);
}

// The program keeps some settings in global variables; every test starts from the defaults
static void reset_global_state() {
	Config::monochrome(false);
	Config::allow_priority(false);
	Config::allow_256_tiles(false);
	Config::drag_and_drop(true);
	Config::print_grid(false);
	Config::print_ids(false);
	Config::print_priority(false);
	Config::print_events(false);
	Config::print_warp_ids(false);
	Config::overworld_map_size(1300);
}

static bool is_root(const fs::path &dir) {
	return fs::exists(dir / "example" / "Makefile") && fs::exists(dir / "tests" / "data");
}

static bool find_root(const char *given) {
	if (given) {
		root_dir = fs::absolute(given);
		return is_root(root_dir);
	}
	for (fs::path dir = fs::current_path(); ; dir = dir.parent_path()) {
		if (is_root(dir)) {
			root_dir = dir;
			return true;
		}
		if (dir == dir.parent_path()) { return false; }
	}
}

static bool run_case(const Case &c) {
	current_name = c.name;
	current_failures = 0;
	current_temp_dirs = 0;
	reset_global_state();
	printf("%s ... ", c.name);
	fflush(stdout);
	try {
		c.run();
	}
	catch (const Abandon &) {}
	catch (const std::exception &e) {
		fail(__FILE__, __LINE__, std::string("unexpected exception: ") + e.what());
	}
	if (!current_failures) { printf("ok\n"); }
	fflush(stdout);
	return !current_failures;
}

}

int main(int argc, char **argv) {
	const char *given_root = NULL;
	bool list = false;
	std::vector<std::string> filters;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--root") && i + 1 < argc) { given_root = argv[++i]; }
		else if (!strcmp(argv[i], "--list")) { list = true; }
		else { filters.push_back(argv[i]); }
	}

	// The order in which the tests register themselves is not defined, so sort them
	std::sort(test::cases().begin(), test::cases().end(), [](const test::Case &a, const test::Case &b) {
		return strcmp(a.name, b.name) < 0;
	});

	if (list) {
		for (const test::Case &c : test::cases()) { printf("%s\n", c.name); }
		return 0;
	}

	if (!test::find_root(given_root)) {
		printf("Cannot find the repository folder (the one with example/ and tests/).\n"
			"Run the tests from inside it, or pass --root DIR.\n");
		return 2;
	}
	test::root_dir = test::root_dir.lexically_normal();
	if (!test::root_dir.has_filename()) { test::root_dir = test::root_dir.parent_path(); }
	test::work_dir = test::root_dir / "tmp" / "tests-work";
	std::error_code ignored;
	fs::remove_all(test::work_dir, ignored);
	fs::create_directories(test::work_dir);

	int ran = 0, failed = 0;
	for (const test::Case &c : test::cases()) {
		bool wanted = filters.empty();
		for (const std::string &f : filters) {
			if (strstr(c.name, f.c_str())) { wanted = true; }
		}
		if (!wanted) { continue; }
		ran++;
		if (!test::run_case(c)) { failed++; }
	}

	printf("\n%d tests, %d passed, %d failed\n", ran, ran - failed, failed);
	return failed || !ran ? 1 : 0;
}
