#ifndef IMAGE_H
#define IMAGE_H

#include <vector>

struct image_t
{
	int width;
	int height;
	int bytes_per_pixel;
	int bytes_per_line;
	unsigned char* data;
};

using image_fn = std::vector<image_t> (*)(const char*, int);

std::vector<image_t> extract_images_djv(const char* file, int page_count);
std::vector<image_t> extract_images_pdf(const char* file, int page_count);

#endif
