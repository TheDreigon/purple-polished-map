// Reading pokered's and pokeyellow's hidden objects (src/hidden-objects.cpp)

#include "hidden-objects.h"
#include "test.h"

static const char *HIDDEN_OBJECTS =
	"HiddenObjectMaps:\n"
	"\tdbw REDS_HOUSE_2F,                SilphCo11FHiddenObjects2\n"
	"\tdbw VIRIDIAN_FOREST,              ViridianForestHiddenObjects\n"
	"\tdbw ROUTE_25,                     Route25HiddenObjects\n"
	"\tdb -1 ; end\n"
	"\n"
	"MACRO hidden_object\n"
	"\tdb \\2, \\1, \\3\n"
	"ENDM\n"
	"\n"
	"SilphCo11FHiddenObjects2:\n"
	"\thidden_object  0,  1, SPRITE_FACING_UP, OpenRedsPC\n"
	"\thidden_object  3,  5, ANY_FACING, PrintRedSNESText\n"
	"\tdb -1 ; end\n"
	"\n"
	"ViridianForestHiddenObjects:\n"
	"\thidden_object  1, 18, LEAF_STONE, HiddenItems\n"
	"\thidden_object 16, 42, POTION, HiddenItems\n"
	"\thidden_object  8,  4, POTION, HiddenItems ; hidden one square down\n"
	"\tdb -1 ; end\n"
	"\n"
	"; Comments between lists are ignored\n"
	"Route25HiddenObjects:\n"
	"\thidden_object 60,  9, ETHER, HiddenItems\n"
	"\thidden_object $2a,  5, X_ATTACK, HiddenItems\n"
	"\tdb -1 ; end\n";

TEST(hidden_objects_reads_the_list_of_a_map) {
	std::string f = test::join(test::temp_dir(), "hidden_objects.asm");
	test::write_file(f, HIDDEN_OBJECTS);
	std::vector<Hidden_Object> objects;

	REQUIRE(read_hidden_objects(f.c_str(), "VIRIDIAN_FOREST", objects));
	REQUIRE(objects.size() == 3);
	CHECK_EQ(objects[0].x, 1);
	CHECK_EQ(objects[0].y, 18);
	CHECK_EQ(objects[0].line, "hidden_object  1, 18, LEAF_STONE, HiddenItems");
	CHECK_EQ(objects[2].x, 8);
	CHECK_EQ(objects[2].y, 4);
	CHECK_EQ(objects[2].line, "hidden_object  8,  4, POTION, HiddenItems ; hidden one square down");

	// The list is found through the map's label, whatever its name
	REQUIRE(read_hidden_objects(f.c_str(), "REDS_HOUSE_2F", objects));
	REQUIRE(objects.size() == 2);
	CHECK_EQ(objects[1].x, 3);
	CHECK_EQ(objects[1].y, 5);

	// Hexadecimal coordinates
	REQUIRE(read_hidden_objects(f.c_str(), "ROUTE_25", objects));
	REQUIRE(objects.size() == 2);
	CHECK_EQ(objects[1].x, 0x2a);
}

TEST(hidden_objects_has_nothing_for_other_maps) {
	std::string f = test::join(test::temp_dir(), "hidden_objects.asm");
	test::write_file(f, HIDDEN_OBJECTS);
	std::vector<Hidden_Object> objects;

	CHECK(read_hidden_objects(f.c_str(), "PALLET_TOWN", objects));
	CHECK(objects.empty());
	// A prefix of another map's constant is not that map
	CHECK(read_hidden_objects(f.c_str(), "ROUTE_2", objects));
	CHECK(objects.empty());

	std::string missing = test::join(test::temp_dir(), "missing.asm");
	CHECK(!read_hidden_objects(missing.c_str(), "VIRIDIAN_FOREST", objects));
}

TEST(hidden_objects_knows_what_can_be_moved) {
	std::string f = test::join(test::temp_dir(), "hidden_objects.asm");
	test::write_file(f,
		"\tdbw VIRIDIAN_FOREST, ViridianForestHiddenObjects\n"
		"ViridianForestHiddenObjects:\n"
		"\thidden_object  1, 18, LEAF_STONE, HiddenItems\n"
		"\thidden_object FOREST_X, 3, ANY_FACING, PrintSignText\n"
		"\thidden_object $0a, %11, COIN + 10, HiddenCoins\n"
		"\tdb -1 ; end\n");
	std::vector<Hidden_Object> objects;
	REQUIRE(read_hidden_objects(f.c_str(), "VIRIDIAN_FOREST", objects));
	REQUIRE(objects.size() == 3);
	CHECK(objects[0].movable);
	CHECK_EQ(objects[0].routine, "HiddenItems");
	CHECK_EQ(objects[0].saved_x, 1);
	CHECK_EQ(objects[0].saved_y, 18);
	CHECK(!objects[0].moved());
	// A constant for a coordinate cannot be rewritten as a number
	CHECK(!objects[1].movable);
	CHECK(objects[2].movable);
	CHECK_EQ(objects[2].x, 10);
	CHECK_EQ(objects[2].y, 3);
	CHECK_EQ(objects[2].routine, "HiddenCoins");
}

