#ifndef clox_builtin_h
#define clox_builtin_h

#include "value.h"

bool makeRoutineBuiltin(ObjRoutine* routine, int argCount, Value* args, Value* result);
bool resumeBuiltin(ObjRoutine* routine, int argCount, Value* args, Value* result);
bool startBuiltin(ObjRoutine* routine, int argCount, Value* args, Value* result);

#endif