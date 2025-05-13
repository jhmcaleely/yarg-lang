#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "parser.h"
#include "compiler_common.h"
#include "ast.h"
#include "memory.h"

typedef ObjExpr* (*AstParseFn)(bool canAssign);

typedef struct {
    AstParseFn prefix;
    AstParseFn infix;
    Precedence precedence;
} AstParseRule;

static AstParseRule* getRule(TokenType type);
static ObjExpr* parsePrecedence(Precedence precedence);
static ObjExpr* expression();

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

static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

static void synchronize() {
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



static ObjExpr* namedVariable(Token name, bool canAssign) {
    ObjExprNamedVariable* expr = newExprNamedVariable(name.start, name.length, NULL);

    if (canAssign && match(TOKEN_EQUAL)) {
        tempRootPush(OBJ_VAL(expr));
        expr->assignment = expression();
        tempRootPop();
    }

    return (ObjExpr*)expr;
}

static ObjExpr* call(bool canAssign) { return NULL;}
static ObjExpr* arrayinit(bool canAssign) { return NULL; }
static ObjExpr* deref(bool canAssign) {return NULL; }
static ObjExpr* dot(bool canAssign) { return NULL; }
static ObjExpr* or_(bool canAssign) { return NULL; }
static ObjExpr* super_(bool canAssign) { return NULL; }
static ObjExpr* this_(bool canAssign) { return NULL; }

static ObjExpr* string(bool canAssign) {
    return (ObjExpr*) newExprString(parser.previous.start + 1, parser.previous.length - 2);
}

static ObjExpr* literal(bool canAssign) {     
    switch (parser.previous.type) {
        case TOKEN_FALSE: return (ObjExpr*) newExprLiteral(EXPR_LITERAL_FALSE);
        case TOKEN_NIL: return (ObjExpr*) newExprLiteral(EXPR_LITERAL_NIL);
        case TOKEN_TRUE: return (ObjExpr*) newExprLiteral(EXPR_LITERAL_TRUE);
        default: return NULL; // Unreachable.
    } 
}

static ObjExpr* variable(bool canAssign) {

    return namedVariable(parser.previous, canAssign);
}

static ObjExpr* unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    ObjExpr* rhs = parsePrecedence(PREC_UNARY);
    tempRootPush(OBJ_VAL(rhs));

    ExprOp op;
    switch (operatorType) {
        case TOKEN_BANG: op = EXPR_OP_NOT; break;
        case TOKEN_MINUS: op = EXPR_OP_NEGATE; break;
        default: break; // Unreachable.
    }

    ObjExprOperation* expr = newExprOperation(rhs, op);
    tempRootPop();
    return (ObjExpr*) expr; 
}

static ObjExpr* grouping(bool canAssign) {
    ObjExpr* expr = expression();
    tempRootPush(OBJ_VAL(expr));
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
    ObjExprGrouping* grp = newExprGrouping(expr);
    tempRootPop();
    return (ObjExpr*)grp; 
}

static ObjExpr* and_(bool canAssign) { 
    ObjExpr* rhs = parsePrecedence(PREC_AND);
    tempRootPush(OBJ_VAL(rhs));
    ObjExprOperation* expr = newExprOperation(rhs, EXPR_OP_LOGICAL_AND);
    tempRootPop();
    return (ObjExpr*) expr;
}


static ObjExpr* binary(bool canAssign) {
    TokenType operatorType = parser.previous.type;
    AstParseRule* rule = getRule(operatorType);
    ObjExpr* rhs = parsePrecedence((Precedence)(rule->precedence + 1));
    tempRootPush(OBJ_VAL(rhs));

    ExprOp op;
    switch (operatorType) {
        case TOKEN_EQUAL_EQUAL:   op = EXPR_OP_EQUAL; break;
        case TOKEN_GREATER:       op = EXPR_OP_GREATER; break;
        case TOKEN_RIGHT_SHIFT:   op = EXPR_OP_RIGHT_SHIFT; break;
        case TOKEN_LESS:          op = EXPR_OP_LESS; break;
        case TOKEN_LEFT_SHIFT:    op = EXPR_OP_LEFT_SHIFT; break;
        case TOKEN_PLUS:          op = EXPR_OP_ADD; break;
        case TOKEN_MINUS:         op = EXPR_OP_SUBTRACT; break;
        case TOKEN_STAR:          op = EXPR_OP_MULTIPLY; break;
        case TOKEN_SLASH:         op = EXPR_OP_DIVIDE; break;
        case TOKEN_BAR:           op = EXPR_OP_BIT_OR; break;
        case TOKEN_AMP:           op = EXPR_OP_BIT_AND; break;
        case TOKEN_CARET:         op = EXPR_OP_BIT_XOR; break;
        case TOKEN_PERCENT:       op = EXPR_OP_MODULO; break;
        case TOKEN_BANG_EQUAL:    op = EXPR_OP_NOT_EQUAL; break;
        case TOKEN_GREATER_EQUAL: op = EXPR_OP_GREATER_EQUAL; break;
        case TOKEN_LESS_EQUAL:    op = EXPR_OP_LESS_EQUAL; break;
        default:
            return NULL; // Unreachable.
    }

    ObjExprOperation* expr = newExprOperation(rhs, op);
    tempRootPop();
    return (ObjExpr*) expr;
}

