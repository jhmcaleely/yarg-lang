#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * (UINT8_COUNT / 2))

#define THREADS_MAX 2
#define ALLOCATION_STASH_MAX 4

typedef struct {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Value stack[STACK_MAX];
    Value* stackTop;

    ObjUpvalue* openUpvalues;
} Thread;

typedef struct {
    Thread threads[THREADS_MAX];

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
void push(int thread, Value value);
Value pop(int thread);

void stash_push(Value value);
Value stash_pop();

InterpretResult run(int thread);
bool callfn(int thread, ObjClosure* closure, int argCount);
void runtimeError(int thread, const char* format, ...);
void fatalMemoryError(const char* format, ...);

#endif