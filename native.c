#ifdef LOX_PICO_SDK
    #include "pico/stdlib.h"
#endif

#include <stdio.h>
#include <time.h>

#include "common.h"
#include "object.h"
#include "value.h"
#include "native.h"
#include "vm.h"

Value clockNative(int thread, int argCount, Value* args) {
    if (argCount != 0) {
        runtimeError(thread, "Expected 0 arguments but got %d.", argCount);
    }

    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value sleepNative(int thread, int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
    }
    if (!IS_NUMBER(args[0])) {
        runtimeError(thread, "Argument must be a number");
    }

    sleep_ms(AS_NUMBER(args[0]));

    return NIL_VAL;
}

Value gpioInitNative(int thread, int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError(thread, "Expected 1 arguments but got %d.", argCount);
    }
    if (!IS_NUMBER(args[0])) {
        runtimeError(thread, "Argument must be a number");
    }

    gpio_init(AS_NUMBER(args[0]));

    return NIL_VAL;
}

Value gpioSetDirectionNative(int thread, int argCount, Value* args) {
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

Value gpioPutNative(int thread, int argCount, Value* args) {
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
    ObjClosure* closure = AS_CLOSURE((uintptr_t)user_data);

    printf("#nativeAlarmCallback %d", id);
    printObject(OBJ_VAL(closure));
    printf("\n");

    run(1);

    return 0;
}

Value alarmAddInMSNative(int thread, int argCount, Value* args) {
    if (argCount != 2) {
        runtimeError(thread, "Expected 2 arguments but got %d.", argCount);
    }
    if (!IS_NUMBER(args[0]) || !IS_CLOSURE(args[1])) {
        runtimeError(thread, "Argument must be a number and a function.");
    }

    ObjClosure* closure = AS_CLOSURE(args[1]);
    push(1, OBJ_VAL(closure));
    callfn(1, closure, 0);

    add_alarm_in_ms(AS_NUMBER(args[0]), nativeAlarmCallback, closure, false);
}
