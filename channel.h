#ifndef clox_channel_h
#define clox_channel_h

#include "value.h"
#include "object.h"

ObjChannel* newChannel();

bool sendChannelBuiltin(ObjThreadStack* thread, int argCount, Value* args, Value* result);
bool receiveChannelBuiltin(ObjThreadStack* thread, int argCount, Value* args, Value* result);
bool shareChannelBuiltin(ObjThreadStack* thread, int argCount, Value* args, Value* result);
bool peekChannelBuiltin(ObjThreadStack* thread, int argCount, Value* args, Value* result);

#endif