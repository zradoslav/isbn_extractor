#include "image.h"

#include <stdlib.h>
#include <string.h>

#include <mupdf/fitz.h>

int extract_images_pdf(const char* file, int page_count, image_t* ibuff)
{
	fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	if(!ctx || !ibuff) /* EFAULT? */
		return -1;

	fz_register_document_handlers(ctx);
	fz_document* doc = fz_open_document(ctx, file);
	if(!doc) /* ENOENT? */
	{
		fz_drop_context(ctx);
		return -2;
	}

	if(page_count < 0 || page_count >= fz_count_pages(ctx, doc)) /* EINVAL? */
	{
		fz_drop_document(ctx, doc);
		fz_drop_context(ctx);
		return -3;
	}

	fz_matrix ctm = fz_scale(1, 1);

	int page_read = 0;
	while(page_read != page_count)
	{
		fz_pixmap* pix = fz_new_pixmap_from_page_number(ctx, doc, page_read, ctm, fz_device_rgb(ctx), 0);
		if(!pix)
			break;

		ibuff[page_read].width = fz_pixmap_width(ctx, pix);
		ibuff[page_read].height = fz_pixmap_height(ctx, pix);
		ibuff[page_read].bytes_per_pixel = pix->n;
		ibuff[page_read].bytes_per_line = fz_pixmap_stride(ctx, pix);

		size_t ibuff_size = ibuff[page_read].width * ibuff[page_read].height * ibuff[page_read].bytes_per_pixel;
		ibuff[page_read].data = malloc(ibuff_size);
		memcpy(ibuff[page_read].data, fz_pixmap_samples(ctx, pix), ibuff_size);

		fz_drop_pixmap(ctx, pix);
		page_read++;
	}

	fz_drop_document(ctx, doc);
	fz_drop_context(ctx);

	return page_read;
}
