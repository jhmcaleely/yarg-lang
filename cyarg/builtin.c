#include <stdlib.h>
#include <stdio.h>

#ifdef CYARG_PICO_TARGET
#include <pico/multicore.h>
#endif

#include "common.h"
#include "object.h"
#include "value.h"
#include "builtin.h"
#include "native.h"
#include "routine.h"
#include "vm.h"
#include "debug.h"
#include "files.h"
#include "compiler.h"
#include "channel.h"

InterpretResult interpretImport(const char* source) {

    ObjFunction* function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;
    tempRootPush(OBJ_VAL(function));

    ObjRoutine* routine = newRoutine(ROUTINE_THREAD);
    tempRootPush(OBJ_VAL(routine));

    ObjClosure* closure = newClosure(function);
    tempRootPop();
    tempRootPop();

    bindEntryFn(routine, closure);

    push(routine, OBJ_VAL(closure));
    callfn(routine, closure, 0);

    tempRootPush(OBJ_VAL(routine));
    InterpretResult result = run(routine);
    tempRootPop();
    
    pop(routine);

    return result;
}

static char* libraryNameFor(const char* importname) {
    size_t namelen = strlen(importname);
    char* filename = malloc(namelen + 5);
    if (filename) {
        strcpy(filename, importname);
        strcat(filename, ".ya");
    }
    return filename;
}

bool importBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result) {
    if (argCount != 1) {
        runtimeError(routineContext, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_STRING(args[0])) {
        runtimeError(routineContext, "Argument to import must be string.");
        return false;
    }

    char* source = NULL;
    char* library = libraryNameFor(AS_CSTRING(args[0]));
    if (library) {
        source = readFile(library);
        free(library);
    }

    if (source) {
        InterpretResult result = interpretImport(source);        
        free(source);

        if (result == INTERPRET_COMPILE_ERROR) {
            runtimeError(routineContext, "Interpret error");
            return false;
        }
        else if (result == INTERPRET_RUNTIME_ERROR) {
            runtimeError(routineContext, "Interpret error");
            return false;
        }
    }

    return true;
}

bool makeRoutineBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result) {
    if (argCount != 2) {
        runtimeError(routineContext, "Expected 2 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_CLOSURE(args[0]) || !IS_BOOL(args[1])) {
        runtimeError(routineContext, "Argument to make_routine must be a function and a boolean.");
        return false;
    }

    ObjClosure* closure = AS_CLOSURE(args[0]);
    bool isISR = AS_BOOL(args[1]);

    ObjRoutine* routine = newRoutine(isISR ? ROUTINE_ISR : ROUTINE_THREAD);

    if (bindEntryFn(routine, closure)) {
        *result = OBJ_VAL(routine);
        return true;
    }
    else {
        runtimeError(routineContext, "Function must take 0 or 1 arguments.");
        return false;
    }
}

bool resumeBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result) {
    if (argCount < 1 || argCount > 2) {
        runtimeError(routineContext, "Expected one or two arguments to resume.");
        return false;
    }

    if (!IS_ROUTINE(args[0])) {
        runtimeError(routineContext, "Argument to resume must be a routine.");
        return false;
    }

    ObjRoutine* target = AS_ROUTINE(args[0]);

    if (target->state != EXEC_SUSPENDED && target->state != EXEC_UNBOUND) {
        runtimeError(routineContext, "routine must be suspended or unbound to resume.");
        return false;
    }

    if (argCount > 1) {
        bindEntryArgs(target, args[1]);
    }

    if (target->state == EXEC_UNBOUND) {
        prepareRoutineStack(target);
        target->state = EXEC_SUSPENDED;
    }
    else if (target->state == EXEC_SUSPENDED) {
        push(target, target->entryArg);
    }

    InterpretResult execResult = run(target);
    if (execResult != INTERPRET_OK) {
        return false;
    }

    Value coroResult = pop(target);

    *result = coroResult;
    return true;
}

// cyarg: use ascii 'y' 'a' 'r' 'g'
#define FLAG_VALUE 0x79617267

#ifdef CYARG_PICO_TARGET
void nativeCore1Entry() {
    multicore_fifo_push_blocking(FLAG_VALUE);
    uint32_t g = multicore_fifo_pop_blocking();

    if (g != FLAG_VALUE) {
        fatalVMError("Core1 entry and sync failed.");
    }

    ObjRoutine* core = vm.core1;

    InterpretResult execResult = run(core);

    if (core->state != EXEC_ERROR) {
        core->state = EXEC_CLOSED;
    }

    vm.core1 = NULL;
}
#endif

