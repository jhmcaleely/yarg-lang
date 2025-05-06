#include <stdlib.h>

#include "common.h"
#include "ast_compiler.h"
#include "parser.h"
#include "ast.h"
#include "compiler_common.h"
#include "memory.h"
#include "scanner.h"

typedef struct AstCompiler {
    struct AstCompiler* enclosing;
    ObjFunction* function;
    ObjExpressionStatement* ast;
    FunctionType type;
} AstCompiler;

static struct AstCompiler* current;

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

static void generateExpr(ObjExpression*) {
    
}


static void generateStmt(ObjExpressionStatement* stmt) {
    generateExpr(stmt->expression);
    emitByte(OP_POP);
}


static void generate() {
    ObjExpressionStatement* stmt = current->ast;
    while (stmt != NULL) {
        generateStmt(stmt);
        stmt = stmt->next;
    }
}


ObjFunction* astCompile(const char* source) {
    initScanner(source);
    struct AstCompiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    current->ast = parse();

    generate();

    return parser.hadError ? NULL : current->function;
}

void markAstCompilerRoots() {
    AstCompiler* compiler = current;
    while (compiler != NULL) {
        markObject((Obj*)compiler->function);
        markObject((Obj*)compiler->ast);
        compiler = compiler->enclosing;
    }
}