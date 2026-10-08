#ifndef cyarg_packed_value_h
#define cyarg_packed_value_h

#include "value.h"

typedef union PackedValueStore PackedValueStore;

typedef struct {
    PackedValueStore* storedValue;
    ObjConcreteYargType* storedType;
} PackedValue;

void initialisePackedValue(PackedValue packedValue);
Value unpackValue(PackedValue packedValue);
PackedValue allocPackedValue(Value type);
void markPackedValue(PackedValue packedValue);

bool assignToPackedValue(PackedValue lhs, Value rhsValue);

bool is_uniformarray(PackedValue val);
bool is_struct(PackedValue val);
bool is_channel(PackedValue val);

PackedValue arrayElement(PackedValue array, size_t index);
size_t arrayCardinality(PackedValue array);

PackedValue structField(PackedValue struct_, size_t index);

PackedValueStore* storedAddressof(Value value);
PackedValue packUintptr(uintptr_t value);

#endif