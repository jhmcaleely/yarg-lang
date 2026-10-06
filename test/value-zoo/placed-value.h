#ifndef PLACED_VALUE_H
#define PLACED_VALUE_H

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
    uintptr_t* address;
} PlacedValuePtr;

typedef struct {
    PlacedValuePtr valuePtr;
    const ObjYargType* type;
} PlacedValue;

typedef struct ValueLocation {
    bool placed;
    union {
        PlacedValue placedValue;
        Value* valuePtr;
    };
} ValueLocation;

typedef struct ObjLocation {
    Obj obj;
    const ObjYarg* owner;
    ValueLocation loc;
} ObjLocation;

PlacedValue allocPlacedValue(const ObjYargType* type);
PlacedValue createPlacedValue(uintptr_t ptr, const ObjYargType* type);

void placeValue(PlacedValue placedStorage, Value value);
Value getPlacedValue(PlacedValue placedStorage);

Value getLocationValue(ObjLocation* location);
void setLocationValue(ObjLocation* location, Value value);

#endif // PLACED_VALUE_H