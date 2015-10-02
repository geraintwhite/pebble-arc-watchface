#include <pebble.h>
#include "drawarc.h"

#define CIRCLE_THICKNESS        5
#define KEY_BATTERY_PERCENTAGE  0
#define KEY_SHOW_DATE           1

typedef struct {
  float percent;
  int radius;
} Arc;

static int BATTERY_PERCENTAGE = 1;
static int SHOW_DATE = 1;

static int MARGIN = 0;

static Window *window;
static TextLayer *battery_layer;
static TextLayer *date_layer;
static Layer *hours_layer;
static Layer *minutes_layer;


static void update_settings() {
  if (persist_exists(KEY_BATTERY_PERCENTAGE)) {
    BATTERY_PERCENTAGE = persist_read_int(KEY_BATTERY_PERCENTAGE);
  }
  layer_set_hidden(text_layer_get_layer(battery_layer), BATTERY_PERCENTAGE == 0);

  if (persist_exists(KEY_SHOW_DATE)) {
    SHOW_DATE = persist_read_int(KEY_SHOW_DATE);
  }
  layer_set_hidden(text_layer_get_layer(date_layer), SHOW_DATE == 0);

  MARGIN = SHOW_DATE ? 10 : 0;

  GRect bounds = layer_get_bounds((Layer*) battery_layer);
  bounds.origin.y = bounds.size.h / 2 - (10 + MARGIN);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *show_battery = dict_find(iter, KEY_BATTERY_PERCENTAGE);
  if (show_battery) {
    persist_write_int(KEY_BATTERY_PERCENTAGE, show_battery->value->uint8);
  }

  Tuple *show_date = dict_find(iter, KEY_SHOW_DATE);
  if (show_date) {
    persist_write_int(KEY_SHOW_DATE, show_date->value->uint8);
  }

  update_settings();
}

static void battery_handler(BatteryChargeState charge_state) {
  static char battery_text[16];

  snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  text_layer_set_text(battery_layer, battery_text);
}

static void arc_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  Arc *arc = (Arc*) layer_get_data(layer);

  GPoint origin = GPoint(bounds.size.w / 2, bounds.size.h / 2 - MARGIN);
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

  battery_layer = text_layer_create(GRect(0, bounds.size.h / 2 - 20, bounds.size.w, 20));
  text_layer_set_background_color(battery_layer, GColorClear);
  text_layer_set_text_color(battery_layer, GColorWhite);
  text_layer_set_font(battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(battery_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(battery_layer));

  date_layer = text_layer_create(GRect(0, bounds.size.h - 30, bounds.size.w, 20));
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(date_layer));

  battery_handler(battery_state_service_peek());
  update_settings();
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

  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
