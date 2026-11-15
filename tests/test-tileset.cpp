// Palette maps, tileset graphics, blocksets and collisions
// (src/palette-map.cpp, src/tileset.cpp, src/tiled-image.cpp, src/metatileset.cpp, src/image.cpp)

#include "config.h"
#include "metatileset.h"
#include "test.h"
#include "tiled-image.h"

struct Tileset_Files {
	std::string palette_map, graphics, metatiles;
};

static Tileset_Files example_tileset(const std::string &name) {
	return {
		test::example_path("gfx/tilesets/" + name + "_palette_map.asm"),
		test::example_path("gfx/tilesets/" + name + ".png"),
		test::example_path("data/tilesets/" + name + "_metatiles.bin")
	};
}

// Loads a tileset in the same order as the main window does
static void load_tileset(Metatileset &metatileset, const Tileset_Files &files) {
	Tileset &tileset = metatileset.tileset();
	REQUIRE(tileset.read_palette_map(files.palette_map.c_str()) == Palette_Map::Result::PALETTE_OK);
	REQUIRE(tileset.read_graphics(files.graphics.c_str(), NULL, NULL, Palettes::DAY) == Tileset::Result::GFX_OK);
	REQUIRE(metatileset.read_metatiles(files.metatiles.c_str()) == Metatileset::Result::META_OK);
}

// What the tileset editor does when its changes are accepted (Tileset_Window::apply_modifications):
// the palette map takes the color of every tile, and tiles drawn in free slots get a color
static void accept_tileset_edits(Tileset &tileset) {
	for (int i = 0; i < MAX_NUM_TILES; i++) {
		tileset.palette_map().palette((uint8_t)i, tileset.tile((uint8_t)i)->palette());
	}
	tileset.color_drawn_tiles();
	tileset.modified(true);
}

// What File > Save does with a modified tileset (Main_Window::save_tileset)
static void save_tileset(Metatileset &metatileset, const Tileset_Files &files) {
	metatileset.trim_tileset();
	Tileset &tileset = metatileset.tileset();
	const char *failed = tileset.write_graphics(files.graphics.c_str(), "", "");
	REQUIRE(failed == NULL);
	if (!Config::monochrome()) {
		REQUIRE(tileset.palette_map().write_palette_map(files.palette_map.c_str()));
	}
}

static int count_different_tiles(Tileset &a, Tileset &b) {
	int different = 0;
	for (int i = 0; i < MAX_NUM_TILES; i++) {
		const Tile *ta = a.const_tile((uint8_t)i), *tb = b.const_tile((uint8_t)i);
		bool same = ta->palette() == tb->palette();
		for (int y = 0; y < TILE_SIZE && same; y++) {
			for (int x = 0; x < TILE_SIZE && same; x++) {
				same = ta->hue(x, y) == tb->hue(x, y);
			}
		}
		if (!same) { different++; }
	}
	return different;
}

TEST(palette_map_reads_the_color_of_each_tile) {
	Palette_Map palette_map;
	std::string f = test::example_path("gfx/tilesets/kanto_palette_map.asm");
	CHECK(palette_map.read_from(f.c_str()) == Palette_Map::Result::PALETTE_OK);

	// Two banks of 96 tiles; the "rept 16 / db $ff" between them is not a tile color
	CHECK_EQ(palette_map.size(), (size_t)192);
	CHECK(palette_map.palette(0) == Palette::GRAY);
	CHECK(palette_map.palette(1) == Palette::BROWN);
	CHECK(palette_map.palette(3) == Palette::RED);
	CHECK(palette_map.palette(10) == Palette::YELLOW);
	CHECK(palette_map.palette(95) == Palette::GRAY);
	CHECK(palette_map.palette(96) == Palette::GRAY);
	CHECK(palette_map.palette(99) == Palette::RED);
	CHECK(palette_map.palette(192) == Palette::UNDEFINED);
	CHECK(!Config::monochrome());
	CHECK(!Config::allow_priority());
}

