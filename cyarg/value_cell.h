#ifndef cyarg_value_cell_h
#define cyarg_value_cell_h

#include "yargtype.h"
#include "value.h"
#include "placed_value.h"
typedef struct ObjRoutine ObjRoutine;

typedef struct ValueCell {
    Value value;
    ObjConcreteYargType* cellType;
} ValueCell;

typedef struct {
    Value* value;
    ObjConcreteYargType* cellType;
} ValueCellTarget;

typedef struct {
    PlacedValuePtr valuePtr;
    const ObjConcreteYargType* type;
    const ObjConcreteYargType* cellType;
} PlacedValueCellTarget;

bool assignToValueCellTarget(ValueCellTarget lhs, Value rhsValue);
bool initialiseValueCellTarget(ValueCellTarget lhs, Value rhsValue);

bool assignToPlacedValueCellTarget(PlacedValueCellTarget lhs, Value rhsValue);

ValueCellTarget peekCellTarget(ObjRoutine* routine, int distance);

#endif