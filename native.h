#ifndef clox_native_h
#define clox_native_h

#include "value.h"

Value clockNative(ObjThreadStack* thread, int argCount, Value* args);
Value sleepNative(ObjThreadStack* thread, int argCount, Value* args);

Value gpioInitNative(ObjThreadStack* thread, int argCount, Value* args);
Value gpioSetDirectionNative(ObjThreadStack* thread, int argCount, Value* args);
Value gpioPutNative(ObjThreadStack* thread, int argCount, Value* args);

Value alarmAddInMSNative(ObjThreadStack* thread, int argCount, Value* args);
Value alarmAddRepeatingMSNative(ObjThreadStack* thread, int argCount, Value* args);
Value alarmCancelRepeatingMSNative(ObjThreadStack* thread, int argCount, Value* args);

#endif