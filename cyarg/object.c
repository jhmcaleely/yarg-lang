#include "object.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "memory.h"
#include "yargobject.h"
#include "vmobject.h"
#include "table.h"
#include "value.h"
#include "vm.h"
#include "yargtype.h"
#include "channel.h"
#include "sync_group.h"
#include "yargstructtype.h"

#include "builtin.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    memset(object, 0, size);

    object->type = type;
    object->isMarked = false;

    vm_mutex_enter_blocking(&vm.heap);

    object->next = vm.objects;
    vm.objects = object;
    
    vm_mutex_exit(&vm.heap);

#ifdef DEBUG_LOG_GC
    PRINTERR("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

    return object;
}

ObjInt* allocateIntObject(size_t numDigits) {
    numDigits += numDigits % 2; // numDigits is always even
    assert(numDigits <= 254 && numDigits >= 2);
    ObjInt *i = (ObjInt *) allocateObject(sizeof (ObjInt) + numDigits * sizeof (uint16_t), OBJ_INT);
    i->bigInt.m_ = numDigits;
    return i;
}

void initDynamicObjArray(DynamicObjArray* array) {
    array->objects = NULL;
    array->stash = NULL;
    array->objectCapacity = 0;
    array->objectCount = 0;
}

void freeDynamicObjArray(DynamicObjArray* array) {
    FREE_ARRAY(Obj*, array->objects, array->objectCapacity);
    initDynamicObjArray(array);
}

void appendToDynamicObjArray(DynamicObjArray* array, Obj* obj) {
    array->stash = obj;
    if (array->objectCapacity < array->objectCount + 1) {
        int oldCapacity = array->objectCapacity;
        array->objectCapacity = GROW_CAPACITY(oldCapacity);
        array->objects = GROW_ARRAY(Obj*, array->objects, oldCapacity, array->objectCapacity);
    }
    array->objects[array->objectCount] = obj;
    array->objectCount++;
    array->stash = NULL;
}

Obj* removeLastFromDynamicObjArray(DynamicObjArray* array) {
    Obj* end = NULL;
    if (array->objectCount > 0) {
        end = array->objects[array->objectCount - 1];
        array->objectCount--;
    }
    return end;
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
    closure->cUpvalueCount = function->upvalueCount;
    return closure;
}

ObjFunction* newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    initFunction(function);
    return function;
}

void initFunction(ObjFunction* function) {
    // may not be called after alloc, so init all fields.
    function->arity = 0;
    function->upvalueCount = 0;
    function->fName = NULL;
    initChunk(&function->chunk);
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

ObjBuiltin* newBuiltin(BuiltinFun function) {
    ObjBuiltin* builtin = ALLOCATE_OBJ(ObjBuiltin, OBJ_BUILTIN);
    builtin->function = function;
    return builtin;
}

ObjInt* newInt(int64_t value) {
    ObjInt *i = allocateIntObject(sizeof value / sizeof (uint16_t));
    int_set_i(value, &i->bigInt);
    return i;
}

ObjInt* newIntU(uint64_t value) {
    ObjInt *i = allocateIntObject(sizeof value / sizeof (uint16_t));
    int_set_u(value, &i->bigInt);
    return i;
}

Value defaultIntValue() {
    ObjInt *intObj = allocateIntObject(1);
    int_init(&intObj->bigInt);
    return INTOBJ_VAL(intObj);
}

ObjMap* newMap(ObjConcreteYargTypeMap* type) {
    ObjMap* map = ALLOCATE_OBJ(ObjMap, OBJ_MAP);
    map->type = type;
    initTable(&map->entries);
    return map;
}

bool isAddressValue(Value val) {
    if (IS_INT(val)) {
        ObjInt *i = AS_INTOBJ(val);
        return i->isLiteral;
    } else if (IS_ADDRESS(val)) {
        return true;
    } else {
        return false;
    }
}

static ObjString* allocateString(char* chars, int length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    tempObjRootPush((Obj*)string);
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

ObjString* copyStringWithEscapes(const char* chars, int length)
{
    char* heapChars = ALLOCATE(char, length + 1);

    char const *in = chars;
    char *out = heapChars;
    int lengthOut = 0;
    for (int i = 0; i < length && *in != '\0'; i++)
    {
        if (*in == '\\')
        {
            in++;
            i++;
        }
        *out++ = *in++;
        lengthOut++;
    }
    uint32_t hash = hashString(heapChars, lengthOut);
    ObjString* interned = tableFindString(&vm.strings, heapChars, lengthOut, hash);
    if (interned != NULL)
    {
        FREE(char, heapChars);
        return interned;
    }

    *out = '\0';
    return allocateString(heapChars, lengthOut, hash);
}


ObjUpvalue* newUpvalue(ValueCell* slot, size_t stackOffset) {
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed.value = NIL_VAL;
    upvalue->closed.cellType = NULL;
    upvalue->contents = slot;
    upvalue->stackOffset = stackOffset;
    upvalue->next = NULL;
    return upvalue;
}

static ObjString* functionToString(const ObjFunction* function) {
    if (function->fName == NULL) {
        return copyString("<script>", 8);
    }
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "<fn %s>", function->fName->chars);
    return copyString(buffer, (int)strlen(buffer));
}

static ObjString* routineToString(const ObjRoutine* routine) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "<R%p>", routine);
    return copyString(buffer, (int)strlen(buffer));
}

