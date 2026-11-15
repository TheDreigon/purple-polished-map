// Connections between maps (src/map-connections.cpp)

#include "map-connections.h"
#include "test.h"

TEST(connections_reads_a_pokered_map_header) {
	std::string f = test::join(test::temp_dir(), "ViridianCity.asm");
	test::write_file(f,
		"\tmap_header ViridianCity, VIRIDIAN_CITY, OVERWORLD, NORTH | SOUTH | WEST\n"
		"\tconnection north, Route2, ROUTE_2, 5\n"
		"\tconnection south, Route1, ROUTE_1, 5\n"
		"\tconnection west, Route22, ROUTE_22, -4 ; a comment\n"
		"\tend_map_header\n");
	std::vector<Map_Connection> connections;
	REQUIRE(read_map_connections(f.c_str(), "ViridianCity", connections));
	REQUIRE(connections.size() == 3);
	CHECK_EQ(connections[0].direction, "north");
	CHECK_EQ(connections[0].map_name, "Route2");
	CHECK_EQ(connections[0].map_constant, "ROUTE_2");
	CHECK_EQ(connections[0].offset, 5);
	CHECK_EQ(connections[2].direction, "west");
	CHECK_EQ(connections[2].offset, -4);

	CHECK_EQ(read_map_header_tileset(f.c_str()), "overworld");
}

TEST(connections_reads_only_the_maps_own_lines_in_pokecrystal_attributes) {
	std::string f = test::join(test::temp_dir(), "attributes.asm");
	test::write_file(f,
		"\tmap_attributes NewBarkTown, NEW_BARK_TOWN, $05, WEST | EAST\n"
		"\tconnection west, Route29, ROUTE_29, 0\n"
		"\tconnection east, Route27, ROUTE_27, 0\n"
		"\n"
		"\tmap_attributes CherrygroveCity, CHERRYGROVE_CITY, $35, NORTH | EAST\n"
		"\tconnection north, Route30, ROUTE_30, 5\n"
		"\tconnection east, Route29, ROUTE_29, 0\n"
		"\n"
		"\tmap_attributes PlayersHouse1F, PLAYERS_HOUSE_1F, $00, 0\n");
	std::vector<Map_Connection> connections;

	REQUIRE(read_map_connections(f.c_str(), "CherrygroveCity", connections));
	REQUIRE(connections.size() == 2);
	CHECK_EQ(connections[0].map_name, "Route30");
	CHECK_EQ(connections[1].map_name, "Route29");

	REQUIRE(read_map_connections(f.c_str(), "NewBarkTown", connections));
	REQUIRE(connections.size() == 2);
	CHECK_EQ(connections[0].map_name, "Route29");

	REQUIRE(read_map_connections(f.c_str(), "PlayersHouse1F", connections));
	CHECK(connections.empty());
}

TEST(connections_reads_map_sizes_from_the_constants) {
	std::string f = test::join(test::temp_dir(), "map_constants.asm");
	test::write_file(f,
		"\tmap_const PALLET_TOWN,                   10, 10 ; $00\n"
		"\tmap_const VIRIDIAN_CITY,                 20, 19 ; $01\n"
		"\tmapconst OLD_TOWN, 9, 10 ; older pokered: height first\n");
	int w = 0, h = 0;
	REQUIRE(read_map_size(f.c_str(), "VIRIDIAN_CITY", w, h));
	CHECK_EQ(w, 20);
	CHECK_EQ(h, 19);
	REQUIRE(read_map_size(f.c_str(), "OLD_TOWN", w, h));
	CHECK_EQ(w, 10);
	CHECK_EQ(h, 9);
	CHECK(!read_map_size(f.c_str(), "VIRIDIAN", w, h));
}
