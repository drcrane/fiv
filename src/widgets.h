#ifndef EPOCH_WIDGETS_H
#define EPOCH_WIDGETS_H

#include <pixman.h>

struct epoch_widgets_context {
	pixman_image_t * surface_img;
	pixman_image_t * highlight_img, * shadow_img;
	pixman_gradient_stop_t * gradient_a_stops;
	int gradient_a_stops_count;
	pixman_color_t button_caption_colour;
	//pixman_image_t * gradient_a_img;
	pixman_color_t frame_colour;
	int highlight_width, frame_width;
	struct epoch_widgets_widget ** widget_list;
	size_t widget_list_size;
};

enum epoch_widgets_type {
	EPOCH_WIDGETS_BUTTON = 0,
	EPOCH_WIDGETS_TITLEBAR
};

enum epoch_widgets_state {
	EPOCH_WIDGETS_DEFAULT = 0,
	EPOCH_WIDGETS_FOCUSSED,
	EPOCH_WIDGETS_PRESSED,
	EPOCH_WIDGETS_DISABLED
};

struct epoch_widgets_widget {
	enum epoch_widgets_type type;
};

struct epoch_widgets_titlebar {
	enum epoch_widgets_type type;
	char *title;
};

struct epoch_widgets_button {
	enum epoch_widgets_type type;
	int x, y;
	int width, height;
	enum epoch_widgets_state state;
	char * caption;
};

struct epoch_widgets_context * epoch_widgets_initialise();

struct epoch_widgets_button * epoch_button_create(struct epoch_widgets_context * ctx, int x, int y, int width, int height, char * caption);
int epoch_widgets_button_draw(struct epoch_widgets_context * ctx, struct epoch_widgets_button * button);

#endif // EPOCH_WIDGETS_H

