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
#include "vm.h"
#include "debug.h"

bool makeRoutineBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result) {
    if (argCount != 2) {
        runtimeError(routineContext, "Expected 2 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_CLOSURE(args[0]) || !IS_BOOL(args[1])) {
        runtimeError(routineContext, "Argument to make_routine must be a function and a boolean.");
        return false;
    }

    ObjClosure* closure = AS_CLOSURE(args[0]);
    bool isISR = AS_BOOL(args[1]);

    ObjRoutine* routine = newRoutine(isISR ? ROUTINE_ISR : ROUTINE_THREAD);

    if (bindEntryFn(routine, closure)) {
        *result = OBJ_VAL(routine);
        return true;
    }
    else {
        runtimeError(routineContext, "Function must take 0 or 1 arguments.");
        return false;
    }
}

bool resumeBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result) {
    if (argCount < 1 || argCount > 2) {
        runtimeError(routineContext, "Expected one or two arguments to resume.");
        return false;
    }

    if (!IS_ROUTINE(args[0])) {
        runtimeError(routineContext, "Argument to resume must be a routine.");
        return false;
    }

    ObjRoutine* target = AS_ROUTINE(args[0]);

    if (target->state != EXEC_SUSPENDED && target->state != EXEC_UNBOUND) {
        runtimeError(routineContext, "routine must be suspended or unbound to resume.");
        return false;
    }

    if (argCount > 1) {
        bindEntryArgs(target, args[1]);
    }

    if (target->state == EXEC_UNBOUND) {
        prepareRoutineStack(target);
        target->state == EXEC_SUSPENDED;
    }
    else if (target->state == EXEC_SUSPENDED) {
        push(target, target->entryArg);
    }

    InterpretResult execResult = run(target);
    if (execResult != INTERPRET_OK) {
        return false;
    }

    Value coroResult = pop(target);

    *result = coroResult;
    return true;
}

// use ascii 'c' 'l' 'o' 'x'
#define FLAG_VALUE 0x636c6f78

void nativeCore1Entry() {
    multicore_fifo_push_blocking(FLAG_VALUE);
    uint32_t g = multicore_fifo_pop_blocking();

    if (g != FLAG_VALUE) {
        fatalVMError("Core1 entry and sync failed.");
    }

    ObjRoutine* core = vm.core1;

    InterpretResult execResult = run(core);

    if (core->state != EXEC_ERROR) {
        core->state = EXEC_CLOSED;
    }

    vm.core1 = NULL;
}

bool startBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result) {
    if (argCount < 1 || argCount > 2) {
        runtimeError(routineContext, "Expected one or two arguments to start.");
        return false;
    }

    if (!IS_ROUTINE(args[0])) {
        runtimeError(routineContext, "Argument to start must be a routine.");
        return false;
    }

    ObjRoutine* target = AS_ROUTINE(args[0]);

    if (target->state != EXEC_UNBOUND) {
        runtimeError(routineContext, "routine must be unbound to start.");
        return false;
    }

    if (vm.core1 != NULL) {
        runtimeError(routineContext, "Core1 already occupied.");
        return false;
    }

    if (argCount > 1) {
        bindEntryArgs(target, args[1]);
    }

    prepareRoutineStack(target);

    vm.core1 = target;
    vm.core1->state = EXEC_RUNNING;

    multicore_reset_core1();
    multicore_launch_core1(nativeCore1Entry);

    // Wait for it to start up
    uint32_t g = multicore_fifo_pop_blocking();
    if (g != FLAG_VALUE) {
        fatalVMError("Core1 startup failure.");
        return false;
    }
    multicore_fifo_push_blocking(FLAG_VALUE);

    *result = NIL_VAL;
    return true;
}
