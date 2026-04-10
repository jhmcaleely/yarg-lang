#ifndef cyarg_pack_h
#define cyarg_pack_h

#include "chunk.h"

int packChunks(char const *sourceFileName, Chunk const *, bool includeLines, FILE *file);

//
#define PACKAGE_MAGIC_LEN 6

typedef struct {
    uint8_t magic_[PACKAGE_MAGIC_LEN]; // 79 0a 72 67 00 42 "Y\012rg\0B"
    uint16_t version_; // update for any changes to byte-code, number format, file format)
    uint8_t numChunks_;
    uint8_t stringHashMod_; // numStrings_ / 3 .. 255 - if there are more than 765 strings
    uint16_t codeLen_; // (multiple of 4-bytes)
    uint16_t stringsLen_; // (multiple of 4-bytes)
    uint16_t intsLen_; // (multiple of 4-bytes)
    uint16_t numStrings_;
    uint16_t numInts_;
    uint16_t numDoubles_;
    uint16_t numLines_; // optional (for debug/error messages)
} PackageFileHeader;

// xN is alignment, number is offset

// 0    magic(6) - 79 0a 72 67 00 42
// 6    version(3) - 26, 52, 01 - year, week, issue (update for any changes to byte-code, number format, file format)
// 8  M chunks(1) -- 0..255 (0: just global)
// 9  P string hash mod(1) -- max 64k hash table entries
// 10 C code size(2) -- max 256k bytes
// 12 T strings size(2) -- max 256k chars
// 14 I ints size(2) -- max 256k chars
// 16 N strings(2) -- max 64k strings
// 18 D doubles(2) -- max 64k floats
// 20 Q ints(2) -- max 64k ints
// 22 L lines(2) -- min zero, max 64k lines
// 24   double D*8
// x8   int offsets Q*3
// x4   string hash table (P)*4 -- index into string offsets + next (0xffff is last)
// x4   chunk offsets M*3
// x1   consts (N+D+Q)*3 -- total 64k consts indexes into double/int offsets/string offsets
// x1   code offsets for lines L*3
// x1   string offsets (N+1)*3 -- extra offset to determine length of last string, string0 is file name
// x1 …padding…
// x4   ints (4T)*1 -- followed by 0..3 bytes of padding
// x4   strings (4T)*1 -- followed by 0..3 bytes of padding
// x4   code (4C)*1 -- followed by 0..3 bytes of padding
// x4 K0    num consts in chunk0 1
// x1 O0    code offset for chunk0 3
// x4       const indexs -- K0*2
// x2 K1    num consts in chunk0 1
// x1 O1    code offset for chunk0 3
// x2       const indexs -- K1*2
// …
// x2 Km    num consts in chunk0 1
// x1 Om    code offset for chunk0 3
// x2       const indexs -- Km*2

#endif
