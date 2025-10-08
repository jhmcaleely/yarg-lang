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
#include "yargtype.h"

bool importBuiltinDummy(ObjRoutine* routineContext, int argCount, Value* result) {
    *result = NIL_VAL;
    return true;
}

static char* libraryNameFor(const char* importname) {
    size_t namelen = strlen(importname);
    char* filename = malloc(namelen + 3 + 1);
    if (filename) {
        strcpy(filename, importname);
        strcat(filename, ".ya");
    }
    return filename;
}

bool importBuiltin(ObjRoutine* routineContext, int argCount) {
    if (argCount != 1) {
        runtimeError(routineContext, "Expected 1 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_STRING(peek(routineContext, 0))) {
        runtimeError(routineContext, "Argument to import must be string.");
        return false;
    }

    Value val;
    if (tableGet(&vm.imports, AS_STRING(peek(routineContext, 0)), &val)) {
        pop(routineContext);
        pop(routineContext);
        push(routineContext, NIL_VAL);
        return true;
    }

    char* source = NULL;
    char* library = libraryNameFor(AS_CSTRING(peek(routineContext, 0)));
    if (library) {
        source = readFile(library);
        free(library);
    }

    if (source) {
        ObjFunction* function = compile(source);
        free(source);
        if (function == NULL) {
            runtimeError(routineContext, "Interpret error; compiling source failed.");
            return false;
        }

        tempRootPush(OBJ_VAL(function));

        Value libstring = pop(routineContext);
        tempRootPush(libstring);
        pop(routineContext);

        ObjClosure* closure = newClosure(function);
        push(routineContext, OBJ_VAL(closure));

        tableSet(&vm.imports, AS_STRING(libstring), BOOL_VAL(true));

        tempRootPop();
        tempRootPop();

        callfn(routineContext, closure, 0);
        return true;
    }
    else {
        runtimeError(routineContext, "source not found");
        return false;
    }

}

bool makeChannelBuiltin(ObjRoutine* routine, int argCount, Value* result) {
    if (argCount >= 2) {
        runtimeError(routine, "Expected 0 or 1 arguments but got %d.", argCount);
        return false;
    }
    Value valCapacity;
    if (argCount == 0) {
        valCapacity = UI32_VAL(1);
    } else {
        Value arg1 = nativeArgument(routine, argCount, 0);
        if (!is_positive_integer(arg1)) {
            runtimeError(routine, "Expected a positive integer");
        }
        valCapacity = arg1;
    }

    size_t capacity = as_positive_integer(valCapacity);

    ObjChannelContainer* channel = newChannel(routine, capacity);

    *result = OBJ_VAL((Obj*)channel);
    return true;
}

bool sendChannelBuiltin(ObjRoutine* routine, int argCount, Value* result) {
    if (argCount != 2) {
        runtimeError(routine, "Expected 2 arguments, got %d.", argCount);
        return false;
    }

    Value channelVal = nativeArgument(routine, argCount, 0);

    if (!IS_CHANNEL(channelVal)) {
        runtimeError(routine, "Argument must be a channel.");
        return false;
    }

    ObjChannelContainer* channel = AS_CHANNEL(channelVal);
    Value data = nativeArgument(routine, argCount, 1);
    sendChannel(channel, data);

    return true;
}

bool shareChannelBuiltin(ObjRoutine* routine, int argCount, Value* result) {
    if (argCount != 2) {
        runtimeError(routine, "Expected 2 arguments, got %d.", argCount);
        return false;
    }

    Value channelVal = nativeArgument(routine, argCount, 0);
    
    if (!IS_CHANNEL(channelVal)) {
        runtimeError(routine, "Argument must be a channel.");
        return false;
    }

    ObjChannelContainer* channel = AS_CHANNEL(channelVal);
    Value data = nativeArgument(routine, argCount, 1);
    bool overflow = shareChannel(channel, data);
    *result = BOOL_VAL(overflow);

    return true;
}

bool cpeekBuiltin(ObjRoutine* routine, int argCount, Value* result) {
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 arguments, got %d.", argCount);
        return false;
    }

    Value channelVal = nativeArgument(routine, argCount, 0);

    if (!IS_CHANNEL(channelVal)) {
        runtimeError(routine, "Argument must be a channel.");
        return false;
    }

    ObjChannelContainer* channel = AS_CHANNEL(channelVal);
    *result = peekChannel(channel);

    return true;
}

