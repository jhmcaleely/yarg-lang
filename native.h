#ifndef clox_native_h
#define clox_native_h

#include "value.h"

Value clockNative(int argCount, Value* args);
Value sleepNative(int argCount, Value* args);

Value gpioInitNative(int argCount, Value* args);
Value gpioSetDirectionNative(int argCount, Value* args);
Value gpioPutNative(int argCount, Value* args);

Value alarmAddInMSNative(int argCount, Value* args);

#endif