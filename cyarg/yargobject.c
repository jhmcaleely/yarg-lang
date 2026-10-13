#include "yargobject.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "memory.h"
#include "yargstructtype.h"
#include "packed_containers.h"

bool intsEqual(const ObjInt* a, const ObjInt* b) {
    if (a == b) return true;

    return int_is(&a->bigInt, &b->bigInt) == INT_EQ;
}
