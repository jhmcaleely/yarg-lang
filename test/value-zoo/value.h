#ifndef cyarg_value_h
#define cyarg_value_h

#include <stdbool.h>
#include <stdint.h>

#include "yargtype.h"
#include "object.h"

typedef struct {
    const ObjYargType* type;
    AnyValue as;
    bool type_assignable;
} Value;

typedef struct ObjValue {
    Obj   core;
    Value value;
} ObjValue;

#define IS_IMPL_OBJ(value) ((value).type == NULL && (value).as.obj != NULL)
#define IS_NIL(value)      is_nil(value)

#define IS_BOOL(value)     ((value).type->yt == TypeBool)
#define IS_INT(value)      ((value).type->yt == TypeInt && (value).as.obj->type == OBJ_INT)
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
#define IS_YARGTYPE(value) ((value).type->yt == TypeYargType)
#define IS_ARRAY(value)    ((value).type->yt == TypeArray && (value).as.obj->type == OBJ_PACKEDUNIFORMARRAY)
#define IS_STRUCT(value)   ((value).type->yt == TypeStruct && (value).as.obj->type == OBJ_PACKEDSTRUCT)
#define IS_FUNCTION(value) ((value).type->yt == TypeFunction && (value).as.obj->type == OBJ_FUNCTION)
#define IS_ROUTINE(value)  ((value).type->yt == TypeRoutine && (value).as.obj->type == OBJ_ROUTINE)
#define IS_CHANNEL(value)  ((value).type->yt == TypeChannel && (value).as.obj->type == OBJ_CHANNELCONTAINER)
#define IS_MAP(value)      ((value).type->yt == TypeMap && (value).as.obj->type == OBJ_MAP)
#define IS_INSTANCE(value) ((value).type->yt == TypeInstance && (value).as.obj->type == OBJ_INSTANCE)
#define IS_CLASS(value)    ((value).type->yt == TypeClass && (value).as.obj->type == OBJ_CLASS)
#define IS_STRING(value)   ((value).type->yt == TypeString && (value).as.obj->type == OBJ_STRING)

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

#define NIL_VAL             ((Value){.type = NULL, .as.obj = NULL })

#define BOOL_VAL(value)     ((Value){.type = &yargTypes.boolean, .as.boolean = value })
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

#define IMPL_OBJ_VAL(object) ((Value){.type = NULL, {.obj = (Obj*)object}})

#if IS_64BIT
#define SIZE_T_UI_VAL(value)   UI64_VAL(value)
#elif IS_32BIT
#define SIZE_T_UI_VAL(value)   UI32_VAL(value)
#endif

bool is_nil(Value value);

bool is_positive_integer(Value a);
size_t as_positive_integer(Value a);

bool valuesEqual(Value a, Value b);

Value duplicateValue(Value src);

typedef struct ObjYargPointer ObjYargPointer;
void setPointerTarget(ObjYargPointer* pointer, Value newVal);

#endif
