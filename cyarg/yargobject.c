#include "yargobject.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "memory.h"
#include "yargstructtype.h"

bool arraysEqual(Value a, Value b) {
    if (!typesEqual(a.type, b.type)) return false;
    if (a.as.obj == b.as.obj) return true;
    return false;
}

bool structsEqual(Value a, Value b) {
    if (!typesEqual(a.type, b.type)) return false;
    if (a.as.obj == b.as.obj) return true;
    return false;
}

bool typesEqual(const ObjConcreteYargType* a, const ObjConcreteYargType* b) {
    if (a == b) return true;
    if (a != NULL) {
        if (b == NULL) return false;
        if (a->yt != b->yt) return false;
        if (a->yt == TypePointer) {
            assert(b->yt == TypePointer);
            const ObjConcreteYargTypePointer* pointer_a = (const ObjConcreteYargTypePointer*)a;
            const ObjConcreteYargTypePointer* pointer_b = (const ObjConcreteYargTypePointer*)b;
            return typesEqual(pointer_a->target_type, pointer_b->target_type);
        }
        if (a->yt == TypeArray) {
            assert(b->yt == TypeArray);
            const ObjConcreteYargTypeArray* array_a = (const ObjConcreteYargTypeArray*)a;
            const ObjConcreteYargTypeArray* array_b = (const ObjConcreteYargTypeArray*)b;
            if (!typesEqual(array_a->element_type, array_b->element_type)) return false;
            return array_a->cardinality == array_b->cardinality;
        }
        // TODO: structs, maps, other complex types
        return a->yt == b->yt;
    }
    return false;
}

bool pointersEqual(Value a, Value b) {
    if (!typesEqual(a.type, b.type)) return false;
    ObjPackedValueContainer* container_a = AS_PACKEDVALUECONTAINER(a);
    ObjPackedValueContainer* container_b = AS_PACKEDVALUECONTAINER(b);
    if (container_a->location.placed && container_b->location.placed) {
        return container_a->location.placedValue.address == container_b->location.placedValue.address;
    }
    if (!container_a->location.placed && !container_b->location.placed) {
        return container_a->location.value == container_b->location.value;
    }

    return false;
}

bool intsEqual(const ObjInt* a, const ObjInt* b) {
    if (a == b) return true;

    return int_is(&a->bigInt, &b->bigInt) == INT_EQ;
}


bool is_uniformarray(Value value) {
    if (value.type == NULL) return false;
    return value.type->yt == TypeArray;
}
bool is_pointer(Value value) {
    if (value.type == NULL) return false;
    return value.type->yt == TypePointer;
}

bool is_struct(Value value) {
    if (value.type == NULL) return false;
    return value.type->yt == TypeStruct;
}

Value defaultPackedValueContainerValue(const ObjConcreteYargType* type) {
    size_t storage_required = yt_sizeof_type_storage(type);
    bool placed = is_placeable_type(type);
    ValueLocation location;
    location.placed = placed;
    if (placed) {
        location.placedValue.address = (uintptr_t*) malloc(storage_required);
        memset(location.placedValue.address, 0, storage_required);
    } else {
        location.value = (Value*) malloc(storage_required);
        memset(location.value, 0, storage_required);
    }
    // TODO: capture type info for later GC
    ObjPackedValueContainer* container = ALLOCATE_OBJ(ObjPackedValueContainer, OBJ_PACKEDVALUECONTAINER);
    container->location = location;
    return TYPED_VAL(container, type);
}


Value createPackedValueContainerAt(ValueLocation location, const ObjConcreteYargType* type) {
    ObjPackedValueContainer* container = ALLOCATE_OBJ(ObjPackedValueContainer, OBJ_UNOWNED_PACKEDVALUECONTAINER);
    container->location = location;
    return TYPED_VAL(container, type);
}

Value pinPackedValueContainer(Value packedValueContainer) {
    ObjPackedValueContainer* container = AS_PACKEDVALUECONTAINER(packedValueContainer);
    void* address = container->location.placed ? (void*) container->location.placedValue.address : (void*) container->location.value;
    return ADDRESS_VAL((uintptr_t) address);
}

bool  isByteArray(Value value) {
    if (!is_uniformarray(value)) return false;
    const ObjConcreteYargTypeArray* arrayType = (const ObjConcreteYargTypeArray*) value.type;
    if (arrayType->element_type) {
        return arrayType->element_type->yt == TypeUint8;
    }
    return false;
}

