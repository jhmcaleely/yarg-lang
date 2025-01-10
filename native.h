#ifndef clox_native_h
#define clox_native_h

#include "value.h"

Value clockNative(int argCount, Value* args);
Value sleepNative(int argCount, Value* args);

#endif