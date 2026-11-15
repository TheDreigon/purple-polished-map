#ifndef COLLISION_TILES_H
#define COLLISION_TILES_H

#include <string>
#include <vector>

// pokered and pokeyellow say which tiles can be walked on, per tileset, in
// data/tilesets/collision_tile_ids.asm:
//     RedsHouse1_Coll::
//     RedsHouse2_Coll::
//         coll_tiles $01, $02, $03, $11, $12, $13, $14, $1c, $1a
// Reads the list after the first "<label>_Coll::" for any of the given labels (like "RedsHouse1")
// into passable (256 entries). Returns false if the file or the list is not there.
bool read_passable_tiles(const char *f, const std::vector<std::string> &labels, std::vector<bool> &passable);

// The labels that a project's gfx/tilesets.asm gives to a graphics file: for "reds_house",
// {"RedsHouse1", "RedsHouse2"}. Without gfx/tilesets.asm, or if it does not mention the file,
// the file's own name in the same style ("Overworld" for "overworld").
std::vector<std::string> tileset_labels(const char *root, const char *file_name);

#endif
