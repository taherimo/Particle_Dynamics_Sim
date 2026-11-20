#ifndef POTENTIAL_ENGINE_H
#define POTENTIAL_ENGINE_H

/*
 * Potential expression engine for the Particle Dynamics Simulator.
 *
 * The engine parses a scalar potential U(x,y) once, then evaluates U and its
 * first spatial derivatives using forward-mode automatic differentiation.
 * The force is returned as F = -grad(U).
 *
 * Supported syntax:
 *   Operators: + - * / ^
 *   Unary signs: + -
 *   Parentheses: ( ... )
 *   Coordinates: x, y
 *   Functions: sin, cos, tan, exp, log, sqrt (one argument each)
 *   Constants: pi, e
 *   Parameters: caller-supplied name/value pairs
 *
 * Multiplication must be explicit (2*x, not 2x). Exponentiation is
 * right-associative and binds more tightly than unary minus, so -x^2 means
 * -(x^2). Function names and built-in constants are lowercase and
 * case-sensitive.
 */

#include <stddef.h> /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

#define POTENTIAL_MAX_IDENTIFIER_LENGTH 63
#define POTENTIAL_MAX_EXPRESSION_LENGTH 4096

/** Opaque, parsed potential expression. */
typedef struct PotentialEngine PotentialEngine;

/** A named, constant parameter used by the potential expression. */
typedef struct PotentialParameter {
    const char *name;
    double value;
} PotentialParameter;

/** Values calculated at a particular particle position. */
typedef struct PotentialResult {
    double energy;       /* U(x,y) */
    double dU_dx;        /* Partial derivative of U with respect to x */
    double dU_dy;        /* Partial derivative of U with respect to y */
    double force_x;      /* -dU_dx */
    double force_y;      /* -dU_dy */
} PotentialResult;

/** Status codes returned by the potential engine. */
typedef enum PotentialStatus {
    POTENTIAL_STATUS_OK = 0,
    POTENTIAL_STATUS_INVALID_ARGUMENT,
    POTENTIAL_STATUS_SYNTAX_ERROR,
    POTENTIAL_STATUS_UNKNOWN_IDENTIFIER,
    POTENTIAL_STATUS_INVALID_PARAMETER,
    POTENTIAL_STATUS_OUT_OF_MEMORY,
    POTENTIAL_STATUS_DOMAIN_ERROR,
    POTENTIAL_STATUS_NONFINITE_RESULT,
    POTENTIAL_STATUS_LIMIT_EXCEEDED
} PotentialStatus;

/**
 * Parse and prepare a potential expression.
 *
 * Parameters are substituted as constants during parsing. The names x, y,
 * pi, e, and the built-in function names are reserved. Parameter names are
 * case-sensitive identifiers (ASCII letters/digits/underscore, with a
 * letter or underscore first). Duplicate names and non-finite values are
 * rejected.
 *
 * On success, *out_engine receives a newly allocated engine. On failure,
 * *out_engine is set to NULL and a diagnostic is written to error_message
 * when a buffer is supplied.
 *
 * parameters may be NULL only when parameter_count is zero. The caller may
 * release or modify its parameter array after this function returns.
 */
PotentialStatus potential_engine_create(
    const char *expression,
    const PotentialParameter *parameters,
    size_t parameter_count,
    PotentialEngine **out_engine,
    char *error_message,
    size_t error_message_size);

/**
 * Evaluate the potential and its gradient at (x,y).
 *
 * The engine is immutable after creation and can be evaluated repeatedly.
 * On success, result contains U, dU/dx, dU/dy, Fx, and Fy. On failure,
 * result is not modified and a diagnostic is written when a buffer is given.
 */
PotentialStatus potential_engine_evaluate(
    const PotentialEngine *engine,
    double x,
    double y,
    PotentialResult *result,
    char *error_message,
    size_t error_message_size);

/** Release an engine created by potential_engine_create. */
void potential_engine_destroy(PotentialEngine *engine);

/** Return a short, human-readable description of a status code. */
const char *potential_engine_status_string(PotentialStatus status);

#ifdef __cplusplus
}
#endif

#endif /* POTENTIAL_ENGINE_H */
