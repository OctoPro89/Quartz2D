#include <asset_loader/tga_loader.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t  id_length;
    uint8_t  color_map_type;
    uint8_t  image_type;
    uint16_t color_map_origin;
    uint16_t color_map_length;
    uint8_t  color_map_depth;
    uint16_t x_origin;
    uint16_t y_origin;
    uint16_t width;
    uint16_t height;
    uint8_t  pixel_depth;
    uint8_t  image_descriptor;
} tga_header_t;
#pragma pack(pop)

tga_image tga_import(const char* filename) {
    tga_image image;
    memset(&image, 0, sizeof(tga_image));
    image.data = NULL;

    if (!filename) return image;

    FILE* f = fopen(filename, "rb");
    if (!f) return image;

    tga_header_t header;
    if (fread(&header, sizeof(header), 1, f) != 1) {
        fclose(f);
        return image;
    }

    // Only support uncompressed true-color TGA (type 2)
    if (header.image_type != 2 || (header.pixel_depth != 24 && header.pixel_depth != 32)) {
        fclose(f);
        return image;
    }

    // Skip ID field if present
    if (header.id_length > 0) {
        fseek(f, header.id_length, SEEK_CUR);
    }

    image.width = header.width;
    image.height = header.height;
    image.bpp = header.pixel_depth;

    int bytes_per_pixel = header.pixel_depth / 8;
    size_t img_size = image.width * image.height * 4;
    image.data = (u8*)malloc(img_size);
    if (!image.data) {
        fclose(f);
        return image;
    }

    for (u32 y = 0; y < image.height; y++) {
        for (u32 x = 0; x < image.width; x++) {
            u8 bgra[4] = { 0, 0, 0, 255 }; // default alpha = 255

            if (fread(bgra, 1, bytes_per_pixel, f) != bytes_per_pixel) {
                free(image.data);
                fclose(f);
                return image;
            }

            // Flip Y-axis if needed (bottom-left origin)
            u32 row = (header.image_descriptor & 0x20) ? (image.height - 1 - y) : y;
            u32 idx = (row * image.width + x) * 4;

            image.data[idx] = bgra[2]; // R
            image.data[idx + 1] = bgra[1]; // G
            image.data[idx + 2] = bgra[0]; // B
            image.data[idx + 3] = (bytes_per_pixel == 4) ? bgra[3] : 255; // A
        }
    }

    fclose(f);
    return image;
}

void tga_free(tga_image* image) {
	if (image && image->data) {
		free(image->data);
	}
}