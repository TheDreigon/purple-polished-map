#include <cstdio>

#include "map.h"
#include "parse-asm.h"

void Map_Attributes::clear() {
	group = 0;
	environment.clear();
	landmark.clear();
	palette.clear();
}

Map::Map() : _attributes(), _width(0), _height(0), _blocks(NULL), _result(Result::MAP_NULL), _modified(false), _mod_time(0),
	_history(), _future() {}

Map::~Map() {
	clear();
}

void Map::size(uint8_t w, uint8_t h) {
	clear();
	_width = w;
	_height = h;
	_blocks = new Block *[size()]();
}

// Changes the dimensions of a loaded map; the caller then places the blocks.
// Unlike size(), which starts a map from scratch, this keeps what is known about
// the map and its file: its attributes, its result, when its file was last read,
// and its Undo and Redo history (whose snapshots know their own size).
void Map::resize(uint8_t w, uint8_t h) {
	delete [] _blocks;
	_width = w;
	_height = h;
	_blocks = new Block *[size()]();
	_modified = true;
}

void Map::block(uint8_t x, uint8_t y, Block *b) {
	_blocks[(size_t)y * _width + (size_t)x] = b;
}

Block *Map::block_under(Event *e) {
	if (!e) { return NULL; }
	int16_t bx = e->event_x() / 2, by = e->event_y() / 2;
	return bx >= 0 && by >= 0 && bx < _width && by < _height ? block((uint8_t)bx, (uint8_t)by) : NULL;
}

bool Map::uses_block(uint8_t id) const {
	size_t n = size();
	for (size_t i = 0; i < n; i++) {
		if (_blocks[i] && _blocks[i]->id() == id) { return true; }
	}
	return false;
}

void Map::clear() {
	delete [] _blocks;
	_blocks = NULL;
	_attributes.clear();
	_width = _height = 0;
	_result = Result::MAP_NULL;
	_modified = false;
	_mod_time = 0;
	_history.clear();
	_future.clear();
}

void Map::resize_blocks(int x, int y, int s) const {
	size_t n = size();
	for (size_t i = 0; i < n; i++) {
		Block *b = block(i);
		int bx = x + ((int)b->col() + MAP_MARGIN) * s, by = y + ((int)b->row() + MAP_MARGIN) * s;
		b->resize(bx, by, s, s);
	}
}

Map::Snapshot Map::snapshot() const {
	Snapshot s;
	s.width = _width;
	s.height = _height;
	s.ids.resize(size());
	for (size_t i = 0; i < size(); i++) {
		s.ids[i] = block(i)->id();
	}
	return s;
}

// Remembers the blocks before an edit that keeps the map's size
void Map::remember() {
	_future.clear();
	while (_history.size() >= MAX_HISTORY_SIZE) { _history.pop_front(); }
	_history.push_back(snapshot());
}

// Remembers the blocks and where the events are before an edit that changes the map's size
void Map::remember(const Event_Coords &events) {
	remember();
	_history.back().has_events = true;
	_history.back().events = events;
}

// Whether Undo or Redo changes the map's size; then the caller rebuilds the map with
// take_undo() or take_redo() instead of calling undo() or redo()
bool Map::undo_resizes() const {
	return !_history.empty() && (_history.back().width != _width || _history.back().height != _height);
}

bool Map::redo_resizes() const {
	return !_future.empty() && (_future.back().width != _width || _future.back().height != _height);
}

// Gives the map as it was before the last edit, for the caller to rebuild it, and keeps the
// current map (with where its events are) for Redo
Map::Snapshot Map::take_undo(const Event_Coords &events) {
	Snapshot now = snapshot();
	now.has_events = true;
	now.events = events;
	Snapshot then = _history.back();
	_history.pop_back();
	while (_future.size() >= MAX_HISTORY_SIZE) { _future.pop_front(); }
	_future.push_back(now);
	_modified = true;
	return then;
}

Map::Snapshot Map::take_redo(const Event_Coords &events) {
	Snapshot now = snapshot();
	now.has_events = true;
	now.events = events;
	Snapshot next = _future.back();
	_future.pop_back();
	while (_history.size() >= MAX_HISTORY_SIZE) { _history.pop_front(); }
	_history.push_back(now);
	_modified = true;
	return next;
}

