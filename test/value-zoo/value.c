#include "value.h"
#include "yargtype.h"

Value unpackValue(PackedValue packedValue) {
    if (packedValue.storedType == NULL) {
        return packedValue.storedValue->asValue;
    } else {
        switch (packedValue.storedType->yt) {
            case TypeAny: return packedValue.storedValue->asValue;
            case TypeBool: return packedValue.storedValue->asValue;
            case TypeDouble: return packedValue.storedValue->asValue;
            case TypeInt8: return I8_VAL(packedValue.storedValue->as.i8);
            case TypeUint8: return UI8_VAL(packedValue.storedValue->as.ui8);
            case TypeInt16: return I16_VAL(packedValue.storedValue->as.i16);
            case TypeUint16: return UI16_VAL(packedValue.storedValue->as.ui16);
            case TypeInt32: return I32_VAL(packedValue.storedValue->as.i32);
            case TypeUint32: return UI32_VAL(packedValue.storedValue->as.ui32);
            case TypeInt64: return I64_VAL(packedValue.storedValue->as.i64);
            case TypeUint64: return UI64_VAL(packedValue.storedValue->as.ui64);
            case TypeAddress: return ADDRESS_VAL(packedValue.storedValue->as.address);
            case TypeStruct:
            case TypeArray:
            case TypeInt:
            case TypePointer:
            case TypeString:
            case TypeClass:
            case TypeInstance:
            case TypeFunction:
            case TypeRoutine:
            case TypeChannel:
            case TypeMap:
            case TypeYargType: {
                if (packedValue.storedValue->as.obj) {
                    return OBJ_VAL(packedValue.storedValue->as.obj);
                } else {
                    return NIL_VAL;
                }
            }
        }
    }
}

void initialisePackedValue(PackedValue packedValue) {

    if (packedValue.storedType == NULL) {
        packedValue.storedValue->asValue = NIL_VAL;
    } else {
        switch (packedValue.storedType->yt) {
            case TypeAny: packedValue.storedValue->asValue = NIL_VAL; break;
            case TypeBool: packedValue.storedValue->asValue = BOOL_VAL(false); break;
            case TypeDouble: packedValue.storedValue->asValue = DOUBLE_VAL(0); break;
            case TypeInt8: packedValue.storedValue->as.i8 = 0; break;
            case TypeUint8: packedValue.storedValue->as.ui8 = 0; break;
            case TypeInt16: packedValue.storedValue->as.i16 = 0; break;
            case TypeUint16: packedValue.storedValue->as.ui16 = 0; break;
            case TypeInt32: packedValue.storedValue->as.i32 = 0; break;
            case TypeUint32: packedValue.storedValue->as.ui32 = 0; break;
            case TypeInt64: packedValue.storedValue->as.i64 = 0; break;
            case TypeUint64: packedValue.storedValue->as.ui64 = 0; break;
            case TypeAddress: packedValue.storedValue->as.address = 0; break;
            case TypeArray: 
                break;
            case TypeStruct:
                break;
            case TypeInt:
            case TypePointer:
            case TypeString:
            case TypeClass:
            case TypeInstance:
            case TypeFunction:
            case TypeRoutine:
            case TypeChannel:
            case TypeMap:
            case TypeYargType: {
                packedValue.storedValue->as.obj = NULL;
                break;
            }
        }
    }
}

void packValue(PackedValue packedStorageTarget, Value value) {
    if (packedStorageTarget.storedType == NULL) {
        packedStorageTarget.storedValue->asValue = value;
    } else {
        switch (packedStorageTarget.storedType->yt) {
            case TypeAny: packedStorageTarget.storedValue->asValue = value; break;
            case TypeBool: packedStorageTarget.storedValue->asValue = value; break;
            case TypeDouble: packedStorageTarget.storedValue->asValue = value; break;
            case TypeInt8: packedStorageTarget.storedValue->as.i8 = AS_I8(value); break;
            case TypeUint8: packedStorageTarget.storedValue->as.ui8 = AS_UI8(value); break;
            case TypeInt16: packedStorageTarget.storedValue->as.i16 = AS_I16(value); break;
            case TypeUint16: packedStorageTarget.storedValue->as.ui16 = AS_UI16(value); break;
            case TypeInt32: packedStorageTarget.storedValue->as.i32 = AS_I32(value); break;
            case TypeUint32: packedStorageTarget.storedValue->as.ui32 = AS_UI32(value); break;
            case TypeInt64: packedStorageTarget.storedValue->as.i64 = AS_I64(value); break;
            case TypeUint64: packedStorageTarget.storedValue->as.ui64 = AS_UI64(value); break;
            case TypeAddress: packedStorageTarget.storedValue->as.address = AS_ADDRESS(value); break;
            case TypePointer:
            case TypeString:
            case TypeClass:
            case TypeInstance:
            case TypeFunction:
            case TypeRoutine:
            case TypeChannel:
            case TypeYargType:
            case TypeInt:
            case TypeMap: {
                packedStorageTarget.storedValue->as.obj = AS_OBJ(value);
                break;
            }
            case TypeStruct:
            case TypeArray:
                break;
        }
    }
}


bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:     return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:      return true;
        case VAL_DOUBLE:   return AS_DOUBLE(a) == AS_DOUBLE(b);
        case VAL_I8:       return AS_I8(a) == AS_I8(b);
        case VAL_UI8:      return AS_UI8(a) == AS_UI8(b);
        case VAL_I16:      return AS_I16(a) == AS_I16(b);
        case VAL_UI16:     return AS_UI16(a) == AS_UI16(b);
        case VAL_I32:      return AS_I32(a) == AS_I32(b);
        case VAL_UI32:     return AS_UI32(a) == AS_UI32(b);
        case VAL_I64:      return AS_I64(a) == AS_I64(b);
        case VAL_UI64:     return AS_UI64(a) == AS_UI64(b);
        case VAL_ADDRESS:  return AS_ADDRESS(a) == AS_ADDRESS(b);
        case VAL_OBJ:      return AS_OBJ(a) == AS_OBJ(b);
        default:           return false; // Unreachable.
    }
}
