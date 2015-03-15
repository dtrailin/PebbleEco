#include <pebble.h>
  
#define KEY_AQI 0
#define KEY_CITY 1
#define KEY_STATE 2  
  
static Window *s_main_window;

static TextLayer *s_time_layer;
static TextLayer *s_location_layer;
static TextLayer *s_aqi_layer;

static GFont s_time_font;
static GFont s_location_font;
static GFont s_aqi_font;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
}

static void main_window_load(Window *window) {
  
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(5, 20, 139, 70));
  text_layer_set_background_color(s_time_layer, GColorWhite);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  
  //Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_48));

  //Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

  // Create location layer
  s_location_layer = text_layer_create(GRect(0, 120, 145, 50)); // 2nd value changes y position, 1st value changes x positon
  text_layer_set_background_color(s_location_layer, GColorBlack);
  text_layer_set_text_color(s_location_layer, GColorWhite);
  text_layer_set_text_alignment(s_location_layer, GTextAlignmentCenter);

  // Create custom font for location layer, apply it and add to Window
  s_location_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_20));
  text_layer_set_font(s_location_layer, s_location_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_location_layer));

  // Create aqi layer
  s_aqi_layer = text_layer_create(GRect(0, 90 , 144, 25)); // 2nd value changes y position, 1st value changes x positon
  text_layer_set_background_color(s_aqi_layer, GColorBlack);
  text_layer_set_text_color(s_aqi_layer, GColorWhite);
  text_layer_set_text_alignment(s_aqi_layer, GTextAlignmentCenter);

  // Create custom font for air quality index, apply it and add to Window
  s_aqi_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_20));
  text_layer_set_font(s_aqi_layer, s_aqi_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer( s_location_layer ));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_aqi_layer));

    
  // Make sure the time is displayed from the start
  update_time();
}

static void main_window_unload(Window *window) {
  //Unload GFont
  fonts_unload_custom_font(s_time_font);
  
  //Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);

  //Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  
  // Destroy location elements
  text_layer_destroy(s_location_layer);
  fonts_unload_custom_font(s_location_font);
  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char aqi_buffer[8];
  static char city_buffer[32];
  static char state_buffer[8];
  static char location_buffer[32];

  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {

    case KEY_AQI:
      snprintf(aqi_buffer, sizeof(aqi_buffer), "AQI: %d", (int)t->value->int32);
      break;

    case KEY_CITY:
      snprintf(city_buffer, sizeof(city_buffer), "%s", t->value->cstring);
      break;

    case KEY_STATE:
      snprintf(state_buffer, sizeof(state_buffer), "%s", t->value->cstring);
      break;

    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  
  // Assemble full string and display
  snprintf(location_buffer, sizeof(location_buffer), "%s, %s", city_buffer, state_buffer);
  text_layer_set_text(s_location_layer, location_buffer);
  
  text_layer_set_text(s_aqi_layer, aqi_buffer);
  
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}
  
static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
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