#include <core/timer.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void timer_init(timer* t) {
    QueryPerformanceFrequency((LARGE_INTEGER*)&t->qpc_frequency);
    QueryPerformanceCounter((LARGE_INTEGER*)&t->start_time);
    t->delta_time = 0.0167f;  // Default assumption ~60fps
}

void timer_begin(timer* t) {
    QueryPerformanceCounter((LARGE_INTEGER*)&t->start_time);
}

void timer_end(timer* t) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    float elapsed_sec = (float)((now.QuadPart - t->start_time) / (double)t->qpc_frequency);
    t->delta_time = elapsed_sec;
}