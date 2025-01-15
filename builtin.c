#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "common.h"
#include "object.h"
#include "value.h"
#include "builtin.h"
#include "native.h"
#include "threadstack.h"
#include "channel.h"
#include "vm.h"

bool makeIsrBuiltin(ObjThreadStack* thread, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_CLOSURE(args[0])) {
        runtimeError(thread, "Argument to make_isr must be function.");
        return false;
    }

    ObjClosure* closure = AS_CLOSURE(args[0]);

    ObjThreadStack* isrThread = newThread(THREAD_ISR);
    isrThread->entryFunction = closure;


    push(isrThread, OBJ_VAL(closure));
    callfn(isrThread, closure, 0);

    *result = OBJ_VAL(isrThread);
    return true;
}

bool makeCoroBuiltin(ObjThreadStack* thread, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_CLOSURE(args[0])) {
        runtimeError(thread, "Argument to make_coro must be function.");
        return false;
    }

    ObjClosure* closure = AS_CLOSURE(args[0]);

    ObjThreadStack* coroThread = newThread(THREAD_NORMAL);
    coroThread->entryFunction = closure;

    push(coroThread, OBJ_VAL(closure));
    callfn(coroThread, closure, 0);

    *result = OBJ_VAL(coroThread);
    return true;
}

bool makeMainBuiltin(ObjThreadStack* thread, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_CLOSURE(args[0])) {
        runtimeError(thread, "Argument to make_main must be function.");
        return false;
    }

    ObjClosure* closure = AS_CLOSURE(args[0]);

    ObjThreadStack* coroThread = newThread(THREAD_NORMAL);
    coroThread->entryFunction = closure;

    push(coroThread, OBJ_VAL(closure));
    callfn(coroThread, closure, 0);

    *result = OBJ_VAL(coroThread);
    return true;
}

bool resumeBuiltin(ObjThreadStack* thread, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_THREAD_STACK(args[0])) {
        runtimeError(thread, "Argument to resume must be coroutine.");
        return false;
    }

    ObjThreadStack* coroThread = AS_THREAD_STACK(args[0]);

    if (coroThread->state != EXEC_SUSPENDED) {
        runtimeError(thread, "coroutine must be suspended to resume.");
        return false;
    }

    InterpretResult execResult = run(coroThread);
    if (execResult != INTERPRET_OK) {
        return false;
    }

    *result = NIL_VAL;
    return true;
}

#define FLAG_VALUE 123

void nativeCoreEntry() {
    multicore_fifo_push_blocking(FLAG_VALUE);
    uint32_t g = multicore_fifo_pop_blocking();

    if (g != FLAG_VALUE) {
        fatalMemoryError("Core1 Entry amd sync failed.");
    }

    InterpretResult execResult = run(vm.core1);
}


bool startBuiltin(ObjThreadStack* thread, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_THREAD_STACK(args[0])) {
        runtimeError(thread, "Argument to start must be coroutine.");
        return false;
    }

    ObjThreadStack* coroThread = AS_THREAD_STACK(args[0]);

    if (coroThread->state != EXEC_SUSPENDED) {
        runtimeError(thread, "coroutine must be suspended to resume.");
        return false;
    }

    vm.core1 = coroThread;

    multicore_launch_core1(nativeCoreEntry);

    // Wait for it to start up
    uint32_t g = multicore_fifo_pop_blocking();
    if (g != FLAG_VALUE) {
        fatalMemoryError("Core startup failure.");
        return false;
    }
    multicore_fifo_push_blocking(FLAG_VALUE);

    *result = NIL_VAL;
    return true;
}

bool makeChannelBuiltin(ObjThreadStack* thread, int argCount, Value* args, Value* result) {
    if (argCount != 0) {
        runtimeError(thread, "Expected 0 arguments but got %d.", argCount);
        return false;
    }

    ObjChannel* channel = newChannel();

    *result = OBJ_VAL(channel);
    return true;
}