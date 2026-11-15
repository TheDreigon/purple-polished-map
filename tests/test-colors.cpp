// Exporting the palettes (src/colors.cpp)

#pragma warning(push, 0)
#include <FL/filename.H>
#pragma warning(pop)

#include "colors.h"
#include "config.h"
#include "test.h"

static std::vector<std::string> lines_of(const std::string &text) {
	std::vector<std::string> lines;
	std::istringstream ss(text);
	for (std::string line; std::getline(ss, line);) {
		lines.push_back(line);
	}
	return lines;
}

TEST(colors_exports_every_palette_set_with_its_name) {
	// The six sets are morn, day, nite, darkness, indoor and custom; "darkness" used to be
	// missing from the names, so the last three sets were mislabeled and the last one was
	// written with a null pointer as its name
	std::string f = test::join(test::temp_dir(), "palettes.pal");
	REQUIRE(Color::write_palettes(f.c_str()));
	std::vector<std::string> lines = lines_of(test::read_file(f));

	REQUIRE(lines.size() == NUM_PALETTE_SETS * (1 + NUM_GAME_PALETTES));
	const char *names[NUM_PALETTE_SETS] = {"morn", "day", "nite", "darkness", "indoor", "custom"};
	for (int l = 0; l < NUM_PALETTE_SETS; l++) {
		CHECK_EQ(lines[l * (1 + NUM_GAME_PALETTES)], std::string("; ") + names[l]);
	}
	CHECK_EQ(lines[1].substr(0, 5), "\tRGB ");
	CHECK(ends_with(lines[1], " ; gray"));
	CHECK(ends_with(lines[8], " ; text"));
}

TEST(colors_exports_monochrome_palettes_with_their_names) {
	Config::monochrome(true);
	std::string f = test::join(test::temp_dir(), "palettes.pal");
	REQUIRE(Color::write_palettes(f.c_str()));
	std::vector<std::string> lines = lines_of(test::read_file(f));

	REQUIRE(lines.size() == NUM_PALETTE_SETS);
	CHECK(ends_with(lines[3], " ; darkness"));
	CHECK(ends_with(lines[5], " ; custom"));
}
