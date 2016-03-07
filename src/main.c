#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_location_layer;

static AppSync s_sync;
static uint8_t s_sync_buffer[64];

static char weather_data[2][80];

enum WeatherKey { // TUPLE_CSTRING
  WEATHER_DESCRIPTION_KEY = 0x0,  
  WEATHER_TEMPERATURE_KEY = 0x1,  
  WEATHER_CITY_KEY = 0x2
};

static void tap_handler(AccelAxisType axis, int32_t direction) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "GOT TAPPED!!!!");
  app_message_outbox_send(); 
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  static char time_buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(time_buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(time_buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time); 
  
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, time_buffer);
  text_layer_set_text(s_date_layer, date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void set_text_layer(Window *window, TextLayer *layer, char *str, GFont font) {
  text_layer_set_background_color(layer, GColorBlack);
  text_layer_set_text_color(layer, GColorWhite);
  text_layer_set_text(layer, str);

  // Improve the layout to be more like a watchface
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(layer));
  
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC callback");
  switch (key) {
    case WEATHER_DESCRIPTION_KEY:
      strcpy(weather_data[0], new_tuple->value->cstring);
      break;
    case WEATHER_TEMPERATURE_KEY:
      // App Sync keeps new_tuple in s_sync_buffer, so we may use it directly
      strcpy(weather_data[1], new_tuple->value->cstring);
      break;

    case WEATHER_CITY_KEY:
      text_layer_set_text(s_location_layer, new_tuple->value->cstring);
      break;
  }
  static char label[ARRAY_LENGTH(weather_data[0]) + 2 + ARRAY_LENGTH(weather_data[1])];
  strcpy(label, weather_data[1]);
  APP_LOG(APP_LOG_LEVEL_DEBUG, label);
  strcat(label, " ");
  APP_LOG(APP_LOG_LEVEL_DEBUG, label);
  strcat(label, weather_data[0]);
  APP_LOG(APP_LOG_LEVEL_DEBUG, label);

  text_layer_set_text(s_weather_layer, label);
}

static void main_window_load(Window *window) {
  s_date_layer = text_layer_create(GRect(0, 5, 144, 30));
  set_text_layer(window, s_date_layer, "... ...", fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));

  s_time_layer = text_layer_create(GRect(0, 40, 144, 50));
  set_text_layer(window, s_time_layer, "00:00", fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));

  s_weather_layer = text_layer_create(GRect(0, 110, 144, 30));
  set_text_layer(window, s_weather_layer, "... ...", fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  
  s_location_layer = text_layer_create(GRect(0, 140, 144, 18));
  set_text_layer(window, s_location_layer, "...", fonts_get_system_font(FONT_KEY_GOTHIC_14));
  
  Tuplet initial_values[] = {
    TupletCString(WEATHER_TEMPERATURE_KEY, "Connecting…"),
    TupletCString(WEATHER_CITY_KEY, ""),
    TupletCString(WEATHER_DESCRIPTION_KEY, "")
  };

  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer),
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);
  
} 

static void main_window_unload(Window *window) {
    // Destroy TextLayer
    text_layer_destroy(s_time_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  // Make sure the time is displayed from the start
  update_time();
  accel_tap_service_subscribe(tap_handler); 
  app_message_open(64, 64);
}

static void deinit() {
    // Destroy Window
    window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
