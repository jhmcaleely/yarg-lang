#include <stdio.h>
#include <time.h>

#include "common.h"
#include "object.h"
#include "value.h"
#include "native.h"
#include "routine.h"
#include "vm.h"

#ifdef CLOX_PICO_TARGET
#include <hardware/gpio.h>
#endif

bool clockNative(ObjRoutine* routine, int argCount, Value* args, Value* result) {
    if (argCount != 0) {
        runtimeError(routine, "Expected 0 arguments but got %d.", argCount);
        return false;
    }

    *result = UINTEGER_VAL(clock() / CLOCKS_PER_SEC);
    return true;
}

#ifdef CLOX_PICO_TARGET
bool sleepNative(ObjRoutine* routine, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_DOUBLE(args[0])) {
        runtimeError(routine, "Argument must be a number");
        return false;
    }

    sleep_ms(AS_DOUBLE(args[0]));

    *result = NIL_VAL;
    return true;
}
#endif

#ifdef CLOX_PICO_TARGET
bool gpioInitNative(ObjRoutine* routine, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_DOUBLE(args[0])) {
        runtimeError(routine, "Argument must be a number");
        return false;
    }

    gpio_init(AS_DOUBLE(args[0]));

    *result = NIL_VAL;
    return true;
}
#endif

#ifdef CLOX_PICO_TARGET
static int64_t nativeOneShotCallback(alarm_id_t id, void* user_data) {

    ObjRoutine* routine = (ObjRoutine*)user_data;

    run(routine);

    if (routine->state != EXEC_ERROR) {
        routine->state = EXEC_CLOSED;
    }

    return 0;
}

static bool nativeRecurringCallback(struct repeating_timer* t) {

    ObjRoutine* routine = (ObjRoutine*)t->user_data;;

    run(routine);

    Value result = pop(routine); // unused.

    prepareRoutineStack(routine);

    return true;
}

bool alarmAddInMSNative(ObjRoutine* routine, int argCount, Value* args, Value* result) {
    if (argCount < 2 || argCount > 3) {
        runtimeError(routine, "Unexpected argument count adding alarm");
        return false;
    }

    if (!IS_ROUTINE(args[1])) {
        runtimeError(routine, "Second argument must be a routine.");
        return false;
    }

    ObjRoutine* isrRoutine = AS_ROUTINE(args[1]);

    int isrArity = 2 + isrRoutine->entryFunction->function->arity;
    if (argCount != isrArity) {
        runtimeError(routine, "Expected %d arguments but got %d.", isrArity, argCount);
        return false;
    }

    if (!IS_DOUBLE(args[0])) {
        runtimeError(routine, "First argument must be a number.");
        return false;
    }

    if (isrRoutine->type != ROUTINE_ISR) {
        runtimeError(routine, "Argument must be an ISR routine.");
        return false;
    }

    if (argCount > 2) {
        bindEntryArgs(isrRoutine, args[2]);
    }
    prepareRoutineStack(isrRoutine);

    isrRoutine->state = EXEC_RUNNING;
    add_alarm_in_ms(AS_DOUBLE(args[0]), nativeOneShotCallback, isrRoutine, false);

    *result = NIL_VAL;
    return true;
}

bool alarmAddRepeatingMSNative(ObjRoutine* routine, int argCount, Value* args, Value* result) {
    if (argCount < 2 || argCount > 3) {
        runtimeError(routine, "Unexpected argument count adding alarm");
        return false;
    }

    if (!IS_ROUTINE(args[1])) {
        runtimeError(routine, "Second argument must be a routine.");
        return false;
    }

    ObjRoutine* isrRoutine = AS_ROUTINE(args[1]);

    int isrArity = 2 + isrRoutine->entryFunction->function->arity;
    if (argCount != isrArity) {
        runtimeError(routine, "Expected %d arguments but got %d.", isrArity, argCount);
        return false;
    }

    if (!IS_DOUBLE(args[0])) {
        runtimeError(routine, "First argument must be a number.");
        return false;
    }

    if (isrRoutine->type != ROUTINE_ISR) {
        runtimeError(routine, "Argument must be an ISR routine.");
        return false;
    }

    ObjBlob* handle = newBlob(sizeof(repeating_timer_t));

    if (argCount > 2) {
        bindEntryArgs(isrRoutine, args[2]);
    }
    prepareRoutineStack(isrRoutine);

    isrRoutine->state = EXEC_RUNNING;
    add_repeating_timer_ms(AS_DOUBLE(args[0]), nativeRecurringCallback, isrRoutine, (repeating_timer_t*)handle->blob);

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
#endif
