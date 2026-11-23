#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>

#pragma warning(push, 0)
#include <FL/fl_draw.H>
#include <FL/fl_utf8.h>
#include <FL/filename.H>
#pragma warning(pop)

#include "themes.h"
#include "event.h"
#include "hidden-objects.h"
#include "main-window.h"

typedef std::vector<std::pair<size_t, size_t>> Arg_Ranges;

// A plain number written for RGBDS: $2A or 0x2A (hexadecimal), %101 or 0b101 (binary), or 42
static bool parse_plain_number(std::string s, int16_t &v) {
	trim(s);
	int base = 10;
	size_t skip = 0;
	if (!s.empty() && s[0] == '$') { base = 16; skip = 1; }
	else if (!s.empty() && s[0] == '%') { base = 2; skip = 1; }
	else if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { base = 16; skip = 2; }
	else if (s.size() > 2 && s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) { base = 2; skip = 2; }
	std::string digits = s.substr(skip);
	const char *allowed = base == 16 ? "0123456789abcdefABCDEF" : base == 2 ? "01" : "0123456789";
	if (digits.empty() || digits.size() > 4 || digits.find_first_not_of(allowed) != std::string::npos) { return false; }
	long n = strtol(digits.c_str(), NULL, base);
	if (n > INT16_MAX) { return false; }
	v = (int16_t)n;
	return true;
}

// Writes v as old was written: in hexadecimal with as many digits if it was, or else in decimal
static std::string format_like(const std::string &old, int16_t v) {
	size_t prefix = 0;
	if (!old.empty() && old[0] == '$') { prefix = 1; }
	else if (old.size() > 2 && old[0] == '0' && (old[1] == 'x' || old[1] == 'X')) { prefix = 2; }
	char buffer[16] = {};
	if (!prefix || v < 0) {
		sprintf(buffer, "%d", (int)v);
		return buffer;
	}
	bool upper = old.find_first_of("ABCDEF", prefix) != std::string::npos;
	sprintf(buffer, upper ? "%0*X" : "%0*x", (int)(old.size() - prefix), (unsigned)v);
	return old.substr(0, prefix) + buffer;
}

static std::string without_cr(const std::string &line) {
	return !line.empty() && line.back() == '\r' ? line.substr(0, line.size() - 1) : line;
}

// Where the arguments of the macro a line starts with are, up to its comment: [begin, end) of
// each one, with the spaces around it
static bool find_macro_args(const std::string &line, const char *macro, Arg_Ranges &args) {
	args.clear();
	size_t end = std::min(line.find(';'), line.find('\r'));
	if (end == std::string::npos) { end = line.size(); }
	size_t p = line.find_first_not_of(" \t");
	if (p == std::string::npos || p >= end) { return false; }
	size_t q = std::min(line.find_first_of(" \t", p), end);
	size_t n = strlen(macro);
	if (q - p != n || line.compare(p, n, macro)) { return false; }
	size_t start = q;
	for (size_t i = q; i < end; i++) {
		if (line[i] == ',') {
			args.emplace_back(start, i);
			start = i + 1;
		}
	}
	args.emplace_back(start, end);
	return true;
}

static std::string arg_text(const std::string &line, const Arg_Ranges &args, size_t i) {
	if (i >= args.size()) { return ""; }
	std::string s = line.substr(args[i].first, args[i].second - args[i].first);
	trim(s);
	return s;
}

// Replaces the number in an argument (given with the spaces around it)
static bool replace_number(std::string &line, const std::pair<size_t, size_t> &arg, int16_t v) {
	std::string raw = line.substr(arg.first, arg.second - arg.first);
	size_t vb = raw.find_first_not_of(" \t");
	if (vb == std::string::npos) { return false; }
	size_t ve = raw.find_last_not_of(" \t") + 1;
	std::string lead = raw.substr(0, vb), old = raw.substr(vb, ve - vb), trail = raw.substr(ve);
	std::string text = format_like(old, v);
	if (lead.find('\t') == std::string::npos) {
		// Keep the argument as wide as it was, so the columns stay lined up
		size_t width = lead.size() + old.size();
		size_t least = lead.empty() ? 0 : 1;
		lead.assign(std::max(least, width > text.size() ? width - text.size() : 0), ' ');
	}
	line.replace(arg.first, raw.size(), lead + text + trail);
	return true;
}

bool rewrite_coordinates(std::string &line, const char *macro, size_t first, int16_t x, int16_t y) {
	Arg_Ranges args;
	if (!find_macro_args(line, macro, args) || args.size() < first + 2) { return false; }
	std::string copy(line);
	// The second one first, so the position of the first one stays right
	if (!replace_number(copy, args[first + 1], y) || !replace_number(copy, args[first], x)) { return false; }
	line = copy;
	return true;
}

