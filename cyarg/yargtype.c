#include "yargtype.h"

#include <string.h>

#include "common.h"
#include "memory.h"
#include "vm.h"
#include "yargobject.h"
#include "vmobject.h"
#include "yargstructtype.h"


SimpleYargTypes yargTypes = {
    .boolean = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeBool },
    .address = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeAddress },
    .dbl = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeDouble },
    .int8 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeInt8 },
    .uint8 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeUint8 },
    .int16 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeInt16 },
    .uint16 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeUint16 },
    .int32 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeInt32 },
    .uint32 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeUint32 },
    .int64 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeInt64 },
    .uint64 = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeUint64 },
    .type = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeYargType },
    .class_ = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeClass },
    .string = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeString },
    .integer = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeInt },
    .function = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeFunction },
    .routine = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeRoutine },
    .channel = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeChannel },
    .syncGroup = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = TypeSyncGroup },
    .implementation_obj = { .obj.isMarked = true, .obj.next = NULL, .obj.type = OBJ_YARGTYPE, .yt = 0 },
};


bool is_obj_yargtype(const ObjConcreteYargType* type) {
    if (type == NULL) return false;
    else if (type == &yargTypes.implementation_obj) return true;
    else if (type == &yargTypes.class_) return true;
    else if (type == &yargTypes.string) return true;
    else if (type == &yargTypes.type) return true;
    else if (type == &yargTypes.integer) return true;
    else if (type == &yargTypes.function) return true;
    else if (type->yt == TypeStruct) return true;
    else if (type->yt == TypeArray) return true;
    else if (type->yt == TypePointer) return true;
    else if (type->yt == TypeChannel) return true;
    else if (type->yt == TypeSyncGroup) return true;
    else if (type->yt == TypeRoutine) return true;
    else if (type->yt == TypeMap) return true;
    return false;
}

ObjConcreteYargType* newYargTypeFromType(ConcreteYargType yt) {
    switch (yt) {
        case TypeBool: return &yargTypes.boolean;
        case TypeInt: return &yargTypes.integer;
        case TypeDouble: return &yargTypes.dbl;  
        case TypeInt8: return &yargTypes.int8;
        case TypeUint8: return &yargTypes.uint8;
        case TypeInt16: return &yargTypes.int16;
        case TypeUint16: return &yargTypes.uint16;
        case TypeInt32: return &yargTypes.int32;
        case TypeUint32: return &yargTypes.uint32;
        case TypeInt64: return &yargTypes.int64;
        case TypeUint64: return &yargTypes.uint64;
        case TypeAddress: return &yargTypes.address;
        case TypeClass: return &yargTypes.class_;
        case TypeString: return &yargTypes.string;
        case TypeYargType: return &yargTypes.type;

        case TypeInstance:
        case TypeFunction:
        case TypeRoutine:
        case TypeSyncGroup: 
        case TypeChannel: {
            ObjConcreteYargType* t = ALLOCATE_OBJ(ObjConcreteYargType, OBJ_YARGTYPE);
            t->yt = yt;
            return t;
        }
        case TypeArray: {
            ObjConcreteYargTypeArray* t = ALLOCATE_OBJ(ObjConcreteYargTypeArray, OBJ_YARGTYPE_ARRAY);
            t->core.yt = yt;
            return (ObjConcreteYargType*)t;
        }
        case TypeStruct: {
            ObjConcreteYargTypeStruct* s = ALLOCATE_OBJ(ObjConcreteYargTypeStruct, OBJ_YARGTYPE_STRUCT);
            s->alignment = 1;
            s->core.yt = yt;
            initTable(&s->field_names);
            return (ObjConcreteYargType*)s;
        }
        case TypePointer: {
            ObjConcreteYargTypePointer* p = ALLOCATE_OBJ(ObjConcreteYargTypePointer, OBJ_YARGTYPE_POINTER);
            p->core.yt = yt;
            return (ObjConcreteYargType*)p;
        }
        case TypeMap: {
            ObjConcreteYargTypeMap* m = ALLOCATE_OBJ(ObjConcreteYargTypeMap, OBJ_YARGTYPE_MAP);
            m->core.yt = yt;
            return (ObjConcreteYargType*)m;
        }
    }
}

