#include "object.h"

#include <stdlib.h>
#include <string.h>

Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)malloc(size);
    memset(object, 0, size);
    object->type = type;
    return object;
}

void freeObject(Obj* object) {
    free(object);
}