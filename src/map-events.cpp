#include <string>
#include <fstream>
#include <sstream>

#pragma warning(push, 0)
#include <FL/filename.H>
#include <FL/fl_utf8.h>
#pragma warning(pop)

#include "map-events.h"

static const std::unordered_map<std::string, Event_Meta> events_meta = {
	// pokered
	{"warp",                  {EventTexture::TX_PURPLE, 'W', 0, false, true}},
	{"sign",                  {EventTexture::TX_RED,    'S', 0, false, false}},
	{"object",                {EventTexture::TX_BLUE,   'O', 1, false, false}},
	{"warp_to",               {EventTexture::TX_ORANGE, 'T', 0, false, false}},
	// pokecrystal
	{"warp_event",            {EventTexture::TX_PURPLE, 'W', 0, false, false}},
	{"coord_event",           {EventTexture::TX_GREEN,  'C', 0, false, false}},
	{"bg_event",              {EventTexture::TX_RED,    'B', 0, false, false}},
	{"object_event",          {EventTexture::TX_BLUE,   'O', 0, false, false}},
	// old pokecrystal
	{"warp_def",              {EventTexture::TX_PURPLE, 'W', 0, true,  true}},
	{"xy_trigger",            {EventTexture::TX_GREEN,  'X', 1, true,  false}},
	{"signpost",              {EventTexture::TX_RED,    'S', 0, true,  false}},
	{"person_event",          {EventTexture::TX_BLUE,   'P', 1, true,  false}},
	// Prism
	{"dummy_warp",            {EventTexture::TX_PURPLE, 'D', 0, true,  false}},
	// Polished Crystal
	{"itemball_event",        {EventTexture::TX_AZURE,  'I', 0, false, false}},
	{"keyitemball_event",     {EventTexture::TX_AZURE,  'K', 0, false, false}},
	{"tmhmball_event",        {EventTexture::TX_AZURE,  'T', 0, false, false}},
	{"cuttree_event",         {EventTexture::TX_AZURE,  'C', 0, false, false}},
	{"fruittree_event",       {EventTexture::TX_AZURE,  'F', 0, false, false}},
	{"strengthboulder_event", {EventTexture::TX_AZURE,  'B', 0, false, false}},
	{"smashrock_event",       {EventTexture::TX_AZURE,  'R', 0, false, false}},
	{"pc_nurse_event",        {EventTexture::TX_AZURE,  'N', 0, false, false}},
	{"mart_clerk_event",      {EventTexture::TX_AZURE,  'M', 0, false, false}},
};

Map_Events::Map_Events() : _map_name(), _events(), _warps(), _result(Result::MAP_EVENTS_NULL),
	_loaded(false), _modified(false), _mod_time(0) {}

Map_Events::~Map_Events() {
	clear();
}

void Map_Events::clear() {
	_map_name[0] = '\0';
	_events.clear();
	_warps.clear();
	_result = Result::MAP_EVENTS_NULL;
	_loaded = false;
	_modified = false;
	_mod_time = 0;
}

void Map_Events::resize_events(int x, int y, int s) const {
	for (Event *e : _events) {
		int ex = x + ((int)e->event_x() + EVENT_MARGIN) * s;
		int ey = y + ((int)e->event_y() + EVENT_MARGIN) * s;
		e->resize(ex, ey, s, s);
	}
}

void Map_Events::refresh_warps() {
	for (Event *e : _events) {
		auto [name, id] = e->warp_destination();
		e->warp_to(name == _map_name && id > 0 ? _warps[id] : NULL);
	}
}

Map_Events::Result Map_Events::read_events(const char *f) {
	std::ifstream ifs;
	open_ifstream(ifs, f);
	if (!ifs.good()) { return (_result = Result::BAD_MAP_EVENTS_FILE); }

	size_t n = 0;
	std::string prelude;
	int warp_id = 1;
	while (ifs.good()) {
		std::string line;
		std::getline(ifs, line);
		n++;
		std::istringstream lss(line);
		std::string token;
		if (!leading_macro(lss, token)) {
			prelude += line + '\n';
			continue;
		}
		const auto it = events_meta.find(token);
		if (it == events_meta.end()) {
			prelude += line + '\n';
			continue;
		}

		if (token == "warp") {
			// warp is a script command in pokecrystal, but with 2 commas, not 3
			// tellg() is -1 when nothing follows the macro name
			std::streampos pos = lss.tellg();
			std::string tail = pos == std::streampos(-1) ? "" : lss.str().substr((size_t)pos);
			remove_comment(tail);
			if (std::count(RANGE(tail), ',') != 3) {
				prelude += line + '\n';
				continue;
			}
		}

		bool is_warp = it->second.is_warp();
		Event *e = new Event(n, prelude, token, it->second, line, is_warp ? warp_id : 0);
		if (is_warp) {
			_warps[warp_id++] = e;
		}
		e->parse(lss);
		_events.push_back(e);
		prelude.clear();
	}
	if (!prelude.empty()) {
		prelude.pop_back(); // remove extra newline
	}
	_coda = prelude;

	remove_dot_ext(f, _map_name);
	refresh_warps();

	_loaded = true;
	_modified = false;
	_mod_time = file_modified(f);

	return (_result = Result::MAP_EVENTS_OK);
}

