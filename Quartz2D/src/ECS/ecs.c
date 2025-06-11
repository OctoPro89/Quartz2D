#include <ECS/ecs.h>

#include <renderer/renderer2D.h>
#include <octomath/radians.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

// -- INTERNAL STRUCTURES --

typedef struct {
	entity_id id;
	entity_components components;
} entity_record;

typedef struct {
	entity_record* entities;
	u32 entity_count;

    transform_component* transforms;
    script_component* scripts;
    custom_component* custom_components;
    sprite_component* sprite_components;
} entity_registry;

// -- INTERNAL STRUCTURES --

// -- INTERNAL GLOBAL VARIABLES --

#define MAX_ENTITIES 10000
static entity_registry registry;

// -- INTERNAL GLOBAL VARIABLES --

// -- HELPERS --

// Basically just a BIG random number generator for generating super long numbers
static uint64_t splitmix64_state = 0xA5A5A5A5A5A5A5A5;

uint64_t generate_random_u64() {
    uint64_t z = (splitmix64_state += 0x9E3779B97F4A7C15);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static entity_id generate_uuid() {
    entity_id id = generate_random_u64();
    return id != 0 ? id : generate_uuid(); // Avoid 0
}

static void transform_system_update(entity_id id, u32 index) {
    registry.transforms[index].x += 1.0f; // TODO: Delta time
}

// -- HELPERS --

// -- INTERNAL FUNCTIONS --

static u8 internal_add_component(entity_id entity, entity_components components) {
    for (u32 i = 0; i < registry.entity_count; i++) {
        if (registry.entities[i].id == entity) {
            registry.entities[i].components |= components;

            if (components & ENTITY_COMPONENT_TRANSFORM) {
                registry.transforms[i] = (transform_component){ 0 };
            }
            if (components & ENTITY_COMPONENT_SCRIPT) {
                registry.scripts[i] = (script_component){ 0 }; // default null funcs
            }

            return true;
        }
    }
    return false;
}

static void internal_add_transform(entity_id entity, f32 x, f32 y, f32 z, f32 rotation) {
    for (uint32_t i = 0; i < registry.entity_count; i++) {
        if (registry.entities[i].id == entity) {
            internal_add_component(entity, ENTITY_COMPONENT_TRANSFORM);
            registry.transforms[i].x = x;
            registry.transforms[i].y = y;
            registry.transforms[i].z = z;
            registry.transforms[i].rotation = rotation;
            break;
        }
    }
}

static void internal_add_script(entity_id entity, on_create_fn create, on_update_fn update, on_destroy_fn destroy) {
    for (u32 i = 0; i < registry.entity_count; i++) {
        if (registry.entities[i].id == entity) {
            internal_add_component(entity, ENTITY_COMPONENT_SCRIPT);
            registry.scripts[i].on_create = create;
            registry.scripts[i].on_update = update;
            registry.scripts[i].on_destroy = destroy;
            break;
        }
    }
}

static void internal_add_custom(entity_id entity, const u8* data) {
    for (uint32_t i = 0; i < registry.entity_count; i++) {
        if (registry.entities[i].id == entity) {
            internal_add_component(entity, ENTITY_COMPONENT_CUSTOM);
            memcpy(&registry.custom_components[i].data[0], data, sizeof(custom_component));
            break;
        }
    }
}

static void internal_add_sprite(entity_id entity, const sprite_component* sprite) {
    for (uint32_t i = 0; i < registry.entity_count; i++) {
        if (registry.entities[i].id == entity) {
            internal_add_component(entity, ENTITY_COMPONENT_SPRITE);
            memcpy(&registry.sprite_components[i], sprite, sizeof(sprite_component));
            break;
        }
    }
}

// -- INTERNAL FUNCTIONS --

// -- ENTITY COMPONENT SYSTEM FUNCTIONS --

void ecs_init() {
	registry.entities = (entity_record*)malloc(sizeof(entity_record) * MAX_ENTITIES);
    registry.transforms = (transform_component*)malloc(sizeof(transform_component) * MAX_ENTITIES);
    registry.scripts = (script_component*)malloc(sizeof(script_component) * MAX_ENTITIES);
    registry.custom_components = malloc(sizeof(custom_component) * MAX_ENTITIES);
    registry.sprite_components = malloc(sizeof(sprite_component) * MAX_ENTITIES);

    memset(registry.entities, 0, sizeof(entity_record) * MAX_ENTITIES);
    memset(registry.transforms, 0, sizeof(transform_component) * MAX_ENTITIES);
    memset(registry.scripts, 0, sizeof(script_component) * MAX_ENTITIES);
    memset(registry.custom_components, 0, sizeof(custom_component) * MAX_ENTITIES);
    memset(registry.sprite_components, 0, sizeof(sprite_component) * MAX_ENTITIES);

    srand((u32)time(NULL));
}

void ecs_shutdown() {
    if (registry.sprite_components) {
        free(registry.sprite_components);
    }

    if (registry.custom_components) {
        free(registry.custom_components);
    }

    if (registry.scripts) {
        free(registry.scripts);
    }

    if (registry.transforms) {
        free(registry.transforms);
    }

	if (registry.entities) {
		free(registry.entities);
	}
}

void ecs_for_each(entity_components filter, ecs_entity_iter_fn fn) {
    for (u32 i = 0; i < registry.entity_count; i++) {
        if ((registry.entities[i].components & filter) == filter) {
            fn(registry.entities[i].id, i);
        }
    }
}

void ecs_initialize_scripts() {
    for (u32 i = 0; i < registry.entity_count; i++) {
        if (registry.entities[i].components & ENTITY_COMPONENT_SCRIPT) {
            if (registry.scripts[i].on_create) {
                registry.scripts[i].on_create(registry.entities[i].id);
            }
        }
    }
}

void ecs_update_scripts(f32 delta_time) {
    for (u32 i = 0; i < registry.entity_count; i++) {
        if (registry.entities[i].components & ENTITY_COMPONENT_SCRIPT) {
            if (registry.scripts[i].on_update) {
                registry.scripts[i].on_update(registry.entities[i].id, delta_time);
            }
        }
    }
}

void ecs_shutdown_scripts() {
    for (u32 i = 0; i < registry.entity_count; i++) {
        if (registry.entities[i].components & ENTITY_COMPONENT_SCRIPT) {
            if (registry.scripts[i].on_destroy) {
                registry.scripts[i].on_destroy(registry.entities[i].id);
            }
        }
    }
}

void ecs_update_sprite_animations(f32 delta_time) {
    for (u32 i = 0; i < registry.entity_count; i++) {
        if (registry.entities[i].components & ENTITY_COMPONENT_SPRITE) {
            sprite_component* s = &registry.sprite_components[i];

            if (s->is_animated) {
                animation_state_update(&s->sprite.anim_state, delta_time);
            }
        }
    }
}

void ecs_draw_sprites() {
    for (u32 i = 0; i < registry.entity_count; i++) {
        if (registry.entities[i].components & ENTITY_COMPONENT_SPRITE) {
            const transform_component* t = &registry.transforms[i];
            const sprite_component* s = &registry.sprite_components[i];

            if (s->is_animated) {
                renderer2D_draw_animated_sprite(t->x, t->y, s->width == 0 ? (f32)s->sprite.atlas.sprite_width : (f32)s->width,
                                                            s->height == 0 ? (f32)s->sprite.atlas.sprite_height : (f32)s->height,
                                                            &s->sprite, (color4) { 1.0f, 1.0f, 1.0f, 1.0f }, degrees_to_radians(t->rotation), t->z);
            }
            else {
                renderer2D_draw_rotated_quad(t->x, t->y, s->width == 0 ? (f32)s->sprite.atlas.sprite_width : (f32)s->width,
                                                         s->height == 0 ? (f32)s->sprite.atlas.sprite_height : (f32)s->height,
                                                         s->texture_id, (color4) { 1.0f, 1.0f, 1.0f, 1.0f }, degrees_to_radians(t->rotation), t->z);
            }
        }
    }
}

// -- ENTITY COMPONENT SYSTEM FUNCTIONS --

// -- ENTITY FUNCTIONS --

entity_id entity_create() {
    if (registry.entity_count >= MAX_ENTITIES) return 0; // no more room

    entity_id id = generate_uuid();

    entity_record* rec = &registry.entities[registry.entity_count++];
    rec->id = id;
    rec->components = 0;

    return id;
}

void entity_add_component(entity_id entity, const void* component_data, entity_components component) {
    switch (component) {
        case ENTITY_COMPONENT_TRANSFORM: {
            const transform_component* transform = (const transform_component*)component_data;
            internal_add_transform(entity, transform->x, transform->y, transform->z, transform->rotation);
            break;
        }
        case ENTITY_COMPONENT_SCRIPT: {
            const script_component* script = (const script_component*)component_data;
            internal_add_script(entity, script->on_create, script->on_update, script->on_destroy);
            break;
        }
        case ENTITY_COMPONENT_CUSTOM: {
            const custom_component* custom = (const custom_component*)component_data;
            internal_add_custom(entity, &custom->data[0]);
            break;
        }
        case ENTITY_COMPONENT_SPRITE: {
            const sprite_component* sprite = (const sprite_component*)component_data;
            internal_add_sprite(entity, sprite);
            break;
        }
    }
}

u8 entity_has_component(entity_id entity, entity_components components) {
    for (u32  i = 0; i < registry.entity_count; i++) {
        if (registry.entities[i].id == entity) {
            return (registry.entities[i].components & components) != 0;
        }
    }
    return false;
}

void entity_remove_component(entity_id entity, entity_components component) {
    for (u32 i = 0; i < registry.entity_count; i++) {
        if (registry.entities[i].id == entity) {
            registry.entities[i].components &= ~component;

            if (component & ENTITY_COMPONENT_TRANSFORM) {
                registry.transforms[i] = (transform_component){ 0 }; // Reset data
            }
            if (component & ENTITY_COMPONENT_SCRIPT) {
                registry.scripts[i] = (script_component){ 0 };
            }
            if (component & ENTITY_COMPONENT_CUSTOM) {
                registry.custom_components[i] = (custom_component){ 0 };
            }
            if (component & ENTITY_COMPONENT_SPRITE) {
                registry.sprite_components[i] = (sprite_component){ 0 };
            }
            break;
        }
    }
}

void entity_destroy(entity_id entity) {
    for (u32 i = 0; i < registry.entity_count; i++) {
        if (registry.entities[i].id == entity) {
            u32 last = registry.entity_count - 1;

            // Move last entity into current slot
            registry.entities[i] = registry.entities[last];
            registry.transforms[i] = registry.transforms[last];

            if (registry.scripts[i].on_destroy) {
                registry.scripts[i].on_destroy(entity);
            }

            registry.scripts[i] = registry.scripts[last];
            registry.custom_components[i] = registry.custom_components[last];
            registry.sprite_components[i] = registry.sprite_components[last];

            registry.entity_count--;
            return;
        }
    }
}

void* _entity_get_component(entity_id entity, entity_components component) {
    for (u32 i = 0; i < registry.entity_count; i++) {
        if (registry.entities[i].id == entity &&
            (registry.entities[i].components & component)) {
            switch (component) {
                case ENTITY_COMPONENT_TRANSFORM: {
                    return &registry.transforms[i];
                }
                case ENTITY_COMPONENT_SCRIPT: {
                    return &registry.scripts[i];
                }
                case ENTITY_COMPONENT_CUSTOM: {
                    return &registry.custom_components[i];
                }
                case ENTITY_COMPONENT_SPRITE: {
                    return &registry.sprite_components[i];
                }
            }
        }
    }

    return NULL; // Not found or the entity doesn't have the component
}

// -- ENTITY FUNCTIONS --