std::string Hidden_Object::current_line() const {
	std::string s(line);
	if (moved() && movable) { rewrite_coordinates(s, "hidden_object", 0, x, y); }
	return s;
}

// Reads a file's lines as they are, each with any '\r' it ends with
static bool read_lines(const char *f, std::vector<std::string> &lines, bool &final_newline) {
	lines.clear();
	final_newline = true;
	FILE *file = fl_fopen(f, "rb");
	if (!file) { return false; }
	std::string bytes;
	char buffer[4096];
	for (size_t n; (n = fread(buffer, 1, sizeof(buffer), file)) > 0;) {
		bytes.append(buffer, n);
	}
	bool ok = !ferror(file);
	fclose(file);
	if (!ok) { return false; }
	size_t start = 0;
	for (size_t i = 0; i < bytes.size(); i++) {
		if (bytes[i] == '\n') {
			lines.push_back(bytes.substr(start, i - start));
			start = i + 1;
		}
	}
	if (start < bytes.size()) {
		lines.push_back(bytes.substr(start));
		final_newline = false;
	}
	return true;
}

static bool write_lines(const char *f, const std::vector<std::string> &lines, bool final_newline) {
	FILE *file = fl_fopen(f, "wb");
	if (!file) { return false; }
	bool ok = true;
	for (size_t i = 0; i < lines.size(); i++) {
		ok = fputs(lines[i].c_str(), file) >= 0 && ok;
		if (i + 1 < lines.size() || final_newline) {
			ok = fputc('\n', file) != EOF && ok;
		}
	}
	ok = fclose(file) == 0 && ok;
	return ok;
}

static bool parse_hidden_object(const std::string &line, Hidden_Object &object) {
	std::string code = without_cr(line);
	Arg_Ranges args;
	if (!find_macro_args(code, "hidden_object", args) || args.size() < 2) { return false; }
	std::string x = arg_text(code, args, 0), y = arg_text(code, args, 1);
	if (x.empty() || y.empty()) { return false; }
	int16_t vx = 0, vy = 0;
	bool plain_x = parse_plain_number(x, vx), plain_y = parse_plain_number(y, vy);
	object.x = object.saved_x = vx;
	object.y = object.saved_y = vy;
	object.movable = plain_x && plain_y;
	object.routine = arg_text(code, args, 3);
	object.line = code;
	trim(object.line);
	return true;
}

// The lines with the hidden objects of a map, in order
static std::vector<size_t> hidden_object_lines(const std::vector<std::string> &lines, const char *map_constant) {
	// "dbw MAP_CONSTANT, Label" gives the label of each map's list
	std::vector<std::string> labels;
	for (const std::string &line : lines) {
		std::string code = without_cr(line);
		remove_comment(code);
		std::istringstream lss(code);
		std::string macro;
		if (!leading_macro(lss, macro, "dbw")) { continue; }
		std::string map, label;
		std::getline(lss, map, ',');
		std::getline(lss, label);
		trim(map);
		trim(label);
		if (map == map_constant && !label.empty() && std::find(labels.begin(), labels.end(), label) == labels.end()) {
			labels.push_back(label);
		}
	}

	// Each list starts with its label and ends at "db -1" or at the next label
	std::vector<size_t> indices;
	for (const std::string &label : labels) {
		bool in_list = false;
		for (size_t i = 0; i < lines.size(); i++) {
			std::string code = without_cr(lines[i]);
			remove_comment(code);
			if (!code.empty() && code[0] != ' ' && code[0] != '\t') {
				size_t colon = code.find(':');
				if (colon != std::string::npos) {
					std::string name = code.substr(0, colon);
					trim(name);
					if (in_list) { break; }
					in_list = name == label;
					continue;
				}
			}
			if (!in_list) { continue; }
			std::istringstream lss(code);
			std::string macro;
			if (!leading_macro(lss, macro)) { continue; }
			if (macro == "db") {
				std::string value;
				std::getline(lss, value);
				trim(value);
				if (value == "-1" || value == "$ff" || value == "$FF") { break; }
				continue;
			}
			Hidden_Object object;
			if (macro == "hidden_object" && parse_hidden_object(lines[i], object)) {
				indices.push_back(i);
			}
		}
	}
	return indices;
}

bool read_hidden_objects(const char *f, const char *map_constant, std::vector<Hidden_Object> &objects) {
	objects.clear();
	std::vector<std::string> lines;
	bool final_newline;
	if (!read_lines(f, lines, final_newline)) { return false; }
	for (size_t i : hidden_object_lines(lines, map_constant)) {
		Hidden_Object object;
		parse_hidden_object(lines[i], object);
		objects.push_back(object);
	}
	return true;
}

