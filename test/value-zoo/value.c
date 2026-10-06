#include "value.h"

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
        default:           return false;
    }
}
