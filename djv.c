#include "image.h"

#include <libdjvu/ddjvuapi.h>

int extract_images_djv(const char* file, int page_count, image_t* ibuff)
{
	ddjvu_context_t* ctx = ddjvu_context_create("djv ctx");
	if(!ctx || !ibuff)
		return -1;

	ddjvu_document_t* doc = ddjvu_document_create_by_filename(ctx, file, 0);
	
	static unsigned int masks[4] = { 0xff0000, 0xff00, 0xff, 0xff000000 };
	ddjvu_format_t* format = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, masks);
	ddjvu_format_set_row_order(format, 1);

	int page_read;
	for(page_read = 0; page_read < page_count; page_read++)
	{
		ddjvu_page_t* page = ddjvu_page_create_by_pageno(doc, page_read);

		while(!ddjvu_page_decoding_done(page))
			continue;

		ddjvu_rect_t rect;
		rect.w = ddjvu_page_get_width(page);
		rect.h = ddjvu_page_get_height(page);

		ddjvu_page_type_t type = ddjvu_page_get_type(page);

		/*(type == DDJVU_PAGETYPE_BITONAL) ? (rect.w + 7) / 8 : rect.w * 3*/;
		unsigned int stride = rect.w * 4;
		
		unsigned char* buffer = malloc(stride * rect.h);
		ddjvu_page_render(page, DDJVU_RENDER_COLOR, &rect, &rect, format, stride, buffer);

		ibuff[page_read].data = buffer;
		ibuff[page_read].width = rect.w;
		ibuff[page_read].height = rect.h;
		ibuff[page_read].bytes_per_pixel = 3;
		ibuff[page_read].bytes_per_line = stride;

		ddjvu_page_release(page);
	}
	
	ddjvu_format_release(format);
	ddjvu_document_release(doc);
	ddjvu_context_release(ctx);

	return page_read;
}
