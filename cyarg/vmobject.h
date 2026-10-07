#ifndef cyarg_vmobject_h
#define cyarg_vmobject_h

#include "object.h"

typedef struct ObjUpvalue {
    Obj obj;
    ValueCell* contents;
    size_t stackOffset;
    ValueCell closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct ObjClosure {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int cUpvalueCount;
} ObjClosure;

typedef struct {
    Obj obj;
    Value reciever;
    ObjClosure* method;
} ObjBoundMethod;

typedef bool (*NativeFn)(ObjRoutine* routine, int argCount, Value* result);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

ObjUpvalue* newUpvalue(ValueCell* slot, size_t stackOffset);
ObjClosure* newClosure(ObjFunction* function);
ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);
ObjNative* newNative(NativeFn function);


#endif