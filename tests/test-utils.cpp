// Text and file helpers (src/utils.cpp) and the db/dw reader (src/parse-asm.cpp)

#pragma warning(push, 0)
#include <FL/filename.H>
#pragma warning(pop)

#include "utils.h" // before parse-asm.h, which needs its integer types
#include "parse-asm.h"
#include "test.h"

TEST(utils_compares_text) {
	CHECK(starts_with("warp_event", "warp"));
	CHECK(!starts_with("wa", "warp"));
	CHECK(ends_with("Route1.blk", ".blk"));
	CHECK(!ends_with("Route1.BLK", ".blk"));
	CHECK(!ends_with("lk", ".blk"));
	CHECK(ends_with_ignore_case("Route1.BLK", ".blk"));
	CHECK(!ends_with_ignore_case("lk", ".blk"));
	CHECK(equals_ignore_case("DB", "db"));
	CHECK(!equals_ignore_case("db", "dw"));
	CHECK(!equals_ignore_case("db", "dbx"));
}

TEST(utils_trims_and_lowercases) {
	std::string s(" \t tilepal 0 \r\n");
	trim(s);
	CHECK_EQ(s, "tilepal 0");

	std::string blank(" \t\r\n");
	trim(blank);
	CHECK_EQ(blank, "");

	std::string mixed("New_Bark_TOWN");
	lowercase(mixed);
	CHECK_EQ(mixed, "new_bark_town");
}

TEST(utils_removes_comments) {
	std::string with("\tdb 1, 2 ; two bytes");
	remove_comment(with);
	CHECK_EQ(with, "\tdb 1, 2 ");

	std::string without("\tdb 1, 2");
	remove_comment(without);
	CHECK_EQ(without, "\tdb 1, 2");
}

TEST(utils_recognises_an_indented_macro) {
	std::string macro;

	std::istringstream indented("\twarp_event  6,  7, NEW_BARK_TOWN, 2");
	CHECK(leading_macro(indented, macro));
	CHECK_EQ(macro, "warp_event");

	std::istringstream spaces("  db 1");
	CHECK(leading_macro(spaces, macro));
	CHECK_EQ(macro, "db");

	std::istringstream label("Label: db 1");
	CHECK(!leading_macro(label, macro));

	std::istringstream wanted("\ttilecoll WALL, WALL, WALL, WALL");
	CHECK(leading_macro(wanted, macro, "tilecoll"));

	std::istringstream other("\ttilepal 0, GRAY");
	CHECK(!leading_macro(other, macro, "tilecoll"));
}

TEST(utils_splits_a_tileset_name_with_suffixes) {
	// A tileset name can be "name.before,after": extra graphics placed before and after the main ones
	char s[FL_PATH_MAX] = {};

	remove_suffix("johto.common,extra", s);
	CHECK_EQ(s, "johto");
	before_suffix("johto.common,extra", s);
	CHECK_EQ(s, "common");
	after_suffix("johto.common,extra", s);
	CHECK_EQ(s, "extra");

	remove_suffix("kanto", s);
	CHECK_EQ(s, "kanto");
	before_suffix("kanto", s);
	CHECK_EQ(s, "");
	after_suffix("kanto", s);
	CHECK_EQ(s, "");

	before_suffix("johto.common", s);
	CHECK_EQ(s, "common");
	after_suffix("johto.common", s);
	CHECK_EQ(s, "");
}

TEST(utils_handles_file_extensions) {
	char s[FL_PATH_MAX] = {};

	remove_dot_ext(test::join("maps", "Route1.blk").c_str(), s);
	CHECK_EQ(s, "Route1");
	// Everything after the first dot goes, which is how "Kanto.180x135.kanto.blk" names its map
	remove_dot_ext(test::join("maps", "Kanto.180x135.kanto.blk").c_str(), s);
	CHECK_EQ(s, "Kanto");

	add_dot_ext("Route1", ".blk", s);
	CHECK_EQ(s, "Route1.blk");
	add_dot_ext("Route1.BLK", ".blk", s);
	CHECK_EQ(s, "Route1.BLK");
}

TEST(utils_reports_files) {
	std::string dir = test::temp_dir();
	std::string there = test::join(dir, "there.bin"), missing = test::join(dir, "missing.bin");
	test::write_file(there, std::string(300, 'x'));

	CHECK(file_exists(there.c_str()));
	CHECK(!file_exists(missing.c_str()));
	CHECK_EQ(file_size(there.c_str()), (size_t)300);
	CHECK_EQ(file_size(missing.c_str()), (size_t)0);
	CHECK(file_modified(there.c_str()) > 0);
	CHECK_EQ(file_modified(missing.c_str()), 0);
	CHECK_EQ(file_modified(NULL), 0);
}

static std::vector<uint8_t> bytes_of(const Parsed_Asm &data) {
	std::vector<uint8_t> bytes;
	for (size_t i = 0; i < data.size(); i++) {
		bytes.push_back(data.get(i));
	}
	return bytes;
}

TEST(parse_asm_reads_db_and_dw_values) {
	std::string f = test::join(test::temp_dir(), "data.asm");
	test::write_file(f,
		"Label:\n"
		"\tdb 1, $2A, %101, 10h ; decimal, hex, binary, hex again\n"
		"\tdw $1234, 5 ; words are stored low byte first\n"
		"  db 7\n"
		"NotIndented db 9\n"
		"\tDB 8\n"
		"\tsomething_else 3\n");

	Parsed_Asm data(f.c_str());
	CHECK(data.result() == Parsed_Asm::Result::ASM_OK);
	std::vector<uint8_t> expected = {1, 0x2A, 5, 0x10, 0x34, 0x12, 5, 0, 7, 8};
	CHECK(bytes_of(data) == expected);
	CHECK_EQ(data.size(), expected.size());
}

TEST(parse_asm_rejects_values_that_do_not_fit) {
	std::string dir = test::temp_dir();

	std::string big_byte = test::join(dir, "byte.asm");
	test::write_file(big_byte, "\tdb 255, 256\n");
	CHECK(Parsed_Asm(big_byte.c_str()).result() == Parsed_Asm::Result::ASM_OVERFLOW);

	std::string big_word = test::join(dir, "word.asm");
	test::write_file(big_word, "\tdw $ffff, $10000\n");
	CHECK(Parsed_Asm(big_word.c_str()).result() == Parsed_Asm::Result::ASM_OVERFLOW);

	std::string missing = test::join(dir, "missing.asm");
	CHECK(Parsed_Asm(missing.c_str()).result() == Parsed_Asm::Result::ASM_BAD_FILE);
}
