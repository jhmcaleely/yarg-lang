#include "pack.h"

#include "object.h"

#include <stdlib.h>
#include <stdio.h>
#include <sysexits.h>
#include <assert.h>

#define PACKAGE_MAGIC_LEN 6

static int8_t const magic[PACKAGE_MAGIC_LEN] = {0x79, 0x0a, 0x72, 0x67, 0xff, 0x42};
static int16_t const version = 0x2601;

//
// xN is alignment from start of body, number is offset, int16(2)/24(3) types are lsb first
//
// =-= Header =-=
// 0    magic(6) - 79 0a 72 67 00 42
// 6    version(2) - 26, 01 - year, issue (update for any changes to byte-code, number format, file format)
// 8  M chunks(2) -- 0..65535 (0: just global)
// 10 N strings(2) -- max 64k strings
// 12 D doubles(2) -- max 64k floats
// 14 Q ints(2) -- max 64k ints
// 16 L lines(2) -- min zero, max 64k lines - debug/error reporting
// 18   packing(2)
// 20 B body length(4)
// =-= Body =-=
// x8   double D*8
// per chunk -- m == M-1
// x8 K0    num consts in chunk0 1
// x1       code length for chunk0 3
// x4       const offsets indexs -- K0*4 - type(1):index/offset(3)
// x4 K1    num consts in chunk0 1
// x1       code length for chunk0 3
// x4       const indexs -- K1*4
// …
// x4 Km    num consts in chunk0 1
// x1       code length for chunk0 3
// x4       const indexs -- Km*4
// x4   ints *1 -- these could be shrunk by two or four bytes each, but would not then be xip
// x1   function arrities(M-1)*1
// x1   code *1
// x1   code offsets for lines L*3
// x1   function names (M)x16 -- included if (L > 0) debug/error reporting, truncated if over 15 chars

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

typedef struct {
    uint32_t n_;
    uint32_t extent_;
    uint32_t *i_;
} LinesFile;

enum { PACK_CONST_TYPE_S, PACK_CONST_TYPE_I, PACK_CONST_TYPE_D, PACK_CONST_TYPE_F} type_;

typedef struct {
    uint8_t type_;
    uint32_t indexOrOffset_; // offset for s and i, index for d and f
} ConstItem;

// if const were sorted by type ConstItem::type_ and FlatChunk::numConsts_ could be replaced by
//  FlatChunk::numS_, FlatChunk::numI_, FlatChunk::numD_
typedef struct {
    uint8_t *code_;
    int codeLength_;
    int codeOffset_;
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
        int chunkIndex_; // index:const used to update chunksFile when deleting duplicate stringss
        int constNumber_;
    } *i_;
    int totalStringLength_; // including sourceFileName and postfix nulls
} StringsFile;

typedef struct {
    uint32_t n_;
    uint32_t extent_;
    struct DoublesFile_I {
        double d_;
        int chunkIndex_; // index:const used to update chunksFile when deleting duplicate floats
        int constNumber_;
    } *i_;
} DoublesFile;

typedef struct {
    uint32_t n_;
    uint32_t extent_;
    struct {
        ObjInt *i_;
        int chunkIndex_; // index:const used to update chunksFile when deleting duplicate ints
        int constNumber_;
    } *i_;
    int totalIntsLength_;
} IntsFile;

typedef struct {
    uint32_t n_;
    uint32_t extent_;
    ObjFunction **i_;
    int totalIntsLength_;
} FunsFile;

typedef struct {
    ChunksFile chunksFile_;
    LinesFile linesFile_; // only included for debugging/error reporting may be excluded for compact binaries
    StringsFile stringsFile_;
    DoublesFile doublesFile_;
    IntsFile intsFile_;
    FunsFile funsFile_;
} PackageFiles;

static void fileSet(void *, int, size_t i);
static void fileExtend(void *, int, size_t);
static void flattenConstants(Chunk const *, PackageFiles *);
static void flattenCode(Chunk const *, PackageFiles *);
static void flattenLines(Chunk const *, PackageFiles *);
static int pack(char const *, Chunk const *, PackageFiles *, FILE *);