TEST(palette_map_turns_monochrome_without_a_file) {
	// Projects without palette maps (pokered, pokeyellow) are shown in monochrome
	Palette_Map palette_map;
	std::string missing = test::join(test::temp_dir(), "missing_palette_map.asm");
	CHECK(palette_map.read_from(missing.c_str()) == Palette_Map::Result::PALETTE_OK);
	CHECK(Config::monochrome());
	CHECK_EQ(palette_map.size(), (size_t)0x60);
	CHECK(palette_map.palette(0) == Palette::MONOCHROME);
	CHECK(palette_map.palette(0x5f) == Palette::MONOCHROME);
	CHECK(palette_map.palette(0x60) == Palette::UNDEFINED);

	Config::allow_256_tiles(true);
	CHECK(palette_map.read_from(missing.c_str()) == Palette_Map::Result::PALETTE_OK);
	CHECK_EQ(palette_map.size(), (size_t)MAX_NUM_TILES);
}

TEST(palette_map_rejects_an_unknown_color) {
	std::string dir = test::temp_dir();

	std::string bad = test::join(dir, "bad_palette_map.asm");
	test::write_file(bad, "\ttilepal 0, GRAY, PURPLE, RED\n");
	Palette_Map bad_map;
	CHECK(bad_map.read_from(bad.c_str()) == Palette_Map::Result::BAD_PALETTE_NAME);

	std::string priority = test::join(dir, "priority_palette_map.asm");
	test::write_file(priority, "\ttilepal 0, GRAY, PRIORITY_RED ; above sprites\n");
	Palette_Map priority_map;
	CHECK(priority_map.read_from(priority.c_str()) == Palette_Map::Result::PALETTE_OK);
	CHECK_EQ(priority_map.size(), (size_t)2);
	CHECK(priority_map.palette(1) == Palette::PRIORITY_RED);
	CHECK(Config::allow_priority());
}

TEST(tileset_reads_png_graphics) {
	Metatileset metatileset;
	Tileset_Files files = example_tileset("johto");
	load_tileset(metatileset, files);
	Tileset &tileset = metatileset.tileset();

	Tiled_Image image(files.graphics.c_str());
	REQUIRE(image.result() == Tiled_Image::Result::IMG_OK);
	CHECK_EQ(image.num_tiles(), (size_t)192);
	CHECK_EQ(tileset.num_tiles(), (size_t)192);

	// Without the "256 tiles" option, tile IDs $60 to $7F are skipped:
	// the 97th tile of the image is tile $80
	int wrong_hues = 0, wrong_palettes = 0;
	for (int i = 0; i < 192; i++) {
		int id = i < 0x60 ? i : i + 0x20;
		const Tile *t = tileset.const_tile((uint8_t)id);
		if (t->palette() != tileset.palette_map().palette((uint8_t)i)) { wrong_palettes++; }
		for (int y = 0; y < TILE_SIZE; y++) {
			for (int x = 0; x < TILE_SIZE; x++) {
				if (t->hue(x, y) != image.tile_hue(i, x, y)) { wrong_hues++; }
			}
		}
	}
	CHECK_EQ(wrong_hues, 0);
	CHECK_EQ(wrong_palettes, 0);
	CHECK(tileset.const_tile(0x60)->palette() == Palette::UNDEFINED);
	CHECK(tileset.const_tile(0x60)->is_blank());
	CHECK(!tileset.modified());
}

TEST(tileset_tracks_changes_to_each_part_of_a_split_tileset) {
	// A tileset can be split into a main image and an "after" image (a name like "kanto.,johto").
	// Saving checks each part for changes by another program with its own time; the "after"
	// part used to be checked with the time of the (here missing) "before" part, so it always
	// looked changed.
	std::string dir = test::temp_dir();
	std::string main_png = test::join(dir, "kanto.png"), after_png = test::join(dir, "johto.png");
	test::copy_file(test::example_path("gfx/tilesets/kanto.png"), main_png);
	test::copy_file(test::example_path("gfx/tilesets/johto.png"), after_png);
	Config::allow_256_tiles(true);

	Tileset tileset;
	REQUIRE(tileset.read_graphics(main_png.c_str(), NULL, after_png.c_str(), Palettes::DAY) == Tileset::Result::GFX_OK);
	CHECK(!tileset.other_modified(main_png.c_str()));
	CHECK(!tileset.other_modified_after(after_png.c_str()));

	test::make_newer(after_png);
	CHECK(tileset.other_modified_after(after_png.c_str()));
	CHECK(!tileset.other_modified(main_png.c_str()));
}

TEST(tileset_reports_missing_graphics) {
	Tileset tileset;
	std::string missing = test::join(test::temp_dir(), "missing.png");
	CHECK(tileset.read_graphics(missing.c_str(), NULL, NULL, Palettes::DAY) == Tileset::Result::GFX_BAD_FILE);

	std::string unknown = test::join(test::temp_dir(), "tileset.bmp");
	test::write_file(unknown, "BM");
	CHECK(tileset.read_graphics(unknown.c_str(), NULL, NULL, Palettes::DAY) == Tileset::Result::GFX_BAD_FILE);
}

