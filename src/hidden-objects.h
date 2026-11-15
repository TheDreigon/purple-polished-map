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
	int16_t x, y;
	std::string line; // the hidden_object line as written, without its indentation
};

// Reads the hidden objects of the map with the given constant (like "VIRIDIAN_FOREST").
// Returns false if the file cannot be read.
bool read_hidden_objects(const char *f, const char *map_constant, std::vector<Hidden_Object> &objects);

// Shows where a hidden object is. It cannot be moved or edited (the program does not write
// hidden_objects.asm); clicks go through it to the map, and hovering shows its line.
class Hidden_Object_Marker : public Fl_Box {
private:
	Hidden_Object _object;
	std::string _tip;
public:
	Hidden_Object_Marker(const Hidden_Object &object);
	inline const Hidden_Object &object(void) const { return _object; }
	void reposition(int dx, int dy, int s);
	void draw(void) override;
	int handle(int event) override;
};

#endif