// The lists that the game searches by map and coordinates, for the routines that use them
struct Coords_List {
	const char *routine, *filename, *macro, *what;
};

static const Coords_List COORDS_LISTS[] = {
	{"HiddenItems", "hidden_item_coords.asm", "hidden_item", "hidden item"},
	{"HiddenCoins", "hidden_coins.asm", "hidden_coin", "hidden coins"},
};

// The rows of such a list for a map and coordinates, like "hidden_item VIRIDIAN_FOREST, 1, 18"
static std::vector<size_t> coords_rows(const std::vector<std::string> &lines, const char *macro, const char *map_constant,
	int16_t x, int16_t y) {
	std::vector<size_t> rows;
	for (size_t i = 0; i < lines.size(); i++) {
		Arg_Ranges args;
		if (!find_macro_args(lines[i], macro, args) || args.size() < 3) { continue; }
		int16_t rx, ry;
		if (arg_text(lines[i], args, 0) == map_constant && parse_plain_number(arg_text(lines[i], args, 1), rx) &&
			parse_plain_number(arg_text(lines[i], args, 2), ry) && rx == x && ry == y) {
			rows.push_back(i);
		}
	}
	return rows;
}

static std::string coords_text(int16_t x, int16_t y) {
	return std::to_string(x) + ", " + std::to_string(y);
}

bool write_hidden_objects(const char *f, const char *map_constant, std::vector<Hidden_Object> &objects,
	std::vector<std::string> &written, std::vector<std::string> &warnings, std::string &error) {
	written.clear();
	warnings.clear();
	error.clear();
	std::string basename = fl_filename_name(f);
	std::string changed = basename + " was modified by another program!\n\n"
		"The hidden objects of " + map_constant + " are not the ones that were read,\n"
		"so nothing was saved. Open the map again to see them as they are now.";

	std::vector<std::string> lines;
	bool final_newline;
	if (!read_lines(f, lines, final_newline)) {
		error = "Could not read " + basename + "!";
		return false;
	}
	std::vector<size_t> indices = hidden_object_lines(lines, map_constant);
	if (indices.size() != objects.size()) {
		error = changed;
		return false;
	}
	std::vector<size_t> moved;
	for (size_t k = 0; k < objects.size(); k++) {
		const Hidden_Object &o = objects[k];
		std::string code = without_cr(lines[indices[k]]);
		trim(code);
		if (code != o.line) {
			error = changed;
			return false;
		}
		if (!o.moved() || !o.movable) { continue; }
		if (!rewrite_coordinates(lines[indices[k]], "hidden_object", 0, o.x, o.y)) {
			error = changed;
			return false;
		}
		moved.push_back(k);
	}
	if (moved.empty()) { return true; }

	// Find every row before rewriting any, so that objects that swap places, or move onto each
	// other's old square, do not find each other's rewritten rows
	struct Coords_File {
		std::string filename;
		std::vector<std::string> lines;
		bool final_newline = true;
	};
	std::vector<Coords_File> coords_files;
	std::string directory(f, fl_filename_name(f) - f);
	for (const Coords_List &list : COORDS_LISTS) {
		std::vector<size_t> ks;
		for (size_t k : moved) {
			if (objects[k].routine == list.routine) { ks.push_back(k); }
		}
		if (ks.empty()) { continue; }
		Coords_File cf;
		cf.filename = directory + list.filename;
		if (!read_lines(cf.filename.c_str(), cf.lines, cf.final_newline)) {
			warnings.push_back(std::string("Could not read ") + list.filename + ", so the " + list.what +
				" of " + map_constant + " that moved were not updated there.");
			continue;
		}
		std::vector<std::pair<size_t, size_t>> rewrites; // row, object
		for (size_t k : ks) {
			const Hidden_Object &o = objects[k];
			std::string where = std::string("The ") + list.what + " moved from " + coords_text(o.saved_x, o.saved_y) +
				" to " + coords_text(o.x, o.y) + " was not updated in " + list.filename + ": ";
			size_t sharing = std::count_if(objects.begin(), objects.end(), [&](const Hidden_Object &p) {
				return p.routine == o.routine && p.saved_x == o.saved_x && p.saved_y == o.saved_y;
			});
			std::vector<size_t> rows = coords_rows(cf.lines, list.macro, map_constant, o.saved_x, o.saved_y);
			if (sharing > 1) {
				warnings.push_back(where + "another one was on the same square.");
			}
			else if (rows.empty()) {
				warnings.push_back(where + "it has no row for " + map_constant + " at " + coords_text(o.saved_x, o.saved_y) + ".");
			}
			else if (rows.size() > 1) {
				warnings.push_back(where + "it has " + std::to_string(rows.size()) + " rows for " + map_constant + " at " +
					coords_text(o.saved_x, o.saved_y) + ".");
			}
			else {
				rewrites.emplace_back(rows[0], k);
			}
		}
		for (const auto &r : rewrites) {
			rewrite_coordinates(cf.lines[r.first], list.macro, 1, objects[r.second].x, objects[r.second].y);
		}
		if (!rewrites.empty()) { coords_files.push_back(cf); }
	}

	if (!write_lines(f, lines, final_newline)) {
		error = "Could not write to " + basename + "!";
		return false;
	}
	written.push_back(basename);
	for (size_t k : moved) {
		Hidden_Object &o = objects[k];
		o.saved_x = o.x;
		o.saved_y = o.y;
		o.line = without_cr(lines[indices[k]]);
		trim(o.line);
	}
	for (const Coords_File &cf : coords_files) {
		const char *name = fl_filename_name(cf.filename.c_str());
		if (write_lines(cf.filename.c_str(), cf.lines, cf.final_newline)) {
			written.push_back(name);
		}
		else {
			warnings.push_back(std::string("Could not write to ") + name + "! It still has the old coordinates.");
		}
	}
	return true;
}