int packScript(char const *sourceFileName, struct Chunk const *chunk, bool includeLines, char const *path) {
    FILE *file = fopen(path, "wb");
    if (file == 0) return EX_DATAERR;

    int r = EX_OK;

    PackageFiles f = {0};
    fileSet(&f.chunksFile_, 4, sizeof *f.chunksFile_.i_);
    if (includeLines) {
        fileSet(&f.linesFile_, 64, sizeof *f.linesFile_.i_);
    }
    fileSet(&f.stringsFile_, 16, sizeof *f.stringsFile_.i_);
    fileSet(&f.doublesFile_, 4, sizeof *f.doublesFile_.i_);
    fileSet(&f.intsFile_, 4, sizeof *f.intsFile_.i_);
    fileSet(&f.funsFile_, 4, sizeof *f.funsFile_.i_);

    f.stringsFile_.totalStringLength_ += strlen(sourceFileName) + 1; // todo also function names for debug/error e.g. do2 is not in code

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
    fclose(file);

    free(f.funsFile_.i_);
    free(f.intsFile_.i_);
    free(f.doublesFile_.i_);
    free(f.stringsFile_.i_);
    free(f.linesFile_.i_);
    free(f.chunksFile_.i_);

    if (r != EX_OK) {
        remove(path);
    }

    return r;
}

struct ObjFunction *loadPackage(char const *path) {
    int r = EX_OK;
    uint8_t *buffer = 0;
    ObjFunction **functions = 0;

    FILE *file = fopen(path, "rb");
    if (file == 0) return 0;

    PackageFileHeader h = {0};
    assert(sizeof h == 24);

    size_t n = fread(&h, sizeof h, 1, file);
    assert(n == 1);

    if (memcmp(h.magic_, magic, PACKAGE_MAGIC_LEN) != 0) {
        r = EX_PROTOCOL;
    }
    if (h.version_ != version) {
        r = EX_DATAERR;
        goto exit;
    }

    buffer = realloc(0, h.bodyLength_);
    assert((long)buffer % 8 == 0);

    size_t bodyLen = fread(buffer, 1, h.bodyLength_, file);
    assert(bodyLen <= h.bodyLength_); // todo why is h.bodyLength_ too long?
    uint8_t const *body = buffer;

    double *doubles = (double *)body;

    for (int i = 0; i < h.numDoubles_; i++) {
        if (i < 7) {
            printf("float@%ld:%f\n", (uint8_t *)&doubles[i] - (uint8_t *)doubles, doubles[i]);
        }
    }

    typedef uint8_t Uint24[3];
    typedef struct {
        uint8_t numConsts_;
        Uint24 codeLength_;
        struct {
            uint8_t type_;
            Uint24 constOffset_;
        } typedIndexs[];
    } PackedChunk;
    PackedChunk const *chunks[256];

    uint8_t const *next = body + h.numDoubles_ * sizeof (double);

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

    ObjFunction *currentFunction;
    functions = realloc(0, h.numChunks_ * sizeof (ObjFunction *));

    for (int i = 1; i < h.numChunks_; i++) {
        currentFunction = functions[i] = newFunction();
        currentFunction->arity = *(uint8_t *)next++;
    }
    functions[0] = newFunction();

    uint32_t codeLen = 0;
    for (int i = 0; i < h.numChunks_; i++) {
        currentFunction = functions[i];
        initDynamicValueArray(&currentFunction->chunk.constants);
        currentFunction->chunk.code = (uint8_t *)next; // discard the const and hope the vm does the right thing
        memcpy(&codeLen, chunks[i]->codeLength_, 3);
        currentFunction->chunk.count = codeLen;
        next += codeLen;
    }

    for (int i = 0; i < h.numLines_; i++) {
        next += 3; // todo
    }

    if (h.numLines_ > 0) {
        for (int i = 0; i < h.numChunks_; i++) {
            functions[i]->fName = copyString((char *)next, (int)strlen((char *)next)); // should be able to shallow copy
            next += 16;
        }
    }

