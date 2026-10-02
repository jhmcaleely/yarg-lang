#include "yargtype.h"
#include "yargobject.h"

#include <string.h>
#include <stdlib.h>

ObjYarg* allocateYargObject(ObjYargType* type) {
    if (type->yt == TypeArray) {
        ObjYargTypeArray* array_type = (ObjYargTypeArray*)type;

        ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_PACKEDUNIFORMARRAY);
        array->core.type = type;

        size_t element_size = storage_size_of_type(array_type->element_type->yt);

        array->elements = malloc(element_size * array_type->cardinality);
        memset(array->elements, 0, array_type->cardinality);
        return (ObjYarg*)array;
    }
    return NULL;
}