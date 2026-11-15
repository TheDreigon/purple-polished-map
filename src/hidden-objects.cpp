#include <fstream>
#include <sstream>

#pragma warning(push, 0)
#include <FL/fl_draw.H>
#pragma warning(pop)

#include "themes.h"
#include "event.h"
#include "hidden-objects.h"
#include "main-window.h"

// A number written for RGBDS: $2A or 2Ah (hexadecimal), %101 (binary) or 42 (decimal)
static int16_t parse_number(std::string s) {
	trim(s);
	if (s.empty()) { return 0; }
	if (s[0] == '$') { return (int16_t)strtol(s.c_str() + 1, NULL, 16); }
	if (s[0] == '%') { return (int16_t)strtol(s.c_str() + 1, NULL, 2); }
	if (s.back() == 'h' || s.back() == 'H') { return (int16_t)strtol(s.c_str(), NULL, 16); }
	return (int16_t)strtol(s.c_str(), NULL, 10);
}

bool read_hidden_objects(const char *f, const char *map_constant, std::vector<Hidden_Object> &objects) {
	objects.clear();
	std::ifstream ifs;
	open_ifstream(ifs, f);
	if (!ifs.good()) { return false; }

	std::vector<std::string> lines;
	for (std::string line; std::getline(ifs, line);) {
		if (!line.empty() && line.back() == '\r') { line.pop_back(); }
		lines.push_back(line);
	}

	// "dbw MAP_CONSTANT, Label" gives the label of each map's list
	std::vector<std::string> labels;
	for (const std::string &line : lines) {
		std::string code(line);
		remove_comment(code);
		std::istringstream lss(code);
		std::string macro;
		if (!leading_macro(lss, macro, "dbw")) { continue; }
		std::string map, label;
		std::getline(lss, map, ',');
		std::getline(lss, label);
		trim(map);
		trim(label);
		if (map == map_constant && !label.empty()) { labels.push_back(label); }
	}

	// Each list starts with its label and ends at "db -1" or at the next label
	for (const std::string &label : labels) {
		bool in_list = false;
		for (const std::string &line : lines) {
			std::string code(line);
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
			if (macro != "hidden_object") { continue; }
			std::string x, y;
			std::getline(lss, x, ',');
			std::getline(lss, y, ',');
			trim(x);
			trim(y);
			if (x.empty() || y.empty()) { continue; }
			Hidden_Object object;
			object.x = parse_number(x);
			object.y = parse_number(y);
			object.line = line;
			trim(object.line);
			objects.push_back(object);
		}
	}
	return true;
}

Hidden_Object_Marker::Hidden_Object_Marker(const Hidden_Object &object) : Fl_Box(0, 0, 0, 0), _object(object),
	_tip("Hidden object (read-only):\n" + object.line) {
	user_data(NULL);
	box(OS_NO_BOX);
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
	fl_rect(X, Y, W, H, FL_BLACK);
	fl_rect(X+1, Y+1, W-2, H-2, FL_YELLOW);
	fl_rect(X+2, Y+2, W-4, H-4, FL_BLACK);
	fl_font(FL_HELVETICA_BOLD, mw->zoom() ? 20 : 11);
	fl_color(FL_BLACK);
	fl_draw("H", X+1, Y+1, W, H, FL_ALIGN_CENTER);
	fl_color(FL_YELLOW);
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
	default:
		return 0; // clicks go to the event or block underneath
	}
}
