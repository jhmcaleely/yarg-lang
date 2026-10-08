#ifndef cyarg_yargobject_h
#define cyarg_yargobject_h

#include "object.h"
#include "big-int/big-int.h"
#include "packed_value.h"
#include "value.h"
#include "chunk.h"
#include "table.h"

typedef struct ObjConcreteYargType ObjConcreteYargType;
typedef struct ObjConcreteYargTypeArray ObjConcreteYargTypeArray;
typedef struct ObjConcreteYargTypeStruct ObjConcreteYargTypeStruct;
typedef struct ObjConcreteYargTypePointer ObjConcreteYargTypePointer;
typedef struct ObjConcreteYargTypeMap ObjConcreteYargTypeMap;

#define IS_CLASS(value)        isObjType(value, OBJ_CLASS)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)
#define IS_ROUTINE(value)      isObjType(value, OBJ_ROUTINE)
#define IS_CHANNEL(value)      isObjType(value, OBJ_CHANNELCONTAINER)
#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define IS_UNIFORMARRAY(value) (isObjType(value, OBJ_PACKEDUNIFORMARRAY)|| isObjType(value, OBJ_UNOWNED_UNIFORMARRAY))
#define IS_YARGTYPE(value)     (isObjType(value, OBJ_YARGTYPE) || isObjType(value, OBJ_YARGTYPE_ARRAY) || isObjType(value, OBJ_YARGTYPE_STRUCT) || isObjType(value, OBJ_YARGTYPE_POINTER) || isObjType(value, OBJ_YARGTYPE_MAP))
#define IS_POINTER(value)      (isObjType(value, OBJ_PACKEDPOINTER) || isObjType(value, OBJ_UNOWNED_PACKEDPOINTER))
#define IS_STRUCT(value)       (isObjType(value, OBJ_PACKEDSTRUCT) || isObjType(value, OBJ_UNOWNED_PACKEDSTRUCT))
#define IS_SYNCGROUP(value)    isObjType(value, OBJ_SYNCGROUP)
#define IS_MAP(value)          isObjType(value, OBJ_MAP)

#define AS_CLASS(value)        ((ObjClass*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))
#define AS_ROUTINE(value)      ((ObjRoutine*)AS_OBJ(value))
#define AS_CHANNEL(value)      ((ObjChannelContainer*)AS_OBJ(value))
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)
#define AS_UNIFORMARRAY(value) ((ObjPackedUniformArray*)AS_OBJ(value))
#define AS_YARGTYPE(value)     ((ObjConcreteYargType*)AS_OBJ(value))
#define AS_POINTER(value)      ((ObjPackedPointer*)AS_OBJ(value))
#define AS_STRUCT(value)       ((ObjPackedStruct*)AS_OBJ(value))
#define AS_SYNCGROUP(value)    ((ObjSyncGroup*)AS_OBJ(value))
#define AS_INTOBJ(value)       ((ObjInt*)AS_OBJ(value))
#define AS_INT(value)          (&(AS_INTOBJ(value)->bigInt))
#define AS_MAP(value)          ((ObjMap*)AS_OBJ(value))

