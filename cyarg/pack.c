#include "pack.h"

#include "debug.h"
#include "object.h"
#include "value.h"
#include "yargtype.h"
#include "routine.h"

#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <assert.h>

typedef struct {
    uint32_t n_;
    uint32_t extent_;
    uint32_t *i_;
} LinesFile;

typedef struct {
    enum { PACK_CONST_S, PACK_CONST_I, PACK_CONST_D } type_;
    uint16_t index_;
} ConstItem;

typedef struct {
    uint32_t codeOffset_;
    uint8_t *code_;
    int codeLength_;
    int numConsts_;
    ConstItem constTypesAndOffsets_[256];
} FlatChunk;

typedef struct {
    uint32_t n_;
    uint32_t extent_;
    FlatChunk *i_;
    int totalCodeLength_;
} ChunksFile;

typedef struct {
    uint32_t n_;
    uint32_t extent_;
    struct {
        ObjString *s_;
        int chunkIndex_; // chunk:const used to update chunksFile when deleting duplicates
        int constNumber_;
    } *i_;
    int totalStringLength_; // including sourceFileName and postfix nulls
} StringsFile;

typedef struct {
    uint32_t n_;
    uint32_t extent_;
    struct DoublesFile_I {
        double f_;
        int chunkIndex_; // chunk:const used to update chunksFile when deleting duplicates
        int constNumber_;
    } *i_;
} DoublesFile;

typedef struct {
    uint32_t n_;
    uint32_t extent_;
    struct {
        ObjInt *i_;
        int chunkIndex_; // chunk:const used to update chunksFile when deleting duplicates
        int constNumber_;
    } *i_;
    int totalIntsLength_;
} IntsFile;

typedef struct {
    ChunksFile chunksFile_;
    LinesFile linesFile_;
    StringsFile stringsFile_;
    DoublesFile doublesFile_;
    IntsFile intsFile_;
} PackageFiles;

static void fileSet(void *, int, size_t i);
static void fileExtend(void *, int, size_t);
static void flattenConstants(Chunk const *, PackageFiles *);
static void flattenCode(Chunk const *, PackageFiles *);
static void flattenLines(Chunk const *, PackageFiles *);
static int pack(char const *, Chunk const *, PackageFiles *, FILE *);

int packChunks(char const *sourceFileName, Chunk const *chunk, bool includeLines, FILE *file) {

    int r = EX_OK;
    PackageFiles f = {0};

    fileExtend(&f.chunksFile_, 8, sizeof *f.chunksFile_.i_);
    fileExtend(&f.linesFile_, 128, sizeof *f.linesFile_.i_);
    fileExtend(&f.stringsFile_, 16, sizeof *f.stringsFile_.i_);
    fileExtend(&f.doublesFile_, 4, sizeof *f.doublesFile_.i_);
    fileExtend(&f.intsFile_, 4, sizeof *f.intsFile_.i_);

    // todo add sourceFileName to strings
    f.stringsFile_.totalStringLength_ += strlen(sourceFileName) + 1;

    f.chunksFile_.n_ = 1;
    flattenConstants(chunk, &f);
    int numChunks = f.chunksFile_.n_;

    // todo remove duplicates
    // todo remove non global, non returned method/property

    f.chunksFile_.n_ = 1;
    flattenCode(chunk, &f);
    assert(numChunks == f.chunksFile_.n_);

    if (includeLines) {
        f.chunksFile_.n_ = 1;
        flattenLines(chunk, &f);
        assert(numChunks == f.chunksFile_.n_);
    }

    r = pack(sourceFileName, chunk, &f, file);
    if (r != EX_OK) goto exit;

exit:
    free(f.intsFile_.i_);
    free(f.doublesFile_.i_);
    free(f.stringsFile_.i_);
    free(f.linesFile_.i_);
    free(f.chunksFile_.i_);

    return r;
}

