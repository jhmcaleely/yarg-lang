#ifndef PACKED_VALUE_H
#define PACKED_VALUE_H

#include "../../cyarg/value.h"

void packValue(PackedValue packedStorageTarget, Value value);

typedef union {
    Obj** obj;
    bool* boolean;
    double* dbl;
    int8_t* i8;
    uint8_t* ui8;
    int16_t* i16;
    uint16_t* ui16;
    int32_t* i32;
    uint32_t* ui32;
    int64_t* i64;
    uint64_t* ui64;
    uintptr_t* uiptr;
} ValuePtr;

typedef struct {
    ValuePtr valuePtr;
    ValueType type;
} PackedValue2;

void initialisePackedValue(PackedValue packedValue);
Value unpackValue(PackedValue packedValue);

#endif // PACKED_VALUE_H