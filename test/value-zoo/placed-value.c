#include "placed-value.h"

#include <stdlib.h>
#include <sysexits.h>

#include "yarg-runtime.h"

static void initialisePlacedValue(PlacedValue placedStorage) {
    if (placedStorage.type) {
        switch (placedStorage.type->yt) {
        case TypeDouble: *placedStorage.valuePtr.dbl = 0.0; break;
            case TypeInt8: *placedStorage.valuePtr.i8 = 0; break;
            case TypeUint8: *placedStorage.valuePtr.ui8 = 0; break;
            case TypeInt16: *placedStorage.valuePtr.i16 = 0; break;
            case TypeUint16: *placedStorage.valuePtr.ui16 = 0; break;
            case TypeInt32: *placedStorage.valuePtr.i32 = 0; break;
            case TypeUint32: *placedStorage.valuePtr.ui32 = 0; break;
            case TypeInt64: *placedStorage.valuePtr.i64 = 0; break;
            case TypeUint64: *placedStorage.valuePtr.ui64 = 0; break;
            case TypeAddress: *placedStorage.valuePtr.address = 0; break;
            default: panic(); break;
        }
    } else {
        panic();
    }
}

PlacedValue createPlacedValue(uintptr_t ptr, const ObjYargType* type) {
    PlacedValue placedStorage;
    placedStorage.type = type;
    switch (placedStorage.type->yt) {
        case TypeDouble: placedStorage.valuePtr.dbl = (double*)ptr; break;
        case TypeInt8: placedStorage.valuePtr.i8 = (int8_t*)ptr; break;
        case TypeUint8: placedStorage.valuePtr.ui8 = (uint8_t*)ptr; break;
        case TypeInt16: placedStorage.valuePtr.i16 = (int16_t*)ptr; break;
        case TypeUint16: placedStorage.valuePtr.ui16 = (uint16_t*)ptr; break;
        case TypeInt32: placedStorage.valuePtr.i32 = (int32_t*)ptr; break;
        case TypeUint32: placedStorage.valuePtr.ui32 = (uint32_t*)ptr; break;
        case TypeInt64: placedStorage.valuePtr.i64 = (int64_t*)ptr; break;
        case TypeUint64: placedStorage.valuePtr.ui64 = (uint64_t*)ptr; break;
        case TypeAddress: placedStorage.valuePtr.address = (uintptr_t*)ptr; break;
        default: panic(); break;
    }
    return placedStorage;
} 

PlacedValue allocPlacedValue(const ObjYargType* type) {
    PlacedValue placedStorage;
    placedStorage.type = type;
    switch (type->yt) {
        case TypeDouble: placedStorage.valuePtr.dbl = (double*)malloc(sizeof(double)); break;
        case TypeInt8: placedStorage.valuePtr.i8 = (int8_t*)malloc(sizeof(int8_t)); break;
        case TypeUint8: placedStorage.valuePtr.ui8 = (uint8_t*)malloc(sizeof(uint8_t)); break;
        case TypeInt16: placedStorage.valuePtr.i16 = (int16_t*)malloc(sizeof(int16_t)); break;
        case TypeUint16: placedStorage.valuePtr.ui16 = (uint16_t*)malloc(sizeof(uint16_t)); break;
        case TypeInt32: placedStorage.valuePtr.i32 = (int32_t*)malloc(sizeof(int32_t)); break;
        case TypeUint32: placedStorage.valuePtr.ui32 = (uint32_t*)malloc(sizeof(uint32_t)); break;
        case TypeInt64: placedStorage.valuePtr.i64 = (int64_t*)malloc(sizeof(int64_t)); break;
        case TypeUint64: placedStorage.valuePtr.ui64 = (uint64_t*)malloc(sizeof(uint64_t)); break;
        case TypeAddress: placedStorage.valuePtr.address = (uintptr_t*)malloc(sizeof(uintptr_t)); break;
        default: panic(); break;
    }
    initialisePlacedValue(placedStorage);
    return placedStorage;
}

void freePlacedValue(PlacedValue placedStorage) {
    switch (placedStorage.type->yt) {
        case TypeDouble: free(placedStorage.valuePtr.dbl); break;
        case TypeInt8: free(placedStorage.valuePtr.i8); break;
        case TypeUint8: free(placedStorage.valuePtr.ui8); break;
        case TypeInt16: free(placedStorage.valuePtr.i16); break;
        case TypeUint16: free(placedStorage.valuePtr.ui16); break;
        case TypeInt32: free(placedStorage.valuePtr.i32); break;
        case TypeUint32: free(placedStorage.valuePtr.ui32); break;
        case TypeInt64: free(placedStorage.valuePtr.i64); break;
        case TypeUint64: free(placedStorage.valuePtr.ui64); break;
        case TypeAddress: free(placedStorage.valuePtr.address); break;
        default: panic(); break;
    }
}

void placeValue(PlacedValue placedStorage, Value value) {

    if (placedStorage.type->yt == TypeDouble && IS_DOUBLE(value)) {
        *placedStorage.valuePtr.dbl = value.as.dbl;
    } else if (placedStorage.type->yt == TypeInt8 && IS_I8(value)) {
        *placedStorage.valuePtr.i8 = value.as.i8;
    } else if (placedStorage.type->yt == TypeUint8 && IS_UI8(value)) {
        *placedStorage.valuePtr.ui8 = value.as.ui8;
    } else if (placedStorage.type->yt == TypeInt16 && IS_I16(value)) {
        *placedStorage.valuePtr.i16 = value.as.i16;
    } else if (placedStorage.type->yt == TypeUint16 && IS_UI16(value)) {
        *placedStorage.valuePtr.ui16 = value.as.ui16;
    } else if (placedStorage.type->yt == TypeInt32 && IS_I32(value)) {
        *placedStorage.valuePtr.i32 = value.as.i32;
    } else if (placedStorage.type->yt == TypeUint32 && IS_UI32(value)) {
        *placedStorage.valuePtr.ui32 = value.as.ui32;
    } else if (placedStorage.type->yt == TypeInt64 && IS_I64(value)) {
        *placedStorage.valuePtr.i64 = value.as.i64;
    } else if (placedStorage.type->yt == TypeUint64 && IS_UI64(value)) {
        *placedStorage.valuePtr.ui64 = value.as.ui64;
    } else if (placedStorage.type->yt == TypeAddress && IS_ADDRESS(value)) {
        *placedStorage.valuePtr.address = value.as.address;
    } else {
        panic();
    }
}

Value getPlacedValue(PlacedValue packedStorage) {
    Value value;
    value.type = packedStorage.type;
    switch (packedStorage.type->yt) {
        case TypeDouble: value.as.dbl = *packedStorage.valuePtr.dbl; break;
        case TypeInt8: value.as.i8 = *packedStorage.valuePtr.i8; break;
        case TypeUint8: value.as.ui8 = *packedStorage.valuePtr.ui8; break;
        case TypeInt16: value.as.i16 = *packedStorage.valuePtr.i16; break;
        case TypeUint16: value.as.ui16 = *packedStorage.valuePtr.ui16; break;
        case TypeInt32: value.as.i32 = *packedStorage.valuePtr.i32; break;
        case TypeUint32: value.as.ui32 = *packedStorage.valuePtr.ui32; break;
        case TypeInt64: value.as.i64 = *packedStorage.valuePtr.i64; break;
        case TypeUint64: value.as.ui64 = *packedStorage.valuePtr.ui64; break;
        case TypeAddress: value.as.address = *packedStorage.valuePtr.address; break;
        default: panic(); break;
    }
    return value;
}