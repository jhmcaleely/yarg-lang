#ifndef cyarg_yargtype_h
#define cyarg_yargtype_h

#include "object.h"

#include <stddef.h>
#include <stdint.h>

//               | Value | Placeable | Container | Parameterised | Obj |
// TypeBool      | x     |           |           |               |     |
// TypeInt       | x     |           |           |               | x   |
// TypePointer   |       |           | x         | x             | x   |
// TypeClass     |       |           |           |               | x   |
// TypeInstance  |       |           |           | x             | x   |
// TypeFunction  |       |           |           | x             | x   |
// TypeRoutine   |       |           | %         | x             | x   |
// TypeChannel   |       |           | x         | x             | x   |
// TypeMap       |       |           | x         | x             | x   |
// TypeYargType  |       |           |           |               | x   |
// TypeDouble    | x     | x         |           |               |     |
// TypeInt8      | x     | x         |           |               |     |
// TypeUint8     | x     | x         |           |               |     |
// TypeInt16     | x     | x         |           |               |     |
// TypeUint16    | x     | x         |           |               |     |
// TypeInt32     | x     | x         |           |               |     |
// TypeUint32    | x     | x         |           |               |     |
// TypeInt64     | x     | x         |           |               |     |
// TypeUint64    | x     | x         |           |               |     |
// TypeAddress   | x     | x         |           |               |     |
// TypeString    |       | !         |           |               | x   |
// TypeArray     | x     | *         | x         | x             | x   |
// TypeStruct    | x     | *         | x         | x             | x   |
//
// Value - values are copied when passed. other types are 'pass by reference'
// Placeable - has a platform defined layout in memory. Container types
//             occupy linear memory suffient for all elements
// Container - storage for one-or-more elements of another type.
// Parameterised - requires information to fully define the type, eg the
//                 size of an array, or the types of its elements.
// Obj - Always stored as a heap Obj(ect).
//
// ! will have a c-style string available
// * placeable if it's contained types are placeable
// % routines contain only functions

typedef enum {
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
   TypeString,
   TypeArray,
   TypeStruct,
} YargType;


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

#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
#define IS_64BIT 1
#define IS_32BIT 0
#else
#define IS_64BIT 0
#define IS_32BIT 1
#endif

typedef struct ObjString ObjString;
typedef struct ObjYarg ObjYarg;

bool is_value_type(YargType yt);
bool is_placeable_type(YargType yt);
bool is_container_type(YargType yt);
bool is_parameterised_type(YargType yt);
bool is_obj_type(YargType yt);

size_t storage_size_of_type(YargType yt);

typedef struct ObjYargType {
    Obj obj;
    YargType yt;
} ObjYargType;

bool is_value_yargtype(const ObjYargType* type);
bool is_placeable_yargtype(const ObjYargType* type);
bool is_container_yargtype(const ObjYargType* type);
bool is_parameterised_yargtype(const ObjYargType* type);
bool is_obj_yargtype(const ObjYargType* type);

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
    ObjYargType type;
    ObjYargType class_;
} SimpleYargTypes;

extern const SimpleYargTypes yargTypes;

typedef struct ObjYargTypeArray {
    ObjYargType core;
    size_t cardinality;
    const ObjYargType* element_type;
} ObjYargTypeArray;

typedef struct ObjYargTypePointer {
    ObjYargType core;
    const ObjYargType* target_type;
} ObjYargTypePointer;

typedef struct ObjYargTypeMap {
    ObjYargType core;
    const ObjYargType* key_type;
    const ObjYargType* value_type;
} ObjYargTypeMap;

typedef struct ObjYargTypeFunction {
    ObjYargType core;
    size_t param_count;
} ObjYargTypeFunction;

typedef struct ObjYargTypeChannel {
    ObjYargType core;
    const ObjYargType* element_type;
} ObjYargTypeChannel;

typedef struct ObjYargTypeInstance {
    ObjYargType core;
    const ObjYargType* class_type;
} ObjYargTypeInstance;

typedef struct ObjYargTypeStruct {
    ObjYargType core;
    // todo.
} ObjYargTypeStruct;

typedef struct ObjYargTypeRoutine {
    ObjYargType core;
    const ObjYargType* entry_function_type;
} ObjYargTypeRoutine;

const ObjYargType* newYargTypeFromType(YargType yt);

const ObjYargTypeArray* newYargArrayTypeFromType(const ObjYargType* elementType, size_t cardinality);


#endif
