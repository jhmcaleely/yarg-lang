#include "yargobject.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "placed-value.h"
#include "yarg-runtime.h"


Obj* allocateYargObject(const ObjYargType* type) {
    if (type->yt == TypeArray) {
        ObjYargTypeArray* array_type = (ObjYargTypeArray*)type;

        ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_PACKEDUNIFORMARRAY);
        array->yarg.type = type;
        if (array_type->element_type == NULL) {
            size_t element_size = sizeof(Value);
            array->elements = malloc(element_size * array_type->cardinality);
            memset(array->elements, 0, element_size * array_type->cardinality);            
        }
        if (is_placeable_yargtype(array_type->element_type)) {
            size_t element_size = storage_size_of_type(array_type->element_type->yt);

            array->elements = malloc(element_size * array_type->cardinality);
            memset(array->elements, 0, element_size * array_type->cardinality);
        } else {
            size_t element_size = sizeof(AnyValue);
            array->elements = malloc(element_size * array_type->cardinality);
            memset(array->elements, 0, element_size * array_type->cardinality);
        }
        return (Obj*)array;
    } else if (type->yt == TypeBool) {
        ObjValue* boolean_value = ALLOCATE_OBJ(ObjValue, OBJ_ANYVALUE);
        boolean_value->value = BOOL_VAL(false);
        return (Obj*)boolean_value;
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
    pointer->owner = (Obj*)target;
    return pointer;
}

ObjYargPointer* newYargPointerToObjVal(ObjValue* target) {
    ObjYargPointer* pointer = ALLOCATE_OBJ(ObjYargPointer, OBJ_PACKEDPOINTER);
    ObjYargTypePointer* pointer_type = ALLOCATE_OBJ(ObjYargTypePointer, OBJ_YARGTYPE_POINTER);
    pointer_type->core.yt = TypePointer;
    pointer_type->target_type = target->value.type;
    pointer->yarg.type = (ObjYargType*)pointer_type;
    pointer->target = &target->value;
    pointer->owner = (Obj*)target;
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
    if (pointer_type->target_type == NULL) {
        *(Value*)pointer->target = newVal;
        return;
    }
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

ObjLocation* arrayElement(ObjArray* array, size_t index) {
    ObjLocation* location = ALLOCATE_OBJ(ObjLocation, OBJ_LOCATION);

    ObjYargTypeArray* array_type = (ObjYargTypeArray*)array->yarg.type;
    size_t element_size = sizeof(AnyValue);
    if (array_type->element_type == NULL) {
        element_size = sizeof(Value);
    } else if (is_placeable_yargtype(array_type->element_type)) {
        assert(array_type->element_type != NULL);
        location->loc.placed = true;
        element_size = storage_size_of_yargtype(array_type->element_type);
    }
    
    if (location->loc.placed) {
        size_t offset = index * element_size;
        uintptr_t element_address = (uintptr_t)((char*)array->elements + offset);
        location->loc.placedValue = createPlacedValue(element_address, array_type->element_type);
    } else {
        size_t offset = index * element_size;
        Value* element = (Value*)((char*)array->elements + offset);
        location->loc.valuePtr = element;
        if (array_type->element_type == NULL) {
            location->loc.valuePtr->type_assignable = true;
        }
    }

    location->owner = (ObjYarg*)array;

    return location;
}

bool arraysEqual(const ObjArray* a, const ObjArray* b) {
    if (a == b) return true;
    if (a->yarg.type != b->yarg.type) return false;

    return memcmp(a->elements, b->elements, storage_size_of_yargtype(a->yarg.type)) == 0;
}

bool structsEqual(const ObjStruct* a, const ObjStruct* b) {
    if (a == b) return true;
    if (a->yarg.type != b->yarg.type) return false;

    return memcmp(a->fields, b->fields, storage_size_of_yargtype(a->yarg.type)) == 0;
}

bool typesEqual(const ObjYargType* a, const ObjYargType* b) {
    if (a == b) return true;
    switch (a->yt) {
        case TypeInt:
        case TypeInt8:
        case TypeUint8:
        case TypeInt16:
        case TypeUint16:
        case TypeInt32:
        case TypeUint32:
        case TypeInt64:
        case TypeUint64:
        case TypeDouble:
        case TypeAddress:
        case TypeClass:
            return a->yt == b->yt;
        case TypePointer: {
            ObjYargTypePointer* pointer_a = (ObjYargTypePointer*)a;
            ObjYargTypePointer* pointer_b = (ObjYargTypePointer*)b;
            return typesEqual(pointer_a->target_type, pointer_b->target_type);
        }
        case TypeArray: {
            ObjYargTypeArray* array_a = (ObjYargTypeArray*)a;
            ObjYargTypeArray* array_b = (ObjYargTypeArray*)b;
            return typesEqual(array_a->element_type, array_b->element_type) && array_a->cardinality == array_b->cardinality;
        }
        default:
            return false;
    }
    return a->yt == b->yt; // Simplistic check, may need to be expanded for complex types
}

bool pointersEqual(const ObjYargPointer* a, const ObjYargPointer* b) {
    if (a == b) return true;
    if (a->yarg.type != b->yarg.type) return false;

    return a->target == b->target;
}

bool intsEqual(const ObjInt* a, const ObjInt* b) {
    if (a == b) return true;
    if (a->yarg.type != b->yarg.type) return false;

    return a->value == b->value;
}