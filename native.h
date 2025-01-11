#ifndef clox_native_h
#define clox_native_h

#include "value.h"

Value clockNative(int thread, int argCount, Value* args);
Value sleepNative(int thread, int argCount, Value* args);

Value gpioInitNative(int thread, int argCount, Value* args);
Value gpioSetDirectionNative(int thread, int argCount, Value* args);
Value gpioPutNative(int thread, int argCount, Value* args);

Value alarmAddInMSNative(int thread, int argCount, Value* args);
Value alarmAddRepeatingMSNative(int thread, int argCount, Value* args);
Value alarmCancelRepeatingMSNative(int thread, int argCount, Value* args);

#endif