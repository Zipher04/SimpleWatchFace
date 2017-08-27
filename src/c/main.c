#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_weekday_layer;
static TextLayer *s_date_layer;

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
	
	// Write the current weekday and date into a buffer
  static char s_weekday_buffer[10];
  static char s_date_buffer[11];
  strftime( s_weekday_buffer, sizeof(s_weekday_buffer), "%A", tick_time);
  strftime( s_date_buffer, sizeof(s_date_buffer), "%F", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text( s_weekday_layer, s_weekday_buffer );
  text_layer_set_text( s_date_layer, s_date_buffer );
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


static void tick_minute_handler(struct tm *tick_time, TimeUnits units_changed)
{
  update_time();
	if ( 0 != ( units_changed & DAY_UNIT ) )
	{
		update_day();
	}
}

static void main_window_load(Window *window)
{
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 42), bounds.size.w, 50));
  s_weekday_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(30, 22), bounds.size.w, 30));
  s_date_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(115, 90), bounds.size.w, 25));

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

  // Add it as a child layer to the Window's root layer
  layer_add_child( window_layer, text_layer_get_layer(s_time_layer) );
  layer_add_child( window_layer, text_layer_get_layer(s_weekday_layer) );
  layer_add_child( window_layer, text_layer_get_layer(s_date_layer) );
}

static void main_window_unload(Window *window)
{
  // Destroy TextLayer
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_weekday_layer);
  text_layer_destroy(s_time_layer);
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
  
  // Make sure the time is displayed from the start
  update_time();
	update_day();
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_minute_handler);
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