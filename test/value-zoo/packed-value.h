#ifndef PACKED_VALUE_H
#define PACKED_VALUE_H

#include "value.h"

typedef union {
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
    ObjYargType* type;
} PackedValue;

void initialisePackedValue(PackedValue packedValue);
PackedValue allocPackedValue(ObjYargType* type);
void packValue(PackedValue packedStorageTarget, Value value);
Value unpackValue(PackedValue packedValue);
PackedValue createPackedValue(uintptr_t ptr, ObjYargType* type);

#endif // PACKED_VALUE_H