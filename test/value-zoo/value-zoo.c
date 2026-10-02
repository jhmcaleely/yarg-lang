#include "value.h"
#include "yargtype.h"

#include "packed-value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sprintValueValue(Value v, char* buffer, size_t bufferSize) {
    switch (v.type) {
        case VAL_NIL:
            snprintf(buffer, bufferSize, "nil");
            break;
        case VAL_BOOL:
            snprintf(buffer, bufferSize, v.as.boolean ? "true" : "false");
            break;
        case VAL_UI32:
            snprintf(buffer, bufferSize, "%u", v.as.ui32);
            break;
        case VAL_ADDRESS:
            snprintf(buffer, bufferSize, "%p", (void*)v.as.address);
            break;
        default:
            snprintf(buffer, bufferSize, "unknown value type");
            break;
    }
}

YargType ValueType2(Value v) {
    switch (v.type) {
        case VAL_BOOL:
            return TypeBool;
        case VAL_UI32:
            return TypeUint32;
        case VAL_ADDRESS:
            return TypeAddress;
        default:
            exit(1); // or handle the error appropriately
    }
}

void sprintConcreteYargType(YargType t, char* buffer, size_t bufferSize) {
    switch (t) {
        case TypeBool:
            snprintf(buffer, bufferSize, "bool");
            break;
        case TypeUint32:
            snprintf(buffer, bufferSize, "uint32");
            break;
        case TypeAddress:
            snprintf(buffer, bufferSize, "address");
            break;
        default:
            snprintf(buffer, bufferSize, "unknown type");
            break;
    }
}

void sprintValue(Value v, char* buffer, size_t bufferSize) {
    sprintValueValue(v, buffer, bufferSize);
    if (!valuesEqual(v, NIL_VAL)) {
        snprintf(buffer + strlen(buffer), bufferSize - strlen(buffer), " : ");
        YargType t = ValueType2(v);
        sprintConcreteYargType(t, buffer + strlen(buffer), bufferSize - strlen(buffer));
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

    PackedValue pv;
    initialisePackedValue(pv);
    Value x = unpackValue(pv);
    sprintValue(x, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    packValue(pv, ADDRESS_VAL((uintptr_t)(void*)&main));

    Value y = unpackValue(pv);
    sprintValue(y, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    ObjYargType* element = newYargTypeFromType(TypeInt);

    ObjYargTypeArray* arrayType = newYargArrayTypeFromType(element);
    arrayType->cardinality = 10;

    ObjArray* array = (ObjArray*)allocateYargObject((ObjYargType*)arrayType);
    

    return 0;
}