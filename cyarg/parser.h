#ifndef cyarg_parser_h
#define cyarg_parser_h

#include "scanner.h"
#include <stdbool.h>

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

extern Parser parser;

typedef struct ObjStmt ObjStmt;

void parse(ObjStmt** ast_root);


#endif