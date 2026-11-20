#include "potential_engine.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define POTENTIAL_MAX_NODES 2048U
#define POTENTIAL_MAX_PARSE_DEPTH 256U

/* ----------------------------- Error helpers ---------------------------- */

static void write_message(char *buffer, size_t buffer_size,
                          const char *format, ...)
{
    va_list args;

    if (buffer == NULL || buffer_size == 0U) {
        return;
    }

    va_start(args, format);
    (void)vsnprintf(buffer, buffer_size, format, args);
    va_end(args);
}

const char *potential_engine_status_string(PotentialStatus status)
{
    switch (status) {
    case POTENTIAL_STATUS_OK:
        return "success";
    case POTENTIAL_STATUS_INVALID_ARGUMENT:
        return "invalid argument";
    case POTENTIAL_STATUS_SYNTAX_ERROR:
        return "syntax error";
    case POTENTIAL_STATUS_UNKNOWN_IDENTIFIER:
        return "unknown identifier";
    case POTENTIAL_STATUS_INVALID_PARAMETER:
        return "invalid parameter";
    case POTENTIAL_STATUS_OUT_OF_MEMORY:
        return "out of memory";
    case POTENTIAL_STATUS_DOMAIN_ERROR:
        return "mathematical domain error";
    case POTENTIAL_STATUS_NONFINITE_RESULT:
        return "non-finite result";
    case POTENTIAL_STATUS_LIMIT_EXCEEDED:
        return "expression limit exceeded";
    default:
        return "unknown potential-engine status";
    }
}

/* ------------------------------ Expression AST -------------------------- */

typedef enum NodeType {
    NODE_NUMBER,
    NODE_X,
    NODE_Y,
    NODE_NEGATE,
    NODE_ADD,
    NODE_SUBTRACT,
    NODE_MULTIPLY,
    NODE_DIVIDE,
    NODE_POWER,
    NODE_FUNCTION
} NodeType;

typedef enum FunctionType {
    FUNCTION_SIN,
    FUNCTION_COS,
    FUNCTION_TAN,
    FUNCTION_EXP,
    FUNCTION_LOG,
    FUNCTION_SQRT
} FunctionType;

typedef struct Node {
    NodeType type;
    double number;
    FunctionType function;
    struct Node *left;
    struct Node *right;
    struct Node *next_allocated;
} Node;

struct PotentialEngine {
    Node *root;
    Node *allocated_nodes;
};

/* ------------------------------ Lexer ------------------------------------ */

static int is_ascii_alpha(unsigned char c)
{
    return (c >= (unsigned char)'A' && c <= (unsigned char)'Z') ||
           (c >= (unsigned char)'a' && c <= (unsigned char)'z');
}

static int is_ascii_digit(unsigned char c)
{
    return c >= (unsigned char)'0' && c <= (unsigned char)'9';
}

typedef enum TokenType {
    TOKEN_END,
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_CARET,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_INVALID
} TokenType;

typedef struct Token {
    TokenType type;
    double number;
    char identifier[POTENTIAL_MAX_IDENTIFIER_LENGTH + 1U];
    size_t position;
} Token;

typedef struct Parser {
    const char *source;
    size_t position;
    Token token;
    PotentialStatus status;
    char *error_message;
    size_t error_message_size;
    Node *allocated_nodes;
    size_t node_count;
    size_t recursion_depth;
} Parser;

static void parser_set_error_at(Parser *parser, PotentialStatus status,
                                size_t position, const char *format, ...)
{
    va_list args;
    char detail[256];

    if (parser->status != POTENTIAL_STATUS_OK) {
        return;
    }

    parser->status = status;
    va_start(args, format);
    (void)vsnprintf(detail, sizeof(detail), format, args);
    va_end(args);

    write_message(parser->error_message, parser->error_message_size,
                  "At character %zu: %s", position + 1U, detail);
}