Hidden_Object_Marker::Hidden_Object_Marker(const Hidden_Object &object) : Fl_Box(0, 0, 0, 0), _object(object), _tip() {
	user_data(NULL);
	box(OS_NO_BOX);
	update_tooltip();
}

void Hidden_Object_Marker::object(const Hidden_Object &object) {
	_object = object;
	update_tooltip();
}

void Hidden_Object_Marker::coords(int16_t x, int16_t y) {
	_object.x = x;
	_object.y = y;
	update_tooltip();
}

void Hidden_Object_Marker::update_tooltip() {
	if (!_object.movable) {
		_tip = "Hidden object (it cannot be moved: its coordinates are not plain numbers):\n" + _object.line;
	}
	else if (_object.moved()) {
		_tip = "Hidden object (moved, not saved yet):\n" + _object.current_line();
	}
	else {
		_tip = "Hidden object (drag it in Events Mode to move it):\n" + _object.line;
	}
	tooltip(_tip.c_str());
}

void Hidden_Object_Marker::reposition(int dx, int dy, int s) {
	resize(dx + ((int)_object.x + EVENT_MARGIN) * s, dy + ((int)_object.y + EVENT_MARGIN) * s, s, s);
}

void Hidden_Object_Marker::draw() {
	Main_Window *mw = (Main_Window *)user_data();
	if (!mw || (mw->mode() != Mode::EVENTS && !mw->show_events())) { return; }
	int X = x(), Y = y(), W = w(), H = h();
	// A hollow square, so the tiles under it stay visible, with an "H" for "hidden"
	// (white instead of yellow while it is moved and not saved yet)
	Fl_Color c = _object.moved() ? FL_WHITE : FL_YELLOW;
	fl_rect(X, Y, W, H, FL_BLACK);
	fl_rect(X+1, Y+1, W-2, H-2, c);
	fl_rect(X+2, Y+2, W-4, H-4, FL_BLACK);
	fl_font(FL_HELVETICA_BOLD, mw->zoom() ? 20 : 11);
	fl_color(FL_BLACK);
	fl_draw("H", X+1, Y+1, W, H, FL_ALIGN_CENTER);
	fl_color(c);
	fl_draw("H", X, Y, W, H, FL_ALIGN_CENTER);
	fl_font(OS_FONT, OS_FONT_SIZE);
}

int Hidden_Object_Marker::handle(int event) {
	Main_Window *mw = (Main_Window *)user_data();
	if (!mw || (mw->mode() != Mode::EVENTS && !mw->show_events())) { return 0; }
	switch (event) {
	case FL_ENTER:
	case FL_LEAVE:
	case FL_MOVE:
		return 1; // enough to show the tooltip
	case FL_PUSH:
		// Left-drag to move it in Events Mode; an event on the same square comes first
		_dragging = mw->mode() == Mode::EVENTS && Fl::event_button() == FL_LEFT_MOUSE && !Fl::event_state(FL_SHIFT | FL_CTRL | FL_ALT) &&
			_object.movable && !mw->has_event_at(_object.x, _object.y);
		return _dragging ? 1 : 0; // other clicks go to the event or block underneath
	case FL_DRAG:
		if (_dragging) { mw->drag_hidden_object(this); }
		return _dragging ? 1 : 0;
	case FL_RELEASE:
		if (_dragging) {
			_dragging = false;
			return 1;
		}
		return 0;
	default:
		return 0;
	}
}
