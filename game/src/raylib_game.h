#pragma once

void draw_spiral_name(int* active_spiral);

void draw_spiral_color(int* active_channel, Spiral* spirals, int* active_spiral);

void process_key_right(int* active_channel, int* active_spiral);

void process_key_left(int* active_channel, int* active_spiral);

void process_key_up(int* active_channel, Spiral* spirals, int* active_spiral);

void process_key_down(int* active_channel, Spiral* spirals, int* active_spiral);
