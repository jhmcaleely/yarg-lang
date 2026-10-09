#include "placed_value.h"

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
        default: panic(); break;
    }
    return value;   
}