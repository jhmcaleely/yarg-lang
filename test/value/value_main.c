#include <stdlib.h>
#ifdef PICO_BOARD
#include <pico/stdlib.h>
#endif

#include "value_nan.h"
#include "value_union.h"
#include "generic_size.h"

int main(int argc, const char* argv[]) {
#ifdef PICO_BOARD
    stdio_init_all();
#endif
    print_info_c();

    print_info_nan();
    print_info_union();

    return 0;
}