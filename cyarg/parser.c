#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "parser.h"
#include "compiler_common.h"
#include "ast.h"
#include "memory.h"

typedef Obj* (*AstParseFn)(bool canAssign);

typedef struct {
    AstParseFn prefix;
    AstParseFn infix;
    Precedence precedence;
} AstParseRule;

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

static uint32_t strtoNum(const char* literal, int length, int radix) {
    uint32_t val = 0;
    for (int i = length - 1 ; i >= 0; i--) {
        uint32_t positionVal = 0;
        switch (literal[i]) {
            case '0': positionVal = 0; break;
            case '1': positionVal = 1; break;
            case '2': positionVal = 2; break;
            case '3': positionVal = 3; break;
            case '4': positionVal = 4; break;
            case '5': positionVal = 5; break;
            case '6': positionVal = 6; break;
            case '7': positionVal = 7; break;
            case '8': positionVal = 8; break;
            case '9': positionVal = 9; break;
            case 'A': positionVal = 10; break;
            case 'B': positionVal = 11; break;
            case 'C': positionVal = 12; break;
            case 'D': positionVal = 13; break;
            case 'E': positionVal = 14; break;
            case 'F': positionVal = 15; break;
            case 'a': positionVal = 10; break;
            case 'b': positionVal = 11; break;
            case 'c': positionVal = 12; break;
            case 'd': positionVal = 13; break;
            case 'e': positionVal = 14; break;
            case 'f': positionVal = 15; break;
        }
        uint32_t power = length - 1 - i;
        val += positionVal * pow(radix, power);
    }
    return val;
}

static Obj* grouping(bool canAssign) { return NULL; }
static Obj* call(bool canAssign) { return NULL;}
static Obj* arrayinit(bool canAssign) { return NULL; }
static Obj* deref(bool canAssign) {return NULL; }
static Obj* dot(bool canAssign) { return NULL; }
static Obj* unary(bool canAssign) { return NULL; }
static Obj* binary(bool canAssign) { return NULL; }
static Obj* variable(bool canAssign) {return NULL; }
static Obj* string(bool canAssign) { return NULL; }
static Obj* and_(bool canAssign) { return NULL; }
static Obj* or_(bool canAssign) { return NULL; }
static Obj* super_(bool canAssign) { return NULL; }
static Obj* this_(bool canAssign) { return NULL; }
static Obj* literal(bool canAssign) { return NULL; }

static Obj* number(bool canAssign) {
    int radix = 0;
    bool sign_bit = true;
    const char* number_start = parser.previous.start;
    int number_len = parser.previous.length;
    if (parser.previous.length > 1) {
        switch (parser.previous.start[1]) {
            case 'x': radix = 16; sign_bit = false; break;
            case 'X': radix = 16; sign_bit = false; break;
            case 'b': radix = 2; sign_bit = false; break;
            case 'B': radix = 2; sign_bit = false; break;
            case 'd': radix = 10; sign_bit = false; break;
            case 'D': radix = 10; sign_bit = false; break;
        }
        if (radix != 0) {
            number_start = &parser.previous.start[2];
            number_len -= 2;
        }
    }
    if (radix == 0) {
        radix = 10;
    }

    ObjNumber* val = NULL;

    if (radix == 10 && sign_bit) {
        if (memchr(number_start, '.', number_len)) {
            // for now, use C's stdlib to reuse double formatting.
            double value = strtod(number_start, NULL);
            val = newNumberDouble(value);
        } else {
            int value = atoi(number_start);
            val = newNumberInteger(value);
        }
    }
    else {
        uint32_t value = strtoNum(number_start, number_len, radix);
        val = newNumberUInteger32(value);
    }
    return (Obj*) val;
}



