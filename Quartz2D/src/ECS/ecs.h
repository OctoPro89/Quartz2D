#pragma once

#include <common.h>

typedef u64 entity_id;

typedef enum {
	ENTITY_FLAGS_SPRITE = (1 << 0),
	ENTITY_FLAGS_BACKGROUND = (1 << 1),
	ENTTIY_FLAGS_USE_SCRIPT = (1 << 2)
} entity_flags;

typedef enum {
	ENTITY_COMPONENT_TRANSFORM = (1 << 0),
	ENTITY_COMPONENT_RIGIDBODY = (1 << 1),
	ENTITY_COMPONENT_SPRITE = (1 << 2),
	ENTITY_COMPONENT_SCRIPT = (1 << 3)
} entity_components;

void ecs_init();
void ecs_shutdown();