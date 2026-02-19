#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"


void load_image(const char* filename, Image* image, int target_width) {
    int orig_w, orig_h, orig_c;

    // Load original image 
    unsigned char* orig_pixels = stbi_load(filename, &orig_w, &orig_h, &orig_c, 3);
    if (!orig_pixels) {
        fprintf(stderr, "Error: Failed to load %s\n", filename);
        exit(1);
    }

    // Calculate aspect ratio
    int target_height = (int)(((float)orig_h / orig_w) * target_width);

    // Allocate memory for resized buffer 
    unsigned char* resized_pixels = (unsigned char*)malloc(target_width * target_height * 3);
    if (!resized_pixels) {
        stbi_image_free(orig_pixels);
        exit(1);
    }

    stbir_resize_uint8_srgb(orig_pixels, orig_w, orig_h, 0,
                           resized_pixels, target_width, target_height, 0,
                           STBIR_RGB);

    // Clean up 
    stbi_image_free(orig_pixels);

    image->pixels = resized_pixels;
    image->width = target_width;
    image->height = target_height;
    image->channels = 3;
}
void free_image(Image* image) {
    stbi_image_free(image->pixels);
}
