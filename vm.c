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
#include "threadstack.h"
#include "channel.h"

VM vm;

void fatalMemoryError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "fatal memory error, exit(5)\n");
    exit(5);
}

static void defineNative(const char* name, NativeFn function) {
    stash_push(OBJ_VAL(copyString(name, (int)strlen(name))));
    stash_push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.allocationStash[0]), vm.allocationStash[1]);
    stash_pop();
    stash_pop();
}

void initVM() {
    initThread(&vm.core0, THREAD_NORMAL);

    vm.coroList = NULL;

    vm.isrStack = NULL;
    vm.isrCount = 0;

    vm.allocationTop = vm.allocationStash;

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
    defineNative("gpio_set_direction", gpioSetDirectionNative);
    defineNative("gpio_put", gpioPutNative);

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

void stash_push(Value value) {
    *vm.allocationTop = value;
    vm.allocationTop++;

    if (vm.allocationTop - &vm.allocationStash[0] > ALLOCATION_STASH_MAX) {
        fatalMemoryError("Allocation Stash Max Exeeded.");
    }
}

Value stash_pop() {
    vm.allocationTop--;
    return *vm.allocationTop;
}

bool callfn(ObjThreadStack* thread, ObjClosure* closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError(thread, "Expected %d arguments but got %d.",
                     closure->function->arity, argCount);
        return false;
    }

    if (thread->frameCount == FRAMES_MAX) {
        runtimeError(thread, "Stack overflow.");
        return false;
    }

    CallFrame* frame = &thread->frames[thread->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = thread->stackTop - argCount - 1;
    return true;
}

static bool callValue(ObjThreadStack* thread, Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                thread->stackTop[-argCount - 1] = bound->reciever;
                return callfn(thread, bound->method, argCount);
            }
            case OBJ_CLASS: {
                ObjClass* klass = AS_CLASS(callee);
                thread->stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
                Value initializer;
                if (tableGet(&klass->methods, vm.initString, &initializer)) {
                    return callfn(thread, AS_CLOSURE(initializer), argCount);
                } else if (argCount != 0) {
                    runtimeError(thread, "Expected 0 arguments but got %d.");
                    return false;
                }
                return true;
            }
            case OBJ_CLOSURE:
                return callfn(thread, AS_CLOSURE(callee), argCount);
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = NIL_VAL; 
                if (native(thread, argCount, thread->stackTop - argCount, &result)) {
                    thread->stackTop -= argCount + 1;
                    push(thread, result);
                    return true;
                } else {
                    return false;
                }
            }
            default:
                break; // Non-callable object type.
        }
    }
    runtimeError(thread, "Can only call functions and classes.");
    return false;
}

static bool invokeFromClass(ObjThreadStack* thread, ObjClass* klass, ObjString* name,
                            int argCount) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeError(thread, "Undefined property '%s'.", name->chars);
        return false;
    }
    return callfn(thread, AS_CLOSURE(method), argCount);
}

static bool invoke(ObjThreadStack* thread, ObjString* name, int argCount) {
    Value receiver = peek(thread, argCount);

    if (!IS_INSTANCE(receiver)) {
        runtimeError(thread, "Only instances have methods.");
        return false;
    }

    ObjInstance* instance = AS_INSTANCE(receiver);

    Value value;
    if (tableGet(&instance->fields, name, &value)) {
        thread->stackTop[-argCount - 1] = value;
        return callValue(thread, value, argCount);
    }

    return invokeFromClass(thread, instance->klass, name, argCount);
}

static bool bindMethod(ObjThreadStack* thread, ObjClass* klass, ObjString* name) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeError(thread, "Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod* bound = newBoundMethod(peek(thread, 0), AS_CLOSURE(method));
    pop(thread);
    push(thread, OBJ_VAL(bound));
    return true;
}

