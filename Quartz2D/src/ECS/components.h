#pragma once

#include <common.h>
#include <ECS/entity_id.h>
#include <animation/sprite_animation.h>

typedef struct {
    f32 x, y, z, rotation;
} transform_component;

typedef void (*on_create_fn)(entity_id);
typedef void (*on_update_fn)(entity_id, float);
typedef void (*on_destroy_fn)(entity_id);

typedef struct {
	on_create_fn on_create;
	on_update_fn on_update;
	on_destroy_fn on_destroy;
} script_component;

#define REGISTER_SCRIPT(entity, create, update, destroy)			\
{																	\
	script_component s = {											\
			.on_create = create,									\
			.on_update = update,									\
			.on_destroy = destroy									\
	};																\
	entity_add_component(entity, &s, ENTITY_COMPONENT_SCRIPT);		\
}

typedef union {
	u8 data[32];
} custom_component;

typedef struct {
	u8 is_animated;

	i32 width;
	i32 height;

	union {
		animated_sprite sprite;
		i32 texture_id;
	};
} sprite_component;