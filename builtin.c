#include <stdlib.h>

#include "common.h"
#include "object.h"
#include "value.h"
#include "builtin.h"
#include "native.h"
#include "threadstack.h"
#include "vm.h"

Value makeIsrBuiltin(ObjThreadStack* thread, int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
    }
    if (!IS_CLOSURE(args[0])) {
        runtimeError(thread, "Argument to make_isr must be function.");
    }

    ObjClosure* closure = AS_CLOSURE(args[0]);

    ObjThreadStack* isrThread = newThread(THREAD_ISR);
    isrThread->entryFunction = closure;
    isrThread->nextThread = vm.isrStack;
    vm.isrStack = isrThread;
    vm.isrCount++;


    push(isrThread, OBJ_VAL(closure));
    callfn(isrThread, closure, 0);

    return OBJ_VAL(isrThread);
}

Value makeCoroBuiltin(ObjThreadStack* thread, int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
    }
    if (!IS_CLOSURE(args[0])) {
        runtimeError(thread, "Argument to make_coro must be function.");
    }

    ObjClosure* closure = AS_CLOSURE(args[0]);

    ObjThreadStack* coroThread = newThread(THREAD_NORMAL);
    coroThread->entryFunction = closure;
//    isrThread->nextThread = vm.isrStack;
//    vm.isrStack = isrThread;
//    vm.isrCount++;


    push(coroThread, OBJ_VAL(closure));
    callfn(coroThread, closure, 0);

    return OBJ_VAL(coroThread);
}