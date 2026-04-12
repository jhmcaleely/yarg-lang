#ifndef cyarg_pack_h
#define cyarg_pack_h

#include "chunk.h"

int packChunks(char const *sourceFileName, Chunk const *, bool includeLines, FILE *file);

int loadPackage(FILE *file);

//
#define PACKAGE_MAGIC_LEN 6

// int16/24 types are lsb first
typedef struct {
    uint8_t magic_[PACKAGE_MAGIC_LEN];  // 79 0a 72 67 ff 42 "y\x0arg\xffB"
    uint16_t version_;                  // update for any changes to byte-code, number format or package format)
    uint16_t numChunks_;
    uint16_t numStrings_;
    uint16_t numDoubles_;
    uint16_t numInts_;
    uint16_t numLines_;
    uint16_t packing_;                  // reserved for future use and to ensure body is 8-byte aligned in xip
    uint32_t bodyLength_;
} PackageFileHeader;

// xN is alignment from start of body, number is offset
//
// =-= Header =-=
// 0    magic(6) - 79 0a 72 67 00 42
// 6    version(2) - 26, 01 - year, issue (update for any changes to byte-code, number format, file format)
// 8  M chunks(2) -- 0..65535 (0: just global)
// 10 N strings(2) -- max 64k strings
// 12 D doubles(2) -- max 64k floats
// 14 Q ints(2) -- max 64k ints
// 16 L lines(2) -- min zero, max 64k lines
// 18   packing(6)
// 24 B body length
// =-= Body =-=
// x8   double D*8
// per chunk -- m == M-1
// x8 K0    num consts in chunk0 1
// x1       code offset for chunk0 3
// x4       const offsets indexs -- K0*4 - type(1):index(3)
// x4 K1    num consts in chunk0 1
// x1       code offset for chunk0 3
// x4       const indexs -- K1*4
// …
// x4 Km    num consts in chunk0 1
// x1       code offset for chunk0 3
// x4       const indexs -- Km*4
// x4   ints (4I)*1 -- these could be shrunk by two or four bytes each, but would not then be xip
// x4   strings (4T)*1
// x1   code (4C)*1
// x1   code offsets for lines L*3

#endif
