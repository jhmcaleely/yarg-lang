#ifndef cyarg_builtin_h
#define cyarg_builtin_h

#include "value.h"

bool importBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result);
bool makeRoutineBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result);
bool resumeBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result);
bool startBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result);
bool rpeekBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result);
bool rpokeBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result);

#endif