static AstParseRule rules[] = {
    [TOKEN_LEFT_PAREN]           = {grouping,  call,   PREC_CALL},
    [TOKEN_RIGHT_PAREN]          = {NULL,      NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]           = {NULL,      NULL,   PREC_NONE},
    [TOKEN_RIGHT_BRACE]          = {NULL,      NULL,   PREC_NONE},
    [TOKEN_LEFT_SQUARE_BRACKET]  = {arrayinit, deref,  PREC_DEREF},
    [TOKEN_RIGHT_SQUARE_BRACKET] = {NULL,      NULL,   PREC_NONE},
    [TOKEN_COMMA]                = {NULL,      NULL,   PREC_NONE},
    [TOKEN_DOT]                  = {NULL,      dot,    PREC_CALL},
    [TOKEN_MINUS]                = {unary,     binary, PREC_TERM},
    [TOKEN_PLUS]                 = {NULL,      binary, PREC_TERM},
    [TOKEN_SEMICOLON]            = {NULL,      NULL,   PREC_NONE},
    [TOKEN_SLASH]                = {NULL,      binary, PREC_FACTOR},
    [TOKEN_STAR]                 = {NULL,      binary, PREC_FACTOR},
    [TOKEN_BAR]                  = {NULL,      binary, PREC_TERM},
    [TOKEN_AMP]                  = {NULL,      binary, PREC_TERM},
    [TOKEN_PERCENT]              = {NULL,      binary, PREC_TERM},
    [TOKEN_CARET]                = {NULL,      binary, PREC_TERM},
    [TOKEN_BANG]                 = {unary,     NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]           = {NULL,      binary, PREC_EQUALITY},
    [TOKEN_EQUAL]                = {NULL,      NULL,   PREC_NONE},
    [TOKEN_EQUAL_EQUAL]          = {NULL,      binary, PREC_EQUALITY},
    [TOKEN_GREATER]              = {NULL,      binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL]        = {NULL,      binary, PREC_COMPARISON},
    [TOKEN_RIGHT_SHIFT]          = {NULL,      binary, PREC_FACTOR},
    [TOKEN_LESS]                 = {NULL,      binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]           = {NULL,      binary, PREC_COMPARISON},
    [TOKEN_LEFT_SHIFT]           = {NULL,      binary, PREC_FACTOR},
    [TOKEN_IDENTIFIER]           = {variable,  NULL,   PREC_NONE},
    [TOKEN_STRING]               = {string,    NULL,   PREC_NONE},
    [TOKEN_NUMBER]               = {number,    NULL,   PREC_NONE},
    [TOKEN_AND]                  = {NULL,      and_,   PREC_AND},
    [TOKEN_CLASS]                = {NULL,      NULL,   PREC_NONE},
    [TOKEN_ELSE]                 = {NULL,      NULL,   PREC_NONE},
    [TOKEN_FALSE]                = {literal,   NULL,   PREC_NONE},
    [TOKEN_FOR]                  = {NULL,      NULL,   PREC_NONE},
    [TOKEN_FUN]                  = {NULL,      NULL,   PREC_NONE},
    [TOKEN_IF]                   = {NULL,      NULL,   PREC_NONE},
    [TOKEN_IMPORT]               = {literal,   NULL,   PREC_NONE},
    [TOKEN_LEN]                  = {literal,   NULL,   PREC_NONE},
    [TOKEN_MACHINE_UINT32]       = {literal,   NULL,   PREC_NONE},
    [TOKEN_MAKE_ARRAY]           = {literal,   NULL,   PREC_NONE},
    [TOKEN_MAKE_CHANNEL]         = {literal,   NULL,   PREC_NONE},
    [TOKEN_MAKE_ROUTINE]         = {literal,   NULL,   PREC_NONE},
    [TOKEN_NIL]                  = {literal,   NULL,   PREC_NONE},
    [TOKEN_OR]                   = {NULL,      or_,    PREC_OR},
    [TOKEN_PEEK]                 = {literal,   NULL,   PREC_NONE},
    [TOKEN_PRINT]                = {NULL,      NULL,   PREC_NONE},
    [TOKEN_RECEIVE]              = {literal,   NULL,   PREC_NONE},
    [TOKEN_RESUME]               = {literal,   NULL,   PREC_NONE},
    [TOKEN_RETURN]               = {NULL,      NULL,   PREC_NONE},
    [TOKEN_RPEEK]                = {literal,   NULL,   PREC_NONE},
    [TOKEN_RPOKE]                = {literal,   NULL,   PREC_NONE},
    [TOKEN_SEND]                 = {literal,   NULL,   PREC_NONE},
    [TOKEN_SHARE]                = {literal,   NULL,   PREC_NONE},
    [TOKEN_START]                = {literal,   NULL,   PREC_NONE},
    [TOKEN_SET_ELEMENT]          = {literal,   NULL,   PREC_NONE},
    [TOKEN_SUPER]                = {super_,    NULL,   PREC_NONE},
    [TOKEN_THIS]                 = {this_,     NULL,   PREC_NONE},
    [TOKEN_TRUE]                 = {literal,   NULL,   PREC_NONE},
    [TOKEN_VAR]                  = {NULL,      NULL,   PREC_NONE},
    [TOKEN_WHILE]                = {NULL,      NULL,   PREC_NONE},
    [TOKEN_YIELD]                = {NULL,      NULL,   PREC_NONE},
    [TOKEN_ERROR]                = {NULL,      NULL,   PREC_NONE},
    [TOKEN_EOF]                  = {NULL,      NULL,   PREC_NONE},
};

static AstParseRule* getRule(TokenType type) {
    return &rules[type];
}

static ObjExpression* parsePrecedence(Precedence precedence) {
    advance();
    AstParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return NULL;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;

    Obj* expr = prefixRule(canAssign);
    tempRootPush(OBJ_VAL(expr));

    ObjExpression* item = newExpression(expr);
    tempRootPush(OBJ_VAL(item));

    ObjExpression** cursor = &item->nextItem;
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        AstParseFn infixRule = getRule(parser.previous.type)->infix;
        Obj* expr = infixRule(canAssign);
        *cursor = newExpression(expr);
        cursor = &(*cursor)->nextItem;
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }

    tempRootPop();
    tempRootPop();
    return item;
}

static ObjExpression* expression() {
    return parsePrecedence(PREC_ASSIGNMENT);
}

static ObjExpressionStatement* expressionStatement() {
    ObjExpression* expr = expression();
    tempRootPush(OBJ_VAL(expr));
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    ObjExpressionStatement* expressionStatement = newExpressionStatement(expr);
    tempRootPop();
    return expressionStatement;
}

ObjExpressionStatement* statement() {
//    if (match(TOKEN_PRINT)) {
//        printStatement();
//    } else if (match(TOKEN_FOR)) {
//        forStatement();
//    } else if (match(TOKEN_IF)) {
//        ifStatement();
//    } else if (match(TOKEN_YIELD)) {
//        yieldStatement();
//    } else if (match(TOKEN_RETURN)) {
//        returnStatement();
//    } else if (match(TOKEN_WHILE)) {
//        whileStatement();
//    } else if (match(TOKEN_LEFT_BRACE)) {
//        beginScope();
//        block();
//        endScope();
//    } else {
        return expressionStatement();
//    }
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