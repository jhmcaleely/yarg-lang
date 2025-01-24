#ifndef clox_builtin_h
#define clox_builtin_h

#include "value.h"

bool makeRoutineBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result);
bool resumeBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result);
bool startBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result);

#endif