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

TEST(map_explains_a_file_of_the_wrong_size) {
	// Shown with "File ends too early" or "larger than the specified size" when opening a map
	std::string hint = Map::size_hint(20, 18, 350);
	CHECK_EQ(hint,
		"The map is 20 x 18 (360 blocks),\n"
		"but the file has 350 bytes.\n"
		"\n"
		"Sizes that fit 350 bytes: 25 x 14, 14 x 25, 10 x 35, 7 x 50"); // closest widths to 20 first

	// A size that cannot be any map
	CHECK_EQ(Map::size_hint(10, 9, 0), "The map is 10 x 9 (90 blocks),\nbut the file has 0 bytes.");
	// 509 is prime and above 255, so no width and height up to 255 fit it
	CHECK_EQ(Map::size_hint(10, 9, 509), "The map is 10 x 9 (90 blocks),\nbut the file has 509 bytes.");
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

TEST(map_notices_a_change_by_another_program) {
	Map map;
	std::string f = load_small_map(map, 2, 2, "\x01\x02\x03\x04");
	CHECK(!map.other_modified(f.c_str()));

	test::make_newer(f);
	CHECK(map.other_modified(f.c_str()));

	// Once the user has agreed to overwrite the file, it is not asked about again
	map.accept_other_modified(f.c_str());
	CHECK(!map.other_modified(f.c_str()));
}

// What the main window does on Edit > Resize Map with the top left corner anchored:
// the blocks that still fit are kept and the new places get block 0
static void resize_like_the_window(Map &map, uint8_t w, uint8_t h) {
	uint8_t old_w = map.width(), old_h = map.height();
	std::vector<Block *> old;
	for (size_t i = 0; i < map.size(); i++) {
		old.push_back(map.block(i));
	}
	map.resize(w, h);
	for (uint8_t y = 0; y < h; y++) {
		for (uint8_t x = 0; x < w; x++) {
			Block *b = (x < old_w && y < old_h) ? old[(size_t)y * old_w + x] : new Block();
			b->coords(y, x);
			map.block(x, y, b);
		}
	}
	map.modified(true);
}

TEST(map_resized_is_still_the_same_file) {
	// The bug: after Resize Map, the first Save always asked "... was modified by another program!
	// Save the map and overwrite it anyway?", although nothing else had touched the file
	Map map;
	std::string f = load_small_map(map, 2, 2, "\x01\x02\x03\x04");

	resize_like_the_window(map, 3, 2);
	CHECK_EQ(map.width(), 3);
	CHECK_EQ(map.height(), 2);
	CHECK(map.modified());
	CHECK(map.result() == Map::Result::MAP_OK);
	CHECK(!map.other_modified(f.c_str()));

	// A real change by another program is still noticed
	test::make_newer(f);
	CHECK(map.other_modified(f.c_str()));

	CHECK(map.write_blocks(f.c_str()));
	CHECK_EQ(test::read_file(f), std::string("\x01\x02\x00\x03\x04\x00", 6));
}

TEST(map_resized_keeps_its_attributes) {
	// Resizing also used to forget the map's group, environment, landmark and palette,
	// which decide the colors a pokecrystal map is shown with
	Map map;
	load_small_map(map, 2, 2, "\x01\x02\x03\x04");
	Map_Attributes attributes;
	attributes.group = 24;
	attributes.environment = "TOWN";
	attributes.landmark = "NEW_BARK_TOWN";
	attributes.palette = "PALETTE_AUTO";
	map.attributes(attributes);

	resize_like_the_window(map, 2, 3);
	CHECK_EQ(map.group(), 24);
	CHECK_EQ(map.environment(), "TOWN");
	CHECK_EQ(map.landmark(), "NEW_BARK_TOWN");
	CHECK_EQ(map.palette(), "PALETTE_AUTO");
	CHECK(map.is_outside());
}

TEST(map_resized_cannot_undo_to_the_old_size) {
	Map map;
	load_small_map(map, 2, 2, "\x01\x02\x03\x04");
	map.remember();
	map.block((size_t)0)->id(9);

	// The remembered edits are for a map of another size, so they are dropped
	resize_like_the_window(map, 3, 3);
	CHECK(!map.can_undo());
	CHECK(!map.can_redo());
}

TEST(map_tells_which_blocks_it_uses) {
	// For View > Highlight Unused (upstream issue 106)
	Map map;
	load_small_map(map, 2, 2, "\x01\x02\x02\x07");
	CHECK(map.uses_block(1));
	CHECK(map.uses_block(2));
	CHECK(map.uses_block(7));
	CHECK(!map.uses_block(0));
	CHECK(!map.uses_block(3));

	map.block((size_t)0)->id(3);
	CHECK(map.uses_block(3));
	CHECK(!map.uses_block(1));
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
