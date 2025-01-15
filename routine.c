#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#include "routine.h"

#include "memory.h"
#include "vm.h"

void initRoutine(ObjRoutine* thread, ThreadType type) {
    thread->type = type;
    thread->entryFunction = NULL;
    thread->state = EXEC_SUSPENDED;

    resetRoutine(thread);
}

void resetRoutine(ObjRoutine* thread) {
    thread->stackTop = thread->stack;
    thread->frameCount = 0;

    thread->openUpvalues = NULL;
}

ObjRoutine* newRoutine(ThreadType type) {
    ObjRoutine* thread = ALLOCATE_OBJ(ObjRoutine, OBJ_ROUTINE);
    initRoutine(thread, type);
    return thread;
}

void prepareRoutine(ObjRoutine* newThread, ObjClosure* closure) {

    newThread->entryFunction = closure;
    push(newThread, OBJ_VAL(closure));
    callfn(newThread, closure, 0);
}

void markRoutine(ObjRoutine* thread) {
    for (Value* slot = thread->stack; slot < thread->stackTop; slot++) {
        markValue(*slot);
    }

    for (int i = 0; i < thread->frameCount; i++) {
        markObject((Obj*)thread->frames[i].closure);
    }

    for (ObjUpvalue* upvalue = thread->openUpvalues;
        upvalue != NULL;
        upvalue = upvalue->next) {
        markObject((Obj*)upvalue);
    }

    markObject((Obj*)thread->entryFunction);
}

void runtimeError(ObjRoutine* thread, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = thread->frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &thread->frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[0x%8.x:%d][line %d] in ",
                thread,
                thread->type,
                function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    thread->state = EXEC_ERROR;
    resetRoutine(thread);
}

void push(ObjRoutine* thread, Value value) {
    *thread->stackTop = value;
    thread->stackTop++;

    if (thread->stackTop - &thread->stack[0] > STACK_MAX) {
        runtimeError(thread, "Value stack size exceeded.");
    }
}

Value pop(ObjRoutine* thread) {
    thread->stackTop--;
    return *thread->stackTop;
}

Value peek(ObjRoutine* thread, int distance) {
    return thread->stackTop[-1 - distance];
}
