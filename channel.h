#ifndef clox_channel_h
#define clox_channel_h

#include "value.h"
#include "object.h"

ObjChannel* newChannel();

bool makeChannelBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result);
bool sendChannelBuiltin(ObjRoutine* routine, int argCount, Value* args, Value* result);
bool receiveChannelBuiltin(ObjRoutine* routine, int argCount, Value* args, Value* result);
bool shareChannelBuiltin(ObjRoutine* routine, int argCount, Value* args, Value* result);
bool peekChannelBuiltin(ObjRoutine* routine, int argCount, Value* args, Value* result);

#endif