static void check_saved_and_reopened(const std::string &name) {
	Metatileset original;
	load_tileset(original, example_tileset(name));
	accept_tileset_edits(original.tileset());

	std::string dir = test::temp_dir();
	Tileset_Files saved = {
		test::join(dir, name + "_palette_map.asm"),
		test::join(dir, name + ".png"),
		example_tileset(name).metatiles
	};
	save_tileset(original, saved);

	Metatileset reopened;
	load_tileset(reopened, saved);
	reopened.trim_tileset();

	CHECK_EQ(count_different_tiles(original.tileset(), reopened.tileset()), 0);
}

TEST(tileset_saved_and_reopened_is_the_same_johto) {
	check_saved_and_reopened("johto");
}

TEST(tileset_saved_and_reopened_is_the_same_kanto) {
	check_saved_and_reopened("kanto");
}

// What drawing one pixel in the tileset editor does to a tile: the pixel takes the hue,
// shown in the colors of whatever color the tile has (which may be none)
static void draw_pixel(Tileset &tileset, uint8_t id, int x, int y, Hue hue) {
	Tile *t = tileset.tile(id);
	const uchar *rgb = Color::color(tileset.palettes(), t->palette(), hue);
	t->pixel(x, y, hue, rgb[0], rgb[1], rgb[2]);
}

static void erase_tile(Tileset &tileset, uint8_t id) {
	for (int y = 0; y < TILE_SIZE; y++) {
		for (int x = 0; x < TILE_SIZE; x++) {
			draw_pixel(tileset, id, x, y, Hue::WHITE);
		}
	}
}

// A monochrome tileset (no palette map, as in pokered and pokeyellow) that has been saved once,
// with its last eight tiles blank and not used by any block. Saving leaves those out of the image
// and marks them as free: they no longer have a color.
static Tileset_Files monochrome_tileset_with_free_tiles(Metatileset &metatileset) {
	std::string dir = test::temp_dir();
	Tileset_Files files = example_tileset("kanto");
	files.palette_map = test::join(dir, "no_palette_map.asm");
	load_tileset(metatileset, files);
	REQUIRE(Config::monochrome());

	metatileset.size(1);
	Tileset &tileset = metatileset.tileset();
	for (int i = 0x58; i < 0x60; i++) {
		erase_tile(tileset, (uint8_t)i);
		REQUIRE(!metatileset.uses_tile((uint8_t)i));
	}
	accept_tileset_edits(tileset);

	files.graphics = test::join(dir, "kanto.png");
	save_tileset(metatileset, files);
	REQUIRE(tileset.const_tile(0x5c)->palette() == Palette::UNDEFINED);
	return files;
}

TEST(tileset_keeps_a_tile_drawn_in_a_free_slot) {
	// The bug: in monochrome mode, a tile drawn in one of the free slots at the end of a tileset
	// that had already been saved was left out of the next save, which still said "Saved!".
	// The tile had no color, there is no way to give it one in monochrome mode, and saving
	// treated a tile without a color as an empty slot.
	Metatileset metatileset;
	Tileset_Files files = monochrome_tileset_with_free_tiles(metatileset);
	Tileset &tileset = metatileset.tileset();

	// Saving has to cope with such a tile by itself, so this does not go through
	// accept_tileset_edits(), which would already give the tile a color
	draw_pixel(tileset, 0x5c, 3, 4, Hue::BLACK);
	for (int i = 0; i < MAX_NUM_TILES; i++) {
		tileset.palette_map().palette((uint8_t)i, tileset.tile((uint8_t)i)->palette());
	}
	tileset.modified(true);
	save_tileset(metatileset, files);

	Metatileset reopened;
	load_tileset(reopened, files);
	CHECK(reopened.tileset().const_tile(0x5c)->hue(3, 4) == Hue::BLACK);
	CHECK(!reopened.tileset().const_tile(0x5c)->is_blank());
	// The free slots after it are still left out
	CHECK(reopened.tileset().const_tile(0x5d)->is_blank());
}

