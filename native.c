#ifdef LOX_PICO_SDK
    #include "pico/stdlib.h"
#endif

#include <stdio.h>
#include <time.h>

#include "common.h"
#include "object.h"
#include "value.h"
#include "native.h"
#include "threadstack.h"
#include "vm.h"

Value clockNative(ObjThreadStack* thread, int argCount, Value* args) {
    if (argCount != 0) {
        runtimeError(thread, "Expected 0 arguments but got %d.", argCount);
    }

    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value sleepNative(ObjThreadStack* thread, int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
    }
    if (!IS_NUMBER(args[0])) {
        runtimeError(thread, "Argument must be a number");
    }

    sleep_ms(AS_NUMBER(args[0]));

    return NIL_VAL;
}

Value gpioInitNative(ObjThreadStack* thread, int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
    }
    if (!IS_NUMBER(args[0])) {
        runtimeError(thread, "Argument must be a number");
    }

    gpio_init(AS_NUMBER(args[0]));

    return NIL_VAL;
}

Value gpioSetDirectionNative(ObjThreadStack* thread, int argCount, Value* args) {
    if (argCount != 2) {
        runtimeError(thread, "Expected 2 arguments but got %d.", argCount);
    }
    if (!IS_NUMBER(args[0]) || !IS_BOOL(args[1])) {
        runtimeError(thread, "Arguments must be a number and a bool");
    }

    bool direction = AS_BOOL(args[1]);
    if (direction) {
        gpio_set_dir(AS_NUMBER(args[0]), GPIO_OUT);
    } else {
        gpio_set_dir(AS_NUMBER(args[0]), GPIO_IN);
    }

    return NIL_VAL;
}

Value gpioPutNative(ObjThreadStack* thread, int argCount, Value* args) {
    if (argCount != 2) {
        runtimeError(thread, "Expected 2 arguments but got %d.", argCount);
    }
    if (!IS_NUMBER(args[0]) || !IS_BOOL(args[1])) {
        runtimeError(thread, "Arguments must be a number and a bool");
    }

    gpio_put(AS_NUMBER(args[0]), AS_BOOL(args[1]));

    return NIL_VAL;
}

static int64_t nativeAlarmCallback(alarm_id_t id, void* user_data) {
    ObjThreadStack* thread = AS_THREAD_STACK((uintptr_t)user_data);

    printf("#nativeAlarmCallback %d", id);
    printObject(OBJ_VAL(thread));
    printf("\n");

    run(thread);

    return 0;
}

static bool nativeRepeatingCallback(struct repeating_timer* t) {
    ObjThreadStack* thread = AS_THREAD_STACK((uintptr_t)t->user_data);

    printf("#nativeRepeatingAlarmCallback %d", t->alarm_id);
    printObject(OBJ_VAL(thread));
    printf("\n");

    run(thread);

    push(thread, OBJ_VAL(thread->entryFunction));
    callfn(thread, thread->entryFunction, 0);

    return true;
}

Value alarmAddInMSNative(ObjThreadStack* thread, int argCount, Value* args) {
    if (argCount != 2) {
        runtimeError(thread, "Expected 2 arguments but got %d.", argCount);
    }
    if (!IS_NUMBER(args[0]) || !IS_THREAD_STACK(args[1])) {
        runtimeError(thread, "Argument must be a number and an isr stack.");
    }

    ObjThreadStack* isrThread = AS_THREAD_STACK(args[1]);

    add_alarm_in_ms(AS_NUMBER(args[0]), nativeAlarmCallback, isrThread, false);
}

Value alarmAddRepeatingMSNative(ObjThreadStack* thread, int argCount, Value* args) {
    if (argCount != 2) {
        runtimeError(thread, "Expected 2 arguments but got %d.", argCount);
    }
    if (!IS_NUMBER(args[0]) || !IS_THREAD_STACK(args[1])) {
        runtimeError(thread, "Argument must be a number and an isr stack.");
    }

    ObjThreadStack* isrThread = AS_THREAD_STACK(args[1]);
    ObjBlob* handle = newBlob(sizeof(repeating_timer_t));

    add_repeating_timer_ms(AS_NUMBER(args[0]), nativeRepeatingCallback, isrThread, (repeating_timer_t*)handle->blob);

    return OBJ_VAL(handle);
}

Value alarmCancelRepeatingMSNative(ObjThreadStack* thread, int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
    }
    if (!IS_BLOB(args[0])) {
        runtimeError(thread, "Argument must be a native blob.");
    }

    ObjBlob* blob = AS_BLOB(args[0]);

    bool cancelled = cancel_repeating_timer((repeating_timer_t*)blob->blob);

    return BOOL_VAL(cancelled);
}

