#ifndef cyarg_yargtype_h
#define cyarg_yargtype_h

#include "object.h"

typedef enum {
   TypeBool,
   TypeInt,
   TypeDouble,
   TypeInt8,
   TypeUint8,
   TypeInt16,
   TypeUint16,
   TypeInt32,
   TypeUint32,
   TypeInt64,
   TypeUint64,
   TypeString,
   TypeAddress,
   TypeClass,
   TypeInstance,
   TypeFunction,
   TypeRoutine,
   TypeChannel,
   TypeSyncGroup,
   TypeArray,
   TypeStruct,
   TypePointer,
   TypeMap,
   TypeYargType
} ConcreteYargType;

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

typedef struct ObjConcreteYargType {
    Obj obj;
    ConcreteYargType yt;
} ObjConcreteYargType;

typedef struct {
    ObjConcreteYargType boolean;
    ObjConcreteYargType address;
    ObjConcreteYargType dbl;
    ObjConcreteYargType int8;
    ObjConcreteYargType uint8;
    ObjConcreteYargType int16;
    ObjConcreteYargType uint16;
    ObjConcreteYargType int32;
    ObjConcreteYargType uint32;
    ObjConcreteYargType int64;
    ObjConcreteYargType uint64;
    ObjConcreteYargType type;
    ObjConcreteYargType class_;
    ObjConcreteYargType string;
    ObjConcreteYargType integer;

    ObjConcreteYargType implementation_obj; // only the address matters.
    // temp
    ObjConcreteYargType function;
    ObjConcreteYargType routine;
    ObjConcreteYargType channel;
    ObjConcreteYargType syncGroup;
} SimpleYargTypes;

extern SimpleYargTypes yargTypes;

typedef struct ObjConcreteYargTypeArray {
    ObjConcreteYargType core;
    size_t cardinality;
    ObjConcreteYargType* element_type;
} ObjConcreteYargTypeArray;

typedef struct ObjConcreteYargTypePointer {
    ObjConcreteYargType core;
    ObjConcreteYargType* target_type;
} ObjConcreteYargTypePointer;

typedef struct ObjConcreteYargTypeMap {
    ObjConcreteYargType core;
    ObjConcreteYargType* key_type;
    ObjConcreteYargType* value_type;
} ObjConcreteYargTypeMap;

ObjConcreteYargType* newYargTypeFromType(ConcreteYargType yt);

ObjConcreteYargType* newYargArrayTypeFromType(ObjConcreteYargType* elementType);
ObjConcreteYargType* newYargPointerType(ObjConcreteYargType* targetType);

size_t arrayElementOffset(const ObjConcreteYargTypeArray* arrayType, size_t index);
size_t arrayElementSize(const ObjConcreteYargTypeArray* arrayType);
ObjConcreteYargType* arrayElementType(const ObjConcreteYargTypeArray* arrayType);

bool type_packs_as_obj(ObjConcreteYargType* type);
bool type_packs_as_container(ObjConcreteYargType* type);
bool is_nil_assignable_type(ObjConcreteYargType* type);
bool is_placeable_type(ObjConcreteYargType* type);
bool is_stored_type(ObjConcreteYargType* type);
bool is_obj_yargtype(const ObjConcreteYargType* type);
size_t yt_sizeof_type_storage(ObjConcreteYargType* type);
size_t yt_alignmentfor_type_storage(ObjConcreteYargType* type);

bool structFieldIndex(ObjConcreteYargType* type, ObjString* name, size_t* index);

bool isSupportedMapKeyType(ObjConcreteYargType* type);

ObjString* typeToString(const ObjConcreteYargType* type);

#endif
