#include "pack.h"

#include "object.h"
#include "memory.h"

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
// x8 K0    num consts in chunk0 2
// x1       code length for chunk0 2
// x1       0 2
// x1       0 2
// x4       const offsets indexs -- K0*4 - type(1):index/offset(3)
// x4 K1    num consts in chunk0 2
// x1       code length for chunk0 2
// x1       arity 2
// x1       num upvalues 2
// x4       const indexs -- K1*4
// …
// x4 Km    num consts in chunk0 2
// x1       code length for chunk0 2
// x1       arity 2
// x1       num upvalues 2
// x4       const indexs -- Km*4
// x4   ints *1 -- these could be shrunk by two or four bytes each, but would not then be xip
// x1   strings *1
// x1   function arities(M-1)*1
// x1   code *1
// x1   code offsets for lines L*3
// x1   function names (M)x16 -- included if (L > 0) debug/error reporting, truncated if over 15 chars

uint32_t chunks__ = 0;
uint32_t ints__ = 0;
uint32_t strings__ = 0;
uint32_t code__ = 0;
uint32_t lines__ = 0;
uint32_t names__ = 0;
uint32_t end__ = 0;

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
    struct ConstTypesAndOffsets {
        uint32_t numConsts_;
        uint32_t extent_;
        ConstItem *i_;
    } constTypesAndOffsets_;
} FlatChunk;

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
    struct Fun {
        ObjFunction const *f_;
        FlatChunk chunk_;
    } *i_;
    int totalCodeLength_;
} FunsFile;

typedef struct {
    LinesFile linesFile_; // only included for debugging/error reporting may be excluded for compact binaries
    StringsFile stringsFile_;
    DoublesFile doublesFile_;
    IntsFile intsFile_;
    FunsFile funsFile_;
} FlatFiles;

static void fileSet(void *, int, size_t i);
static void fileExtend(void *, int, size_t);
static void flattenConstants(int funIndex, Chunk const *, FlatFiles *);
static void flattenCode(FlatFiles *);
static void flattenLines(FlatFiles *);
static int pack(char const *, FlatFiles *, FILE *);

int packScript(char const *sourceFileName, struct ObjFunction const *scriptFn, bool includeLines, char const *path) {
    FILE *file = fopen(path, "wb");
    if (file == 0) return EX_DATAERR;

    int r = EX_OK;

    FlatFiles f = {0};
    if (includeLines) {
        fileSet(&f.linesFile_, 64, sizeof *f.linesFile_.i_);
    }
    fileSet(&f.stringsFile_, 16, sizeof *f.stringsFile_.i_);
    fileSet(&f.doublesFile_, 4, sizeof *f.doublesFile_.i_);
    fileSet(&f.intsFile_, 4, sizeof *f.intsFile_.i_);
    fileSet(&f.funsFile_, 4, sizeof *f.funsFile_.i_);

    f.funsFile_.n_ = 1;
    f.funsFile_.i_[0].f_ = scriptFn;
    f.funsFile_.totalCodeLength_ = 0;
    flattenConstants(0, &scriptFn->chunk, &f);
    int numFuns = f.funsFile_.n_;

    // todo remove duplicates
    // todo remove non global, non returned method/property

    flattenCode(&f);

    if (includeLines) {
        flattenLines(&f);
    }

    r = pack(sourceFileName, &f, file);
    if (r != EX_OK) goto exit;

exit:
    fclose(file);

    for (int i = 0; i < f.funsFile_.n_; i++) {
        free(f.funsFile_.i_[i].chunk_.constTypesAndOffsets_.i_);
    }
    free(f.funsFile_.i_);
    free(f.intsFile_.i_);
    free(f.doublesFile_.i_);
    free(f.stringsFile_.i_);
    free(f.linesFile_.i_);

    if (r != EX_OK) {
        remove(path);
    }

    return r;
}

struct ObjFunction *loadPackage(char const *path) {
    int r = EX_OK;
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

    uint8_t* const body = realloc(0, h.bodyLength_);
    assert((long)body % 8 == 0);

    size_t bodyLen = fread(body, 1, h.bodyLength_, file);
    assert(bodyLen <= h.bodyLength_); // todo why is h.bodyLength_ too long?

    double *doubles = (double *)body;

    for (int i = 0; i < h.numDoubles_; i++) {
        if (i < 7) {
            printf("float@%ld:%f\n", (uint8_t *)&doubles[i] - (uint8_t *)doubles, doubles[i]);
        }
    }

    typedef uint8_t Uint24[3];
    typedef struct {
        uint16_t numConsts_;
        uint16_t codeLength_;
        uint16_t arity_;
        uint16_t numUpvalues_;
        struct {
            uint8_t type_;
            Uint24 constOffset_;
        } typedIndexs[];
    } PackedChunk;
    PackedChunk const *chunks[256];

