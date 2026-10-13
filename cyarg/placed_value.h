#ifndef cyarg_placed_value_h
#define cyarg_placed_value_h

#include "yargtype.h"
#include "value.h"

typedef struct {
    void* value;
    const ObjConcreteYargType* type;
} PlacedValue;

void setDefaultPlacedValue(PlacedValue value);

Value getPlacedValue(PlacedValue placedStorage);
void setPlacedValue(PlacedValue placedStorage, Value rhs);

#endif // cyarg_placed_value_h