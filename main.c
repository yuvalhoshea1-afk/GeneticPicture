#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <SDL.h>
#include "genetic.h"
#include "image.h"
#include "render.h"


int cmp_individual_fitness(const void* a, const void* b) {
    Individual* ind1 = (Individual*) a;
    Individual* ind2 = (Individual*) b;

    if (ind1->fitness > ind2->fitness) return 1;
    if (ind1->fitness < ind2->fitness) return -1;
    return 0;
}
void sort_population(Individual* population, int pop_size) {
    qsort(population, pop_size, sizeof(Individual), cmp_individual_fitness);
}


void create_child(const Individual* population, int pop_size, int index, int current_generation,
                  int generations_stuck, int triangle_amount, Individual* child, const Image* target) {
    // Use local RNG to avoid thread collisions
    uint32_t thread_rng = population[index].rng_state ^ (uint32_t) current_generation;
    // Pick two randoms, winner stays
    int r1 = xorshift32(&thread_rng) % pop_size;
    int r2 = xorshift32(&thread_rng) % pop_size;
    int p1 = (population[r1].fitness < population[r2].fitness) ? r1 : r2;
    int r3 = xorshift32(&thread_rng) % pop_size;
    int r4 = xorshift32(&thread_rng) % pop_size;
    int p2 = (population[r3].fitness < population[r4].fitness) ? r3 : r4;

    create_empty_individual(triangle_amount, child);
    child->rng_state = thread_rng;

    // Generate child genes from parents
    crossover(&population[p1], &population[p2], child);
    // Mutate: Passes 'generations_stuck' to allow radical changes if progress stops
    mutate(child, target->width, target->height, generations_stuck, triangle_amount);
}

void save_to_disk(const Individual* to_save, const Image* target, const char* output_file){
    // Render the winner to a final buffer
    unsigned char* final_pixels = render_individual(to_save, target->width, target->height);

    SDL_Surface* save_surface = SDL_CreateRGBSurfaceFrom(
        final_pixels,
        target->width,
        target->height,
        24,                 // 24 bits (RGB)
        target->width * 3,   // Pitch (bytes per row)
        0x0000FF,           // Red mask (for RGB24)
        0x00FF00,           // Green mask
        0xFF0000,           // Blue mask
        0                   // Alpha mask (0 because we're saving 24-bit RGB)
    );

    if (save_surface) {
        // Save to disk
        if (SDL_SaveBMP(save_surface, output_file) == 0) {
            printf("Successfully saved to '%s'\n", output_file);
        } else {
            printf("SDL_SaveBMP failed: %s\n", SDL_GetError());
        }
        SDL_FreeSurface(save_surface);
    }
    free(final_pixels);
}


void print_usage(const char* prog) {
    printf("Usage: %s [options]\n\n", prog);
    printf("Options:\n");
    printf("  -i, --input       <file>    Input image file          (default: target.jpg)\n");
    printf("  -o, --output      <file>    Output BMP file           (default: output.bmp)\n");
    printf("  -t, --triangles   <int>     Number of triangles       (default: 100)\n");
    printf("  -p, --population  <int>     Population size           (default: 10)\n");
    printf("  -g, --generations <int>     Max generations           (default: 40000)\n");
    printf("  -w, --width       <int>     Scale input to this width (default: 200)\n");
    printf("  -e, --epsilon     <float>   Convergence threshold     (default: 0.002)\n");
    printf("  -s, --stuck       <int>     Stuck threshold           (default: 800)\n");
    printf("      --no-vis               Disable visualisation\n");
    printf("  -h, --help                 Show this help message\n");
}

void parse_arguments(int argc, char* argv[],
                     char** file_location,
                     char** output_file,
                     int* triangle_amount,
                     double* epsilon,
                     int* max_generations,
                     bool* visualise,
                     int* scale_width,
                     int* pop_size,
                     int* stuck_threshold) {

    // Default values
    *file_location   = "target.jpg";
    *output_file     = "output.bmp";
    *triangle_amount = 100;
    *epsilon         = 0.002;
    *max_generations = 40000;
    *visualise       = true;
    *scale_width     = 200;
    *pop_size        = 10;
    *stuck_threshold = 800;

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) && i+1 < argc) {
            *file_location = argv[++i];
        } else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) && i+1 < argc) {
            *output_file = argv[++i];
        } else if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--triangles") == 0) && i+1 < argc) {
            *triangle_amount = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--population") == 0) && i+1 < argc) {
            *pop_size = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--generations") == 0) && i+1 < argc) {
            *max_generations = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0) && i+1 < argc) {
            *scale_width = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--epsilon") == 0) && i+1 < argc) {
            *epsilon = atof(argv[++i]);
        } else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--stuck") == 0) && i+1 < argc) {
            *stuck_threshold = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--no-vis") == 0) {
            *visualise = false;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            exit(1);
        }
    }
}

