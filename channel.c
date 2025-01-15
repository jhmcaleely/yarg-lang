#include <stdio.h>
#include "pico/stdlib.h"

#include "common.h"

#include "channel.h"
#include "memory.h"
#include "vm.h"

ObjChannel* newChannel() {
    ObjChannel* channel = ALLOCATE_OBJ(ObjChannel, OBJ_CHANNEL);
    channel->data = NIL_VAL;
    channel->present = false;
    channel->overflow = false;
    return channel;
}

bool sendChannelBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (argCount != 2) {
        runtimeError(thread, "Expected 2 arguments, got %d.", argCount);
        return false;
    }
    if (!IS_CHANNEL(args[0])) {
        runtimeError(thread, "Argument must be a channel.");
        return false;
    }

    ObjChannel* channel = AS_CHANNEL(args[0]);
    while (channel->present) {
        // stall/block until space
        tight_loop_contents();
    }

    channel->data = args[1];
    channel->present = true;
    channel->overflow = false;
    *result = BOOL_VAL(channel->overflow);

    return true;
}

bool receiveChannelBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments, got %d.", argCount);
        return false;
    }
    if (!IS_CHANNEL(args[0])) {
        runtimeError(thread, "Argument must be a channel.");
        return false;
    }

    ObjChannel* channel = AS_CHANNEL(args[0]);
    while (!channel->present) {
        // stall/block until space
        tight_loop_contents();
    }

    *result = channel->data;
    channel->present = false;
    channel->overflow = false;
    
    return true;
}

bool shareChannelBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (argCount != 2) {
        runtimeError(thread, "Expected 2 arguments, got %d.", argCount);
        return false;
    }
    if (!IS_CHANNEL(args[0])) {
        runtimeError(thread, "Argument must be a channel.");
        return false;
    }

    ObjChannel* channel = AS_CHANNEL(args[0]);
    if (channel->present) {
        channel->overflow = true;
    }
    channel->data = args[1];
    channel->present = true;
    *result = BOOL_VAL(channel->overflow);

    return true;
}

bool peekChannelBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments, got %d.", argCount);
        return false;
    }
    if (!IS_CHANNEL(args[0])) {
        runtimeError(thread, "Argument must be a channel.");
        return false;
    }

    ObjChannel* channel = AS_CHANNEL(args[0]);
    if (channel->present) {
        *result = channel->data;
    }
    else {
        *result = NIL_VAL;
    }

    return true;
}