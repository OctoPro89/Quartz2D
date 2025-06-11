#include <platform/platform.h>
#include <renderer/renderer2D.h>
#include <asset_loader/asset_loader.h>
#include <ECS/ecs.h>
#include <core/timer.h>
#include <scripts.h>

#include <stdio.h>
#include <crtdbg.h>

int main(int argc, char* argv[]) {
	// Setup window, per-platform
	if (!platform_open_window(WIDTH, HEIGHT)) {
		printf("Failed to open window!\n");
		return -1;
	}

	// Turn vsync on
	if (!platform_set_vsync(true)) {
		printf("Failed to set vsync!\n");
		return -1;
	}

	// Initialize the 2D renderer
	if (!renderer2D_init(WIDTH, HEIGHT)) {
		printf("Failed to initialize renderer2D!\n");
		return -1;
	}

	// -- ECS --

	ecs_init();

	entity_id player_id = entity_create();

	// Player entity
	{
		transform_component t = {
			.x = (f32)WIDTH / 2.0f,
			.y = 40.0f,
			.z = 0.0f,
			.rotation = 0.0f
		};

		entity_add_component(player_id, &t, ENTITY_COMPONENT_TRANSFORM);

		// Set up an animated sprite
		f32 animation_rate = 50.0f / 1000.0f;

		sprite_animation ship_fly = {
			.frames = (animation_frame[]) {
				{ 0, animation_rate }, { 1, animation_rate }, { 2, animation_rate }, { 3, animation_rate } // Looping cycle
			},
			.frame_count = 4,
			.looping = true
		};

		animation_state ship_anim = {
			.animation = &ship_fly,
			.current_frame = 0,
			.time_accumulator = 0.0f
		};

		animated_sprite player_sprite = {
			.atlas = asset_loader_load_texture_atlas_from_tga("player_ship.tga", 16, 16, 4),
			.anim_state = ship_anim
		};

		sprite_component s = {
			.height = 16 * UPSCALE_MULTIPLIER,
			.width = 16 * UPSCALE_MULTIPLIER,
			.is_animated = true,
			.sprite = player_sprite
		};

		entity_add_component(player_id, &s, ENTITY_COMPONENT_SPRITE);

		REGISTER_SCRIPT(player_id, NULL, my_script_on_update, NULL);
	}

	entity_id enemy_id = entity_create();

	{
		transform_component t = {
			.x = (f32)WIDTH / 2.0f,
			.y = (f32)HEIGHT - 40.0f,
			.z = 0.0f,
			.rotation = 180.0f
		};

		entity_add_component(enemy_id, &t, ENTITY_COMPONENT_TRANSFORM);

		// Set up an animated sprite
		f32 animation_rate = 50.0f / 1000.0f;

		sprite_animation ship_fly = {
			.frames = (animation_frame[]) {
				{ 0, animation_rate }, { 1, animation_rate }, { 2, animation_rate }, { 3, animation_rate } // Looping cycle
			},
			.frame_count = 4,
			.looping = true
		};

		animation_state ship_anim = {
			.animation = &ship_fly,
			.current_frame = 0,
			.time_accumulator = 0.0f
		};

		animated_sprite enemy_sprite = {
			.atlas = asset_loader_load_texture_atlas_from_tga("enemy_ship.tga", 16, 16, 4),
			.anim_state = ship_anim
		};

		sprite_component s = {
			.height = 16 * UPSCALE_MULTIPLIER,
			.width = 16 * UPSCALE_MULTIPLIER,
			.is_animated = true,
			.sprite = enemy_sprite
		};

		entity_add_component(enemy_id, &s, ENTITY_COMPONENT_SPRITE);
	}
	
	entity_id hearts[5] = { 0 };

	{
		sprite_component heart_sprite = {
			.is_animated = false,
			.width = 16 * UPSCALE_MULTIPLIER,
			.height = 16 * UPSCALE_MULTIPLIER,
			.texture_id = asset_loader_load_texture_from_tga("heart.tga")
		};

		for (i32 i = 0; i < 5; ++i) {
			entity_id e = entity_create();
			entity_add_component(e, &heart_sprite, ENTITY_COMPONENT_SPRITE);

			transform_component t = {
				.x = (f32)i * heart_sprite.width + (heart_sprite.width / 2.0f) + 5.0f,
				.y = heart_sprite.width / 2.0f,
				.z = 1.0f,
				.rotation = 0.0f
			};

			entity_add_component(e, &t, ENTITY_COMPONENT_TRANSFORM);

			hearts[i] = e;
		}
	}

	bitmap_font en_font = {
		.texture_id = asset_loader_load_texture_from_tga("font_en.tga"),
		.glyph_width = 8,
		.glyph_height = 8,
		.atlas_columns = 7,
		.atlas_rows = 8,
		.space_width = 1.0f,
		.kerning = 0.8f
	};

	ecs_initialize_scripts();

	// -- ECS --
	timer time = { 0 };
	timer_init(&time);

	while (platform_should_run()) {
		timer_begin(&time);

		// This will be updated at the end of the frame
		platform_pump_messages();
		ecs_update_scripts(time.delta_time);
		ecs_update_sprite_animations(time.delta_time);

		renderer2D_begin_batch();
		renderer2D_draw_bitmap_text(50.0f, 80.0f, 24.0f, "Player one Start", &en_font, (color4) { 1, 1, 1, 1 }, 0.0f);
		ecs_draw_sprites();
		ecs_for_each(ENTITY_COMPONENT_CUSTOM, draw_blasts);
		renderer2D_end_batch();
		renderer2D_flush();

		timer_end(&time);
	}

	asset_loader_destroy_texture(en_font.texture_id);

	for (i32 i = 0; i < 5; ++i) {
		asset_loader_destroy_texture(entity_get_sprite(hearts[i])->texture_id);
	}

	asset_loader_destroy_texture_atlas(&entity_get_sprite(enemy_id)->sprite.atlas);
	asset_loader_destroy_texture_atlas(&entity_get_sprite(player_id)->sprite.atlas);

	entity_destroy(enemy_id);
	entity_destroy(player_id);

	ecs_shutdown_scripts();
	ecs_shutdown();

	renderer2D_shutdown();

	platform_shutdown();

	_CrtDumpMemoryLeaks();

	return 0;
}