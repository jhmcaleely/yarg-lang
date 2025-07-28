#ifndef cyarg_vm_h
#define cyarg_vm_h

#include "object.h"
#include "table.h"
#include "value.h"

#include "memory.h"
#include "routine.h"
#include "platform_hal.h"

typedef struct {
    ObjRoutine core0;
    ObjRoutine* core1;

    Table globals;
    Table strings;
    ObjString* initString;

    platform_mutex heap;

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

InterpretResult run(ObjRoutine* routine);
bool callfn(ObjRoutine* routine, ObjClosure* closure, int argCount);
void fatalVMError(const char* format, ...);

#endif