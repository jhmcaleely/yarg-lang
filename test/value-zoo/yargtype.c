#include "yargtype.h"
#include "yargobject.h"

#include <string.h>
#include <stdlib.h>

const SimpleYargTypes yargTypes = {
    .boolean = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeBool },
    .address = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeAddress },
    .dbl = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeDouble },
    .int8 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeInt8 },
    .uint8 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeUint8 },
    .int16 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeInt16 },
    .uint16 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeUint16 },
    .int32 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeInt32 },
    .uint32 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeUint32 },
    .int64 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeInt64 },
    .uint64 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeUint64 }
};

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

bool is_placeable_type(YargType yt) {
    switch (yt) {
    case TypeDouble:
    case TypeInt8:
    case TypeUint8:
    case TypeInt16:
    case TypeUint16:
    case TypeInt32:
    case TypeUint32:
    case TypeInt64:
    case TypeUint64:
    case TypeAddress:
        return true;
    default:
        return false;
    }
}

size_t storage_size_of_type(YargType yt) {
    switch (yt) {
    case TypeDouble: return sizeof(double);
    case TypeInt8: return sizeof(int8_t);
    case TypeUint8: return sizeof(uint8_t);
    case TypeInt16: return sizeof(int16_t);
    case TypeUint16: return sizeof(uint16_t);
    case TypeInt32: return sizeof(int32_t);
    case TypeUint32: return sizeof(uint32_t);
    case TypeInt64: return sizeof(int64_t);
    case TypeUint64: return sizeof(uint64_t);
    case TypeAddress: return sizeof(uintptr_t);
    default:
        return 0;
    }
}