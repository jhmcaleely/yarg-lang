#include <stdlib.h>
#include <stdio.h>

#include "parser.h"
#include "ast.h"

Parser parser;

void errorAt(Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

void error(const char* message) {
    errorAt(&parser.previous, message);
}

void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

bool check(TokenType type) {
    return parser.current.type == type;
}

bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_IMPORT:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            
            default:
                ; // Do nothing.
        }

        advance();
    }
}

void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static ObjExpressionStatement* expressionStatement() {
    ObjExpression* expr = NULL; //expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    ObjExpressionStatement* expressionStatement = newExpressionStatement(expr);
    return expressionStatement;
}

ObjExpressionStatement* statement() {
    if (match(TOKEN_PRINT)) {
//        printStatement();
    } else if (match(TOKEN_FOR)) {
//        forStatement();
    } else if (match(TOKEN_IF)) {
//        ifStatement();
    } else if (match(TOKEN_YIELD)) {
//        yieldStatement();
    } else if (match(TOKEN_RETURN)) {
//        returnStatement();
    } else if (match(TOKEN_WHILE)) {
//        whileStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
//        beginScope();
//        block();
//        endScope();
    } else {
        return expressionStatement();
    }
}

ObjExpressionStatement* declaration() {
    ObjExpressionStatement* stmt = NULL;

    if (match(TOKEN_CLASS)) {
//        classDeclaration();
    } else if (match(TOKEN_FUN)) {
//        funDeclaration();
    } else if (match(TOKEN_VAR)) {
//        varDeclaration();
    } else {
        stmt = statement();
    }

    if (parser.panicMode) synchronize();
    return stmt;
}


ObjExpressionStatement* parse() {
    ObjExpressionStatement* statements = NULL;
    ObjExpressionStatement** cursor = &statements;

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    while (!match(TOKEN_EOF)) {
        *cursor = declaration();
        if (*cursor) {
            cursor = &(*cursor)->next;
        }
    }
    return statements;
}