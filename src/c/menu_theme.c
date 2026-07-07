#include "menu_theme.h"

#include "repositories/app_config_repository.h"

void menu_theme_apply_colors(MenuLayer* menu_layer)
{
    menu_layer_set_normal_colors(menu_layer,
        config_get_background_color(), config_get_foreground_color());
    menu_layer_set_highlight_colors(menu_layer,
        config_get_foreground_color(), config_get_background_color());
}

bool menu_theme_icon_light(const Layer* cell_layer)
{
    bool highlighted = menu_cell_layer_is_highlighted(cell_layer);
    return highlighted ? !config_is_dark_theme() : config_is_dark_theme();
}

StatusBarLayer* status_bar_create_themed(Layer* window_layer)
{
    StatusBarLayer* status_bar = status_bar_layer_create();
    status_bar_layer_set_colors(status_bar,
        config_get_background_color(), config_get_foreground_color());
    status_bar_layer_set_separator_mode(status_bar, StatusBarLayerSeparatorModeDotted);
    layer_add_child(window_layer, status_bar_layer_get_layer(status_bar));
    return status_bar;
}
