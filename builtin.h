#ifndef clox_builtin_h
#define clox_builtin_h

#include "value.h"

Value makeIsrBuiltin(ObjThreadStack* thread, int argCount, Value* args);
Value makeCoroBuiltin(ObjThreadStack* thread, int argCount, Value* args);


#endif