TEST(tileset_gives_a_color_to_a_tile_drawn_without_one) {
	Metatileset metatileset;
	monochrome_tileset_with_free_tiles(metatileset);
	Tileset &tileset = metatileset.tileset();

	// Nothing to do while the free slots are blank
	CHECK_EQ(tileset.color_drawn_tiles(), (size_t)0);
	CHECK(tileset.const_tile(0x5c)->palette() == Palette::UNDEFINED);

	draw_pixel(tileset, 0x5c, 3, 4, Hue::BLACK);
	CHECK_EQ(tileset.color_drawn_tiles(), (size_t)1);
	CHECK(tileset.const_tile(0x5c)->palette() == Palette::MONOCHROME);
	CHECK(tileset.const_tile(0x5c)->hue(3, 4) == Hue::BLACK);
	// The blank slots next to it stay free
	CHECK(tileset.const_tile(0x5b)->palette() == Palette::UNDEFINED);
	CHECK(tileset.const_tile(0x5d)->palette() == Palette::UNDEFINED);

	// The tile is now shown in the monochrome colors, not in those of a free slot
	const uchar *expected = Color::color(tileset.palettes(), Palette::MONOCHROME, Hue::BLACK);
	const uchar *shown = tileset.const_tile(0x5c)->const_pixel(3, 4);
	CHECK(shown[0] == expected[0] && shown[1] == expected[1] && shown[2] == expected[2]);
}

TEST(tileset_gives_gray_to_a_tile_drawn_without_a_color_in_a_color_project) {
	Metatileset metatileset;
	load_tileset(metatileset, example_tileset("kanto"));
	Tileset &tileset = metatileset.tileset();
	REQUIRE(!Config::monochrome());
	// Tile $60 is outside what a project without the "256 tiles" option uses, so it has no color
	REQUIRE(tileset.const_tile(0x60)->palette() == Palette::UNDEFINED);

	draw_pixel(tileset, 0x60, 0, 0, Hue::DARK);
	CHECK_EQ(tileset.color_drawn_tiles(), (size_t)1);
	CHECK(tileset.const_tile(0x60)->palette() == Palette::GRAY);
}

static uint32_t big_endian_32(const std::string &bytes, size_t at) {
	return ((uint32_t)(uint8_t)bytes[at] << 24) | ((uint32_t)(uint8_t)bytes[at + 1] << 16) |
		((uint32_t)(uint8_t)bytes[at + 2] << 8) | (uint32_t)(uint8_t)bytes[at + 3];
}

TEST(tileset_is_saved_as_a_2_bit_grayscale_png) {
	Metatileset metatileset;
	load_tileset(metatileset, example_tileset("johto"));
	accept_tileset_edits(metatileset.tileset());

	std::string dir = test::temp_dir();
	Tileset_Files saved = {test::join(dir, "johto_palette_map.asm"), test::join(dir, "johto.png"), ""};
	save_tileset(metatileset, saved);

	// The PNG header: 8 signature bytes, then the IHDR chunk with the size and pixel format
	std::string png = test::read_file(saved.graphics);
	REQUIRE(png.size() > 26);
	CHECK_EQ(png.substr(1, 3), "PNG");
	CHECK_EQ(big_endian_32(png, 16), (uint32_t)128); // width: 16 tiles
	// Height: 10 rows of tiles. The original image has 12, but its last two rows are blank tiles
	// that no block uses, and saving leaves those out.
	CHECK_EQ(big_endian_32(png, 20), (uint32_t)80);
	CHECK_EQ((int)(uint8_t)png[24], 2);              // bits per pixel
	CHECK_EQ((int)(uint8_t)png[25], 0);              // grayscale
}

TEST(metatileset_needs_graphics_first) {
	Metatileset metatileset;
	std::string f = example_tileset("kanto").metatiles;
	CHECK(metatileset.read_metatiles(f.c_str()) == Metatileset::Result::META_NO_GFX);
}

TEST(metatileset_reads_blocks_of_16_tiles) {
	Metatileset metatileset;
	Tileset_Files files = example_tileset("kanto");
	load_tileset(metatileset, files);

	std::string bytes = test::read_file(files.metatiles);
	CHECK_EQ(metatileset.size(), bytes.size() / 16);
	CHECK_EQ(metatileset.size(), (size_t)128);
	// Tiles are stored row by row, 4 per row
	const Metatile *mt = metatileset.metatile(5);
	CHECK_EQ(mt->tile_id(0, 0), (uint8_t)bytes[5 * 16]);
	CHECK_EQ(mt->tile_id(3, 0), (uint8_t)bytes[5 * 16 + 3]);
	CHECK_EQ(mt->tile_id(0, 1), (uint8_t)bytes[5 * 16 + 4]);
	CHECK_EQ(mt->tile_id(3, 3), (uint8_t)bytes[5 * 16 + 15]);
	CHECK(!metatileset.modified());
}

