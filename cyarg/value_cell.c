#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value_cell.h"

void initValueCellArray(ValueCellArray* array) {
    array->cells = NULL;
    array->capacity = 0;
    array->count = 0;
}

void appendToValueCellArray(ValueCellArray* array, ValueCell value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->cells = GROW_ARRAY(ValueCell, array->cells, oldCapacity, array->capacity);
    }

    array->cells[array->count] = value;
    array->count++;
}

void freeValueCellArray(ValueCellArray* array) {
    FREE_ARRAY(Value, array->cells, array->capacity);
    initValueCellArray(array);
}