ObjConcreteYargType* newYargArrayTypeFromType(ObjConcreteYargType* elementType) {
    ObjConcreteYargTypeArray* t = (ObjConcreteYargTypeArray*) newYargTypeFromType(TypeArray);
    t->element_type = elementType;
    t->core.yt = TypeArray;
    return (ObjConcreteYargType*)t;
}

ObjConcreteYargType* arrayElementType(const ObjConcreteYargTypeArray* arrayType) {
    return arrayType->element_type;
}

size_t arrayElementOffset(const ObjConcreteYargTypeArray* arrayType, size_t index) {
    return index * arrayElementSize(arrayType);
}

size_t arrayElementSize(const ObjConcreteYargTypeArray* arrayType) {
    return yt_sizeof_type_storage(arrayElementType(arrayType));
}

ObjConcreteYargType* newYargStructType(size_t fieldCount) {
    ObjConcreteYargTypeStruct* t = (ObjConcreteYargTypeStruct*) newYargTypeFromType(TypeStruct);
    tempObjRootPush((Obj*)t);

    ObjConcreteYargType** fieldTypes = ALLOCATE(ObjConcreteYargType*, fieldCount);
    for (size_t i = 0; i < fieldCount; i++) {
        fieldTypes[i] = NULL;
    }

    size_t* fieldIndexes = ALLOCATE(size_t, fieldCount);
    for (size_t i = 0; i < fieldCount; i++) {
        fieldIndexes[i] = 0;
    }

    t->field_indexes = fieldIndexes;
    t->field_types = fieldTypes;
    t->field_count = fieldCount;

    tempRootPop();
    return (ObjConcreteYargType*)t;
}

ObjConcreteYargType* newYargPointerType(ObjConcreteYargType* targetType) {
    ObjConcreteYargTypePointer* p = (ObjConcreteYargTypePointer*) newYargTypeFromType(TypePointer);
    p->target_type = targetType;
    return (ObjConcreteYargType*)p;
}
size_t addFieldType(ObjConcreteYargTypeStruct* st, size_t index, size_t fieldOffset, ObjConcreteYargType* type, ObjString* name) {
    st->field_types[index] = type;
    tableSet(&st->field_names, name, SIZE_T_UI_VAL(index));
    uint32_t alignmentPadding = 0;
    size_t alignment = yt_alignmentfor_type_storage(type);
    alignmentPadding = (alignment - (fieldOffset % alignment)) % alignment;
    st->field_indexes[index] = fieldOffset + alignmentPadding;
    if (alignment > st->alignment) {
        st->alignment = alignment;
    }
    st->storage_size = fieldOffset + alignmentPadding + yt_sizeof_type_storage(type);
    return st->storage_size;
}

size_t addFieldTypeAtOffset(ObjConcreteYargTypeStruct* st, size_t index, ObjConcreteYargType* type, size_t offset, ObjString* name) {
    st->field_types[index] = type;
    tableSet(&st->field_names, name, SIZE_T_UI_VAL(index));
    uint32_t alignmentPadding = 0;
    st->field_indexes[index] = offset;
    st->storage_size = offset + alignmentPadding + yt_sizeof_type_storage(type);
    return st->storage_size;
}


bool isUint32Pointer(Value val) {
    if (IS_POINTER(val)) {
        ObjConcreteYargTypePointer* pointer = AS_POINTER(val)->type;
        ObjConcreteYargType* dest = pointer->target_type;
        if (dest) {
            return dest->yt == TypeUint32;
        }
    }
    return false;
}