void update_display(SDL_Texture* texture, SDL_Renderer* renderer, const Image* target, const unsigned char* pixels) {
    // Update the GPU texture with our CPU-rendered pixel buffer
    SDL_UpdateTexture(texture, NULL, pixels, target->width * 3);
    SDL_RenderClear(renderer);
    
    // Draw at original size (SDL will scale it if the window is resized)
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    // Make sure the window doesn't freeze or "Not Respond"
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            SDL_Quit();
            exit(0);
        }
    }
}

int main(int argc, char* argv[]) {
    char* file_location;
    char* output_file;
    int triangle_amount, max_generations, scale_width, pop_size, stuck_threshold;
    double epsilon;
    bool visualise;
    parse_arguments(argc, argv, &file_location, &output_file, &triangle_amount, &epsilon,
                    &max_generations, &visualise, &scale_width, &pop_size, &stuck_threshold);
    printf("--- Configuration ---\n");
    printf("File: %s | Output: %s | Triangles: %d | Epsilon: %.5f\n", file_location, output_file, triangle_amount, epsilon);
    printf("Visualise: %s | Population: %d | Stuck threshold: %d\n", visualise ? "ON" : "OFF", pop_size, stuck_threshold);

    // Loading the target image
    Image target;
    load_image(file_location, &target, scale_width);

    /* --- SDL Setup --- */
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Texture* texture = NULL;
    if (visualise) {

        if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;

        window = SDL_CreateWindow("Genetic Art Evolution", 
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                target.width * 3, target.height * 3, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, 
                                    SDL_TEXTUREACCESS_STREAMING, target.width, target.height);
    }

    /* --- Population Setup --- */
    Individual* population = malloc(sizeof(Individual) * pop_size);
    double last_best_fitness = 1.0;
    int generations_stuck = 0;
    for (int i = 0; i < pop_size; i++) {
        // Unique seed for every individual
        population[i].rng_state = (uint32_t)time(NULL) ^ (uint32_t)(i * 0x9E3779B9);
        create_random_individual(triangle_amount, &population[i], target.width, target.height);
        population[i].fitness = calculate_fitness(&population[i], &target);
    }

    printf("Evolution Active. Press Ctrl+C or close window to stop.\n");
    /* --- Main Evolution Loop --- */
    for (int gen = 0; gen < max_generations; gen++) {
        // Convergence
        if (population[0].fitness < epsilon) {
            printf("Target reached! Fitness: %f\n", population[0].fitness);
            break; 
        }

        if (generations_stuck > 10000) {
            printf("We got stuck after %d generations\n", gen-10000);
            printf("We've achieved fitness: %f\n", population[0].fitness);
            printf("It is probably the optimum for given width AND/OR number of triangles (But I am not promising it :)\n");
            break;
        }

        // 1. Sort the population (Best fitness at index 0)
        sort_population(population, pop_size);

        // 2. Monitor Progress
        if (population[0].fitness < last_best_fitness) {
            last_best_fitness = population[0].fitness;
            generations_stuck = 0;
        } else {
            generations_stuck++;
        }

        // 3. Parallel Evolution of the Children
        // We start from i=1 to preserve the absolute best (Elitism)
        #pragma omp parallel for
        for (int i = 1; i < pop_size; i++) {
            Individual child;
            create_child(population, pop_size, i, gen, generations_stuck, triangle_amount, &child, &target);
            child.fitness = calculate_fitness(&child, &target);

            // Thread-safe update: If the child is better than what's there, swap it.  
            // Because mabey population[i] is used in creation of another child in the parallet evolution.
            // If we are stuck, we swap anyway to force new "ideas" into the pool.
            #pragma omp critical
            {
                if (child.fitness < population[i].fitness || (generations_stuck > stuck_threshold)) {
                    free_individual(&population[i]);
                    population[i] = child;
                } else {
                    free_individual(&child);
                }
            }
        }

        // 4. Update the screen every 100 generations
        if (visualise && gen % 100 == 0) {
            unsigned char* pixels = render_individual(&population[0], target.width, target.height);
            update_display(texture, renderer, &target, pixels);
            free(pixels);
        }
        if (gen % 1000 == 0) {
            printf("Gen %d | Fitness: %.5f | Stuck: %d\n", gen, population[0].fitness, generations_stuck);
        }
    }

    printf("Evolution complete. Saving best result...\n");
    save_to_disk(&population[0], &target, output_file);
    
    /* --- Cleanup --- */
    for (int i = 0; i < pop_size; i++) free_individual(&population[i]);
    free_image(&target);
    free(population);
    if (visualise) {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    return 0;
}
