#ifndef clox_channel_h
#define clox_channel_h

#include "value.h"
#include "object.h"

ObjChannel* newChannel();

bool sendChannelBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result);
bool receiveChannelBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result);
bool shareChannelBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result);
bool peekChannelBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result);

#endif