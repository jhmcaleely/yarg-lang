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
    ROUTINE_THREAD,
    ROUTINE_ISR
} RoutineKind;

typedef enum {
    EXEC_RUNNING,
    EXEC_SUSPENDED,
    EXEC_CLOSED,
    EXEC_ERROR
} ExecState;

typedef struct ObjRoutine {
    Obj obj;

    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Value stack[STACK_MAX];
    Value* stackTop;

    ObjClosure* entryFunction;

    ObjUpvalue* openUpvalues;

    RoutineKind type;
    ExecState state;
} ObjRoutine;

void initRoutine(ObjRoutine* routine, RoutineKind type);
ObjRoutine* newRoutine(RoutineKind type);
void resetRoutine(ObjRoutine* routine);
void prepareRoutineEntry(ObjRoutine* routine, ObjClosure* closure);
void prepareRoutineStack(ObjRoutine* routine, int argCount, Value* args);

void markRoutine(ObjRoutine* routine);

void push(ObjRoutine* routine, Value value);
Value pop(ObjRoutine* routine);
Value peek(ObjRoutine* routine, int distance);

void runtimeError(ObjRoutine* routine, const char* format, ...);

#endif