#include "placed_value.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"

#include "yargobject.h"

Value getPlacedValue(PlacedValue placedStorage) {
    Value value;
    value.type = placedStorage.type;
    switch (placedStorage.type->yt) {
        case TypeDouble: value.as.dbl = *placedStorage.valuePtr.dbl; break;
        case TypeInt8: value.as.i8 = *placedStorage.valuePtr.i8; break;
        case TypeUint8: value.as.ui8 = *placedStorage.valuePtr.ui8; break;
        case TypeInt16: value.as.i16 = *placedStorage.valuePtr.i16; break;
        case TypeUint16: value.as.ui16 = *placedStorage.valuePtr.ui16; break;
        case TypeInt32: value.as.i32 = *placedStorage.valuePtr.i32; break;
        case TypeUint32: value.as.ui32 = *placedStorage.valuePtr.ui32; break;
        case TypeInt64: value.as.i64 = *placedStorage.valuePtr.i64; break;
        case TypeUint64: value.as.ui64 = *placedStorage.valuePtr.ui64; break;
        case TypeAddress: value.as.address = *placedStorage.valuePtr.address; break;

        case TypePointer:
        case TypeStruct:
        case TypeArray: {
            ValueLocation location;
            location.placedValue = placedStorage.valuePtr;
            value = createPackedValueContainerAt(location, placedStorage.type);
        }
        break;
        default: yarg_panic(); break;
    }
    return value;   
}

PlacedValue allocPlacedValue(const ObjConcreteYargType* type) {
    PlacedValue placed;
    size_t storage_required = yt_sizeof_type_storage(type);
    placed.valuePtr.address = (uintptr_t*) malloc(storage_required);
    memset(placed.valuePtr.address, 0, storage_required);
    placed.type = type;
    return placed;
}

PlacedValue createPlacedValue(uintptr_t ptr, const ObjConcreteYargType* type) {
    PlacedValue placed;
    placed.valuePtr.address = (uintptr_t*) ptr;
    placed.type = type;
    return placed;
}

void assignToPlacedValue(PlacedValueCellTarget lhs, Value rhs) {
    if (IS_I8(rhs)) {
        *lhs.valuePtr.i8 = rhs.as.i8;
    } else if (IS_UI8(rhs)) {
        *lhs.valuePtr.ui8 = rhs.as.ui8;
    } else if (IS_I16(rhs)) {
        *lhs.valuePtr.i16 = rhs.as.i16;
    } else if (IS_UI16(rhs)) {
        *lhs.valuePtr.ui16 = rhs.as.ui16;
    } else if (IS_I32(rhs)) {
        *lhs.valuePtr.i32 = rhs.as.i32;
    } else if (IS_UI32(rhs)) {
        *lhs.valuePtr.ui32 = rhs.as.ui32;
    } else if (IS_I64(rhs)) {
        *lhs.valuePtr.i64 = rhs.as.i64;
    } else if (IS_UI64(rhs)) {
        *lhs.valuePtr.ui64 = rhs.as.ui64;
    } else if (IS_ADDRESS(rhs)) {
        *lhs.valuePtr.address = rhs.as.address;
    } else if (IS_DOUBLE(rhs)) {
        *lhs.valuePtr.dbl = rhs.as.dbl;
    } else {
        // TODO: review struct, array, and pointer handling
        assert(false && "Unhandled type in assignToPlacedValueCellTarget");
        yarg_panic();
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
