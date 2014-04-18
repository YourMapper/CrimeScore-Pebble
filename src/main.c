#include <pebble.h>
	
const bool TESTING = false;

const bool DEFAULT_ALERT_STATE = true;
const int DEFAULT_THRESHOLD = 50;

static int DEFAULT_TIMER_INTERVAL = 120000;

static uint8_t iter = 0;
static bool fixFound = false;
static char *fix_icons[4];

static bool pushAlerts = false;
static int pushThreshold = 0;

//GUI stuff
static Window *window;
static Window *splash;
static Window *menu;

//MENU
static TextLayer *thresh_tl;
static TextLayer *instruct_tl;

//SPLASH
static BitmapLayer *image_layer_splash;
static GBitmap *image_splash;

static TextLayer *tw_tl;
static TextLayer *g_tl;

static AppTimer *splash_timer;
static AppTimer *refresh_timer;
static AppTimer *flick_timer;
static AppTimer *fix_timer;

//STATUS

static BitmapLayer *image_layer_badge;
static TextLayer *score_tl;
static TextLayer *location_tl;
static TextLayer *grade_tl;
static TextLayer *crimes_tl;

static GBitmap *image_badge;

//BUFFERS
char score_buffer[8];
char location_buffer[64];
char grade_buffer[32];
char message_buffer[32];
char crimes_buffer[32];
char thresh_buffer[8];

enum {
    SUCCESS = 1,
    REFRESH = 3,
	SCORE = 2,
	MSKEY = 4,
	GPSFIX = 5,
	LOCATION = 6,
	GRADE = 7,
	MESSAGE = 8,
	CRIMES = 9,
	ALERTS = 10,
	THRESHOLD = 11,
};

static void applog(AppMessageResult reason)
{
	if(reason == APP_MSG_OK)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG,"APP_MSG_OK");
		text_layer_set_text(location_tl,"APP_MSG_OK");
	} 

	else if(reason == APP_MSG_SEND_TIMEOUT)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG,"APP_MSG_SEND_TIMEOUT");
		text_layer_set_text(location_tl,"TIMEOUT");
	} 

	else if(reason == APP_MSG_SEND_REJECTED)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG,"APP_MSG_SEND_REJECTED");
		text_layer_set_text(location_tl,"REJECTED");
	}

	else if(reason == APP_MSG_NOT_CONNECTED)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG,"APP_MSG_NOT_CONNECTED");
		text_layer_set_text(location_tl,"NO CONNECT");
	}

	else if(reason == APP_MSG_APP_NOT_RUNNING)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG,"APP_MSG_APP_NOT_RUNNING");
		text_layer_set_text(location_tl,"NOT RUNNING");
	}

	else if(reason == APP_MSG_INVALID_ARGS)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG,"APP_MSG_INVALID_ARGS");
		text_layer_set_text(location_tl,"INVALID ARGS");
	}

	else if(reason == APP_MSG_BUSY)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG,"APP_MSG_BUSY");
		text_layer_set_text(location_tl,"MSG BUSY");
	}

	else if(reason == APP_MSG_BUFFER_OVERFLOW)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG,"APP_MSG_BUFFER_OVERFLOW");
		text_layer_set_text(location_tl,"BUFF OVERFLOW");
	}

	else if(reason == APP_MSG_ALREADY_RELEASED)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG,"APP_MSG_ALREADY_RELEASED");
		text_layer_set_text(location_tl,"APP_MSG_ALREADY_RELEASED");
	}

	else if(reason == APP_MSG_CALLBACK_ALREADY_REGISTERED)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG,"APP_MSG_CALLBACK_ALREADY_REGISTERED");
		text_layer_set_text(location_tl,"APP_MSG_CALLBACK_ALREADY_REGISTERED");
	}

	else if(reason == APP_MSG_CALLBACK_NOT_REGISTERED)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG,"APP_MSG_CALLBACK_NOT_REGISTERED");
		text_layer_set_text(location_tl,"APP_MSG_CALLBACK_NOT_REGISTERED");
	}

	else if(reason == APP_MSG_OUT_OF_MEMORY)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG,"APP_MSG_OUT_OF_MEMORY");
		text_layer_set_text(location_tl,"OUT OF MEMORY");
	}
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "enter select click");
	
	if (fixFound)
	{
		//launch the menu
		window_stack_push(menu, true);
	}
}


