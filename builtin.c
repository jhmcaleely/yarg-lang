#include <stdlib.h>

#include "common.h"
#include "object.h"
#include "value.h"
#include "builtin.h"
#include "native.h"
#include "threadstack.h"
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
    isrThread->nextThread = vm.isrStack;
    vm.isrStack = isrThread;
    vm.isrCount++;


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
    coroThread->nextThread = vm.coroList;
    vm.coroList = coroThread;

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