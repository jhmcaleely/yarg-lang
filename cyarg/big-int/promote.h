//
//  promote.h
//  BigInt
//
//  Created by dlm on 16/02/2026.
//
#ifndef cyarg_bigint_promote_h
#define cyarg_bigint_promote_h

// Try to promote literal int expressions as follows
// var uint8 x = 1; => var uint8 x = 1u32;
// var uint8 x; x = 1; => var uint8 x; x = 1u32;
// x[1] = 0; => x[1u32] = 0;
// poke @x100, 5 => poke @x100, 5u32
// int32(5) => 5i32
// uint32(5) => 5u2
// int64(5) => 5i64
// uint64(5) => 5u64
// todo - investigate if it is worth promoting all lit ints to the smallest machine type. This
//   may make the intermediate code smaller but would require the vm to promote back to the
//   expected type.
// todo - bug? should var int[] x = [1,2,3]; run? It results in RT error “Cannot initialise variable with this value.”
// todo - further - should var int8[] x = [1,2,3]; promote?

struct ObjStmt;
void promoteLitIntStatements(struct ObjStmt* stmt);

#endif
