#pragma once

#include <pebble.h>

// Shared helpers so every MenuLayer in the app renders with the app theme and
// inverts on the highlighted row (instead of the SimpleMenuLayer default, which
// ignores the theme and can't recolour icons per row).

// Sets the menu's normal colours to the theme (background / foreground) and the
// highlight colours to their inverse, so the selected row inverts text + icon.
void menu_theme_apply_colors(MenuLayer* menu_layer);

// True when a cell should draw the white (light) icon variant: the highlighted
// row in a light theme, or any normal row in a dark theme — i.e. whenever the
// cell's background is dark.
bool menu_theme_icon_light(const Layer* cell_layer);

// Creates a status bar in the theme colours with the app's dotted separator and
// adds it to window_layer. The caller owns it (destroy in .unload).
StatusBarLayer* status_bar_create_themed(Layer* window_layer);
