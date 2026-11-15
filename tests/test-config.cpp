// Where the program looks for a project's files (src/config.cpp)

#pragma warning(push, 0)
#include <FL/filename.H>
#pragma warning(pop)

#include "config.h"
#include "test.h"
#include "utils.h"

// Makes a project folder with the given (empty) files and returns it with its trailing separator
static std::string make_project(const std::vector<std::string> &files) {
	std::string dir = test::temp_dir();
	test::write_file(test::join(dir, "Makefile"), "");
	for (const std::string &f : files) {
		test::write_file(test::join(dir, f), "");
	}
	return test::as_project_dir(dir);
}

static std::string example_project() {
	return test::as_project_dir(test::example_path(""));
}

TEST(config_finds_the_project_folder_from_a_map_file) {
	std::string project = make_project({"maps/Route1.blk", "maps/extra/Cave.blk"});
	char found[FL_PATH_MAX] = {};

	// The project folder is the nearest one above the map that has a Makefile
	CHECK(Config::project_path_from_blk_path(test::join(project, "maps/Route1.blk").c_str(), found));
	CHECK_EQ(found, project);
	CHECK(Config::project_path_from_blk_path(test::join(project, "maps/extra/Cave.blk").c_str(), found));
	CHECK_EQ(found, project);
}

TEST(config_finds_pokecrystal_files) {
	std::string project = example_project();
	char path[FL_PATH_MAX] = {};

	Config::metatileset_path(path, project.c_str(), "kanto");
	CHECK_EQ(path, test::join(project, "data/tilesets/kanto_metatiles.bin"));
	Config::tileset_path(path, project.c_str(), "kanto");
	CHECK_EQ(path, test::join(project, "gfx/tilesets/kanto.png"));
	Config::palette_map_path(path, project.c_str(), "kanto");
	CHECK_EQ(path, test::join(project, "gfx/tilesets/kanto_palette_map.asm"));

	// A tileset named "kanto.something" falls back to kanto's files when it has none of its own
	Config::metatileset_path(path, project.c_str(), "kanto.something");
	CHECK_EQ(path, test::join(project, "data/tilesets/kanto_metatiles.bin"));
	Config::palette_map_path(path, project.c_str(), "kanto.something");
	CHECK_EQ(path, test::join(project, "gfx/tilesets/kanto_palette_map.asm"));
}

TEST(config_finds_pokered_files) {
	std::string project = make_project({
		"gfx/blocksets/overworld.bst",
		"gfx/tilesets/overworld.png",
		"data/maps/objects/PalletTown.asm",
		"data/maps/headers/PalletTown.asm",
		"constants/map_constants.asm",
		"constants/tileset_constants.asm",
	});
	char path[FL_PATH_MAX] = {};

	Config::metatileset_path(path, project.c_str(), "overworld");
	CHECK_EQ(path, test::join(project, "gfx/blocksets/overworld.bst"));
	Config::tileset_path(path, project.c_str(), "overworld");
	CHECK_EQ(path, test::join(project, "gfx/tilesets/overworld.png"));
	Config::event_script_path(path, project.c_str(), "PalletTown");
	CHECK_EQ(path, test::join(project, "data/maps/objects/PalletTown.asm"));
	Config::map_header_path(path, project.c_str(), "PalletTown");
	CHECK_EQ(path, test::join(project, "data/maps/headers/PalletTown.asm"));
	Config::map_constants_path(path, project.c_str());
	CHECK_EQ(path, test::join(project, "constants/map_constants.asm"));
	Config::tileset_constants_path(path, project.c_str());
	CHECK_EQ(path, test::join(project, "constants/tileset_constants.asm"));
	// pokered has no file that lists every map's header
	CHECK(!Config::map_headers_path(path, project.c_str()));
}

TEST(config_finds_pokecrystal_map_files) {
	std::string project = make_project({
		"maps/NewBarkTown.asm",
		"data/maps/maps.asm",
		"constants/map_constants.asm",
		"data/tilesets/johto_collision.asm",
	});
	char path[FL_PATH_MAX] = {};

	Config::event_script_path(path, project.c_str(), "NewBarkTown");
	CHECK_EQ(path, test::join(project, "maps/NewBarkTown.asm"));
	CHECK(Config::map_headers_path(path, project.c_str()));
	CHECK_EQ(path, test::join(project, "data/maps/maps.asm"));
	// The return value says whether the collisions are a binary file
	CHECK(!Config::collisions_path(path, project.c_str(), "johto"));
	CHECK_EQ(path, test::join(project, "data/tilesets/johto_collision.asm"));
}

TEST(config_prefers_png_graphics_and_asm_collisions) {
	std::string project = make_project({
		"gfx/tilesets/cave.png",
		"gfx/tilesets/cave.2bpp",
		"gfx/tilesets/forest.2bpp",
		"data/tilesets/cave_collision.asm",
		"data/tilesets/cave_collision.bin",
		"data/tilesets/forest_collision.bin",
	});
	char path[FL_PATH_MAX] = {};

	Config::tileset_path(path, project.c_str(), "cave");
	CHECK_EQ(path, test::join(project, "gfx/tilesets/cave.png"));
	Config::tileset_path(path, project.c_str(), "forest");
	CHECK_EQ(path, test::join(project, "gfx/tilesets/forest.2bpp"));

	CHECK(!Config::collisions_path(path, project.c_str(), "cave"));
	CHECK_EQ(path, test::join(project, "data/tilesets/cave_collision.asm"));
	CHECK(Config::collisions_path(path, project.c_str(), "forest"));
	CHECK_EQ(path, test::join(project, "data/tilesets/forest_collision.bin"));
}

TEST(config_names_the_png_files_to_save_a_tileset_to) {
	std::string project = make_project({});
	char png[FL_PATH_MAX] = {}, before[FL_PATH_MAX] = {}, after[FL_PATH_MAX] = {};

	Config::tileset_png_paths(png, before, after, project.c_str(), "cave");
	CHECK_EQ(png, test::join(project, "gfx/tilesets/cave.png"));
	CHECK_EQ(before, "");
	CHECK_EQ(after, "");

	// A tileset saved as .2bpp is written back as .png
	test::write_file(test::join(project, "gfx/tilesets/forest.2bpp"), "");
	Config::tileset_png_paths(png, before, after, project.c_str(), "forest");
	CHECK_EQ(png, test::join(project, "gfx/tilesets/forest.png"));
}
