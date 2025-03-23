#ifndef clox_native_h
#define clox_native_h

#include "value.h"

bool clockNative(ObjRoutine* routine, int argCount, Value* args, Value* result);
bool sleepNative(ObjRoutine* routine, int argCount, Value* args, Value* result);

bool alarmAddInMSNative(ObjRoutine* routine, int argCount, Value* args, Value* result);
bool alarmAddRepeatingMSNative(ObjRoutine* routine, int argCount, Value* args, Value* result);
bool alarmCancelRepeatingMSNative(ObjRoutine* routine, int argCount, Value* args, Value* result);

#endif