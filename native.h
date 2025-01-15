#ifndef clox_native_h
#define clox_native_h

#include "value.h"

bool clockNative(ObjRoutine* thread, int argCount, Value* args, Value* result);
bool sleepNative(ObjRoutine* thread, int argCount, Value* args, Value* result);

bool gpioInitNative(ObjRoutine* thread, int argCount, Value* args, Value* result);
bool gpioSetDirectionNative(ObjRoutine* thread, int argCount, Value* args, Value* result);
bool gpioPutNative(ObjRoutine* thread, int argCount, Value* args, Value* result);

bool alarmAddInMSNative(ObjRoutine* thread, int argCount, Value* args, Value* result);
bool alarmAddRepeatingMSNative(ObjRoutine* thread, int argCount, Value* args, Value* result);
bool alarmCancelRepeatingMSNative(ObjRoutine* thread, int argCount, Value* args, Value* result);

#endif