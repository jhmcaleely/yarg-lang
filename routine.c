#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#include "routine.h"

#include "memory.h"
#include "vm.h"

void initRoutine(ObjRoutine* routine, RoutineKind type) {
    routine->type = type;
    routine->entryFunction = NULL;
    routine->state = EXEC_SUSPENDED;

    resetRoutine(routine);
}

void resetRoutine(ObjRoutine* routine) {
    routine->stackTop = routine->stack;
    routine->frameCount = 0;

    routine->openUpvalues = NULL;
}

ObjRoutine* newRoutine(RoutineKind type) {
    ObjRoutine* routine = ALLOCATE_OBJ(ObjRoutine, OBJ_ROUTINE);
    initRoutine(routine, type);
    return routine;
}

void prepareRoutineEntry(ObjRoutine* routine, ObjClosure* closure) {

    routine->entryFunction = closure;
}

void prepareRoutineStack(ObjRoutine* routine, int argCount, Value* args) {

    push(routine, OBJ_VAL(routine->entryFunction));

    for (int arg = 0; arg < routine->entryFunction->function->arity; arg++) {
        push(routine, args[arg]);
    }

    callfn(routine, routine->entryFunction, routine->entryFunction->function->arity);
}

void markRoutine(ObjRoutine* routine) {
    for (Value* slot = routine->stack; slot < routine->stackTop; slot++) {
        markValue(*slot);
    }

    for (int i = 0; i < routine->frameCount; i++) {
        markObject((Obj*)routine->frames[i].closure);
    }

    for (ObjUpvalue* upvalue = routine->openUpvalues;
        upvalue != NULL;
        upvalue = upvalue->next) {
        markObject((Obj*)upvalue);
    }

    markObject((Obj*)routine->entryFunction);
}

void runtimeError(ObjRoutine* routine, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = routine->frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &routine->frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "<R%s 0x%8.x>[line %d] in ",
                routine->type == ROUTINE_ISR ? "i" : "n",
                routine,
                function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    routine->state = EXEC_ERROR;
    resetRoutine(routine);
}

void push(ObjRoutine* routine, Value value) {
    *routine->stackTop = value;
    routine->stackTop++;

    if (routine->stackTop - &routine->stack[0] > STACK_MAX) {
        runtimeError(routine, "Value stack size exceeded.");
    }
}

Value pop(ObjRoutine* routine) {
    routine->stackTop--;
    return *routine->stackTop;
}

Value peek(ObjRoutine* routine, int distance) {
    return routine->stackTop[-1 - distance];
}
