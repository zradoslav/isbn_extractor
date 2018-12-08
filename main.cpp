#include "image.h"

#include <iostream>
#include <regex>

#include <unistd.h>

#include <tesseract/baseapi.h>

static tesseract::TessBaseAPI* api;

// later i'll add per-format embedded text extraction (if any)
static std::string extract_text_ocr(const image_t& image)
{
	api->SetImage(image.data, image.width, image.height,
	              image.bytes_per_pixel, image.bytes_per_line);
	return api->GetUTF8Text();
}

static std::vector<std::string> match_isbn(std::string text)
{
	static std::regex isbn_regex("isbn[- \\t]*?(1[03])?[: \\t]*([- \\d\\t]+[x\\d])",
	                             std::regex::ECMAScript | std::regex::icase);
	std::smatch match;

	std::vector<std::string> isbn_matches;
	while(std::regex_search(text, match, isbn_regex))
	{
		std::string isbn = match[2].str();
		/* remove all dashes */
		isbn.erase(std::remove(isbn.begin(), isbn.end(), '-'), isbn.end());
		isbn_matches.push_back(isbn);

		text = match.suffix();
	}

	if(!isbn_matches.empty())
	{
		std::sort(isbn_matches.begin(), isbn_matches.end());
		/* remove all dups */
		isbn_matches.erase(std::unique(isbn_matches.begin(), isbn_matches.end()));
	}

	return isbn_matches;
}

static const std::map<char, const char*> opt_desc = {
    { 't', "document type - pdf, djvu, ps, ..." },
    { 'n', "a number of pages to process" },
    { 'f', "document filename" },
    { 'l', "document language in ISO 639-3 (default: eng)" },
    { 'v', "be verbose" },
    { 'h', "print this help" }
};

static void help()
{
	printf("\n");
	for(auto& opt : opt_desc)
		printf(" -%c\t%s\n", opt.first, opt.second);
	printf("\n");
}

int main(int argc, char* argv[])
{
	int retcode = EXIT_SUCCESS;

	const char* type = nullptr;
	const char* file = nullptr;
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
			std::exit(retcode);
		default:
			printf("Usage: %s [-v] -t type -n pages [-l lang] [-f] file\n", argv[0]);
			retcode = EXIT_FAILURE;
			std::exit(retcode);
		}
	}
	if(!type || !page_count)
	{
		fprintf(stderr, "Invalid arguments\n");
		retcode = EXIT_FAILURE;
		std::exit(retcode);
	}
	if(!file)
	{
		if(optind + 1 == argc)
			file = argv[optind];
		else
		{
			fprintf(stderr, "No file passed\n");
			retcode = EXIT_FAILURE;
			std::exit(retcode);
		}
	}

	image_fn extract_images = nullptr;
	if(!strcasecmp(type, "djv") || !strcasecmp(type, "djvu"))
		extract_images = &extract_images_djv;
	else if(!strcasecmp(type, "pdf"))
		extract_images = &extract_images_pdf;
	else
	{
		fprintf(stderr, "Unsupported filetype\n");
		retcode = EXIT_FAILURE;
		std::exit(retcode);
	}

	/* initialize OCR */
	api = new tesseract::TessBaseAPI();
	if(!api)
	{
		retcode = EXIT_FAILURE;
		std::exit(retcode);
	}
	if(api->Init(nullptr, lang))
	{
		fprintf(stderr, "OCR engine init failure\n");
		api->End();
		retcode = EXIT_FAILURE;
		std::exit(retcode);
	}
	/* make Tesseract quiet */
	api->SetVariable("debug_file", "/dev/null");

	std::vector<image_t> images = extract_images(file, page_count);
	if(images.size() != page_count)
	{
		perror("Failed processing file");
		retcode = EXIT_FAILURE;
		goto clean_mem;
	}

	for(int i = 0; i < page_count; i++)
	{
		if(verbose)
			printf("page[%d]: %dx%d, %d kB\n", i, images[i].width, images[i].height,
			       images[i].bytes_per_pixel * images[i].width * images[i].height / 1024);

		std::string ocr_text = extract_text_ocr(images[i]);

		for(auto& m : match_isbn(ocr_text))
			std::cout << "ISBN: " << m << std::endl;
	}

clean_mem:
	for(auto& image : images)
		if(image.data)
			delete[] image.data;

	api->End();
	delete api;

	exit(retcode);
}
