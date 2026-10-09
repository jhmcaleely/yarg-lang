#ifndef cyarg_placed_value_h
#define cyarg_placed_value_h

#include "yargtype.h"
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
    const ObjConcreteYargType* type;
} PlacedValue;

PlacedValue allocPlacedValue(const ObjConcreteYargType* type);
PlacedValue createPlacedValue(uintptr_t ptr, const ObjConcreteYargType* type);

void setPlacedValue(PlacedValue placedStorage, Value value);
Value getPlacedValue(PlacedValue placedStorage);


#endif // cyarg_placed_value_h