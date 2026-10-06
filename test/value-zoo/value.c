#include "value.h"

#include "yarg-runtime.h"
#include "yargobject.h"

bool is_positive_integer(Value a) {
    if (IS_I8(a)) return AS_I8(a) > 0;
    if (IS_UI8(a)) return AS_UI8(a);
    if (IS_I16(a)) return AS_I16(a) > 0;
    if (IS_UI16(a)) return AS_UI16(a);
    if (IS_I32(a)) return AS_I32(a) > 0;
    if (IS_UI32(a)) return AS_UI32(a);
#if IS_64BIT
    if (IS_I64(a)) return AS_I64(a) > 0;
    if (IS_UI64(a)) return AS_UI64(a);
#else
    if (IS_I64(a)) return AS_I64(a) > 0 && AS_I64(a) <= UINT32_MAX;
    if (IS_UI64(a)) return AS_UI64(a) <= UINT32_MAX;
#endif
    return false;
}

size_t as_positive_integer(Value a) {
    if (!is_positive_integer(a)) {
        panic();
        return 0;
    }
    if (IS_I8(a)) return (size_t)AS_I8(a);
    if (IS_UI8(a)) return (size_t)AS_UI8(a);
    if (IS_I16(a)) return (size_t)AS_I16(a);
    if (IS_UI16(a)) return (size_t)AS_UI16(a);
    if (IS_I32(a)) return (size_t)AS_I32(a);
    if (IS_UI32(a)) return (size_t)AS_UI32(a);
    if (IS_I64(a)) return (size_t)AS_I64(a);
    if (IS_UI64(a)) return (size_t)AS_UI64(a);
    return 0;   
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    if (a.type == NULL && a.as.obj == NULL) return true; // NIL_VAL

    switch (a.type->yt) {
        case TypeBool:     return AS_BOOL(a) == AS_BOOL(b);
        case TypeDouble:   return AS_DOUBLE(a) == AS_DOUBLE(b);
        case TypeInt8:     return AS_I8(a) == AS_I8(b);
        case TypeUint8:    return AS_UI8(a) == AS_UI8(b);
        case TypeInt16:    return AS_I16(a) == AS_I16(b);
        case TypeUint16:   return AS_UI16(a) == AS_UI16(b);
        case TypeInt32:    return AS_I32(a) == AS_I32(b);
        case TypeUint32:   return AS_UI32(a) == AS_UI32(b);
        case TypeInt64:    return AS_I64(a) == AS_I64(b);
        case TypeUint64:   return AS_UI64(a) == AS_UI64(b);
        case TypeAddress:  return AS_ADDRESS(a) == AS_ADDRESS(b);
        case TypeArray:    return arraysEqual(AS_ARRAY(a), AS_ARRAY(b));
        case TypeStruct:   return structsEqual(AS_STRUCT(a), AS_STRUCT(b));
        case TypeYargType: return typesEqual(AS_YARGTYPE(a), AS_YARGTYPE(b));
        case TypePointer:  return pointersEqual(AS_POINTER(a), AS_POINTER(b));
        case TypeInt:      return intsEqual(AS_INT(a), AS_INT(b));

        case TypeMap:
        case TypeString:
        case TypeRoutine:
        case TypeClass:
        case TypeInstance:
        case TypeFunction:
        case TypeChannel:
            return AS_YARG_OBJ(a) == AS_YARG_OBJ(b);
    }
    panic();
    return false;
}
