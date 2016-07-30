#include <pebble.h>
#include "settings.c"

#define CIRCLE_THICKNESS        5
#define KEY_BATTERY_PERCENTAGE  0
#define KEY_SHOW_DATE           1
#define KEY_INVERT_COLOURS      2
#define KEY_BLUETOOTH_VIBRATE   3
#define KEY_HOURLY_VIBRATE      4

typedef struct {
  float percent;
  int radius;
} Arc;

static bool BATTERY_PERCENTAGE = true;
static bool SHOW_DATE = true;
static bool INVERT_COLOURS = false;
static bool BLUETOOTH_VIBRATE = false;
static bool HOURLY_VIBRATE = false;

static int MARGIN = 10;

bool bluetooth_connected = true;

static Window *window;
static TextLayer *battery_layer;
static TextLayer *date_layer;
static Layer *hours_layer;
static Layer *minutes_layer;


static void save_settings(DictionaryIterator *iter) {
  save_setting(iter, KEY_BATTERY_PERCENTAGE);
  save_setting(iter, KEY_SHOW_DATE);
  save_setting(iter, KEY_INVERT_COLOURS);
  save_setting(iter, KEY_BLUETOOTH_VIBRATE);
  save_setting(iter, KEY_HOURLY_VIBRATE);
}

static void update_settings() {
  BATTERY_PERCENTAGE = load_setting(KEY_BATTERY_PERCENTAGE, BATTERY_PERCENTAGE);
  SHOW_DATE = load_setting(KEY_SHOW_DATE, SHOW_DATE);
  INVERT_COLOURS = load_setting(KEY_INVERT_COLOURS, INVERT_COLOURS);
  BLUETOOTH_VIBRATE = load_setting(KEY_BLUETOOTH_VIBRATE, BLUETOOTH_VIBRATE);
  HOURLY_VIBRATE = load_setting(KEY_HOURLY_VIBRATE, HOURLY_VIBRATE);

  MARGIN = SHOW_DATE ? 10 : 0;
}

static void battery_handler(BatteryChargeState charge_state) {
  static char battery_text[16];

  snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  text_layer_set_text(battery_layer, battery_text);
}

static void bluetooth_handler(bool connected) {
  if (BLUETOOTH_VIBRATE && connected != bluetooth_connected) {
    bluetooth_connected = connected;
    vibes_double_pulse();
  }
}

static void arc_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  Arc *arc = (Arc*) layer_get_data(layer);

  GRect rect = GRect(
      bounds.size.w / 2 - arc->radius,
      bounds.size.h / 2 - arc->radius - MARGIN,
      arc->radius * 2,
      arc->radius * 2
  );

  graphics_context_set_stroke_color(ctx, INVERT_COLOURS ? GColorBlack : GColorWhite);
  graphics_context_set_stroke_width(ctx, CIRCLE_THICKNESS);

  graphics_draw_arc(
      ctx, rect,
      GOvalScaleModeFitCircle,
      0, DEG_TO_TRIGANGLE(360 * arc->percent)
  );
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

  update_arc(hours_layer, (float) (t->tm_hour > 12 ? t->tm_hour - 12 : t->tm_hour) / 12);
  update_arc(minutes_layer, (float) t->tm_min / 60);

  strftime(date_text, sizeof(date_text), "%B %e", t);
  text_layer_set_text(date_layer, date_text);

  if (HOURLY_VIBRATE && t->tm_min % 60 == 0) {
    vibes_short_pulse();
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  update_settings();

  window_set_background_color(window, INVERT_COLOURS ? GColorWhite : GColorBlack);

  hours_layer = create_arc_layer(window_layer, bounds, hours_layer, &(Arc) {
    .percent = 0,
    .radius = 40
  });
  minutes_layer = create_arc_layer(window_layer, bounds, minutes_layer, &(Arc) {
    .percent = 0,
    .radius = 60
  });

  battery_layer = text_layer_create(GRect(0, bounds.size.h / 2 - (10 + MARGIN), bounds.size.w, 20));
  text_layer_set_background_color(battery_layer, GColorClear);
  text_layer_set_text_color(battery_layer, INVERT_COLOURS ? GColorBlack : GColorWhite);
  text_layer_set_font(battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(battery_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(battery_layer));
  layer_set_hidden(text_layer_get_layer(battery_layer), BATTERY_PERCENTAGE == 0);

  date_layer = text_layer_create(GRect(0, bounds.size.h - 30, bounds.size.w, 20));
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_text_color(date_layer, INVERT_COLOURS ? GColorBlack : GColorWhite);
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(date_layer));
  layer_set_hidden(text_layer_get_layer(date_layer), SHOW_DATE == 0);

  battery_handler(battery_state_service_peek());
  bluetooth_connected = bluetooth_connection_service_peek();
  update_time();
}

static void window_unload(Window *window) {
  text_layer_destroy(battery_layer);
  text_layer_destroy(date_layer);
  layer_destroy(hours_layer);
  layer_destroy(minutes_layer);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  save_settings(iter);
  window_unload(window);
  window_load(window);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  battery_state_service_subscribe(battery_handler);
  bluetooth_connection_service_subscribe(bluetooth_handler);
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