Value concrete_typeof(Value a) {
    if (IS_NIL(a)) {
        return NIL_VAL;
    } else if (IS_BOOL(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeBool));
    } else if (IS_DOUBLE(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeDouble));
    } else if (IS_I8(a)) {
        return (YARGTYPE_VAL(newYargTypeFromType(TypeInt8)));
    } else if (IS_UI8(a)) {
        return (YARGTYPE_VAL(newYargTypeFromType(TypeUint8)));
    } else if (IS_I16(a)) {
        return (YARGTYPE_VAL(newYargTypeFromType(TypeInt16)));
    } else if (IS_UI16(a)) {
        return (YARGTYPE_VAL(newYargTypeFromType(TypeUint16)));
    } else if (IS_I32(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeInt32));
    } else if (IS_UI32(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeUint32));
    } else if (IS_I64(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeInt64));
    } else if (IS_UI64(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeUint64));
    } else if (IS_FUNCTION(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeFunction));
    } else if (IS_CLOSURE(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeFunction));
    } else if (IS_NATIVE(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeFunction));
    } else if (IS_BOUND_METHOD(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeFunction));
    } else if (IS_CLASS(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeClass));
    } else if (IS_INSTANCE(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeInstance));
    } else if (IS_ROUTINE(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeRoutine));
    } else if (IS_CHANNEL(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeChannel));
    } else if (IS_SYNCGROUP(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeSyncGroup));
    } else if (IS_STRING(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeString));
    } else if (IS_ADDRESS(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeAddress));
    } else if (IS_UNIFORMARRAY(a)) {
        return YARGTYPE_VAL(AS_UNIFORMARRAY(a)->store.storedType);
    } else if (IS_STRUCT(a)) {
        return YARGTYPE_VAL(AS_STRUCT(a)->store.storedType);
    } else if (IS_YARGTYPE(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeYargType));
    } else if (IS_POINTER(a)) {
        return YARGTYPE_VAL(AS_POINTER(a)->type);
    } else if (IS_MAP(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeMap));
    } else if (IS_INT(a)) {
        return YARGTYPE_VAL(newYargTypeFromType(TypeInt));
    }
    fatalVMError("Unexpected object type");
    return NIL_VAL;
}

bool type_packs_as_obj(ObjConcreteYargType* type) {
    switch (type->yt) {
        case TypeBool:
        case TypeDouble:
        case TypeInt8:
        case TypeUint8:
        case TypeInt16:
        case TypeUint16:
        case TypeInt32:
        case TypeUint32:
        case TypeInt64:
        case TypeUint64:
        case TypeAddress:
        case TypeArray:
        case TypeStruct:
            return false;
        case TypeInt:
        case TypeString:
        case TypeClass:
        case TypeInstance:
        case TypeFunction:
        case TypeRoutine:
        case TypeChannel:
        case TypeSyncGroup:
        case TypePointer:
        case TypeMap:
        case TypeYargType:
            return true;
    }
}

bool type_packs_as_container(ObjConcreteYargType* type) {
    switch (type->yt) {
        case TypeBool:
        case TypeInt:
        case TypeDouble:
        case TypeInt8:
        case TypeUint8:
        case TypeInt16:
        case TypeUint16:
        case TypeInt32:
        case TypeUint32:
        case TypeInt64:
        case TypeUint64:
        case TypeAddress:
        case TypeString:
        case TypeClass:
        case TypeInstance:
        case TypeFunction:
        case TypeRoutine:
        case TypeChannel:
        case TypeSyncGroup:
        case TypeMap:
        case TypeYargType:
            return false;
        case TypePointer:
        case TypeArray:
        case TypeStruct:
            return true;
    }
}

bool is_nil_assignable_type(ObjConcreteYargType* type) {
    if (type == NULL) {
        return true;
    } else {
        switch (type->yt) {
            case TypeBool:
            case TypeInt:
            case TypeDouble:
            case TypeInt8:
            case TypeUint8:
            case TypeInt16:
            case TypeUint16:
            case TypeInt32:
            case TypeUint32:
            case TypeInt64:
            case TypeUint64:
            case TypeStruct:
            case TypeAddress:
                return false;
            case TypeString:
            case TypeClass:
            case TypeInstance:
            case TypeFunction:
            case TypeRoutine:
            case TypeChannel:
            case TypeSyncGroup:
            case TypeArray:
            case TypePointer:
            case TypeMap:
            case TypeYargType:
                return true;
        } 
    }
    return false;
}

bool is_placeable_type(ObjConcreteYargType* type) {
    if (type != NULL) {
        switch(type->yt) {
            case TypeInt8: return true;
            case TypeUint8: return true;
            case TypeInt16: return true;
            case TypeUint16: return true;
            case TypeInt32: return true;
            case TypeUint32: return true;
            case TypeInt64: return true;
            case TypeUint64: return true;
            case TypeArray: {
                ObjConcreteYargTypeArray* ct = (ObjConcreteYargTypeArray*)type;
                ObjConcreteYargType* elementType = arrayElementType(ct);
                return is_placeable_type(elementType);
            }
            case TypeStruct: {
                const ObjConcreteYargTypeStruct* ct = (const ObjConcreteYargTypeStruct*)type;
                bool is_placeable = true;
                for (size_t i = 0; i < ct->field_count; i++) {
                    ObjConcreteYargType* fieldType = ct->field_types[i];
                    is_placeable &= is_placeable_type(fieldType);
                }
                return is_placeable;
            }
            default: return false;
        }
    }
    return false;
}

