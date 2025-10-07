#ifndef cyarg_value_h
#define cyarg_value_h

#include <string.h>
#include <stdio.h>

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct ObjRoutine ObjRoutine;
typedef struct ObjValArray ObjValArray;
typedef struct ObjConcreteYargType ObjConcreteYargType;

typedef union {
    bool boolean;
    double dbl;
    uint8_t ui8;
    int8_t i8;
    uint16_t ui16;
    int16_t i16;
    uint32_t ui32;
    int32_t i32;
    uint64_t ui64;
    int64_t i64;
    uintptr_t address;
    Obj* obj;
} AnyValue;

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_DOUBLE,
    VAL_I8,
    VAL_UI8,
    VAL_I16,
    VAL_UI16,
    VAL_I32,
    VAL_UI32,
    VAL_UI64,
    VAL_I64,
    VAL_ADDRESS,
    VAL_OBJ,
} ValueType;

typedef struct {
    ValueType type;
    AnyValue as;
} Value;

#define IS_BOOL(value)     ((value).type == VAL_BOOL)
#define IS_NIL(value)      ((value).type == VAL_NIL)
#define IS_DOUBLE(value)   ((value).type == VAL_DOUBLE)
#define IS_I8(value)       ((value).type == VAL_I8)
#define IS_UI8(value)      ((value).type == VAL_UI8)
#define IS_I16(value)      ((value).type == VAL_I16)
#define IS_UI16(value)     ((value).type == VAL_UI16)
#define IS_I32(value)      ((value).type == VAL_I32)
#define IS_UI32(value)     ((value).type == VAL_UI32)
#define IS_UI64(value)     ((value).type == VAL_UI64)
#define IS_I64(value)      ((value).type == VAL_I64)
#define IS_ADDRESS(value)  ((value).type == VAL_ADDRESS)
#define IS_OBJ(value)      ((value).type == VAL_OBJ)

#define AS_OBJ(value)      ((value).as.obj)
#define AS_BOOL(value)     ((value).as.boolean)
#define AS_I8(value)       ((value).as.i8)
#define AS_UI8(value)      ((value).as.ui8)
#define AS_I16(value)      ((value).as.i16)
#define AS_UI16(value)     ((value).as.ui16)
#define AS_I32(value)      ((value).as.i32)
#define AS_UI32(value)     ((value).as.ui32)
#define AS_UI64(value)     ((value).as.ui64)
#define AS_I64(value)      ((value).as.i64)
#define AS_ADDRESS(value)  ((value).as.address)
#define AS_DOUBLE(value)   ((value).as.dbl)

#define BOOL_VAL(value)     ((Value){VAL_BOOL, {.boolean = value }})
#define NIL_VAL             ((Value){VAL_NIL, {.i32 = 0 }})
#define DOUBLE_VAL(value)   ((Value){VAL_DOUBLE, {.dbl = value }})
#define I8_VAL(value)       ((Value){VAL_I8, {.i8 = value}})
#define UI8_VAL(value)      ((Value){VAL_UI8, {.ui8 = value}})
#define I16_VAL(value)      ((Value){VAL_I16, {.i16 = value}})
#define UI16_VAL(value)     ((Value){VAL_UI16, {.ui16 = value}})
#define I32_VAL(value)      ((Value){VAL_I32, {.i32 = value }})
#define UI32_VAL(value)     ((Value){VAL_UI32, {.ui32 = value }})
#define I64_VAL(a)          ((Value){VAL_I64, {.i64 = a}})
#define UI64_VAL(a)         ((Value){VAL_UI64, {.ui64 = a}})
#define ADDRESS_VAL(value)  ((Value){VAL_ADDRESS, { .address = value}})
#define OBJ_VAL(object)     ((Value){VAL_OBJ, {.obj = (Obj*)object}})

typedef union {
    AnyValue as;
    Value asValue;
} StoredValue;

typedef struct {
    StoredValue* storedValue;
    ObjConcreteYargType* storedType;
} StoredValueTarget;

void initialisePackedStorage(Value type, StoredValue* storage);
Value unpackStoredValue(Value type, StoredValue* packedStorage);
void packValueStorage(StoredValueTarget packedStorageTarget, Value value);
void* createHeapCell(Value type);

typedef struct {
    Value value;
    Value type;
} ValueCell;

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void appendToValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);

void printValue(Value value);
void fprintValue(FILE* op, Value value);

bool is_positive_integer(Value a);
uint32_t as_positive_integer(Value a);

typedef struct {
    Value* value;
    ObjConcreteYargType* cellType;
} ValueCellTarget;

#endif