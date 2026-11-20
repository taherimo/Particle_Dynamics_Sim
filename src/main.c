
/*
 * Particle Dynamics Simulator - student starter code
 *
 * Students must complete:
 *   1. read_config(): read the settings in resources/config.txt.
 *   2. verlet_step(): implement one Velocity Verlet integration step.
 *
 * Compile from the project root:
 * gcc -std=c11 -Wall -Wextra -Wpedantic -I./engine \
 *     src/main.c engine/potential_engine.c -o particle_sim -lm
 */

#include "potential_engine.h"

#include <stdio.h>
#include <stdlib.h>

#define CONFIG_FILE "resources/config.txt"
#define MAX_PARAMETERS 16
#define OUTPUT_FILE_LENGTH 256

typedef struct {
    char name[POTENTIAL_MAX_IDENTIFIER_LENGTH + 1];
    double value;
} Parameter;

typedef struct {
    double mass, dt;
    size_t steps;
    double x0, y0, vx0, vy0;
    char output_file[OUTPUT_FILE_LENGTH];
    char expression[POTENTIAL_MAX_EXPRESSION_LENGTH + 1];
    Parameter parameters[MAX_PARAMETERS];
    size_t parameter_count;
} Config;

typedef struct {
    double x, y;
    double vx, vy;
    double ax, ay;
} Particle;

/* TODO 1: Read resources/config.txt and fill the Config structure.
 *
 * The file has four sections: [simulation], [initial_conditions],
 * [parameters], and [potential]. Each setting has the form key = value.
 * Ignore blank lines and lines beginning with '#'.
 * Store the potential expression as a string. Store each parameter name
 * and value in config->parameters. Return 1 on success and 0 on error.
 */
static int read_config(const char *filename, Config *config)
{
    (void)filename;
    (void)config;
    fprintf(stderr, "TODO: implement read_config()\n");
    return 0;
}

/* TODO 2: Implement one Velocity Verlet step.
 *
 * 1. Update x and y using the current velocity and acceleration.
 * 2. Evaluate the potential at the new position using
 *    potential_engine_evaluate().
 * 3. Calculate the new acceleration: a = F / mass.
 * 4. Update vx and vy using the average of the old and new accelerations.
 * 5. Store the new acceleration and return 1. Return 0 if evaluation fails.
 *
 * The PotentialResult must contain the potential and force at the new
 * position when the function returns successfully.
 */
static int verlet_step(Particle *p, const PotentialEngine *potential,
                       double mass, double dt, PotentialResult *result)
{
    (void)p;
    (void)potential;
    (void)mass;
    (void)dt;
    (void)result;
    fprintf(stderr, "TODO: implement verlet_step()\n");
    return 0;
}

static void write_row(FILE *file, size_t step, double time, double mass,
                      const Particle *p, const PotentialResult *potential)
{
    double kinetic = 0.5 * mass * (p->vx * p->vx + p->vy * p->vy);

    fprintf(file, "%zu,%.10g,%.10g,%.10g,%.10g,%.10g,%.10g,%.10g,%.10g\n",
            step, time, p->x, p->y, p->vx, p->vy,
            kinetic, potential->energy, kinetic + potential->energy);
}

int main(void)
{
    Config config = {0};
    Particle particle;
    PotentialParameter parameters[MAX_PARAMETERS];
    PotentialEngine *potential = NULL;
    PotentialResult result;
    PotentialStatus status;
    char error[256] = "";
    FILE *output;
    size_t i, step;

    if (!read_config(CONFIG_FILE, &config))
        return EXIT_FAILURE;

    /* Pass the configured parameters to the potential engine. */
    for (i = 0; i < config.parameter_count; ++i) {
        parameters[i].name = config.parameters[i].name;
        parameters[i].value = config.parameters[i].value;
    }

    status = potential_engine_create(
        config.expression, parameters, config.parameter_count,
        &potential, error, sizeof(error));

    if (status != POTENTIAL_STATUS_OK) {
        fprintf(stderr, "Potential error: %s\n", error);
        return EXIT_FAILURE;
    }

    output = fopen(config.output_file, "w");
    if (output == NULL) {
        perror("Could not open output file");
        potential_engine_destroy(potential);
        return EXIT_FAILURE;
    }

    fprintf(output,
            "step,time,x,y,vx,vy,kinetic_energy,potential_energy,total_energy\n");

    /* Set the initial position and velocity. */
    particle.x = config.x0;
    particle.y = config.y0;
    particle.vx = config.vx0;
    particle.vy = config.vy0;
    particle.ax = 0.0;
    particle.ay = 0.0;

    /* Calculate the initial force and acceleration. */
    status = potential_engine_evaluate(
        potential, particle.x, particle.y, &result, error, sizeof(error));

    if (status != POTENTIAL_STATUS_OK) {
        fprintf(stderr, "Potential evaluation error: %s\n", error);
        fclose(output);
        potential_engine_destroy(potential);
        return EXIT_FAILURE;
    }

    particle.ax = result.force_x / config.mass;
    particle.ay = result.force_y / config.mass;
    write_row(output, 0, 0.0, config.mass, &particle, &result);

    /* Run the simulation. */
    for (step = 1; step <= config.steps; ++step) {
        if (!verlet_step(&particle, potential, config.mass,
                         config.dt, &result)) {
            fclose(output);
            potential_engine_destroy(potential);
            return EXIT_FAILURE;
        }

        write_row(output, step, step * config.dt,
                  config.mass, &particle, &result);
    }

    fclose(output);
    potential_engine_destroy(potential);
    printf("Simulation results saved to %s\n", config.output_file);
    return EXIT_SUCCESS;
}
