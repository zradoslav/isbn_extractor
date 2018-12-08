#ifndef IMAGE_H
#define IMAGE_H

struct image_t
{
	image_t(int w, int h, int bpp, int bpl, unsigned char* d = nullptr);
	image_t(const image_t& img);
	image_t(image_t&& img);
	~image_t();

	int width;
	int height;
	int bytes_per_pixel;
	int bytes_per_line;
	unsigned char* data;
};

#endif
