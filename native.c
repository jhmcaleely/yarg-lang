#ifdef LOX_PICO_SDK
    #include "pico/stdlib.h"
#endif

#include <stdio.h>
#include <time.h>

#include "common.h"
#include "object.h"
#include "value.h"
#include "native.h"
#include "routine.h"
#include "vm.h"

bool clockNative(ObjRoutine* routine, int argCount, Value* args, Value* result) {
    if (argCount != 0) {
        runtimeError(routine, "Expected 0 arguments but got %d.", argCount);
        return false;
    }

    *result = NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
    return true;
}

bool sleepNative(ObjRoutine* routine, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_NUMBER(args[0])) {
        runtimeError(routine, "Argument must be a number");
        return false;
    }

    sleep_ms(AS_NUMBER(args[0]));

    *result = NIL_VAL;
    return true;
}

bool gpioInitNative(ObjRoutine* routine, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_NUMBER(args[0])) {
        runtimeError(routine, "Argument must be a number");
        return false;
    }

    gpio_init(AS_NUMBER(args[0]));

    *result = NIL_VAL;
    return true;
}

bool gpioSetDirectionNative(ObjRoutine* routine, int argCount, Value* args, Value* result) {
    if (argCount != 2) {
        runtimeError(routine, "Expected 2 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_NUMBER(args[0]) || !IS_BOOL(args[1])) {
        runtimeError(routine, "Arguments must be a number and a bool");
        return false;
    }

    bool direction = AS_BOOL(args[1]);
    if (direction) {
        gpio_set_dir(AS_NUMBER(args[0]), GPIO_OUT);
    } else {
        gpio_set_dir(AS_NUMBER(args[0]), GPIO_IN);
    }

    *result = NIL_VAL;
    return true;
}

bool gpioPutNative(ObjRoutine* routine, int argCount, Value* args, Value* result) {
    if (argCount != 2) {
        runtimeError(routine, "Expected 2 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_NUMBER(args[0]) || !IS_BOOL(args[1])) {
        runtimeError(routine, "Arguments must be a number and a bool");
        return false;
    }

    gpio_put(AS_NUMBER(args[0]), AS_BOOL(args[1]));

    *result = NIL_VAL;
    return true;
}

static int64_t nativeOneShotCallback(alarm_id_t id, void* user_data) {
    ObjRoutine* routine = AS_ROUTINE((uintptr_t)user_data);

    run(routine);

    if (routine->state != EXEC_ERROR) {
        routine->state = EXEC_CLOSED;
    }

    return 0;
}

static bool nativeRecurringCallback(struct repeating_timer* t) {
    ObjRoutine* routine = AS_ROUTINE((uintptr_t)t->user_data);

    run(routine);

    Value result = pop(routine); // unused.

    push(routine, OBJ_VAL(routine->entryFunction));

    for (int arg = routine->entryFunction->function->arity; arg > 0; arg--) {
        push(routine, peek(routine, arg));
    }

    callfn(routine, routine->entryFunction, routine->entryFunction->function->arity);

    return true;
}

bool alarmAddInMSNative(ObjRoutine* routine, int argCount, Value* args, Value* result) {
    if (argCount < 2 || !IS_ROUTINE(args[1])) {
        runtimeError(routine, "Second argument must be a routine.");
        return false;
    }

    ObjRoutine* isrRoutine = AS_ROUTINE(args[1]);

    int isrArity = 2 + isrRoutine->entryFunction->function->arity;
    if (argCount != isrArity) {
        runtimeError(routine, "Expected %d arguments but got %d.", isrArity, argCount);
        return false;
    }

    if (!IS_NUMBER(args[0])) {
        runtimeError(routine, "First argument must be a number.");
        return false;
    }

    if (isrRoutine->type != ROUTINE_ISR) {
        runtimeError(routine, "Argument must be an ISR routine.");
        return false;
    }

    prepareRoutineStack(isrRoutine, argCount - 2, &args[2]);

    isrRoutine->state = EXEC_RUNNING;
    add_alarm_in_ms(AS_NUMBER(args[0]), nativeOneShotCallback, isrRoutine, false);

    *result = NIL_VAL;
    return true;
}

bool alarmAddRepeatingMSNative(ObjRoutine* routine, int argCount, Value* args, Value* result) {
    if (argCount < 2 || !IS_ROUTINE(args[1])) {
        runtimeError(routine, "Second argument must be a routine.");
        return false;
    }

    ObjRoutine* isrRoutine = AS_ROUTINE(args[1]);

    int isrArity = 2 + isrRoutine->entryFunction->function->arity;
    if (argCount != isrArity) {
        runtimeError(routine, "Expected %d arguments but got %d.", isrArity, argCount);
        return false;
    }

    if (!IS_NUMBER(args[0])) {
        runtimeError(routine, "First argument must be a number.");
        return false;
    }

    if (isrRoutine->type != ROUTINE_ISR) {
        runtimeError(routine, "Argument must be an ISR routine.");
        return false;
    }

    ObjBlob* handle = newBlob(sizeof(repeating_timer_t));

    for (int arg = 0; arg < isrRoutine->entryFunction->function->arity; arg++) {
        push(isrRoutine, args[arg + 2]);
    }

    prepareRoutineStack(isrRoutine, isrRoutine->entryFunction->function->arity, &args[2]);

    isrRoutine->state = EXEC_RUNNING;
    add_repeating_timer_ms(AS_NUMBER(args[0]), nativeRecurringCallback, isrRoutine, (repeating_timer_t*)handle->blob);

    *result = OBJ_VAL(handle);
    return true;
}

bool alarmCancelRepeatingMSNative(ObjRoutine* routine, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_BLOB(args[0])) {
        runtimeError(routine, "Argument must be a native blob.");
        return false;
    }

    ObjBlob* blob = AS_BLOB(args[0]);

    bool cancelled = cancel_repeating_timer((repeating_timer_t*)blob->blob);

    // EXEC_CLOSED ?

    *result = BOOL_VAL(cancelled);
    return true;
}