static void menu_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "menu enter select click");
	
	window_stack_remove(menu, true);
	
	
}

static void menu_up_click_handler(ClickRecognizerRef recognizer, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "menu up select click");
	if (pushThreshold < 95)
	{
		persist_write_int(THRESHOLD, pushThreshold = (pushThreshold + 5) % 100);
	}
	snprintf(thresh_buffer, sizeof(thresh_buffer), "%d", pushThreshold);
	text_layer_set_text(thresh_tl, thresh_buffer);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "THRESH");
	APP_LOG(APP_LOG_LEVEL_DEBUG, thresh_buffer);
	//do a little buzz here?
	static const uint32_t const segments[] = { 100 };
	VibePattern pat = {
		.durations = segments,
		.num_segments = ARRAY_LENGTH(segments),
	};
	vibes_enqueue_custom_pattern(pat);
}

static void menu_down_click_handler(ClickRecognizerRef recognizer, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "menu down select click");
	if (pushThreshold > 5)
	{
		persist_write_int(THRESHOLD, pushThreshold = (pushThreshold - 5) % 100);
	}
	snprintf(thresh_buffer, sizeof(thresh_buffer), "%d", pushThreshold);
	text_layer_set_text(thresh_tl, thresh_buffer);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "THRESH");
	APP_LOG(APP_LOG_LEVEL_DEBUG, thresh_buffer);
	//do a little buzz here?
	static const uint32_t const segments[] = { 100 };
	VibePattern pat = {
		.durations = segments,
		.num_segments = ARRAY_LENGTH(segments),
	};
	vibes_enqueue_custom_pattern(pat);
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "enter long select click");
	//turn the alerts on and off
	pushAlerts = pushAlerts ? false : true;
	
	if (pushAlerts)
	{
		static const uint32_t const segments[] = { 100 };
		
		text_layer_set_text(location_tl, "ALERTS ON");
		VibePattern pat = {
		.durations = segments,
		.num_segments = ARRAY_LENGTH(segments),
	};
	vibes_enqueue_custom_pattern(pat);
	}
	else
	{
		//buzz 
		static const uint32_t const segments[] = { 100, 100, 100 };
		text_layer_set_text(location_tl, "ALERTS OFF");
		VibePattern pat = {
		.durations = segments,
		.num_segments = ARRAY_LENGTH(segments),
	};
		vibes_enqueue_custom_pattern(pat);
	}
	
	persist_write_bool(ALERTS, pushAlerts);
}

static void reqRefresh()
{
    DictionaryIterator *it;
    app_message_outbox_begin(&it);
  
    Tuplet tuplet = TupletInteger(REFRESH, 1);
    dict_write_tuplet(it, &tuplet);

    dict_write_end(it);
    app_message_outbox_send();
}

static void sendKey()
{
	char *a;
	if (TESTING)
		a = "xxx"; // your key
	else
		a = "yyy"; // your key
	
	DictionaryIterator *it;
    app_message_outbox_begin(&it);
	
	//TODO: send stuff here
	Tuplet tupletKey = TupletCString(MSKEY, a); //send KEY first, then refresh
	dict_write_tuplet(it, &tupletKey);

	dict_write_end(it);
    app_message_outbox_send();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "up select click");

	//do a little buzz here?
	static const uint32_t const segments[] = { 20 };
	VibePattern pat = {
		.durations = segments,
		.num_segments = ARRAY_LENGTH(segments),
	};
	vibes_enqueue_custom_pattern(pat);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "down select click");
	
	//do a little buzz here?
	static const uint32_t const segments[] = { 20 };
	VibePattern pat = {
		.durations = segments,
		.num_segments = ARRAY_LENGTH(segments),
	};
	vibes_enqueue_custom_pattern(pat);
}

static void window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "enter window load");

}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, select_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void menu_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, menu_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, menu_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, menu_down_click_handler);
}

