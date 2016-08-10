#include "simple_analog.h"


#include <pebble.h>
#define COLORS       true
#define BATTERY_LEVEL 80
#define CIRCLE_RADIUS 4
  
static Window *window;
static Layer *s_simple_bg_layer, *s_date_layer, *s_hands_layer;
static TextLayer *s_num_label;

static GPath *s_tick_paths[NUM_CLOCK_TICKS];
static GPath *s_minute_arrow, *s_hour_arrow;
static char s_num_buffer[4];

//Battery Code//
Layer *battery_layer;
int batt_level;
  
static void handle_battery(BatteryChargeState charge_state) {

if (charge_state.is_charging) {
    batt_level = 100;
  } else {
    //snprintf(batt_level, 3, "%d", charge_state.charge_percent);
    batt_level = charge_state.charge_percent;  
  }
  
}

void battery_layer_update_callback(Layer *layer, GContext* ctx) {
  //graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorFromHEX(0x555555));
  graphics_context_set_fill_color(ctx, GColorFromHEX(0x555555));
  
  if (batt_level >= 80) {
    graphics_fill_circle(ctx, GPoint(4,4), CIRCLE_RADIUS);
    graphics_fill_circle(ctx, GPoint(13,4), CIRCLE_RADIUS);
    graphics_fill_circle(ctx, GPoint(22,4), CIRCLE_RADIUS);
    graphics_fill_circle(ctx, GPoint(31,4), CIRCLE_RADIUS);
  }
  if (batt_level >= 60 && batt_level < 80 ) {
    graphics_fill_circle(ctx, GPoint(4,4), CIRCLE_RADIUS);
    graphics_fill_circle(ctx, GPoint(13,4), CIRCLE_RADIUS);
    graphics_fill_circle(ctx, GPoint(22,4), CIRCLE_RADIUS);
    graphics_draw_circle(ctx, GPoint(31,4), CIRCLE_RADIUS);

  }
  if (batt_level >= 40 && batt_level < 60 ) {
    graphics_fill_circle(ctx, GPoint(4,4), CIRCLE_RADIUS);
    graphics_fill_circle(ctx, GPoint(13,4), CIRCLE_RADIUS);
    graphics_draw_circle(ctx, GPoint(22,4), CIRCLE_RADIUS);
    graphics_draw_circle(ctx, GPoint(31,4), CIRCLE_RADIUS);
  }
  if (batt_level >= 20 && batt_level < 40 ) {
    graphics_fill_circle(ctx, GPoint(4,4), CIRCLE_RADIUS);
    graphics_draw_circle(ctx, GPoint(13,4), CIRCLE_RADIUS);
    graphics_draw_circle(ctx, GPoint(22,4), CIRCLE_RADIUS);
    graphics_draw_circle(ctx, GPoint(31,4), CIRCLE_RADIUS);

  }
  if (batt_level < 20 ) {
    graphics_draw_circle(ctx, GPoint(4,4), CIRCLE_RADIUS);
    graphics_draw_circle(ctx, GPoint(13,4), CIRCLE_RADIUS);
    graphics_draw_circle(ctx, GPoint(22,4), CIRCLE_RADIUS);
    graphics_draw_circle(ctx, GPoint(31,4), CIRCLE_RADIUS);
  }
      
} 


static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorFromHEX(0xAAAAAA));
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite); 
  graphics_fill_circle(ctx, GPoint(72, 84), 71);

  graphics_context_set_fill_color(ctx, GColorBlack);
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    graphics_context_set_fill_color(ctx, GColorFromHEX(BAR_COLOR[i]));
    gpath_draw_filled(ctx, s_tick_paths[i]);

  }
    // Draw face
  graphics_context_set_stroke_color(ctx, GColorFromHEX(0xAAAAAA)); 
  graphics_draw_circle(ctx, GPoint(72, 84), 71);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_circle(ctx, GPoint(72, 84), 62);
  
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  int16_t second_hand_length = bounds.size.w / 2;

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  GPoint second_hand = {
    .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y,
  };

  // second hand
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, second_hand, center);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);

  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);

  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1, 3, 3), 0, GCornerNone);
  
  //Battery update
  handle_battery(battery_state_service_peek());
}

static void date_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  strftime(s_num_buffer, sizeof(s_num_buffer), "%d", t);
  text_layer_set_text(s_num_label, s_num_buffer);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(window));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  s_date_layer = layer_create(bounds);
  layer_set_update_proc(s_date_layer, date_update_proc);
  layer_add_child(window_layer, s_date_layer);

   //Load font
  ResHandle font_num = resource_get_handle(RESOURCE_ID_FONT_HELV_BOLD_30);

  s_num_label = text_layer_create(GRect(50, 100, 45, 35));
  text_layer_set_text(s_num_label, s_num_buffer);
  text_layer_set_background_color(s_num_label, GColorWhite);
  text_layer_set_text_color(s_num_label, GColorBlack);
  text_layer_set_text_alignment(s_num_label, GTextAlignmentCenter);
  text_layer_set_font(s_num_label, fonts_load_custom_font(font_num));

  layer_add_child(s_date_layer, text_layer_get_layer(s_num_label));

  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
  
  //Display Battery
  battery_layer = layer_create(GRect(0, 0, 144, 10));
  layer_set_update_proc(battery_layer, battery_layer_update_callback);
  layer_add_child(window_get_root_layer(window), (Layer*) battery_layer); 
  
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);
  layer_destroy(s_date_layer);
  text_layer_destroy(s_num_label);
  layer_destroy(s_hands_layer);
  layer_destroy(battery_layer);
}

static void init() {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  s_num_buffer[0] = '\0';

  // init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    s_tick_paths[i] = gpath_create(&ANALOG_BG_POINTS[i]);
  }

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}

static void deinit() {
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_destroy(s_tick_paths[i]);
  }

  tick_timer_service_unsubscribe();
  window_destroy(window);
  
  //Battery//
  //layer_destroy(battery_layer);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
