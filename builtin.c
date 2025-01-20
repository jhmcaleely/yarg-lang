#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "common.h"
#include "object.h"
#include "value.h"
#include "builtin.h"
#include "native.h"
#include "routine.h"
#include "channel.h"
#include "vm.h"

bool makeRoutineBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (argCount != 2) {
        runtimeError(thread, "Expected 2 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_CLOSURE(args[0]) || !IS_BOOL(args[1])) {
        runtimeError(thread, "Argument to make_routine must be a function and a boolean.");
        return false;
    }

    ObjClosure* closure = AS_CLOSURE(args[0]);
    bool isISR = AS_BOOL(args[1]);

    ObjRoutine* newThread = newRoutine(isISR ? THREAD_ISR : THREAD_NORMAL);

    prepareRoutine(newThread, closure);

    *result = OBJ_VAL(newThread);
    return true;
}

bool resumeBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (!IS_ROUTINE(args[0])) {
        runtimeError(thread, "Argument to resume must be a routine.");
        return false;
    }

    ObjRoutine* coroThread = AS_ROUTINE(args[0]);

    if (coroThread->state != EXEC_SUSPENDED) {
        runtimeError(thread, "routine must be suspended to resume.");
        return false;
    }

    int resumeArity = 1 + coroThread->entryFunction->function->arity;
    if (argCount != resumeArity) {
        runtimeError(thread, "Expected %d arguments but got %d.", resumeArity, argCount);
        return false;
    }

    push(coroThread, OBJ_VAL(coroThread->entryFunction));

    for (int arg = 0; arg < coroThread->entryFunction->function->arity; arg++) {
        push(coroThread, args[arg + 1]);
    }

    callfn(coroThread, coroThread->entryFunction, coroThread->entryFunction->function->arity);

    InterpretResult execResult = run(coroThread);
    if (execResult != INTERPRET_OK) {
        return false;
    }

    Value coroResult = pop(coroThread);

    *result = coroResult;
    return true;
}

#define FLAG_VALUE 123

void nativeCore1Entry() {
    multicore_fifo_push_blocking(FLAG_VALUE);
    uint32_t g = multicore_fifo_pop_blocking();

    if (g != FLAG_VALUE) {
        fatalMemoryError("Core1 Entry amd sync failed.");
    }

    ObjRoutine* core = vm.core1;

    InterpretResult execResult = run(core);

}

bool startBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (!IS_ROUTINE(args[0])) {
        runtimeError(thread, "Argument to start must be a routine.");
        return false;
    }

    ObjRoutine* coreThread = AS_ROUTINE(args[0]);

    if (coreThread->state != EXEC_SUSPENDED) {
        runtimeError(thread, "routine must be suspended to resume.");
        return false;
    }

    int startArity = 1 + coreThread->entryFunction->function->arity;
    if (argCount != startArity) {
        runtimeError(thread, "Expected %d arguments but got %d.", startArity, argCount);
        return false;
    }

    push(coreThread, OBJ_VAL(coreThread->entryFunction));

    for (int arg = 0; arg < coreThread->entryFunction->function->arity; arg++) {
        push(coreThread, args[arg + 1]);
    }

    callfn(coreThread, coreThread->entryFunction, coreThread->entryFunction->function->arity);

    vm.core1 = coreThread;

    multicore_launch_core1(nativeCore1Entry);

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

bool makeChannelBuiltin(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (argCount != 0) {
        runtimeError(thread, "Expected 0 arguments but got %d.", argCount);
        return false;
    }

    ObjChannel* channel = newChannel();

    *result = OBJ_VAL(channel);
    return true;
}