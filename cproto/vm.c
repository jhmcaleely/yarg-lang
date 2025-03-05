#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"
#include "native.h"
#include "builtin.h"
#include "routine.h"
#include "channel.h"

VM vm;

void fatalVMError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "fatal VM error\n");
    exit(5);
}

static void defineNative(const char* name, NativeFn function) {
    tempRootPush(OBJ_VAL(copyString(name, (int)strlen(name))));
    tempRootPush(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.tempRoots[0]), vm.tempRoots[1]);
    tempRootPop();
    tempRootPop();
}

void initVM() {
    
    // We have an Obj here not on the heap. hack up its init.
    vm.core0.obj.type = OBJ_ROUTINE;
    vm.core0.obj.isMarked = false;
    vm.core0.obj.next = NULL;
    initRoutine(&vm.core0, ROUTINE_THREAD);

    vm.core1 = NULL;

    recursive_mutex_init(&vm.heap);

    vm.tempRootsTop = vm.tempRoots;

    vm.objects = NULL;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;

    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;

    initTable(&vm.globals);
    initTable(&vm.strings);

    vm.initString = NULL;
    vm.initString = copyString("init", 4);

    defineNative("clock", clockNative);
    defineNative("sleep_ms", sleepNative);

    defineNative("gpio_init", gpioInitNative);

    defineNative("alarm_add_in_ms", alarmAddInMSNative);
    defineNative("alarm_add_repeating_ms", alarmAddRepeatingMSNative);
    defineNative("alarm_cancel_repeating", alarmCancelRepeatingMSNative);
}

void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    vm.initString = NULL;
    freeObjects();
}

bool callfn(ObjRoutine* routine, ObjClosure* closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError(routine, "Expected %d arguments but got %d.",
                     closure->function->arity, argCount);
        return false;
    }

    if (routine->frameCount == FRAMES_MAX) {
        runtimeError(routine, "Stack overflow.");
        return false;
    }

    CallFrame* frame = &routine->frames[routine->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = routine->stackTop - argCount - 1;
    return true;
}

static bool callValue(ObjRoutine* routine, Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                routine->stackTop[-argCount - 1] = bound->reciever;
                return callfn(routine, bound->method, argCount);
            }
            case OBJ_CLASS: {
                ObjClass* klass = AS_CLASS(callee);
                routine->stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
                Value initializer;
                if (tableGet(&klass->methods, vm.initString, &initializer)) {
                    return callfn(routine, AS_CLOSURE(initializer), argCount);
                } else if (argCount != 0) {
                    runtimeError(routine, "Expected 0 arguments but got %d.");
                    return false;
                }
                return true;
            }
            case OBJ_CLOSURE:
                return callfn(routine, AS_CLOSURE(callee), argCount);
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = NIL_VAL; 
                if (native(routine, argCount, routine->stackTop - argCount, &result)) {
                    routine->stackTop -= argCount + 1;
                    push(routine, result);
                    return true;
                } else {
                    return false;
                }
            }
            default:
                break; // Non-callable object type.
        }
    }
    runtimeError(routine, "Can only call functions and classes.");
    return false;
}

static bool invokeFromClass(ObjRoutine* routine, ObjClass* klass, ObjString* name,
                            int argCount) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeError(routine, "Undefined property '%s'.", name->chars);
        return false;
    }
    return callfn(routine, AS_CLOSURE(method), argCount);
}

static bool invoke(ObjRoutine* routine, ObjString* name, int argCount) {
    Value receiver = peek(routine, argCount);

    if (!IS_INSTANCE(receiver)) {
        runtimeError(routine, "Only instances have methods.");
        return false;
    }

    ObjInstance* instance = AS_INSTANCE(receiver);

    Value value;
    if (tableGet(&instance->fields, name, &value)) {
        routine->stackTop[-argCount - 1] = value;
        return callValue(routine, value, argCount);
    }

    return invokeFromClass(routine, instance->klass, name, argCount);
}

static bool bindMethod(ObjRoutine* routine, ObjClass* klass, ObjString* name) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeError(routine, "Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod* bound = newBoundMethod(peek(routine, 0), AS_CLOSURE(method));
    pop(routine);
    push(routine, OBJ_VAL(bound));
    return true;
}

