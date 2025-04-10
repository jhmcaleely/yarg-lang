#include "common.h"

#include "yargtype.h"

#include "memory.h"
#include "vm.h"

ObjYargType* newYargType(Value a) {
    ObjYargType* t = ALLOCATE_OBJ(ObjYargType, OBJ_YARGTYPE);
    t->yt = yt_typeof(a);
    if (t->yt == TypeInstance) {
        ObjInstance* instance = AS_INSTANCE(a);
        t->instanceKlass = instance->klass;
    } else {
        t->instanceKlass = NULL;
    }
    return t;
}

YargType yt_typeof(Value a) {
    if (IS_BOOL(a)) {
        return TypeBool;
    } else if (IS_NIL(a)) {
        return TypeNilVal;
    } else if (IS_DOUBLE(a)) {
        return TypeDouble;
    } else if (IS_UINTEGER(a)) {
        return TypeMachineUint32;
    } else if (IS_INTEGER(a)) {
        return TypeInteger;
    } else if (IS_BOUND_METHOD(a)) {
        return TypeFunction; //?
    } else if (IS_CLASS(a)) {
        return TypeClass;
    } else if (IS_CLOSURE(a)) {
        return TypeFunction;
    } else if (IS_INSTANCE(a)) {
        return TypeInstance;
    } else if (IS_NATIVE(a)) {
        return TypeFunction;
    } else if (IS_BLOB(a)) {
        return TypeNativeBlob;
    } else if (IS_ROUTINE(a)) {
        return TypeRoutine;
    } else if (IS_CHANNEL(a)) {
        return TypeChannel;
    } else if (IS_STRING(a)) {
        return TypeString;
    } else if (IS_VALARRAY(a)) {
        return TypeArray;
    } else if (IS_YARGTYPE(a)) {
        return TypeYargType;
    }
    fatalVMError("Unexpected object type");
    return TypeNilVal;
}

bool is_obj_type(ObjYargType* type) {
    switch (type->yt) {
        case TypeAny:
        case TypeBool:
        case TypeNilVal:
        case TypeDouble:
        case TypeMachineUint32:
        case TypeInteger:
            return false;
        case TypeString:
        case TypeClass:
        case TypeInstance:
        case TypeFunction:
        case TypeNativeBlob:
        case TypeRoutine:
        case TypeChannel:
        case TypeArray:
        case TypeYargType:
            return true;
    }
}