static void window_unload(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "enter window UNload");
  //app_sync_deinit(&sync);
	accel_tap_service_unsubscribe();
	
	text_layer_destroy(thresh_tl);
	text_layer_destroy(instruct_tl);
	  
	text_layer_destroy(score_tl);
	text_layer_destroy(location_tl);
	text_layer_destroy(grade_tl);
	text_layer_destroy(crimes_tl);

	bitmap_layer_destroy(image_layer_badge);
	
	gbitmap_destroy(image_badge);
	
	text_layer_destroy(tw_tl);
	text_layer_destroy(g_tl);
	
	bitmap_layer_destroy(image_layer_splash);
	
	gbitmap_destroy(image_splash);
  
	window_destroy(splash);
	window_destroy(menu);
	window_destroy(window);
	
}

static void refresh_callback(void *data) {
	reqRefresh();
	refresh_timer = app_timer_register(DEFAULT_TIMER_INTERVAL, refresh_callback, NULL);
}

static void fix_timer_callback(void *data)
{
	psleep(10);
	if (!fixFound)
	{
		iter = (iter + 1) % 4;
		text_layer_set_text(grade_tl, fix_icons[iter]);
		refresh_timer = app_timer_register(500, fix_timer_callback, NULL);
	}
}

static void process_tuple(Tuple *t)
{
  //Get key
  uint8_t key = t->key;
 
  //Get integer value, if present
  int value = t->value->int32;
 
  //Get string value, if present
  char string_value[32];
  strcpy(string_value, t->value->cstring);

  //Decide what to do
switch(key) {
	case CRIMES:
	//crimes_tl
		APP_LOG(APP_LOG_LEVEL_DEBUG, "CRIMES");
		snprintf(crimes_buffer, sizeof(crimes_buffer), "Area Crimes: %d", value);
		APP_LOG(APP_LOG_LEVEL_DEBUG, crimes_buffer);
		text_layer_set_text(crimes_tl, crimes_buffer);
	break;
	case MESSAGE:
		APP_LOG(APP_LOG_LEVEL_DEBUG, "MESSAGE");
		strcpy(message_buffer, string_value);
		APP_LOG(APP_LOG_LEVEL_DEBUG, message_buffer);
		text_layer_set_text(location_tl, message_buffer);
		text_layer_set_text(score_tl, "NA");
		text_layer_set_text(grade_tl, "NA");
		fixFound = true;
	break;
	case GPSFIX:
		APP_LOG(APP_LOG_LEVEL_DEBUG, "GPSFix");
		if (value == 0)
		{
			text_layer_set_text(location_tl, "NO GPS FIX!");
			fixFound = false;
			fix_timer = app_timer_register(500, fix_timer_callback, NULL);			
		}
	break;
	case SCORE:
		APP_LOG(APP_LOG_LEVEL_DEBUG, "SCORE");
		snprintf(score_buffer, sizeof(score_buffer), "%d", value);
		APP_LOG(APP_LOG_LEVEL_DEBUG, score_buffer);
		text_layer_set_text(score_tl, score_buffer);
		if (value <= pushThreshold && pushAlerts)
		{
			static const uint32_t const segments[] = { 100, 100, 100, 100, 100, 100, 100 };
				VibePattern pat = {.durations = segments,.num_segments = ARRAY_LENGTH(segments)};
            vibes_enqueue_custom_pattern(pat);
		}
	break;
	case GRADE:
		APP_LOG(APP_LOG_LEVEL_DEBUG, "GRADE");
		strcpy(grade_buffer, string_value);
		APP_LOG(APP_LOG_LEVEL_DEBUG, grade_buffer);
		text_layer_set_text(grade_tl, grade_buffer);
		fixFound = true;
	break;
	case LOCATION:
		APP_LOG(APP_LOG_LEVEL_DEBUG, "LOCATION");
		strcpy(location_buffer, string_value);
		APP_LOG(APP_LOG_LEVEL_DEBUG, location_buffer);
		text_layer_set_text(location_tl, location_buffer);
		fixFound = true;
	break;
	case MSKEY:
		sendKey();
		fix_icons[0] = ". . .";
		fix_icons[1] = "o . .";
		fix_icons[2] = ". o .";
		fix_icons[3] = ". . o";
		fix_timer = app_timer_register(500, fix_timer_callback, NULL);
		refresh_timer = app_timer_register(7000, refresh_callback, NULL);
	break;
	case SUCCESS:
	APP_LOG(APP_LOG_LEVEL_DEBUG, "handling success...like a boss");
	if (value)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG, "allgood");
	}
	else
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG, "ohnoes");
		if (!fix_timer)
		{
			fix_timer = app_timer_register(500, fix_timer_callback, NULL);
		}
	}
	break;
  }
}

