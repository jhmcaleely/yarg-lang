#include <stdlib.h>

#include "common.h"
#include "ast_compiler.h"
#include "parser.h"
#include "ast.h"
#include "compiler_common.h"
#include "memory.h"
#include "object.h"
#include "scanner.h"

typedef struct AstCompiler {
    struct AstCompiler* enclosing;
    ObjFunction* function;
    ObjStmt* ast;
    FunctionType type;
} AstCompiler;

static struct AstCompiler* current = NULL;

static void initCompiler(AstCompiler* compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->ast = NULL;
    compiler->function = NULL;
    compiler->type = type;

    compiler->function = newFunction();
    current = compiler;
    if (type != TYPE_SCRIPT) {
        current->function->name = copyString(parser.previous.start,
                                             parser.previous.length);
    }
}


static Chunk* currentChunk() {
    return &current->function->chunk;
}


static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static uint8_t makeConstant(Value value) {

    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static int emitConstant(Value value) {

    int32_t i = AS_INTEGER(value);
    if (IS_INTEGER(value) && i <= INT8_MAX && i >= INT8_MIN) {
        emitBytes(OP_IMMEDIATE, (uint8_t)i);
    } else {
        emitBytes(OP_CONSTANT, makeConstant(value));
    }
    return 2;
}

static void emitReturn() {
    if (current->type == TYPE_INITIALIZER) {
        emitBytes(OP_GET_LOCAL, 0);
    } else {
        emitByte(OP_NIL);
    }

    emitByte(OP_RETURN);
}

static void generateNumber(ObjNumber* num) {
    switch(num->type) {
        case NUMBER_DOUBLE: emitConstant(DOUBLE_VAL(num->val.dbl)); break;
        case NUMBER_INTEGER: emitConstant(INTEGER_VAL(num->val.integer)); break;
        case NUMBER_UINTEGER32: emitConstant(UINTEGER_VAL(num->val.uinteger32)); break;
    }
}

static void generateExprElt(Obj* expr) {
    
    switch (expr->type) {
        case OBJ_NUMBER: {
            ObjNumber* num = (ObjNumber*)expr;
            generateNumber(num);
            break;
        }
    }
}


static void generateExpr(ObjExpression* expr) {

    while (expr != NULL) {
        generateExprElt(expr->expr);
        expr = expr->nextItem;
    }
}


static void generateStmt(ObjStmt* stmt) {
    switch (stmt->obj.type) {
        case OBJ_EXPRESSIONSTMT:
            generateExpr(((ObjExpressionStatement*)stmt)->expression);
            emitByte(OP_POP);
            break;
        case OBJ_PRINTSTMT:
            generateExpr(((ObjPrintStatement*)stmt)->expression);
            emitByte(OP_PRINT);
            break;
    }
}


static void generate() {
    ObjStmt* stmt = current->ast;
    while (stmt != NULL) {
        generateStmt(stmt);
        stmt = stmt->nextStmt;
    }
}



static ObjFunction* endCompiler() {
    emitReturn();

    current->ast = NULL;
    ObjFunction* function = current->function;
    current = current->enclosing;

    return function;
}


ObjFunction* astCompile(const char* source) {
    initScanner(source);
    struct AstCompiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    current->ast = parse();

    generate();

    ObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
}

void markAstCompilerRoots() {
    AstCompiler* compiler = current;
    while (compiler != NULL) {
        markObject((Obj*)compiler->function);
        markObject((Obj*)compiler->ast);
        compiler = compiler->enclosing;
    }
}