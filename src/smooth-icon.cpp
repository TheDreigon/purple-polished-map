#include <algorithm>
#include <cmath>

#pragma warning(push, 0)
#include <FL/fl_draw.H>
#pragma warning(pop)

#include "smooth-icon.h"

std::vector<uchar> smooth_resize_rgba(const uchar *src, int sw, int sh, int sld, int dw, int dh) {
	std::vector<uchar> dst((size_t)dw * dh * 4);
	if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) { return dst; }
	if (!sld) { sld = sw * 4; }
	for (int y = 0; y < dh; y++) {
		// Map the centre of the destination pixel to the source
		double fy = std::clamp((y + 0.5) * sh / dh - 0.5, 0.0, (double)(sh - 1));
		int y0 = (int)fy, y1 = std::min(y0 + 1, sh - 1);
		double ty = fy - y0;
		for (int x = 0; x < dw; x++) {
			double fx = std::clamp((x + 0.5) * sw / dw - 0.5, 0.0, (double)(sw - 1));
			int x0 = (int)fx, x1 = std::min(x0 + 1, sw - 1);
			double tx = fx - x0;
			const uchar *p[4] = {
				src + (size_t)y0 * sld + x0 * 4, src + (size_t)y0 * sld + x1 * 4,
				src + (size_t)y1 * sld + x0 * 4, src + (size_t)y1 * sld + x1 * 4
			};
			double weight[4] = {(1 - tx) * (1 - ty), tx * (1 - ty), (1 - tx) * ty, tx * ty};
			double r = 0, g = 0, b = 0, a = 0;
			for (int i = 0; i < 4; i++) {
				double pa = p[i][3] * weight[i];
				r += p[i][0] * pa;
				g += p[i][1] * pa;
				b += p[i][2] * pa;
				a += pa;
			}
			uchar *q = dst.data() + ((size_t)y * dw + x) * 4;
			if (a > 0) {
				q[0] = (uchar)std::lround(std::min(r / a, 255.0));
				q[1] = (uchar)std::lround(std::min(g / a, 255.0));
				q[2] = (uchar)std::lround(std::min(b / a, 255.0));
			}
			q[3] = (uchar)std::lround(std::min(a, 255.0));
		}
	}
	return dst;
}

Smooth_Icon::Smooth_Icon(const char * const *xpm) : Smooth_Icon(new Fl_Pixmap(xpm)) {}

Smooth_Icon::Smooth_Icon(Fl_Pixmap *pixmap) : Fl_Image(pixmap->w(), pixmap->h(), 0), _pixmap(pixmap), _cache(NULL) {}

Smooth_Icon::~Smooth_Icon() {
	drop_cache();
	delete _pixmap;
}

Fl_Image *Smooth_Icon::copy(int W, int H) const {
	return new Smooth_Icon((Fl_Pixmap *)_pixmap->copy(W, H));
}

void Smooth_Icon::color_average(Fl_Color c, float i) {
	_pixmap->color_average(c, i);
	drop_cache();
}

void Smooth_Icon::desaturate() {
	_pixmap->desaturate();
	drop_cache();
}

void Smooth_Icon::uncache() {
	_pixmap->uncache();
	drop_cache();
}

void Smooth_Icon::drop_cache() {
	delete _cache;
	_cache = NULL;
}

void Smooth_Icon::make_cache(int dw, int dh) {
	drop_cache();
	Fl_RGB_Image rgba(_pixmap);
	if (rgba.fail() || !rgba.array || rgba.d() != 4) { return; }
	std::vector<uchar> pixels = smooth_resize_rgba(rgba.array, rgba.data_w(), rgba.data_h(), rgba.ld(), dw, dh);
	uchar *array = new uchar[pixels.size()];
	std::copy(pixels.begin(), pixels.end(), array);
	_cache = new Fl_RGB_Image(array, dw, dh, 4);
	_cache->alloc_array = 1;
	// Drawn at the icon's size, which at this scale is exactly dw x dh screen pixels
	_cache->scale(w(), h(), 0, 1);
}

void Smooth_Icon::draw(int X, int Y, int W, int H, int cx, int cy) {
	float s = fl_graphics_driver->scale();
	if (s <= 0.0f || std::fabs(s - std::round(s)) < 0.01f) {
		// At a whole scale every pixel stays square and sharp
		_pixmap->draw(X, Y, W, H, cx, cy);
		return;
	}
	int dw = (int)std::lround(w() * s), dh = (int)std::lround(h() * s);
	if (!_cache || _cache->data_w() != dw || _cache->data_h() != dh) {
		make_cache(dw, dh);
	}
	if (_cache) {
		_cache->draw(X, Y, W, H, cx, cy);
	}
	else {
		_pixmap->draw(X, Y, W, H, cx, cy);
	}
}
