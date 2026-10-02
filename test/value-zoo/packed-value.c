#include "packed-value.h"

#include "yargtype.h"

#include <stdlib.h>

void initializePackedValue(PackedValue2 packedValue) {
    switch (packedValue.type) {
        case VAL_NIL: *packedValue.valuePtr.ui32 = 0; break;
        case VAL_BOOL: *packedValue.valuePtr.boolean = false; break;
        case VAL_DOUBLE: *packedValue.valuePtr.dbl = 0.0; break;
        case VAL_I8: *packedValue.valuePtr.i8 = 0; break;
        case VAL_UI8: *packedValue.valuePtr.ui8 = 0; break;
        case VAL_I16: *packedValue.valuePtr.i16 = 0; break;
        case VAL_UI16: *packedValue.valuePtr.ui16 = 0; break;
        case VAL_I32: *packedValue.valuePtr.i32 = 0; break;
        case VAL_UI32: *packedValue.valuePtr.ui32 = 0; break;
        case VAL_I64: *packedValue.valuePtr.i64 = 0; break;
        case VAL_UI64: *packedValue.valuePtr.ui64 = 0; break;
        case VAL_ADDRESS: *packedValue.valuePtr.uiptr = 0; break;
        case VAL_OBJ: *packedValue.valuePtr.obj = NULL; break;
        break;
    }
}

PackedValue2 createPackedValue(uintptr_t ptr, ValueType type) {
    PackedValue2 packedValue;
    packedValue.type = type;
    switch (type) {
        case VAL_NIL: packedValue.valuePtr.ui32 = (uint32_t*)ptr; break;
        case VAL_BOOL: packedValue.valuePtr.boolean = (bool*)ptr; break;
        case VAL_DOUBLE: packedValue.valuePtr.dbl = (double*)ptr; break;
        case VAL_I8: packedValue.valuePtr.i8 = (int8_t*)ptr; break;
        case VAL_UI8: packedValue.valuePtr.ui8 = (uint8_t*)ptr; break;
        case VAL_I16: packedValue.valuePtr.i16 = (int16_t*)ptr; break;
        case VAL_UI16: packedValue.valuePtr.ui16 = (uint16_t*)ptr; break;
        case VAL_I32: packedValue.valuePtr.i32 = (int32_t*)ptr; break;
        case VAL_UI32: packedValue.valuePtr.ui32 = (uint32_t*)ptr; break;
        case VAL_I64: packedValue.valuePtr.i64 = (int64_t*)ptr; break;
        case VAL_UI64: packedValue.valuePtr.ui64 = (uint64_t*)ptr; break;
        case VAL_ADDRESS: packedValue.valuePtr.uiptr = (uintptr_t*)ptr; break;
        case VAL_OBJ: packedValue.valuePtr.obj = (Obj**)ptr; break;
    }
    return packedValue;
} 

PackedValue2 allocPackedValue2(ValueType type) {
    PackedValue2 packedValue;
    packedValue.type = type;
    switch (type) {
        case VAL_NIL: packedValue.valuePtr.ui32 = (uint32_t*)malloc(sizeof(uint32_t)); break;
        case VAL_BOOL: packedValue.valuePtr.boolean = (bool*)malloc(sizeof(bool)); break;
        case VAL_DOUBLE: packedValue.valuePtr.dbl = (double*)malloc(sizeof(double)); break;
        case VAL_I8: packedValue.valuePtr.i8 = (int8_t*)malloc(sizeof(int8_t)); break;
        case VAL_UI8: packedValue.valuePtr.ui8 = (uint8_t*)malloc(sizeof(uint8_t)); break;
        case VAL_I16: packedValue.valuePtr.i16 = (int16_t*)malloc(sizeof(int16_t)); break;
        case VAL_UI16: packedValue.valuePtr.ui16 = (uint16_t*)malloc(sizeof(uint16_t)); break;
        case VAL_I32: packedValue.valuePtr.i32 = (int32_t*)malloc(sizeof(int32_t)); break;
        case VAL_UI32: packedValue.valuePtr.ui32 = (uint32_t*)malloc(sizeof(uint32_t)); break;
        case VAL_I64: packedValue.valuePtr.i64 = (int64_t*)malloc(sizeof(int64_t)); break;
        case VAL_UI64: packedValue.valuePtr.ui64 = (uint64_t*)malloc(sizeof(uint64_t)); break;
        case VAL_ADDRESS: packedValue.valuePtr.uiptr = (uintptr_t*)malloc(sizeof(uintptr_t)); break;
        case VAL_OBJ: packedValue.valuePtr.obj = (Obj**)malloc(sizeof(Obj*)); break;
    }
    return packedValue;
}

void freePackedValue(PackedValue2 packedValue) {
    switch (packedValue.type) {
        case VAL_NIL: free(packedValue.valuePtr.ui32); break;
        case VAL_BOOL: free(packedValue.valuePtr.boolean); break;
        case VAL_DOUBLE: free(packedValue.valuePtr.dbl); break;
        case VAL_I8: free(packedValue.valuePtr.i8); break;
        case VAL_UI8: free(packedValue.valuePtr.ui8); break;
        case VAL_I16: free(packedValue.valuePtr.i16); break;
        case VAL_UI16: free(packedValue.valuePtr.ui16); break;
        case VAL_I32: free(packedValue.valuePtr.i32); break;
        case VAL_UI32: free(packedValue.valuePtr.ui32); break;
        case VAL_I64: free(packedValue.valuePtr.i64); break;
        case VAL_UI64: free(packedValue.valuePtr.ui64); break;
        case VAL_ADDRESS: free(packedValue.valuePtr.uiptr); break;
        case VAL_OBJ: free(packedValue.valuePtr.obj); break;
    }
}