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

typedef struct ObjExpressionStatement ObjExpressionStatement;

ObjExpressionStatement* parse();

bool check(TokenType type);
bool match(TokenType type);
void advance();
void consume(TokenType type, const char* message);

ObjExpressionStatement* declaration();
ObjExpressionStatement* statement();

void synchronize();
void errorAt(Token* token, const char* message);
void error(const char* message);
void errorAtCurrent(const char* message);


#endif