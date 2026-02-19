#ifndef IMAGE_H
#define IMAGE_H

typedef struct {
    int height;
    int width;
    int channels;
    unsigned char* pixels; 
}  Image;

void load_image(const char* filename, Image* image, int target_width);
void free_image(Image* image);
#endif