bool is_stored_type(ObjConcreteYargType* type) {
    if (type != NULL) {
        switch(type->yt) {
            case TypeArray:
            case TypeStruct:
            case TypePointer:
                return true;
            default:
                return false;
        }
    }
    return false;
}

size_t yt_sizeof_type_storage(ObjConcreteYargType* type) {
    if (type == NULL) {
        return sizeof(Value);
    } else {
        switch (type->yt) {
        case TypeBool:
        case TypeDouble:
            return sizeof(Value);
        case TypeInt8:
            return sizeof(int8_t);
        case TypeUint8:
            return sizeof(uint8_t);
        case TypeInt16:
            return sizeof(int16_t);
        case TypeUint16:
            return sizeof(uint16_t);
        case TypeInt32:
            return sizeof(int32_t);
        case TypeUint32:
            return sizeof(uint32_t);
        case TypeInt64:
            return sizeof(int64_t);
        case TypeUint64:
            return sizeof(uint64_t);
        case TypeAddress:
            return sizeof(uintptr_t);
        case TypeStruct: {
            const ObjConcreteYargTypeStruct* st = (const ObjConcreteYargTypeStruct*)type;
            return st->storage_size;
        }
        case TypeArray: {
            const ObjConcreteYargTypeArray* array = (const ObjConcreteYargTypeArray*)type;
            return arrayElementSize(array) * array->cardinality;
        }
        case TypeInt:
        case TypeString:
        case TypeClass:
        case TypeInstance:
        case TypeFunction:
        case TypeRoutine:
        case TypeChannel:
        case TypeSyncGroup:
        case TypePointer:
        case TypeMap:
        case TypeYargType:
            return sizeof(Obj*);
        }
    }
}

size_t yt_alignmentfor_type_storage(ObjConcreteYargType* type) {
    if (type == NULL) {
        return 8;
    } else {
        switch (type->yt) {
        case TypeBool:
        case TypeDouble:
            return sizeof(Value);
        case TypeInt8:
            return sizeof(int8_t);
        case TypeUint8:
            return sizeof(uint8_t);
        case TypeInt16:
            return sizeof(int16_t);
        case TypeUint16:
            return sizeof(uint16_t);
        case TypeInt32:
            return sizeof(int32_t);
        case TypeUint32:
            return sizeof(uint32_t);
        case TypeInt64:
            return sizeof(int64_t);
        case TypeUint64:
            return sizeof(uint64_t);
        case TypeAddress:
            return sizeof(uintptr_t);
        case TypeStruct: {
            const ObjConcreteYargTypeStruct* st = (const ObjConcreteYargTypeStruct*)type;
            return st->alignment;
        }
        case TypeArray: {
            const ObjConcreteYargTypeArray* array = (const ObjConcreteYargTypeArray*)type;
            ObjConcreteYargType* elementType = array->element_type;
            return yt_alignmentfor_type_storage(elementType);
        }
        case TypeInt:
        case TypeString:
        case TypeClass:
        case TypeInstance:
        case TypeFunction:
        case TypeRoutine:
        case TypeChannel:
        case TypeSyncGroup:
        case TypePointer:
        case TypeMap:
        case TypeYargType:
            return sizeof(Obj*);
        }
    }
}

Value defaultValue(Value type) {
    if (IS_NIL(type)) {
        return NIL_VAL;
    } else {
        ObjConcreteYargType* ct = AS_YARGTYPE(type);
        switch (ct->yt) {
            case TypeBool: return BOOL_VAL(false);
            case TypeInt: return defaultIntValue();
            case TypeDouble: return DOUBLE_VAL(0);
            case TypeInt8: return I8_VAL(0);
            case TypeUint8: return UI8_VAL(0);
            case TypeInt16: return I16_VAL(0);
            case TypeUint16: return UI16_VAL(0);
            case TypeInt32: return I32_VAL(0);
            case TypeUint32: return UI32_VAL(0);
            case TypeInt64: return I64_VAL(0);
            case TypeUint64: return UI64_VAL(0);
            case TypeAddress: return ADDRESS_VAL(0);
            case TypeStruct: return defaultStructValue(ct);
            case TypeArray: return defaultArrayValue(ct);
            case TypePointer:
            case TypeString:
            case TypeClass:
            case TypeInstance:
            case TypeFunction:
            case TypeRoutine:
            case TypeChannel:
            case TypeSyncGroup:
            case TypeMap:
            case TypeYargType:
                return NIL_VAL;
        }
    }
}

