#ifndef cyarg_ast_compiler_h
#define cyarg_ast_compiler_h

#include "object.h"
#include "vm.h"

ObjFunction* astCompile(const char* source);
void markAstCompilerRoots();

#endif