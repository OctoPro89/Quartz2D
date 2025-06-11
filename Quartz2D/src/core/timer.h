#pragma once

#include <common.h>

typedef struct {
    long long qpc_frequency;
    long long start_time;

    float delta_time;  // time in seconds for this frame
} timer;

void timer_init(timer* t);
void timer_begin(timer* t);
void timer_end(timer* t);