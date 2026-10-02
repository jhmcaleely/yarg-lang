#ifndef cyarg_yargobject_h
#define cyarg_yargobject_h

#include "yargtype.h"

typedef struct ObjYarg {
    Obj          core;
    ObjYargType* type;
} ObjYarg;

typedef struct ObjArray {
    ObjYarg core;
    void*   elements;
} ObjArray;

ObjYarg* allocateYargObject(ObjYargType* type);

#endif