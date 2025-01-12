#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#include "threadstack.h"

#include "memory.h"

void initThread(ObjThreadStack* thread, ThreadType type) {
    thread->type = type;
    thread->nextThread = NULL;
    thread->entryFunction = NULL;

    resetThread(thread);
}

void resetThread(ObjThreadStack* thread) {
    thread->stackTop = thread->stack;
    thread->frameCount = 0;

    thread->openUpvalues = NULL;
}

ObjThreadStack* newThread(ThreadType type) {
    ObjThreadStack* thread = ALLOCATE_OBJ(ObjThreadStack,
                                          OBJ_THREAD_STACK);
    initThread(thread, type);
    return thread;
}

void markThread(ObjThreadStack* thread) {
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

    markObject(thread->entryFunction);
}

void runtimeError(ObjThreadStack* thread, const char* format, ...) {
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
    resetThread(thread);
}

void push(ObjThreadStack* thread, Value value) {
    *thread->stackTop = value;
    thread->stackTop++;

    if (thread->stackTop - &thread->stack[0] > STACK_MAX) {
        runtimeError(thread, "Value stack size exceeded.");
    }
}

Value pop(ObjThreadStack* thread) {
    thread->stackTop--;
    return *thread->stackTop;
}

Value peek(ObjThreadStack* thread, int distance) {
    return thread->stackTop[-1 - distance];
}
