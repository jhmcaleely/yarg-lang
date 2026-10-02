#ifndef cyarg_value_h
#define cyarg_value_h

#include <stdbool.h>
#include <stdint.h>

#include "yargtype.h"
#include "yargobject.h"

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

typedef struct {
    const ObjYargType* type;
    AnyValue as;
} Value;

#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
#define IS_64BIT 1
#define IS_32BIT 0
#else
#define IS_64BIT 0
#define IS_32BIT 1
#endif

#define IS_BOOL(value)     ((value).type->yt == TypeBool)
#define IS_NIL(value)      ((value).type == NULL && (value).as.i32 == 0)
#define IS_DOUBLE(value)   ((value).type->yt == TypeDouble)
#define IS_I8(value)       ((value).type->yt == TypeInt8)
#define IS_UI8(value)      ((value).type->yt == TypeUint8)
#define IS_I16(value)      ((value).type->yt == TypeInt16)
#define IS_UI16(value)     ((value).type->yt == TypeUint16)
#define IS_I32(value)      ((value).type->yt == TypeInt32)
#define IS_UI32(value)     ((value).type->yt == TypeUint32)
#define IS_UI64(value)     ((value).type->yt == TypeUint64)
#define IS_I64(value)      ((value).type->yt == TypeInt64)
#define IS_ADDRESS(value)  ((value).type->yt == TypeAddress)
#define IS_OBJ(value)      ((value).type == NULL && (value).as.obj != NULL)
#define IS_ARRAY(value)    ((value).type != NULL && (value).as.obj != NULL && ((ObjArray*)(value).as.obj)->core.type == (value).type && ((ObjArray*)(value).as.obj)->core.type->yt == TypeArray)
#define IS_INT(value)      ((value).type == VAL_OBJ && (value).as.obj->type == OBJ_INT)

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

#define BOOL_VAL(value)     ((Value){.type = &yargTypes.boolean, .as.boolean = value })
#define NIL_VAL             ((Value){.type = NULL, .as.i32 = 0 })
#define DOUBLE_VAL(value)   ((Value){.type = &yargTypes.dbl, {.dbl = value }})
#define I8_VAL(value)       ((Value){.type = &yargTypes.int8, .as.i8 = value})
#define UI8_VAL(value)      ((Value){.type = &yargTypes.uint8, .as.ui8 = value})
#define I16_VAL(value)      ((Value){.type = &yargTypes.int16, .as.i16 = value})
#define UI16_VAL(value)     ((Value){.type = &yargTypes.uint16, .as.ui16 = value})
#define I32_VAL(value)      ((Value){.type = &yargTypes.int32, .as.i32 = value })
#define UI32_VAL(value)     ((Value){.type = &yargTypes.uint32, .as.ui32 = value })
#define I64_VAL(a)          ((Value){.type = &yargTypes.int64, .as.i64 = a})
#define UI64_VAL(a)         ((Value){.type = &yargTypes.uint64, .as.ui64 = a})
#define ADDRESS_VAL(value)  ((Value){.type = &yargTypes.address, .as.address = value })
#define OBJ_VAL(object)     ((Value){.type = NULL, {.obj = (Obj*)object}})
#define ARRAY_VAL(array)    ((Value){.type = array->core.type, .as.obj = (Obj*)array})

#if IS_64BIT
#define SIZE_T_UI_VAL(value)   UI64_VAL(value)
#elif IS_32BIT
#define SIZE_T_UI_VAL(value)   UI32_VAL(value)
#endif

bool is_positive_integer32(Value a);
uint32_t as_positive_integer32(Value a);

bool valuesEqual(Value a, Value b);

Value duplicateValue(Value src);

#endif
