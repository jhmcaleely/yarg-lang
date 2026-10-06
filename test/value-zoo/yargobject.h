#ifndef cyarg_yargobject_h
#define cyarg_yargobject_h

#include "yargtype.h"
#include "value.h"

typedef struct ObjYarg {
    Obj          core;
    const ObjYargType* type;
} ObjYarg;

typedef struct ObjLocation ObjLocation;

#define AS_YARG_OBJ(value) ((ObjYarg*)(AS_OBJ(value)))
#define AS_ARRAY(value) ((ObjArray*)(AS_YARG_OBJ(value)))
#define AS_STRUCT(value) ((ObjStruct*)(AS_YARG_OBJ(value)))
#define AS_MAP(value) ((ObjMap*)(AS_YARG_OBJ(value)))
#define AS_INT(value) ((ObjInt*)(AS_YARG_OBJ(value)))
#define AS_POINTER(value) ((ObjYargPointer*)(AS_YARG_OBJ(value)))
#define AS_YARGTYPE(value) ((ObjYargType*)(AS_YARG_OBJ(value)))

#define YARG_OBJ_VAL(object) ((Value){.type = ((const ObjYarg*)(object))->type, .as.obj = (Obj*)object})

#define ARRAY_VAL(array)    YARG_OBJ_VAL(array)

typedef struct ObjYargPointer {
    ObjYarg  yarg;
    Obj* owner;
    void* target;
} ObjYargPointer;

typedef struct ObjArray {
    ObjYarg yarg;
    void*   elements;
} ObjArray;

typedef struct ObjStruct {
    ObjYarg yarg;
    void*   fields;
} ObjStruct;

typedef struct ObjMap {
    ObjYarg yarg;
    void*   entries;
} ObjMap;

typedef struct ObjInt {
    ObjYarg yarg;
    int64_t value;
} ObjInt;

Obj* allocateYargObject(const ObjYargType* type);

ObjYargPointer* newYargPointerToObj(ObjYarg* target);
ObjYargPointer* newYargPointerToObjVal(ObjValue* target);
ObjYargPointer* newYargPointerToPlaceable(const ObjYargType* targetType, void* memory);

ObjLocation* arrayElement(ObjArray* array, size_t index);

bool arraysEqual(const ObjArray* a, const ObjArray* b);
bool structsEqual(const ObjStruct* a, const ObjStruct* b);
bool typesEqual(const ObjYargType* a, const ObjYargType* b);
bool pointersEqual(const ObjYargPointer* a, const ObjYargPointer* b);
bool intsEqual(const ObjInt* a, const ObjInt* b);

#endif