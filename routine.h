#ifndef clox_routine_h
#define clox_routine_h

#include "value.h"
#include "object.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * (UINT8_COUNT / 2))

typedef struct {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef enum {
    THREAD_NORMAL,
    THREAD_ISR
} ThreadType;

typedef enum {
    EXEC_RUNNING,
    EXEC_SUSPENDED,
    EXEC_CLOSED,
    EXEC_ERROR
} ThreadState;

typedef struct ObjRoutine {
    Obj obj;

    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Value stack[STACK_MAX];
    Value* stackTop;

    ObjClosure* entryFunction;

    ObjUpvalue* openUpvalues;

    ThreadType type;
    ThreadState state;
} ObjRoutine;

void initRoutine(ObjRoutine* thread, ThreadType type);
ObjRoutine* newRoutine(ThreadType type);
void resetRoutine(ObjRoutine* thread);

void markRoutine(ObjRoutine* thread);

void push(ObjRoutine* thread, Value value);
Value pop(ObjRoutine* thread);
Value peek(ObjRoutine* thread, int distance);

void runtimeError(ObjRoutine* thread, const char* format, ...);

#endif