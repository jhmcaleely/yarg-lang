#include "yargtype.h"
#include "yargobject.h"

#include <string.h>
#include <stdlib.h>

#include "yarg-runtime.h"


ObjYarg* allocateYargObject(const ObjYargType* type) {
    if (type->yt == TypeArray) {
        ObjYargTypeArray* array_type = (ObjYargTypeArray*)type;

        ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_PACKEDUNIFORMARRAY);
        array->yarg.type = type;

        size_t element_size = storage_size_of_type(array_type->element_type->yt);

        array->elements = malloc(element_size * array_type->cardinality);
        memset(array->elements, 0, element_size * array_type->cardinality);
        return (ObjYarg*)array;
    } else if (type->yt == TypeBool) {
        ObjYargValue* boolean_value = ALLOCATE_OBJ(ObjYargValue, OBJ_ANYVALUE);
        boolean_value->yarg.type = type;
        return (ObjYarg*)boolean_value;
    }
    panic();
    return NULL;
}

ObjYargPointer* newYargPointerToObj(ObjYarg* target) {
    ObjYargPointer* pointer = ALLOCATE_OBJ(ObjYargPointer, OBJ_PACKEDPOINTER);
    ObjYargTypePointer* pointer_type = ALLOCATE_OBJ(ObjYargTypePointer, OBJ_YARGTYPE_POINTER);
    pointer_type->core.yt = TypePointer;
    pointer_type->target_type = target->type;
    pointer->yarg.type = (ObjYargType*)pointer_type;
    pointer->target = target;
    pointer->owner = target;
    return pointer;
}

ObjYargPointer* newYargPointerToPlaceable(const ObjYargType* targetType, void* memory) {
    ObjYargPointer* pointer = ALLOCATE_OBJ(ObjYargPointer, OBJ_PACKEDPOINTER);
    ObjYargTypePointer* pointer_type = ALLOCATE_OBJ(ObjYargTypePointer, OBJ_YARGTYPE_POINTER);
    pointer_type->core.yt = TypePointer;
    pointer_type->target_type = targetType;
    pointer->yarg.type = (ObjYargType*)pointer_type;
    pointer->target = memory;
    pointer->owner = NULL;
    return pointer;
}

void setPointerTarget(ObjYargPointer* pointer, Value newVal) {
    ObjYargTypePointer* pointer_type = (ObjYargTypePointer*)pointer->yarg.type;
    if (pointer_type->target_type->yt != newVal.type->yt) {
        panic();
    }

    switch (pointer_type->target_type->yt) {
        case TypeBool:
            *((bool*)pointer->target) = newVal.as.boolean;
            break;
        case TypeAddress:
            *((uintptr_t*)pointer->target) = newVal.as.address;
            break;
        case TypeInt8:
            *((int8_t*)pointer->target) = newVal.as.i8;
            break;
        case TypeUint8:
            *((uint8_t*)pointer->target) = newVal.as.ui8;
            break;
        case TypeInt16:
            *((int16_t*)pointer->target) = newVal.as.i16;
            break;
        case TypeUint16:
            *((uint16_t*)pointer->target) = newVal.as.ui16;
            break;
        case TypeInt32:
            *((int32_t*)pointer->target) = newVal.as.i32;
            break;
        case TypeUint32:
            *((uint32_t*)pointer->target) = newVal.as.ui32;
            break;
        case TypeInt64:
            *((int64_t*)pointer->target) = newVal.as.i64;
            break;
        case TypeUint64:
            *((uint64_t*)pointer->target) = newVal.as.ui64;
            break;
        case TypeDouble:
            *((double*)pointer->target) = newVal.as.dbl;
            break;
        default:
            panic();
    }
}

ObjYargPointer* arrayElement(ObjArray* array, size_t index) {
    ObjYargTypeArray* array_type = (ObjYargTypeArray*)array->yarg.type;
    size_t element_size = storage_size_of_type(array_type->element_type->yt);

    ObjYargPointer* pointer = ALLOCATE_OBJ(ObjYargPointer, OBJ_PACKEDPOINTER);
    ObjYargTypePointer* pointer_type = ALLOCATE_OBJ(ObjYargTypePointer, OBJ_YARGTYPE_POINTER);
    pointer_type->core.yt = TypePointer;
    pointer_type->target_type = array_type->element_type;
    pointer->yarg.type = (ObjYargType*)pointer_type;
    pointer->target = (char*)array->elements + index * element_size;
    pointer->owner = (ObjYarg*)array;

    return pointer;
}