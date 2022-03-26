#include "minunit.h"
#include "imageloader.h"
#include "imgterm.h"

static void write_greenish_checkerbord_pattern(uint32_t * buf, int buf_width, int buf_height) {
	for (int y = 0; y < buf_height; ++y) {
		for (int x = 0; x < buf_width; ++x) {
			if ((x + y / 8 * 8) % 16 < 8) {
				buf[y*buf_width+x] = 0x80668866;
			} else {
				buf[y*buf_width+x] = 0x8099BB99;
			}
		}
	}
}

static uint32_t premultiply(uint32_t input) {
	uint32_t output;
	int a, r, g, b;
	a = (input >> 24) & 0xff;
	r = (input >> 16) & 0xff;
	g = (input >> 8) & 0xff;
	b = (input) & 0xff;
	r = (r * a / 255) & 0xff;
	g = (g * a / 255) & 0xff;
	b = (b * a / 255) & 0xff;
	a = a;
	output = a << 24 | r << 16 | g << 8 | b;
	return output;
}

static void premultiply_buf(uint32_t * buf, size_t buf_sz) {
	while (buf_sz --) {
		*buf = premultiply(*buf);
		buf ++;
	}
}

const char * free_validation_test() {
	pixman_image_t * image;
	int rc;
	rc = imageloader_load(&image, "testdata/image-x-generic.png");
	mu_assert(rc == 0, "could not load test image");
	debug("%dx%d", pixman_image_get_width(image), pixman_image_get_height(image));
	pixman_image_unref(image);
	return NULL;
}

const char * superimpose_image_test() {
	pixman_image_t * src_img;
	pixman_image_t * dst_img;
	uint32_t * bits;
	int32_t dst_width = 800, dst_height = 600;
	int rc;
	bits = malloc(dst_width * dst_height * 4);
	mu_assert(bits != NULL, "could not allocate required ram");
	dst_img = pixman_image_create_bits(PIXMAN_a8r8g8b8, dst_width, dst_height, bits, dst_width * 4);
	mu_assert(dst_img != NULL, "could not create the destination image");
	rc = imageloader_load(&src_img, "testdata/image-x-generic.png");
	mu_assert(rc == 0, "could not load sample image");
	rc = imageloader_render(dst_img, 0, 0, src_img,
			imageloader_render_fittype_actual);
	mu_assert(rc == 0, "image not correctly rendered");
	pixman_image_unref(src_img);
	pixman_image_unref(dst_img);
	free(bits);
	return NULL;
}

const char * imageloader_dump_pixels_test() {
	pixman_image_t * src_img;
	pixman_image_t * dst_img;
	int rc;
	uint32_t * src_buf, * dst_buf;
	dst_buf = malloc(32 * 32 * 4);
	dst_img = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, 32, 32, dst_buf, 32*4);
	write_greenish_checkerbord_pattern(dst_buf, 32, 32);
	rc = imageloader_load(&src_img, "testdata/image-x-generic.png");
	mu_assert(rc == 0, "loading image to pixman buffer");
	src_buf = pixman_image_get_data(src_img);
	mu_assert(src_buf != NULL, "trying to get buffer");
	fprintf(stdout, "=== BEFORE OVERLAYING IMAGE ===\n");
	fprintf(stdout, "DST PIXEL AT 1x4   %08x\n", dst_buf[32*4+1]);
	//src_buf[32*4+1] = premultiply(src_buf[32*4+1]);
	premultiply_buf(src_buf, 32*32);
	rc = imageloader_render(dst_img, 0, 0, src_img,
			imageloader_render_fittype_actual);
	mu_assert(rc == 0, "imageloader_render failed to render the image");
	/* A5BBC0BB AARRGGBB */
	fprintf(stdout, "SRC PIXEL AT 1x4   %08x\n", src_buf[32*4+1]);
	fprintf(stdout, "=== AFTER OVERLAYING IMAGE ===\n");
	fprintf(stdout, "DST PIXEL AT 1x4   %08x\n", dst_buf[32*4+1]);
	imgterm_dump(dst_img);
	pixman_image_unref(src_img);
	pixman_image_unref(dst_img);
	free(dst_buf);
	return NULL;
}

const char * all_tests() {
	mu_suite_start();
	mu_run_test(free_validation_test);
	mu_run_test(superimpose_image_test);
	mu_run_test(imageloader_dump_pixels_test);
	return NULL;
}

RUN_TESTS(all_tests)
