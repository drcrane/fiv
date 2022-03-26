#include "imageloader.h"
#include "lodepng.h"
#include <malloc.h>
#include <string.h>

static void imageloader_destroy_image(pixman_image_t * image, void * data) {
	free(data);
}

int imageloader_load(pixman_image_t ** dst, char * filename) {
	pixman_image_t * image;
	unsigned char * buffer;
	unsigned width, height;
	unsigned rc;
	rc = lodepng_decode32_file(&buffer, &width, &height, filename);
	if (rc != 0) {
		return (int)rc;
	}
	image = pixman_image_create_bits_no_clear(PIXMAN_a8b8g8r8, (int)width, (int)height, (uint32_t *)buffer, width * 4);
	if (image == NULL) {
		free(buffer);
		return -1;
	}
	pixman_image_set_destroy_function(image, imageloader_destroy_image, buffer);
	*dst = image;
	return 0;
}

int imageloader_render(pixman_image_t * dst, int offs_x, int offs_y,
		pixman_image_t * src, enum imageloader_render_fittype fittype) {
	if (fittype == imageloader_render_fittype_stretch ||
			fittype == imageloader_render_fittype_scale ||
			fittype == imageloader_render_fittype_actual) {
		int32_t x, y, width, height;
		int32_t d_width, d_height;
		width = pixman_image_get_width(src);
		height = pixman_image_get_height(src);
		d_width = pixman_image_get_width(dst);
		d_height = pixman_image_get_height(dst);
		x = 0;
		y = 0;
		if (width < d_width) {
			x = (d_width >> 1) - (width >> 2);
		}
		if (height < d_height) {
			y = (d_height >> 1) - (height >> 2);
		}
		pixman_image_composite32(PIXMAN_OP_OVER,
				src, NULL, dst,
				0,0, 0,0, x,y, width,height
				);
		return 0;
	}
	return 1;
}