TEST(tile_tells_whether_its_graphics_changed) {
	// The tileset editor uses this so that leaving it with OK, without changing anything,
	// does not mark the tileset as modified (which made the next save rewrite its PNG)
	Metatileset metatileset;
	load_tileset(metatileset, example_tileset("kanto"));
	Tileset &tileset = metatileset.tileset();

	Tile copy(5);
	copy.copy(tileset.const_tile(5));
	CHECK(copy.same_graphics(tileset.const_tile(5)));

	Hue h = copy.hue(2, 2) == Hue::BLACK ? Hue::WHITE : Hue::BLACK;
	copy.pixel(2, 2, h, 0, 0, 0);
	CHECK(!copy.same_graphics(tileset.const_tile(5)));

	copy.copy(tileset.const_tile(5));
	copy.palette(copy.palette() == Palette::RED ? Palette::GRAY : Palette::RED);
	CHECK(!copy.same_graphics(tileset.const_tile(5)));
}

TEST(metatile_tells_whether_it_changed) {
	Metatileset metatileset;
	load_tileset(metatileset, example_tileset("kanto"));

	Metatile copy(7);
	copy.copy(metatileset.metatile(7));
	CHECK(copy.same_as(metatileset.metatile(7)));

	copy.tile_id(1, 2, (uint8_t)(copy.tile_id(1, 2) + 1));
	CHECK(!copy.same_as(metatileset.metatile(7)));

	copy.copy(metatileset.metatile(7));
	copy.collision(Quadrant::TOP_RIGHT, "WATER");
	CHECK(!copy.same_as(metatileset.metatile(7)));

	copy.copy(metatileset.metatile(7));
	copy.bin_collision(Quadrant::BOTTOM_LEFT, 0x29);
	CHECK(!copy.same_as(metatileset.metatile(7)));
}

TEST(metatileset_read_twice_does_not_add_the_blocks_again) {
	// Opening a map on top of another one used to read the new blockset after the old one,
	// and a later save wrote both into the new blockset file
	Metatileset metatileset;
	Tileset_Files files = example_tileset("kanto");
	load_tileset(metatileset, files);
	REQUIRE(metatileset.size() == 128);

	CHECK(metatileset.read_metatiles(files.metatiles.c_str()) == Metatileset::Result::META_OK);
	CHECK_EQ(metatileset.size(), (size_t)128);

	std::string saved = test::join(test::temp_dir(), "metatiles.bin");
	CHECK(metatileset.write_metatiles(saved.c_str()));
	CHECK(test::read_file(saved) == test::read_file(files.metatiles));
}

TEST(metatileset_writes_back_the_same_bytes) {
	for (const char *name : {"kanto", "johto", "johto_modern"}) {
		Metatileset metatileset;
		Tileset_Files files = example_tileset(name);
		load_tileset(metatileset, files);

		std::string saved = test::join(test::temp_dir(), "metatiles.bin");
		CHECK(metatileset.write_metatiles(saved.c_str()));
		CHECK(test::read_file(saved) == test::read_file(files.metatiles));
	}
}

TEST(metatileset_reports_a_bad_blockset_file) {
	std::string dir = test::temp_dir();
	Tileset_Files files = example_tileset("kanto");

	files.metatiles = test::join(dir, "incomplete.bin");
	test::write_file(files.metatiles, std::string(17, '\1'));
	Metatileset incomplete;
	Tileset &incomplete_tileset = incomplete.tileset();
	REQUIRE(incomplete_tileset.read_palette_map(files.palette_map.c_str()) == Palette_Map::Result::PALETTE_OK);
	REQUIRE(incomplete_tileset.read_graphics(files.graphics.c_str(), NULL, NULL, Palettes::DAY) == Tileset::Result::GFX_OK);
	CHECK(incomplete.read_metatiles(files.metatiles.c_str()) == Metatileset::Result::META_TOO_SHORT);

	files.metatiles = test::join(dir, "too_many.bin");
	test::write_file(files.metatiles, std::string(16 * (MAX_NUM_METATILES + 1), '\1'));
	Metatileset too_many;
	Tileset &too_many_tileset = too_many.tileset();
	REQUIRE(too_many_tileset.read_palette_map(files.palette_map.c_str()) == Palette_Map::Result::PALETTE_OK);
	REQUIRE(too_many_tileset.read_graphics(files.graphics.c_str(), NULL, NULL, Palettes::DAY) == Tileset::Result::GFX_OK);
	CHECK(too_many.read_metatiles(files.metatiles.c_str()) == Metatileset::Result::META_TOO_LONG);

	files.metatiles = test::join(dir, "missing.bin");
	Metatileset missing;
	Tileset &missing_tileset = missing.tileset();
	REQUIRE(missing_tileset.read_palette_map(files.palette_map.c_str()) == Palette_Map::Result::PALETTE_OK);
	REQUIRE(missing_tileset.read_graphics(files.graphics.c_str(), NULL, NULL, Palettes::DAY) == Tileset::Result::GFX_OK);
	CHECK(missing.read_metatiles(files.metatiles.c_str()) == Metatileset::Result::META_BAD_FILE);
}

