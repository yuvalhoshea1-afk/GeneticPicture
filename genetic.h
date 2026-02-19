#ifndef GENTIC_H
#define GENTIC_H
#include <stdlib.h>
#include "image.h"

typedef struct {
    unsigned char r, g, b, a;
    int x1, y1, x2, y2, x3, y3; 
} Triangle;

typedef struct {
    Triangle *genes; 
    int num_triangles;
    double fitness; 
    uint32_t rng_state;
} Individual;

uint32_t xorshift32(uint32_t *state);
void mutate(Individual* ind, int width, int height, int stuck_level, int max_mutations);
void create_random_individual(int num_triangles, Individual* ind, int w, int h);
void free_individual(Individual* individual);
void crossover(const Individual* parentA, const Individual* parentB, Individual* child);
void create_empty_individual(int traingle_amnt, Individual* dst);
#endif


