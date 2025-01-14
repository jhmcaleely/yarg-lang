#include "common.h"

#include "channel.h"
#include "memory.h"

ObjChannel* newChannel() {
    ObjChannel* channel = ALLOCATE_OBJ(ObjChannel, OBJ_CHANNEL);
    channel->data = NIL_VAL;
    channel->present = false;
    channel->overflow = false;
    return channel;
}

bool sendChannelBuiltin(ObjThreadStack* thread, int argCount, Value* args, Value* result) {
    return false;
}

bool receiveChannelBuiltin(ObjThreadStack* thread, int argCount, Value* args, Value* result) {
    return false;
}

bool shareChannelBuiltin(ObjThreadStack* thread, int argCount, Value* args, Value* result) {
    return false;
}

bool peekChannelBuiltin(ObjThreadStack* thread, int argCount, Value* args, Value* result) {
    return false;
}