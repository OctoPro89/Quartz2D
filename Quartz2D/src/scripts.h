#pragma once

#include <common.h>
#include <ECS/ecs.h>

extern const int WIDTH;
extern const int HEIGHT;
extern const int UPSCALE_MULTIPLIER;

void draw_blasts(entity_id id, u32 index);
void blast_on_update(entity_id entity, f32 delta_time);
void my_script_on_update(entity_id entity, f32 delta_time);