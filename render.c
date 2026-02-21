#include "render.h"
#include "image.h"
#include "genetic.h"
#include <stdlib.h>
#include <stdint.h>
#include <omp.h>

/* ========= HELPERS ========= */
static void sort_vertices(int* x, int* y) {
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2 - i; j++) {
            if (y[j] > y[j + 1]) {
                int tempY = y[j]; y[j] = y[j + 1]; y[j + 1] = tempY;
                int tempX = x[j]; x[j] = x[j + 1]; x[j + 1] = tempX;
            }
        }
    }
}

static inline void blend_pixel(unsigned char* r, unsigned char* g, unsigned char* b, const Triangle* t) {
    unsigned char a = t->a;
    unsigned char inv_a = 255 - a;

    *r = (t->r * a + (*r) * inv_a) >> 8;
    *g = (t->g * a + (*g) * inv_a) >> 8;
    *b = (t->b * a + (*b) * inv_a) >> 8;
}

/* ========= FILL TRIANGLE (SCANLINE + OPENMP) ========= */
void draw_triangle(unsigned char* pixels,
                                 int width, int height,
                                 const Triangle* t)
{
    int x[3] = { t->x1, t->x2, t->x3 };
    int y[3] = { t->y1, t->y2, t->y3 };
    sort_vertices(x, y);

    int total_height = y[2] - y[0];
    if (total_height == 0) return;

    // Compute slopes once
    float dx01 = (y[1] - y[0]) ? (float)(x[1] - x[0]) / (y[1] - y[0]) : 0.0f;
    float dx02 = (y[2] - y[0]) ? (float)(x[2] - x[0]) / (y[2] - y[0]) : 0.0f;
    float dx12 = (y[2] - y[1]) ? (float)(x[2] - x[1]) / (y[2] - y[1]) : 0.0f;

    // Upper part
    #pragma omp parallel for schedule(static)
    for (int curr_y = y[0]; curr_y <= y[1]; curr_y++) {
        if (curr_y < 0 || curr_y >= height) continue;

        float xs = x[0] + dx02 * (curr_y - y[0]);
        float xe = x[0] + dx01 * (curr_y - y[0]);
        if (xs > xe) { float tmp = xs; xs = xe; xe = tmp; }

        int x_start = (int)(xs + 0.5f);
        int x_end   = (int)(xe + 0.5f);

        for (int curr_x = x_start; curr_x <= x_end; curr_x++) {
            if (curr_x < 0 || curr_x >= width) continue;
            int idx = (curr_y * width + curr_x) * 3;
            blend_pixel(&pixels[idx], &pixels[idx+1], &pixels[idx+2], t);
        }
    }

    // Lower part
    #pragma omp parallel for schedule(static)
    for (int curr_y = y[1]+1; curr_y <= y[2]; curr_y++) {
        if (curr_y < 0 || curr_y >= height) continue;

        float xs = x[0] + dx02 * (curr_y - y[0]);
        float xe = x[1] + dx12 * (curr_y - y[1]);
        if (xs > xe) { float tmp = xs; xs = xe; xe = tmp; }

        int x_start = (int)(xs + 0.5f);
        int x_end   = (int)(xe + 0.5f);

        for (int curr_x = x_start; curr_x <= x_end; curr_x++) {
            if (curr_x < 0 || curr_x >= width) continue;
            int idx = (curr_y * width + curr_x) * 3;
            blend_pixel(&pixels[idx], &pixels[idx+1], &pixels[idx+2], t);
        }
    }
}

/* ========= RENDER INDIVIDUAL ========= */
unsigned char* render_individual(const Individual* ind, int width, int height)
{
    unsigned char* buffer = (unsigned char*)calloc(width*height*3, sizeof(unsigned char));
    if (!buffer) return NULL;

    for (int i = 0; i < ind->num_triangles; i++) {
        draw_triangle(buffer, width, height, &ind->genes[i]);
    }

    return buffer;
}

/* ========= FITNESS ========= */
double calculate_fitness(const Individual* individual, const Image* target)
{
    int amnt = target->width * target->height * 3;
    unsigned char* buffer = render_individual(individual, target->width, target->height);
    if (!buffer) return 1e30;

    uint64_t error = 0;
    #pragma omp parallel for reduction(+:error) schedule(static)
    for (int i=0; i<amnt; i++) {
        int diff = (int)buffer[i] - (int)target->pixels[i];
        error += (uint64_t)(diff * diff);
    }

    free(buffer);
    double max_error = amnt * 255.0 * 255.0;
    return (double)error / max_error;
}
