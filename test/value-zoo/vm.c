#include "vm.h"

#include "value.h"
#include "placed-value.h"
#include "yargobject.h"

#include <stdlib.h>

#include "yarg-runtime.h"

ObjYarg* builtin_new(const ObjYargType* type) {
    if (type == NULL) {
        ObjValue* obj = ALLOCATE_OBJ(ObjValue, OBJ_ANYVALUE);
        ObjYargPointer* pointer = newYargPointerToObjVal(obj);
        return (ObjYarg*)pointer;
    } else if (is_value_type(type->yt) && is_placeable_yargtype(type)) {
        void* memory = malloc(storage_size_of_yargtype(type));
        ObjYargPointer* pointer = newYargPointerToPlaceable(type, memory);
        return (ObjYarg*)pointer;
    } else if (is_value_type(type->yt)) {
        ObjYarg* obj = (ObjYarg*) allocateYargObject(type);
        ObjYargPointer* pointer = newYargPointerToObj(obj);
        return (ObjYarg*)pointer;
    } else {
        return (ObjYarg*)allocateYargObject(type);
    }
}