    assert(h.numChunks_ >= 1);
    for (int i = 0; i < h.numChunks_; i++) {
        printf("Chunk:%d code@%p numConsts:%d\n", i, next, (int)chunks[i]->numConsts_);
        currentFunction = functions[i];
        for (int c = 0; c < chunks[i]->numConsts_; c++) {
            uint32_t index = 0;
            memcpy(&index, chunks[i]->typedIndexs[c].constOffset_, 3);
            uint8_t type = chunks[i]->typedIndexs[c].type_;
            static const char *typesName[] = {"string", "int", "float", "fun"};
            printf("%d %s@%u", (int)c, typesName[type], index);
            switch (type) {
            case PACK_CONST_TYPE_S: {
                char const *thisString = (char const *)&stringFile[index];
                ObjString *obj = copyString(thisString, (int)strlen(thisString)); // should be able to shallow copy
                Value value = OBJ_VAL(obj);
                appendToDynamicValueArray(&currentFunction->chunk.constants, value);
                printf(":\"%s\"", thisString);
                break;
            }
            case PACK_CONST_TYPE_I: {
                Int const *thisInt = (Int const *)&intFile[index];
                ObjInt *obj = (ObjInt *)allocateObject(sizeof (ObjInt) + thisInt->m_ * sizeof (uint16_t), OBJ_INT);
                memcpy(&obj->bigInt, thisInt, sizeof (Int) + thisInt->m_ * sizeof (uint16_t)); // should be able to shallow copy
                Value value = OBJ_VAL(obj);
                appendToDynamicValueArray(&currentFunction->chunk.constants, value);
                printf(":");
                int_print(thisInt);
                break;
            }
            case PACK_CONST_TYPE_D: {
                double thisFloat = doubles[index];
                Value value = DOUBLE_VAL(thisFloat);
                appendToDynamicValueArray(&currentFunction->chunk.constants, value); // should be able to shallow copy
                printf(":%f", thisFloat);
                break;
            }
            case PACK_CONST_TYPE_F: {
                ObjFunction *thisFun = functions[index]; // todo figure out index
                Value value = OBJ_VAL(thisFun);
                appendToDynamicValueArray(&currentFunction->chunk.constants, value);
                printf(":%d", index);
                break;
            }
            }
            printf("\n");
        }
    }

exit:
    fclose(file);
    if (functions != 0) {
        currentFunction = functions[0];
        free(functions);
    } else {
        currentFunction = 0;
    }

//   free(buffer); // todo leak for the moment until we can have the buffer owned by functions[0] - perhaps add it as the last chun const?

