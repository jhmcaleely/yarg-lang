//
//  big-int.h
//  BigInt
//
//  Created by dlm on 18/01/2026.
//
#ifndef cyarg_bigint_bigint_h
#define cyarg_bigint_bigint_h

#include <stdint.h>
#include <stdbool.h>

#define INT_DIGITS_FOR_INT8 1
#define INT_DIGITS_FOR_INT16 1
#define INT_DIGITS_FOR_INT32 2
#define INT_DIGITS_FOR_INT64 4
#define INT_DIGITS_FOR_ADDRESS 4 // target address size could be 32 or 64
#define INT_DIGITS_FOR_S(STRLEN) ((1651124 + (STRLEN) * 342808) / 1651125) // multiply by 1/log10(65536) - fails at 1224 decimal digits (255 (16-bit) digits)

typedef struct Int
{
    bool neg_;
    bool overflow_;
    uint8_t d_; // num (16-bit) digits 1 -
    uint8_t m_; // max (16-bit) digits - allocated size, always even
    union
    {
        uint8_t b_[];
        uint16_t h_[];
        uint32_t w_[];
    };
} Int;

typedef enum
{
    INT_LT,
    INT_EQ,
    INT_GT
} IntComp;

typedef enum
{
    INT_BELOW,
    INT_WITHIN,
    INT_ABOVE
} IntRange;

// allocators
Int *int_new(int digits); // may return m_ > digits
Int *int_resize(Int *, int digits); // digits == -1 reduce to d_, digits == 0 deallocates and returns 0, may return m_ > digits

// constructors
void int_init(Int *);
void int_set_i(int64_t, Int *);
void int_set_u(uint64_t, Int *);
void int_set_s(char const *, Int *);
void int_set_t(Int const *, Int *);


// funtions
void int_add(Int const *, Int const *, Int *);
void int_sub(Int const *, Int const *, Int *);
void int_shift(int, Int *); // by half words
void int_mul(Int const *, Int const *, Int *);
void int_div(Int const *, Int const *, Int *i, Int *r); // r may be nil and not returned
void int_neg(Int *);

// comparisons
bool int_is_zero(Int const *);
IntComp int_is(Int const *, Int const *);
IntComp int_is_abs(Int const *, Int const *);
IntRange int_is_range(Int const *, int64_t, uint64_t);

// output
char const *int_to_s(Int const *, char *, int n);
int64_t int_to_i64(Int const *);
uint64_t int_to_u64(Int const *);
int32_t int_to_i32(Int const *);
uint32_t int_to_u32(Int const *);

// testing
void int_invariant(Int const *);
void int_make_random(Int *); // constructor
void int_print(Int const *); // todo deprecated: remove once added to language
void int_for_bc(Int const *); // todo deprecated: remove once added to language
void int_run_tests(void); // todo deprecated: remove once added to language

#endif
