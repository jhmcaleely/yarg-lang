#include "yargtype.h"

#include <string.h>
#include <stdlib.h>

ObjYargType* newYargTypeFromType(YargType yt) {
    ObjYargType* type = (ObjYargType*)malloc(sizeof(ObjYargType));
    memset(type, 0, sizeof(ObjYargType));
    type->yt = yt;
    return type;
}

ObjYargTypeArray* newYargArrayTypeFromType(ObjYargType* elementType) {
    ObjYargTypeArray* arrayType = (ObjYargTypeArray*)malloc(sizeof(ObjYargTypeArray));
    memset(arrayType, 0, sizeof(ObjYargTypeArray));
    arrayType->element_type = elementType;
    return arrayType;
}