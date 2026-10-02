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

extern ObjYargType boolType;
extern ObjYargType uint32Type;
extern ObjYargType addressType;

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

ObjYargTypeArray* newYargArrayTypeFromType(ObjYargType* elementType, size_t cardinality);
ObjYargTypePointer* newYargPointerType(ObjYargType* targetType);


#endif