static void flick_timer_callback(void *data)
{
	layer_set_hidden(text_layer_get_layer(score_tl), true);
	layer_set_hidden(text_layer_get_layer(grade_tl), false);
}

static void handle_accel_tap(AccelAxisType axis, int32_t direction)
{
	layer_set_hidden(text_layer_get_layer(grade_tl), true);
	layer_set_hidden(text_layer_get_layer(score_tl), false);
	flick_timer = app_timer_register(2000, flick_timer_callback, NULL);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
 APP_LOG(APP_LOG_LEVEL_DEBUG, "in_received_handler");
  Tuple *t = dict_read_first(iter);
  if(t)
  {
    process_tuple(t);
  }
   
  //Get next
  while(t != NULL)
  {
    t = dict_read_next(iter);
    if(t)
    {
      process_tuple(t);
    }
  }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "in_dropped_handler");
	applog(reason);
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "out_failed_handler");
	applog(reason);
}

static void timer_callback(void *data) {

	window_stack_remove(splash, true);
	window_stack_push(window, true);
}

void handle_init(void) 
{
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "enter init");
	
	window = window_create();
	splash = window_create();
	menu = window_create();

	accel_tap_service_subscribe(&handle_accel_tap);
	
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
	
	//set menu clicks here
	window_set_click_config_provider(menu, menu_click_config_provider);
	
    APP_LOG(APP_LOG_LEVEL_DEBUG, "opening app_message");
    app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_register_outbox_failed(out_failed_handler);
    
    app_message_open(256, 256);
	
//layer start setup
  
    Layer *window_layer = window_get_root_layer(window);
	Layer *splash_layer = window_get_root_layer(splash);
	Layer *menu_layer = window_get_root_layer(menu);
	
