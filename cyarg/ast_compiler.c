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

typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

typedef struct AstCompiler {
    struct AstCompiler* enclosing;
    ObjFunction* function;
    ObjStmt* ast;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount;
    Upvalue upvalues[UINT8_COUNT];
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
    } else {
        local->name = copyString("", 0);
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

static int addUpvalue(AstCompiler* compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

static int resolveUpvalue(AstCompiler* compiler, ObjString* name) {
    if (compiler->enclosing == NULL) return -1;

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t)upvalue, false);
    }

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

static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

static void patchJump(int offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
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

static void generateNumber(ObjExprNumber* num) {
    switch(num->type) {
        case NUMBER_DOUBLE: emitConstant(DOUBLE_VAL(num->val.dbl)); break;
        case NUMBER_INTEGER: emitConstant(INTEGER_VAL(num->val.integer)); break;
        case NUMBER_UINTEGER32: emitConstant(UINTEGER_VAL(num->val.uinteger32)); break;
        default:
            return; //  unreachable
    }
}

static void generateExprString(ObjExprString* str) {
    emitConstant(OBJ_VAL(str->string));
}

static void generateExprLogicalAnd(ObjExprOperation* bin) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    generateExpr(bin->rhs);

    patchJump(endJump);
}

static void generateExprOperation(ObjExprOperation* bin) {

    switch (bin->operation) {
        case EXPR_OP_LOGICAL_AND: generateExprLogicalAnd(bin); return;
        default:
            break;
    }

    generateExpr(bin->rhs);

    switch (bin->operation) {
        case EXPR_OP_EQUAL: emitByte(OP_EQUAL); return;
        case EXPR_OP_GREATER: emitByte(OP_GREATER); return;
        case EXPR_OP_RIGHT_SHIFT: emitByte(OP_RIGHT_SHIFT); return;
        case EXPR_OP_LESS: emitByte(OP_LESS); return;
        case EXPR_OP_LEFT_SHIFT: emitByte(OP_LEFT_SHIFT); return;
        case EXPR_OP_ADD: emitByte(OP_ADD); return;
        case EXPR_OP_SUBTRACT: emitByte(OP_SUBTRACT); return;
        case EXPR_OP_MULTIPLY: emitByte(OP_MULTIPLY); return;
        case EXPR_OP_DIVIDE: emitByte(OP_DIVIDE); return;
        case EXPR_OP_BIT_OR: emitByte(OP_BITOR); return;
        case EXPR_OP_BIT_AND: emitByte(OP_BITAND); return;
        case EXPR_OP_BIT_XOR: emitByte(OP_BITXOR); return;
        case EXPR_OP_MODULO: emitByte(OP_MODULO); return;
        case EXPR_OP_NOT_EQUAL: emitBytes(OP_EQUAL, OP_NOT); return;
        case EXPR_OP_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); return;
        case EXPR_OP_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT); return;
        case EXPR_OP_NOT: emitByte(OP_NOT); return;
        case EXPR_OP_NEGATE: emitByte(OP_NEGATE); return;
        default: break;
    }
}

