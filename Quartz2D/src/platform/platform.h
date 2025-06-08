#pragma once
#include <common.h>
#include <platform/input/input.h>

u8 platform_open_window(i32 width, i32 height);
void platform_pump_messages();
void platform_shutdown();
void platform_sleep_ms(u64 ms);
f64 platform_get_elapsed_time_ms();
u8 platform_should_run();
void platform_swap_buffers();
keys platform_get_keys();
u8 platform_set_vsync(u8 sync);