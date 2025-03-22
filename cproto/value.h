#ifndef clox_value_h
#define clox_value_h

#include <string.h>

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct ObjRoutine ObjRoutine;

#ifdef NAN_BOXING

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((uint64_t)0x7ffc000000000000)
#define UINT_TAG ((uint64_t)0x0001000000000000)
#define INT_TAG  ((uint64_t)0x0002000000000000)
#define OBJ_MASK ((uint64_t)0x0000FFFFFFFFFFFF)
#define INT_MASK ((uint64_t)0x00000000FFFFFFFF)


#define TAG_NIL   1 // 01.
#define TAG_FALSE 2 // 10.
#define TAG_TRUE  3 // 11.

typedef uint64_t Value;

#define IS_BOOL(value)      (((value) | 1) == TRUE_VAL)
#define IS_NIL(value)       ((value) == NIL_VAL)
#define IS_DOUBLE(value)    (((value) & QNAN) != QNAN)
#define IS_UINTEGER(value) \
    (((value) & (QNAN | UINT_TAG)) == (QNAN | UINT_TAG))
#define IS_INTEGER(value) \
    (((value) & (QNAN | INT_TAG)) == (QNAN | INT_TAG))
#define IS_OBJ(value) \
    (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value)      ((value) == TRUE_VAL)
#define AS_DOUBLE(value)    valueToDbl(value)
#define AS_UINTEGER(value) \
     ((uint32_t)((uint64_t)(value) & (INT_MASK)))
#define AS_INTEGER(value) \
     ((uint32_t)((uint64_t)(value) & (INT_MASK)))
#define AS_OBJ(value) \
     ((Obj*)(uintptr_t)((value) & (OBJ_MASK)))

#define BOOL_VAL(b)     ((b) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL       ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL        ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL         ((Value)(uint64_t)(QNAN | TAG_NIL))
#define DOUBLE_VAL(num) dblToValue(num)
#define UINTEGER_VAL(uinteger) \
    (Value)( QNAN | UINT_TAG | (INT_MASK & (uint64_t)(uinteger)))
#define INTEGER_VAL(integer) \
    (Value)( QNAN | INT_TAG | (INT_MASK & (uint64_t)(integer)))
#define OBJ_VAL(obj) \
    (Value)(SIGN_BIT | QNAN | (OBJ_MASK & (uint64_t)(uintptr_t)(obj)))

static inline double valueToDbl(Value value) {
    double num;
    memcpy(&num, &value, sizeof(Value));
    return num;
}

static inline Value dblToValue(double num) {
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
}

#else 

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_DOUBLE,
    VAL_UINTEGER,
    VAL_INTEGER,
    VAL_OBJ,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double dbl;
        uint32_t uinteger;
        int32_t integer;
        Obj* obj;
    } as;
} Value;

#define IS_BOOL(value)     ((value).type == VAL_BOOL)
#define IS_NIL(value)      ((value).type == VAL_NIL)
#define IS_DOUBLE(value)   ((value).type == VAL_DOUBLE)
#define IS_UINTEGER(value) ((value).type == VAL_UINTEGER)
#define IS_INTEGER(value)  ((value).type == VAL_INTEGER)
#define IS_OBJ(value)      ((value).type == VAL_OBJ)

#define AS_OBJ(value)      ((value).as.obj)
#define AS_BOOL(value)     ((value).as.boolean)
#define AS_UINTEGER(value) ((value).as.uinteger)
#define AS_INTEGER(value)  ((value).as.integer)
#define AS_DOUBLE(value)   ((value).as.dbl)

#define BOOL_VAL(value)     ((Value){VAL_BOOL, {.boolean = value }})
#define NIL_VAL             ((Value){VAL_NIL, {.integer = 0 }})
#define DOUBLE_VAL(value)   ((Value){VAL_DOUBLE, {.dbl = value }})
#define UINTEGER_VAL(value) ((Value){VAL_UINTEGER, {.uinteger = value }})
#define INTEGER_VAL(value)  ((Value){VAL_INTEGER, {.integer = value }})
#define OBJ_VAL(object)     ((Value){VAL_OBJ, {.obj = (Obj*)object}})

#endif

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif