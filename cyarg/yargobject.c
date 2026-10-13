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
    return container_a->loc == container_b->loc;
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

Value defaultArrayValue(const ObjConcreteYargType* type) {
    ObjConcreteYargTypeArray* arrayType = (ObjConcreteYargTypeArray*) type;  
    size_t storage_required = yt_sizeof_type_storage(type);

    void* storage = malloc(storage_required);
    if (is_placeable_type(type)) {
        memset(storage, 0, storage_required);
    } else {
        Value* values = (Value*) storage;
        for (size_t i = 0; i < arrayType->cardinality; i++) {
            Value elementTypeVal = arrayType->element_type ? YARGTYPE_VAL(arrayType->element_type) : NIL_VAL;
            values[i] = defaultValue(elementTypeVal);
        }
    }

    // TODO: capture type info for later GC
    ObjPackedValueContainer* container = ALLOCATE_OBJ(ObjPackedValueContainer, OBJ_PACKEDVALUECONTAINER);
    container->loc = storage;

    return TYPED_VAL(container, type);
}

Value defaultPointerValue(const ObjConcreteYargType* type) {
    ObjConcreteYargTypePointer* pointerType = (ObjConcreteYargTypePointer*) type;  
    size_t storage_required = yt_sizeof_type_storage(type);

    void* storage = malloc(storage_required);
    if (is_placeable_type(type)) {
        memset(storage, 0, storage_required);
    } else {
        Value* values = (Value*) storage;
        Value targetTypeVal = pointerType->target_type ? YARGTYPE_VAL(pointerType->target_type) : NIL_VAL;
        *values = defaultValue(targetTypeVal);
    }

    // TODO: capture type info for later GC
    ObjPackedValueContainer* container = ALLOCATE_OBJ(ObjPackedValueContainer, OBJ_PACKEDVALUECONTAINER);
    container->loc = storage;

    return TYPED_VAL(container, type);
}

Value defaultStructValue(const ObjConcreteYargType* type) {
    ObjConcreteYargTypeStruct* structType = (ObjConcreteYargTypeStruct*) type;  
    size_t storage_required = yt_sizeof_type_storage(type);

    void* storage = malloc(storage_required);

    if (is_placeable_type(type)) {
        memset(storage, 0, storage_required);
    } else {
        uintptr_t storageBase = (uintptr_t) storage;
        // for an unplaced struct, initialize each field with its default value
        for (size_t i = 0; i < structType->field_count; i++) {
            size_t fieldOffset = structType->field_indexes[i];

            void* fieldStorage = (void*) (storageBase + fieldOffset);
            if (is_placeable_type(structType->field_types[i])) {
                memset(fieldStorage, 0, yt_sizeof_type_storage(structType->field_types[i]));
            } else {
                *(Value*)fieldStorage = defaultValue(structType->field_types[i] ? YARGTYPE_VAL(structType->field_types[i]) : NIL_VAL);
            }
        }
    }

    // TODO: capture type info for later GC
    ObjPackedValueContainer* container = ALLOCATE_OBJ(ObjPackedValueContainer, OBJ_PACKEDVALUECONTAINER);
    container->loc = storage;

    return TYPED_VAL(container, type);
}


Value createPackedValueContainerAt(void* location, const ObjConcreteYargType* type) {
    ObjPackedValueContainer* container = ALLOCATE_OBJ(ObjPackedValueContainer, OBJ_UNOWNED_PACKEDVALUECONTAINER);
    container->loc = location;
    return TYPED_VAL(container, type);
}

Value pinPackedValueContainer(Value packedValueContainer) {
    ObjPackedValueContainer* container = AS_PACKEDVALUECONTAINER(packedValueContainer);
    void* address = container->loc;
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
    const ObjConcreteYargType* element = &yargTypes.uint8;
    tempRootPush(YARGTYPE_VAL(element));
    const ObjConcreteYargType* arrayType = newYargArrayTypeFromType(element, length);
    tempRootPush(YARGTYPE_VAL(arrayType));

    Value result = defaultArrayValue(arrayType);
    tempRootPop(); // arrayType
    tempRootPop(); // element
    return result;
}

Value createByteArrayAt(uint8_t* location, size_t length) {
    const ObjConcreteYargType* element = &yargTypes.uint8;
    tempRootPush(YARGTYPE_VAL(element));
    const ObjConcreteYargType* arrayType = newYargArrayTypeFromType(element, length);
    tempRootPush(YARGTYPE_VAL(arrayType));

    Value result = createPackedValueContainerAt(location, arrayType);
    tempRootPop(); // arrayType
    tempRootPop(); // element
    return result;
}

Value allocValueArray(size_t length) {
    const ObjConcreteYargType* arrayType = newYargArrayTypeFromType(NULL, length);
    tempRootPush(YARGTYPE_VAL(arrayType));

    Value result = defaultArrayValue(arrayType);
    tempRootPop(); // arrayType
    return result;
}

void offsetPointerDestination(Value pointerVal, Value offsetVal) {
    size_t offset = as_positive_integer(offsetVal);
    ObjPackedValueContainer* pointer = AS_PACKEDVALUECONTAINER(pointerVal);
    uintptr_t addr = (uintptr_t) pointer->loc;
    addr += offset;
    pointer->loc = (void*) addr;
}

Value arrayElement(Value array, size_t index) {
    const ObjConcreteYargTypeArray* arrayType = (const ObjConcreteYargTypeArray*) array.type;
    size_t element_offset = arrayElementOffset(arrayType, index);
    ObjPackedValueContainer* container = AS_PACKEDVALUECONTAINER(array);
    uintptr_t addr = (uintptr_t) container->loc;
    addr += element_offset;

    if (is_placeable_type(array.type)) {
        PlacedValue store;
        store.value = (void*) addr;
        store.type = arrayType->element_type;
        return getPlacedValue(store);
    } else {
        return *(Value*) addr;
    }
}

Value arrayElementPointer(Value array, size_t index) {
    const ObjConcreteYargTypeArray* arrayType = (const ObjConcreteYargTypeArray*) array.type;
    size_t element_offset = arrayElementOffset(arrayType, index);
    ObjPackedValueContainer* container = AS_PACKEDVALUECONTAINER(array);
    uintptr_t addr = (uintptr_t) container->loc;
    addr += element_offset;

    const ObjConcreteYargType* ptrType = newYargPointerType(arrayType->element_type);
    tempRootPush(YARGTYPE_VAL(ptrType));

    Value result = createPackedValueContainerAt((void*) addr, ptrType);
    tempRootPop();
    return result;
}

Value structFieldPointer(Value structVal, size_t fieldIndex) {
    const ObjConcreteYargTypeStruct* structType = (const ObjConcreteYargTypeStruct*) structVal.type;
    size_t field_offset = structFieldOffset(structType, fieldIndex);
    ObjPackedValueContainer* container = AS_PACKEDVALUECONTAINER(structVal);
    uintptr_t addr = (uintptr_t) container->loc;
    addr += field_offset;

    const ObjConcreteYargType* ptrType = newYargPointerType(structType->field_types[fieldIndex]);
    tempRootPush(YARGTYPE_VAL(ptrType));

    Value result = createPackedValueContainerAt((void*) addr, ptrType);
    tempRootPop();
    return result;
}


Value pointerDestination(Value pointer) {
    const ObjConcreteYargTypePointer* pointerType = (const ObjConcreteYargTypePointer*) pointer.type;

    ObjPackedValueContainer* container = AS_PACKEDVALUECONTAINER(pointer);
    uintptr_t addr = (uintptr_t) container->loc;

    if (is_placeable_type(pointerType->target_type)) {
        PlacedValue store;
        store.value = (void*) addr;
        store.type = pointerType->target_type;
        return getPlacedValue(store);
    } else if (pointerType->target_type && pointerType->target_type->yt == TypeStruct) {
        return createPackedValueContainerAt((void*) addr, pointerType->target_type);
    } else if (pointerType->target_type && pointerType->target_type->yt == TypeArray) {
        return createPackedValueContainerAt((void*) addr, pointerType->target_type);
    } else if (pointerType->target_type && pointerType->target_type->yt == TypePointer) {
        return createPackedValueContainerAt((void*) addr, pointerType->target_type);
    } else {
        return *(Value*) addr;
    }
}

Value structField(Value structVal, size_t fieldIndex) {
    const ObjConcreteYargTypeStruct* structType = (const ObjConcreteYargTypeStruct*) structVal.type;
    size_t field_offset = structFieldOffset(structType, fieldIndex);
    ObjPackedValueContainer* container = AS_PACKEDVALUECONTAINER(structVal);
    uintptr_t addr = (uintptr_t) container->loc;
    addr += field_offset;

    if (is_placeable_type(structType->field_types[fieldIndex])) {
        PlacedValue store;
        store.value = (void*) addr;
        store.type = structType->field_types[fieldIndex];
        return getPlacedValue(store);
    } else {
        return *(Value*) addr;
    }
}

static bool setPlacedContainerElement(const ObjConcreteYargType* element_type, uintptr_t elementAddr, Value value) {
    if (is_placeable_type(element_type)) {
        PlacedValueCellTarget pvct;
        pvct.value = (void*) elementAddr;
        pvct.type = element_type;
        pvct.cellType = element_type;

        return assignToPlacedValueCellTarget(pvct, value);
    } else {
        ValueCellTarget vct = { 
            .cellType = element_type, 
            .value = (Value*) elementAddr 
        };

        return assignToValueCellTarget(vct, value);
    }
}

bool setArrayElement(Value array, size_t index, Value value) {
    const ObjConcreteYargTypeArray* arrayType = (const ObjConcreteYargTypeArray*) array.type;
    size_t element_offset = arrayElementOffset(arrayType, index);
    ObjPackedValueContainer* container = AS_PACKEDVALUECONTAINER(array);
    uintptr_t addr = (uintptr_t) container->loc;
    addr += element_offset;
    return setPlacedContainerElement(arrayType->element_type, addr, value);
}

bool setStructField(Value structVal, size_t fieldIndex, Value value) {
    const ObjConcreteYargTypeStruct* structType = (const ObjConcreteYargTypeStruct*) structVal.type;
    size_t field_offset = structFieldOffset(structType, fieldIndex);
    ObjPackedValueContainer* container = AS_PACKEDVALUECONTAINER(structVal);
    uintptr_t addr = (uintptr_t) container->loc;
    addr += field_offset;
    return setPlacedContainerElement(structType->field_types[fieldIndex], addr, value);
}

bool setPointerDestination(Value pointer, Value value) {
    const ObjConcreteYargTypePointer* pointerType = (const ObjConcreteYargTypePointer*) pointer.type;

    ObjPackedValueContainer* container = AS_PACKEDVALUECONTAINER(pointer);
    uintptr_t addr = (uintptr_t) container->loc;

    return setPlacedContainerElement(pointerType->target_type, addr, value);
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