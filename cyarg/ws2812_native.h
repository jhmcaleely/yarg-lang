#ifndef cyarg_ws2812_builtin_h
#define cyarg_ws2812_builtin_h

#include "value.h"

bool ws2812initNative(ObjRoutine* routineContext, int argCount, Value* args, Value* result);
bool ws2812writepixelNative(ObjRoutine* routineContext, int argCount, Value* args, Value* result);


#endif