#ifndef IMAGE_H
#define IMAGE_H

typedef struct
{
	int width;
	int height;
	int bytes_per_pixel;
	int bytes_per_line;
	unsigned char* data;
} image_t;

#endif
