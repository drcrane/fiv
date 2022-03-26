#include "imgterm.h"

#include <stdio.h>

/*
 * Display a true colour image on the terminal using true colour terminal
 * escape codes. Assuming that the terminal is wide enough to accomodate
 * one character per pixel.
 */

void imgterm_dump(pixman_image_t * image) {
	unsigned int width, height;
	unsigned int x, y;
	unsigned int r, g, b;
	uint32_t * fb;
	uint32_t prev_pixel = 0;
	width = pixman_image_get_width(image);
	height = pixman_image_get_height(image);
	fb = pixman_image_get_data(image);
	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			uint32_t pixel = fb[y * width + x] & 0xffffff;
			r = (pixel >> 16) & 0xff;
			g = (pixel >> 8) & 0xff;
			b = pixel & 0xff;
			if (pixel != prev_pixel) {
			fprintf(stdout, "\x1b[48;2;%u;%u;%um ", r, g, b);
			} else {
				fprintf(stdout, " ");
			}
			prev_pixel = pixel;
		}
		prev_pixel = 0;
		fprintf(stdout, "\x1b[0m\n");
	}
}

