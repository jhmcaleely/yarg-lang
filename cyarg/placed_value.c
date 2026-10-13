#include "placed_value.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"

#include "yargtype.h"
#include "value_cell.h"

Value getPlacedValue(PlacedValue placedStorage) {
    
    if (is_placeable_type(placedStorage.type)) {
        Value value = NIL_VAL;
        value.type = placedStorage.type;
        if (placedStorage.type == &yargTypes.dbl) {
            value.as.dbl = *(double*) placedStorage.value;
        } else if (placedStorage.type == &yargTypes.int8) {
            value.as.i8 = *(int8_t*) placedStorage.value;
        } else if (placedStorage.type == &yargTypes.uint8) {
            value.as.ui8 = *(uint8_t*) placedStorage.value;
        } else if (placedStorage.type == &yargTypes.int16) {
            value.as.i16 = *(int16_t*) placedStorage.value;
        } else if (placedStorage.type == &yargTypes.uint16) {
            value.as.ui16 = *(uint16_t*) placedStorage.value;
        } else if (placedStorage.type == &yargTypes.int32) {
            value.as.i32 = *(int32_t*) placedStorage.value;
        } else if (placedStorage.type == &yargTypes.uint32) {
            value.as.ui32 = *(uint32_t*) placedStorage.value;
        } else if (placedStorage.type == &yargTypes.int64) {
            value.as.i64 = *(int64_t*) placedStorage.value;
        } else if (placedStorage.type == &yargTypes.uint64) {
            value.as.ui64 = *(uint64_t*) placedStorage.value;
        } else if (placedStorage.type == &yargTypes.address) {
            value.as.address = *(uintptr_t*) placedStorage.value;
        } else {
            fatalVMError("Unsupported placeable type: %s", placedStorage.type->yt);
        }
        return value;
    } else {
        fatalVMError("Unsupported non-placeable type: %s", placedStorage.type->yt);
    }
    return NIL_VAL;
}

void setDefaultPlacedValue(PlacedValue value) {
    assert(is_placeable_type(value.type));
    size_t storage_size = yt_sizeof_type_storage(value.type);
    memset(value.value, 0, storage_size);
}

void assignToPlacedValue(PlacedValueCellTarget lhs, Value rhs) {
    if (IS_I8(rhs)) {
        *(int8_t*)lhs.value = rhs.as.i8;
    } else if (IS_UI8(rhs)) {
        *(uint8_t*)lhs.value = rhs.as.ui8;
    } else if (IS_I16(rhs)) {
        *(int16_t*)lhs.value = rhs.as.i16;
    } else if (IS_UI16(rhs)) {
        *(uint16_t*)lhs.value = rhs.as.ui16;
    } else if (IS_I32(rhs)) {
        *(int32_t*)lhs.value = rhs.as.i32;
    } else if (IS_UI32(rhs)) {
        *(uint32_t*)lhs.value = rhs.as.ui32;
    } else if (IS_I64(rhs)) {
        *(int64_t*)lhs.value = rhs.as.i64;
    } else if (IS_UI64(rhs)) {
        *(uint64_t*)lhs.value = rhs.as.ui64;
    } else if (IS_ADDRESS(rhs)) {
        *(uintptr_t*)lhs.value = rhs.as.address;
    } else if (IS_DOUBLE(rhs)) {
        *(double*)lhs.value = rhs.as.dbl;
    } else {
        // TODO: review struct, array, and pointer handling
        fatalVMError("Unhandled type in assignToPlacedValueCellTarget");
    }
}

bool assignToPlacedValueCellTarget(PlacedValueCellTarget lhs, Value rhsValue) {
    assert(lhs.cellType != NULL);

    Value promoted;
    if (isInitialisableType(lhs.cellType, rhsValue, &promoted)) {
        assert(promoted.type != NULL || rhsValue.type != NULL);

        if (promoted.type == NULL) {
            noLongerLiteralInt(&rhsValue);
            assert(is_placeable_type(rhsValue.type));
            assignToPlacedValue(lhs, rhsValue);
        } else {
            assert(is_placeable_type(promoted.type));
            assignToPlacedValue(lhs, promoted);
        }

        return true;
    } else {
        return false;
    }
}
