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

bool check(TokenType type);
bool match(TokenType type);
void advance();
void consume(TokenType type, const char* message);

ObjStmt* declaration();
ObjStmt* statement();

void synchronize();
void errorAt(Token* token, const char* message);
void error(const char* message);
void errorAtCurrent(const char* message);


#endif