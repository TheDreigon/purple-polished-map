// Smoothly resized toolbar icons (src/smooth-icon.cpp)

#include "smooth-icon.h"
#include "test.h"

static std::vector<uchar> rgba(std::initializer_list<int> values) {
	std::vector<uchar> v;
	for (int x : values) { v.push_back((uchar)x); }
	return v;
}

static std::string pixel(const std::vector<uchar> &image, int width, int x, int y) {
	const uchar *p = image.data() + ((size_t)y * width + x) * 4;
	return std::to_string(p[0]) + "," + std::to_string(p[1]) + "," + std::to_string(p[2]) + "," + std::to_string(p[3]);
}

TEST(smooth_icon_keeps_the_corners_and_blends_between_them) {
	// A 2x2 icon (black, white / white, black) made 3x3, as at 150%
	std::vector<uchar> src = rgba({
		0, 0, 0, 255,   255, 255, 255, 255,
		255, 255, 255, 255,   0, 0, 0, 255,
	});
	std::vector<uchar> dst = smooth_resize_rgba(src.data(), 2, 2, 0, 3, 3);
	REQUIRE(dst.size() == 3 * 3 * 4);
	CHECK_EQ(pixel(dst, 3, 0, 0), "0,0,0,255");
	CHECK_EQ(pixel(dst, 3, 2, 0), "255,255,255,255");
	CHECK_EQ(pixel(dst, 3, 2, 2), "0,0,0,255");
	// The middle is halfway between the four pixels
	CHECK_EQ(pixel(dst, 3, 1, 1), "128,128,128,255");
}

TEST(smooth_icon_does_not_darken_edges_next_to_transparent_pixels) {
	// An opaque white pixel next to a fully transparent (black) one: the edge between them
	// must fade to transparent, not turn gray
	std::vector<uchar> src = rgba({255, 255, 255, 255,   0, 0, 0, 0});
	std::vector<uchar> dst = smooth_resize_rgba(src.data(), 2, 1, 0, 3, 1);
	CHECK_EQ(pixel(dst, 3, 0, 0), "255,255,255,255");
	CHECK_EQ(pixel(dst, 3, 1, 0), "255,255,255,128");
	CHECK_EQ(pixel(dst, 3, 2, 0), "0,0,0,0");
}

TEST(smooth_icon_resizes_to_the_same_size_unchanged) {
	std::vector<uchar> src = rgba({
		10, 20, 30, 255,   40, 50, 60, 128,
		70, 80, 90, 0,   100, 110, 120, 255,
	});
	std::vector<uchar> dst = smooth_resize_rgba(src.data(), 2, 2, 0, 2, 2);
	CHECK_EQ(pixel(dst, 2, 0, 0), "10,20,30,255");
	CHECK_EQ(pixel(dst, 2, 1, 0), "40,50,60,128");
	CHECK_EQ(pixel(dst, 2, 0, 1), "0,0,0,0");
	CHECK_EQ(pixel(dst, 2, 1, 1), "100,110,120,255");
}

static const char *TEST_XPM[] = {
	"2 2 3 1",
	". c None",
	"# c #000000",
	"o c #FFFFFF",
	"#o",
	"o.",
};

TEST(smooth_icon_copies_and_grays_out_like_a_pixmap) {
	// Disabled toolbar buttons show a desaturated copy of the icon
	Smooth_Icon icon(TEST_XPM);
	CHECK_EQ(icon.w(), 2);
	CHECK_EQ(icon.h(), 2);
	Fl_Image *copy = icon.copy();
	REQUIRE(copy != NULL);
	CHECK_EQ(copy->w(), 2);
	CHECK_EQ(copy->h(), 2);
	CHECK(dynamic_cast<Smooth_Icon *>(copy) != NULL);
	copy->desaturate();
	copy->color_average(FL_GRAY, 0.5f);
	delete copy;
}
