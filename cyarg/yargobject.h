#ifndef cyarg_yargobject_h
#define cyarg_yargobject_h

#include "object.h"
#include "big-int/big-int.h"

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

ObjString* objectToString(Value value);

bool isAddressValue(Value value);

bool isArrayPointer(Value value);
bool isStructPointer(Value value);


#endif