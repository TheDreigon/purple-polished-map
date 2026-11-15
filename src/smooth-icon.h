#ifndef SMOOTH_ICON_H
#define SMOOTH_ICON_H

#include <vector>

#pragma warning(push, 0)
#include <FL/Fl_Image.H>
#include <FL/Fl_Pixmap.H>
#pragma warning(pop)

// Resizes RGBA pixels with bilinear filtering (with premultiplied alpha, so transparent pixels
// do not darken the edges). src has sw x sh pixels and sld bytes per line; the result has
// dw x dh pixels and dw * 4 bytes per line.
std::vector<uchar> smooth_resize_rgba(const uchar *src, int sw, int sh, int sld, int dw, int dh);

// A pixel-art icon that looks smooth at fractional display scales.
// At 150%, every pixel of a 16x16 icon has to cover one and a half screen pixels. Drawn as is,
// some pixels become twice as wide as others and the icon looks jagged. Polished Map 4.7.1
// let Windows stretch the whole window smoothly instead, which blurs it a little but keeps
// the icons even. This icon does the same for itself: at a fractional scale it draws a smoothly
// resized copy made for that scale, and at whole scales (100%, 200%) the original pixels.
class Smooth_Icon : public Fl_Image {
private:
	Fl_Pixmap *_pixmap;
	Fl_RGB_Image *_cache;
public:
	Smooth_Icon(const char * const *xpm);
	~Smooth_Icon();
	Fl_Image *copy(int W, int H) const override;
	Fl_Image *copy(void) const { return Fl_Image::copy(); }
	void color_average(Fl_Color c, float i) override;
	void desaturate(void) override;
	void draw(int X, int Y, int W, int H, int cx = 0, int cy = 0) override;
	void draw(int X, int Y) { draw(X, Y, w(), h(), 0, 0); }
	void uncache(void) override;
private:
	Smooth_Icon(Fl_Pixmap *pixmap);
	void drop_cache(void);
	void make_cache(int dw, int dh);
};

#endif
