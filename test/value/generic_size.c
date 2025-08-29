#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "generic_size.h"

typedef struct Obj Obj;

typedef enum {
    YargNil
} yarg_values;

typedef union {
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    uint8_t ui8;
    uint16_t ui16;
    uint32_t ui32;
    uint64_t ui64;
    float f32;
    double f64;
    Obj* objRef;
    Obj* objBox;
    bool boolean;
    uintptr_t address;
} yarg_type_storage;

typedef enum {
    YargNilVal,
    YargBool,
    YargInt,
    YargUInt,
    YargDouble,
    i8,
    i16,
    i32,
    i64,
    ui8,
    ui16,
    ui32,
    ui64,
    float32,
    float64,
    address,
    YargObj
} yarg_value_types;

typedef struct {
    yarg_value_types type;
} yarg_type;

typedef struct {
    yarg_value_types as;
    yarg_type* type;
} yarg_value;

typedef struct {
    yarg_value val;
    yarg_type* permitted_type;
} yarg_store;

void print_info_c() {
    printf("yarg: sizeof(yarg_types) = %lu\n", sizeof(yarg_value_types));
    printf("yarg: sizeof(yarg_value) = %lu\n", sizeof(yarg_value));
    printf("yarg: sizeof(yarg_store) = %lu\n", sizeof(yarg_store));

    printf("\n");

    printf("C: sizeof(int) = %lu\n", sizeof(int));
    printf("C: sizeof(unsigned int) = %lu\n", sizeof(unsigned int));
    printf("C: sizeof(uintptr_t) = %lu\n", sizeof(uintptr_t));
    printf("C: sizeof(void*) = %lu\n", sizeof(void*));
    printf("C: sizeof(size_t) = %lu\n", sizeof(size_t));

    printf("C: sizeof(int8_t) = %lu\n", sizeof(int8_t));
    printf("C: sizeof(int16_t) = %lu\n", sizeof(int16_t));
    printf("C: sizeof(int32_t) = %lu\n", sizeof(int32_t));
    printf("C: sizeof(int64_t) = %lu\n", sizeof(int64_t));

    printf("C: sizeof(uint8_t) = %lu\n", sizeof(uint8_t));
    printf("C: sizeof(uint16_t) = %lu\n", sizeof(uint16_t));
    printf("C: sizeof(uint32_t) = %lu\n", sizeof(uint32_t));
    printf("C: sizeof(uint64_t) = %lu\n", sizeof(uint64_t));

    printf("C: sizeof(double) = %lu\n", sizeof(double));
    printf("C: sizeof(float) = %lu\n", sizeof(float));

    uint8_t buffer[8];
    for (size_t i = 0; i < 8; i++) {
        printf("uint8_t &buffer[%zu] = %p\n", i, &buffer[i]);
    }

    typedef struct {
        uint8_t buffer1;
        uint8_t buffer2;
        uint32_t word1;
    } byte_buffer;
    byte_buffer* buf = malloc(sizeof(byte_buffer));


    printf("sizeof(byte_buffer) = %zu\n", sizeof(byte_buffer));
    printf("addr buffer1 %p\n", &buf->buffer1);
    printf("addr buffer2 %p\n", &buf->buffer2);
    uint32_t* misaligned_word = (uint32_t*)&buf->buffer2;
    printf("misaligned word (%p) %d\n", misaligned_word, *misaligned_word);
    printf("addr word1 %p\n", &buf->word1);

    free(buf);
}