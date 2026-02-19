#include <stdio.h>
#include <math.h>
#include <time.h>
#include "genetic.h"
#include "render.h"

#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}


void mutate(Individual* ind, int width, int height, int stuck_level, int triangle_amnt) {
    // Choose a random number of triangles to mutate 
    int max_mutate = triangle_amnt / 10;
    int num_to_mutate = (xorshift32(&ind->rng_state) % max_mutate) + 1;
    num_to_mutate += (stuck_level > 500) ? max_mutate : 0;

    for (int i = 0; i < num_to_mutate; i++) {
        int t_idx = xorshift32(&ind->rng_state) % ind->num_triangles;
        Triangle* t = &ind->genes[t_idx];
        uint32_t type = xorshift32(&ind->rng_state) % 100;

        if (type < 30) {
            // 1. POSITION JIGGLE (Small adjustment)
            int range = 16; 
            t->x1 += (int)(xorshift32(&ind->rng_state) % range) - (range/2);
            t->y1 += (int)(xorshift32(&ind->rng_state) % range) - (range/2);
            t->x2 += (int)(xorshift32(&ind->rng_state) % range) - (range/2);
            t->y2 += (int)(xorshift32(&ind->rng_state) % range) - (range/2);
            t->x3 += (int)(xorshift32(&ind->rng_state) % range) - (range/2);
            t->y3 += (int)(xorshift32(&ind->rng_state) % range) - (range/2);
        } 
        else if (type < 60) {
            // 2. COLOR JIGGLE (Subtle shift)
            t->r = CLAMP(t->r + (int)(xorshift32(&ind->rng_state) % 21) - 10, 0, 255);
            t->g = CLAMP(t->g + (int)(xorshift32(&ind->rng_state) % 21) - 10, 0, 255);
            t->b = CLAMP(t->b + (int)(xorshift32(&ind->rng_state) % 21) - 10, 0, 255);
            t->a = CLAMP(t->a + (int)(xorshift32(&ind->rng_state) % 21) - 10, 10, 200);
        }
        else if (type < 85) {
            // 3. Complete triangle reset
            // This is vital to escape from being stuck sometimes
            t->x1 = xorshift32(&ind->rng_state) % width;
            t->y1 = xorshift32(&ind->rng_state) % height;
            t->x2 = t->x1 + (int)(xorshift32(&ind->rng_state) % 60) - 30;
            t->y2 = t->y1 + (int)(xorshift32(&ind->rng_state) % 60) - 30;
            t->x3 = t->x1 + (int)(xorshift32(&ind->rng_state) % 60) - 30;
            t->y3 = t->y1 + (int)(xorshift32(&ind->rng_state) % 60) - 30;
            t->r = xorshift32(&ind->rng_state) % 256;
            t->g = xorshift32(&ind->rng_state) % 256;
            t->b = xorshift32(&ind->rng_state) % 256;
            t->a = (xorshift32(&ind->rng_state) % 100) + 20;
        }
        else {
            // 4. Swaping order 
            int other = xorshift32(&ind->rng_state) % ind->num_triangles;
            Triangle temp = ind->genes[t_idx];
            ind->genes[t_idx] = ind->genes[other];
            ind->genes[other] = temp;
        }
    }
}

void create_random_individual(int num_triangles, Individual* ind, int w, int h) {
    ind->num_triangles = num_triangles;
    ind->genes = malloc(sizeof(Triangle) * num_triangles);
    
    for (int i = 0; i < num_triangles; i++) {
        Triangle* t = &ind->genes[i];
        
        // Position: Uniform distribution across the whole canvas 
        t->x1 = xorshift32(&ind->rng_state) % w;
        t->y1 = xorshift32(&ind->rng_state) % h;

        // Size: Mix of big and small triangles 
        int size_range = (xorshift32(&ind->rng_state) % 100 < 10) ? w/2 : 30; 
        
        t->x2 = t->x1 + (int)(xorshift32(&ind->rng_state) % size_range) - (size_range/2);
        t->y2 = t->y1 + (int)(xorshift32(&ind->rng_state) % size_range) - (size_range/2);
        t->x3 = t->x1 + (int)(xorshift32(&ind->rng_state) % size_range) - (size_range/2);
        t->y3 = t->y1 + (int)(xorshift32(&ind->rng_state) % size_range) - (size_range/2);

        // Color: Completely random 
        t->r = xorshift32(&ind->rng_state) % 256;
        t->g = xorshift32(&ind->rng_state) % 256;
        t->b = xorshift32(&ind->rng_state) % 256;
        
        // Alpha: Start subtle (Lower alpha usually evolves faster becuase it can mixed easily with other triangles) 
        t->a = (xorshift32(&ind->rng_state) % 100) + 30; 
    }
}

void crossover(const Individual* parentA, const Individual* parentB, Individual* child) {
    for (int i = 0; i < parentA->num_triangles; i++) {
        uint32_t r = xorshift32(&child->rng_state);
        int method = r % 100;

        if (method < 45) {
            // 45% chance: Take from Parent A
            child->genes[i] = parentA->genes[i];
        } else if (method < 90) {
            // 45% chance: Take from Parent B
            child->genes[i] = parentB->genes[i];
        } else {
            // 10% chance: BLEND (Averaging properties)
            child->genes[i].r = (parentA->genes[i].r + parentB->genes[i].r) / 2;
            child->genes[i].g = (parentA->genes[i].g + parentB->genes[i].g) / 2;
            child->genes[i].b = (parentA->genes[i].b + parentB->genes[i].b) / 2;
            child->genes[i].a = (parentA->genes[i].a + parentB->genes[i].a) / 2;
            child->genes[i].x1 = (parentA->genes[i].x1 + parentB->genes[i].x1) / 2;
            child->genes[i].y1 = (parentA->genes[i].y1 + parentB->genes[i].y1) / 2;
            child->genes[i].x2 = (parentA->genes[i].x2 + parentB->genes[i].x2) / 2;
            child->genes[i].y2 = (parentA->genes[i].y2 + parentB->genes[i].y2) / 2;
            child->genes[i].x3 = (parentA->genes[i].x3 + parentB->genes[i].x3) / 2;
            child->genes[i].y3 = (parentA->genes[i].y3 + parentB->genes[i].y3) / 2;
        }
    }
}

void create_empty_individual(int traingle_amnt, Individual* dst) {
    dst->num_triangles = traingle_amnt;
    dst->genes = malloc(sizeof(Triangle) * traingle_amnt);
    dst->rng_state = (uint32_t) time(NULL);
    dst->fitness = 1.0;
}

void free_individual(Individual* individual) { 
    free(individual->genes);
}
