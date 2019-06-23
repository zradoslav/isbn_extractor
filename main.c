#include "image.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pcre2posix.h>
#include <tesseract/capi.h>

#define isbn_max_num 4
static const char* isbn_rx_str = "isbn[- \\t\\(]*?(1[03])?[\\): \\t]*([- \\d\\t]+[x\\d])";
static regex_t isbn_rx;

static TessBaseAPI* api;

// later i'll add per-format embedded text extraction (if any)
static char* extract_text_ocr(const image_t image, size_t* tbuff_size)
{
	TessBaseAPISetImage(api, image.data, image.width, image.height, image.bytes_per_pixel, image.bytes_per_line);
	if(TessBaseAPIRecognize(api, NULL))
		return NULL;

	char* text = TessBaseAPIGetUTF8Text(api);
	if(!text)
		return NULL;

	char* tbuff = strdup(text);
	*tbuff_size = strlen(text) + 1;

	TessDeleteText(text);

	return tbuff;
}

static void match_isbn(const char* text, char** mbuff, int mbuff_len)
{
	const char* cursor = text;

	int max_groups = isbn_rx.re_nsub + 1;
	regmatch_t* groups = malloc(max_groups * sizeof(regmatch_t));

	int match_num = 0;
	while(true)
	{
		if(regexec(&isbn_rx, cursor, max_groups, groups, 0) || match_num == mbuff_len)
			break;

		int offset = 0;
		for(int i = 0; i < max_groups; i++)
		{
			// empty group, just skip
			if(groups[i].rm_so == -1)
				continue;

			// group 0 indicates whole match
			if(i == 0)
			{
				offset = groups[i].rm_eo;
				continue;
			}

			int match_len = groups[i].rm_eo - groups[i].rm_so;
			mbuff[match_num] = strndup(cursor + groups[i].rm_so, match_len);
		}
		cursor += offset;
		match_num++;
	}

	free(groups);
}

static const char* opt_desc[] = {
    ['t'] = "document type - pdf, djvu, ps, ...",
    ['n'] = "a number of pages to process",
    ['f'] = "document filename",
    ['l'] = "document language in ISO 639-3 (default: eng)",
    ['v'] = "be verbose",
    ['h'] = "print this help"
};

static void help()
{
	const char* opts = "tnflhv";
	printf("\n");
	for(int i = 0; i < strlen(opts); i++)
		printf(" -%c\t%s\n", opts[i], opt_desc[opts[i]]);
	printf("\n");
}

int main(int argc, char* argv[])
{
	int retcode = EXIT_SUCCESS;

	const char* type = NULL;
	const char* file = NULL;
	const char* lang = "eng";
	int page_count = 0;
	bool verbose = false;

	/* process command line */
	int option;
	while((option = getopt(argc, argv,"f:n:t:l:hv")) != -1)
	{
		switch(option)
		{
		case 't':
			type = optarg;
			break;
		case 'n':
			/* here '-1' can be used to read the whole file (?) */
			page_count = atoi(optarg);
			break;
		case 'f':
			file = optarg;
			break;
		case 'l':
			lang = optarg;
			break;
		case 'v':
			verbose = true;
			break;
		case 'h':
			printf("Usage: %s [-v] -t type -n pages [-l lang] [-f] file\n", argv[0]);
			help();
			retcode = EXIT_SUCCESS;
			goto exit;
		default:
			printf("Usage: %s [-v] -t type -n pages [-l lang] [-f] file\n", argv[0]);
			retcode = EXIT_FAILURE;
			goto exit;
		}
	}
	if(!type || !page_count)
	{
		fprintf(stderr, "Invalid arguments\n");
		retcode = EXIT_FAILURE;
		goto exit;
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
	/* make Tesseract quiet */
	TessBaseAPISetVariable(api, "debug_file", "/dev/null");

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

	regcomp(&isbn_rx, isbn_rx_str, REG_ICASE);
	for(int i = 0; i < page_count; i++)
	{
		size_t ocr_len = 0;
		char* ocr_text = extract_text_ocr(images[i], &ocr_len);

		if(verbose)
			printf("page[%d]: %dx%d, %d kB, %zu B\n", i, images[i].width, images[i].height,
			       images[i].bytes_per_line * images[i].height / 1024, ocr_len);

		char* matches[isbn_max_num] = { 0 };
		match_isbn(ocr_text, matches, isbn_max_num);
		for(int i = 0; i < isbn_max_num; i++)
			if(matches[i])
			{
				printf("%s\n", matches[i]);
				free(matches[i]);
			}

		free(ocr_text);
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
