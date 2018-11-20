#ifdef DEBUG
#include "debug.h"
#endif
#include "image.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pcre2posix.h>
#include <tesseract/capi.h>

static TessBaseAPI* api;

static const char* isbn_rx_str = "isbn[\\-\\s]*?(1[03])?[:\\s]*([\\-\\d\\s]+\\d)";
static regex_t isbn_rx;

// later i'll add per-format embedded text extraction (if any)
int extract_text_ocr_static(const image_t image, char* tbuff, size_t tbuff_size)
{
	TessBaseAPISetImage(api, image.data, image.width, image.height, image.bytes_per_pixel, image.bytes_per_line);
	if(TessBaseAPIRecognize(api, NULL))
		return -1;

	char* text = TessBaseAPIGetUTF8Text(api);
	if(!text)
		return -1;

#ifdef DEBUG
	size_t text_len = strlen(text);
	if(tbuff_size < text_len)
		print_err("%s: insufficient storage: %zu < %zu\n", __FUNCTION__, tbuff_size, text_len);
#endif

	strncpy(tbuff, text, tbuff_size);
	TessDeleteText(text);

	return strlen(tbuff);
}

void match_isbn(const char* text)
{
	const char* cursor = text;

	int max_groups = isbn_rx.re_nsub + 1;
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

static const char* opt_desc[] = {
    ['t'] = "document type - pdf, djvu, ps, ...",
    ['n'] = "a number of pages to process",
    ['f'] = "document filename",
    ['l'] = "document language in ISO 639-3 (default: eng)",
    ['h'] = "print this help"
};

static void help()
{
	const char* opts = "tnflh";
	printf("\n");
	for(size_t i = 0; i < strlen(opts); i++)
		printf(" -%c\t%s\n", opts[i], opt_desc[opts[i]]);
	printf("\n");
}

int main(int argc, char* argv[])
{
	int retcode = EXIT_SUCCESS;

	const char* type = 0;
	const char* file = 0;
	const char* lang = "eng";
	int page_count = 0;

	/* process command line */
	int option;
	while((option = getopt(argc, argv,"f:n:t:l:h")) != -1)
	{
		switch(option)
		{
		case 't':
			type = optarg;
			break;
		case 'n':
			page_count = atoi(optarg);
			break;
		case 'f':
			file = optarg;
			break;
		case 'l':
			lang = optarg;
			break;
		case 'h':
			printf("Usage: %s -t type -n pages [-l lang] [-f] file\n", argv[0]);
			help();
			retcode = EXIT_SUCCESS;
			goto exit;
		default:
			printf("Usage: %s -t type -n pages [-l lang] [-f] file\n", argv[0]);
			retcode = EXIT_FAILURE;
			goto exit;
		}
	}
	if(!file)
	{
		if(optind + 1 == argc)
			file = argv[optind];
		else
		{
			fprintf(stderr, "No file passed\n");
			retcode = EXIT_FAILURE;
			goto exit;
		}
	}

	extract_func process = NULL;
	if(!strcasecmp(type, "djv") || !strcasecmp(type, "djvu"))
		process = &extract_images_djv;
	else if(!strcasecmp(type, "pdf"))
		process = &extract_images_pdf;
	else
	{
		fprintf(stderr, "Unsupported filetype\n");
		retcode = EXIT_FAILURE;
		goto exit;
	}

	/* initialize OCR */
	api = TessBaseAPICreate();
	if(!api || TessBaseAPIInit3(api, NULL, lang))
	{
		fprintf(stderr, "OCR engine init failure\n");
		retcode = EXIT_FAILURE;
		goto clean_ocr;
	}

	image_t* images = calloc(page_count, sizeof(image_t));
	if(!images)
	{
		perror("Failed allocation");
		retcode = EXIT_FAILURE;
		goto clean_ocr;
	}
	if(process(file, page_count, images) != page_count)
	{
		perror("Failed processing file");
		retcode = EXIT_FAILURE;
		goto clean_mem;
	}

#ifdef DEBUG
	for(int i = 0; i < page_count; i++)
		print_log("page[%d]: %dx%d, %d kB\n", i,
		          images[i].width, images[i].height,
		          images[i].bytes_per_pixel * images[i].width * images[i].height / 1024);
#endif

	regcomp(&isbn_rx, isbn_rx_str, REG_ICASE);
	for(int i = 0; i < page_count; i++)
	{
		char text[2048] = { 0 };
		extract_text_ocr_static(images[i], text, sizeof(text));

		match_isbn(text);
	}
	regfree(&isbn_rx);

clean_mem:
	for(int i = 0; i < page_count; i++)
		if(images[i].data)
			free(images[i].data);
	free(images);

clean_ocr:
	TessBaseAPIEnd(api);
	TessBaseAPIDelete(api);

exit:
	exit(retcode);
}
