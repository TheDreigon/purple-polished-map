// Reading and writing .blk map files, and undo/redo (src/map.cpp)

#include "map.h"
#include "test.h"

static std::string ids_of(const Map &map) {
	std::string ids;
	for (size_t i = 0; i < map.size(); i++) {
		ids += (char)map.block(i)->id();
	}
	return ids;
}

// Loads a small map whose file holds the given bytes
static std::string load_small_map(Map &map, uint8_t w, uint8_t h, const std::string &bytes) {
	std::string f = test::join(test::temp_dir(), "Small.blk");
	test::write_file(f, bytes);
	map.size(w, h);
	REQUIRE(map.read_blocks(f.c_str()) == Map::Result::MAP_OK);
	return f;
}

TEST(map_reads_a_blk_file_row_by_row) {
	Map map;
	load_small_map(map, 3, 2, "\x01\x02\x03\x04\x05\x06");

	CHECK_EQ(map.width(), 3);
	CHECK_EQ(map.height(), 2);
	CHECK_EQ(map.size(), (size_t)6);
	CHECK_EQ(map.block((uint8_t)0, (uint8_t)0)->id(), 1);
	CHECK_EQ(map.block((uint8_t)2, (uint8_t)0)->id(), 3);
	CHECK_EQ(map.block((uint8_t)0, (uint8_t)1)->id(), 4);
	CHECK_EQ(map.block((uint8_t)2, (uint8_t)1)->id(), 6);
	CHECK_EQ(map.block((uint8_t)2, (uint8_t)1)->col(), 2);
	CHECK_EQ(map.block((uint8_t)2, (uint8_t)1)->row(), 1);
	CHECK(!map.modified());
	CHECK(!map.can_undo());
	CHECK(!map.can_redo());
}

TEST(map_reports_a_file_of_the_wrong_size) {
	std::string dir = test::temp_dir();

	std::string too_short = test::join(dir, "Short.blk");
	test::write_file(too_short, "\x01\x02\x03\x04\x05");
	Map short_map;
	short_map.size(3, 2);
	CHECK(short_map.read_blocks(too_short.c_str()) == Map::Result::MAP_TOO_SHORT);

	// A file that is too long still loads; the extra bytes are left out
	std::string too_long = test::join(dir, "Long.blk");
	test::write_file(too_long, "\x01\x02\x03\x04\x05\x06\x07");
	Map long_map;
	long_map.size(3, 2);
	CHECK(long_map.read_blocks(too_long.c_str()) == Map::Result::MAP_TOO_LONG);
	CHECK_EQ(ids_of(long_map), "\x01\x02\x03\x04\x05\x06");

	std::string missing = test::join(dir, "Missing.blk");
	Map missing_map;
	missing_map.size(3, 2);
	CHECK(missing_map.read_blocks(missing.c_str()) == Map::Result::MAP_BAD_FILE);
}

TEST(map_reads_block_ids_from_an_asm_map_file) {
	std::string f = test::join(test::temp_dir(), "Small.map");
	test::write_file(f,
		"\tdb $01, 2, %11 ; first row\n"
		"\tdb 4, 5, 6\n");
	Map map;
	map.size(3, 2);
	CHECK(map.read_blocks(f.c_str()) == Map::Result::MAP_OK);
	CHECK_EQ(ids_of(map), "\x01\x02\x03\x04\x05\x06");
}

TEST(map_writes_back_the_same_bytes) {
	std::string original = test::example_path("maps/Kanto.180x135.kanto.blk");
	std::string saved = test::join(test::temp_dir(), "Kanto.blk");

	Map map;
	map.size(180, 135);
	REQUIRE(map.read_blocks(original.c_str()) == Map::Result::MAP_OK);
	CHECK(map.write_blocks(saved.c_str()));
	CHECK(test::read_file(saved) == test::read_file(original));
}

TEST(map_writes_an_edited_block) {
	Map map;
	std::string f = load_small_map(map, 3, 2, "\x01\x02\x03\x04\x05\x06");

	map.block((uint8_t)1, (uint8_t)1)->id(0x7f);
	CHECK(map.write_blocks(f.c_str()));
	CHECK_EQ(test::read_file(f), "\x01\x02\x03\x04\x7f\x06");
}

