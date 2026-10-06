#include "value.h"
#include "placed-value.h"
#include "yargobject.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yarg-runtime.h"
#include "vm.h"

void sprintValueValue(Value v, char* buffer, size_t bufferSize) {

    if (IS_NIL(v)) {
        snprintf(buffer, bufferSize, "nil");
        return;
    }
    else {
        switch (v.type->yt) {
            case TypeBool: snprintf(buffer, bufferSize, v.as.boolean ? "true" : "false"); break;
            case TypeInt8: snprintf(buffer, bufferSize, "%d", v.as.i8); break;
            case TypeUint32: snprintf(buffer, bufferSize, "%u", v.as.ui32); break;
            case TypeAddress: snprintf(buffer, bufferSize, "%p", (void*)v.as.address); break;
            case TypeArray: {
                ObjArray* array = (ObjArray*)v.as.obj;
                ObjYargTypeArray* array_type  = (ObjYargTypeArray*) array->yarg.type;
                size_t increment = storage_size_of_type(array_type->element_type->yt);
                uintptr_t elems = (uintptr_t) array->elements;
                strcpy(buffer, "");
                for (size_t i = 0; i < array_type->cardinality; i++){
                    uintptr_t offset = elems + increment * i;
                    PlacedValue pv = createPlacedValue(offset, array_type->element_type);
                    Value x = getPlacedValue(pv);
                    char elem[100];
                    sprintValueValue(x, elem, 100);
                    strcat(buffer, elem);
                    if (i < array_type->cardinality - 1) {
                        strcat(buffer, ", ");
                    }
                }
            }
                break;
            case TypePointer: snprintf(buffer, bufferSize, "<pointer>"); break;
            default:
                snprintf(buffer, bufferSize, "unknown value");
                break;
        }
    }
}

void sprintYargType(const ObjYargType* t, char* buffer, size_t bufferSize) {
    switch (t->yt) {
        case TypeInt8: snprintf(buffer, bufferSize, "int8"); break;
        case TypeUint8: snprintf(buffer, bufferSize, "uint8"); break;
        case TypeInt16: snprintf(buffer, bufferSize, "int16"); break;
        case TypeUint16: snprintf(buffer, bufferSize, "uint16"); break;
        case TypeInt32: snprintf(buffer, bufferSize, "int32"); break;
        case TypeUint32: snprintf(buffer, bufferSize, "uint32"); break;
        case TypeInt64: snprintf(buffer, bufferSize, "int64"); break;
        case TypeUint64: snprintf(buffer, bufferSize, "uint64"); break;
        case TypeDouble: snprintf(buffer, bufferSize, "mfloat64"); break;
        case TypeAddress: snprintf(buffer, bufferSize, "address"); break;

        case TypeBool: snprintf(buffer, bufferSize, "bool"); break;
        case TypeChannel: snprintf(buffer, bufferSize, "channel"); break;
        case TypeArray: {
            ObjYargTypeArray* array = (ObjYargTypeArray*)t;
            sprintYargType(array->element_type, buffer, bufferSize);
            size_t offset = strlen(buffer);
            snprintf(&buffer[offset], (bufferSize - offset), "[%zu]", array->cardinality);
            break;
        }
        case TypePointer: {
            ObjYargTypePointer* pointer = (ObjYargTypePointer*)t;
            sprintYargType(pointer->target_type, buffer, bufferSize);
            snprintf(buffer + strlen(buffer), bufferSize - strlen(buffer), "*");
            break;
        }
        default:
            panic();
    }
}

void sprintValue(Value v, char* buffer, size_t bufferSize) {
    sprintValueValue(v, buffer, bufferSize);
    if (!valuesEqual(v, NIL_VAL)) {
        snprintf(buffer + strlen(buffer), bufferSize - strlen(buffer), " : ");
        sprintYargType(v.type, buffer + strlen(buffer), bufferSize - strlen(buffer));
    }
}

int main(void) {
    char buffer[256];

    printf("sizeof(uintptr_t) = %zu\n", sizeof(uintptr_t));

    Value v;

    printf("sizeof(Value) = %zu\n", sizeof(Value));
    printf("sizeof(Value.as) = %zu\n", sizeof(v.as));
    printf("sizeof(Value.type) = %zu\n", sizeof(v.type));
    printf("sizeof(YargType) = %zu\n", sizeof(YargType));
    printf("sizeof(bool) = %zu\n", sizeof(v.as.boolean));
    printf("sizeof(Value.as.obj) = %zu\n", sizeof(v.as.obj));
    printf("sizeof(address) = %zu\n", sizeof(v.as.address));
    printf("sizeof(double) = %zu\n", sizeof(v.as.dbl));
    printf("sizeof(int8_t) = %zu\n", sizeof(v.as.i8));
    printf("sizeof(uint8_t) = %zu\n", sizeof(v.as.ui8));
    printf("sizeof(int16_t) = %zu\n", sizeof(v.as.i16));
    printf("sizeof(uint16_t) = %zu\n", sizeof(v.as.ui16));
    printf("sizeof(int32_t) = %zu\n", sizeof(v.as.i32));
    printf("sizeof(uint32_t) = %zu\n", sizeof(v.as.ui32));
    printf("sizeof(int64_t) = %zu\n", sizeof(v.as.i64));
    printf("sizeof(uint64_t) = %zu\n", sizeof(v.as.ui64));


    v = NIL_VAL;
    sprintValue(v, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    v = BOOL_VAL(true);
    sprintValue(v, buffer, sizeof(buffer));
    printf("%s\n", buffer);
    v = BOOL_VAL(false);
    sprintValue(v, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    v = UI32_VAL(42);
    sprintValue(v, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    v = ADDRESS_VAL((uintptr_t)(void*)0);
    sprintValue(v, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    v = ADDRESS_VAL((uintptr_t)(void*)&main);
    sprintValue(v, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    const ObjYargType* element = newYargTypeFromType(TypeInt8);

    PlacedValue pv = allocPlacedValue(element);
    Value x = getPlacedValue(pv);
    sprintValue(x, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    element = newYargTypeFromType(TypeAddress);

    pv = allocPlacedValue(element);
    placeValue(pv, ADDRESS_VAL((uintptr_t)(void*)&main));

    Value y = getPlacedValue(pv);
    sprintValue(y, buffer, sizeof(buffer));
    printf("%s\n", buffer);


    const ObjYargTypeArray* arrayType = newYargArrayTypeFromType(element, 10);
    ObjArray* array = (ObjArray*)allocateYargObject((ObjYargType*)arrayType);

    Value z = ARRAY_VAL(array);
    sprintValue(z, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    ObjLocation* firstElement = arrayElement(array, 3);
    Value firstElementValue = getLocationValue(firstElement);


    sprintValue(firstElementValue, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    setLocationValue(firstElement, ADDRESS_VAL((uintptr_t)(void*)99));

    sprintValue(z, buffer, sizeof(buffer));
    printf("%s\n", buffer);



    if (IS_ARRAY(z)) {
        printf("z is an array\n");
    }

    ObjYarg* something = builtin_new(&yargTypes.boolean);
    Value w = YARG_OBJ_VAL(something);
    sprintValue(w, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    ObjYarg* another = builtin_new(&yargTypes.int8);
    Value u = YARG_OBJ_VAL(another);
    sprintValue(u, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    return 0;
}