static void parser_set_error(Parser *parser, PotentialStatus status,
                             const char *format, ...)
{
    va_list args;
    char detail[256];

    if (parser->status != POTENTIAL_STATUS_OK) {
        return;
    }

    va_start(args, format);
    (void)vsnprintf(detail, sizeof(detail), format, args);
    va_end(args);
    parser_set_error_at(parser, status, parser->token.position, "%s", detail);
}

static void lexer_next(Parser *parser)
{
    const char *start;
    char *end_pointer;
    size_t length;
    unsigned char character;

    if (parser->status != POTENTIAL_STATUS_OK) {
        parser->token.type = TOKEN_INVALID;
        return;
    }

    while (parser->source[parser->position] != '\0' &&
           isspace((unsigned char)parser->source[parser->position])) {
        parser->position++;
    }

    parser->token.position = parser->position;
    character = (unsigned char)parser->source[parser->position];

    if (character == '\0') {
        parser->token.type = TOKEN_END;
        return;
    }

    switch (character) {
    case '+':
        parser->token.type = TOKEN_PLUS;
        parser->position++;
        return;
    case '-':
        parser->token.type = TOKEN_MINUS;
        parser->position++;
        return;
    case '*':
        parser->token.type = TOKEN_STAR;
        parser->position++;
        return;
    case '/':
        parser->token.type = TOKEN_SLASH;
        parser->position++;
        return;
    case '^':
        parser->token.type = TOKEN_CARET;
        parser->position++;
        return;
    case '(':
        parser->token.type = TOKEN_LEFT_PAREN;
        parser->position++;
        return;
    case ')':
        parser->token.type = TOKEN_RIGHT_PAREN;
        parser->position++;
        return;
    default:
        break;
    }

    if (is_ascii_digit(character) ||
        (character == '.' &&
         is_ascii_digit((unsigned char)parser->source[parser->position + 1U]))) {
        start = parser->source + parser->position;
        errno = 0;
        parser->token.number = strtod(start, &end_pointer);
        if (end_pointer == start) {
            parser_set_error(parser, POTENTIAL_STATUS_SYNTAX_ERROR,
                             "invalid numeric literal");
            parser->token.type = TOKEN_INVALID;
            return;
        }
        if (errno == ERANGE || !isfinite(parser->token.number)) {
            parser_set_error(parser, POTENTIAL_STATUS_NONFINITE_RESULT,
                             "numeric literal is outside the supported range");
            parser->token.type = TOKEN_INVALID;
            return;
        }
        parser->position = (size_t)(end_pointer - parser->source);
        parser->token.type = TOKEN_NUMBER;
        return;
    }

    if (is_ascii_alpha(character) || character == '_') {
        start = parser->source + parser->position;
        parser->position++;
        while (is_ascii_alpha((unsigned char)parser->source[parser->position]) ||
               is_ascii_digit((unsigned char)parser->source[parser->position]) ||
               parser->source[parser->position] == '_') {
            parser->position++;
        }
        length = parser->position - (size_t)(start - parser->source);
        if (length > POTENTIAL_MAX_IDENTIFIER_LENGTH) {
            parser_set_error(parser, POTENTIAL_STATUS_LIMIT_EXCEEDED,
                             "identifier exceeds %d characters",
                             POTENTIAL_MAX_IDENTIFIER_LENGTH);
            parser->token.type = TOKEN_INVALID;
            return;
        }
        memcpy(parser->token.identifier, start, length);
        parser->token.identifier[length] = '\0';
        parser->token.type = TOKEN_IDENTIFIER;
        return;
    }

    parser_set_error(parser, POTENTIAL_STATUS_SYNTAX_ERROR,
                     "unexpected character '%c'", character);
    parser->token.type = TOKEN_INVALID;
}

/* ------------------------------ Parser ----------------------------------- */