TEST(map_does_not_see_its_own_save_as_an_outside_change) {
	Map map;
	std::string f = load_small_map(map, 3, 2, "\x01\x02\x03\x04\x05\x06");

	CHECK(!map.other_modified(f.c_str()));
	CHECK(map.write_blocks(f.c_str()));
	CHECK(!map.other_modified(f.c_str()));
}

TEST(map_undoes_and_redoes_edits) {
	Map map;
	load_small_map(map, 2, 2, "\x01\x02\x03\x04");

	map.remember();
	map.block((size_t)0)->id(9);
	map.remember();
	map.block((size_t)3)->id(8);
	CHECK_EQ(ids_of(map), "\x09\x02\x03\x08");
	CHECK(map.can_undo());
	CHECK(!map.can_redo());

	map.undo();
	CHECK_EQ(ids_of(map), "\x09\x02\x03\x04");
	map.undo();
	CHECK_EQ(ids_of(map), "\x01\x02\x03\x04");
	CHECK(!map.can_undo());
	CHECK(map.can_redo());

	map.redo();
	CHECK_EQ(ids_of(map), "\x09\x02\x03\x04");
	map.redo();
	CHECK_EQ(ids_of(map), "\x09\x02\x03\x08");
	CHECK(!map.can_redo());

	// A new edit after an undo throws away what could have been redone
	map.undo();
	map.remember();
	map.block((size_t)1)->id(7);
	CHECK(!map.can_redo());
	CHECK_EQ(ids_of(map), "\x09\x07\x03\x04");
}

TEST(map_remembers_at_most_100_edits) {
	Map map;
	load_small_map(map, 2, 2, std::string(4, '\0'));

	for (int i = 1; i <= MAX_HISTORY_SIZE + 5; i++) {
		map.remember();
		map.block((size_t)0)->id((uint8_t)i);
	}
	int undone = 0;
	while (map.can_undo()) {
		map.undo();
		undone++;
	}
	CHECK_EQ(undone, MAX_HISTORY_SIZE);
	// The oldest five edits can no longer be undone
	CHECK_EQ(map.block((size_t)0)->id(), 5);
}

// What the main window does on File > Save: the file is only written if the map was modified
static void save_like_the_window(Map &map, const std::string &f) {
	if (map.modified()) {
		REQUIRE(map.write_blocks(f.c_str()));
		map.modified(false);
	}
}

TEST(map_saves_an_undo_made_after_a_save) {
	// The bug: edit, save, undo, save again. The second save said "Saved!" but wrote nothing,
	// because Undo did not count as a modification, so the file kept the edit that had been undone.
	Map map;
	std::string f = load_small_map(map, 2, 2, "\x01\x02\x03\x04");

	map.remember();
	map.block((size_t)0)->id(9);
	map.modified(true);
	save_like_the_window(map, f);
	CHECK_EQ(test::read_file(f), "\x09\x02\x03\x04");

	map.undo();
	CHECK(map.modified());
	save_like_the_window(map, f);
	CHECK_EQ(test::read_file(f), "\x01\x02\x03\x04");

	// The same goes for Redo
	map.redo();
	CHECK(map.modified());
	save_like_the_window(map, f);
	CHECK_EQ(test::read_file(f), "\x09\x02\x03\x04");
}

TEST(map_stays_unmodified_when_there_is_nothing_to_undo) {
	Map map;
	load_small_map(map, 2, 2, "\x01\x02\x03\x04");

	map.undo();
	map.redo();
	CHECK(!map.modified());
}

TEST(map_clear_forgets_everything) {
	Map map;
	load_small_map(map, 2, 2, "\x01\x02\x03\x04");
	map.remember();
	map.modified(true);

	map.clear();
	CHECK_EQ(map.width(), 0);
	CHECK_EQ(map.height(), 0);
	CHECK(map.result() == Map::Result::MAP_NULL);
	CHECK(!map.modified());
	CHECK(!map.can_undo());
}
