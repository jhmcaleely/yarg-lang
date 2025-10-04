#include <stdio.h>

#include "common.h"

#include "channel.h"
#include "memory.h"
#include "vm.h"
#include "debug.h"

ObjChannel* newChannel(size_t capacity) {
    ObjChannel* channel = ALLOCATE_OBJ(ObjChannel, OBJ_CHANNEL);
    tempRootPush(OBJ_VAL(channel));
    channel->overflow = false;

    channel->buffer = ALLOCATE(Value, capacity);
    channel->occupied = ALLOCATE(bool, capacity);
    channel->bufferSize = capacity;

    for (int i = 0; i < capacity; i++) {
        channel->buffer[i] = NIL_VAL;
        channel->occupied[i] = false;
    }

    return channel;
}

bool makeChannelBuiltin(ObjRoutine* routine, int argCount, Value* result) {
    if (argCount != 0) {
        runtimeError(routine, "Expected 0 arguments but got %d.", argCount);
        return false;
    }

    ObjChannel* channel = newChannel(1);

    *result = OBJ_VAL(channel);
    return true;
}

bool sendChannelBuiltin(ObjRoutine* routine, int argCount, Value* result) {
    if (argCount != 2) {
        runtimeError(routine, "Expected 2 arguments, got %d.", argCount);
        return false;
    }

    Value channelVal = nativeArgument(routine, argCount, 0);

    if (!IS_CHANNEL(channelVal)) {
        runtimeError(routine, "Argument must be a channel.");
        return false;
    }

    ObjChannel* channel = AS_CHANNEL(channelVal);
#ifdef CYARG_PICO_TARGET
    while (channel->occupied[0]) {
        // stall/block until space
        tight_loop_contents();
    }
#endif

    channel->buffer[0] = nativeArgument(routine, argCount, 1);
    channel->occupied[0] = true;
    channel->overflow = false;
    *result = BOOL_VAL(channel->overflow);

    return true;
}

bool receiveChannelBuiltin(ObjRoutine* routine, int argCount, Value* result) {
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 arguments, got %d.", argCount);
        return false;
    }

    Value channelVal = nativeArgument(routine, argCount, 0);

    if (!IS_CHANNEL(channelVal) && !IS_ROUTINE(channelVal)) {
        runtimeError(routine, "Argument must be a channel or a routine.");
        return false;
    }

    if (IS_CHANNEL(channelVal)) {
        ObjChannel* channel = AS_CHANNEL(channelVal);
#ifdef CYARG_PICO_TARGET
        while (!channel->occupied[0]) {
            // stall/block until space
            tight_loop_contents();
        }
#endif
        *result = channel->buffer[0];
        channel->occupied[0] = false;
        channel->overflow = false;

    } 
    else if (IS_ROUTINE(channelVal)) {
        ObjRoutine* routineParam = AS_ROUTINE(channelVal);

#ifdef CYARG_PICO_TARGET
        while (routineParam->state == EXEC_RUNNING) {
            tight_loop_contents();
        }
#endif    
        
        if (routineParam->state == EXEC_CLOSED || routineParam->state == EXEC_SUSPENDED) {
            *result = peek(routineParam, -1);
        } 
        else {
            *result = NIL_VAL;
        }
    }
    return true;
}

bool shareChannelBuiltin(ObjRoutine* routine, int argCount, Value* result) {
    if (argCount != 2) {
        runtimeError(routine, "Expected 2 arguments, got %d.", argCount);
        return false;
    }

    Value channelVal = nativeArgument(routine, argCount, 0);
    
    if (!IS_CHANNEL(channelVal)) {
        runtimeError(routine, "Argument must be a channel.");
        return false;
    }

    ObjChannel* channel = AS_CHANNEL(channelVal);
    if (channel->occupied[0]) {
        channel->overflow = true;
    }
    channel->buffer[0] = nativeArgument(routine, argCount, 1);
    channel->occupied[0] = true;
    *result = BOOL_VAL(channel->overflow);

    return true;
}

bool peekChannelBuiltin(ObjRoutine* routine, int argCount, Value* result) {
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 arguments, got %d.", argCount);
        return false;
    }

    Value channelVal = nativeArgument(routine, argCount, 0);

    if (!IS_CHANNEL(channelVal)) {
        runtimeError(routine, "Argument must be a channel.");
        return false;
    }

    ObjChannel* channel = AS_CHANNEL(channelVal);
    if (channel->occupied[0]) {
        *result = channel->buffer[0];
    }
    else {
        *result = NIL_VAL;
    }

    return true;
}