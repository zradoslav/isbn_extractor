#include "image.h"

#include <mupdf/fitz.h>

int extract_images_pdf(const char* file, int page_count, image_t* ibuff)
{
	fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	if(!ctx | !ibuff)
		return -1;

	fz_register_document_handlers(ctx);
	fz_document* doc = fz_open_document(ctx, file);
	fz_matrix ctm = fz_scale(1, 1);

	int page_read;
	for(page_read = 0; page_read < page_count; page_read++)
	{
		fz_pixmap* pix = fz_new_pixmap_from_page_number(ctx, doc, page_read, ctm, fz_device_rgb(ctx), 0);

		ibuff[page_read].data = fz_pixmap_samples(ctx, pix);
		ibuff[page_read].width = fz_pixmap_width(ctx, pix);
		ibuff[page_read].height = fz_pixmap_height(ctx, pix);
		ibuff[page_read].bytes_per_pixel = fz_pixmap_stride(ctx, pix);
		ibuff[page_read].bytes_per_line = pix->n;

		fz_drop_pixmap(ctx, pix);
	}

	fz_drop_document(ctx, doc);
	fz_drop_context(ctx);

	return page_read;
}