//MENU
	instruct_tl = text_layer_create(GRect(0, 0, 144, 49));
	thresh_tl = text_layer_create(GRect(0, 60, 144, 43));
	
	text_layer_set_text_color(instruct_tl, GColorBlack);
    text_layer_set_background_color(instruct_tl, GColorClear);
    text_layer_set_font(instruct_tl, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_text_alignment(instruct_tl, GTextAlignmentLeft);
	text_layer_set_overflow_mode(instruct_tl, GTextOverflowModeWordWrap);
	
	text_layer_set_text_color(thresh_tl, GColorBlack);
    text_layer_set_background_color(thresh_tl, GColorClear);
    text_layer_set_font(thresh_tl, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(thresh_tl, GTextAlignmentCenter);
	
	layer_add_child(menu_layer, text_layer_get_layer(instruct_tl));
	layer_add_child(menu_layer, text_layer_get_layer(thresh_tl));

//SCORE
	window_set_background_color(window, GColorBlack);
	
    score_tl = text_layer_create(GRect(0, 45, 144, 50));
	location_tl = text_layer_create(GRect(0, 105, 144, 15));
	grade_tl = text_layer_create(GRect(0, 45, 144, 50));
	crimes_tl = text_layer_create(GRect(0,91,144, 15));

	text_layer_set_text_color(crimes_tl, GColorBlack);
    text_layer_set_background_color(crimes_tl, GColorClear);
    text_layer_set_font(crimes_tl, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_text_alignment(crimes_tl, GTextAlignmentCenter);
	
	text_layer_set_text_color(location_tl, GColorBlack);
    text_layer_set_background_color(location_tl, GColorClear);
    text_layer_set_font(location_tl, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_text_alignment(location_tl, GTextAlignmentCenter);

    text_layer_set_text_color(score_tl, GColorBlack);
    text_layer_set_background_color(score_tl, GColorClear);
    text_layer_set_font(score_tl, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(score_tl, GTextAlignmentCenter);
	
	text_layer_set_text_color(grade_tl, GColorBlack);
    text_layer_set_background_color(grade_tl, GColorClear);
    text_layer_set_font(grade_tl, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(grade_tl, GTextAlignmentCenter);
    
    text_layer_set_text(score_tl, "00");
	text_layer_set_text(grade_tl, ". . .");
	text_layer_set_text(crimes_tl, "Area Crimes: NA");
	text_layer_set_text(location_tl, "Finding GPS");
	
	image_badge = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BADGE);

	image_layer_badge = bitmap_layer_create(GRect(0, 2,144,168));
	
	bitmap_layer_set_alignment(image_layer_badge, GAlignTop);
	
	bitmap_layer_set_bitmap(image_layer_badge, image_badge);
	
	bitmap_layer_set_compositing_mode(image_layer_badge, GCompOpAssign);
	
    //background before anything else
	layer_add_child(window_layer, bitmap_layer_get_layer(image_layer_badge));
    
	layer_add_child(window_layer, text_layer_get_layer(score_tl));
	layer_add_child(window_layer, text_layer_get_layer(location_tl));
	layer_add_child(window_layer, text_layer_get_layer(grade_tl));
	layer_add_child(window_layer, text_layer_get_layer(crimes_tl));
	
	layer_set_hidden(text_layer_get_layer(score_tl), true);
	
//SPLASH
	image_splash = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SPLASH);
	
	image_layer_splash = bitmap_layer_create(GRect(0,20,144,55));
	
	bitmap_layer_set_bitmap(image_layer_splash, image_splash);
	
	bitmap_layer_set_compositing_mode(image_layer_splash, GCompOpAssign );
	bitmap_layer_set_alignment(image_layer_splash, GAlignCenter);
	
	layer_add_child(splash_layer, bitmap_layer_get_layer(image_layer_splash));
	
	tw_tl = text_layer_create(GRect(9, 83, 144, 35));
    g_tl = text_layer_create(GRect(9, 113, 140, 35));
	
	text_layer_set_text_color(tw_tl, GColorBlack);
    text_layer_set_background_color(tw_tl, GColorClear);
    text_layer_set_font(tw_tl, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
    text_layer_set_text_alignment(tw_tl, GAlignLeft);
		
	text_layer_set_text_color(g_tl, GColorBlack);
    text_layer_set_background_color(g_tl, GColorClear);
    text_layer_set_font(g_tl, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
    text_layer_set_text_alignment(g_tl, GAlignLeft);
	
	text_layer_set_text(tw_tl, "by");  
	text_layer_set_text(g_tl, "STIdeas");
    
    layer_add_child(splash_layer, text_layer_get_layer(tw_tl));
	layer_add_child(splash_layer, text_layer_get_layer(g_tl));

	window_stack_push(splash, true);    
	if (TESTING) DEFAULT_TIMER_INTERVAL = 10000;
	
//TIMERS
	splash_timer = app_timer_register(1500 /* milliseconds */, timer_callback, NULL);
	
//STORAGE
	pushAlerts = persist_exists(ALERTS) ? persist_read_bool(ALERTS) : DEFAULT_ALERT_STATE;
	pushThreshold = persist_exists(THRESHOLD) ? persist_read_bool(THRESHOLD) : DEFAULT_THRESHOLD;

//CHECKING LOADED VARIABLES
	if (!persist_exists(ALERTS))
	{
		pushAlerts = DEFAULT_ALERT_STATE;
		persist_write_bool(ALERTS, pushAlerts);
	}
	
	if (!persist_exists(THRESHOLD) || pushThreshold < 5 || (pushThreshold % 5 != 0) || pushThreshold > 95)
	{
		pushThreshold = DEFAULT_THRESHOLD;
		persist_write_int(THRESHOLD, DEFAULT_THRESHOLD);
	}
	
	text_layer_set_text(instruct_tl, "Set your Crimescore alert threshold.");

	snprintf(thresh_buffer, sizeof(thresh_buffer), "%d", pushThreshold);
	text_layer_set_text(thresh_tl, thresh_buffer);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "THRESH");
	APP_LOG(APP_LOG_LEVEL_DEBUG, thresh_buffer);
}

void handle_deinit(void) {
	  
    
}

int main(void) {
	  handle_init();
	  app_event_loop();
	  handle_deinit();
}