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