int loadPackage(FILE *file) {
    PackageFileHeader h = {0};

    size_t n = fread(&h, sizeof h, 1, file);
    assert(n == 1);

    if (memcmp(h.magic_, &(int8_t [PACKAGE_MAGIC_LEN]){0x79, 0x0a, 0x72, 0x67, 0x00, 0x42}, PACKAGE_MAGIC_LEN) != 0) {
        return EX_PROTOCOL;
    }
    if (h.version_ != 0x2601) {
        return EX_DATAERR;
    }
    int bufferLen = 0;
    // 24   double D*8
    // per chunk -- m == M-1
    // x8 K0    num consts in chunk0 1
    // x1       code offset for chunk0 3
    // x4       const offsets indexs -- K0*4 - type(1):index(3)
    // x1       code offset for chunk0 3
    // x4       const indexs -- K1*4
    // …
    // x4 Km    num consts in chunk0 1
    // x1       code offset for chunk0 3
    // x4       const indexs -- Km*4
    // x4   ints (4I)*1
    // x4   strings (4T)*1
    // x1   …padding(0..3)
    // x4   code (4C)*1
    // x1   code offsets for lines L*3
    bufferLen += h.numDoubles_ * sizeof (double);
    bufferLen += h.numChunks_ * 256 * 4 + 4; // this is max most chunks will have fewer consts
    bufferLen += h.intsLen_;
    bufferLen += h.stringsLen_;
    bufferLen += h.codeLen_;
    bufferLen += h.numLines_ * 3;

    uint8_t *buffer = realloc(0, bufferLen);
    assert((long)buffer % 8 == 0);

    size_t bodyLen = fread(buffer, 1, bufferLen, file);
    assert(bodyLen > 0 && bodyLen <= bufferLen);
    uint8_t const *body = buffer;

    double *doubles = (double *)body;

    for (int i = 0; i < h.numDoubles_; i++) {
        if (i < 7) {
            printf("float@%ld:%f\n", (uint8_t *)&doubles[i] - (uint8_t *)doubles, doubles[i]);
        }
    }

    typedef struct {
        uint8_t numConsts_;
        uint8_t codeOffset[3];
        uint32_t typedIndexs[];
    } PackedChunk;
    PackedChunk const *chunks[256];
    uint8_t const *next = body + h.numDoubles_ * sizeof (double);
    uint8_t const *chunkFile = next;
    for (int i = 0; i < h.numChunks_; i++) {
        chunks[i] = (PackedChunk const *)next;
        next += 4 + 4 * chunks[i]->numConsts_;
    }
    uint8_t const *intFile = next;

    for (int i = 0; i < h.numInts_; i++) {
        Int const *thisInt = (Int const *)next;
        if (i < 11) {
            printf("int@%ld:", (uint8_t *)thisInt - intFile); int_print(thisInt); printf("\n");
        }
        next += sizeof (Int) + sizeof (uint16_t) * thisInt->m_;
    }
    uint8_t const *stringFile = next;

    for (int i = 0; i < h.numStrings_ + 1; i++) {
        char const *thisString = (char const *)next;
        if (i < 13) {
            printf("string@%ld:\"%s\"\n", (uint8_t *)thisString - stringFile, thisString);
        }
        next += strlen(thisString) + 1;
    }
    int pad = 4 - (next - body) % 4;
    if (pad != 4) next += pad;
    uint8_t const *codeFile = next;

    for (int i = 0; i < h.numChunks_; i++) {
        uint32_t codeOffset = 0;
        memcpy(&codeOffset, chunks[i]->codeOffset, 3);
        printf("Chunk:%d code@%u numConsts:%d\n", i, codeOffset, (int)chunks[i]->numConsts_);
        for (int c = 0; c < chunks[i]->numConsts_; c++) {
            uint32_t index = chunks[i]->typedIndexs[c] >> 8;
            uint8_t type = chunks[i]->typedIndexs[c] & 0xffu;
            static const char *typesName[] = {"string", "int", "float"};
            printf("%s@%u:", typesName[type], index);
            if (type == 0) {
                char const *thisString = (char const *)&stringFile[index];
                printf("\"%s\"", thisString);
            } else if (type == 1) {
                Int const *thisInt = (Int const *)&intFile[index];
                int_print(thisInt);
            } else {
                assert(type == 2);
                double thisFloat = doubles[index];
                printf("\"%f\"", thisFloat);
            }
            printf("\n");
        }
    }

exit:
    free(buffer);

    return EX_OK;
}

/////  todo  file and sort strings first
//////            mark constants as used when parsing byte code
//////            methods + properties of returned struct or class, global var names, 1st level function names are exported, op_const are not
///////            returned includes copied to var parameters
//////            remove non-global consts

