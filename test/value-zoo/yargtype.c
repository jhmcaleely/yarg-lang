#include "yargtype.h"
#include "yargobject.h"

#include <string.h>
#include <stdlib.h>

ObjYargType boolType = { .obj = {0}, .yt = TypeBool };
ObjYargType uint32Type = { .obj = {0}, .yt = TypeUint32 };
ObjYargType addressType = { .obj = {0}, .yt = TypeAddress };


ObjYarg* allocateYargObject(ObjYargType* type) {
    if (type->yt == TypeArray) {
        ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_PACKEDUNIFORMARRAY);
        array->core.type = type;
            // allocate memory for the array elements 
        return (ObjYarg*)array;
    }
    return NULL;
}

ObjYargType* newYargTypeFromType(YargType yt) {
    ObjYargType* type = (ObjYargType*)malloc(sizeof(ObjYargType));
    memset(type, 0, sizeof(ObjYargType));
    type->yt = yt;
    return type;
}

ObjYargTypeArray* newYargArrayTypeFromType(ObjYargType* elementType, size_t cardinality) {
    ObjYargTypeArray* arrayType = ALLOCATE_OBJ(ObjYargTypeArray, OBJ_YARGTYPE_ARRAY);
    arrayType->core.yt = TypeArray;
    arrayType->element_type = elementType;
    arrayType->cardinality = cardinality;

    return arrayType;
}