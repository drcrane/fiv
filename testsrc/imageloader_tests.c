#include "minunit.h"
#include "imageloader.h"

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

const char * all_tests() {
	mu_suite_start();
	mu_run_test(free_validation_test);
	mu_run_test(superimpose_image_test);
	return NULL;
}

RUN_TESTS(all_tests)
