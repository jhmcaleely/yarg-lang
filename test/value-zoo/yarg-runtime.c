#include "yarg-runtime.h"

#include <stdlib.h>
#include <assert.h>

void panic(void) {
    assert(0);
    exit(EXIT_FAILURE);
}