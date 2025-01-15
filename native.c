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

bool clockNative(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (argCount != 0) {
        runtimeError(thread, "Expected 0 arguments but got %d.", argCount);
        return false;
    }

    *result = NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
    return true;
}

bool sleepNative(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_NUMBER(args[0])) {
        runtimeError(thread, "Argument must be a number");
        return false;
    }

    sleep_ms(AS_NUMBER(args[0]));

    *result = NIL_VAL;
    return true;
}

bool gpioInitNative(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_NUMBER(args[0])) {
        runtimeError(thread, "Argument must be a number");
        return false;
    }

    gpio_init(AS_NUMBER(args[0]));

    *result = NIL_VAL;
    return true;
}

bool gpioSetDirectionNative(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (argCount != 2) {
        runtimeError(thread, "Expected 2 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_NUMBER(args[0]) || !IS_BOOL(args[1])) {
        runtimeError(thread, "Arguments must be a number and a bool");
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

bool gpioPutNative(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (argCount != 2) {
        runtimeError(thread, "Expected 2 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_NUMBER(args[0]) || !IS_BOOL(args[1])) {
        runtimeError(thread, "Arguments must be a number and a bool");
        return false;
    }

    gpio_put(AS_NUMBER(args[0]), AS_BOOL(args[1]));

    *result = NIL_VAL;
    return true;
}

static int64_t nativeAlarmCallback(alarm_id_t id, void* user_data) {
    ObjRoutine* thread = AS_ROUTINE((uintptr_t)user_data);

    printf("#nativeAlarmCallback %d", id);
    printObject(OBJ_VAL(thread));
    printf("\n");

    run(thread);

    if (thread->state != EXEC_ERROR) {
        thread->state = EXEC_CLOSED;
    }

    return 0;
}

static bool nativeRepeatingCallback(struct repeating_timer* t) {
    ObjRoutine* thread = AS_ROUTINE((uintptr_t)t->user_data);

    printf("#nativeRepeatingAlarmCallback %d", t->alarm_id);
    printObject(OBJ_VAL(thread));
    printf("\n");

    run(thread);

    push(thread, OBJ_VAL(thread->entryFunction));
    callfn(thread, thread->entryFunction, 0);

    return true;
}

bool alarmAddInMSNative(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (argCount != 2) {
        runtimeError(thread, "Expected 2 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_NUMBER(args[0]) || !IS_ROUTINE(args[1])) {
        runtimeError(thread, "Argument must be a number and a routine.");
        return false;
    }

    ObjRoutine* isrThread = AS_ROUTINE(args[1]);
    if (isrThread->type != THREAD_ISR) {
        runtimeError(thread, "Argument must be an ISR routine.");
        return false;
    }

    isrThread->state = EXEC_RUNNING;
    add_alarm_in_ms(AS_NUMBER(args[0]), nativeAlarmCallback, isrThread, false);

    *result = NIL_VAL;
    return true;
}

bool alarmAddRepeatingMSNative(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (argCount != 2) {
        runtimeError(thread, "Expected 2 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_NUMBER(args[0]) || !IS_ROUTINE(args[1])) {
        runtimeError(thread, "Argument must be a number and an isr stack.");
        return false;
    }

    ObjRoutine* isrThread = AS_ROUTINE(args[1]);
    if (isrThread->type != THREAD_ISR) {
        runtimeError(thread, "Argument must be an ISR routine.");
        return false;
    }

    ObjBlob* handle = newBlob(sizeof(repeating_timer_t));

    isrThread->state = EXEC_RUNNING;
    add_repeating_timer_ms(AS_NUMBER(args[0]), nativeRepeatingCallback, isrThread, (repeating_timer_t*)handle->blob);

    *result = OBJ_VAL(handle);
    return true;
}

bool alarmCancelRepeatingMSNative(ObjRoutine* thread, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_BLOB(args[0])) {
        runtimeError(thread, "Argument must be a native blob.");
        return false;
    }

    ObjBlob* blob = AS_BLOB(args[0]);

    bool cancelled = cancel_repeating_timer((repeating_timer_t*)blob->blob);

    // EXEC_CLOSED ?

    *result = BOOL_VAL(cancelled);
    return true;
}