TEST(metatileset_can_shrink_and_grow) {
	Metatileset metatileset;
	Tileset_Files files = example_tileset("kanto");
	load_tileset(metatileset, files);
	std::string original = test::read_file(files.metatiles);
	std::string saved = test::join(test::temp_dir(), "metatiles.bin");

	metatileset.size(3);
	CHECK(metatileset.modified());
	CHECK(metatileset.write_metatiles(saved.c_str()));
	CHECK(test::read_file(saved) == original.substr(0, 3 * 16));

	// New blocks start out filled with tile 0
	metatileset.size(5);
	CHECK(metatileset.write_metatiles(saved.c_str()));
	CHECK(test::read_file(saved) == original.substr(0, 3 * 16) + std::string(2 * 16, '\0'));
}

TEST(metatileset_reads_and_writes_asm_collisions) {
	Metatileset metatileset;
	load_tileset(metatileset, example_tileset("kanto"));
	metatileset.size(3);

	std::string collisions =
		"\ttilecoll WALL, WALL, FLOOR, FLOOR ; 00\n"
		"\ttilecoll WATER, WATER, WATER, WATER ; 01\n"
		"\ttilecoll FLOOR, $12, TALL_GRASS, DOOR ; 02\n";
	std::string dir = test::temp_dir();
	std::string f = test::join(dir, "kanto_collision.asm"), saved = test::join(dir, "saved_collision.asm");
	test::write_file(f, collisions);

	metatileset.bin_collisions(false);
	CHECK(metatileset.read_collisions(f.c_str()) == Metatileset::Result::META_OK);
	CHECK_EQ(metatileset.metatile(0)->collision(Quadrant::TOP_LEFT), "WALL");
	CHECK_EQ(metatileset.metatile(0)->collision(Quadrant::BOTTOM_RIGHT), "FLOOR");
	CHECK_EQ(metatileset.metatile(2)->collision(Quadrant::TOP_RIGHT), "$12");
	CHECK_EQ(metatileset.metatile(2)->collision(Quadrant::BOTTOM_LEFT), "TALL_GRASS");

	CHECK(metatileset.write_collisions(saved.c_str()));
	CHECK_EQ(test::read_file(saved), collisions);
}

TEST(metatileset_reads_and_writes_binary_collisions) {
	Metatileset metatileset;
	load_tileset(metatileset, example_tileset("kanto"));
	metatileset.size(3);

	std::string collisions("\x07\x07\x00\x00\x29\x29\x29\x29\x00\x12\x18\x71", 12);
	std::string dir = test::temp_dir();
	std::string f = test::join(dir, "kanto_collision.bin"), saved = test::join(dir, "saved_collision.bin");
	test::write_file(f, collisions);

	metatileset.bin_collisions(true);
	CHECK(metatileset.read_collisions(f.c_str()) == Metatileset::Result::META_OK);
	CHECK_EQ(metatileset.metatile(0)->bin_collision(Quadrant::TOP_LEFT), 0x07);
	CHECK_EQ(metatileset.metatile(2)->bin_collision(Quadrant::BOTTOM_RIGHT), 0x71);

	CHECK(metatileset.write_collisions(saved.c_str()));
	CHECK(test::read_file(saved) == collisions);

	std::string incomplete = test::join(dir, "incomplete_collision.bin");
	test::write_file(incomplete, collisions.substr(0, 5));
	CHECK(metatileset.read_collisions(incomplete.c_str()) == Metatileset::Result::META_TOO_SHORT);
}
