#ifndef cyarg_yargtype_h
#define cyarg_yargtype_h

#include "object.h"

#include <stddef.h>
#include <stdint.h>

typedef struct ObjString ObjString;
typedef struct ObjYarg ObjYarg;

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

bool is_placeable_type(YargType yt);
bool is_value_type(YargType yt);
size_t storage_size_of_type(YargType yt);


typedef struct ObjYargType {
    Obj obj;
    YargType yt;
} ObjYargType;

bool is_placeable_yargtype(const ObjYargType* type);
size_t storage_size_of_yargtype(const ObjYargType* type);


typedef struct {
    ObjYargType boolean;
    ObjYargType address;
    ObjYargType dbl;
    ObjYargType int8;
    ObjYargType uint8;
    ObjYargType int16;
    ObjYargType uint16;
    ObjYargType int32;
    ObjYargType uint32;
    ObjYargType int64;
    ObjYargType uint64;
} SimpleYargTypes;

extern const SimpleYargTypes yargTypes;

typedef struct ObjYargTypeArray {
    ObjYargType core;
    size_t cardinality;
    ObjYargType* element_type;
} ObjYargTypeArray;

typedef struct ObjYargTypePointer {
    ObjYargType core;
    const ObjYargType* target_type;
} ObjYargTypePointer;

typedef struct ObjYargTypeMap {
    ObjYargType core;
    ObjYargType* key_type;
    ObjYargType* value_type;
} ObjYargTypeMap;

ObjYargType* newYargTypeFromType(YargType yt);

ObjYargTypeArray* newYargArrayTypeFromType(ObjYargType* elementType, size_t cardinality);

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


#endif
