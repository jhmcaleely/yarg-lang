
#ifdef CLOX_PICO_TARGET
#include <pico/stdlib.h>
#endif

#include "platform_hal.h"

void plaform_hal_init() {
#ifdef CLOX_PICO_TARGET
    stdio_init_all();
#endif
}