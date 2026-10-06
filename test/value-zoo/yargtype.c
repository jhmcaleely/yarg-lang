#include "yargtype.h"
#include "yargobject.h"

#include "yarg-runtime.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

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
    .uint64 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeUint64 },
    .type = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeYargType },
    .class_ = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeClass },
};

bool is_value_type(YargType yt) {
    switch (yt) {
    case TypeBool:
    case TypeInt:
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
    case TypeArray:
    case TypeStruct:
        return true;
    default:
        return false;
    }
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
    case TypeArray:
    case TypeStruct:
        panic(); return false;
    default:
        return false;
    }
}

bool is_container_type(YargType yt) {
    switch (yt) {
    case TypeArray:
    case TypeStruct:
    case TypeMap:
    case TypeChannel:
    case TypeRoutine:
    case TypePointer:
        return true;
    default:
        return false;
    }
}

bool is_parameterised_type(YargType yt) {
    switch (yt) {
    case TypePointer:
    case TypeInstance:
    case TypeFunction:
    case TypeRoutine:
    case TypeChannel:
    case TypeMap:
    case TypeArray:
    case TypeStruct:
        return true;
    default:
        return false;
    }
}

bool is_obj_type(YargType yt) {
    switch (yt) {
    case TypeInt:
    case TypePointer:
    case TypeClass:
    case TypeInstance:
    case TypeFunction:
    case TypeRoutine:
    case TypeChannel:
    case TypeMap:
    case TypeYargType:
    case TypeString:
    case TypeArray:
    case TypeStruct:
        return true;
    default:
        return false;
    }
}

bool is_nil_assignable(YargType yt) {
    return !is_placeable_type(yt);
}

const ObjYargType* newYargTypeFromType(YargType yt) {
    switch (yt) {
    case TypeBool: return &yargTypes.boolean;
    case TypeDouble: return &yargTypes.dbl;
    case TypeInt8: return &yargTypes.int8;
    case TypeUint8: return &yargTypes.uint8;
    case TypeInt16: return &yargTypes.int16;
    case TypeUint16: return &yargTypes.uint16;
    case TypeInt32: return &yargTypes.int32;
    case TypeUint32: return &yargTypes.uint32;
    case TypeInt64: return &yargTypes.int64;
    case TypeUint64: return &yargTypes.uint64;
    case TypeAddress: return &yargTypes.address;
    case TypeYargType: return &yargTypes.type;
    default:
        // parameterised types should not be created with this function
        assert(is_parameterised_type(yt));
        panic(); return NULL;
    }
}


bool is_placeable_yargtype(const ObjYargType* type) {
    assert(type != NULL);
    if (type->yt == TypeArray) {
        ObjYargTypeArray* arrayType = (ObjYargTypeArray*)type;
        return is_placeable_yargtype(arrayType->element_type);
    } else {
        return is_placeable_type(type->yt);
    }
}

bool is_container_yargtype(const ObjYargType* type) {
    assert(type != NULL);
    return is_container_type(type->yt);
}

bool is_parameterised_yargtype(const ObjYargType* type) {
    assert(type != NULL);
    return is_parameterised_type(type->yt);
}

bool is_value_yargtype(const ObjYargType* type) {
    assert(type != NULL);
    return is_value_type(type->yt);
}

bool is_obj_yargtype(const ObjYargType* type) {
    assert(type != NULL);
    return is_obj_type(type->yt);
}

bool is_nil_assignable_yargtype(const ObjYargType* type) {
    if (type == NULL) return true;

    return is_nil_assignable(type->yt);
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
    default: panic(); return 0;
    }
}

size_t storage_size_of_yargtype(const ObjYargType* type) {
    if (type->yt == TypeArray) {
        ObjYargTypeArray* arrayType = (ObjYargTypeArray*)type;
        return storage_size_of_yargtype(arrayType->element_type) * arrayType->cardinality;
    } else {
        return storage_size_of_type(type->yt);
    }
}

const ObjYargTypeArray* newYargArrayTypeFromType(const ObjYargType* elementType, size_t cardinality) {
    ObjYargTypeArray* arrayType = ALLOCATE_OBJ(ObjYargTypeArray, OBJ_YARGTYPE_ARRAY);
    arrayType->core.yt = TypeArray;
    arrayType->element_type = elementType;
    arrayType->cardinality = cardinality;

    return arrayType;
}