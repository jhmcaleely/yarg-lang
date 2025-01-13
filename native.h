#ifndef clox_native_h
#define clox_native_h

#include "value.h"

bool clockNative(ObjThreadStack* thread, int argCount, Value* args, Value* result);
bool sleepNative(ObjThreadStack* thread, int argCount, Value* args, Value* result);

bool gpioInitNative(ObjThreadStack* thread, int argCount, Value* args, Value* result);
bool gpioSetDirectionNative(ObjThreadStack* thread, int argCount, Value* args, Value* result);
bool gpioPutNative(ObjThreadStack* thread, int argCount, Value* args, Value* result);

bool alarmAddInMSNative(ObjThreadStack* thread, int argCount, Value* args, Value* result);
bool alarmAddRepeatingMSNative(ObjThreadStack* thread, int argCount, Value* args, Value* result);
bool alarmCancelRepeatingMSNative(ObjThreadStack* thread, int argCount, Value* args, Value* result);

#endif