bool makeRoutineBuiltin(ObjRoutine* routineContext, int argCount, Value* result) {
    if (argCount != 2) {
        runtimeError(routineContext, "Expected 2 arguments but got %d.", argCount);
        return false;
    }
    if (!IS_CLOSURE(nativeArgument(routineContext, argCount, 0)) || !IS_BOOL(nativeArgument(routineContext, argCount, 1))) {
        runtimeError(routineContext, "Argument to make_routine must be a function and a boolean.");
        return false;
    }

    ObjClosure* closure = AS_CLOSURE(nativeArgument(routineContext, argCount, 0));
    bool isISR = AS_BOOL(nativeArgument(routineContext, argCount, 1));

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

bool resumeBuiltin(ObjRoutine* routineContext, int argCount, Value* result) {
    if (argCount < 1 || argCount > 2) {
        runtimeError(routineContext, "Expected one or two arguments to resume.");
        return false;
    }

    if (!IS_ROUTINE(nativeArgument(routineContext, argCount, 0))) {
        runtimeError(routineContext, "Argument to resume must be a routine.");
        return false;
    }

    ObjRoutine* target = AS_ROUTINE(nativeArgument(routineContext, argCount, 0));

    if (target->state != EXEC_SUSPENDED && target->state != EXEC_UNBOUND) {
        runtimeError(routineContext, "routine must be suspended or unbound to resume.");
        return false;
    }

    if (argCount > 1) {
        bindEntryArgs(target, nativeArgument(routineContext, argCount, 1));
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

bool startBuiltin(ObjRoutine* routineContext, int argCount, Value* result) {
#ifdef CYARG_PICO_TARGET
    if (argCount < 1 || argCount > 2) {
        runtimeError(routineContext, "Expected one or two arguments to start.");
        return false;
    }

    Value routineVal = nativeArgument(routineContext, argCount, 0);

    if (!IS_ROUTINE(routineVal)) {
        runtimeError(routineContext, "Argument to start must be a routine.");
        return false;
    }

    ObjRoutine* target = AS_ROUTINE(routineVal);

    if (target->state != EXEC_UNBOUND) {
        runtimeError(routineContext, "routine must be unbound to start.");
        return false;
    }

    if (vm.core1 != NULL) {
        runtimeError(routineContext, "Core1 already occupied.");
        return false;
    }

    if (argCount > 1) {
        bindEntryArgs(target, nativeArgument(routineContext, argCount, 1));
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

bool receiveBuiltin(ObjRoutine* routine, int argCount, Value* result) {
    if (argCount != 1) {
        runtimeError(routine, "Expected 1 arguments, got %d.", argCount);
        return false;
    }

    Value channelVal = nativeArgument(routine, argCount, 0);

    if (!IS_CHANNEL(channelVal) && !IS_ROUTINE(channelVal)) {
        runtimeError(routine, "Argument must be a channel or a routine.");
        return false;
    }

    if (IS_CHANNEL(channelVal)) {
        *result = receiveChannel(AS_CHANNEL(channelVal));
    } 
    else if (IS_ROUTINE(channelVal)) {
        ObjRoutine* routineParam = AS_ROUTINE(channelVal);

#ifdef CYARG_PICO_TARGET
        while (routineParam->state == EXEC_RUNNING) {
            tight_loop_contents();
        }
#endif    
        
        if (routineParam->state == EXEC_CLOSED || routineParam->state == EXEC_SUSPENDED) {
            *result = peek(routineParam, -1);
        } 
        else {
            *result = NIL_VAL;
        }
    }
    return true;
}

bool peekBuiltin(ObjRoutine* routineContext, int argCount, Value* result) {

    Value address = nativeArgument(routineContext, argCount, 0);

    if (!isAddressValue(address)) {
        runtimeError(routineContext, "Expected an address or pointer.");
        return false;
    }

    uintptr_t nominal_address = 0;
    if (IS_POINTER(address)) {
        nominal_address = (uintptr_t) AS_POINTER(address)->destination;
    } else {
        nominal_address = AS_ADDRESS(address);
    }
    volatile uint32_t* reg = (volatile uint32_t*) nominal_address;

#ifdef CYARG_PICO_TARGET
    uint32_t res = *reg;
    *result = UI32_VAL(res);
#else
    printf("peek(%p)\n", (void*)nominal_address);
    *result = UI32_VAL(0);
#endif
    return true;
}

bool lenBuiltin(ObjRoutine* routineContext, int argCount, Value* result) {
    if (argCount != 1) {
        runtimeError(routineContext, "Expected 1 argument, but got %d.", argCount);
        return false;
    }

    Value arg = nativeArgument(routineContext, argCount, 0);

    if (IS_STRING(arg)) {
        ObjString* string = AS_STRING(arg);
        *result = UI32_VAL(string->length);
        return true;
    } else if (IS_UNIFORMARRAY(arg)) {
        ObjPackedUniformArray* array = AS_UNIFORMARRAY(arg);
        *result = UI32_VAL(arrayCardinality(array->store));
        return true;
    } else {
        runtimeError(routineContext, "Expected a string or array.");
        return false;
    }
}

bool pinBuiltin(ObjRoutine* routineContext, int argCount, Value* result) {
    if (argCount != 1) {
        runtimeError(routineContext, "Expected 1 argument, but got %d.", argCount);
        return false;
    }

    Value arg = nativeArgument(routineContext, argCount, 0);

    if (!IS_ROUTINE(arg)) {
        runtimeError(routineContext, "Argument to pin must be a routine.");
        return false;
    }

    for (size_t i = 0; i < MAX_PINNED_ROUTINES; i++) {
        if (vm.pinnedRoutines[i] == NULL) {
            vm.pinnedRoutines[i] = AS_ROUTINE(arg);
            *result = ADDRESS_VAL((uintptr_t)vm.pinnedRoutineHandlers[i]);
            return true;
        }
    }

    runtimeError(routineContext, "No more pinned routines available.");
    *result = NIL_VAL;
    return false;
}

bool newBuiltin(ObjRoutine* routineContext, int argCount, Value* result) {

    Value typeToCreate = NIL_VAL;
    if (   argCount == 1
        && IS_YARGTYPE(nativeArgument(routineContext, argCount, 0))) {
        typeToCreate = nativeArgument(routineContext, argCount, 0);
    }

    ConcreteYargType typeRequested = IS_NIL(typeToCreate) ? TypeAny : AS_YARGTYPE(typeToCreate)->yt;
    switch (typeRequested) {
        case TypeAny:    // fall through
        case TypeBool:
        case TypeDouble:
        case TypeInt8:
        case TypeUint8:
        case TypeInt16:
        case TypeUint16:
        case TypeInt32:
        case TypeUint32:
        case TypeInt64:
        case TypeUint64: {
            PackedValue heapValue = allocPackedValue(typeToCreate);
            initialisePackedValue(heapValue);
            *result = OBJ_VAL(newPointerForHeapCell(heapValue));
            return true;
        }
        case TypeStruct: {
            PackedValue heapValue = allocPackedValue(typeToCreate);
            initialisePackedValue(heapValue);
            *result = OBJ_VAL(newPointerForHeapCell(heapValue));
            return true;
        }
        case TypeArray: {
            *result = defaultValue(typeToCreate);
            return true;
        }
        case TypeString:
        case TypeClass:
        case TypeInstance:
        case TypeFunction:
        case TypeNativeBlob:
        case TypeRoutine:
        case TypeChannel:
        case TypePointer: 
        case TypeYargType:
            return false; // unsupported for now.
    }

    return false;
}

bool uint64Builtin(ObjRoutine* routineContext, int argCount, Value* result) {
    Value arg = nativeArgument(routineContext, argCount, 0);
    if (IS_I8(arg)) {
        *result = UI64_VAL(AS_I8(arg));
        return true;
    } else if (IS_UI8(arg)) {
        *result = UI64_VAL(AS_UI8(arg));
        return true;
    } else if (IS_I16(arg)) {
        *result = UI64_VAL(AS_I16(arg));
        return true;
    } else if (IS_UI16(arg)) {
        *result = UI64_VAL(AS_UI16(arg));
        return true;
    } else if (IS_I32(arg)) {
        *result = UI64_VAL(AS_I32(arg));
        return true;
    } else if (IS_UI32(arg)) {
        *result = UI64_VAL(AS_UI32(arg));
        return true;
    } else if (IS_UI64(arg)) {
        *result = arg;
        return true;
    } else if (IS_I64(arg) && AS_I64(arg) >= 0) {
        *result = UI64_VAL(AS_I64(arg));
        return true;
    }
    return false;
}

bool int64Builtin(ObjRoutine* routineContext, int argCount, Value* result) {
    Value arg = nativeArgument(routineContext, argCount, 0);
    if (IS_I8(arg)) {
        *result = I64_VAL(AS_I8(arg));
        return true;
    } else if (IS_I16(arg)) {
        *result = I64_VAL(AS_I16(arg));
        return true;
    } else if (IS_I32(arg)) {
        *result = I64_VAL(AS_I32(arg));
        return true;
    } else if (IS_I64(arg)) {
        *result = arg;
        return true;
    } else if (IS_UI8(arg)) {
        *result = I64_VAL(AS_UI8(arg));
        return true;
    } else if (IS_UI16(arg)) {
        *result = I64_VAL(AS_UI16(arg));
        return true;
    } else if (IS_UI32(arg)) {
        *result = I64_VAL(AS_UI32(arg));
        return true;
    } else if (IS_UI64(arg) && AS_UI64(arg) <= INT64_MAX) {
        *result = I64_VAL(AS_UI64(arg));
        return true;
    } else {
        return false;
    }
}

bool uint32Builtin(ObjRoutine* routineContext, int argCount, Value* result) {
    Value arg = nativeArgument(routineContext, argCount, 0);
    if (IS_I8(arg) && AS_I8(arg) >= 0) {
        *result = UI32_VAL(AS_I8(arg));
        return true;
    } else if (IS_I16(arg) && AS_I16(arg) >= 0) {
        *result = UI32_VAL(AS_I16(arg));
        return true;
    } else if (IS_I32(arg) && AS_I32(arg) >= 0) {
        *result = UI32_VAL(AS_I32(arg));
        return true;
    } else if (IS_I64(arg) && AS_I64(arg) >= 0 && AS_I64(arg) < UINT32_MAX) {
        *result = UI32_VAL(AS_I64(arg));
        return true;
    } else if (IS_UI8(arg)) {
        *result = UI32_VAL(AS_UI8(arg));
        return true;
    } else if (IS_UI16(arg)) {
        *result = UI32_VAL(AS_UI16(arg));
        return true;
    } else if (IS_UI32(arg)) {
        *result = arg;
        return true;
    } else if (IS_UI64(arg) && AS_UI64(arg) <= UINT32_MAX) {
        *result = UI32_VAL(AS_UI64(arg));
        return true;
    }
    return false;
}

bool int32Builtin(ObjRoutine* routineContext, int argCount, Value* result) {
    Value arg = nativeArgument(routineContext, argCount, 0);
    if (IS_I8(arg)) {
        *result = I32_VAL(AS_I8(arg));
        return true;
    } else if (IS_I16(arg)) {
        *result = I32_VAL(AS_I16(arg));
        return true;
    } else if (IS_I32(arg)) {
        *result = arg;
        return true;
    } else if (IS_I64(arg) && AS_I64(arg) >= INT32_MIN && AS_I64(arg) <= INT32_MAX) {
        *result = I32_VAL(AS_I64(arg));
        return true;
    } else if (IS_UI8(arg)) {
        *result = I32_VAL(AS_UI8(arg));
        return true;
    } else if (IS_UI16(arg)) {
        *result = I32_VAL(AS_UI16(arg));
        return true;
    } else if (IS_UI32(arg) && AS_UI32(arg) <= INT32_MAX) {
        *result = I32_VAL(AS_UI32(arg));
        return true;
    } else if (IS_UI64(arg) && AS_UI64(arg) <= INT32_MAX) {
        *result = I32_VAL(AS_UI64(arg));
        return true;
    } else {
        return false;
    }
}

bool uint16Builtin(ObjRoutine* routineContext, int argCount, Value* result) {
    Value arg = nativeArgument(routineContext, argCount, 0);
    if (IS_I8(arg) && AS_I8(arg) >= 0) {
        *result = UI16_VAL(AS_I8(arg));
        return true;
    } else if (IS_I16(arg) && AS_I16(arg) >= 0) {
        *result = UI16_VAL(AS_I16(arg));
        return true;
    } else if (IS_I32(arg) && AS_I32(arg) >= 0 && AS_I32(arg) <= UINT16_MAX) {
        *result = UI16_VAL(AS_I32(arg));
        return true;
    } else if (IS_I64(arg) && AS_I64(arg) >= 0 && AS_I64(arg) <= UINT16_MAX) {
        *result = UI16_VAL(AS_I64(arg));
        return true;
    } else if (IS_UI8(arg)) {
        *result = UI16_VAL(AS_UI8(arg));
        return true;
    } else if (IS_UI16(arg)) {
        *result = arg;
        return true;
    } else if (IS_UI32(arg) && AS_UI32(arg) <= UINT16_MAX) {
        *result = UI16_VAL(AS_UI32(arg));
        return true;
    } else if (IS_UI64(arg) && AS_UI64(arg) <= UINT16_MAX) {
        *result = UI16_VAL(AS_UI64(arg));
        return true;
    } else {
        return false;
    }
}

bool int16Builtin(ObjRoutine* routineContext, int argCount, Value* result) {
    Value arg = nativeArgument(routineContext, argCount, 0);
    if (IS_I8(arg)) {
        *result = I16_VAL(AS_I8(arg));
        return true;
    } else if (IS_I16(arg)) {
        *result = arg;
        return true;
    } else if (IS_I32(arg) && AS_I32(arg) >= INT16_MIN && AS_I32(arg) <= INT16_MAX) {
        *result = I16_VAL(AS_I32(arg));
        return true;
    } else if (IS_I64(arg) && AS_I64(arg) >= INT16_MIN && AS_I64(arg) <= INT16_MAX) {
        *result = I16_VAL(AS_I64(arg));
        return true;
    } else if (IS_UI8(arg)) {
        *result = I16_VAL(AS_UI8(arg));
        return true;
    } else if (IS_UI16(arg) && AS_UI16(arg) <= INT16_MAX) {
        *result = I16_VAL(AS_UI16(arg));
        return true;
    } else if (IS_UI32(arg) && AS_UI32(arg) <= INT16_MAX) {
        *result = I16_VAL(AS_UI32(arg));
        return true;
    } else if (IS_UI64(arg) && AS_UI64(arg) <= INT16_MAX) {
        *result = I16_VAL(AS_UI64(arg));
        return true;
    } else {
        return false;
    }
}

bool uint8Builtin(ObjRoutine* routineContext, int argCount, Value* result) {
    Value arg = nativeArgument(routineContext, argCount, 0);
    if (IS_I8(arg) && AS_I8(arg) >= 0) {
        *result = UI8_VAL(AS_I8(arg));
        return true;
    } else if (IS_I16(arg) && AS_I16(arg) >= 0 && AS_I16(arg) <= UINT8_MAX) {
        *result = UI8_VAL(AS_I16(arg));
        return true;
    } else if (IS_I32(arg) && AS_I32(arg) >= 0 && AS_I32(arg) <= UINT8_MAX) {
        *result = UI8_VAL(AS_I32(arg));
        return true;
    } else if (IS_I64(arg) && AS_I64(arg) >= 0 && AS_I64(arg) <= UINT8_MAX) {
        *result = UI8_VAL(AS_I64(arg));
        return true;
    } else if (IS_UI8(arg)) {
        *result = arg;
        return true;
    } else if (IS_UI32(arg) && AS_UI32(arg) <= UINT8_MAX) {
        *result = UI8_VAL(AS_UI32(arg));
        return true;
    } else if (IS_UI16(arg) && AS_UI16(arg) <= UINT8_MAX) {
        *result = UI8_VAL(AS_UI16(arg));
        return true;
    } else if (IS_UI64(arg) && AS_UI64(arg) <= UINT8_MAX) {
        *result = UI8_VAL(AS_UI64(arg));
        return true;
    } else {
        return false;
    }
}

bool int8Builtin(ObjRoutine* routineContext, int argCount, Value* result) {
    Value arg = nativeArgument(routineContext, argCount, 0);
    if (IS_I8(arg)) {
        *result = arg;
        return true;
    } else if (IS_I16(arg) && AS_I16(arg) >= INT8_MIN && AS_I16(arg) <= INT8_MAX) {
        *result = I8_VAL(AS_I16(arg));
        return true;
    } else if (IS_I32(arg) && AS_I32(arg) >= INT8_MIN && AS_I32(arg) <= INT8_MAX) {
        *result = I8_VAL(AS_I32(arg));
        return true;
    } else if (IS_I64(arg) && AS_I64(arg) >= INT8_MIN && AS_I64(arg) <= INT8_MAX) {
        *result = I8_VAL(AS_I64(arg));
        return true;
    } else if (IS_UI8(arg) && AS_UI8(arg) <= INT8_MAX) {
        *result = I8_VAL(AS_UI8(arg));
        return true;
    } else if (IS_UI16(arg) && AS_UI16(arg) <= INT8_MAX) {
        *result = I8_VAL(AS_UI16(arg));
        return true;
    } else if (IS_UI32(arg) && AS_UI32(arg) <= INT8_MAX) {
        *result = I8_VAL(AS_UI32(arg));
        return true;
    } else if (IS_UI64(arg) && AS_UI64(arg) <= INT8_MAX) {
        *result = I8_VAL(AS_UI64(arg));
        return true;
    } else {
        return false;
    }
}

Value getBuiltin(uint8_t builtin) {
    switch (builtin) {
        case BUILTIN_PEEK: return OBJ_VAL(newNative(peekBuiltin));
        case BUILTIN_IMPORT: return OBJ_VAL(newNative(importBuiltinDummy));
        case BUILTIN_MAKE_ROUTINE: return OBJ_VAL(newNative(makeRoutineBuiltin));
        case BUILTIN_RESUME: return OBJ_VAL(newNative(resumeBuiltin));
        case BUILTIN_START: return OBJ_VAL(newNative(startBuiltin));
        case BUILTIN_MAKE_CHANNEL: return OBJ_VAL(newNative(makeChannelBuiltin));
        case BUILTIN_SEND: return OBJ_VAL(newNative(sendChannelBuiltin));
        case BUILTIN_RECEIVE: return OBJ_VAL(newNative(receiveBuiltin));
        case BUILTIN_SHARE: return OBJ_VAL(newNative(shareChannelBuiltin));
        case BUILTIN_CPEEK: return OBJ_VAL(newNative(cpeekBuiltin));
        case BUILTIN_LEN: return OBJ_VAL(newNative(lenBuiltin));
        case BUILTIN_PIN: return OBJ_VAL(newNative(pinBuiltin));
        case BUILTIN_NEW: return OBJ_VAL(newNative(newBuiltin));
        case BUILTIN_INT8: return OBJ_VAL(newNative(int8Builtin));
        case BUILTIN_INT16: return OBJ_VAL(newNative(int16Builtin));
        case BUILTIN_UINT16: return OBJ_VAL(newNative(uint16Builtin));
        case BUILTIN_UINT8: return OBJ_VAL(newNative(uint8Builtin));
        case BUILTIN_INT32: return OBJ_VAL(newNative(int32Builtin));
        case BUILTIN_UINT32: return OBJ_VAL(newNative(uint32Builtin));
        case BUILTIN_INT64: return OBJ_VAL(newNative(int64Builtin));
        case BUILTIN_UINT64: return OBJ_VAL(newNative(uint64Builtin));
        default: return NIL_VAL;
    }
}
