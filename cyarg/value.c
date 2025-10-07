#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"
#include "yargtype.h"

void initialisePackedStorage(Value type, StoredValue* packedStorage) {

    if (IS_NIL(type)) {
        packedStorage->asValue = NIL_VAL;
    } else {
        ObjConcreteYargType* ct = AS_YARGTYPE(type);
        switch (ct->yt) {
            case TypeAny: packedStorage->asValue = NIL_VAL; break;
            case TypeBool: packedStorage->asValue = BOOL_VAL(false); break;
            case TypeDouble: packedStorage->asValue = DOUBLE_VAL(0); break;
            case TypeInt8: packedStorage->as.i8 = 0; break;
            case TypeUint8: packedStorage->as.ui8 = 0; break;
            case TypeInt16: packedStorage->as.i16 = 0; break;
            case TypeUint16: packedStorage->as.ui16 = 0; break;
            case TypeInt32: packedStorage->as.i32 = 0; break;
            case TypeUint32: packedStorage->as.ui32 = 0; break;
            case TypeInt64: packedStorage->as.i64 = 0; break;
            case TypeUint64: packedStorage->as.ui64 = 0; break;
            case TypeArray: {
                ObjConcreteYargTypeArray* at = (ObjConcreteYargTypeArray*)ct;
                Value elementType = arrayElementType(at);
                if (at->cardinality > 0) {
                    for (size_t i = 0; i < at->cardinality; i++) {
                        initialisePackedStorage(elementType, arrayElement(at, packedStorage, i));
                    }
                }
                break;
            }
            case TypeStruct: {
                ObjConcreteYargTypeStruct* st = (ObjConcreteYargTypeStruct*)ct;
                for (size_t i = 0; i < st->field_count; i++) {
                    StoredValue* field = structField(st, packedStorage, i);
                    initialisePackedStorage(st->field_types[i], field);
                }
                break;
            }
            case TypePointer:
            case TypeString:
            case TypeClass:
            case TypeInstance:
            case TypeFunction:
            case TypeNativeBlob:
            case TypeRoutine:
            case TypeChannel:
            case TypeYargType: {
                packedStorage->as.obj = NULL;
                break;
            }
        }
    }
}

Value unpackStoredValue(Value type, StoredValue* packedStorage) {
    if (IS_NIL(type)) {
        return packedStorage->asValue;
    } else {
        ObjConcreteYargType* ct = AS_YARGTYPE(type);
        switch (ct->yt) {
            case TypeAny: return packedStorage->asValue;
            case TypeBool: return packedStorage->asValue;
            case TypeDouble: return packedStorage->asValue;
            case TypeInt8: return I8_VAL(packedStorage->as.i8);
            case TypeUint8: return UI8_VAL(packedStorage->as.ui8);
            case TypeInt16: return I16_VAL(packedStorage->as.i16);
            case TypeUint16: return UI16_VAL(packedStorage->as.ui16);
            case TypeInt32: return I32_VAL(packedStorage->as.i32);
            case TypeUint32: return UI32_VAL(packedStorage->as.ui32);
            case TypeInt64: return I64_VAL(packedStorage->as.i64);
            case TypeUint64: return UI64_VAL(packedStorage->as.ui64);
            case TypeStruct: {
                return OBJ_VAL(newPackedStructAt((ObjConcreteYargTypeStruct*)ct, packedStorage));
            }
            case TypeArray: {
                return OBJ_VAL(newPackedUniformArrayAt((ObjConcreteYargTypeArray*)ct, packedStorage));
            }
            case TypePointer:
            case TypeString:
            case TypeClass:
            case TypeInstance:
            case TypeFunction:
            case TypeNativeBlob:
            case TypeRoutine:
            case TypeChannel:
            case TypeYargType: {
                if (packedStorage->as.obj) {
                    return OBJ_VAL(packedStorage->as.obj);
                } else {
                    return NIL_VAL;
                }
            }
        }
    }
}

void packValueStorage(StoredValueTarget packedStorageTarget, Value value) {
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
            case TypePointer:
            case TypeString:
            case TypeClass:
            case TypeInstance:
            case TypeFunction:
            case TypeNativeBlob:
            case TypeRoutine:
            case TypeChannel:
            case TypeYargType: {
                packedStorageTarget.storedValue->as.obj = AS_OBJ(value);
                break;
            }
            case TypeStruct:
            case TypeArray:
                break;
        }
    }
}

void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void appendToValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

void printValue(Value value) {
    fprintValue(stdout, value);
}

void fprintValue(FILE* op, Value value) {
    switch (value.type) {
        case VAL_BOOL:
            FPRINTMSG(op, AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL: FPRINTMSG(op, "nil"); break;
        case VAL_DOUBLE: FPRINTMSG(op, "%#g", AS_DOUBLE(value)); break;
        case VAL_I8: FPRINTMSG(op, "%d", AS_I8(value)); break;
        case VAL_UI8: FPRINTMSG(op, "%u", AS_UI8(value)); break;
        case VAL_I16: FPRINTMSG(op, "%d", AS_I16(value)); break;
        case VAL_UI16: FPRINTMSG(op, "%u", AS_UI16(value)); break;
        case VAL_I32: FPRINTMSG(op, "%d", AS_I32(value)); break;
        case VAL_UI32: FPRINTMSG(op, "%u", AS_UI32(value)); break;
        case VAL_I64: FPRINTMSG(op, "%lld", AS_I64(value)); break;
        case VAL_UI64: FPRINTMSG(op, "%llu", AS_UI64(value)); break;
        case VAL_ADDRESS: FPRINTMSG(op, "%p", (void*) AS_ADDRESS(value)); break;
        case VAL_OBJ: fprintObject(op, value); break;
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

bool is_positive_integer(Value a) {
    if (IS_UI32(a) || IS_UI16(a) || IS_UI8(a)) {
        return true;
    } else if (IS_UI64(a) && AS_UI64(a) <= UINT32_MAX) {
        return true;
    } else if (IS_I32(a) && AS_I32(a) >= 0) {
        return true;
    } else if (IS_I16(a) && AS_I16(a) >= 0) {
        return true;
    } else if (IS_I8(a) && AS_I8(a) >= 0) {
        return true;
    } else if (IS_I64(a) && AS_I64(a) >= 0) {
        return true;
    }
    return false;
}

uint32_t as_positive_integer(Value a) {
    if (IS_I32(a)) {
        return AS_I32(a);
    } else if (IS_I8(a)) {
        return AS_I8(a);
    } else if (IS_I16(a)) {
        return AS_I16(a);
    } else if (IS_I64(a) && AS_I64(a) <= UINT32_MAX) {
        return AS_I64(a);
    } else if (IS_UI32(a)) {
        return AS_UI32(a);
    } else if (IS_UI8(a)) {
        return AS_UI8(a);
    } else if (IS_UI16(a)) {
        return AS_UI16(a);
    } else if (IS_UI64(a) && AS_UI64(a) <= UINT32_MAX) {
        return AS_UI64(a);
    }
    return 0;
}