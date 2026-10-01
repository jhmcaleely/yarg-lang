#include "../../cyarg/value.h"
#include "../../cyarg/yargtype.h"

#include <stdio.h>
#include <stdlib.h>

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
        default:
            snprintf(buffer, bufferSize, "unknown value type");
            break;
    }
}

ConcreteYargType ValueType2(Value v) {
    switch (v.type) {
        case VAL_BOOL:
            return TypeBool;
        case VAL_UI32:
            return TypeUint32;
        default:
            exit(1); // or handle the error appropriately
    }
}

void sprintConcreteYargType(ConcreteYargType t, char* buffer, size_t bufferSize) {
    switch (t) {
        case TypeBool:
            snprintf(buffer, bufferSize, "bool");
            break;
        case TypeUint32:
            snprintf(buffer, bufferSize, "uint32");
            break;
        default:
            snprintf(buffer, bufferSize, "unknown type");
            break;
    }
}

void sprintValue(Value v, char* buffer, size_t bufferSize) {
    sprintValueValue(v, buffer, bufferSize);
    if (v.type != VAL_NIL) {
        snprintf(buffer + strlen(buffer), bufferSize - strlen(buffer), " : ");
        ConcreteYargType t = ValueType2(v);
        sprintConcreteYargType(t, buffer + strlen(buffer), bufferSize - strlen(buffer));
    }
}

int main(void) {
    char buffer[256];

    Value v;
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
    return 0;
}