#include "image.h"

#include <cstring>

image_t::image_t(int w, int h, int bpp, int bpl, unsigned char* d)
	: width(w), height(h),
	bytes_per_pixel(bpp), bytes_per_line(bpl),
	data(d)
{}

image_t::image_t(const image_t& img)
	: image_t(img.width, img.height, img.bytes_per_pixel, img.bytes_per_line)
{
	size_t len = bytes_per_line * height;
	data = new unsigned char[len];
	memcpy(data, img.data, len);
}

image_t::image_t(image_t&& img)
	: image_t(img.width, img.height, img.bytes_per_pixel, img.bytes_per_line)
{
	data = img.data;
	img.data = nullptr;
}

image_t::~image_t()
{
	delete[] data;
}
