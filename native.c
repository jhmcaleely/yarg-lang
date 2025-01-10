#ifdef LOX_PICO_SDK
    #include "pico/stdlib.h"
#endif

#include <time.h>

#include "common.h"
#include "native.h"
#include "vm.h"

Value clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value sleepNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 0 arguments but got %d.", argCount);
    }
    if (!IS_NUMBER(args[0])) {
        runtimeError("Argument must be a number");
    }

    sleep_ms(AS_NUMBER(args[0]));

    return NIL_VAL;
}