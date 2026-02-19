#ifndef REDNER_H
#define REDNER_H
#include "genetic.h"
unsigned char* render_individual(const Individual* ind, int width, int height);
double calculate_fitness(const Individual* individual, const Image* target);
#endif