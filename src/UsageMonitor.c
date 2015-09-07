#include <pebble.h>
#include "drawarc.h"

enum {
  SCREEN_WIDTH = 144,
  SCREEN_HEIGHT = 168,
  CIRCLE_THICKNESS = 5,
  HEADER_SIZE = 16
};

typedef struct {
  float percent;
  int radius;
} Arc;

static Window *window;
static TextLayer *text_layer;
static Layer *hours_layer;
static Layer *minutes_layer;
static Layer *seconds_layer;

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Select");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Down");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void arc_update_proc(Layer *layer, GContext *ctx) {
  Arc *arc = (Arc*) layer_get_data(layer);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Origin (%d, %d)", SCREEN_WIDTH / 2, (SCREEN_HEIGHT - HEADER_SIZE) / 2);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Radius %d, Percent %d", arc->radius, (int) (arc->percent * 100));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Start -90, End %d", (int) (360 * arc->percent - 90));

  GPoint origin = GPoint(SCREEN_WIDTH / 2, (SCREEN_HEIGHT - HEADER_SIZE) / 2);
  graphics_draw_arc(ctx, origin, arc->radius, CIRCLE_THICKNESS, -90, 360 * arc->percent - 90);
}

static void create_arc_layer(Layer *window_layer, GRect bounds, Layer *arc_layer, Arc *arc) {
  arc_layer = layer_create_with_data(bounds, sizeof(Arc));
  layer_set_update_proc(arc_layer, arc_update_proc);
  layer_add_child(window_layer, arc_layer);

  Arc *arc_ctx = (Arc*) layer_get_data(arc_layer);
  arc_ctx->radius = arc->radius;
  arc_ctx->percent = arc->percent;
}

static void update_arc(Layer *arc_layer, float p) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Percent %d", (int) (100 * p));

  Arc *arc_ctx = (Arc*) layer_get_data(arc_layer);
  arc_ctx->percent = p;
}

static void update_time() {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  update_arc(hours_layer, t->tm_hour / 12);
  update_arc(minutes_layer, t->tm_min / 60);
  update_arc(seconds_layer, t->tm_sec / 60);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  create_arc_layer(window_layer, bounds, hours_layer, &(Arc) {
    .percent = 0.75,
    .radius = 60
  });
  create_arc_layer(window_layer, bounds, minutes_layer, &(Arc) {
    .percent = 0.5,
    .radius = 40
  });
  create_arc_layer(window_layer, bounds, seconds_layer, &(Arc) {
    .percent = 0.25,
    .radius = 20
  });

  // update_arc(hours_layer, 0.95);

  // update_time();

  // text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 20 } });
  // text_layer_set_text(text_layer, "Press a button");
  // text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  // layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
  layer_destroy(hours_layer);
  layer_destroy(minutes_layer);
  layer_destroy(seconds_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void init(void) {
  window = window_create();
  // window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);
  // tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
