// Writing PNG images (src/image.cpp)

#include "image.h"
#include "test.h"

// An image with a different color in every pixel, which compresses badly
static Fl_RGB_Image *noise_image(int w, int h) {
	uchar *pixels = new uchar[w * h * 3];
	uint32_t x = 12345;
	for (int i = 0; i < w * h * 3; i++) {
		x = x * 1103515245 + 12345;
		pixels[i] = (uchar)(x >> 16);
	}
	Fl_RGB_Image *image = new Fl_RGB_Image(pixels, w, h, 3);
	image->alloc_array = 1;
	return image;
}

TEST(image_writes_an_rgb_png) {
	std::string f = test::join(test::temp_dir(), "map.png");
	Fl_RGB_Image *image = noise_image(64, 48);
	CHECK(Image::write_rgb_image(f.c_str(), image) == Image::Result::IMAGE_OK);
	// The image no longer owns its pixels, so deleting it is safe
	delete image;

	std::string png = test::read_file(f);
	REQUIRE(png.size() > 26);
	CHECK_EQ(png.substr(1, 3), "PNG");
	CHECK_EQ((int)(uint8_t)png[25], 2); // RGB
}

TEST(image_reports_a_file_it_cannot_create) {
	std::string f = test::join(test::temp_dir(), "missing folder/map.png");
	Fl_RGB_Image *image = noise_image(8, 8);
	CHECK(Image::write_rgb_image(f.c_str(), image) == Image::Result::IMAGE_BAD_FILE);
	delete image;
}

#ifndef _WIN32
TEST(image_reports_a_full_disk_instead_of_ending_the_program) {
	// /dev/full accepts the file but fails every write, like a full disk.
	// Writing a PNG to it used to make libpng end the whole program.
	Fl_RGB_Image *image = noise_image(256, 256);
	Image::Result result = Image::write_rgb_image("/dev/full", image);
	CHECK(result == Image::Result::IMAGE_BAD_PNG || result == Image::Result::IMAGE_BAD_FILE);
	delete image;
}
#endif