// Undoes the last edit when it kept the map's size
void Map::undo() {
	if (_history.empty() || undo_resizes()) { return; }
	while (_future.size() >= MAX_HISTORY_SIZE) { _future.pop_front(); }
	_future.push_back(snapshot());

	const Snapshot &prev = _history.back();
	for (size_t i = 0; i < size(); i++) {
		block(i)->id(prev.ids[i]);
	}
	_history.pop_back();

	// The blocks no longer match the saved file, even if the map was just saved
	_modified = true;
}

void Map::redo() {
	if (_future.empty() || redo_resizes()) { return; }
	while (_history.size() >= MAX_HISTORY_SIZE) { _history.pop_front(); }
	_history.push_back(snapshot());

	const Snapshot &next = _future.back();
	for (size_t i = 0; i < size(); i++) {
		block(i)->id(next.ids[i]);
	}
	_future.pop_back();

	_modified = true;
}

Map::Result Map::read_blocks(const char *f) {
	if (ends_with_ignore_case(f, ".map")) { return read_asm_blocks(f); }

	bool too_long = false;

	FILE *file = fl_fopen(f, "rb");
	if (file == NULL) { return (_result = Result::MAP_BAD_FILE); } // cannot load file

	uint8_t *data = new uint8_t[size() + 1];
	size_t c = fread(data, 1, size() + 1, file);
	fclose(file);
	_mod_time = file_modified(f);
	if (c < size()) { delete [] data; return (_result = Result::MAP_TOO_SHORT); } // too-short blk
	if (c == size() + 1) { too_long = true; }

	for (uint8_t y = 0; y < (size_t)_height; y++) {
		for (uint8_t x = 0; x < (size_t)_width; x++) {
			size_t i = (size_t)y * _width + (size_t)x;
			uint8_t id = data[i];
			_blocks[i] = new Block(y, x, id);
		}
	}

	delete [] data;
	return (_result = too_long ? Result::MAP_TOO_LONG : Result::MAP_OK);
}

Map::Result Map::read_asm_blocks(const char *f) {
	Parsed_Asm data(f);
	if (data.result() != Parsed_Asm::Result::ASM_OK) {
		return (_result = Result::MAP_BAD_FILE); // cannot parse file
	}

	bool too_long = false;
	size_t c = data.size();
	if (c < size()) { return (_result = Result::MAP_TOO_SHORT); } // too-short CHR
	if (c > size()) { too_long = true; }

	FILE *file = fl_fopen(f, "rb");
	if (file == NULL) { return (_result = Result::MAP_BAD_FILE); } // cannot load file

	for (uint8_t y = 0; y < (size_t)_height; y++) {
		for (uint8_t x = 0; x < (size_t)_width; x++) {
			size_t i = (size_t)y * _width + (size_t)x;
			uint8_t id = data.get(i);
			_blocks[i] = new Block(y, x, id);
		}
	}

	fclose(file);
	_mod_time = file_modified(f);

	return (_result = too_long ? Result::MAP_TOO_LONG : Result::MAP_OK);
}

bool Map::write_blocks(const char *f) {
	FILE *file = fl_fopen(f, "wb");
	if (!file) { return false; }
	size_t n = size();
	for (size_t i = 0; i < n; i++) {
		uint8_t id = block(i)->id();
		fputc(id, file);
	}
	fclose(file);
	_mod_time = file_modified(f);
	return true;
}

// The size of a w x h map after the edit. Returns false if the edit is not possible:
// a map is 1 to 255 blocks wide and high.
bool Map::edited_size(Edit e, int w, int h, int &nw, int &nh) {
	nw = w;
	nh = h;
	switch (e) {
	case Edit::INSERT_ROW: nh++; break;
	case Edit::DELETE_ROW: nh--; break;
	case Edit::INSERT_COLUMN: nw++; break;
	case Edit::DELETE_COLUMN: nw--; break;
	}
	return nw >= 1 && nh >= 1 && nw <= 255 && nh <= 255;
}