    uint8_t const *next = body + h.numDoubles_ * sizeof (double);

    chunks__ = next - body;
    for (int i = 0; i < h.numChunks_; i++) {
        chunks[i] = (PackedChunk const *)next;
        next += 8 + 4 * chunks[i]->numConsts_;
    }
    uint8_t const *intFile = next;

    ints__ = next - body;
    for (int i = 0; i < h.numInts_; i++) {
        Int const *thisInt = (Int const *)next;
        if (i < 11) {
            printf("int@%ld:", (uint8_t *)thisInt - intFile); int_print(thisInt); printf("\n");
        }
        next += sizeof (Int) + sizeof (uint16_t) * thisInt->m_;
    }
    uint8_t const *stringFile = next;

    strings__ = next - body;
    for (int i = 0; i < h.numStrings_; i++) {
        char const *thisString = (char const *)next;
        if (i < 13) {
            printf("string@%ld:\"%s\"\n", (uint8_t *)thisString - stringFile, thisString);
        }
        next += strlen(thisString) + 1;
//        printf("%s, %u, %u\n", thisString, (uint32_t)((next - body) - strings__), (uint32_t)(next - body));
    }

    ObjFunction *currentFunction;
    functions = realloc(0, h.numChunks_ * sizeof (ObjFunction *));

    for (int i = 0; i < h.numChunks_; i++) {
        functions[i] = newFunction();
    }

    uint8_t const *startOfCode = next;
    code__ = next - body;
    for (int i = 0; i < h.numChunks_; i++) {
        currentFunction = functions[i];
        initDynamicValueArray(&currentFunction->chunk.constants);
        currentFunction->chunk.xip = true;
        currentFunction->chunk.code = (uint8_t *)next; // discard the const and hope the vm does the right thing
        currentFunction->chunk.count = chunks[i]->codeLength_;
        currentFunction->arity = chunks[i]->arity_;
        currentFunction->upvalueCount = chunks[i]->numUpvalues_;
        next += chunks[i]->codeLength_;
    }

    lines__ = next - body;
//    printf("line, offset\n");
    for (int i = 0; i < h.numLines_; i++) {
        int offset = 0;
        memcpy(&offset, next, 3);
        if (offset != 0xffffff) {
            for (int c = 0; c < h.numChunks_; c++) {
                ObjFunction *cf = functions[c];
                size_t start = cf->chunk.code - startOfCode;
                size_t end = start + cf->chunk.count;
                if (offset >= start && offset < end) {
//                   printf("%d, %d, %d\n", i + 1, offset, offset - (int)start);
                    if (cf->chunk.numLines == cf->chunk.lineCapacity) {
                        int capacity = cf->chunk.lineCapacity;
                        int newCapacity = capacity + 16;
                        cf->chunk.lineCapacity = newCapacity;
                        cf->chunk.lines = reallocate(cf->chunk.lines, sizeof (ChunkSource) * capacity, sizeof (ChunkSource) * newCapacity);
                    }
                    cf->chunk.lines[cf->chunk.numLines++] = (ChunkSource){ .address = offset - start, .line = i + 1};
                    break;
                }
            }
        }
        next += 3;
    }

    names__ = next - body;
    if (h.numLines_ > 0) {
        for (int i = 0; i < h.numChunks_; i++) {
            functions[i]->fName = copyString((char *)next, (int)strlen((char *)next)); // should be able to shallow copy
            next += 16;
        }
    }