static bool isAssignableCardinality(size_t lhsCardinality, size_t rhsCardinality) {
    if (lhsCardinality == 0) {
        return true;
    } else {
        return lhsCardinality == rhsCardinality;
    }
}

static bool isInitializableArray(ObjConcreteYargTypeArray* lhsConcreteType, ObjConcreteYargTypeArray* rhsConcreteType) {

    if (isAssignableCardinality(lhsConcreteType->cardinality, rhsConcreteType->cardinality)) {
        if (lhsConcreteType->element_type == NULL) {
            return true;
        } else if (lhsConcreteType->element_type == NULL && rhsConcreteType->element_type == NULL) {
            return true;
        } else if (rhsConcreteType->element_type == NULL) {
            return false;
        } else {
            return lhsConcreteType->element_type->yt == rhsConcreteType->element_type->yt;
        }
    } else {
        return false;
    }
}

bool isInitialisableType(ObjConcreteYargType* lhsType, Value rhsValue, Value *promotedRhs) {

    promotedRhs->type = NULL;

    if (lhsType == NULL) {
        return true;
    }
    
    if (IS_NIL(rhsValue)) {
        return is_nil_assignable_type(lhsType);
    }

    Value rhsType = concrete_typeof(rhsValue);
    ObjConcreteYargType* rhsConcreteType = AS_YARGTYPE(rhsType);

    if (lhsType->yt == TypeArray && rhsConcreteType->yt == TypeArray) {       
        return isInitializableArray((ObjConcreteYargTypeArray*)lhsType, (ObjConcreteYargTypeArray*)rhsConcreteType); 
    } else {
        if (IS_INT(rhsValue))
        {
            ObjInt *i = (ObjInt *) rhsValue.as.obj;
            if (i->isLiteral)
            {
                switch (lhsType->yt)
                {
                case TypeInt8:
                    if (int_is_range(&i->bigInt, INT8_MIN, INT8_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = I8_VAL(int_to_i32(&i->bigInt));
                        return true;
                    }
                    break;
                case TypeUint8:
                    if (int_is_range(&i->bigInt, 0, UINT8_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = UI8_VAL(int_to_u32(&i->bigInt));
                        return true;
                    }
                    break;
                case TypeInt16:
                    if (int_is_range(&i->bigInt, INT16_MIN, INT16_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = I16_VAL(int_to_i32(&i->bigInt));
                        return true;
                    }
                    break;
                case TypeUint16:
                    if (int_is_range(&i->bigInt, 0, UINT16_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = UI16_VAL(int_to_u32(&i->bigInt));
                        return true;
                    }
                    break;
                case TypeInt32:
                    if (int_is_range(&i->bigInt, INT32_MIN, INT32_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = I32_VAL(int_to_i32(&i->bigInt));
                        return true;
                    }
                    break;
                case TypeUint32:
                    if (int_is_range(&i->bigInt, 0, UINT32_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = UI32_VAL(int_to_u32(&i->bigInt));
                        return true;
                    }
                    break;
                case TypeInt64:
                    if (int_is_range(&i->bigInt, INT64_MIN, INT64_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = I64_VAL(int_to_i64(&i->bigInt));
                        return true;
                    }
                    break;
                case TypeUint64:
                    if (int_is_range(&i->bigInt, 0, UINT64_MAX) == INT_WITHIN)
                    {
                        *promotedRhs = UI64_VAL(int_to_u64(&i->bigInt));
                        return true;
                    }
                    break;
                default:
                    break;
                }
            }
        }
        return lhsType->yt == rhsConcreteType->yt;
    }
}

// this is a temporary measure, until we have a more complete hashing setup.
bool isSupportedMapKeyType(ObjConcreteYargType* type) {
    if (type != NULL) {
        switch (type->yt) {
            case TypeMap: {
                const ObjConcreteYargTypeMap* mt = (const ObjConcreteYargTypeMap*)type;
                return isSupportedMapKeyType(mt->key_type);
            }
            case TypeString:
                return true;
            default:
                return false;
        }
    } else {
         return false;
    }
}

static ObjString* typeLiteralToString(const ObjConcreteYargType* type) {
    if (type == NULL) {
        return copyString("any", 3);
    }

    switch (type->yt) {
        case TypeBool: return copyString("bool", 4);
        case TypeDouble: return copyString("mfloat64", 8);
        case TypeInt: return copyString("int", 3);
        case TypeInt8: return copyString("int8", 4);
        case TypeUint8: return copyString("uint8", 5);
        case TypeInt16: return copyString("int16", 5);
        case TypeUint16: return copyString("uint16", 6);
        case TypeInt32: return copyString("int32", 5);
        case TypeUint32: return copyString("uint32", 6);
        case TypeInt64: return copyString("int64", 5);
        case TypeUint64: return copyString("uint64", 6);
        case TypeAddress: return copyString("address", 7);
        case TypeString: return copyString("string", 6);
        case TypeClass: return copyString("Class", 5);
        case TypeInstance: return copyString("Instance", 8);
        case TypeFunction: return copyString("Function", 8);
        case TypeRoutine: return copyString("Routine", 7);
        case TypeChannel: return copyString("Channel", 7);
        case TypeSyncGroup: return copyString("SyncGroup", 9);
        case TypeYargType: return copyString("Type", 4);
        case TypeArray: {
            const ObjConcreteYargTypeArray* array = (const ObjConcreteYargTypeArray*) type;
            ObjString* typeStr = typeLiteralToString(array->element_type);
            tempObjRootPush((Obj*)typeStr);
            char buffer[128];
            if (array->cardinality > 0) {
                snprintf(buffer, sizeof(buffer), "%s[%zu]", typeStr->chars, array->cardinality);
            } else {
                snprintf(buffer, sizeof(buffer), "%s[]", typeStr->chars);
            }
            ObjString* result = copyString(buffer, (int)strlen(buffer));
            tempRootPop();
            return result;
        }
        case TypeStruct: {
            const ObjConcreteYargTypeStruct* st = (const ObjConcreteYargTypeStruct*) type;
            char buffer[1024];
            snprintf(buffer, sizeof(buffer), "struct{|%zu:%zu| ", st->field_count, st->storage_size);
            size_t cursor = strlen(buffer);
            for (size_t i = 0; i < st->field_count; i++) {
                ObjString* fieldTypeStr = typeLiteralToString(st->field_types[i]);
                tempObjRootPush((Obj*)fieldTypeStr);
                snprintf(buffer + cursor, sizeof(buffer) - cursor, "%s; ", fieldTypeStr->chars);
                cursor = strlen(buffer);
                tempRootPop();
            }
            snprintf(buffer + cursor, sizeof(buffer) - cursor, "}");
            return copyString(buffer, (int)strlen(buffer));
        }
        case TypePointer: {
            const ObjConcreteYargTypePointer* st = (const ObjConcreteYargTypePointer*) type;
            ObjString* typeStr = typeLiteralToString(st->target_type);
            tempObjRootPush((Obj*)typeStr);
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "*%s", typeStr->chars);
            ObjString* result = copyString(buffer, (int)strlen(buffer));
            tempRootPop();
            return result;
        }
        case TypeMap: {
            const ObjConcreteYargTypeMap* mt = (const ObjConcreteYargTypeMap*) type;
            ObjString* typeStr = typeLiteralToString(mt->value_type);
            tempObjRootPush((Obj*)typeStr);
            ObjString* keyTypeStr = typeLiteralToString(mt->key_type);
            tempObjRootPush((Obj*)keyTypeStr);
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "%s[%s]", typeStr->chars, keyTypeStr->chars);
            ObjString* result = copyString(buffer, (int)strlen(buffer));
            tempRootPop();
            tempRootPop();
            return result;
        }
        default: {
            return copyString("Unknown", 7);
        }
    }
}

ObjString* typeToString(const ObjConcreteYargType* type) {
    ObjString* literalStr = typeLiteralToString(type);
    tempObjRootPush((Obj*)literalStr);
    ObjString* prefix = copyString("Type:", 5);
    tempObjRootPush((Obj*)prefix);
    ObjString* result = concatenateStrings(prefix, literalStr);
    tempRootPop();
    tempRootPop();
    return result;
}
