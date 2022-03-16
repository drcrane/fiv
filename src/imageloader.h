#ifndef IMAGELOADER_H
#define IMAGELOADER_H
#include <pixman.h>

#ifdef __cplusplus
extern "C" {
#endif

enum imageloader_render_fittype {
	imageloader_render_fittype_stretch,
	imageloader_render_fittype_scale,
	imageloader_render_fittype_actual
};

int imageloader_load(pixman_image_t ** dst, char * filename);
int imageloader_render(pixman_image_t * dst, int offs_x, int offs_y,
		pixman_image_t * src, enum imageloader_render_fittype fittype);

#ifdef __cplusplus
};
#endif

#endif /* IMAGELOADER_H */