TEST(hidden_objects_rewrites_only_the_coordinates) {
	std::string line = "\thidden_object  8,  4, POTION, HiddenItems ; hidden one square down";
	REQUIRE(rewrite_coordinates(line, "hidden_object", 0, 8, 5));
	CHECK_EQ(line, "\thidden_object  8,  5, POTION, HiddenItems ; hidden one square down");
	// Wider numbers take spaces from before them, so the columns stay lined up
	REQUIRE(rewrite_coordinates(line, "hidden_object", 0, 10, 42));
	CHECK_EQ(line, "\thidden_object 10, 42, POTION, HiddenItems ; hidden one square down");
	// If there is no room, one space stays
	line = "\thidden_item VIRIDIAN_FOREST, 1, 18";
	REQUIRE(rewrite_coordinates(line, "hidden_item", 1, 100, 7));
	CHECK_EQ(line, "\thidden_item VIRIDIAN_FOREST, 100,  7");
	// Hexadecimal stays hexadecimal, and the end of a Windows line stays
	line = "\thidden_object $2a,  5, X_ATTACK, HiddenItems\r";
	REQUIRE(rewrite_coordinates(line, "hidden_object", 0, 11, 6));
	CHECK_EQ(line, "\thidden_object $0b,  6, X_ATTACK, HiddenItems\r");
	// Another macro, or too few arguments, leaves the line alone
	line = "\thidden_item VIRIDIAN_FOREST, 1, 18";
	CHECK(!rewrite_coordinates(line, "hidden_coin", 1, 2, 3));
	CHECK(!rewrite_coordinates(line, "hidden_item", 2, 2, 3));
	CHECK_EQ(line, "\thidden_item VIRIDIAN_FOREST, 1, 18");
	// The line as it would be written, for the tooltip
	Hidden_Object o;
	o.line = "hidden_object  1, 18, LEAF_STONE, HiddenItems";
	o.x = o.saved_x = 1;
	o.y = o.saved_y = 18;
	o.movable = true;
	CHECK_EQ(o.current_line(), o.line);
	o.y = 20;
	CHECK_EQ(o.current_line(), "hidden_object  1, 20, LEAF_STONE, HiddenItems");
}

// A small pokered project's data/events, with Windows line ends in hidden_objects.asm
static const char *MOVE_OBJECTS =
	"HiddenObjectMaps:\r\n"
	"\tdbw VIRIDIAN_FOREST, ViridianForestHiddenObjects\r\n"
	"\tdbw GAME_CORNER,     GameCornerHiddenObjects\r\n"
	"\tdb -1 ; end\r\n"
	"\r\n"
	"ViridianForestHiddenObjects:\r\n"
	"\thidden_object  1, 18, LEAF_STONE, HiddenItems\r\n"
	"\thidden_object 16, 42, POTION, HiddenItems ; near the exit\r\n"
	"\thidden_object  2,  2, ANY_FACING, PrintSignText\r\n"
	"\tdb -1 ; end\r\n"
	"\r\n"
	"GameCornerHiddenObjects:\r\n"
	"\thidden_object  0,  8, COIN + 10, HiddenCoins\r\n"
	"\tdb -1 ; end\r\n";

static const char *MOVE_ITEM_COORDS =
	"HiddenItemCoords:\n"
	"\t; map id, x, y\n"
	"\thidden_item MT_MOON_B2F,                   18,  12\n"
	"\thidden_item VIRIDIAN_FOREST,                1,  18\n"
	"\thidden_item VIRIDIAN_FOREST,               16,  42\n"
	"\tdb -1 ; end\n";

static const char *MOVE_COIN_COORDS =
	"HiddenCoinCoords:\n"
	"\thidden_coin GAME_CORNER,  0,  8\n"
	"\tdb -1 ; end\n";

struct Move_Project {
	std::string dir, objects, items, coins;
	Move_Project() : dir(test::temp_dir()), objects(test::join(dir, "hidden_objects.asm")),
		items(test::join(dir, "hidden_item_coords.asm")), coins(test::join(dir, "hidden_coins.asm")) {
		test::write_file(objects, MOVE_OBJECTS);
		test::write_file(items, MOVE_ITEM_COORDS);
		test::write_file(coins, MOVE_COIN_COORDS);
	}
};

