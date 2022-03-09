#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <pixman.h>

#include "widgets.h"
#include "dbg.h"

#define POINT(x,y) { pixman_double_to_fixed((x)), pixman_double_to_fixed((y)) }
#define POINTI(x,y) { pixman_int_to_fixed((x)), pixman_int_to_fixed((y)) }

static void epoch_widgets_draw_highlight(struct epoch_widgets_context * ctx, int x, int y, int width, int height, int highlight_width) {
	pixman_trapezoid_t highlight_traps[] = {
		{ pixman_int_to_fixed(y),
		  pixman_int_to_fixed(y + highlight_width),
			{ POINTI(x,y),
			  POINTI(x,y+highlight_width) },
			{ POINTI(x+width,y),
			  POINTI(x+width-highlight_width,y+highlight_width) } },
		{ pixman_int_to_fixed(y+highlight_width),
		  pixman_int_to_fixed(y+height-highlight_width),
			{ POINTI(x,y+highlight_width),
			  POINTI(x,y+height-highlight_width) },
			{ POINTI(x+highlight_width,y+highlight_width),
			  POINTI(x+highlight_width,y+height-highlight_width) } },
		{ pixman_int_to_fixed(y+height-highlight_width),
		  pixman_int_to_fixed(y+height),
			{ POINTI(x,y+height-highlight_width),
			  POINTI(x,y+height) },
			{ POINTI(x,y+height),
			  POINTI(x+highlight_width,y+height-highlight_width) } }
	};
	pixman_trapezoid_t shadow_traps[] = {
		{ pixman_int_to_fixed(y + height - highlight_width),
		  pixman_int_to_fixed(y + height),
			{ POINTI(x+highlight_width,y+height-highlight_width),
			  POINTI(x,y+height) },
			{ POINTI(x+width,y+height-highlight_width),
			  POINTI(x+width,y+height) } },
		{ pixman_int_to_fixed(y),
		  pixman_int_to_fixed(y + highlight_width),
			{ POINTI(x+width,y),
			  POINTI(x+width-highlight_width,y+highlight_width) },
			{ POINTI(x+width,y),
			  POINTI(x+width,y+highlight_width) } },
		{ pixman_int_to_fixed(y + highlight_width),
		  pixman_int_to_fixed(y + height - highlight_width),
			{ POINTI(x+width-highlight_width,y+highlight_width),
			  POINTI(x+width-highlight_width,y+height-highlight_width) },
			{ POINTI(x+width,y+highlight_width),
			  POINTI(x+width,y+height-highlight_width) } }
	};

	pixman_composite_trapezoids(PIXMAN_OP_OVER, ctx->highlight_img, ctx->surface_img, PIXMAN_a8, 0, 0, 0, 0, 3, highlight_traps);
	pixman_composite_trapezoids(PIXMAN_OP_OVER, ctx->shadow_img, ctx->surface_img, PIXMAN_a8, 0, 0, 0, 0, 3, shadow_traps);

}

int epoch_widgets_draw_empty_rectangle(struct epoch_widgets_context * ctx, pixman_color_t *colour, int x, int y, int width, int height, int thickness) {
	pixman_rectangle16_t rectTop = { x, y, width, thickness };
	pixman_rectangle16_t rectBot = { x, y + height - thickness, width, thickness };
	pixman_rectangle16_t rectLeft = { x, y + thickness, thickness, height - (thickness * 2) };
	pixman_rectangle16_t rectRight = { x + width - thickness, y + thickness, thickness, height - (thickness * 2) };
	pixman_image_fill_rectangles(PIXMAN_OP_OVER, ctx->surface_img, colour, 1, &rectTop);
	pixman_image_fill_rectangles(PIXMAN_OP_OVER, ctx->surface_img, colour, 1, &rectBot);
	pixman_image_fill_rectangles(PIXMAN_OP_OVER, ctx->surface_img, colour, 1, &rectLeft);
	pixman_image_fill_rectangles(PIXMAN_OP_OVER, ctx->surface_img, colour, 1, &rectRight);
	return 0;
}

struct epoch_widgets_context * epoch_widgets_initialise() {
	pixman_gradient_stop_t stops[] = {
		{ 0x00000, { 69 << 8, 136 << 8, 105 << 8, 0xffff } },
		{ 0x05000, { 33 << 8,  76 << 8,  56 << 8, 0xffff } },
		{ 0x08000, { 69 << 8, 136 << 8, 105 << 8, 0xffff } },
		{ 0x10000, { 19 << 8,  43 << 8,  32 << 8, 0xffff } }
	};
	struct epoch_widgets_context * ctx = malloc(sizeof(struct epoch_widgets_context));
	if (ctx == NULL) { return NULL; }
	ctx->gradient_a_stops = malloc(sizeof(pixman_gradient_stop_t) * 4);
	memcpy(ctx->gradient_a_stops, stops, sizeof(pixman_gradient_stop_t) * 4);
	ctx->gradient_a_stops_count = 4;
	ctx->frame_colour.red = 0;
	ctx->frame_colour.blue = 0;
	ctx->frame_colour.green = 0;
	ctx->frame_colour.alpha = 0xffff;
	ctx->button_caption_colour.red = 0xffff;
	ctx->button_caption_colour.blue = 0xffff;
	ctx->button_caption_colour.green = 0xffff;
	ctx->button_caption_colour.alpha = 0xffff;
	pixman_color_t highlight_colour = { 0x8080, 0x8080, 0x8080, 0x8080 };
	pixman_color_t shadow_colour = { 0x0000, 0x0000, 0x0000, 0x8000 };
	ctx->highlight_img = pixman_image_create_solid_fill(&highlight_colour);
	ctx->shadow_img = pixman_image_create_solid_fill(&shadow_colour);
	ctx->frame_width = 2;
	ctx->highlight_width = 3;
	return ctx;
}

struct epoch_widgets_button * epoch_button_create(struct epoch_widgets_context * ctx, int x, int y, int width, int height, char * caption) {
	struct epoch_widgets_button * button;
	button = malloc(sizeof(struct epoch_widgets_button));
	if (button == NULL) {
		return NULL;
	}
	button->type = EPOCH_WIDGETS_BUTTON;
	button->x = x;
	button->y = y;
	button->width = width;
	button->height = height;
	button->caption = caption;
	return button;
}

int epoch_widgets_button_draw(struct epoch_widgets_context * ctx, struct epoch_widgets_button * button) {
	epoch_widgets_draw_empty_rectangle(ctx, &ctx->frame_colour, button->x, button->y, button->width, button->height, ctx->frame_width);

	pixman_image_t *gradient;
	pixman_point_fixed_t p1, p2;
	p1.x = p1.y = 0;
	p2.x = pixman_int_to_fixed(button->width);
	p2.y = pixman_int_to_fixed(button->height * 5);
	gradient = pixman_image_create_linear_gradient(&p1, &p2, ctx->gradient_a_stops, ctx->gradient_a_stops_count);
	pixman_image_composite32(PIXMAN_OP_OVER, gradient, NULL, ctx->surface_img, 0,0, 0,0,
			button->x + ctx->frame_width, button->y + ctx->frame_width,
			button->width - (ctx->frame_width * 2), button->height - (ctx->frame_width * 2));
	pixman_image_unref(gradient);

	epoch_widgets_draw_highlight(ctx,
			button->x + ctx->frame_width, button->y + ctx->frame_width,
			button->width - (ctx->frame_width * 2), button->height - (ctx->frame_width * 2),
			ctx->highlight_width);
	return 0;
}

