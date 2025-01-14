#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "table.h"
#include "value.h"
#include "threadstack.h"

typedef struct {
    ObjThreadStack core0;

    Table globals;
    Table strings;
    ObjString* initString;

    Value allocationStash[ALLOCATION_STASH_MAX];
    Value* allocationTop;

    size_t bytesAllocated;
    size_t nextGC;
    Obj* objects;
    int grayCount;
    int grayCapacity;
    Obj** grayStack;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);

void stash_push(Value value);
Value stash_pop();

InterpretResult run(ObjThreadStack* thread);
bool callfn(ObjThreadStack* thread, ObjClosure* closure, int argCount);
void fatalMemoryError(const char* format, ...);

#endif