#pragma once

#include <pebble.h>


void graphics_draw_outer_dots(GContext *ctx, GRect bounds);

void graphics_fill_outer_ring(GContext *ctx, int32_t current_steps, int fill_thickness, GRect frame, GColor color, int day_average_steps );

void graphics_fill_goal_line(GContext *ctx, int32_t day_average_steps, int line_length, int line_width, GRect frame, GColor color, int current_average );

void graphics_set_window(Window *window);