static Node *new_node(Parser *parser, NodeType type, Node *left, Node *right,
                      double number, FunctionType function)
{
    Node *node;

    if (parser->status != POTENTIAL_STATUS_OK) {
        return NULL;
    }
    if (parser->node_count >= POTENTIAL_MAX_NODES) {
        parser_set_error(parser, POTENTIAL_STATUS_LIMIT_EXCEEDED,
                         "expression contains too many operations (maximum %u nodes)",
                         POTENTIAL_MAX_NODES);
        return NULL;
    }

    node = (Node *)malloc(sizeof(*node));
    if (node == NULL) {
        parser_set_error(parser, POTENTIAL_STATUS_OUT_OF_MEMORY,
                         "unable to allocate expression node");
        return NULL;
    }

    node->type = type;
    node->number = number;
    node->function = function;
    node->left = left;
    node->right = right;
    node->next_allocated = parser->allocated_nodes;
    parser->allocated_nodes = node;
    parser->node_count++;
    return node;
}

static int is_reserved_name(const char *name)
{
    static const char *const reserved[] = {
        "x", "y", "pi", "e", "sin", "cos", "tan", "exp", "log", "sqrt"
    };
    size_t i;

    for (i = 0U; i < sizeof(reserved) / sizeof(reserved[0]); i++) {
        if (strcmp(name, reserved[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

static int is_valid_identifier(const char *name)
{
    size_t i;
    unsigned char c;

    if (name == NULL || name[0] == '\0') {
        return 0;
    }
    c = (unsigned char)name[0];
    if (!(is_ascii_alpha(c) || c == '_')) {
        return 0;
    }
    for (i = 1U; name[i] != '\0'; i++) {
        c = (unsigned char)name[i];
        if (!(is_ascii_alpha(c) || is_ascii_digit(c) || c == '_')) {
            return 0;
        }
    }
    return i <= POTENTIAL_MAX_IDENTIFIER_LENGTH;
}

static const PotentialParameter *find_parameter(
    const char *name,
    const PotentialParameter *parameters,
    size_t parameter_count)
{
    size_t i;

    for (i = 0U; i < parameter_count; i++) {
        if (strcmp(name, parameters[i].name) == 0) {
            return &parameters[i];
        }
    }
    return NULL;
}

static int get_function(const char *name, FunctionType *function)
{
    if (strcmp(name, "sin") == 0) {
        *function = FUNCTION_SIN;
    } else if (strcmp(name, "cos") == 0) {
        *function = FUNCTION_COS;
    } else if (strcmp(name, "tan") == 0) {
        *function = FUNCTION_TAN;
    } else if (strcmp(name, "exp") == 0) {
        *function = FUNCTION_EXP;
    } else if (strcmp(name, "log") == 0) {
        *function = FUNCTION_LOG;
    } else if (strcmp(name, "sqrt") == 0) {
        *function = FUNCTION_SQRT;
    } else {
        return 0;
    }
    return 1;
}

static Node *parse_expression(Parser *parser,
                              const PotentialParameter *parameters,
                              size_t parameter_count);

static Node *parse_primary(Parser *parser,
                           const PotentialParameter *parameters,
                           size_t parameter_count)
{
    Node *node;
    FunctionType function;
    const PotentialParameter *parameter;
    char name[POTENTIAL_MAX_IDENTIFIER_LENGTH + 1U];
    size_t identifier_position;

    if (parser->status != POTENTIAL_STATUS_OK) {
        return NULL;
    }

    if (parser->token.type == TOKEN_NUMBER) {
        double value = parser->token.number;
        lexer_next(parser);
        return new_node(parser, NODE_NUMBER, NULL, NULL, value, FUNCTION_SIN);
    }

    if (parser->token.type == TOKEN_IDENTIFIER) {
        (void)snprintf(name, sizeof(name), "%s", parser->token.identifier);
        identifier_position = parser->token.position;
        lexer_next(parser);

        if (parser->token.type == TOKEN_LEFT_PAREN) {
            if (!get_function(name, &function)) {
                parser_set_error_at(parser, POTENTIAL_STATUS_UNKNOWN_IDENTIFIER,
                                    identifier_position,
                                    "'%s' is not a supported function", name);
                return NULL;
            }
            lexer_next(parser);
            node = parse_expression(parser, parameters, parameter_count);
            if (parser->status != POTENTIAL_STATUS_OK) {
                return NULL;
            }
            if (parser->token.type != TOKEN_RIGHT_PAREN) {
                parser_set_error(parser, POTENTIAL_STATUS_SYNTAX_ERROR,
                                 "expected ')' after argument to %s()", name);
                return NULL;
            }
            lexer_next(parser);
            return new_node(parser, NODE_FUNCTION, node, NULL, 0.0, function);
        }

        if (strcmp(name, "x") == 0) {
            return new_node(parser, NODE_X, NULL, NULL, 0.0, FUNCTION_SIN);
        }
        if (strcmp(name, "y") == 0) {
            return new_node(parser, NODE_Y, NULL, NULL, 0.0, FUNCTION_SIN);
        }
        if (strcmp(name, "pi") == 0) {
            return new_node(parser, NODE_NUMBER, NULL, NULL, 3.14159265358979323846,
                            FUNCTION_SIN);
        }
        if (strcmp(name, "e") == 0) {
            return new_node(parser, NODE_NUMBER, NULL, NULL, 2.71828182845904523536,
                            FUNCTION_SIN);
        }

        parameter = find_parameter(name, parameters, parameter_count);
        if (parameter != NULL) {
            return new_node(parser, NODE_NUMBER, NULL, NULL, parameter->value,
                            FUNCTION_SIN);
        }

        parser_set_error_at(parser, POTENTIAL_STATUS_UNKNOWN_IDENTIFIER,
                            identifier_position,
                            "unknown name '%s' (declare it in [parameters] if it is a constant)",
                            name);
        return NULL;
    }

    if (parser->token.type == TOKEN_LEFT_PAREN) {
        lexer_next(parser);
        node = parse_expression(parser, parameters, parameter_count);
        if (parser->status != POTENTIAL_STATUS_OK) {
            return NULL;
        }
        if (parser->token.type != TOKEN_RIGHT_PAREN) {
            parser_set_error(parser, POTENTIAL_STATUS_SYNTAX_ERROR,
                             "expected ')' to close parenthesized expression");
            return NULL;
        }
        lexer_next(parser);
        return node;
    }

    if (parser->token.type == TOKEN_END) {
        parser_set_error(parser, POTENTIAL_STATUS_SYNTAX_ERROR,
                         "expected a number, coordinate, function, or '('");
    } else {
        parser_set_error(parser, POTENTIAL_STATUS_SYNTAX_ERROR,
                         "expected a number, coordinate, function, or '('");
    }
    return NULL;
}

/* Forward declaration is needed because power's exponent is a unary expression. */
static Node *parse_unary(Parser *parser,
                         const PotentialParameter *parameters,
                         size_t parameter_count);

static Node *parse_power_impl(Parser *parser,
                              const PotentialParameter *parameters,
                              size_t parameter_count)
{
    Node *left;
    Node *right;

    left = parse_primary(parser, parameters, parameter_count);
    if (parser->status != POTENTIAL_STATUS_OK) {
        return NULL;
    }

    if (parser->token.type == TOKEN_CARET) {
        lexer_next(parser);
        right = parse_unary(parser, parameters, parameter_count);
        if (parser->status != POTENTIAL_STATUS_OK) {
            return NULL;
        }
        return new_node(parser, NODE_POWER, left, right, 0.0, FUNCTION_SIN);
    }
    return left;
}

static Node *parse_unary(Parser *parser,
                         const PotentialParameter *parameters,
                         size_t parameter_count)
{
    TokenType sign;
    Node *operand;

    if (parser->recursion_depth >= POTENTIAL_MAX_PARSE_DEPTH) {
        parser_set_error(parser, POTENTIAL_STATUS_LIMIT_EXCEEDED,
                         "expression nesting exceeds %u levels",
                         POTENTIAL_MAX_PARSE_DEPTH);
        return NULL;
    }

    parser->recursion_depth++;
    sign = parser->token.type;
    if (sign == TOKEN_PLUS || sign == TOKEN_MINUS) {
        lexer_next(parser);
        operand = parse_unary(parser, parameters, parameter_count);
        parser->recursion_depth--;
        if (parser->status != POTENTIAL_STATUS_OK) {
            return NULL;
        }
        if (sign == TOKEN_PLUS) {
            return operand;
        }
        return new_node(parser, NODE_NEGATE, operand, NULL, 0.0, FUNCTION_SIN);
    }

    operand = parse_power_impl(parser, parameters, parameter_count);
    parser->recursion_depth--;
    return operand;
}

static Node *parse_term(Parser *parser,
                        const PotentialParameter *parameters,
                        size_t parameter_count)
{
    Node *left = parse_unary(parser, parameters, parameter_count);

    while (parser->status == POTENTIAL_STATUS_OK &&
           (parser->token.type == TOKEN_STAR || parser->token.type == TOKEN_SLASH)) {
        TokenType operation = parser->token.type;
        Node *right;
        lexer_next(parser);
        right = parse_unary(parser, parameters, parameter_count);
        if (parser->status != POTENTIAL_STATUS_OK) {
            return NULL;
        }
        left = new_node(parser,
                        operation == TOKEN_STAR ? NODE_MULTIPLY : NODE_DIVIDE,
                        left, right, 0.0, FUNCTION_SIN);
    }
    return left;
}

static Node *parse_expression(Parser *parser,
                              const PotentialParameter *parameters,
                              size_t parameter_count)
{
    Node *left = parse_term(parser, parameters, parameter_count);

    while (parser->status == POTENTIAL_STATUS_OK &&
           (parser->token.type == TOKEN_PLUS || parser->token.type == TOKEN_MINUS)) {
        TokenType operation = parser->token.type;
        Node *right;
        lexer_next(parser);
        right = parse_term(parser, parameters, parameter_count);
        if (parser->status != POTENTIAL_STATUS_OK) {
            return NULL;
        }
        left = new_node(parser,
                        operation == TOKEN_PLUS ? NODE_ADD : NODE_SUBTRACT,
                        left, right, 0.0, FUNCTION_SIN);
    }
    return left;
}

/* ------------------------------ Evaluation ------------------------------ */

typedef struct DualValue {
    double value;
    double dx;
    double dy;
} DualValue;

typedef struct Evaluator {
    PotentialStatus status;
    char *error_message;
    size_t error_message_size;
} Evaluator;

static DualValue dual_error(void)
{
    DualValue value = {0.0, 0.0, 0.0};
    return value;
}

static int dual_is_finite(DualValue value)
{
    return isfinite(value.value) && isfinite(value.dx) && isfinite(value.dy);
}

static void evaluator_error(Evaluator *evaluator, PotentialStatus status,
                            const char *message)
{
    if (evaluator->status != POTENTIAL_STATUS_OK) {
        return;
    }
    evaluator->status = status;
    write_message(evaluator->error_message, evaluator->error_message_size,
                  "%s", message);
}

static DualValue evaluate_node(const Node *node, double x, double y,
                               Evaluator *evaluator)
{
    DualValue left;
    DualValue right;
    DualValue result;
    double denominator;
    double cosine;
    double exponent_integer;

    if (evaluator->status != POTENTIAL_STATUS_OK || node == NULL) {
        return dual_error();
    }

    switch (node->type) {
    case NODE_NUMBER:
        result.value = node->number;
        result.dx = 0.0;
        result.dy = 0.0;
        break;

    case NODE_X:
        result.value = x;
        result.dx = 1.0;
        result.dy = 0.0;
        break;

    case NODE_Y:
        result.value = y;
        result.dx = 0.0;
        result.dy = 1.0;
        break;

    case NODE_NEGATE:
        left = evaluate_node(node->left, x, y, evaluator);
        result.value = -left.value;
        result.dx = -left.dx;
        result.dy = -left.dy;
        break;

    case NODE_ADD:
        left = evaluate_node(node->left, x, y, evaluator);
        right = evaluate_node(node->right, x, y, evaluator);
        result.value = left.value + right.value;
        result.dx = left.dx + right.dx;
        result.dy = left.dy + right.dy;
        break;

    case NODE_SUBTRACT:
        left = evaluate_node(node->left, x, y, evaluator);
        right = evaluate_node(node->right, x, y, evaluator);
        result.value = left.value - right.value;
        result.dx = left.dx - right.dx;
        result.dy = left.dy - right.dy;
        break;

    case NODE_MULTIPLY:
        left = evaluate_node(node->left, x, y, evaluator);
        right = evaluate_node(node->right, x, y, evaluator);
        result.value = left.value * right.value;
        result.dx = left.dx * right.value + left.value * right.dx;
        result.dy = left.dy * right.value + left.value * right.dy;
        break;

    case NODE_DIVIDE:
        left = evaluate_node(node->left, x, y, evaluator);
        right = evaluate_node(node->right, x, y, evaluator);
        if (right.value == 0.0) {
            evaluator_error(evaluator, POTENTIAL_STATUS_DOMAIN_ERROR,
                            "division by zero while evaluating the potential");
            return dual_error();
        }
        denominator = right.value * right.value;
        result.value = left.value / right.value;
        result.dx = (left.dx - result.value * right.dx) / right.value;
        result.dy = (left.dy - result.value * right.dy) / right.value;
        break;

    case NODE_POWER:
        left = evaluate_node(node->left, x, y, evaluator);
        right = evaluate_node(node->right, x, y, evaluator);

        if (left.value == 0.0 && right.value == 0.0) {
            evaluator_error(evaluator, POTENTIAL_STATUS_DOMAIN_ERROR,
                            "0^0 is undefined in a potential expression");
            return dual_error();
        }

        if (left.value < 0.0) {
            if (right.dx != 0.0 || right.dy != 0.0 ||
                modf(right.value, &exponent_integer) != 0.0) {
                evaluator_error(evaluator, POTENTIAL_STATUS_DOMAIN_ERROR,
                                "a negative base requires a constant integer exponent");
                return dual_error();
            }
            result.value = pow(left.value, right.value);
            if (right.value == 0.0) {
                result.dx = 0.0;
                result.dy = 0.0;
            } else if (right.value == 1.0) {
                result.dx = left.dx;
                result.dy = left.dy;
            } else {
                denominator = right.value * pow(left.value, right.value - 1.0);
                result.dx = denominator * left.dx;
                result.dy = denominator * left.dy;
            }
        } else if (left.value == 0.0) {
            if (right.dx != 0.0 || right.dy != 0.0 || right.value < 0.0) {
                evaluator_error(evaluator, POTENTIAL_STATUS_DOMAIN_ERROR,
                                "the exponent or derivative is undefined at a zero base");
                return dual_error();
            }
            result.value = 0.0;
            if (right.value == 1.0) {
                result.dx = left.dx;
                result.dy = left.dy;
            } else if (right.value > 1.0) {
                result.dx = 0.0;
                result.dy = 0.0;
            } else {
                evaluator_error(evaluator, POTENTIAL_STATUS_DOMAIN_ERROR,
                                "the derivative of this power is singular at a zero base");
                return dual_error();
            }
        } else {
            result.value = pow(left.value, right.value);
            denominator = log(left.value);
            result.dx = result.value *
                        (right.dx * denominator + right.value * left.dx / left.value);
            result.dy = result.value *
                        (right.dy * denominator + right.value * left.dy / left.value);
        }
        break;

    case NODE_FUNCTION:
        left = evaluate_node(node->left, x, y, evaluator);
        switch (node->function) {
        case FUNCTION_SIN:
            result.value = sin(left.value);
            denominator = cos(left.value);
            result.dx = denominator * left.dx;
            result.dy = denominator * left.dy;
            break;

        case FUNCTION_COS:
            result.value = cos(left.value);
            denominator = -sin(left.value);
            result.dx = denominator * left.dx;
            result.dy = denominator * left.dy;
            break;

        case FUNCTION_TAN:
            cosine = cos(left.value);
            if (cosine == 0.0) {
                evaluator_error(evaluator, POTENTIAL_STATUS_DOMAIN_ERROR,
                                "tan() is undefined at this position");
                return dual_error();
            }
            result.value = tan(left.value);
            denominator = 1.0 / (cosine * cosine);
            result.dx = denominator * left.dx;
            result.dy = denominator * left.dy;
            break;

        case FUNCTION_EXP:
            result.value = exp(left.value);
            result.dx = result.value * left.dx;
            result.dy = result.value * left.dy;
            break;

        case FUNCTION_LOG:
            if (left.value <= 0.0) {
                evaluator_error(evaluator, POTENTIAL_STATUS_DOMAIN_ERROR,
                                "log() requires a strictly positive argument");
                return dual_error();
            }
            result.value = log(left.value);
            result.dx = left.dx / left.value;
            result.dy = left.dy / left.value;
            break;

        case FUNCTION_SQRT:
            if (left.value < 0.0) {
                evaluator_error(evaluator, POTENTIAL_STATUS_DOMAIN_ERROR,
                                "sqrt() requires a non-negative argument");
                return dual_error();
            }
            result.value = sqrt(left.value);
            if (result.value == 0.0) {
                evaluator_error(evaluator, POTENTIAL_STATUS_DOMAIN_ERROR,
                                "the derivative of sqrt() is undefined at zero");
                return dual_error();
            } else {
                result.dx = left.dx / (2.0 * result.value);
                result.dy = left.dy / (2.0 * result.value);
            }
            break;

        default:
            evaluator_error(evaluator, POTENTIAL_STATUS_INVALID_ARGUMENT,
                            "internal error: unsupported function node");
            return dual_error();
        }
        break;

    default:
        evaluator_error(evaluator, POTENTIAL_STATUS_INVALID_ARGUMENT,
                        "internal error: unsupported expression node");
        return dual_error();
    }

    if (!dual_is_finite(result)) {
        evaluator_error(evaluator, POTENTIAL_STATUS_NONFINITE_RESULT,
                        "potential or derivative is not finite (check overflow and the expression domain)");
        return dual_error();
    }
    return result;
}

/* ------------------------------ Public API ------------------------------ */

PotentialStatus potential_engine_create(
    const char *expression,
    const PotentialParameter *parameters,
    size_t parameter_count,
    PotentialEngine **out_engine,
    char *error_message,
    size_t error_message_size)
{
    Parser parser;
    PotentialEngine *engine;
    size_t expression_length;
    size_t i;
    size_t j;

    if (out_engine == NULL) {
        write_message(error_message, error_message_size,
                      "output engine pointer must not be NULL");
        return POTENTIAL_STATUS_INVALID_ARGUMENT;
    }
    *out_engine = NULL;
    if (expression == NULL || (parameters == NULL && parameter_count != 0U)) {
        write_message(error_message, error_message_size,
                      "expression must not be NULL and parameters must be supplied when parameter_count is nonzero");
        return POTENTIAL_STATUS_INVALID_ARGUMENT;
    }

    if (error_message != NULL && error_message_size > 0U) {
        error_message[0] = '\0';
    }

    expression_length = strlen(expression);
    if (expression_length == 0U) {
        write_message(error_message, error_message_size,
                      "potential expression must not be empty");
        return POTENTIAL_STATUS_SYNTAX_ERROR;
    }
    if (expression_length > POTENTIAL_MAX_EXPRESSION_LENGTH) {
        write_message(error_message, error_message_size,
                      "potential expression exceeds %d characters",
                      POTENTIAL_MAX_EXPRESSION_LENGTH);
        return POTENTIAL_STATUS_LIMIT_EXCEEDED;
    }

    for (i = 0U; i < parameter_count; i++) {
        if (!is_valid_identifier(parameters[i].name)) {
            write_message(error_message, error_message_size,
                          "parameter %zu has an invalid name (identifiers must contain at most %d ASCII letters, digits, or underscores and cannot start with a digit)",
                          i + 1U, POTENTIAL_MAX_IDENTIFIER_LENGTH);
            return POTENTIAL_STATUS_INVALID_PARAMETER;
        }
        if (is_reserved_name(parameters[i].name)) {
            write_message(error_message, error_message_size,
                          "parameter name '%s' is reserved", parameters[i].name);
            return POTENTIAL_STATUS_INVALID_PARAMETER;
        }
        if (!isfinite(parameters[i].value)) {
            write_message(error_message, error_message_size,
                          "parameter '%s' must have a finite numeric value",
                          parameters[i].name);
            return POTENTIAL_STATUS_INVALID_PARAMETER;
        }
        for (j = 0U; j < i; j++) {
            if (strcmp(parameters[i].name, parameters[j].name) == 0) {
                write_message(error_message, error_message_size,
                              "parameter '%s' is defined more than once",
                              parameters[i].name);
                return POTENTIAL_STATUS_INVALID_PARAMETER;
            }
        }
    }

    memset(&parser, 0, sizeof(parser));
    parser.source = expression;
    parser.status = POTENTIAL_STATUS_OK;
    parser.error_message = error_message;
    parser.error_message_size = error_message_size;
    lexer_next(&parser);

    parser.allocated_nodes = NULL;
    parser.node_count = 0U;
    parser.recursion_depth = 0U;

    {
        Node *root = parse_expression(&parser, parameters, parameter_count);

        if (parser.status == POTENTIAL_STATUS_OK && parser.token.type != TOKEN_END) {
            parser_set_error(&parser, POTENTIAL_STATUS_SYNTAX_ERROR,
                             "unexpected token after a complete expression");
        }

        if (parser.status != POTENTIAL_STATUS_OK) {
            Node *node = parser.allocated_nodes;
            while (node != NULL) {
                Node *next = node->next_allocated;
                free(node);
                node = next;
            }
            return parser.status;
        }

        engine = (PotentialEngine *)malloc(sizeof(*engine));
        if (engine == NULL) {
            Node *node = parser.allocated_nodes;
            while (node != NULL) {
                Node *next = node->next_allocated;
                free(node);
                node = next;
            }
            write_message(error_message, error_message_size,
                          "unable to allocate potential engine");
            return POTENTIAL_STATUS_OUT_OF_MEMORY;
        }
        engine->root = root;
        engine->allocated_nodes = parser.allocated_nodes;
    }

    *out_engine = engine;
    return POTENTIAL_STATUS_OK;
}

PotentialStatus potential_engine_evaluate(
    const PotentialEngine *engine,
    double x,
    double y,
    PotentialResult *result,
    char *error_message,
    size_t error_message_size)
{
    Evaluator evaluator;
    DualValue value;
    PotentialResult temporary_result;

    if (error_message != NULL && error_message_size > 0U) {
        error_message[0] = '\0';
    }
    if (engine == NULL || result == NULL) {
        write_message(error_message, error_message_size,
                      "engine and result pointers must not be NULL");
        return POTENTIAL_STATUS_INVALID_ARGUMENT;
    }
    if (!isfinite(x) || !isfinite(y)) {
        write_message(error_message, error_message_size,
                      "coordinates x and y must be finite");
        return POTENTIAL_STATUS_INVALID_ARGUMENT;
    }

    evaluator.status = POTENTIAL_STATUS_OK;
    evaluator.error_message = error_message;
    evaluator.error_message_size = error_message_size;

    value = evaluate_node(engine->root, x, y, &evaluator);
    if (evaluator.status != POTENTIAL_STATUS_OK) {
        return evaluator.status;
    }

    temporary_result.energy = value.value;
    temporary_result.dU_dx = value.dx;
    temporary_result.dU_dy = value.dy;
    temporary_result.force_x = -value.dx;
    temporary_result.force_y = -value.dy;

    if (!isfinite(temporary_result.force_x) ||
        !isfinite(temporary_result.force_y)) {
        write_message(error_message, error_message_size,
                      "force is not finite");
        return POTENTIAL_STATUS_NONFINITE_RESULT;
    }

    *result = temporary_result;
    return POTENTIAL_STATUS_OK;
}

void potential_engine_destroy(PotentialEngine *engine)
{
    Node *node;

    if (engine == NULL) {
        return;
    }

    node = engine->allocated_nodes;
    while (node != NULL) {
        Node *next = node->next_allocated;
        free(node);
        node = next;
    }
    free(engine);
}
