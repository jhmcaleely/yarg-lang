#include <stdio.h>
#include "value_union.h"

#include "../../cyarg/value.h"

void print_info_union() {
    printf("union: sizeof(Value) = %lu\n", sizeof(Value));
}