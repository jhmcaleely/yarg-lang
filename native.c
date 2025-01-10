#ifdef LOX_PICO_SDK
    #include "pico/stdlib.h"
#endif

#include <time.h>

#include "common.h"
#include "native.h"

Value clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}