Value allocByteArray(size_t length) {
    const ObjConcreteYargType* element = newYargTypeFromType(TypeUint8);
    tempRootPush(YARGTYPE_VAL(element));
    const ObjConcreteYargType* arrayType = newYargArrayTypeFromType(element, length);
    tempRootPush(YARGTYPE_VAL(arrayType));

    Value result = defaultPackedValueContainerValue(arrayType);
    tempRootPop(); // arrayType
    tempRootPop(); // element
    return result;
}

Value createByteArrayAt(uint8_t* location, size_t length) {
    const ObjConcreteYargType* element = newYargTypeFromType(TypeUint8);
    tempRootPush(YARGTYPE_VAL(element));
    const ObjConcreteYargType* arrayType = newYargArrayTypeFromType(element, length);
    tempRootPush(YARGTYPE_VAL(arrayType));
    ValueLocation valLocation;
    valLocation.placed = true;
    valLocation.placedValue.ui8 = location;

    Value result = createPackedValueContainerAt(valLocation, arrayType);
    tempRootPop(); // arrayType
    tempRootPop(); // element
    return result;
}

Value allocValueArray(size_t length) {
    const ObjConcreteYargType* arrayType = newYargArrayTypeFromType(NULL, length);
    tempRootPush(YARGTYPE_VAL(arrayType));

    Value result = defaultPackedValueContainerValue(arrayType);
    tempRootPop(); // arrayType
    return result;
}

void offsetPointerDestination(Value pointerVal, Value offsetVal) {
    size_t offset = as_positive_integer(offsetVal);
    ObjPackedValueContainer* pointer = AS_PACKEDVALUECONTAINER(pointerVal);
    void* dest = pointer->location.placed ? (void*) pointer->location.placedValue.address : (void*) pointer->location.value;
    uintptr_t addr = (uintptr_t) dest;
    addr += offset;
    if (pointer->location.placed) {
        pointer->location.placedValue.address = (uintptr_t*) addr;
    } else {
        pointer->location.value = (Value*) addr;
    }
}

Value arrayElement(Value array, size_t index) {
    const ObjConcreteYargTypeArray* arrayType = (const ObjConcreteYargTypeArray*) array.type;
    size_t element_offset = arrayElementOffset(arrayType, index);
    ObjPackedValueContainer* container = AS_PACKEDVALUECONTAINER(array);
    void* base = container->location.placed ? (void*) container->location.placedValue.address : (void*) container->location.value;
    uintptr_t addr = (uintptr_t) base;
    addr += element_offset;

    if (is_placeable_type(array.type)) {
        PlacedValue store;
        store.valuePtr.address = (uintptr_t*) addr;
        store.type = arrayType->element_type;
        return getPlacedValue(store);
    } else {
        return *(Value*) addr;
    }
}

Value pointerDestination(Value pointer) {
    const ObjConcreteYargTypePointer* pointerType = (const ObjConcreteYargTypePointer*) pointer.type;

    ObjPackedValueContainer* container = AS_PACKEDVALUECONTAINER(pointer);
    void* base = container->location.placed ? (void*) container->location.placedValue.address : (void*) container->location.value;
    uintptr_t addr = (uintptr_t) base;

    if (is_placeable_type(pointer.type)) {
        PlacedValue store;
        store.valuePtr.address = (uintptr_t*) addr;
        store.type = pointerType->target_type;
        return getPlacedValue(store);
    } else {
        return *(Value*) addr;
    }
}

Value structField(Value structVal, size_t fieldIndex) {
    const ObjConcreteYargTypeStruct* structType = (const ObjConcreteYargTypeStruct*) structVal.type;
    size_t field_offset = structFieldOffset(structType, fieldIndex);
    ObjPackedValueContainer* container = AS_PACKEDVALUECONTAINER(structVal);
    void* base = container->location.placed ? (void*) container->location.placedValue.address : (void*) container->location.value;
    uintptr_t addr = (uintptr_t) base;
    addr += field_offset;

    if (is_placeable_type(structVal.type)) {
        PlacedValue store;
        store.valuePtr.address = (uintptr_t*) addr;
        store.type = structType->field_types[fieldIndex];
        return getPlacedValue(store);
    } else {
        return *(Value*) addr;
    }
}

bool isArrayPointer(Value value) {
    if (!IS_POINTER(value)) return false;
    const ObjConcreteYargTypePointer* pointerType = (const ObjConcreteYargTypePointer*) value.type;
    if (pointerType->target_type == NULL) return false;
    return pointerType->target_type->yt == TypeArray;
}

bool isStructPointer(Value value) {
    if (!IS_POINTER(value)) return false;
    const ObjConcreteYargTypePointer* pointerType = (const ObjConcreteYargTypePointer*) value.type;
    if (pointerType->target_type == NULL) return false;
    return pointerType->target_type->yt == TypeStruct;
}