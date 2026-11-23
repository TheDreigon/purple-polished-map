#ifndef HIDDEN_OBJECTS_H
#define HIDDEN_OBJECTS_H

#include <string>
#include <vector>

#pragma warning(push, 0)
#include <FL/Fl_Box.H>
#pragma warning(pop)

#include "utils.h"

// A hidden object of a pokered or pokeyellow map, as listed in data/events/hidden_objects.asm:
//     HiddenObjectMaps:
//         dbw VIRIDIAN_FOREST, ViridianForestHiddenObjects
//     ...
//     ViridianForestHiddenObjects:
//         hidden_object  1, 18, LEAF_STONE, HiddenItems
//         db -1 ; end
// x and y are in the same units as event coordinates.
struct Hidden_Object {
	int16_t x = 0, y = 0;             // where it is now
	int16_t saved_x = 0, saved_y = 0; // where its file puts it
	std::string line;    // the hidden_object line as its file has it, without its indentation
	std::string routine; // what it does, like "HiddenItems"
	bool movable = false; // its coordinates are plain numbers, which can be rewritten
	inline bool moved(void) const { return x != saved_x || y != saved_y; }
	std::string current_line(void) const; // the line with the coordinates it has now
};

// Reads the hidden objects of the map with the given constant (like "VIRIDIAN_FOREST").
// Returns false if the file cannot be read.
bool read_hidden_objects(const char *f, const char *map_constant, std::vector<Hidden_Object> &objects);

// Rewrites the two coordinates of the macro a line starts with, from its argument "first"
// (0 for the first one), and leaves the rest of the line as it was. A narrower or wider number
// takes or gives back spaces before it, so the columns stay lined up. Returns false if the line
// does not use that macro or has too few arguments.
bool rewrite_coordinates(std::string &line, const char *macro, size_t first, int16_t x, int16_t y);

// Writes where the moved hidden objects of a map are now (objects as read_hidden_objects gave
// them), changing only their coordinates. The game finds hidden items and hidden coins by their
// map and coordinates in hidden_item_coords.asm and hidden_coins.asm (next to hidden_objects.asm),
// so their rows there are rewritten too. If hidden_objects.asm does not have the lines it had
// when it was read, nothing is written and error says why. A row that cannot be found, or that
// is not the only one, is left alone and added to warnings. On success, the objects are saved.
bool write_hidden_objects(const char *f, const char *map_constant, std::vector<Hidden_Object> &objects,
	std::vector<std::string> &written, std::vector<std::string> &warnings, std::string &error);

// Shows where a hidden object is. In Events Mode it can be dragged to another square, unless an
// event is on the same square (the event comes first) or its coordinates are not plain numbers.
// Other clicks go through it to the map, and hovering shows its line.
class Hidden_Object_Marker : public Fl_Box {
private:
	Hidden_Object _object;
	std::string _tip;
	bool _dragging = false;
public:
	Hidden_Object_Marker(const Hidden_Object &object);
	inline const Hidden_Object &object(void) const { return _object; }
	void object(const Hidden_Object &object);
	void coords(int16_t x, int16_t y);
	void reposition(int dx, int dy, int s);
	void draw(void) override;
	int handle(int event) override;
private:
	void update_tooltip(void);
};

#endif
