#include <ECS/ecs.h>

#include <stdlib.h>

#define MAX_ENTITIES 10000

typedef struct {
	entity_id id;
	entity_flags flags;
	entity_components component;
} entity_record;

typedef struct {
	entity_record* entities;
	u32 entity_count;
} entity_registry;

static entity_registry registry;

void ecs_init() {
	registry.entities = (entity_record*)malloc(sizeof(entity_record) * MAX_ENTITIES);
}

void ecs_shutdown() {
	if (registry.entities) {
		free(registry.entities);
	}
}