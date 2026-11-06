#include "object_dyn_array.h"

#include <stddef.h>

#include "common.h"
#include "memory.h"
#include "object_gc_impl.h"

// note that this call can be omitted for objects where
// the DynamicObjArray is a member, and Zero filled
void initDynamicObjArray(DynamicObjArray* array) {
    array->objects = NULL;
    array->stash = NULL;
    array->objectCapacity = 0;
    array->objectCount = 0;
}

void freeDynamicObjArray(DynamicObjArray* array) {
    vm_free(array->objects);
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
