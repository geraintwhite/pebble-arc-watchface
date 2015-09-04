#include <pebble.h>
#include "drawarc.h"

enum {
  SCREEN_WIDTH = 144,
  SCREEN_HEIGHT = 168,
  CIRCLE_THICKNESS = 5,
  HEADER_SIZE = 16
};

struct Arc {
  float percent;
  int radius;
  Layer layer;
  void update_proc;
};

static Window *window;
static TextLayer *text_layer;
static Layer *arc_layer;

struct Arc arc1;
arc1.percent = 75;
arc1.radius = 60;
arc1.update_proc = arc1_update_proc;

struct Arc arc2;
arc2.percent = 50;
arc2.radius = 40;
arc2.update_proc = arc2_update_proc;

struct Arc arc3;
arc3.percent = 25;
arc3.radius = 20;
arc3.update_proc = arc3_update_proc;

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

static void create_arc_layer(struct arc) {
  arc.layer = layer_create(bounds);
  layer_set_update_proc(arc.layer, arc.update_proc);
  layer_add_child(window_layer, arc.layer);
}

static void draw_arc(GContext *ctx, struct arc) {
  GPoint origin = GPoint(SCREEN_WIDTH / 2, (SCREEN_HEIGHT - HEADER_SIZE) / 2);
  graphics_draw_arc(ctx, origin, arc.radius, CIRCLE_THICKNESS, 270, 270 + 360 * arc.percent);
}

static void arc1_update_proc(Layer *layer, GContext *ctx) {
  draw_arc(ctx, arc1);
}

static void arc2_update_proc(Layer *layer, GContext *ctx) {
  draw_arc(ctx, arc2);
}

static void arc3_update_proc(Layer *layer, GContext *ctx) {
  draw_arc(ctx, arc3);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  create_arc_layer(arc1);
  create_arc_layer(arc2);
  create_arc_layer(arc3);

  // text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 20 } });
  // text_layer_set_text(text_layer, "Press a button");
  // text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  // layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
  layer_destroy(arc_layer);
}

static void init(void) {
  window = window_create();
  // window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
