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

static AstCompiler* current = NULL;

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

static void generate() {
    return;
}

ObjFunction* astCompile(const char* source) {
    initScanner(source);
    AstCompiler compiler;
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