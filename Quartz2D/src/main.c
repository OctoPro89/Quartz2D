#include <platform/platform.h>
#include <renderer/renderer2D.h>
#include <asset_loader/asset_loader.h>

#include <stdio.h>
#include <glad/glad.h>

const int WIDTH = 1280;
const int HEIGHT = 720;

int main(int argc, char* argv[]) {
	if (!platform_open_window(WIDTH, HEIGHT)) {
		printf("Failed to open window!\n");
		return -1;
	}

	if (!platform_set_vsync(true)) {
		printf("Failed to set vsync!\n");
		return -1;
	}

	if (!renderer2D_init(WIDTH, HEIGHT)) {
		printf("Failed to initialize renderer2D!\n");
		return -1;
	}

	i32 texture = asset_loader_load_texture_from_tga("ship1.tga");

	f32 x = (f32)WIDTH / 2.0f;
	f32 y = (f32)HEIGHT / 2.0f;

	while (platform_should_run()) {
		platform_pump_messages();

		keys keyboard = platform_get_keys();

		if (keyboard.left) {
			x -= 1.0f;
		}
		else if (keyboard.right) {
			x += 1.0f;
		}

		if (keyboard.up) {
			y += 1.0f;
		}
		else if (keyboard.down) {
			y -= 1.0f;
		}

		renderer2D_begin_batch();

		color4 white = { 1.0f, 1.0f, 1.0f, 1.0f };
		renderer2D_draw_quad(x, y, 64.0f, 64.0f, texture, white);

		renderer2D_end_batch();
		renderer2D_flush();
	}

	asset_loader_destroy_texture(texture);
	renderer2D_shutdown();
	platform_shutdown();

	return 0;
}