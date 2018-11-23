#include "image.h"

#include <cstring>

#include <mupdf/fitz.h>

std::vector<image_t> extract_images_pdf(const char* file, int page_count)
{
	std::vector<image_t> result;

	fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	if(!ctx) /* EFAULT? */
		return result;

	fz_register_document_handlers(ctx);
	fz_document* doc = fz_open_document(ctx, file);
	if(!doc) /* ENOENT? */
	{
		fz_drop_context(ctx);
		return result;
	}

	if(page_count < 0 || page_count >= fz_count_pages(ctx, doc)) /* EINVAL? */
	{
		fz_drop_document(ctx, doc);
		fz_drop_context(ctx);
		return result;
	}

	fz_matrix ctm = fz_scale(1, 1);

	int page_read = 0;
	while(page_read != page_count)
	{
		fz_pixmap* pix = fz_new_pixmap_from_page_number(ctx, doc, page_read, ctm, fz_device_gray(ctx), 0);
		if(!pix)
			break;

		image_t img =
		{
			fz_pixmap_width(ctx, pix),
			fz_pixmap_height(ctx, pix),
			pix->n,
			fz_pixmap_stride(ctx, pix),
			nullptr
		};

		size_t ibuff_size = img.width * img.height * img.bytes_per_pixel;
		img.data = new unsigned char[ibuff_size];
		memcpy(img.data, fz_pixmap_samples(ctx, pix), ibuff_size);

		fz_drop_pixmap(ctx, pix);

		result.push_back(img);
		page_read++;
	}

	fz_drop_document(ctx, doc);
	fz_drop_context(ctx);

	return result;
}
