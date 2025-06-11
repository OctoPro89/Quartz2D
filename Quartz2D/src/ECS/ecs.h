#pragma once

#include <common.h>
#include <ECS/components.h>
#include <ECS/entity_id.h>

typedef u64 entity_id;

typedef enum {
	ENTITY_COMPONENT_TRANSFORM = (1 << 0),
	ENTITY_COMPONENT_RIGIDBODY = (1 << 1),
	ENTITY_COMPONENT_SPRITE = (1 << 2),
	ENTITY_COMPONENT_SCRIPT = (1 << 3),
	ENTITY_COMPONENT_CUSTOM = (1 << 4)
} entity_components;

typedef void (*ecs_entity_iter_fn)(entity_id id, u32 index);

void ecs_init();
void ecs_shutdown();
void ecs_for_each(entity_components filter, ecs_entity_iter_fn fn);

void ecs_initialize_scripts();
void ecs_update_scripts(f32 delta_time);
void ecs_shutdown_scripts();

// Component functions
void ecs_update_sprite_animations(f32 delta_time);
void ecs_draw_sprites();

entity_id entity_create();
void entity_add_component(entity_id entity, const void* component_data, entity_components component);
void entity_add_custom_component(entity_id entity, const void* custom_component, u64 custom_component_size);
u8 entity_has_component(entity_id entity, entity_components components);
void entity_remove_component(entity_id entity, entity_components component);
void entity_destroy(entity_id entity);
void* _entity_get_component(entity_id entity, entity_components component);

// Helpers for getting components

#define entity_get_transform(entity) ((transform_component*)_entity_get_component(entity, ENTITY_COMPONENT_TRANSFORM))
#define entity_get_script(entity) ((script_component*)_entity_get_component(entity, ENTITY_COMPONENT_SCRIPT))
#define entity_get_custom(entity, cast_type) ((cast_type*)_entity_get_component(entity, ENTITY_COMPONENT_CUSTOM))
#define entity_get_sprite(entity) ((sprite_component*)_entity_get_component(entity, ENTITY_COMPONENT_SPRITE))