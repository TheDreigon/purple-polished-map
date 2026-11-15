#include <fstream>
#include <sstream>

#pragma warning(push, 0)
#include <FL/fl_draw.H>
#pragma warning(pop)

#include "themes.h"
#include "event.h"
#include "map-connections.h"
#include "main-window.h"

// The comma-separated arguments of a macro line, trimmed
static std::vector<std::string> arguments(std::istringstream &lss) {
	std::vector<std::string> args;
	for (std::string a; std::getline(lss, a, ',');) {
		trim(a);
		args.push_back(a);
	}
	return args;
}

bool read_map_connections(const char *f, const char *map_label, std::vector<Map_Connection> &connections) {
	connections.clear();
	std::ifstream ifs;
	open_ifstream(ifs, f);
	if (!ifs.good()) { return false; }

	// Until a map_header or map_attributes line says otherwise, the lines are this map's
	bool ours = true, seen_header = false;
	while (ifs.good()) {
		std::string line;
		std::getline(ifs, line);
		remove_comment(line);
		std::istringstream lss(line);
		std::string macro;
		if (!leading_macro(lss, macro)) { continue; }
		if (macro == "map_header" || macro == "map_attributes") {
			std::vector<std::string> args = arguments(lss);
			if (seen_header && ours) { break; } // the next map's lines start here
			seen_header = true;
			ours = !args.empty() && args[0] == map_label;
			continue;
		}
		if (macro == "end_map_header" && ours && seen_header) { break; }
		if (macro != "connection" || !ours) { continue; }
		std::vector<std::string> args = arguments(lss);
		if (args.size() < 4) { continue; }
		Map_Connection c;
		c.direction = args[0];
		lowercase(c.direction);
		c.map_name = args[1];
		c.map_constant = args[2];
		c.offset = (int)strtol(args[3].c_str(), NULL, 10);
		connections.push_back(c);
	}
	return true;
}

bool read_map_size(const char *f, const char *map_constant, int &w, int &h) {
	std::ifstream ifs;
	open_ifstream(ifs, f);
	if (!ifs.good()) { return false; }
	while (ifs.good()) {
		std::string line;
		std::getline(ifs, line);
		remove_comment(line);
		std::istringstream lss(line);
		std::string macro;
		if (!leading_macro(lss, macro)) { continue; }
		if (macro != "map_const" && macro != "mapconst") { continue; }
		std::vector<std::string> args = arguments(lss);
		if (args.size() < 3 || args[0] != map_constant) { continue; }
		int a = (int)strtol(args[1].c_str(), NULL, 10), b = (int)strtol(args[2].c_str(), NULL, 10);
		if (macro == "map_const") { w = a; h = b; }
		else { h = a; w = b; }
		return w > 0 && h > 0;
	}
	return false;
}

std::string read_map_header_tileset(const char *f) {
	std::ifstream ifs;
	open_ifstream(ifs, f);
	while (ifs.good()) {
		std::string line;
		std::getline(ifs, line);
		remove_comment(line);
		std::istringstream lss(line);
		std::string macro;
		if (!leading_macro(lss, macro, "map_header")) { continue; }
		std::vector<std::string> args = arguments(lss);
		if (args.size() < 3) { return ""; }
		std::string tileset = args[2];
		lowercase(tileset);
		return tileset;
	}
	return "";
}

Connection_View::Connection_View(const Map_Connection &c, const std::vector<uint8_t> &ids, int cw, int ch, int w, int h,
	bool drawable) : Fl_Box(0, 0, 0, 0), _connection(c), _ids(ids), _cw(cw), _ch(ch), _bx(0), _by(0), _bw(0), _bh(0),
	_sx(0), _sy(0), _drawable(drawable), _tip() {
	user_data(NULL);
	box(OS_NO_BOX);
	const int m = MAP_MARGIN;
	// The connected map's rectangle relative to this map, then the part inside the margin
	int x0 = 0, y0 = 0;
	if (c.direction == "north") { x0 = c.offset; y0 = -ch; }
	else if (c.direction == "south") { x0 = c.offset; y0 = h; }
	else if (c.direction == "west") { x0 = -cw; y0 = c.offset; }
	else if (c.direction == "east") { x0 = w; y0 = c.offset; }
	else { return; }
	int left = std::max(x0, -m), top = std::max(y0, -m);
	int right = std::min(x0 + cw, w + m), bottom = std::min(y0 + ch, h + m);
	// Only the margin, not this map itself
	if (c.direction == "north") { bottom = std::min(bottom, 0); }
	else if (c.direction == "south") { top = std::max(top, h); }
	else if (c.direction == "west") { right = std::min(right, 0); }
	else { left = std::max(left, w); }
	_bx = left;
	_by = top;
	_bw = right - left;
	_bh = bottom - top;
	_sx = left - x0;
	_sy = top - y0;
	_tip = "Connection " + c.direction + ": " + c.map_name + " (offset " + std::to_string(c.offset) + ")";
	if (!drawable) { _tip += "\nIt uses another tileset, so its blocks are not shown."; }
	tooltip(_tip.c_str());
}

void Connection_View::reposition(int dx, int dy, int s) {
	resize(dx + (_bx + MAP_MARGIN) * s, dy + (_by + MAP_MARGIN) * s, _bw * s, _bh * s);
}

void Connection_View::draw() {
	Main_Window *mw = (Main_Window *)user_data();
	if (!mw || !mw->show_connections() || empty()) { return; }
	int s = mw->metatile_size();
	for (int y = 0; y < _bh; y++) {
		for (int x = 0; x < _bw; x++) {
			int px = this->x() + x * s, py = this->y() + y * s;
			size_t i = (size_t)(_sy + y) * _cw + (_sx + x);
			if (_drawable && i < _ids.size()) {
				mw->draw_metatile(px, py, _ids[i]);
			}
			else {
				fl_color(FL_DARK2);
				fl_rectf(px, py, s, s);
			}
		}
	}
	// Dim it with a light grid of dots, so it does not look like part of this map
	fl_push_clip(this->x(), this->y(), w(), h());
	fl_color(FL_BLACK);
	for (int py = this->y(); py < this->y() + h(); py += 2) {
		for (int px = this->x() + (py / 2) % 2; px < this->x() + w(); px += 2) {
			fl_point(px, py);
		}
	}
	fl_pop_clip();
	fl_rect(this->x(), this->y(), w(), h(), FL_YELLOW);
}

int Connection_View::handle(int event) {
	Main_Window *mw = (Main_Window *)user_data();
	if (!mw || !mw->show_connections()) { return 0; }
	switch (event) {
	case FL_ENTER:
	case FL_LEAVE:
	case FL_MOVE:
		return 1; // enough to show the tooltip
	default:
		return 0; // clicks go to whatever is underneath
	}
}
