// Gen 1 walkable tiles (src/collision-tiles.cpp)

#pragma warning(push, 0)
#include <FL/filename.H>
#pragma warning(pop)

#include "collision-tiles.h"
#include "test.h"

static const char *COLLISION_TILE_IDS =
	"MACRO coll_tiles\n"
	"\tIF _NARG\n"
	"\t\tdb \\# ; all args\n"
	"\tENDC\n"
	"\tdb -1 ; end\n"
	"ENDM\n"
	"\n"
	"Overworld_Coll::\n"
	"\tcoll_tiles $00, $10, $1b, $20\n"
	"\n"
	"RedsHouse1_Coll::\n"
	"RedsHouse2_Coll::\n"
	"\tcoll_tiles $01, $02, $1a\n"
	"\n"
	"House_Coll::\n"
	"; a comment between the label and the list\n"
	"\tcoll_tiles $01, 18, %10100 ; three ways to write numbers\n";

static std::vector<int> passable_list(const std::vector<bool> &passable) {
	std::vector<int> list;
	for (size_t i = 0; i < passable.size(); i++) {
		if (passable[i]) { list.push_back((int)i); }
	}
	return list;
}

TEST(collision_tiles_reads_the_walkable_tiles_of_a_tileset) {
	std::string f = test::join(test::temp_dir(), "collision_tile_ids.asm");
	test::write_file(f, COLLISION_TILE_IDS);
	std::vector<bool> passable;

	REQUIRE(read_passable_tiles(f.c_str(), {"Overworld"}, passable));
	CHECK_EQ(passable.size(), (size_t)256);
	CHECK(passable_list(passable) == std::vector<int>({0x00, 0x10, 0x1b, 0x20}));

	// Either of two labels in front of the same list
	REQUIRE(read_passable_tiles(f.c_str(), {"RedsHouse2"}, passable));
	CHECK(passable_list(passable) == std::vector<int>({0x01, 0x02, 0x1a}));

	REQUIRE(read_passable_tiles(f.c_str(), {"House"}, passable));
	CHECK(passable_list(passable) == std::vector<int>({0x01, 18, 20}));

	CHECK(!read_passable_tiles(f.c_str(), {"Cavern"}, passable));
	std::string missing = test::join(test::temp_dir(), "missing.asm");
	CHECK(!read_passable_tiles(missing.c_str(), {"Overworld"}, passable));
}

TEST(collision_tiles_finds_the_labels_of_a_graphics_file) {
	std::string dir = test::temp_dir();
	std::string project = test::as_project_dir(dir);
	test::write_file(test::join(dir, "gfx/tilesets.asm"),
		"Overworld_GFX::     INCBIN \"gfx/tilesets/overworld.2bpp\"\n"
		"Overworld_Block::   INCBIN \"gfx/blocksets/overworld.bst\"\n"
		"\n"
		"RedsHouse1_GFX::\n"
		"RedsHouse2_GFX::    INCBIN \"gfx/tilesets/reds_house.2bpp\"\n"
		"RedsHouse1_Block::\n"
		"RedsHouse2_Block::  INCBIN \"gfx/blocksets/reds_house.bst\"\n");

	CHECK(tileset_labels(project.c_str(), "reds_house") == std::vector<std::string>({"RedsHouse1", "RedsHouse2"}));
	CHECK(tileset_labels(project.c_str(), "overworld") == std::vector<std::string>({"Overworld"}));
	// Not in gfx/tilesets.asm: the name itself, written like a label
	CHECK(tileset_labels(project.c_str(), "ship_port") == std::vector<std::string>({"ShipPort"}));
}
