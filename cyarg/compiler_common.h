#ifndef cyarg_compiler_common_h
#define cyarg_compiler_common_h

typedef enum {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT
} FunctionType;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + - | ^ & %
    PREC_FACTOR,     // * / << >>
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_DEREF,      // []
    PREC_PRIMARY
} Precedence;

#endif