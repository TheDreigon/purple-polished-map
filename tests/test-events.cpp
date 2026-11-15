// Reading and writing the events of a map's .asm file (src/map-events.cpp, src/event.cpp)
//
// tests/data/TestHouse.asm uses pokecrystal's macros, TestTown.asm uses pokered's
// and OldTown.asm uses the ones pokered had before 2020.

#pragma warning(push, 0)
#include <FL/filename.H>
#pragma warning(pop)

#include "map-events.h"
#include "test.h"

// Copies a data file to a new folder, so the original is never written to
static std::string working_copy(const char *name) {
	std::string f = test::join(test::temp_dir(), name);
	test::copy_file(test::data_path(name), f);
	return f;
}

static void replace(std::string &text, const std::string &from, const std::string &to) {
	size_t at = text.find(from);
	REQUIRE(at != std::string::npos);
	text.replace(at, from.size(), to);
}

TEST(events_reads_pokecrystal_events) {
	std::string f = working_copy("TestHouse.asm");
	Map_Events events;
	REQUIRE(events.read_events(f.c_str()) == Map_Events::Result::MAP_EVENTS_OK);

	CHECK(events.loaded());
	CHECK(!events.modified());
	// "warp TEST_CAVE, 3, 4" is a script command, not an event
	REQUIRE(events.size() == 7);

	CHECK_EQ(events.event(0)->tip(), "warp_event  2,  7, TEST_TOWN, 1");
	CHECK_EQ(events.event(0)->event_x(), 2);
	CHECK_EQ(events.event(0)->event_y(), 7);
	CHECK_EQ(events.event(2)->event_x(), 9);
	CHECK_EQ(events.event(2)->event_y(), 0);
	CHECK_EQ(events.event(3)->tip(), "coord_event  5,  4, SCENE_TESTHOUSE_NOOP, TestHouseLadderScript");
	CHECK_EQ(events.event(3)->event_x(), 5);
	CHECK_EQ(events.event(3)->event_y(), 4);
	CHECK_EQ(events.event(4)->event_x(), 0);
	CHECK_EQ(events.event(4)->event_y(), 1);
	// Hexadecimal coordinates
	CHECK_EQ(events.event(5)->event_x(), 10);
	CHECK_EQ(events.event(5)->event_y(), 1);
	CHECK_EQ(events.event(6)->event_x(), 7);
	CHECK_EQ(events.event(6)->event_y(), 4);
}

TEST(events_knows_where_warps_lead) {
	std::string f = working_copy("TestHouse.asm");
	Map_Events events;
	REQUIRE(events.read_events(f.c_str()) == Map_Events::Result::MAP_EVENTS_OK);
	REQUIRE(events.size() == 7);

	// Map constants are turned into map names: TEST_TOWN is TestTown
	CHECK_EQ(events.event(0)->warp_destination().first, "TestTown");
	CHECK_EQ(events.event(0)->warp_destination().second, 1);
	CHECK_EQ(events.event(2)->warp_destination().first, "TestHouse");
	CHECK_EQ(events.event(2)->warp_destination().second, 1);
	// Only warps have a destination
	CHECK_EQ(events.event(4)->warp_destination().first, "");

	// The third warp leads to the first warp of this same map
	CHECK(events.event(2)->warp_to() == events.event(0));
	CHECK(events.event(0)->warp_to() == NULL);
	CHECK(events.event(4)->warp_to() == NULL);
}

TEST(events_writes_back_the_same_file) {
	for (const char *name : {"TestHouse.asm", "TestTown.asm", "OldTown.asm"}) {
		std::string f = working_copy(name);
		std::string original = test::read_file(f);
		Map_Events events;
		REQUIRE(events.read_events(f.c_str()) == Map_Events::Result::MAP_EVENTS_OK);
		CHECK(events.write_event_script(f.c_str()));
		CHECK_EQ(test::read_file(f), original);
	}
}

TEST(events_writes_a_moved_event) {
	std::string f = working_copy("TestHouse.asm");
	std::string expected = test::read_file(f);
	Map_Events events;
	REQUIRE(events.read_events(f.c_str()) == Map_Events::Result::MAP_EVENTS_OK);
	REQUIRE(events.size() == 7);

	events.event(0)->coords(4, 6);
	events.event(0)->update_tooltip();
	events.event(5)->coords(11, 2);
	events.event(5)->update_tooltip();
	CHECK(events.write_event_script(f.c_str()));

	// Only the two moved events change; hexadecimal coordinates stay hexadecimal
	replace(expected, "warp_event  2,  7, TEST_TOWN, 1", "warp_event  4,  6, TEST_TOWN, 1");
	replace(expected, "bg_event $a, $1, BGEVENT_READ", "bg_event $b, $2, BGEVENT_READ");
	CHECK_EQ(test::read_file(f), expected);
}