// For each block of the edited map, row by row, the index of the block of the old map that it
// shows. A row or column is inserted before row or column "at" (at the end if "at" is past the
// last one) and starts as a copy of the one it is inserted before, or of the last one at the end.
// A deleted row or column is "at" itself. Returns nothing if the edit is not possible.
std::vector<size_t> Map::edited_sources(Edit e, int w, int h, int at) {
	std::vector<size_t> sources;
	int nw, nh;
	if (!edited_size(e, w, h, nw, nh)) { return sources; }
	bool rows = e == Edit::INSERT_ROW || e == Edit::DELETE_ROW;
	int n = rows ? h : w;
	if (e == Edit::INSERT_ROW || e == Edit::INSERT_COLUMN) { at = std::clamp(at, 0, n); }
	else { at = std::clamp(at, 0, n - 1); }
	for (int y = 0; y < nh; y++) {
		for (int x = 0; x < nw; x++) {
			int ox = x, oy = y;
			int &o = rows ? oy : ox;
			switch (e) {
			case Edit::INSERT_ROW:
			case Edit::INSERT_COLUMN:
				if (o > at) { o--; }
				else if (o == at && at == n) { o = n - 1; } // the copy of the last one, at the end
				break;
			case Edit::DELETE_ROW:
			case Edit::DELETE_COLUMN:
				if (o >= at) { o++; }
				break;
			}
			sources.push_back((size_t)oy * w + ox);
		}
	}
	return sources;
}

// Moves an event (whose coordinates are in half blocks) along with the blocks around it.
// Events on a deleted row or column stay where they are, on the row or column that takes its
// place. Returns whether the event moved.
bool Map::edited_event_coords(Edit e, int at, int16_t &ex, int16_t &ey) {
	int16_t &c = (e == Edit::INSERT_ROW || e == Edit::DELETE_ROW) ? ey : ex;
	int16_t before = c;
	switch (e) {
	case Edit::INSERT_ROW:
	case Edit::INSERT_COLUMN:
		if (c >= at * 2) { c = std::min((int16_t)(c + 2), MAX_EVENT_COORD); }
		break;
	case Edit::DELETE_ROW:
	case Edit::DELETE_COLUMN:
		if (c >= at * 2 + 2) { c = std::max((int16_t)(c - 2), MIN_EVENT_COORD); }
		break;
	}
	return c != before;
}

// Explains a .blk file that does not match the chosen size, and suggests sizes that do match
std::string Map::size_hint(uint8_t w, uint8_t h, size_t file_size) {
	size_t n = (size_t)w * (size_t)h;
	std::ostringstream ss;
	ss << "The map is " << (int)w << " x " << (int)h << " (" << n << " blocks),\n"
		"but the file has " << file_size << " bytes.";
	std::vector<std::pair<int, int>> fits;
	for (int fw = 1; fw <= 255; fw++) {
		if (file_size % fw) { continue; }
		size_t fh = file_size / fw;
		if (fh >= 1 && fh <= 255) { fits.emplace_back(fw, (int)fh); }
	}
	// The sizes closest to the chosen width are the likeliest
	std::stable_sort(RANGE(fits), [w](const auto &a, const auto &b) {
		return std::abs(a.first - (int)w) < std::abs(b.first - (int)w);
	});
	if (fits.size() > 4) { fits.resize(4); }
	if (!fits.empty()) {
		ss << "\n\nSizes that fit " << file_size << " bytes: ";
		for (size_t i = 0; i < fits.size(); i++) {
			if (i) { ss << ", "; }
			ss << fits[i].first << " x " << fits[i].second;
		}
	}
	return ss.str();
}

const char *Map::error_message(Result result) {
	switch (result) {
	case Result::MAP_OK:
		return "OK.";
	case Result::MAP_BAD_FILE:
		return "Cannot open file.";
	case Result::MAP_TOO_SHORT:
		return "File ends too early.";
	case Result::MAP_TOO_LONG:
		return "The .blk file is larger than the specified size.";
	case Result::MAP_NULL:
		return "No *.blk file chosen.";
	default:
		return "Unspecified error.";
	}
}
