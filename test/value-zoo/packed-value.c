#include "packed-value.h"

#include "yargtype.h"
#include "yarg-runtime.h"

#include <stdlib.h>
#include <sysexits.h>

void initialisePackedValue(PackedValue packedValue) {
    if (packedValue.type) {
        switch (packedValue.type->yt) {
        case TypeDouble: *packedValue.valuePtr.dbl = 0.0; break;
            case TypeInt8: *packedValue.valuePtr.i8 = 0; break;
            case TypeUint8: *packedValue.valuePtr.ui8 = 0; break;
            case TypeInt16: *packedValue.valuePtr.i16 = 0; break;
            case TypeUint16: *packedValue.valuePtr.ui16 = 0; break;
            case TypeInt32: *packedValue.valuePtr.i32 = 0; break;
            case TypeUint32: *packedValue.valuePtr.ui32 = 0; break;
            case TypeInt64: *packedValue.valuePtr.i64 = 0; break;
            case TypeUint64: *packedValue.valuePtr.ui64 = 0; break;
            case TypeAddress: *packedValue.valuePtr.uiptr = 0; break;
            default: panic(); break;
        }
    } else {
        panic();
    }
}

PackedValue createPackedValue(uintptr_t ptr, const ObjYargType* type) {
    PackedValue packedValue;
    packedValue.type = type;
    switch (packedValue.type->yt) {
        case TypeDouble: packedValue.valuePtr.dbl = (double*)ptr; break;
        case TypeInt8: packedValue.valuePtr.i8 = (int8_t*)ptr; break;
        case TypeUint8: packedValue.valuePtr.ui8 = (uint8_t*)ptr; break;
        case TypeInt16: packedValue.valuePtr.i16 = (int16_t*)ptr; break;
        case TypeUint16: packedValue.valuePtr.ui16 = (uint16_t*)ptr; break;
        case TypeInt32: packedValue.valuePtr.i32 = (int32_t*)ptr; break;
        case TypeUint32: packedValue.valuePtr.ui32 = (uint32_t*)ptr; break;
        case TypeInt64: packedValue.valuePtr.i64 = (int64_t*)ptr; break;
        case TypeUint64: packedValue.valuePtr.ui64 = (uint64_t*)ptr; break;
        case TypeAddress: packedValue.valuePtr.uiptr = (uintptr_t*)ptr; break;
        default: panic(); break;
    }
    return packedValue;
} 

PackedValue allocPackedValue(const ObjYargType* type) {
    PackedValue packedValue;
    packedValue.type = type;
    switch (type->yt) {
        case TypeDouble: packedValue.valuePtr.dbl = (double*)malloc(sizeof(double)); break;
        case TypeInt8: packedValue.valuePtr.i8 = (int8_t*)malloc(sizeof(int8_t)); break;
        case TypeUint8: packedValue.valuePtr.ui8 = (uint8_t*)malloc(sizeof(uint8_t)); break;
        case TypeInt16: packedValue.valuePtr.i16 = (int16_t*)malloc(sizeof(int16_t)); break;
        case TypeUint16: packedValue.valuePtr.ui16 = (uint16_t*)malloc(sizeof(uint16_t)); break;
        case TypeInt32: packedValue.valuePtr.i32 = (int32_t*)malloc(sizeof(int32_t)); break;
        case TypeUint32: packedValue.valuePtr.ui32 = (uint32_t*)malloc(sizeof(uint32_t)); break;
        case TypeInt64: packedValue.valuePtr.i64 = (int64_t*)malloc(sizeof(int64_t)); break;
        case TypeUint64: packedValue.valuePtr.ui64 = (uint64_t*)malloc(sizeof(uint64_t)); break;
        case TypeAddress: packedValue.valuePtr.uiptr = (uintptr_t*)malloc(sizeof(uintptr_t)); break;
        default: panic(); break;
    }
    initialisePackedValue(packedValue);
    return packedValue;
}

void freePackedValue(PackedValue packedValue) {
    switch (packedValue.type->yt) {
        case TypeDouble: free(packedValue.valuePtr.dbl); break;
        case TypeInt8: free(packedValue.valuePtr.i8); break;
        case TypeUint8: free(packedValue.valuePtr.ui8); break;
        case TypeInt16: free(packedValue.valuePtr.i16); break;
        case TypeUint16: free(packedValue.valuePtr.ui16); break;
        case TypeInt32: free(packedValue.valuePtr.i32); break;
        case TypeUint32: free(packedValue.valuePtr.ui32); break;
        case TypeInt64: free(packedValue.valuePtr.i64); break;
        case TypeUint64: free(packedValue.valuePtr.ui64); break;
        case TypeAddress: free(packedValue.valuePtr.uiptr); break;
        default: panic(); break;
    }
}

void packValue(PackedValue packedStorageTarget, Value value) {

    if (packedStorageTarget.type->yt == TypeDouble && IS_DOUBLE(value)) {
        *packedStorageTarget.valuePtr.dbl = value.as.dbl;
    } else if (packedStorageTarget.type->yt == TypeInt8 && IS_I8(value)) {
        *packedStorageTarget.valuePtr.i8 = value.as.i8;
    } else if (packedStorageTarget.type->yt == TypeUint8 && IS_UI8(value)) {
        *packedStorageTarget.valuePtr.ui8 = value.as.ui8;
    } else if (packedStorageTarget.type->yt == TypeInt16 && IS_I16(value)) {
        *packedStorageTarget.valuePtr.i16 = value.as.i16;
    } else if (packedStorageTarget.type->yt == TypeUint16 && IS_UI16(value)) {
        *packedStorageTarget.valuePtr.ui16 = value.as.ui16;
    } else if (packedStorageTarget.type->yt == TypeInt32 && IS_I32(value)) {
        *packedStorageTarget.valuePtr.i32 = value.as.i32;
    } else if (packedStorageTarget.type->yt == TypeUint32 && IS_UI32(value)) {
        *packedStorageTarget.valuePtr.ui32 = value.as.ui32;
    } else if (packedStorageTarget.type->yt == TypeInt64 && IS_I64(value)) {
        *packedStorageTarget.valuePtr.i64 = value.as.i64;
    } else if (packedStorageTarget.type->yt == TypeUint64 && IS_UI64(value)) {
        *packedStorageTarget.valuePtr.ui64 = value.as.ui64;
    } else if (packedStorageTarget.type->yt == TypeAddress && IS_ADDRESS(value)) {
        *packedStorageTarget.valuePtr.uiptr = value.as.address;
    } else {
        panic();
    }
}

Value unpackValue(PackedValue packedValue) {
    Value value;
    value.type = packedValue.type;
    switch (packedValue.type->yt) {
        case TypeDouble: value.as.dbl = *packedValue.valuePtr.dbl; break;
        case TypeInt8: value.as.i8 = *packedValue.valuePtr.i8; break;
        case TypeUint8: value.as.ui8 = *packedValue.valuePtr.ui8; break;
        case TypeInt16: value.as.i16 = *packedValue.valuePtr.i16; break;
        case TypeUint16: value.as.ui16 = *packedValue.valuePtr.ui16; break;
        case TypeInt32: value.as.i32 = *packedValue.valuePtr.i32; break;
        case TypeUint32: value.as.ui32 = *packedValue.valuePtr.ui32; break;
        case TypeInt64: value.as.i64 = *packedValue.valuePtr.i64; break;
        case TypeUint64: value.as.ui64 = *packedValue.valuePtr.ui64; break;
        case TypeAddress: value.as.address = *packedValue.valuePtr.uiptr; break;
        default: panic(); break;
    }
    return value;
}