TEST(events_reads_pokered_events) {
	std::string f = working_copy("TestTown.asm");
	Map_Events events;
	REQUIRE(events.read_events(f.c_str()) == Map_Events::Result::MAP_EVENTS_OK);
	REQUIRE(events.size() == 4);

	CHECK_EQ(events.event(0)->event_x(), 5);
	CHECK_EQ(events.event(0)->event_y(), 5);
	CHECK_EQ(events.event(1)->event_x(), 13);
	CHECK_EQ(events.event(1)->event_y(), 5);
	CHECK_EQ(events.event(1)->warp_destination().first, "TestLab2F");
	CHECK_EQ(events.event(1)->warp_destination().second, 2);
	CHECK_EQ(events.event(2)->tip(), "bg_event  3,  5, TEXT_TESTTOWN_SIGN");
	CHECK_EQ(events.event(2)->event_x(), 3);
	CHECK_EQ(events.event(3)->tip(), "object_event  8,  5, SPRITE_GIRL, WALK, ANY_DIR, TEXT_TESTTOWN_GIRL");
	CHECK_EQ(events.event(3)->event_x(), 8);
	CHECK_EQ(events.event(3)->event_y(), 5);
}

TEST(events_reads_old_pokered_events) {
	std::string f = working_copy("OldTown.asm");
	Map_Events events;
	REQUIRE(events.read_events(f.c_str()) == Map_Events::Result::MAP_EVENTS_OK);
	REQUIRE(events.size() == 6);

	// warp x, y, destination warp, destination map
	CHECK_EQ(events.event(1)->event_x(), 13);
	CHECK_EQ(events.event(1)->event_y(), 5);
	CHECK_EQ(events.event(1)->warp_destination().first, "OldLab");
	CHECK_EQ(events.event(1)->warp_destination().second, 1);
	// sign x, y, text
	CHECK_EQ(events.event(2)->event_x(), 3);
	CHECK_EQ(events.event(2)->event_y(), 5);
	// object sprite, x, y, ...: the coordinates come after the sprite
	CHECK_EQ(events.event(3)->event_x(), 8);
	CHECK_EQ(events.event(3)->event_y(), 5);
	// warp_to x, y, map width
	CHECK_EQ(events.event(5)->event_x(), 13);
	CHECK_EQ(events.event(5)->event_y(), 5);
}

TEST(events_reads_a_line_with_only_the_word_warp) {
	// Such a line used to throw std::out_of_range and close the program when the map was opened
	std::string f = test::join(test::temp_dir(), "BareWarp.asm");
	std::string text =
		"BareWarp_Script:\n"
		"\twarp\n"
		"\twarp_event  2,  7, TEST_TOWN, 1\n";
	test::write_file(f, text);

	Map_Events events;
	REQUIRE(events.read_events(f.c_str()) == Map_Events::Result::MAP_EVENTS_OK);
	CHECK_EQ(events.size(), (size_t)1);
	CHECK(events.write_event_script(f.c_str()));
	CHECK_EQ(test::read_file(f), text);
}

TEST(events_reports_a_missing_file) {
	std::string missing = test::join(test::temp_dir(), "Missing.asm");
	Map_Events events;
	CHECK(events.read_events(missing.c_str()) == Map_Events::Result::BAD_MAP_EVENTS_FILE);
	CHECK(!events.loaded());
	CHECK_EQ(events.size(), (size_t)0);
}

TEST(events_notices_a_change_by_another_program) {
	std::string f = working_copy("TestHouse.asm");
	Map_Events events;
	REQUIRE(events.read_events(f.c_str()) == Map_Events::Result::MAP_EVENTS_OK);
	CHECK(!events.other_modified(f.c_str()));

	test::make_newer(f);
	CHECK(events.other_modified(f.c_str()));

	// Once the user has agreed to overwrite the file, it is not asked about again
	events.accept_other_modified(f.c_str());
	CHECK(!events.other_modified(f.c_str()));
}

TEST(events_keeps_a_file_without_events_unchanged) {
	std::string f = test::join(test::temp_dir(), "NoEvents.asm");
	std::string text =
		"NoEvents_MapScripts:\n"
		"\tdef_scene_scripts\n"
		"\n"
		"NoEvents_MapEvents:\n"
		"\tdb 0, 0 ; filler\n";
	test::write_file(f, text);

	Map_Events events;
	REQUIRE(events.read_events(f.c_str()) == Map_Events::Result::MAP_EVENTS_OK);
	CHECK_EQ(events.size(), (size_t)0);
	CHECK(events.write_event_script(f.c_str()));
	CHECK_EQ(test::read_file(f), text);
}
