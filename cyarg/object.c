#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"
#include "yargtype.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;

    object->next = vm.objects;
    vm.objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

    return object;
}

ObjBoundMethod* newBoundMethod(Value reciever, ObjClosure* method) {
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod,
                                         OBJ_BOUND_METHOD);
    bound->reciever = reciever;
    bound->method = method;
    return bound;
}

ObjClass* newClass(ObjString* name) {
    ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    initTable(&klass->methods);
    return klass;
}

ObjClosure* newClosure(ObjFunction* function) {
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

ObjFunction* newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

ObjInstance* newInstance(ObjClass* klass) {
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    return instance;
}

ObjNative* newNative(NativeFn function) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

ObjBlob* newBlob(size_t count) {
    ObjBlob* blob = ALLOCATE_OBJ(ObjBlob, OBJ_BLOB);
    blob->blob = NULL;
    
    tempRootPush(OBJ_VAL(blob));
    blob->blob = reallocate(NULL, 0, count);
    tempRootPop();
    return blob;
}

ObjValArray* newValArray(size_t capacity) {
    ObjValArray* array = ALLOCATE_OBJ(ObjValArray, OBJ_VALARRAY);
    initValueArray(&array->array);
    tempRootPush(OBJ_VAL(array));

    for (int i = 0; i < capacity; i++) {
        appendToValueArray(&array->array, NIL_VAL);
    }

    tempRootPop();
    return array;
}

ObjUniformArray* newUniformArray(ObjYargType* element_type, size_t capacity) {
    ObjUniformArray* array = ALLOCATE_OBJ(ObjUniformArray, OBJ_UNIFORMARRAY);
    array->array = NULL;
    array->element_size = 0;
    array->element_type = element_type;
    array->count = 0;

    tempRootPush(OBJ_VAL(array));

    if (is_obj_type(element_type)) {
        array->element_size = sizeof(Obj*);
        array->count = capacity;
        array->array = reallocate(array->array, 0, capacity * array->element_size);

        for (int i = 0; i < capacity; i++) {
            Obj** elements = (Obj**) array->array;
            elements[i] = NULL;
        }
    } else if (element_type->yt == TypeMachineUint32) {
        array->element_size = sizeof(uint32_t);
        array->count = capacity;
        array->array = reallocate(array->array, 0, capacity * array->element_size);

        for (int i = 0; i < capacity; i++) {
            uint32_t* elements = (uint32_t*) array->array;
            elements[i] = 0;
        }
    }

    tempRootPop();
    return array;
}

static ObjString* allocateString(char* chars, int length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    tempRootPush(OBJ_VAL(string));
    tableSet(&vm.strings, string, NIL_VAL);
    tempRootPop();
    return string;
}

static uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash += 16777619;
    }
    return hash;
}

ObjString* takeString(char* chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

ObjString* copyString(const char* chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

ObjUpvalue* newUpvalue(Value* slot) {
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
}

static void printFunction(ObjFunction* function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

static void printRoutine(ObjRoutine* routine) {
    printf("<R%s %p>"
          , routine->type == ROUTINE_ISR ? "i" : "n"
          , routine);
}

static void printChannel(ObjChannel* channel) {
    printf("<ch ");
    if (channel->present) {
        printValue(channel->data);
    }
    else {
        printf(" NIL");
    }
    printf(">");
}

static void printArray(Value a) {
    if (IS_VALARRAY(a)) {
        printf("[array %d]", AS_VALARRAY(a)->array.count);
    } else if (IS_UNIFORMARRAY(a)) {
        printf("[array %.zd]", AS_UNIFORMARRAY(a)->count);
    }
}

static void printType(ObjYargType* type) {
    switch (type->yt) {
        case TypeAny: printf("Type:Any"); break;
        case TypeBool: printf("Type:Bool"); break;
        case TypeNilVal: printf("Type:NilVal"); break;
        case TypeDouble: printf("Type:Double"); break;
        case TypeMachineUint32: printf("Type:TypeMachineUint32"); break;
        case TypeInteger: printf("Type:Integer"); break;
        case TypeString: printf("Type:String"); break;
        case TypeClass: printf("Type:Class"); break;
        case TypeInstance: printf("Type:%s", type->instanceKlass->name->chars); break;
        case TypeFunction: printf("Type:Function"); break;
        case TypeNativeBlob: printf("Type:NativeBlob"); break;
        case TypeRoutine: printf("Type:Routine"); break;
        case TypeChannel: printf("Type:Channel"); break;
        case TypeArray: printf("Type:Array"); break;
        case TypeYargType: printf("Type:Type"); break;
        default: printf("Type:Unknown"); break;
    }
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD:
            printFunction(AS_BOUND_METHOD(value)->method->function);
            break;
        case OBJ_CLASS:
            printf("%s", AS_CLASS(value)->name->chars);
            break;
        case OBJ_CLOSURE:
            printFunction(AS_CLOSURE(value)->function);
            break;
        case OBJ_FUNCTION:
            printFunction(AS_FUNCTION(value));
            break;
        case OBJ_INSTANCE:
            printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
            break;
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
        case OBJ_BLOB:
            printf("<blob %p>", AS_BLOB(value)->blob);
            break;
        case OBJ_ROUTINE:
            printRoutine(AS_ROUTINE(value));
            break;
        case OBJ_CHANNEL:
            printChannel(AS_CHANNEL(value));
            break;
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
        case OBJ_UPVALUE:
            printf("upvalue");
            break;
        case OBJ_VALARRAY:
            printArray(value);
            break;
        case OBJ_UNIFORMARRAY:
            printArray(value);
            break;
        case OBJ_YARGTYPE:
            printType(AS_YARGTYPE(value));
            break;
    }
}