#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "ast_compiler.h"
#include "parser.h"
#include "ast.h"
#include "compiler_common.h"
#include "memory.h"
#include "object.h"
#include "scanner.h"

static void generateExpr(ObjExpr* expr);

typedef struct {
    ObjString* name;
    int depth;
    bool isCaptured;
} Local;

typedef struct AstCompiler {
    struct AstCompiler* enclosing;
    ObjFunction* function;
    ObjStmt* ast;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
} AstCompiler;

static struct AstCompiler* current = NULL;

static void initCompiler(AstCompiler* compiler, FunctionType type) {

    compiler->enclosing = current;
    compiler->ast = NULL;
    compiler->function = NULL;
    compiler->type = type;

    compiler->localCount = 0;
    compiler->scopeDepth = 0;

    compiler->function = newFunction();
    current = compiler;
    if (type != TYPE_SCRIPT) {
        current->function->name = copyString(parser.previous.start,
                                             parser.previous.length);
    }

    Local* local = &current->locals[current->localCount++];
    local->name = NULL;
    local->depth = 0;
    local->isCaptured = false;
    if (type != TYPE_FUNCTION) {
        local->name = copyString("this", 4);
    }
}



static void error(const char* message) {
    fprintf(stderr, "%s\n", message);
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

static bool identifiersEqual(ObjString* a, ObjString* b) {
    if (a->length != b->length) return false;
    return memcmp(a->chars, b->chars, a->length) == 0;
}

static int resolveLocal(AstCompiler* compiler, ObjString* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static void addLocal(ObjString* name) {
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
}


static int resolveUpvalue(AstCompiler* compiler, ObjString* name) {
    if (compiler->enclosing == NULL) return -1;

    // TODO!
    return -1;
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

static uint8_t identifierConstant(ObjString* name) {
    return makeConstant(OBJ_VAL(name));
}


static void declareVariable(ObjString* name) {
    if (current->scopeDepth == 0) return;

    for (int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(name);
}

static uint8_t parseVariable(ObjString* name) {

    declareVariable(name);
    if (current->scopeDepth > 0) return 0;

    return identifierConstant(name);
}

static void generateNumber(ObjNumber* num) {
    switch(num->type) {
        case NUMBER_DOUBLE: emitConstant(DOUBLE_VAL(num->val.dbl)); break;
        case NUMBER_INTEGER: emitConstant(INTEGER_VAL(num->val.integer)); break;
        case NUMBER_UINTEGER32: emitConstant(UINTEGER_VAL(num->val.uinteger32)); break;
        default:
            return; //  unreachable
    }
}

static void generateOperationExpr(ObjOperationExpr* bin) {
    generateExpr(bin->rhs);

    switch (bin->operation) {
        case EXPR_OP_EQUAL: emitByte(OP_EQUAL); break;
        case EXPR_OP_GREATER: emitByte(OP_GREATER); break;
        case EXPR_OP_RIGHT_SHIFT: emitByte(OP_RIGHT_SHIFT); break;
        case EXPR_OP_LESS: emitByte(OP_LESS); break;
        case EXPR_OP_LEFT_SHIFT: emitByte(OP_LEFT_SHIFT); break;
        case EXPR_OP_ADD: emitByte(OP_ADD); break;
        case EXPR_OP_SUBTRACT: emitByte(OP_SUBTRACT); break;
        case EXPR_OP_MULTIPLY: emitByte(OP_MULTIPLY); break;
        case EXPR_OP_DIVIDE: emitByte(OP_DIVIDE); break;
        case EXPR_OP_BITOR: emitByte(OP_BITOR); break;
        case EXPR_OP_BITAND: emitByte(OP_BITAND); break;
        case EXPR_OP_BITXOR: emitByte(OP_BITXOR); break;
        case EXPR_OP_MODULO: emitByte(OP_MODULO); break;
        case EXPR_OP_NOT_EQUAL: emitBytes(OP_EQUAL, OP_NOT); break;
        case EXPR_OP_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case EXPR_OP_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT); break;
        case EXPR_OP_NOT: emitByte(OP_NOT); break;
        case EXPR_OP_NEGATE: emitByte(OP_NEGATE); break;
    }
}

static void generateGroupingExpr(ObjGroupingExpr* grp) {
    generateExpr(grp->expression);
}

static void generateExprNamedVariable(ObjExprNamedVariable* var) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, var->name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, var->name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(var->name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }
    
    if (var->assignment) {
        generateExpr(var->assignment);
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void generateExprElt(ObjExpr* expr) {
    
    switch (expr->obj.type) {
        case OBJ_EXPR_NUMBER: {
            ObjNumber* num = (ObjNumber*)expr;
            generateNumber(num);
            break;
        }
        case OBJ_EXPR_OPERATION: {
            ObjOperationExpr* op = (ObjOperationExpr*)expr;
            generateOperationExpr(op);
            break;
        }
        case OBJ_EXPR_GROUPING: {
            ObjGroupingExpr* grp = (ObjGroupingExpr*)expr;
            generateGroupingExpr(grp);
            break;
        }
        case OBJ_EXPR_NAMEDVARIABLE: {
            ObjExprNamedVariable* var = (ObjExprNamedVariable*)expr;
            generateExprNamedVariable(var);
        }
        default:
            return; // unexpected
    }
}


static void generateExpr(ObjExpr* expr) {

    while (expr != NULL) {
        generateExprElt(expr);
        expr = expr->nextExpr;
    }
}

static void markInitialized() {
    if (current->scopeDepth == 0) return;
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

static void generateVarDeclaration(ObjStmtVarDeclaration* decl) {
    uint8_t global = parseVariable(decl->name);

    if (decl->initialiser) {
        generateExpr(decl->initialiser);
    } else {
        emitByte(OP_NIL);
    }

    defineVariable(global);
}


static void generateStmt(ObjStmt* stmt) {
    switch (stmt->obj.type) {
        case OBJ_STMT_EXPRESSION:
            generateExpr(((ObjExpressionStatement*)stmt)->expression);
            emitByte(OP_POP);
            break;
        case OBJ_STMT_PRINT:
            generateExpr(((ObjPrintStatement*)stmt)->expression);
            emitByte(OP_PRINT);
            break;
        case OBJ_STMT_VARDECLARATION:
            generateVarDeclaration((ObjStmtVarDeclaration*)stmt);
            break;
        default:
            return; // Unexpected
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

    parse(&current->ast);
    printStmts(current->ast);

    generate();

    ObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
}

void markAstCompilerRoots() {
    AstCompiler* compiler = current;
    while (compiler != NULL) {
        markObject((Obj*)compiler->function);
        markObject((Obj*)compiler->ast);

        for (int i = 0; i < compiler->localCount; i++) {
            markObject((Obj*)compiler->locals[i].name);
        }

        compiler = compiler->enclosing;
    }
}