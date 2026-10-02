#ifndef cyarg_yargobject_h
#define cyarg_yargobject_h

#include "yargtype.h"
#include "value.h"

typedef struct ObjYarg {
    Obj          core;
    const ObjYargType* type;
} ObjYarg;

typedef struct ObjYargPointer {
    ObjYarg  yarg;
    Obj* owner;
    void* target;
} ObjYargPointer;

typedef struct ObjArray {
    ObjYarg yarg;
    void*   elements;
} ObjArray;

typedef struct ObjValue ObjValue;

Obj* allocateYargObject(const ObjYargType* type);

ObjYargPointer* newYargPointerToObj(ObjYarg* target);
ObjYargPointer* newYargPointerToObjVal(ObjValue* target);
ObjYargPointer* newYargPointerToPlaceable(const ObjYargType* targetType, void* memory);

ObjYargPointer* arrayElement(ObjArray* array, size_t index);

#endif