#define STRING_VAL(object)     ((Value){.type = &yargTypes.string, .as.obj = (Obj*) object})
#define YARGTYPE_VAL(object)   ((Value){.type = &yargTypes.type, .as.obj = (Obj*) object})
#define INTOBJ_VAL(object)     ((Value){.type = &yargTypes.integer, .as.obj = (Obj*) object})
#define CLASS_VAL(object)      ((Value){.type = &yargTypes.class_, .as.obj = (Obj*) object})
#define INSTANCE_VAL(object)   ((Value){.type = &yargTypes.function, .as.obj = (Obj*) object})
#define FUNCTION_VAL(object)   ((Value){.type = &yargTypes.function, .as.obj = (Obj*) object})
#define CLOSURE_VAL(object)    ((Value){.type = &yargTypes.function, .as.obj = (Obj*) object})
#define ROUTINE_VAL(object)    ((Value){.type = &yargTypes.routine, .as.obj = (Obj*) object})
#define NATIVE_VAL(object)     ((Value){.type = &yargTypes.function, .as.obj = (Obj*) object})
#define CHANNEL_VAL(object)    ((Value){.type = &yargTypes.channel, .as.obj = (Obj*) object})
#define SYNCGROUP_VAL(object)  ((Value){.type = &yargTypes.syncGroup, .as.obj = (Obj*) object})
#define POINTER_VAL(object)    ((Value){.type = ((const ObjConcreteYargType*)((ObjPackedPointer*)object)->type), .as.obj = (Obj*) object})
#define BOUNDMETHOD_VAL(object)  ((Value){.type = &yargTypes.function, .as.obj = (Obj*) object})
#define MAP_VAL(object)        ((Value){.type = ((const ObjConcreteYargType*)((ObjMap*)object)->type), .as.obj = (Obj*) object})
#define STRUCT_VAL(object)     ((Value){.type = ((const ObjConcreteYargType*)((ObjPackedStruct*)object)->store.storedType), .as.obj = (Obj*) object})
#define ARRAY_VAL(object)      ((Value){.type = ((const ObjConcreteYargType*)((ObjPackedUniformArray*)object)->store.storedType), .as.obj = (Obj*) object})

struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

typedef struct ObjInt {
    Obj obj;
    bool isLiteral;
    Int bigInt;
} ObjInt;


typedef struct ObjFunction {
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjString* fName;
} ObjFunction;

typedef struct {
    Obj obj;
    ObjString* name;
    ValueTable methods;
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass* klass;
    ValueTable fields;
} ObjInstance;

typedef struct ObjPackedUniformArray {
    Obj obj;
    PackedValue store;
} ObjPackedUniformArray;

typedef struct {
    Obj obj;
    ObjConcreteYargTypePointer* type;
    PackedValueStore* destination;
} ObjPackedPointer;

typedef struct {
    Obj obj;
    PackedValue store;
} ObjPackedStruct;

typedef struct {
    Obj obj;
    ObjConcreteYargTypeMap* type;
    ValueTable entries;
} ObjMap;

ObjFunction* newFunction();
void initFunction(ObjFunction* function);
ObjClass* newClass(ObjString* name);
ObjInstance* newInstance(ObjClass* klass);
ObjInt* allocateIntObject(size_t numDigits);

ObjPackedUniformArray* newPackedUniformArray(ObjConcreteYargTypeArray* type);
ObjMap* newMap(ObjConcreteYargTypeMap* type);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
ObjString* copyStringWithEscapes(const char* chars, int length);
ObjString* concatenateStrings(ObjString* a, ObjString* b);
ObjInt* newInt(int64_t value);
ObjInt* newIntU(uint64_t value);

ObjPackedPointer* newPointerForHeapCell(PackedValue location);
ObjPackedPointer* newPointerAtHeapCell(PackedValue location);

void offsetPointerDestination(ObjPackedPointer* pointer, size_t offset);

ObjPackedStruct* newPackedStructAt(PackedValue location);
ObjPackedUniformArray* newPackedUniformArrayAt(PackedValue location);

Value defaultIntValue();
Value defaultArrayValue(ObjConcreteYargType* type);
Value defaultStructValue(ObjConcreteYargType* type);

Value placeObjectAt(Value type, Value location);

uintptr_t pinUniformArray(ObjPackedUniformArray* array);

bool isArrayPointer(Value value);
bool isStructPointer(Value value);

bool arraysEqual(const ObjPackedUniformArray* a, const ObjPackedUniformArray* b);
bool structsEqual(const ObjPackedStruct* a, const ObjPackedStruct* b);
bool typesEqual(const ObjConcreteYargType* a, const ObjConcreteYargType* b);
bool pointersEqual(const ObjPackedPointer* a, const ObjPackedPointer* b);
bool intsEqual(const ObjInt* a, const ObjInt* b);

#endif