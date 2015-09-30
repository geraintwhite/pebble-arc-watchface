#include <pebble.h>
#include "drawarc.h"

enum {
  SCREEN_WIDTH = 144,
  SCREEN_HEIGHT = 168,
  CIRCLE_THICKNESS = 5,
  KEY_BATTERY_PERCENTAGE = 0,
  KEY_SHOW_DATE = 1
};

typedef struct {
  float percent;
  int radius;
} Arc;

static Window *window;
static TextLayer *battery_layer;
static TextLayer *date_layer;
static Layer *hours_layer;
static Layer *minutes_layer;

static void battery_handler(BatteryChargeState charge_state) {
  static char battery_text[16];

  snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  text_layer_set_text(battery_layer, battery_text);
}

static void arc_update_proc(Layer *layer, GContext *ctx) {
  Arc *arc = (Arc*) layer_get_data(layer);

  GPoint origin = GPoint(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 10);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_arc(ctx, origin, arc->radius, CIRCLE_THICKNESS, -90, 360 * arc->percent - 90);
}

static Layer *create_arc_layer(Layer *window_layer, GRect bounds, Layer *arc_layer, Arc *arc) {
  arc_layer = layer_create_with_data(bounds, sizeof(Arc));
  layer_set_update_proc(arc_layer, arc_update_proc);
  layer_add_child(window_layer, arc_layer);

  Arc *arc_ctx = (Arc*) layer_get_data(arc_layer);
  arc_ctx->radius = arc->radius;
  arc_ctx->percent = arc->percent;

  return arc_layer;
}

static void update_arc(Layer *arc_layer, float p) {
  Arc *arc_ctx = (Arc*) layer_get_data(arc_layer);
  float old_p = arc_ctx->percent;
  arc_ctx->percent = p;

  if (p != old_p) layer_mark_dirty(arc_layer);
}

static void update_time() {
  static char date_text[32];

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  update_arc(hours_layer, (float) t->tm_hour / 12);
  update_arc(minutes_layer, (float) t->tm_min / 60);

  strftime(date_text, sizeof(date_text), "%B %e", t);
  text_layer_set_text(date_layer, date_text);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  hours_layer = create_arc_layer(window_layer, bounds, hours_layer, &(Arc) {
    .percent = 0,
    .radius = 40
  });
  minutes_layer = create_arc_layer(window_layer, bounds, minutes_layer, &(Arc) {
    .percent = 0,
    .radius = 60
  });

  battery_layer = text_layer_create(GRect(0, SCREEN_HEIGHT / 2 - 20, SCREEN_WIDTH, 20));
  text_layer_set_background_color(battery_layer, GColorClear);
  text_layer_set_text_color(battery_layer, GColorWhite);
  text_layer_set_font(battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(battery_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(battery_layer));

  date_layer = text_layer_create(GRect(0, SCREEN_HEIGHT - 30, SCREEN_WIDTH, 20));
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(date_layer));

  battery_handler(battery_state_service_peek());
  update_time();
}

static void window_unload(Window *window) {
  text_layer_destroy(battery_layer);
  text_layer_destroy(date_layer);
  layer_destroy(hours_layer);
  layer_destroy(minutes_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void init(void) {
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  battery_state_service_subscribe(battery_handler);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
