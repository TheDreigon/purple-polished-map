#ifndef MAP_CONNECTIONS_H
#define MAP_CONNECTIONS_H

#include <string>
#include <vector>

#pragma warning(push, 0)
#include <FL/Fl_Box.H>
#pragma warning(pop)

#include "utils.h"

// A connection to the map next to this one, as written in pokered's data/maps/headers/*.asm or
// pokecrystal's data/maps/attributes.asm:
//     connection north, Route2, ROUTE_2, 5
// The offset is where the other map starts along this map's edge, in blocks: its column for a
// north or south connection, its row for a west or east one.
struct Map_Connection {
	std::string direction, map_name, map_constant;
	int offset;
};

// Reads the connections of the map with the given label (like "ViridianCity"). In a file with
// several maps, only the lines after that map's map_header or map_attributes line count.
bool read_map_connections(const char *f, const char *map_label, std::vector<Map_Connection> &connections);

// Reads a map's width and height in blocks from the map constants:
//     map_const VIRIDIAN_CITY, 20, 18 (pokered and pokecrystal: width, height)
//     mapconst VIRIDIAN_CITY, 18, 20  (older pokered: height, width)
bool read_map_size(const char *f, const char *map_constant, int &w, int &h);

// Reads the tileset constant of a pokered map header, in lowercase ("overworld"), or "".
std::string read_map_header_tileset(const char *f);

// Shows, in the margin around the map, the edge of a connected map: the three rows or columns
// of it that touch this map. It is only a view; clicks go through it.
class Connection_View : public Fl_Box {
private:
	Map_Connection _connection;
	std::vector<uint8_t> _ids; // the connected map's blocks
	int _cw, _ch;              // the connected map's size
	int _bx, _by, _bw, _bh;    // the shown part, in blocks, relative to this map's top left
	int _sx, _sy;              // which block of the connected map is at (_bx, _by)
	bool _drawable;            // false if the connected map uses another tileset
	std::string _tip;
public:
	Connection_View(const Map_Connection &c, const std::vector<uint8_t> &ids, int cw, int ch, int w, int h, bool drawable);
	inline bool empty(void) const { return _bw <= 0 || _bh <= 0; }
	void reposition(int dx, int dy, int s);
	void draw(void) override;
	int handle(int event) override;
};

#endif