bool startBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result) {
#ifdef CYARG_PICO_TARGET
    if (argCount < 1 || argCount > 2) {
        runtimeError(routineContext, "Expected one or two arguments to start.");
        return false;
    }

    if (!IS_ROUTINE(args[0])) {
        runtimeError(routineContext, "Argument to start must be a routine.");
        return false;
    }

    ObjRoutine* target = AS_ROUTINE(args[0]);

    if (target->state != EXEC_UNBOUND) {
        runtimeError(routineContext, "routine must be unbound to start.");
        return false;
    }

    if (vm.core1 != NULL) {
        runtimeError(routineContext, "Core1 already occupied.");
        return false;
    }

    if (argCount > 1) {
        bindEntryArgs(target, args[1]);
    }

    prepareRoutineStack(target);

    vm.core1 = target;
    vm.core1->state = EXEC_RUNNING;

    multicore_reset_core1();
    multicore_launch_core1(nativeCore1Entry);

    // Wait for it to start up
    uint32_t g = multicore_fifo_pop_blocking();
    if (g != FLAG_VALUE) {
        fatalVMError("Core1 startup failure.");
        return false;
    }
    multicore_fifo_push_blocking(FLAG_VALUE);

#endif
    *result = NIL_VAL;
    return true;
}

struct Register {
    volatile uint32_t value;
};

bool rpeekBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result) {
    
    uint32_t nominal_address = AS_UINTEGER(args[0]);
    uintptr_t memptr = (uint32_t) nominal_address;
    struct Register* reg = (struct Register*) memptr;

    uint32_t res = reg->value;

    *result = UINTEGER_VAL(res);
    return true;
}

bool rpokeBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result) {

    uint32_t nominal_address = AS_UINTEGER(args[0]);
    uintptr_t memptr = (uint32_t) nominal_address;
    struct Register* reg = (struct Register*) memptr;

    uint32_t val = AS_UINTEGER(args[1]);

    reg->value = val;

    return true;
}

bool makeArrayBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result) {

    if (argCount != 1) {
        runtimeError(routineContext, "Expected 1 argument, but got %d.", argCount);
        return false;
    }

    if (!IS_UINTEGER(args[0])) {
        runtimeError(routineContext, "Argument must be unsigned integer");
        return false;
    }

    uint32_t capcity = AS_UINTEGER(args[0]);

    ObjValArray* array = newValArray(capcity);

    *result = OBJ_VAL(array);
    return true;
}

bool setElementBuiltin(ObjRoutine* routineContext, int argCount, Value* args, Value* result) {

    ObjValArray* array = AS_VALARRAY(args[0]);
    int32_t index = AS_INTEGER(args[1]);

    if (index >= array->array.count) {
        runtimeError(routineContext, "Array index %d out of bounds (0:%d)", index, array->array.count - 1);
        return false;
    }

    Value val = args[2];

    array->array.values[index] = val;

    *result = val;

    return true;
}

Value getBuiltin(uint8_t builtin) {
    switch (builtin) {
        case BUILTIN_RPEEK: return OBJ_VAL(newNative(rpeekBuiltin));
        case BUILTIN_RPOKE: return OBJ_VAL(newNative(rpokeBuiltin));
        case BUILTIN_IMPORT: return OBJ_VAL(newNative(importBuiltin));
        case BUILTIN_MAKE_ARRAY: return OBJ_VAL(newNative(makeArrayBuiltin));
        case BUILTIN_MAKE_ROUTINE: return OBJ_VAL(newNative(makeRoutineBuiltin));
        case BUILTIN_RESUME: return OBJ_VAL(newNative(resumeBuiltin));
        case BUILTIN_START: return OBJ_VAL(newNative(startBuiltin));
        case BUILTIN_MAKE_CHANNEL: return OBJ_VAL(newNative(makeChannelBuiltin));
        case BUILTIN_SEND: return OBJ_VAL(newNative(sendChannelBuiltin));
        case BUILTIN_RECEIVE: return OBJ_VAL(newNative(receiveChannelBuiltin));
        case BUILTIN_SHARE: return OBJ_VAL(newNative(shareChannelBuiltin));
        case BUILTIN_PEEK: return OBJ_VAL(newNative(peekChannelBuiltin));
        case BUILTIN_SET_ELEMENT: return OBJ_VAL(newNative(setElementBuiltin));
        default: return NIL_VAL;
    }
}