    if (r == EX_OK)
        return currentFunction;
    else
        return 0;
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
            fc->constTypesAndOffsets_[fc->numConsts_++] = (ConstItem){ .type_ = PACK_CONST_TYPE_D, .indexOrOffset_ = f->doublesFile_.n_ };
            fileExtend(&f->doublesFile_, 4, sizeof *f->doublesFile_.i_);
            f->doublesFile_.i_[f->doublesFile_.n_].d_ = AS_DOUBLE(*v);
            f->doublesFile_.i_[f->doublesFile_.n_].chunkIndex_ =  chunkIndex;
            f->doublesFile_.i_[f->doublesFile_.n_++].constNumber_ =  i;
        } else if (IS_OBJ(*v)) {
            switch (AS_OBJ(*v)->type) {
            case OBJ_FUNCTION: {
                fc->constTypesAndOffsets_[fc->numConsts_++] = (ConstItem){ .type_ = PACK_CONST_TYPE_F, .indexOrOffset_ = f->funsFile_.n_ };
                fileExtend(&f->funsFile_, 4, sizeof *f->funsFile_.i_);
                f->funsFile_.i_[f->funsFile_.n_++] = AS_FUNCTION(*v);
                break;
            }
            case OBJ_INT: {
                fc->constTypesAndOffsets_[fc->numConsts_++] = (ConstItem){ .type_ = PACK_CONST_TYPE_I, .indexOrOffset_ = f->intsFile_.totalIntsLength_ };
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
                fc->constTypesAndOffsets_[fc->numConsts_++] = (ConstItem){ .type_ = PACK_CONST_TYPE_S, .indexOrOffset_ = f->stringsFile_.totalStringLength_ };
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

    fileSet(&f->chunksFile_, f->funsFile_.n_ + 1, sizeof *f->chunksFile_.i_);
    f->chunksFile_.n_ = chunkIndex + 1;
    for (int i = 0; i < chunk->constants.count; i++) {
        Value *v = &chunk->constants.values[i];
        if (IS_OBJ(*v)) {
            switch (AS_OBJ(*v)->type) {
            case OBJ_FUNCTION:
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

    memcpy(&h.magic_, magic, PACKAGE_MAGIC_LEN);
    h.version_ = version;
    h.numChunks_ = f->chunksFile_.n_;
    h.numStrings_ = f->stringsFile_.n_;
    h.numInts_ = f->intsFile_.n_;
    h.numDoubles_ = f->doublesFile_.n_;
    h.numLines_ = f->linesFile_.n_;
    h.bodyLength_ = 0;

    h.bodyLength_ += h.numDoubles_ * sizeof (double);
    for (int i = 0; i < f->chunksFile_.n_; i++) {
        h.bodyLength_ += 1 + 3 + 4 * f->chunksFile_.i_[i].numConsts_;
    }
    assert(h.bodyLength_ % 4 == 0);
    h.bodyLength_ += f->intsFile_.totalIntsLength_;
    assert(h.bodyLength_ % 4 == 0);
    h.bodyLength_ += f->stringsFile_.totalStringLength_;
    h.bodyLength_ += h.numChunks_ - 1;
    h.bodyLength_ += f->chunksFile_.totalCodeLength_;
    h.bodyLength_ += 3 * h.numLines_;
    if (h.numLines_ > 0) {
        h.bodyLength_ += 16 * h.numChunks_;
    }

    size_t written = fwrite(&h, sizeof h, 1, file);
    if (written != 1) return EX_SOFTWARE;

    for (int i = 0; i < f->doublesFile_.n_; i++) {
        struct DoublesFile_I *di = &f->doublesFile_.i_[i];
        written = fwrite(&di->d_, sizeof (double), 1, file);
        if (written != 1) return EX_SOFTWARE;
    }

    for (int i = 0; i < f->chunksFile_.n_; i++) {
        FlatChunk *fc = &f->chunksFile_.i_[i];

        written = fwrite(&fc->numConsts_, sizeof (char), 1, file);
        if (written != 1) return EX_SOFTWARE;
        written = fwrite(&fc->codeLength_, sizeof (char), 3, file);
        if (written != 3) return EX_SOFTWARE;
        for (int k = 0; k < fc->numConsts_; k++) {
            ConstItem *ci = &fc->constTypesAndOffsets_[k];
            assert(ci->type_ >= 0 && ci->type_ <= 3);
            written = fwrite(&ci->type_, sizeof (char), 1, file);
            if (written != 1) return EX_SOFTWARE;
            written = fwrite(&ci->indexOrOffset_, sizeof (char), 3, file);
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
    if (written != 1) return EX_SOFTWARE;
    for (int i = 0; i < f->stringsFile_.n_; i++) {
        ObjString *s = f->stringsFile_.i_[i].s_;
        written = fwrite(s->chars, s->length, 1, file);
        if (written != 1) return EX_SOFTWARE;
        written = fwrite(nullChar, 1, 1, file);
        if (written != 1) return EX_SOFTWARE;
    }

    for (int i = 0; i < f->funsFile_.n_; i++) {
        uint8_t arity = f->funsFile_.i_[i]->arity;
        written = fwrite(&arity, sizeof (char), 1, file);
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

    if (f->linesFile_.n_ > 0) {
        for (int i = 0; i < f->chunksFile_.n_; i++) {
            char const *name;
            int len;
            if (i == 0) {
                name = sourceFileName;
                len = (int)sourceFileNameLen;
            } else {
                name = f->funsFile_.i_[i - 1]->fName->chars;
                len = f->funsFile_.i_[i - 1]->fName->length;
            }
            if (len > 15) len = 15;
            written = fwrite(name, sizeof (char), len, file);
            if (written != len) return EX_SOFTWARE;
            for (; len < 16; len++) {
                written = fwrite(nullChar, sizeof (char), 1, file);
                if (written != 1) return EX_SOFTWARE;
            }
        }
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
