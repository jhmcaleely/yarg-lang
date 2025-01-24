#ifndef clox_builtin_h
#define clox_builtin_h

#include "value.h"

bool makeRoutineBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result);
bool resumeBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result);
bool startBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result);

#endif