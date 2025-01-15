#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "table.h"
#include "value.h"

#include "memory.h"
#include "routine.h"

typedef struct {
    ObjRoutine core0;
    ObjRoutine* core1;

    Table globals;
    Table strings;
    ObjString* initString;

    Value tempRoots[TEMP_ROOTS_MAX];
    Value* tempRootsTop;

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



InterpretResult run(ObjRoutine* thread);
bool callfn(ObjRoutine* thread, ObjClosure* closure, int argCount);
void fatalMemoryError(const char* format, ...);

#endif