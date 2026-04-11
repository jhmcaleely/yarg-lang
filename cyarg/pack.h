#ifndef cyarg_pack_h
#define cyarg_pack_h

#include "chunk.h"

int packChunks(char const *sourceFileName, Chunk const *, bool includeLines, FILE *file);

int loadPackage(FILE *file);

//
#define PACKAGE_MAGIC_LEN 6

// int16/24 types are lsb first
typedef struct {
    uint8_t magic_[PACKAGE_MAGIC_LEN]; // 79 0a 72 67 00 42 "y\xarg\0B"
    uint16_t version_; // update for any changes to byte-code, number format, file format)
    uint8_t numChunks_;
    uint8_t stringHashMod_; // numStrings_ / 3 .. 255 - if there are more than 765 strings
    uint16_t codeLen_; // (multiple of 4-bytes)
    uint16_t stringsLen_; // (multiple of 4-bytes)
    uint16_t intsLen_; // (multiple of 4-bytes)
    uint16_t numStrings_;
    uint16_t numDoubles_;
    uint16_t numInts_;
    uint16_t numLines_; // optional (for debug/error messages)
} PackageFileHeader;

// xN is alignment, number is offset
//
// 0    magic(6) - 79 0a 72 67 00 42
// 6    version(2) - 26, 01 - year, issue (update for any changes to byte-code, number format, file format)
// 8  M chunks(1) -- 0..255 (0: just global)
// 9  …padding(1)
// 10 C code size(2) -- max 256k bytes
// 12 T strings size(2) -- max 256k chars
// 14 I ints size(2) -- max 256k chars
// 16 N strings(2) -- max 64k strings
// 18 D doubles(2) -- max 64k floats
// 20 Q ints(2) -- max 64k ints
// 22 L lines(2) -- min zero, max 64k lines
// 24   double D*8
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
// x1   …padding(0..3)
// x4   code (4C)*1
// x1   code offsets for lines L*3

#endif
