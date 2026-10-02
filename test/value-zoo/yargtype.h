#ifndef cyarg_yargtype_h
#define cyarg_yargtype_h

#include "object.h"

#include <stddef.h>

typedef struct ObjString ObjString;

typedef enum {
   // Yarg types with implementation defined storage
   TypeBool,
   TypeInt,
   TypePointer,
   TypeClass,
   TypeInstance,
   TypeFunction,
   TypeRoutine,
   TypeChannel,
   TypeMap,
   TypeYargType,

   // Yarg types which have platform defined storage
   TypeDouble,
   TypeInt8,
   TypeUint8,
   TypeInt16,
   TypeUint16,
   TypeInt32,
   TypeUint32,
   TypeInt64,
   TypeUint64,
   TypeAddress,

   // Yarg types with partially platform defined storage
   TypeString, // will contain a 'c' style \0 terminated string

   // Container Types that can have defined storage, if they contain elements with defined storage
   TypeArray,
   TypeStruct,
} YargType;

typedef struct ObjYargType {
    Obj obj;
    YargType yt;
} ObjYargType;

typedef struct ObjYargTypeArray {
    ObjYargType core;
    size_t cardinality;
    ObjYargType* element_type;
} ObjYargTypeArray;

typedef struct ObjYargTypePointer {
    ObjYargType core;
    ObjYargType* target_type;
} ObjYargTypePointer;

typedef struct ObjYargTypeMap {
    ObjYargType core;
    ObjYargType* key_type;
    ObjYargType* value_type;
} ObjYargTypeMap;

ObjYargType* newYargTypeFromType(YargType yt);

ObjYargTypeArray* newYargArrayTypeFromType(ObjYargType* elementType);
ObjYargType* newYargStructType(size_t fieldCount);
ObjYargTypePointer* newYargPointerType(ObjYargType* targetType);

size_t arrayElementOffset(ObjYargTypeArray* arrayType, size_t index);
size_t arrayElementSize(ObjYargTypeArray* arrayType);
Value arrayElementType(ObjYargTypeArray* arrayType);

size_t addFieldType(ObjYargType* st, size_t index, size_t fieldOffset, Value type, Value offset, Value name);

bool isUint32Pointer(Value val);

Value concrete_typeof(Value a);
bool type_packs_as_obj(ObjYargType* type);
bool type_packs_as_container(ObjYargType* type);
bool is_nil_assignable_type(Value type);
bool is_placeable_type(Value type);
bool is_stored_type(Value type);
size_t yt_sizeof_type_storage(Value type);
size_t yt_alignmentfor_type_storage(Value type);

Value defaultValue(Value type);

bool isInitialisableType(ObjYargType* lhsType, Value rhsValue, Value *promotedRhs); // promotedRhs will be VAL_NIL if no promotion

bool isSupportedMapKeyType(Value type);

ObjString* typeToString(ObjYargType* type);

#endif