static void generateExprGrouping(ObjExprGrouping* grp) {
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

static void generateExprLiteral(ObjExprLiteral* lit) {
    switch (lit->literal) {
        case EXPR_LITERAL_FALSE: emitByte(OP_FALSE); break;
        case EXPR_LITERAL_TRUE: emitByte(OP_TRUE); break;
        case EXPR_LITERAL_NIL: emitByte(OP_NIL); break;
    }
}

static void generateArgs(ObjArguments* args) {
    for (int i = 0; i < args->count; i++) {
        generateExpr((ObjExpr*)args->arguments[i]);
    }
}

static void generateExprCall(ObjExprCall* call) {
    generateArgs(call->args);

    emitBytes(OP_CALL, call->args->count);
}

static void generateExprArrayInit(ObjExprArrayInit* array) {
 
    emitBytes(OP_GET_BUILTIN, BUILTIN_MAKE_ARRAY);
    emitConstant(INTEGER_VAL(array->args->count));
    emitBytes(OP_CALL, 1);
 
    for (int i = 0; i < array->args->count; i++) {
        emitConstant(INTEGER_VAL(i));
        generateExpr((ObjExpr*)array->args->arguments[i]);
        emitByte(OP_SET_ELEMENT);
    }
}

static void generateExprArrayElement(ObjExprArrayElement* array) {

    generateExpr(array->element);

    if (array->assignment) {
        generateExpr(array->assignment);
        emitByte(OP_SET_ELEMENT);
    } else {
        emitByte(OP_ELEMENT);
    }
}

static void generateExprBuiltin(ObjExprBuiltin* fn) {
    switch(fn->builtin) {
        case EXPR_BUILTIN_IMPORT: emitBytes(OP_GET_BUILTIN, BUILTIN_IMPORT); break;
        case EXPR_BUILTIN_MAKE_ARRAY: emitBytes(OP_GET_BUILTIN, BUILTIN_MAKE_ARRAY); break;
        case EXPR_BUILTIN_MAKE_ROUTINE: emitBytes(OP_GET_BUILTIN, BUILTIN_MAKE_ROUTINE); break;
        case EXPR_BUILTIN_MAKE_CHANNEL: emitBytes(OP_GET_BUILTIN, BUILTIN_MAKE_CHANNEL); break;
        case EXPR_BUILTIN_RESUME: emitBytes(OP_GET_BUILTIN, BUILTIN_RESUME); break;
        case EXPR_BUILTIN_START: emitBytes(OP_GET_BUILTIN, BUILTIN_START); break;
        case EXPR_BUILTIN_RECEIVE: emitBytes(OP_GET_BUILTIN, BUILTIN_RECEIVE); break;
        case EXPR_BUILTIN_SEND: emitBytes(OP_GET_BUILTIN, BUILTIN_SEND); break;
        case EXPR_BUILTIN_PEEK: emitBytes(OP_GET_BUILTIN, BUILTIN_PEEK); break;
        case EXPR_BUILTIN_SHARE: emitBytes(OP_GET_BUILTIN, BUILTIN_SHARE); break;
        case EXPR_BUILTIN_RPEEK: emitBytes(OP_GET_BUILTIN, BUILTIN_RPEEK); break;
        case EXPR_BUILTIN_RPOKE: emitBytes(OP_GET_BUILTIN, BUILTIN_RPOKE); break;
        case EXPR_BUILTIN_LEN: emitBytes(OP_GET_BUILTIN, BUILTIN_LEN); break;
    }    
}

static void generateExprElt(ObjExpr* expr) {
    
    switch (expr->obj.type) {
        case OBJ_EXPR_NUMBER: {
            ObjExprNumber* num = (ObjExprNumber*)expr;
            generateNumber(num);
            break;
        }
        case OBJ_EXPR_OPERATION: {
            ObjExprOperation* op = (ObjExprOperation*)expr;
            generateExprOperation(op);
            break;
        }
        case OBJ_EXPR_GROUPING: {
            ObjExprGrouping* grp = (ObjExprGrouping*)expr;
            generateExprGrouping(grp);
            break;
        }
        case OBJ_EXPR_NAMEDVARIABLE: {
            ObjExprNamedVariable* var = (ObjExprNamedVariable*)expr;
            generateExprNamedVariable(var);
            break;
        }
        case OBJ_EXPR_LITERAL: {
            ObjExprLiteral* lit = (ObjExprLiteral*)expr;
            generateExprLiteral(lit);
            break;
        }
        case OBJ_EXPR_STRING: {
            ObjExprString* str = (ObjExprString*)expr;
            generateExprString(str);
            break;
        }
        case OBJ_EXPR_CALL: {
            ObjExprCall* call = (ObjExprCall*)expr;
            generateExprCall(call);
            break;
        }
        case OBJ_EXPR_ARRAYINIT: {
            ObjExprArrayInit* array = (ObjExprArrayInit*)expr;
            generateExprArrayInit(array);
            break;
        }
        case OBJ_EXPR_ARRAYELEMENT: {
            ObjExprArrayElement* array = (ObjExprArrayElement*)expr;
            generateExprArrayElement(array);
            break;
        }
        case OBJ_EXPR_BUILTIN: {
            ObjExprBuiltin* fn = (ObjExprBuiltin*)expr;
            generateExprBuiltin(fn);
            break;
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

static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;

    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth > current->scopeDepth) {
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
        current->localCount--;
    }
}

static void generate(ObjStmt* stmt);
static ObjFunction* endCompiler();

static void generateStmtBlock(ObjStmtBlock* block) {
    beginScope();
    generate(block->statements);
    endScope();
}

static void generateStmtIf(ObjStmtIf* ctrl) {
    generateExpr(ctrl->test);
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    generate(ctrl->ifStmt);

    int elseJump = emitJump(OP_JUMP);

    patchJump(thenJump);
    emitByte(OP_POP);
    if (ctrl->elseStmt) {
        generate(ctrl->elseStmt);
    }
    patchJump(elseJump);
}

static void generateFunction(FunctionType type, ObjFunctionDeclaration* decl) {
    AstCompiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    for (int i = 0; i < decl->arity; i++) {
        uint8_t constant = parseVariable(((ObjExprNamedVariable*)decl->params[i])->name);
        defineVariable(constant);
    }

    generate((ObjStmt*)decl->body);

    ObjFunction* function = endCompiler();
    function->arity = decl->arity;
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

static void generateStmtFunDeclaration(ObjStmtFunDeclaration* decl) {

    uint8_t global = parseVariable(decl->name);
    markInitialized();

    generateFunction(TYPE_FUNCTION, decl->function);

    defineVariable(global);
}


static void generateStmt(ObjStmt* stmt) {
    switch (stmt->obj.type) {
        case OBJ_STMT_EXPRESSION:
            generateExpr(((ObjStmtExpression*)stmt)->expression);
            emitByte(OP_POP);
            break;
        case OBJ_STMT_PRINT:
            generateExpr(((ObjStmtPrint*)stmt)->expression);
            emitByte(OP_PRINT);
            break;
        case OBJ_STMT_VARDECLARATION:
            generateVarDeclaration((ObjStmtVarDeclaration*)stmt);
            break;
        case OBJ_STMT_BLOCK:
            generateStmtBlock((ObjStmtBlock*)stmt);
            break;
        case OBJ_STMT_IF:
            generateStmtIf((ObjStmtIf*)stmt);
            break;
        case OBJ_STMT_FUNDECLARATION:
            generateStmtFunDeclaration((ObjStmtFunDeclaration*)stmt);
            break;
        default:
            return; // Unexpected
    }
}


static void generate(ObjStmt* stmt) {

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

    generate(current->ast);

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