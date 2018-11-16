#ifdef DEBUG
#include "debug.h"
#endif
#include "image.h"

#include <stdlib.h>
#include <string.h>

#include <pcre2posix.h>
#include <tesseract/capi.h>

static TessBaseAPI* api;

static const char* isbn_rx_str = "isbn[\\-\\s]*?(1[03])?[:\\s]*([\\-\\d\\s]+\\d)";
static regex_t isbn_rx;

// later i'll add per-format embedded text extraction (if any)
int extract_text_ocr_static(const image_t image, char* tbuff, int tbuff_size)
{
	TessBaseAPISetImage(api, image.data, image.width, image.height, image.bytes_per_pixel, image.bytes_per_line);
	if(TessBaseAPIRecognize(api, NULL))
		return -1;

	char* text = TessBaseAPIGetUTF8Text(api);
	if(!text)
		return -1;

#ifdef DEBUG
	int text_len = strlen(text);
	if(tbuff_size < text_len)
		print_err("%s: insufficient storage: %d < %d\n", __FUNCTION__, tbuff_size, text_len);
#endif

	strncpy(tbuff, text, tbuff_size);
	TessDeleteText(text);

	return strlen(tbuff);
}

void match_isbn(const char* text)
{
	char* cursor = text;

	size_t max_groups = isbn_rx.re_nsub + 1;
	regmatch_t* groups = malloc(max_groups * sizeof(regmatch_t));

	int matches = 0;
	while(true)
	{
		if(regexec(&isbn_rx, cursor, max_groups, groups, 0))
			break;

		int offset = 0;
		for(int i = 0; i < max_groups; i++)
		{
			// empty group, just skip
			if(groups[i].rm_so == (size_t)(-1))
				continue;

			// group 0 indicates whole match
			if(i == 0)
				offset = groups[i].rm_eo;

#ifdef DEBUG
			int match_len = groups[i].rm_eo - groups[i].rm_so;
			char* match_str = malloc(match_len + 1);
			strncpy(match_str, cursor + groups[i].rm_so, match_len);
			match_str[match_len] = '\0';

			print_log("%s: match %u, group %u: [%u-%u]: %s\n", __FUNCTION__,
					  matches, i, groups[i].rm_so, groups[i].rm_eo, match_str);

			free(match_str);
#endif
		}
		cursor += offset;
		matches++;
	}

	free(groups);
}


// isbn-extract -t [pdf] -n [3] [filename]
int main(int argc, char* argv[])
{
	regcomp(&isbn_rx, isbn_rx_str, REG_ICASE);

	api = TessBaseAPICreate();
	if(TessBaseAPIInit3(api, NULL, "eng"))
	{
		fprintf(stderr, "Tesseract init failure\n");
		abort();
	}

	char* filename = argv[1];
	int page_count = atoi(argv[2]);

	image_t* images = malloc(page_count * sizeof(image_t));
	extract_images_djv(filename, page_count, images);

#ifdef DEBUG
	for(int i = 0; i < page_count; i++)
		print_log("%s: page #%d: %dx%d, %d kB\n", __FUNCTION__,
				  i, images[i].width, images[i].height,
				  images[i].bytes_per_line * images[i].height / 1024);
#endif

	for(int i = 0; i < page_count; i++)
	{
		char text[2048] = { 0 };
		extract_text_ocr_static(images[i], text, sizeof(text));

		match_isbn(text);
	}

	TessBaseAPIEnd(api);
	TessBaseAPIDelete(api);

	for(int i = 0; i < page_count; i++)
		free(images[i].data);
	free(images);

	regfree(&isbn_rx);

	return 0;
}
