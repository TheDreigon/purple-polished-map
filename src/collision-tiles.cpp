#include <fstream>
#include <sstream>

#pragma warning(push, 0)
#include <FL/filename.H>
#pragma warning(pop)

#include "utils.h"
#include "collision-tiles.h"

// A number written for RGBDS: $2A or 2Ah (hexadecimal), %101 (binary) or 42 (decimal)
static long parse_number(std::string s) {
	trim(s);
	if (s.empty()) { return -1; }
	if (s[0] == '$') { return strtol(s.c_str() + 1, NULL, 16); }
	if (s[0] == '%') { return strtol(s.c_str() + 1, NULL, 2); }
	if (s.back() == 'h' || s.back() == 'H') { return strtol(s.c_str(), NULL, 16); }
	return strtol(s.c_str(), NULL, 10);
}

// The label at the start of a line, without its colons, or "" if the line has none
static std::string line_label(const std::string &line) {
	if (line.empty() || line[0] == ' ' || line[0] == '\t') { return ""; }
	size_t colon = line.find(':');
	if (colon == std::string::npos) { return ""; }
	std::string label = line.substr(0, colon);
	trim(label);
	return label;
}

bool read_passable_tiles(const char *f, const std::vector<std::string> &labels, std::vector<bool> &passable) {
	passable.assign(256, false);
	std::ifstream ifs;
	open_ifstream(ifs, f);
	if (!ifs.good()) { return false; }

	bool wanted = false;
	while (ifs.good()) {
		std::string line;
		std::getline(ifs, line);
		remove_comment(line);
		std::string label = line_label(line);
		if (!label.empty()) {
			for (const std::string &l : labels) {
				if (label == l + "_Coll") { wanted = true; }
			}
			continue;
		}
		if (!wanted) { continue; }
		std::istringstream lss(line);
		std::string macro;
		if (!leading_macro(lss, macro, "coll_tiles")) { continue; }
		for (std::string token; std::getline(lss, token, ',');) {
			long t = parse_number(token);
			if (t >= 0 && t < 256) { passable[(size_t)t] = true; }
		}
		return true;
	}
	return false;
}

// "reds_house" -> "RedsHouse"
static std::string camel_case(const std::string &file_name) {
	std::string s;
	bool upper = true;
	for (char c : file_name) {
		if (c == '_') { upper = true; continue; }
		s += upper ? (char)toupper((unsigned char)c) : c;
		upper = false;
	}
	return s;
}

std::vector<std::string> tileset_labels(const char *root, const char *file_name) {
	std::vector<std::string> labels;
	char tilesets_asm[FL_PATH_MAX] = {};
	snprintf(tilesets_asm, sizeof(tilesets_asm), "%sgfx" DIR_SEP "tilesets.asm", root);
	std::ifstream ifs;
	open_ifstream(ifs, tilesets_asm);
	std::vector<std::string> pending;
	while (ifs.good()) {
		std::string line;
		std::getline(ifs, line);
		remove_comment(line);
		std::string label = line_label(line);
		if (!label.empty()) {
			if (ends_with(label, "_GFX")) { pending.push_back(label.substr(0, label.size() - 4)); }
			else { pending.clear(); }
		}
		size_t incbin = line.find("INCBIN");
		if (incbin == std::string::npos) { continue; }
		size_t open = line.find('"', incbin), close = open == std::string::npos ? open : line.find('"', open + 1);
		if (close != std::string::npos) {
			std::string path = line.substr(open + 1, close - open - 1);
			size_t slash = path.find_last_of("/\\");
			std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
			name.erase(std::min(name.find('.'), name.size()));
			if (name == file_name) {
				labels.insert(labels.end(), pending.begin(), pending.end());
			}
		}
		pending.clear();
	}
	if (labels.empty()) { labels.push_back(camel_case(file_name)); }
	return labels;
}
