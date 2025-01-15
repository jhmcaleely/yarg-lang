#ifndef clox_thread_stack_h
#define clox_thread_stack_h

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

typedef struct ObjThreadStack {
    Obj obj;

    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Value stack[STACK_MAX];
    Value* stackTop;

    ObjClosure* entryFunction;

    ObjUpvalue* openUpvalues;

    ThreadType type;
    ThreadState state;
} ObjThreadStack;

void initThread(ObjThreadStack* thread, ThreadType type);
ObjThreadStack* newThread(ThreadType type);
void resetThread(ObjThreadStack* thread);

void markThread(ObjThreadStack* thread);

void push(ObjThreadStack* thread, Value value);
Value pop(ObjThreadStack* thread);
Value peek(ObjThreadStack* thread, int distance);

void runtimeError(ObjThreadStack* thread, const char* format, ...);

#endif