ObjString* mapToString(const ObjMap* map) {
    ObjString* typeStr = valueToString(YARGTYPE_VAL(map->type));
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "<map (%d) %s >", map->entries.count, typeStr->chars);
    return copyString(buffer, (int)strlen(buffer));
}

ObjString* objectToString(const Obj* value) {
    switch (value->type) {
        case OBJ_BOUND_METHOD: {
            const ObjBoundMethod* boundMethod = (const ObjBoundMethod*)value;
            return functionToString(boundMethod->method->function);
        }
        case OBJ_CLASS: {
            const ObjClass* klass = (const ObjClass*)value;
            return copyString(klass->name->chars, klass->name->length);
        }
        case OBJ_CLOSURE: {
            const ObjClosure* closure = (const ObjClosure*)value;
            return functionToString(closure->function);
        }
        case OBJ_FUNCTION: {
            const ObjFunction* function = (const ObjFunction*)value;
            return functionToString(function);
        }
        case OBJ_INSTANCE: {
            char buffer[64];
            const ObjInstance* instance = (const ObjInstance*)value;
            snprintf(buffer, sizeof(buffer), "%s instance", instance->klass->name->chars);
            return copyString(buffer, (int)strlen(buffer));
            }
        case OBJ_NATIVE:
            return copyString("<native fn>", 11);
        case OBJ_BUILTIN:
            return builtinToString((const ObjBuiltin*)value);
        case OBJ_ROUTINE: {
            const ObjRoutine* routine = (const ObjRoutine*)value;
            return routineToString(routine);
        }
        case OBJ_CHANNELCONTAINER: {
            const ObjChannelContainer* channel = (const ObjChannelContainer*)value;
            return channelToString(channel);
        }
        case OBJ_SYNCGROUP: {
            const ObjSyncGroup* syncGroup = (const ObjSyncGroup*)value;
            return syncGroupToString(syncGroup);
        }
        case OBJ_STRING: {
            const ObjString* string = (const ObjString*)value;
            return copyString(string->chars, string->length);
        }
        case OBJ_UPVALUE: {
            const ObjUpvalue* upvalue = (const ObjUpvalue*)value;
            return copyString("upvalue", 7);
        }
        case OBJ_YARGTYPE:
        case OBJ_YARGTYPE_ARRAY:
        case OBJ_YARGTYPE_STRUCT:
        case OBJ_YARGTYPE_MAP: {
            const ObjConcreteYargType* type = (const ObjConcreteYargType*)value;
            return typeToString(type);
        }
        case OBJ_INT: {
            const ObjInt* intObj = (const ObjInt*)value;
            const Int *i = &intObj->bigInt;
            char sb[INT_STRLEN_FOR_INT254];
            char const* s = int_to_s(i, sb, INT_STRLEN_FOR_INT254);
            return copyString(s, (int)strlen(s));
        }
        case OBJ_MAP: {
            const ObjMap* map = (const ObjMap*)value;
            return mapToString(map);
        }
        default: {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "<implementation object %d>", value->type);
            return copyString(buffer, (int)strlen(buffer));
        }
    }
}
