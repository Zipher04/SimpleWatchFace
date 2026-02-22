#include <pebble.h>

#include "health.h"
#include "graphics.h"

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_weekday_layer;
static TextLayer *s_date_layer;
static TextLayer *s_step_layer;
static TextLayer *s_weather_layer;
static Layer *s_progress_layer;

static int s_battery_level;
static bool s_is_charging;
static int s_quiet_start = 22;
static int s_quiet_end = 8;

#define SETTINGS_PERSIST_KEY 1
typedef struct {
  int quiet_start;
  int quiet_end;
} __attribute__((__packed__)) SettingsStruct;

static void update_time( void )
{
  // Get a tm structure
  time_t temp_time = time(NULL);
  struct tm *tick_time = localtime(&temp_time);

  // Write the current hours and minutes into a buffer
  static char s_time_buffer[8];
  strftime( s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  
  // Display this time on the TextLayer
  text_layer_set_text( s_time_layer, s_time_buffer );
}

static void update_day( void )
{
  // Get a tm structure
  time_t temp_time = time(NULL);
  struct tm *tick_time = localtime(&temp_time);
	
  // Write the current weekday and date into a buffer
  static char s_weekday_buffer[10];
  static char s_date_buffer[11];
  strftime( s_weekday_buffer, sizeof(s_weekday_buffer), "%A", tick_time);
  strftime( s_date_buffer, sizeof(s_date_buffer), "%F", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text( s_weekday_layer, s_weekday_buffer );
  text_layer_set_text( s_date_layer, s_date_buffer );
}

static void update_step( void )
{
  text_layer_set_text( s_step_layer, health_get_current_steps_buffer() );
}

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  s_is_charging = state.is_charging;
  layer_mark_dirty(s_progress_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  static char temp_buffer[8];
  static char cond_buffer[32];
  static char weather_layer_buffer[40];

  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *cond_tuple = dict_find(iterator, MESSAGE_KEY_CONDITION);
  Tuple *qs_tuple = dict_find(iterator, MESSAGE_KEY_QUIET_START);
  Tuple *qe_tuple = dict_find(iterator, MESSAGE_KEY_QUIET_END);

  if(temp_tuple && cond_tuple) {
    if (strlen(cond_tuple->value->cstring) > 0) {
      snprintf(temp_buffer, sizeof(temp_buffer), "%d°C", (int)temp_tuple->value->int32);
      snprintf(cond_buffer, sizeof(cond_buffer), "%s", cond_tuple->value->cstring);
      snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s %s", cond_buffer, temp_buffer);
      text_layer_set_text(s_weather_layer, weather_layer_buffer);
    } else {
      text_layer_set_text(s_weather_layer, "");
    }
  }
  
  if (qs_tuple && qe_tuple) {
    s_quiet_start = qs_tuple->value->int32;
    s_quiet_end = qe_tuple->value->int32;
    
    SettingsStruct settings = {
        .quiet_start = s_quiet_start,
        .quiet_end = s_quiet_end
    };
    persist_write_data(SETTINGS_PERSIST_KEY, &settings, sizeof(settings));
    APP_LOG(APP_LOG_LEVEL_INFO, "Quiet hours updated: %d - %d", s_quiet_start, s_quiet_end);
  }
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

static void progress_update_proc(Layer *layer, GContext *ctx) 
{
  GRect bounds = layer_get_bounds(layer);
  const int fill_thickness = PBL_IF_RECT_ELSE(8, (180 - grect_inset(bounds, GEdgeInsets(12)).size.h) / 2);
  int current_steps = health_get_current_steps();
  int daily_average = health_get_daily_average();
  int current_average = health_get_current_average();

  // Set new exceeded daily average
  if(current_steps > daily_average) {
    daily_average = current_steps;
  }

  // Decide color scheme based on progress to/past goal
  GColor scheme_color;
  if(current_steps >= current_average) {
    scheme_color = GColorJaegerGreen;
  } else {
    scheme_color = GColorPictonBlue;
  }

  // Perform drawing
  // Draw battery ring (outermost)
  GColor battery_color;
  if (s_is_charging) {
    battery_color = GColorJaegerGreen;
  } else if (s_battery_level < 30) {
    battery_color = GColorRed;
  } else {
    battery_color = GColorYellow;
  }
  graphics_fill_outer_ring(ctx, s_battery_level, 2, bounds, battery_color, 100);

  // Draw steps ring (inside battery ring)
  GRect step_bounds = grect_inset(bounds, GEdgeInsets(2));
  graphics_fill_outer_ring(ctx, current_steps, fill_thickness, step_bounds, scheme_color, daily_average );
  graphics_fill_goal_line(ctx, daily_average, 8, 4, step_bounds, GColorYellow, current_average );
}

static void tick_minute_handler(struct tm *tick_time, TimeUnits units_changed)
{
	update_time();
	//health_reload_averages(0);
	if ( is_health_updated() )
	{
	  health_reload_averages(0);
	  update_step();
	  layer_mark_dirty(s_progress_layer);
	}
	if ( 0 != ( units_changed & HOUR_UNIT ) )
	{
	  // Hourly vibration check
	  bool quiet = false;
	  int hour = tick_time->tm_hour;
	  
	  if (s_quiet_start < s_quiet_end) {
	    // Quiet range is within the same day (e.g., 9-17)
	    if (hour >= s_quiet_start && hour < s_quiet_end) quiet = true;
	  } else {
	    // Quiet range spans midnight (e.g., 22-8)
	    if (hour >= s_quiet_start || hour < s_quiet_end) quiet = true;
	  }
	  
	  if (!quiet) {
	    vibes_double_pulse();
	  }

	  DictionaryIterator *iter;
	  app_message_outbox_begin(&iter);
	  if (iter) {
	    dict_write_uint8(iter, 0, 0); // Send a dummy value to trigger JS
	    app_message_outbox_send();
	  }
	}
	if ( 0 != ( units_changed & DAY_UNIT ) )
	{
		update_day();
		health_reload_averages(0);
	}
}

static void main_window_load(Window *window)
{
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  health_init();
	
  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(46, 30), bounds.size.w, 50));
  s_weekday_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(18, 10), bounds.size.w, 30));
  s_date_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(95, 78), bounds.size.w, 25));
  s_step_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(115, 103), bounds.size.w, 25));
  s_weather_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(140, 128), bounds.size.w, 25));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color( s_time_layer, GColorClear );
  text_layer_set_text_color( s_time_layer, GColorWhite );
  text_layer_set_font( s_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49) );
  text_layer_set_text_alignment( s_time_layer, GTextAlignmentCenter );
  
  text_layer_set_background_color( s_weekday_layer, GColorClear );
  text_layer_set_text_color( s_weekday_layer, GColorWhite );
  text_layer_set_font( s_weekday_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD) );
  text_layer_set_text_alignment( s_weekday_layer, GTextAlignmentCenter );
  
  text_layer_set_background_color( s_date_layer, GColorClear );
  text_layer_set_text_color( s_date_layer, GColorWhite );
  text_layer_set_font( s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD) );
  text_layer_set_text_alignment( s_date_layer, GTextAlignmentCenter );
	
  text_layer_set_background_color( s_step_layer, GColorClear );
  text_layer_set_text_color( s_step_layer, GColorWhite );
  text_layer_set_font( s_step_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24) );
  text_layer_set_text_alignment( s_step_layer, GTextAlignmentCenter );
  //text_layer_set_text( s_step_layer, "\U0001F4951,234" );

  text_layer_set_background_color( s_weather_layer, GColorClear );
  text_layer_set_text_color( s_weather_layer, GColorWhite );
  text_layer_set_font( s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18) );
  text_layer_set_text_alignment( s_weather_layer, GTextAlignmentCenter );
  //text_layer_set_text( s_weather_layer, "Cloudy, 22°C" );
	
  graphics_set_window( window );
  s_progress_layer = layer_create( bounds );
  layer_set_update_proc( s_progress_layer, progress_update_proc );
  
  // Add it as a child layer to the Window's root layer
  layer_add_child( window_layer, text_layer_get_layer(s_weekday_layer) );
  layer_add_child( window_layer, text_layer_get_layer(s_date_layer) );
  layer_add_child( window_layer, s_progress_layer);
  layer_add_child( window_layer, text_layer_get_layer(s_step_layer) );
  layer_add_child( window_layer, text_layer_get_layer(s_weather_layer) );
  layer_add_child( window_layer, text_layer_get_layer(s_time_layer) );
	
  // Make sure the time is displayed from the start
  health_reload_averages(0);
  update_time();
  update_day();
  update_step();
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_minute_handler);

  // Load settings
  if (persist_exists(SETTINGS_PERSIST_KEY)) {
    SettingsStruct settings;
    persist_read_data(SETTINGS_PERSIST_KEY, &settings, sizeof(settings));
    s_quiet_start = settings.quiet_start;
    s_quiet_end = settings.quiet_end;
  }

  // Register with BatteryStateService
  battery_state_service_subscribe(battery_callback);
  battery_callback(battery_state_service_peek());

  // AppMessage listeners
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
}

static void main_window_unload(Window *window)
{
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();

  // Destroy TextLayer
  layer_destroy(s_progress_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_step_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_weekday_layer);
  text_layer_destroy(s_time_layer);
	
  health_deinit();
}

static void Initialize( void )
{
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers( s_main_window, 
                             (WindowHandlers) {
                                .load = main_window_load,
                                .unload = main_window_unload
                              }
  );
  window_set_background_color( s_main_window, GColorBlack );
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, false);
  
  
}

static void Finalize( void )
{
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) 
{
  Initialize();
  app_event_loop();
  Finalize();
}