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

typedef int (*extract_func)(const char*, int, image_t*);

int extract_images_djv(const char* file, int page_count, image_t* ibuff);
int extract_images_pdf(const char* file, int page_count, image_t* ibuff);

#endif
