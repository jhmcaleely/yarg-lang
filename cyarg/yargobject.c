#include "yargobject.h"

#include "common.h"

bool arraysEqual(const ObjPackedUniformArray* a, const ObjPackedUniformArray* b) {
    if (a == b) return true;
    return false;
}

bool structsEqual(const ObjPackedStruct* a, const ObjPackedStruct* b) {
    if (a == b) return true;
    return false;
}

bool typesEqual(const ObjConcreteYargType* a, const ObjConcreteYargType* b) {
    if (a == b) return true;
    switch (a->yt) {
        case TypeInt:
        case TypeInt8:
        case TypeUint8:
        case TypeInt16:
        case TypeUint16:
        case TypeInt32:
        case TypeUint32:
        case TypeInt64:
        case TypeUint64:
        case TypeDouble:
        case TypeAddress:
        case TypeClass:
            return a->yt == b->yt;
        case TypePointer: {
            const ObjConcreteYargTypePointer* pointer_a = (const ObjConcreteYargTypePointer*)a;
            const ObjConcreteYargTypePointer* pointer_b = (const ObjConcreteYargTypePointer*)b;
            return typesEqual(pointer_a->target_type, pointer_b->target_type);
        }
        case TypeArray: {
            const ObjConcreteYargTypeArray* array_a = (const ObjConcreteYargTypeArray*)a;
            const ObjConcreteYargTypeArray* array_b = (const ObjConcreteYargTypeArray*)b;
            return typesEqual(array_a->element_type, array_b->element_type) && array_a->cardinality == array_b->cardinality;
        }
        default:
            return false;
    }
    return a->yt == b->yt; // Simplistic check, may need to be expanded for complex types
}

bool pointersEqual(const ObjPackedPointer* a, const ObjPackedPointer* b) {
    if (a == b) return true;
    if (a->type != b->type) return false;

    return a->destination == b->destination;
}

bool intsEqual(const ObjInt* a, const ObjInt* b) {
    if (a == b) return true;

    return int_is(&a->bigInt, &b->bigInt) == INT_EQ;
}