    end__ = next - body;
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
                ObjFunction *thisFun = functions[index];
                Value value = OBJ_VAL(thisFun);
                appendToDynamicValueArray(&currentFunction->chunk.constants, value);
                if (thisFun->fName != 0) {
                    char name[21];
                    int len = thisFun->fName->length;
                    len = len > 20 ? 20 : len;
                    memcpy(name, thisFun->fName->chars, len);
                    name[len] = '\0';
                    printf(":%s", name);
                } else {
                    printf(":<no name>");
                }
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
    printf("chunks__:%u\nints__:%u\nstrings__:%u\ncode__:%u\nlines__:%u\nnames__:%u\nend__:%u\n",
           chunks__, ints__, strings__, code__, lines__, names__, end__);

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

void flattenConstants(int chunkIndex, Chunk const *chunk, FlatFiles *f) {
    int startSubs = f->funsFile_.n_;
    int constCount = chunk->constants.count;

    ObjFunction const *cf = f->funsFile_.i_[chunkIndex].f_;
    FlatChunk *fc = &f->funsFile_.i_[chunkIndex].chunk_;

    fc->code_ = cf->chunk.code;
    fc->codeLength_ = cf->chunk.count;
    fc->codeOffset_ = f->funsFile_.totalCodeLength_;
    f->funsFile_.totalCodeLength_ += fc->codeLength_;

    fc->constTypesAndOffsets_.numConsts_ = 0;
    fc->constTypesAndOffsets_.i_ = 0;
    fileSet(&fc->constTypesAndOffsets_, constCount, sizeof *fc->constTypesAndOffsets_.i_);
    fc->constTypesAndOffsets_.numConsts_ = constCount;

    for (int i = 0; i < constCount; i++) {
        Value *v = &chunk->constants.values[i];
        if (IS_DOUBLE(*v)) {
            fc->constTypesAndOffsets_.i_[i] = (ConstItem){ .type_ = PACK_CONST_TYPE_D, .indexOrOffset_ = f->doublesFile_.n_ };
            fileExtend(&f->doublesFile_, 4, sizeof *f->doublesFile_.i_);
            f->doublesFile_.i_[f->doublesFile_.n_].d_ = AS_DOUBLE(*v);
            f->doublesFile_.i_[f->doublesFile_.n_].chunkIndex_ =  chunkIndex;
            f->doublesFile_.i_[f->doublesFile_.n_++].constNumber_ =  i;
        } else if (IS_OBJ(*v)) {
            switch (AS_OBJ(*v)->type) {
            case OBJ_FUNCTION: {
                fc->constTypesAndOffsets_.i_[i] = (ConstItem){ .type_ = PACK_CONST_TYPE_F, .indexOrOffset_ = f->funsFile_.n_ };
                fileExtend(&f->funsFile_, 4, sizeof *f->funsFile_.i_);
                fc = &f->funsFile_.i_[chunkIndex].chunk_; // fc invalidated by fileExtend() above
                f->funsFile_.i_[f->funsFile_.n_++].f_ = AS_FUNCTION(*v);
                break;
            }
            case OBJ_INT: {
                fc->constTypesAndOffsets_.i_[i] = (ConstItem){ .type_ = PACK_CONST_TYPE_I, .indexOrOffset_ = f->intsFile_.totalIntsLength_ };
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
                fc->constTypesAndOffsets_.i_[i] = (ConstItem){ .type_ = PACK_CONST_TYPE_S, .indexOrOffset_ = f->stringsFile_.totalStringLength_ };
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

    int endSubs = f->funsFile_.n_;
    for (int i = startSubs; i < endSubs; i++) {
        char nn[21];
        int len = f->funsFile_.i_[i].f_->fName->length;
        len = len > 20 ? 20 : len;
        memcpy(nn, f->funsFile_.i_[i].f_->fName->chars, len);
        nn[len] = 0;
        printf("%s\n", nn);
        flattenConstants(i, &f->funsFile_.i_[i].f_->chunk, f);
    }
}

static void flattenCode(FlatFiles *f) {

}

void flattenLines(FlatFiles *f) {

    for (int c = 0; c < f->funsFile_.n_; c++) {
        FlatChunk *cf = &f->funsFile_.i_[c].chunk_;
        Chunk const *chunk = &f->funsFile_.i_[c].f_->chunk;

        if (chunk->numLines > 0) {
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
        }
    }
}

uint32_t offset__ = 0;
size_t fwrite__(const void *__ptr, size_t __size, size_t __nitems, FILE *__stream) {
    size_t w = fwrite(__ptr, __size, __nitems, __stream);
    offset__ += (uint32_t)(__size * w);
    return w;
}

int pack(char const *sourceFileName, FlatFiles *f, FILE *file) {

    PackageFileHeader h;

    memcpy(&h.magic_, magic, PACKAGE_MAGIC_LEN);
    h.version_ = version;
    h.numChunks_ = f->funsFile_.n_;
    h.numStrings_ = f->stringsFile_.n_;
    h.numInts_ = f->intsFile_.n_;
    h.numDoubles_ = f->doublesFile_.n_;
    h.numLines_ = f->linesFile_.n_;
    h.packing_ = 0xffff;
    h.bodyLength_ = 0;

    h.bodyLength_ += h.numDoubles_ * sizeof (double);

    h.bodyLength_ += 8 * f->funsFile_.n_;
    for (int i = 0; i < f->funsFile_.n_; i++) {
        h.bodyLength_ += 4 * f->funsFile_.i_[i].chunk_.constTypesAndOffsets_.numConsts_;
    }
    h.bodyLength_ += f->intsFile_.totalIntsLength_;
    assert(h.bodyLength_ % 4 == 0);
    h.bodyLength_ += f->stringsFile_.totalStringLength_;
    h.bodyLength_ += f->funsFile_.totalCodeLength_;
    h.bodyLength_ += 3 * h.numLines_;
    if (h.numLines_ > 0) {
        h.bodyLength_ += 16 * h.numChunks_;
    }

    size_t written = fwrite__(&h, sizeof h, 1, file);
    if (written != 1) return EX_SOFTWARE;

    offset__ = 0;
    for (int i = 0; i < f->doublesFile_.n_; i++) {
        struct DoublesFile_I *di = &f->doublesFile_.i_[i];
        written = fwrite__(&di->d_, sizeof (double), 1, file);
        if (written != 1) return EX_SOFTWARE;
    }

    chunks__ = offset__;
    for (int i = 0; i < f->funsFile_.n_; i++) {
        FlatChunk *fc = &f->funsFile_.i_[i].chunk_;
        uint16_t arity, numUpvalues;
        if (i == 0) {
            arity = numUpvalues = 0;
        } else {
            ObjFunction const *fn = f->funsFile_.i_[i].f_;
            arity = fn->arity;
            numUpvalues = fn->upvalueCount;
        }

        written = fwrite__(&fc->constTypesAndOffsets_.numConsts_, sizeof (uint16_t), 1, file);
        if (written != 1) return EX_SOFTWARE;
        written = fwrite__(&fc->codeLength_, sizeof (uint16_t), 1, file);
        if (written != 1) return EX_SOFTWARE;
        written = fwrite__(&arity, sizeof (uint16_t), 1, file);
        if (written != 1) return EX_SOFTWARE;
        written = fwrite__(&numUpvalues, sizeof (uint16_t), 1, file);
        if (written != 1) return EX_SOFTWARE;
        for (int k = 0; k < fc->constTypesAndOffsets_.numConsts_; k++) {
            ConstItem *ci = &fc->constTypesAndOffsets_.i_[k];
            assert(ci->type_ >= 0 && ci->type_ <= 3);
            written = fwrite__(&ci->type_, sizeof (char), 1, file);
            if (written != 1) return EX_SOFTWARE;
            written = fwrite__(&ci->indexOrOffset_, sizeof (char), 3, file);
            if (written != 3) return EX_SOFTWARE;
        }
    }

    ints__ += offset__;
    for (int i = 0; i < f->intsFile_.n_; i++) {
        Int *bigInt = &f->intsFile_.i_[i].i_->bigInt;
        IntConcrete254 t;
        int_set_t(bigInt, int_init_concrete254(&t));
        t.m_ = t.d_ + t.d_ % 2;
        written = fwrite__(&t, sizeof (Int) + sizeof (uint16_t) * t.m_, 1, file);
        if (written != 1) return EX_SOFTWARE;
    }

    strings__ += offset__;
    size_t sourceFileNameLen = strlen(sourceFileName);
    char const *nullChar = &sourceFileName[sourceFileNameLen];
    for (int i = 0; i < f->stringsFile_.n_; i++) {
        ObjString *s = f->stringsFile_.i_[i].s_;
        written = fwrite__(s->chars, s->length, 1, file);
        if (written != 1) return EX_SOFTWARE;
        written = fwrite__(nullChar, 1, 1, file);
        if (written != 1) return EX_SOFTWARE;
//        char sb[100];
//        memcpy(sb, s->chars, s->length); sb[s->length] = '\0';
//        printf("%s, %u, %u\n", sb, offset__ - strings__, offset__);
    }

    code__ += offset__;
    for (int i = 0; i < f->funsFile_.n_; i++) {
        FlatChunk *fc = &f->funsFile_.i_[i].chunk_;
        written = fwrite__(fc->code_, fc->codeLength_, 1, file);
        if (written != 1) return EX_SOFTWARE;
    }

    lines__ += offset__;
    for (int i = 0; i < f->linesFile_.n_; i++) {
        uint32_t offset = f->linesFile_.i_[i];
        written = fwrite__(&offset, sizeof (char), 3, file);
        if (written != 3) return EX_SOFTWARE;
    }

    names__ += offset__;
    if (f->linesFile_.n_ > 0) {
        for (int i = 0; i < f->funsFile_.n_; i++) {
            char const *name;
            int len;
            if (i == 0) {
                name = sourceFileName;
                len = (int)sourceFileNameLen;
            } else {
                name = f->funsFile_.i_[i].f_->fName->chars;
                len = f->funsFile_.i_[i].f_->fName->length;
            }
            if (len > 15) len = 15;
            written = fwrite__(name, sizeof (char), len, file);
            if (written != len) return EX_SOFTWARE;
            for (; len < 16; len++) {
                written = fwrite__(nullChar, sizeof (char), 1, file);
                if (written != 1) return EX_SOFTWARE;
            }
            printf("%s\n", name);
        }
    }
    end__ += offset__;
    printf("chunks__:%u\nints__:%u\nstrings__:%u\ncode__:%u\nlines__:%u\nnames__:%u\nend__:%u\n",
           chunks__, ints__, strings__, code__, lines__, names__, end__);

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
