#include <scripts.h>
#include <renderer/renderer2D.h>
#include <platform/platform.h>

extern const int WIDTH = 1280;
extern const int HEIGHT = 720;
extern const int UPSCALE_MULTIPLIER = 4;

// Custom component for blasts
typedef struct {
	f32 speed; // Units per second
} blast_component;

void draw_blasts(entity_id id, u32 index) {
	transform_component* t = entity_get_transform(id);
	renderer2D_draw_quad(t->x, t->y, 8, 16, -1, (color4) { 1.0f, 1.0f, 1.0f, 1.0f }, t->z); // Or draw with texture
}

void blast_on_update(entity_id entity, f32 delta_time) {
	transform_component* t = entity_get_transform(entity);
	blast_component* b = entity_get_custom(entity, blast_component);

	t->y += b->speed * delta_time;

	if (t->y > HEIGHT) {
		entity_destroy(entity); // Off-screen, delete
	}
}

void my_script_on_update(entity_id entity, f32 delta_time) {
	transform_component* t = entity_get_transform(entity);

	keys keyboard = platform_get_keys();

	static f32 shoot_cooldown = 0.0f;

	if (keyboard.left) {
		t->x -= 500.0f * delta_time;
	}
	else if (keyboard.right) {
		t->x += 500.0f * delta_time;
	}

	if (keyboard.space && shoot_cooldown <= 0.0f) {
		entity_id blast = entity_create();

		transform_component blast_t = {
			.x = t->x,
			.y = t->y + (16.0f * 2),
			.z = 1.0f,
		};

		blast_component blast_c = {
			.speed = 1000.0f // Move up fast
		};

		entity_add_component(blast, &blast_t, ENTITY_COMPONENT_TRANSFORM);
		entity_add_component(blast, (custom_component*)&blast_c, ENTITY_COMPONENT_CUSTOM);

		REGISTER_SCRIPT(blast, NULL, blast_on_update, NULL);

		shoot_cooldown = 0.1f;
	}

	if (shoot_cooldown > 0.0f) {
		shoot_cooldown -= 1.0f * delta_time;
	}
}