static ObjUpvalue* captureUpvalue(ObjThreadStack* thread, Value* local) {
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = thread->openUpvalues;
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
        thread->openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(ObjThreadStack* thread, Value* last) {
    while (thread->openUpvalues != NULL && thread->openUpvalues->location >= last) {
        ObjUpvalue* upvalue = thread->openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        thread->openUpvalues = upvalue->next;
    }
}

static void defineMethod(ObjThreadStack* thread, ObjString* name) {
    Value method = peek(thread, 0);
    ObjClass* klass = AS_CLASS(peek(thread, 1));
    tableSet(&klass->methods, name, method);
    pop(thread);
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate(ObjThreadStack* thread) {
    ObjString* b = AS_STRING(peek(thread, 0));
    ObjString* a = AS_STRING(peek(thread, 1));

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    pop(thread);
    pop(thread);
    push(thread, OBJ_VAL(result));
}

InterpretResult run(ObjThreadStack* thread) {
    CallFrame* frame = &thread->frames[thread->frameCount - 1];
    thread->state = EXEC_RUNNING;

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() \
    (frame->ip += 2, \
    (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() \
    (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(thread, valueType, op) \
    do { \
        if (!IS_NUMBER(peek(thread, 0)) || !IS_NUMBER(peek(thread, 1))) { \
            runtimeError(thread, "Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop(thread)); \
        double a = AS_NUMBER(pop(thread)); \
        push(thread, valueType(a op b)); \
    } while (false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.threads[thread].stack; slot < vm.threads[thread].stackTop; slot++) {
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
                push(thread, constant);
                break;
            }
            case OP_NIL: push(thread, NIL_VAL); break;
            case OP_TRUE: push(thread, BOOL_VAL(true)); break;
            case OP_FALSE: push(thread, BOOL_VAL(false)); break;
            case OP_POP: pop(thread); break;
            case OP_GET_BUILTIN: {
                uint8_t builtin = READ_BYTE();
                switch (builtin) {
                    case BUILTIN_MAKE_ISR: {
                        Value builtinFn = OBJ_VAL(newNative(makeIsrBuiltin));
                        push(thread, builtinFn);
                        break;
                    }
                    case BUILTIN_MAKE_CORO: {
                        Value builtinFn = OBJ_VAL(newNative(makeCoroBuiltin));
                        push(thread, builtinFn);
                        break;
                    }
                    case BUILTIN_RESUME: {
                        Value builtinFn = OBJ_VAL(newNative(resumeBuiltin));
                        push(thread, builtinFn);
                        break;
                    }
                    case BUILTIN_MAKE_CHANNEL: {
                        Value builtinFn = OBJ_VAL(newNative(makeChannelBuiltin));
                        push(thread, builtinFn);
                        break;
                    }
                    case BUILTIN_SEND: {
                        Value builtinFn = OBJ_VAL(newNative(sendChannelBuiltin));
                        push(thread, builtinFn);
                        break;
                    }
                    case BUILTIN_RECEIVE: {
                        Value builtinFn = OBJ_VAL(newNative(receiveChannelBuiltin));
                        push(thread, builtinFn);
                        break;
                    }
                    case BUILTIN_SHARE: {
                        Value builtinFn = OBJ_VAL(newNative(shareChannelBuiltin));
                        push(thread, builtinFn);
                        break;
                    }
                    case BUILTIN_PEEK: {
                        Value builtinFn = OBJ_VAL(newNative(peekChannelBuiltin));
                        push(thread, builtinFn);
                        break;
                    }
                }
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(thread, 0);
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(thread, frame->slots[slot]);
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError(thread, "Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(thread, value);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                tableSet(&vm.globals, name, peek(thread, 0));
                pop(thread);
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(thread, 0))) {
                    tableDelete(&vm.globals, name);
                    runtimeError(thread, "Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(thread, *frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(thread, 0);
                break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(thread, 0))) {
                    runtimeError(thread, "Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjInstance* instance = AS_INSTANCE(peek(thread, 0));
                ObjString* name = READ_STRING();

                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    pop(thread); // Instance
                    push(thread, value);
                    break;
                }

                if (!bindMethod(thread, instance->klass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(thread, 1))) {
                    runtimeError(thread, "Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjInstance* instance = AS_INSTANCE(peek(thread, 1));
                tableSet(&instance->fields, READ_STRING(), peek(thread, 0));
                Value value = pop(thread);
                pop(thread);
                push(thread, value);
                break;
            }
            case OP_GET_SUPER: {
                ObjString* name = READ_STRING();
                ObjClass* superclass = AS_CLASS(pop(thread));

                if (!bindMethod(thread, superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_EQUAL: {
                Value b = pop(thread);
                Value a = pop(thread);
                push(thread, BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:  BINARY_OP(thread, BOOL_VAL, >); break;
            case OP_LESS:     BINARY_OP(thread, BOOL_VAL, <); break;
            case OP_ADD: {
                if (IS_STRING(peek(thread, 0)) && IS_STRING(peek(thread, 1))) {
                    concatenate(thread);
                } else if (IS_NUMBER(peek(thread, 0)) && IS_NUMBER(peek(thread, 1))) {
                    double b = AS_NUMBER(pop(thread));
                    double a = AS_NUMBER(pop(thread));
                    push(thread, NUMBER_VAL(a + b));
                } else {
                    runtimeError(thread, "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(thread, NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(thread, NUMBER_VAL, *); break;
            case OP_DIVIDE:   BINARY_OP(thread, NUMBER_VAL, /); break;
            case OP_NOT:
                push(thread, BOOL_VAL(isFalsey(pop(thread))));
                break;
            case OP_NEGATE:
                if (!IS_NUMBER(peek(thread, 0))) {
                    runtimeError(thread, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(thread, NUMBER_VAL(-AS_NUMBER(pop(thread))));
                break;
            case OP_PRINT: {
                printValue(pop(thread));
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
                if (isFalsey(peek(thread, 0))) frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {
                int argCount = READ_BYTE();
                if (!callValue(thread, peek(thread, argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &thread->frames[thread->frameCount - 1];
                break;
            }
            case OP_INVOKE: {
                ObjString* method = READ_STRING();
                int argCount = READ_BYTE();
                if (!invoke(thread, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &thread->frames[thread->frameCount - 1];
                break;
            }
            case OP_SUPER_INVOKE: {
                ObjString* method = READ_STRING();
                int argCount = READ_BYTE();
                ObjClass* superclass = AS_CLASS(pop(thread));
                if (!invokeFromClass(thread, superclass, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &thread->frames[thread->frameCount - 1];
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = newClosure(function);
                push(thread, OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] = captureUpvalue(thread, frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_CLOSE_UPVALUE:
                closeUpvalues(thread, thread->stackTop - 1);
                pop(thread);
                break;
            case OP_YIELD: {
                if (thread == &vm.core0) {
                    runtimeError(thread, "Cannot yield from initial thread.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value result = pop(thread); // ignored for now
                thread->state = EXEC_SUSPENDED;
                return INTERPRET_OK;
            }
            case OP_RETURN: {
                Value result = pop(thread);
                closeUpvalues(thread, frame->slots);
                thread->frameCount--;
                if (thread->frameCount == 0) {
                    pop(thread);
                    return INTERPRET_OK;
                }

                thread->stackTop = frame->slots;
                push(thread, result);
                frame = &thread->frames[thread->frameCount - 1];
                break;
            }
            case OP_CLASS:
                push(thread, OBJ_VAL(newClass(READ_STRING())));
                break;
            case OP_INHERIT: {
                Value superclass = peek(thread, 1);
                if (!IS_CLASS(superclass)) {
                    runtimeError(thread, "Superclass must be a class.");
                }

                ObjClass* subclass = AS_CLASS(peek(thread, 0));
                tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
                pop(thread); // Subclass.
                break;
            }
            case OP_METHOD:
                defineMethod(thread, READ_STRING());
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

    stash_push(OBJ_VAL(function));
    ObjClosure* closure = newClosure(function);
    stash_pop();
    push(&vm.core0, OBJ_VAL(closure));
    callfn(&vm.core0, closure, 0);

    return run(&vm.core0);
}