TEST(hidden_objects_writes_moved_objects_and_their_coordinates) {
	Move_Project p;
	std::vector<Hidden_Object> objects;
	REQUIRE(read_hidden_objects(p.objects.c_str(), "VIRIDIAN_FOREST", objects));
	REQUIRE(objects.size() == 3);
	objects[1].y = 40; // the potion
	objects[2].x = 3;  // the sign, which no coordinate list has
	std::vector<std::string> written, warnings;
	std::string error;
	REQUIRE(write_hidden_objects(p.objects.c_str(), "VIRIDIAN_FOREST", objects, written, warnings, error));
	CHECK_EQ(error, "");
	CHECK(warnings.empty());
	REQUIRE(written.size() == 2);
	CHECK_EQ(written[0], "hidden_objects.asm");
	CHECK_EQ(written[1], "hidden_item_coords.asm");

	std::string expected(MOVE_OBJECTS);
	expected.replace(expected.find("16, 42"), 6, "16, 40");
	expected.replace(expected.find(" 2,  2"), 6, " 3,  2");
	CHECK_EQ(test::read_file(p.objects), expected);
	expected = MOVE_ITEM_COORDS;
	expected.replace(expected.find("16,  42"), 7, "16,  40");
	CHECK_EQ(test::read_file(p.items), expected);
	CHECK_EQ(test::read_file(p.coins), MOVE_COIN_COORDS);

	// The objects are saved now, so writing again changes nothing
	CHECK(!objects[1].moved());
	CHECK_EQ(objects[1].line, "hidden_object 16, 40, POTION, HiddenItems ; near the exit");
	REQUIRE(write_hidden_objects(p.objects.c_str(), "VIRIDIAN_FOREST", objects, written, warnings, error));
	CHECK(written.empty());
}

TEST(hidden_objects_writes_hidden_coins_coordinates) {
	Move_Project p;
	std::vector<Hidden_Object> objects;
	REQUIRE(read_hidden_objects(p.objects.c_str(), "GAME_CORNER", objects));
	REQUIRE(objects.size() == 1);
	objects[0].x = 1;
	objects[0].y = 9;
	std::vector<std::string> written, warnings;
	std::string error;
	REQUIRE(write_hidden_objects(p.objects.c_str(), "GAME_CORNER", objects, written, warnings, error));
	CHECK(warnings.empty());
	REQUIRE(written.size() == 2);
	CHECK_EQ(written[1], "hidden_coins.asm");
	CHECK_EQ(test::read_file(p.coins), "HiddenCoinCoords:\n\thidden_coin GAME_CORNER,  1,  9\n\tdb -1 ; end\n");
	CHECK_EQ(test::read_file(p.items), MOVE_ITEM_COORDS);
}

TEST(hidden_objects_can_swap_places) {
	Move_Project p;
	std::vector<Hidden_Object> objects;
	REQUIRE(read_hidden_objects(p.objects.c_str(), "VIRIDIAN_FOREST", objects));
	std::swap(objects[0].x, objects[1].x);
	std::swap(objects[0].y, objects[1].y);
	std::vector<std::string> written, warnings;
	std::string error;
	REQUIRE(write_hidden_objects(p.objects.c_str(), "VIRIDIAN_FOREST", objects, written, warnings, error));
	CHECK(warnings.empty());
	// Each row follows its own item, so the row of the leaf stone now has the old square of the potion
	std::string expected(MOVE_ITEM_COORDS);
	size_t leaf = expected.find(" 1,  18"), potion = expected.find("16,  42");
	expected.replace(potion, 7, " 1,  18");
	expected.replace(leaf, 7, "16,  42");
	CHECK_EQ(test::read_file(p.items), expected);
}

TEST(hidden_objects_warns_about_missing_coordinate_rows) {
	Move_Project p;
	test::write_file(p.items, "HiddenItemCoords:\n\thidden_item VIRIDIAN_FOREST, 1, 18\n\tdb -1 ; end\n");
	std::vector<Hidden_Object> objects;
	REQUIRE(read_hidden_objects(p.objects.c_str(), "VIRIDIAN_FOREST", objects));
	objects[1].x = 17; // the potion has no row
	std::vector<std::string> written, warnings;
	std::string error;
	REQUIRE(write_hidden_objects(p.objects.c_str(), "VIRIDIAN_FOREST", objects, written, warnings, error));
	REQUIRE(warnings.size() == 1);
	CHECK(warnings[0].find("16, 42") != std::string::npos);
	CHECK(warnings[0].find("hidden_item_coords.asm") != std::string::npos);
	// hidden_objects.asm is still written, and the list that could not be updated is left alone
	REQUIRE(written.size() == 1);
	CHECK(test::read_file(p.objects).find("hidden_object 17, 42, POTION") != std::string::npos);
	CHECK_EQ(test::read_file(p.items), "HiddenItemCoords:\n\thidden_item VIRIDIAN_FOREST, 1, 18\n\tdb -1 ; end\n");
}

TEST(hidden_objects_writes_nothing_if_the_file_changed) {
	Move_Project p;
	std::vector<Hidden_Object> objects;
	REQUIRE(read_hidden_objects(p.objects.c_str(), "VIRIDIAN_FOREST", objects));
	objects[0].x = 5;
	// Another program changes the line of the leaf stone after it was read
	std::string changed(MOVE_OBJECTS);
	changed.replace(changed.find("LEAF_STONE"), 10, "MOON_STONE");
	test::write_file(p.objects, changed);
	std::vector<std::string> written, warnings;
	std::string error;
	CHECK(!write_hidden_objects(p.objects.c_str(), "VIRIDIAN_FOREST", objects, written, warnings, error));
	CHECK(error.find("modified by another program") != std::string::npos);
	CHECK(written.empty());
	CHECK_EQ(test::read_file(p.objects), changed);
	CHECK_EQ(test::read_file(p.items), MOVE_ITEM_COORDS);
	CHECK(objects[0].moved());
}