void flattenConstants(Chunk const *chunk, PackageFiles *f) {

    int chunkIndex = f->chunksFile_.n_ - 1;
    int constCount = chunk->constants.count;

    FlatChunk *fc = &f->chunksFile_.i_[chunkIndex];
    fc->numConsts_ = 0;

    for (int i = 0; i < constCount; i++) {
        Value *v = &chunk->constants.values[i];
        if (IS_DOUBLE(*v)) {
            fc->constTypesAndOffsets_[fc->numConsts_++] = (ConstItem){ .type_ = PACK_CONST_D, .index_ = f->doublesFile_.n_ };
            fileExtend(&f->doublesFile_, 4, sizeof *f->doublesFile_.i_);
            f->doublesFile_.i_[f->doublesFile_.n_].f_ = AS_DOUBLE(*v);
            f->doublesFile_.i_[f->doublesFile_.n_].chunkIndex_ =  chunkIndex;
            f->doublesFile_.i_[f->doublesFile_.n_++].constNumber_ =  i;
        } else if (IS_OBJ(*v)) {
            switch (AS_OBJ(*v)->type) {
            case OBJ_FUNCTION:
                break;
            case OBJ_INT: {
                fc->constTypesAndOffsets_[fc->numConsts_++] = (ConstItem){ .type_ = PACK_CONST_I, .index_ = f->intsFile_.totalIntsLength_ };
                fileExtend(&f->intsFile_, 16, sizeof *f->intsFile_.i_);
                f->intsFile_.i_[f->intsFile_.n_].i_ = AS_INTOBJ(*v);
                f->intsFile_.i_[f->intsFile_.n_].chunkIndex_ =  chunkIndex;
                f->intsFile_.i_[f->intsFile_.n_++].constNumber_ =  i;
                Int *from = AS_INT(*v);
                int len = (int)((char *) from->w_ - (char *) from);
                assert(len % 4 == 0);
                len += sizeof (uint16_t) * (from->d_ + from->d_ % 2);
                f->intsFile_.totalIntsLength_ += len;
               break;
            }
            case OBJ_STRING: {
                fc->constTypesAndOffsets_[fc->numConsts_++] = (ConstItem){ .type_ = PACK_CONST_S, .index_ = f->stringsFile_.totalStringLength_ };
                fileExtend(&f->stringsFile_, 16, sizeof *f->stringsFile_.i_);
                ObjString *from = AS_STRING(*v);
                f->stringsFile_.i_[f->stringsFile_.n_].s_ = from;
                f->stringsFile_.i_[f->stringsFile_.n_].chunkIndex_ =  chunkIndex;
                f->stringsFile_.i_[f->stringsFile_.n_++].constNumber_ =  i;
                int len = from->length;
                f->stringsFile_.totalStringLength_ += len + 1;
               break;
            }
            default:
                assert(!"Unexpected constant obj");
                break;
            }
        } else {
            assert(!"Unexpected constant value");
        }

    }

    for (int i = 0; i < chunk->constants.count; i++) {
        Value *v = &chunk->constants.values[i];
        if (IS_OBJ(*v)) {
            switch (AS_OBJ(*v)->type) {
            case OBJ_FUNCTION:
                assert (f->chunksFile_.n_ != 255);
                fileExtend(&f->chunksFile_, 4, sizeof *f->chunksFile_.i_);
                f->chunksFile_.n_++;
                flattenConstants(&AS_FUNCTION(*v)->chunk, f);
            default:
                break;
            }
        }
    }
}

static void flattenCode(Chunk const *chunk, PackageFiles *f) {

    int chunkIndex = f->chunksFile_.n_ - 1;
    FlatChunk *fc = &f->chunksFile_.i_[chunkIndex];

    fc->code_ = chunk->code;
    fc->codeLength_ = chunk->count;
    fc->codeOffset_ = f->chunksFile_.totalCodeLength_;
    f->chunksFile_.totalCodeLength_ += fc->codeLength_;

    for (int i = 0; i < chunk->constants.count; i++) {
        Value *v = &chunk->constants.values[i];
        if (IS_OBJ(*v)) {
            switch (AS_OBJ(*v)->type) {
            case OBJ_FUNCTION:
                f->chunksFile_.n_++;
                flattenCode(&AS_FUNCTION(*v)->chunk, f);
            default:
                break;
            }
        }
    }
}

void flattenLines(Chunk const *chunk, PackageFiles *f) {

    int chunkIndex = f->chunksFile_.n_ - 1;
    FlatChunk *cf = &f->chunksFile_.i_[chunkIndex];

    int lastLine = chunk->lines[chunk->numLines - 1].line;
    int flattenedLines = f->linesFile_.n_;

    if (lastLine > flattenedLines) {
        fileSet(&f->linesFile_, lastLine, sizeof *f->linesFile_.i_);
        f->linesFile_.n_ = lastLine;
        memset(&f->linesFile_.i_[flattenedLines], 0xff, sizeof (uint32_t) * (lastLine - flattenedLines));
    }

    for (int i = 0; i < chunk->numLines; i++) {
        int a = chunk->lines[i].address;
        int l = chunk->lines[i].line;
        assert(l <= f->linesFile_.extent_);
        if (l > f->linesFile_.n_) f->linesFile_.n_ = l;
        f->linesFile_.i_[l - 1] = a + cf->codeOffset_;
    }

    for (int i = 0; i < chunk->constants.count; i++) {
        Value *v = &chunk->constants.values[i];
        if (IS_OBJ(*v)) {
            switch (AS_OBJ(*v)->type) {
            case OBJ_FUNCTION:
                f->chunksFile_.n_++;
                flattenLines(&AS_FUNCTION(*v)->chunk, f);
            default:
                break;
            }
        }
    }
}

