#include "object.h"

#include "yargtype.h"

#include <stdlib.h>
#include <string.h>

Obj* allocateYargObject(ObjYargType* type) {
    if (type->yt == TypeArray) {
        ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_PACKEDUNIFORMARRAY);
        return (Obj*)array;
    }
    return NULL;
}

Obj* allocateImplementationObject(size_t size, ObjType type) {
    Obj* object = (Obj*)malloc(size);
    memset(object, 0, size);
    object->type = type;
    return object;
}
