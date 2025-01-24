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

    prepareRoutine(routine, closure);

    *result = OBJ_VAL(routine);
    return true;
}

bool resumeBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result) {
    if (!IS_ROUTINE(args[0])) {
        runtimeError(routineContext, "Argument to resume must be a routine.");
        return false;
    }

    ObjRoutine* target = AS_ROUTINE(args[0]);

    if (target->state != EXEC_SUSPENDED) {
        runtimeError(routineContext, "routine must be suspended to resume.");
        return false;
    }

    int resumeArity = 1 + target->entryFunction->function->arity;
    if (argCount != resumeArity) {
        runtimeError(routineContext, "Expected %d arguments but got %d.", resumeArity, argCount);
        return false;
    }

    prepareRoutineStack(target, target->entryFunction->function->arity, &args[1]);

    InterpretResult execResult = run(target);
    if (execResult != INTERPRET_OK) {
        return false;
    }

    Value coroResult = pop(target);

    *result = coroResult;
    return true;
}

#define FLAG_VALUE 123

void nativeCore1Entry() {
    multicore_fifo_push_blocking(FLAG_VALUE);
    uint32_t g = multicore_fifo_pop_blocking();

    if (g != FLAG_VALUE) {
        fatalVMError("Core1 Entry amd sync failed.");
    }

    ObjRoutine* core = vm.core1;

    InterpretResult execResult = run(core);

}

bool startBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result) {
    if (!IS_ROUTINE(args[0])) {
        runtimeError(routineContext, "Argument to start must be a routine.");
        return false;
    }

    ObjRoutine* target = AS_ROUTINE(args[0]);

    if (target->state != EXEC_SUSPENDED) {
        runtimeError(routineContext, "routine must be suspended to resume.");
        return false;
    }

    int startArity = 1 + target->entryFunction->function->arity;
    if (argCount != startArity) {
        runtimeError(routineContext, "Expected %d arguments but got %d.", startArity, argCount);
        return false;
    }

    prepareRoutineStack(target, target->entryFunction->function->arity, &args[1]);

    vm.core1 = target;

    multicore_launch_core1(nativeCore1Entry);

    // Wait for it to start up
    uint32_t g = multicore_fifo_pop_blocking();
    if (g != FLAG_VALUE) {
        fatalVMError("Core startup failure.");
        return false;
    }
    multicore_fifo_push_blocking(FLAG_VALUE);

    *result = NIL_VAL;
    return true;
}