int pack(char const *sourceFileName, Chunk const *chunk, PackageFiles *f, FILE *file) {

    PackageFileHeader h;
    assert(sizeof h == 24);

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
    // x4   ints (4I)*1
    // x4   strings (4T)*1
    // x1   …padding(0..3)
    // x4   code (4C)*1
    // x1   code offsets for lines L*3

    memcpy(&h.magic_, &(int8_t [PACKAGE_MAGIC_LEN]){0x79, 0x0a, 0x72, 0x67, 0x00, 0x42}, PACKAGE_MAGIC_LEN);
    h.version_ = 0x2601;
    h.numChunks_ = f->chunksFile_.n_;
    h.stringHashMod_ = f->stringsFile_.n_ < 765 ? (uint8_t) f->stringsFile_.n_ / 3 : 255;
    h.codeLen_ = (f->chunksFile_.totalCodeLength_ + 3) / 4;
    h.stringsLen_ = (f->stringsFile_.totalStringLength_ + 3) / 4;
    h.intsLen_= (f->intsFile_.totalIntsLength_ + 3) / 4;
    h.numStrings_ = f->stringsFile_.n_;
    h.numInts_ = f->intsFile_.n_;
    h.numDoubles_ = f->doublesFile_.n_;
    h.numLines_ = f->linesFile_.n_;

    size_t written = fwrite(&h, sizeof h, 1, file);
    if (written != 1) return EX_SOFTWARE;

    for (int i = 0; i < f->doublesFile_.n_; i++) {
        struct DoublesFile_I *di = &f->doublesFile_.i_[i];
        written = fwrite(&di->f_, sizeof (double), 1, file);
        if (written != 1) return EX_SOFTWARE;
    }

    for (int i = 0; i < f->chunksFile_.n_; i++) {
        FlatChunk *fc = &f->chunksFile_.i_[i];

        written = fwrite(&fc->numConsts_, sizeof (char), 1, file);
        if (written != 1) return EX_SOFTWARE;
        written = fwrite(&fc->codeOffset_, sizeof (char), 3, file);
        if (written != 3) return EX_SOFTWARE;
        for (int k = 0; k < fc->numConsts_; k++) {
            ConstItem *ci = &fc->constTypesAndOffsets_[k];
            assert(ci->type_ >= 0 && ci->type_ <= 3);
            written = fwrite(&ci->type_, sizeof (char), 1, file);
            if (written != 1) return EX_SOFTWARE;
            written = fwrite(&ci->index_, sizeof (char), 3, file);
            if (written != 3) return EX_SOFTWARE;
        }
    }

    for (int i = 0; i < f->intsFile_.n_; i++) {
        Int *bigInt = &f->intsFile_.i_[i].i_->bigInt;
        IntConcrete254 t;
        int_set_t(bigInt, int_init_concrete254(&t));
        t.m_ = t.d_ + t.d_ % 2;
        written = fwrite(&t, sizeof (Int) + sizeof (uint16_t) * t.m_, 1, file);
        if (written != 1) return EX_SOFTWARE;
    }

    size_t sourceFileNameLen = strlen(sourceFileName);
    char const *nullChar = &sourceFileName[sourceFileNameLen];
    written = fwrite(sourceFileName, strlen(sourceFileName) + 1, 1, file);
    if (written != 1) return EX_SOFTWARE;
    for (int i = 0; i < f->stringsFile_.n_; i++) {
        ObjString *s = f->stringsFile_.i_[i].s_;
        written = fwrite(s->chars, s->length, 1, file);
        if (written != 1) return EX_SOFTWARE;
        written = fwrite(nullChar, 1, 1, file);
        if (written != 1) return EX_SOFTWARE;
    }
    int pad = 4 - f->stringsFile_.totalStringLength_ % 4;
    if (pad != 4) {
        written = fwrite(&pad, pad, 1, file);
        if (written != 1) return EX_SOFTWARE;
    }

    for (int i = 0; i < f->chunksFile_.n_; i++) {
        FlatChunk *fc = &f->chunksFile_.i_[i];
        written = fwrite(fc->code_, fc->codeLength_, 1, file);
        if (written != 1) return EX_SOFTWARE;
    }

    for (int i = 0; i < f->linesFile_.n_; i++) {
        uint32_t offset = f->linesFile_.i_[i];
        written = fwrite(&offset, sizeof (char), 3, file);
        if (written != 3) return EX_SOFTWARE;
    }

    return EX_OK;
}

void fileSet(void *f, int n, size_t i) {

    LinesFile *file = (LinesFile *) f;

    file->extent_ = n;
    file->i_ = realloc(file->i_, file->extent_ * i);
}

void fileExtend(void *f, int n, size_t i) {

    LinesFile *file = (LinesFile *) f;

    if (n > file->extent_ - file->n_) {
        fileSet(f, file->extent_ + n * 2, i);
    }
}
