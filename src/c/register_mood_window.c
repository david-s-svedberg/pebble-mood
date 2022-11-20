// #include "register_mood_window.h"

// #include "repositories/app_config_repository.h"

// static Window* mood_window;
// static ActionBarLayer* action_bar;
// static StatusBarLayer* status_bar;
// static BitmapLayer* icon_layer;
// static TextLayer* score_layer;

// static void setup_mood_window_action_bar_layer(Layer *window_layer, GRect bounds)
// {
//     action_bar = action_bar_layer_create();
//     action_bar_layer_set_background_color(action_bar, config_get_foreground_color());
//     action_bar_layer_add_to_window(action_bar, mood_window);
// }

// static void setup_status_bar(Layer *window_layer, GRect bounds)
// {
//     status_bar = status_bar_layer_create();

//     status_bar_layer_set_colors(status_bar, config_get_background_color(), config_get_foreground_color());
//     status_bar_layer_set_separator_mode(status_bar, StatusBarLayerSeparatorModeDotted);

//     layer_add_child(window_layer, status_bar_layer_get_layer(status_bar));
// }

// static void setup_icon_layer(Layer *window_layer, GRect bounds)
// {
//     icon_layer = bitmap_layer_create(GRect(bounds.size.w - 45, 100, 50, 50));
//     bitmap_layer_set_compositing_mode(icon_layer, GCompOpSet);
//     layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));
// }

// static void setup_score_layer(Layer *window_layer, GRect bounds)
// {
//     score_layer = text_layer_create(GRect(bounds.size.w / 2, 10, 20, 20));

//     text_layer_set_background_color(score_layer, config_get_background_color());
//     text_layer_set_text_color(score_layer, config_get_foreground_color());
//     text_layer_set_font(score_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
//     text_layer_set_text_alignment(score_layer, GTextAlignmentCenter);

//     layer_add_child(window_layer, text_layer_get_layer(score_layer));
// }

// static void load_mood_window(Window *window)
// {
//     window_set_background_color(window, config_get_background_color());
//     Layer *window_layer = window_get_root_layer(window);
//     GRect bounds = layer_get_bounds(window_layer);

//     setup_mood_window_action_bar_layer(window_layer, bounds);
//     setup_status_bar(window_layer, bounds);
//     setup_icon_layer(window_layer, bounds);
//     setup_score_layer(window_layer, bounds);

//     setup_measurement_points();
// }

// static void unload_mood_window(Window *window)
// {

// }

// void setup_register_mood_window(GColor8 background_color, GColor8 foreground_color)
// {
//     mood_window = window_create();
//     window_set_window_handlers(mood_window, (WindowHandlers) {
//         .load = load_mood_window,
//         .unload = unload_mood_window
//     });

//     window_stack_push(mood_window, true);
// }

// void tear_down_register_mood_window()
// {
//     window_destroy(mood_window);
// }