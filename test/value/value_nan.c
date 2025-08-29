#include <stdio.h>
#include "value_nan.h"

#define NAN_BOXING

#include "../../cyarg/value.h"

void print_info_nan() {
    printf("nan_box: sizeof(Value) = %lu\n", sizeof(Value));
}