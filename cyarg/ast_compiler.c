#include <stdlib.h>

#include "common.h"
#include "ast_compiler.h"
#include "compiler_common.h"
#include "memory.h"
#include "scanner.h"

typedef struct AstCompiler {
    struct AstCompiler* enclosing;
    ObjFunction* function;
    FunctionType type;
} AstCompiler;

static AstCompiler* current = NULL;

static void initCompiler(AstCompiler* compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
}

ObjFunction* astCompile(const char* source) {
    initScanner(source);
    AstCompiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    return NULL;
}

void markAstCompilerRoots() {
    AstCompiler* compiler = current;
    while (compiler != NULL) {
        markObject((Obj*)compiler->function);
        compiler = compiler->enclosing;
    }
}