static ObjExpr* number(bool canAssign) {
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

    ObjExprNumber* val = NULL;

    if (radix == 10 && sign_bit) {
        if (memchr(number_start, '.', number_len)) {
            // for now, use C's stdlib to reuse double formatting.
            double value = strtod(number_start, NULL);
            val = newExprNumberDouble(value);
        } else {
            int value = atoi(number_start);
            val = newExprNumberInteger(value);
        }
    }
    else {
        uint32_t value = strtoNum(number_start, number_len, radix);
        val = newExprNumberUInteger32(value);
    }
    return (ObjExpr*) val;
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

static ObjExpr* parsePrecedence(Precedence precedence) {
    advance();
    AstParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return NULL;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;

    ObjExpr* expr = prefixRule(canAssign);
    tempRootPush(OBJ_VAL(expr));

    ObjExpr** cursor = &expr->nextExpr;
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        AstParseFn infixRule = getRule(parser.previous.type)->infix;
        *cursor = infixRule(canAssign);
        cursor = &(*cursor)->nextExpr;
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }

    tempRootPop();
    return expr;
}

static ObjExpr* expression() {
    return parsePrecedence(PREC_ASSIGNMENT);
}

static ObjStmtPrint* printStatement() {
    ObjExpr* expr = expression();
    tempRootPush(OBJ_VAL(expr));
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    ObjStmtPrint* print = newStmtPrint(expr);
    tempRootPop();
    return print;
}

static ObjStmtExpression* expressionStatement() {
    ObjExpr* expr = expression();
    tempRootPush(OBJ_VAL(expr));
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    ObjStmtExpression* expressionStatement = newStmtExpression(expr);
    tempRootPop();
    return expressionStatement;
}

static ObjStmt* varDeclaration() {
    consume(TOKEN_IDENTIFIER, "Expect variable name.");
    ObjStmtVarDeclaration* decl = newStmtVarDeclaration((char*)parser.previous.start, parser.previous.length, NULL);
    tempRootPush(OBJ_VAL(decl));

    if (match(TOKEN_EQUAL)) {
        decl->initialiser = expression();
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration");

    tempRootPop();

    return (ObjStmt*) decl;
}

static ObjStmt* declaration();
static ObjStmt* statement();

static ObjStmt* block() {

    ObjStmtBlock* block = newStmtBlock();
    tempRootPush(OBJ_VAL(block));

    ObjStmt** cursor = &block->statements;

    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        *cursor = declaration();
        cursor = &(*cursor)->nextStmt;
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");

    tempRootPop();

    return (ObjStmt*) block;
}

ObjStmt* ifStatement() {
    ObjStmtIf* ctrl = newStmtIf();
    tempRootPush(OBJ_VAL(ctrl));

    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    ctrl->test = expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    ctrl->ifStmt = statement();
    if (match(TOKEN_ELSE)) {
        ctrl->elseStmt = statement();
    }
    tempRootPop();
    return (ObjStmt*)ctrl;
}

ObjStmt* statement() {
    if (match(TOKEN_PRINT)) {
        return (ObjStmt*) printStatement();
//    } else if (match(TOKEN_FOR)) {
//        forStatement();
    } else if (match(TOKEN_IF)) {
        return ifStatement();
//    } else if (match(TOKEN_YIELD)) {
//        yieldStatement();
//    } else if (match(TOKEN_RETURN)) {
//        returnStatement();
//    } else if (match(TOKEN_WHILE)) {
//        whileStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        return block();
    } else {
        return (ObjStmt*) expressionStatement();
    }
}

ObjStmt* declaration() {
    ObjStmt* stmt = NULL;

    if (match(TOKEN_CLASS)) {
//        classDeclaration();
    } else if (match(TOKEN_FUN)) {
//        funDeclaration();
    } else if (match(TOKEN_VAR)) {
        stmt = varDeclaration();
    } else {
        stmt = statement();
    }

    if (parser.panicMode) synchronize();
    return stmt;
}


void parse(ObjStmt** ast_root) {
    ObjStmt** cursor = ast_root;

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    while (!match(TOKEN_EOF)) {
        *cursor = declaration();
        if (*cursor) {
            cursor = &(*cursor)->nextStmt;
        }
    }
}