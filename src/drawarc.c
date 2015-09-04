#include <pebble.h>
#include "drawarc.h"

void graphics_draw_arc(GContext *ctx, GPoint centre, int radius, int thickness, float start_angle, float end_angle) {

	float minus_bit = (radius - thickness/2 + 0.5); //add 0.5 for the int casting, later
	float add_bit = (radius + thickness/2 + 0.5);

	for (float i = start_angle; i <= end_angle; i+=0.5) {

		GPoint inside_point = (GPoint) {
			.x = (int16_t)(sin_lookup(i * TRIG_MAX_ANGLE / 360) * minus_bit / TRIG_MAX_RATIO) + centre.x,
			.y = (int16_t)(-cos_lookup(i * TRIG_MAX_ANGLE / 360) * minus_bit / TRIG_MAX_RATIO) + centre.y,
		};

		GPoint outside_point = (GPoint) {
			.x = (int16_t)(sin_lookup(i * TRIG_MAX_ANGLE / 360) * add_bit / TRIG_MAX_RATIO) + centre.x,
			.y = (int16_t)(-cos_lookup(i * TRIG_MAX_ANGLE / 360) * add_bit / TRIG_MAX_RATIO) + centre.y,
		};

		graphics_draw_line(ctx, inside_point, outside_point);
	}
}
