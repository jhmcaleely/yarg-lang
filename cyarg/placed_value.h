#ifndef cyarg_placed_value_h
#define cyarg_placed_value_h

#include "yargtype.h"
#include "value.h"

typedef struct {
    void* value;
    const ObjConcreteYargType* type;
} PlacedValue;

PlacedValue allocPlacedValue(const ObjConcreteYargType* type);
void setDefaultPlacedValue(PlacedValue value);

Value getPlacedValue(PlacedValue placedStorage);

#endif // cyarg_placed_value_h