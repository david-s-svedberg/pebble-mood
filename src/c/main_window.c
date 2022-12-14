// #include "main_window.h"

// #include <pebble.h>

// #include "main_window_logic.h"
// #include "icons.h"

// static Window *m_config_window;

// static StatusBarLayer* status_bar;

// static TextLayer* title_layer;
// static TextLayer* next_alarm_label_layer;
// static TextLayer* next_alarm_layer;

// static ActionBarLayer* main_window_action_bar_layer;

// static void main_window_click_config_provider(void* context)
// {
//     window_single_click_subscribe(BUTTON_ID_DOWN, goto_config_window);
//     window_long_click_subscribe(BUTTON_ID_SELECT, 500, take_next_medicine, NULL);
// }

// static void setup_main_window_action_bar_layer(Layer *window_layer, GRect bounds)
// {
//     main_window_action_bar_layer = action_bar_layer_create();
//     action_bar_layer_set_background_color(main_window_action_bar_layer, m_foreground_color);
//     action_bar_layer_add_to_window(main_window_action_bar_layer, m_config_window);
//     action_bar_layer_set_click_config_provider(main_window_action_bar_layer, main_window_click_config_provider);

//     action_bar_layer_set_icon_animated(main_window_action_bar_layer, BUTTON_ID_SELECT, get_check_icon(), true);
//     action_bar_layer_set_icon_animated(main_window_action_bar_layer, BUTTON_ID_DOWN, get_config_icon(), true);
// }

// static void appear_main_window(Window *window)
// {
//     update_next_alarm_time();
// }

// static void setup_next_alarm_layer(Layer *window_layer, GRect bounds)
// {
//     next_alarm_label_layer = text_layer_create(GRect(0, 80, bounds.size.w - 10, 30));
//     next_alarm_layer = text_layer_create(GRect(0, 104, bounds.size.w - 10, 30));

//     text_layer_set_background_color(next_alarm_label_layer, m_background_color);
//     text_layer_set_background_color(next_alarm_layer, m_background_color);
//     text_layer_set_text_color(next_alarm_label_layer, m_foreground_color);
//     text_layer_set_text_color(next_alarm_layer, m_foreground_color);
//     text_layer_set_font(next_alarm_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
//     text_layer_set_font(next_alarm_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
//     text_layer_set_text_alignment(next_alarm_label_layer, GTextAlignmentLeft);
//     text_layer_set_text_alignment(next_alarm_layer, GTextAlignmentLeft);

//     text_layer_set_text(next_alarm_label_layer, "Next Alarm:");

//     layer_add_child(window_layer, text_layer_get_layer(next_alarm_label_layer));
//     layer_add_child(window_layer, text_layer_get_layer(next_alarm_layer));
// }

// static void setup_title_layer(Layer *window_layer, GRect bounds)
// {
//     title_layer = text_layer_create(GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w - ACTION_BAR_WIDTH, 30));

//     text_layer_set_background_color(title_layer, get_ba);
//     text_layer_set_text_color(title_layer, m_foreground_color);
//     text_layer_set_font(title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
//     text_layer_set_text_alignment(title_layer, GTextAlignmentCenter);

//     text_layer_set_text(title_layer, "Mood");

//     layer_add_child(window_layer, text_layer_get_layer(title_layer));
// }

// static void setup_status_bar(Layer *window_layer, GRect bounds)
// {
//     status_bar = status_bar_layer_create();

//     status_bar_layer_set_colors(status_bar, m_background_color, m_foreground_color);
//     status_bar_layer_set_separator_mode(status_bar, StatusBarLayerSeparatorModeDotted);

//     layer_add_child(window_layer, status_bar_layer_get_layer(status_bar));
// }

// static void load_main_window(Window *window)
// {
//     window_set_background_color(window, m_background_color);
//     Layer *window_layer = window_get_root_layer(window);
//     GRect bounds = layer_get_bounds(window_layer);

//     setup_title_layer(window_layer, bounds);
//     setup_next_alarm_layer(window_layer, bounds);
//     setup_main_window_action_bar_layer(window_layer, bounds);
//     setup_status_bar(window_layer, bounds);

//     set_main_window_layers(next_alarm_layer);
// }

// static void unload_main_window(Window *window)
// {
//     status_bar_layer_destroy(status_bar);
//     text_layer_destroy(title_layer);
//     text_layer_destroy(next_alarm_layer);
//     text_layer_destroy(next_alarm_label_layer);
//     action_bar_layer_remove_from_window(main_window_action_bar_layer);
//     action_bar_layer_destroy(main_window_action_bar_layer);
// }

// void setup_main_window()
// {
//     m_config_window = window_create();

//     window_set_window_handlers(m_config_window, (WindowHandlers) {
//         .load = load_main_window,
//         .unload = unload_main_window,
//         .appear = appear_main_window
//     });

//     window_stack_push(m_config_window, true);
// }

// void tear_down_main_window()
// {
//     window_destroy(m_config_window);
// }