// The header line that pret's map event files have before each kind of event
static const char *events_header(const std::string &macro) {
	if (macro == "warp_event") { return "def_warp_events"; }
	if (macro == "coord_event") { return "def_coord_events"; }
	if (macro == "bg_event") { return "def_bg_events"; }
	if (macro == "object_event") { return "def_object_events"; }
	return NULL;
}

// Where the line with the given header ends in text (after its newline), or npos
static size_t after_header_line(const std::string &text, const char *header) {
	size_t start = 0;
	while (start < text.size()) {
		size_t end = text.find('\n', start);
		std::string line = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
		remove_comment(line);
		std::istringstream lss(line);
		std::string token;
		if (leading_macro(lss, token) && token == header) {
			return end == std::string::npos ? text.size() : end + 1;
		}
		if (end == std::string::npos) { break; }
		start = end + 1;
	}
	return std::string::npos;
}

bool Map_Events::can_add_event(const std::string &macro) const {
	if (events_meta.find(macro) == events_meta.end()) { return false; }
	for (const Event *e : _events) {
		if (e->macro() == macro) { return true; }
	}
	const char *header = events_header(macro);
	if (!header) { return false; }
	for (const Event *e : _events) {
		if (after_header_line(e->prelude(), header) != std::string::npos) { return true; }
	}
	return after_header_line(_coda, header) != std::string::npos;
}

// Adds an event written as "macro x, y" followed by rest (which starts with its comma).
// It goes after the last event of the same kind or, if there is none, right after the
// def_*_events line for that kind. Returns NULL if the file has neither.
// A new warp always becomes the last warp, so no other warp changes its number.
Event *Map_Events::add_event(const std::string &macro, int16_t x, int16_t y, const std::string &rest) {
	const auto meta = events_meta.find(macro);
	if (meta == events_meta.end()) { return NULL; }

	size_t index = SIZE_MAX;
	std::string prelude;
	for (size_t i = _events.size(); i-- > 0;) {
		if (_events[i]->macro() == macro) {
			index = i + 1;
			break;
		}
	}
	if (index == SIZE_MAX) {
		const char *header = events_header(macro);
		if (!header) { return NULL; }
		// The header is in the text before some event, or after the last one
		for (size_t i = 0; i < _events.size() && index == SIZE_MAX; i++) {
			std::string p = _events[i]->prelude();
			size_t cut = after_header_line(p, header);
			if (cut != std::string::npos) {
				prelude = p.substr(0, cut);
				_events[i]->prelude(p.substr(cut));
				index = i;
			}
		}
		if (index == SIZE_MAX) {
			size_t cut = after_header_line(_coda, header);
			if (cut == std::string::npos) { return NULL; }
			prelude = _coda.substr(0, cut);
			_coda.erase(0, cut);
			index = _events.size();
		}
		if (!prelude.empty() && prelude.back() != '\n') { prelude += '\n'; }
	}

	bool is_warp = meta->second.is_warp();
	int warp_id = 0;
	if (is_warp) {
		warp_id = 1;
		for (size_t i = 0; i < index; i++) {
			if (_events[i]->macro() == macro) { warp_id++; }
		}
	}

	char coords[32] = {};
	snprintf(coords, sizeof(coords), "%2d, %2d", (int)x, (int)y);
	std::string text = macro + " " + coords + rest;
	Event *e = new Event(0, prelude, macro, meta->second, text, warp_id);
	std::istringstream lss("\t" + text);
	std::string token;
	leading_macro(lss, token);
	e->parse(lss);
	_events.insert(_events.begin() + index, e);
	if (is_warp) { _warps[warp_id] = e; }
	refresh_warps();
	_modified = true;
	return e;
}

// Takes an event out of the list (the caller deletes it). The text that came before it
// stays in the file, before the next event.
void Map_Events::remove_event(Event *e) {
	auto it = std::find(RANGE(_events), e);
	if (it == _events.end()) { return; }
	size_t i = it - _events.begin();
	std::string prelude = e->prelude();
	_events.erase(it);
	if (i < _events.size()) {
		_events[i]->prelude(prelude + _events[i]->prelude());
	}
	else {
		_coda = prelude + _coda;
	}
	for (auto w = _warps.begin(); w != _warps.end();) {
		if (w->second == e) { w = _warps.erase(w); }
		else { ++w; }
	}
	refresh_warps();
}

const char *Map_Events::error_message(Result result) {
	switch (result) {
	case Result::MAP_EVENTS_OK:
		return "OK.";
	case Result::BAD_MAP_EVENTS_FILE:
		return "Cannot open file.";
	case Result::MAP_EVENTS_NULL:
		return "No *.asm file chosen.";
	default:
		return "Unspecified error.";
	}
}

bool Map_Events::write_event_script(const char *f) {
	FILE *file = fl_fopen(f, "wb");
	if (!file) { return false; }
	for (Event *e : _events) {
		fputs(e->prelude().c_str(), file);
		fputc('\t', file);
		fputs(e->tip().c_str(), file);
		fputc('\n', file);
	}
	fputs(_coda.c_str(), file);
	fclose(file);
	_mod_time = file_modified(f);
	return true;
}
