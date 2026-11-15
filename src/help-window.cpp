#pragma warning(push, 0)
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_ask.H>
#pragma warning(pop)

#include "themes.h"
#include "widgets.h"
#include "help-window.h"

Help_Window::Help_Window(int x, int y, int w, int h, const char *t) : _dx(x), _dy(y), _width(w), _height(h), _title(t),
	_content(NULL), _window(NULL), _body(NULL), _find(NULL), _find_next(NULL), _find_pos(0), _ok_button(NULL),
	_spacer(NULL) {}

Help_Window::~Help_Window() {
	delete _window;
	delete _body;
	delete _find;
	delete _find_next;
	delete _ok_button;
	delete _spacer;
}

void Help_Window::initialize() {
	if (_window) { return; }
	Fl_Group *prev_current = Fl_Group::current();
	Fl_Group::current(NULL);
	// Populate window
	_window = new Fl_Double_Window(_dx, _dy, _width, _height, _title);
	_body = new HTML_View(10, 10, _width-20, _height-52);
	_find = new OS_Input(50, _height-32, 200, 22, "Find:");
	_find_next = new OS_Button(256, _height-32, 70, 22, "Next");
	_ok_button = new Default_Button(_width-90, _height-32, 80, 22, "OK");
	// The find box and its button keep their size; the text grows with the window
	_spacer = new Fl_Box(340, 10, _width-440, _height-52);
	_window->end();
	// Initialize window
	_window->box(OS_BG_BOX);
	_window->resizable(_spacer);
	_window->callback((Fl_Callback *)close_cb, this);
	// Initialize window's children
	_find->tooltip("Type to find text in the help; Enter finds the next match");
	_find->when(FL_WHEN_CHANGED | FL_WHEN_ENTER_KEY_ALWAYS);
	_find->callback((Fl_Callback *)find_cb, this);
	_find_next->tooltip("Find the next match (Enter)");
	_find_next->callback((Fl_Callback *)find_cb, this);
	_ok_button->tooltip("OK (Enter)");
	_ok_button->callback((Fl_Callback *)close_cb, this);
	Fl_Group::current(prev_current);
}

void Help_Window::refresh() {
	_window->label(_title ? _title : "Help");
	_body->value(_content ? _content : "");
}

void Help_Window::show(const Fl_Widget *p) {
	initialize();
	refresh();
	Fl_Window *prev_grab = Fl::grab();
	_window->position(p->x() + _dx, p->y() + _dy);
	Fl::grab(NULL);
	_window->show();
	Fl::grab(prev_grab);
}

void Help_Window::redraw() {
	if (!_window) { return; }
	_body->textsize(_body->textsize()); // roundabout way of calling private function format()
	_window->redraw();
}

void Help_Window::close_cb(Fl_Widget *, Help_Window *hw) {
	hw->_window->hide();
}

// Typing in the find box shows the first match; Enter or Next shows the next one, and after
// the last match the search starts again from the top
void Help_Window::find_cb(Fl_Widget *w, Help_Window *hw) {
	const char *s = hw->_find->value();
	if (!s || !*s) {
		hw->_find_pos = 0;
		return;
	}
	bool next = w == hw->_find_next || Fl::event_key() == FL_Enter || Fl::event_key() == FL_KP_Enter;
	int from = next ? hw->_find_pos : 0;
	int p = hw->_body->find(s, from);
	if (p < 0 && from > 0) {
		p = hw->_body->find(s, 0);
	}
	if (p < 0) {
		fl_beep();
		hw->_find_pos = 0;
		return;
	}
	hw->_find_pos = p + 1;
	hw->_body->redraw();
}
