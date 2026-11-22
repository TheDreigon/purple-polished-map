#ifndef MAP_H
#define MAP_H

#include <deque>
#include <vector>

#include "utils.h"
#include "map-buttons.h"
#include "event.h"

#define MAX_HISTORY_SIZE 100

struct Map_Attributes {
public:
	uint8_t group;
	std::string environment;
	std::string landmark;
	std::string palette;
public:
	inline Map_Attributes() : group(), environment(), landmark(), palette() {}
	void clear(void);
};

class Map {
public:
	// What the map was like before a change, for Undo and Redo. A change of size (Resize Map,
	// inserting or deleting rows and columns) also keeps where the events were, since it moves them.
	typedef std::vector<std::pair<int16_t, int16_t>> Event_Coords;
	struct Snapshot {
		uint8_t width = 0, height = 0;
		std::vector<uint8_t> ids;
		bool has_events = false;
		Event_Coords events;
	};
	enum class Result { MAP_OK, MAP_BAD_FILE, MAP_TOO_SHORT, MAP_TOO_LONG, MAP_NULL };
	// Inserting or deleting a whole row or column in the middle of the map
	enum class Edit { INSERT_ROW, DELETE_ROW, INSERT_COLUMN, DELETE_COLUMN };
private:
	Map_Attributes _attributes;
	uint8_t _width, _height;
	Block **_blocks;
	Result _result;
	bool _modified;
	std::deque<Snapshot> _history, _future;
	int64_t _mod_time;
public:
	Map();
	~Map();
	inline uint8_t width(void) const { return _width; }
	inline uint8_t height(void) const { return _height; }
	inline Map_Attributes attributes(void) const { return _attributes; }
	inline void attributes(const Map_Attributes &a) { _attributes = a; }
	inline uint8_t group(void) const { return _attributes.group; }
	inline void group(uint8_t g) { _attributes.group = g; }
	inline std::string environment(void) const { return _attributes.environment; }
	inline void environment(const std::string &e) { _attributes.environment = e; }
	inline std::string landmark(void) const { return _attributes.landmark; }
	inline void landmark(const std::string &l) { _attributes.landmark = l; }
	inline std::string palette(void) const { return _attributes.palette; }
	inline void palette(const std::string &p) { _attributes.palette = p; }
	inline bool is_outside(void) const {
		return _attributes.environment == "TOWN" || _attributes.environment == "ROUTE" ||
			_attributes.environment == "1" || _attributes.environment == "2"; // TPP:AC uses numbers
	}
	inline int16_t max_event_x(void) const {
		return _width <= (uint8_t)((MAX_EVENT_COORD - EVENT_MARGIN + 1) / 2) ? (int16_t)_width * 2 + EVENT_MARGIN - 1 : MAX_EVENT_COORD;
	}
	inline int16_t max_event_y(void) const {
		return _height <= (uint8_t)((MAX_EVENT_COORD - EVENT_MARGIN + 1) / 2) ? (int16_t)_height * 2 + EVENT_MARGIN - 1 : MAX_EVENT_COORD;
	}
	void size(uint8_t w, uint8_t h);
	void resize(uint8_t w, uint8_t h);
	inline size_t size(void) const { return (size_t)_width * (size_t)_height; }
	inline Block *block(uint8_t x, uint8_t y) const { return _blocks[(size_t)y * _width + (size_t)x]; }
	inline Block *block(size_t i) const { return _blocks[i]; }
	Block *block_under(Event *e);
	bool uses_block(uint8_t id) const;
	void block(uint8_t x, uint8_t y, Block *b);
	inline Result result(void) const { return _result; }
	inline bool modified(void) const { return _modified; }
	inline void modified(bool m) { _modified = m; }
	inline bool other_modified(const char *f) const { return file_modified(f) > _mod_time; }
	inline void accept_other_modified(const char *f) { _mod_time = file_modified(f); }
	inline bool can_undo(void) const { return !_history.empty(); }
	inline bool can_redo(void) const { return !_future.empty(); }
	void clear();
	void resize_blocks(int x, int y, int s) const;
	void remember(void);
	void remember(const Event_Coords &events);
	bool undo_resizes(void) const;
	bool redo_resizes(void) const;
	Snapshot take_undo(const Event_Coords &events);
	Snapshot take_redo(const Event_Coords &events);
	void undo(void);
	void redo(void);
private:
	Snapshot snapshot(void) const;
public:
	Result read_blocks(const char *f);
	bool write_blocks(const char *f);
private:
	Result read_asm_blocks(const char *f);
public:
	static const char *error_message(Result result);
	static std::string size_hint(uint8_t w, uint8_t h, size_t file_size);
	static bool edited_size(Edit e, int w, int h, int &nw, int &nh);
	static std::vector<size_t> edited_sources(Edit e, int w, int h, int at);
	static bool edited_event_coords(Edit e, int at, int16_t &ex, int16_t &ey);
};

#endif
