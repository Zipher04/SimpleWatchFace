#pragma once

void health_init();
void health_deinit();

int health_get_current_steps();
void health_set_current_steps(int value);

int health_get_current_average();
void health_set_current_average(int value);

int health_get_daily_average();
void health_set_daily_average(int value);

void health_reload_averages();

void health_update_steps_buffer();

char* health_get_current_steps_buffer();

int is_health_updated( void );