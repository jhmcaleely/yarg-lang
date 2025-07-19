#ifndef cyarg_value_cell_h
#define cyarg_value_cell_h

#include "value.h"

typedef enum {
    CELL_TYPE_ANY,
    CELL_TYPE_BOOL,
    CELL_TYPE_NIL,
    CELL_TYPE_DOUBLE,
    CELL_TYPE_UINTEGER,
    CELL_TYPE_INTEGER,
    CELL_TYPE_OBJ,
} CellType;

typedef struct {
    Value val;
    CellType required_type;
} ValueCell;

typedef struct {
    int capacity;
    int count;
    ValueCell* cells;
} ValueCellArray;

void initValueCellArray(ValueCellArray* array);
void appendToValueCellArray(ValueCellArray* array, ValueCell value);
void freeValueCellArray(ValueCellArray* array);

#endif