static ObjUpvalue* captureUpvalue(ObjRoutine* routine, Value* local) {
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = routine->openUpvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL) {
        routine->openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(ObjRoutine* routine, Value* last) {
    while (routine->openUpvalues != NULL && routine->openUpvalues->location >= last) {
        ObjUpvalue* upvalue = routine->openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        routine->openUpvalues = upvalue->next;
    }
}

static void defineMethod(ObjRoutine* routine, ObjString* name) {
    Value method = peek(routine, 0);
    ObjClass* klass = AS_CLASS(peek(routine, 1));
    tableSet(&klass->methods, name, method);
    pop(routine);
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate(ObjRoutine* routine) {
    ObjString* b = AS_STRING(peek(routine, 0));
    ObjString* a = AS_STRING(peek(routine, 1));

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    pop(routine);
    pop(routine);
    push(routine, OBJ_VAL(result));
}

InterpretResult run(ObjRoutine* routine) {
    CallFrame* frame = &routine->frames[routine->frameCount - 1];
    routine->state = EXEC_RUNNING;

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() \
    (frame->ip += 2, \
    (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() \
    (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(routine, valueType, op) \
    do { \
        if (!IS_NUMBER(peek(routine, 0)) || !IS_NUMBER(peek(routine, 1))) { \
            runtimeError(routine, "Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop(routine)); \
        double a = AS_NUMBER(pop(routine)); \
        push(routine, valueType(a op b)); \
    } while (false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = routine->stack; slot < routine->stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(&frame->closure->function->chunk, 
                               (int)(frame->ip - frame->closure->function->chunk.code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(routine, constant);
                break;
            }
            case OP_NIL: push(routine, NIL_VAL); break;
            case OP_TRUE: push(routine, BOOL_VAL(true)); break;
            case OP_FALSE: push(routine, BOOL_VAL(false)); break;
            case OP_POP: pop(routine); break;
            case OP_GET_BUILTIN: {
                uint8_t builtin = READ_BYTE();
                switch (builtin) {
                    case BUILTIN_RPEEK: {
                        Value builtinFn = OBJ_VAL(newNative(rpeekBuiltin));
                        push(routine, builtinFn);
                        break;                        
                    }
                    case BUILTIN_RPOKE: {
                        Value builtinFn = OBJ_VAL(newNative(rpokeBuiltin));
                        push(routine, builtinFn);
                        break;                        
                    }
                    case BUILTIN_IMPORT: {
                        Value builtinFn = OBJ_VAL(newNative(importBuiltin));
                        push(routine, builtinFn);
                        break;                        
                    }
                    case BUILTIN_MAKE_ROUTINE: {
                        Value builtinFn = OBJ_VAL(newNative(makeRoutineBuiltin));
                        push(routine, builtinFn);
                        break;
                    }
                    case BUILTIN_RESUME: {
                        Value builtinFn = OBJ_VAL(newNative(resumeBuiltin));
                        push(routine, builtinFn);
                        break;
                    }
                    case BUILTIN_START: {
                        Value builtinFn = OBJ_VAL(newNative(startBuiltin));
                        push(routine, builtinFn);
                        break;
                    }
                    case BUILTIN_MAKE_CHANNEL: {
                        Value builtinFn = OBJ_VAL(newNative(makeChannelBuiltin));
                        push(routine, builtinFn);
                        break;
                    }
                    case BUILTIN_SEND: {
                        Value builtinFn = OBJ_VAL(newNative(sendChannelBuiltin));
                        push(routine, builtinFn);
                        break;
                    }
                    case BUILTIN_RECEIVE: {
                        Value builtinFn = OBJ_VAL(newNative(receiveChannelBuiltin));
                        push(routine, builtinFn);
                        break;
                    }
                    case BUILTIN_SHARE: {
                        Value builtinFn = OBJ_VAL(newNative(shareChannelBuiltin));
                        push(routine, builtinFn);
                        break;
                    }
                    case BUILTIN_PEEK: {
                        Value builtinFn = OBJ_VAL(newNative(peekChannelBuiltin));
                        push(routine, builtinFn);
                        break;
                    }
                }
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(routine, 0);
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(routine, frame->slots[slot]);
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError(routine, "Undefined variable (OP_GET_GLOBAL) '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(routine, value);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                tableSet(&vm.globals, name, peek(routine, 0));
                pop(routine);
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(routine, 0))) {
                    tableDelete(&vm.globals, name);
                    runtimeError(routine, "Undefined variable (OP_SET_GLOBAL) '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(routine, *frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(routine, 0);
                break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(routine, 0))) {
                    runtimeError(routine, "Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjInstance* instance = AS_INSTANCE(peek(routine, 0));
                ObjString* name = READ_STRING();

                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    pop(routine); // Instance
                    push(routine, value);
                    break;
                }

                if (!bindMethod(routine, instance->klass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(routine, 1))) {
                    runtimeError(routine, "Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjInstance* instance = AS_INSTANCE(peek(routine, 1));
                tableSet(&instance->fields, READ_STRING(), peek(routine, 0));
                Value value = pop(routine);
                pop(routine);
                push(routine, value);
                break;
            }
            case OP_GET_SUPER: {
                ObjString* name = READ_STRING();
                ObjClass* superclass = AS_CLASS(pop(routine));

                if (!bindMethod(routine, superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_EQUAL: {
                Value b = pop(routine);
                Value a = pop(routine);
                push(routine, BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:  BINARY_OP(routine, BOOL_VAL, >); break;
            case OP_LESS:     BINARY_OP(routine, BOOL_VAL, <); break;
            case OP_ADD: {
                if (IS_STRING(peek(routine, 0)) && IS_STRING(peek(routine, 1))) {
                    concatenate(routine);
                } else if (IS_NUMBER(peek(routine, 0)) && IS_NUMBER(peek(routine, 1))) {
                    double b = AS_NUMBER(pop(routine));
                    double a = AS_NUMBER(pop(routine));
                    push(routine, NUMBER_VAL(a + b));
                } else {
                    runtimeError(routine, "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(routine, NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(routine, NUMBER_VAL, *); break;
            case OP_DIVIDE:   BINARY_OP(routine, NUMBER_VAL, /); break;
            case OP_NOT:
                push(routine, BOOL_VAL(isFalsey(pop(routine))));
                break;
            case OP_NEGATE:
                if (!IS_NUMBER(peek(routine, 0))) {
                    runtimeError(routine, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(routine, NUMBER_VAL(-AS_NUMBER(pop(routine))));
                break;
            case OP_PRINT: {
                printValue(pop(routine));
                printf("\n");
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(routine, 0))) frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {
                int argCount = READ_BYTE();
                if (!callValue(routine, peek(routine, argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &routine->frames[routine->frameCount - 1];
                break;
            }
            case OP_INVOKE: {
                ObjString* method = READ_STRING();
                int argCount = READ_BYTE();
                if (!invoke(routine, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &routine->frames[routine->frameCount - 1];
                break;
            }
            case OP_SUPER_INVOKE: {
                ObjString* method = READ_STRING();
                int argCount = READ_BYTE();
                ObjClass* superclass = AS_CLASS(pop(routine));
                if (!invokeFromClass(routine, superclass, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &routine->frames[routine->frameCount - 1];
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = newClosure(function);
                push(routine, OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] = captureUpvalue(routine, frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_CLOSE_UPVALUE:
                closeUpvalues(routine, routine->stackTop - 1);
                pop(routine);
                break;
            case OP_YIELD: {
                if (routine == &vm.core0) {
                    runtimeError(routine, "Cannot yield from initial routine.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                routine->state = EXEC_SUSPENDED;
                return INTERPRET_OK;
            }
            case OP_RETURN: {
                Value result = pop(routine);
                closeUpvalues(routine, frame->slots);
                routine->frameCount--;
                if (routine->frameCount == 0) {
                    pop(routine);
                    push(routine, result);
                    routine->state = EXEC_CLOSED;
                    return INTERPRET_OK;
                }

                routine->stackTop = frame->slots;
                push(routine, result);
                frame = &routine->frames[routine->frameCount - 1];
                break;
            }
            case OP_CLASS:
                push(routine, OBJ_VAL(newClass(READ_STRING())));
                break;
            case OP_INHERIT: {
                Value superclass = peek(routine, 1);
                if (!IS_CLASS(superclass)) {
                    runtimeError(routine, "Superclass must be a class.");
                }

                ObjClass* subclass = AS_CLASS(peek(routine, 0));
                tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
                pop(routine); // Subclass.
                break;
            }
            case OP_METHOD:
                defineMethod(routine, READ_STRING());
                break;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {
    ObjFunction* function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    tempRootPush(OBJ_VAL(function));
    ObjClosure* closure = newClosure(function);
    tempRootPop();

    bindEntryFn(&vm.core0, closure);

    push(&vm.core0, OBJ_VAL(closure));
    callfn(&vm.core0, closure, 0);

    InterpretResult result = run(&vm.core0);

    pop(&vm.core0);

    return result;
}