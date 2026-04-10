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
    char *i_;
} CodeFile;

typedef struct {
    uint32_t n_;
    uint32_t extent_;
    uint32_t *i_;
} LinesFile;

typedef struct {
    enum { PACK_CONST_S, PACK_CONST_I, PACK_CONST_D } type_;
    uint16_t index_;
} ConstItems;

typedef struct {
    uint32_t codeOffset;
    uint8_t numConsts;
    ConstItems constOffsets_[256];
} ChunkFile;

typedef struct {
    uint32_t n_;
    uint32_t extent_;
    ChunkFile *i_;
} ChunksFile;

typedef struct {
    uint32_t n_;
    uint32_t extent_;
    struct {
        ObjString *s_;
        int chunkIndex_;
        int constNumber_;
    } *i_;
    int totalStringLength_;
} StringsFile;

typedef struct {
    uint32_t n_;
    uint32_t extent_;
    struct {
        double f_;
        int chunkIndex_;
        int constNumber_;
    } *i_;
} DoublesFile;

typedef struct {
    uint32_t n_;
    uint32_t extent_;
    struct {
        ObjInt *i_;
        int chunkIndex_;
        int constNumber_;
    } *i_;
    int totalIntsLength_;
} IntsFile;

typedef struct {
    ChunksFile chunksFile_;
    CodeFile codeFile_;
    LinesFile linesFile_;
    StringsFile stringsFile_;
    DoublesFile doublesFile_;
    IntsFile intsFile_;
} PackageFiles;

static void fileExtend(void *, int, size_t);
static void packConstants(Chunk const *, bool, PackageFiles *);
static void packCode(Chunk const *, bool, PackageFiles *);
static int packInstruction(Chunk const *, int, PackageFiles *);

int packChunks(char const *sourceFileName, Chunk const *chunk, bool includeLines, FILE *file)
{
    int r = EX_OK;
    PackageFiles f = {0};

    fileExtend(&f.chunksFile_, 4, sizeof *f.chunksFile_.i_);
    fileExtend(&f.codeFile_, 512, sizeof *f.codeFile_.i_);
    fileExtend(&f.linesFile_, 128, sizeof *f.linesFile_.i_);
    fileExtend(&f.stringsFile_, 16, sizeof *f.stringsFile_.i_);
    fileExtend(&f.doublesFile_, 4, sizeof *f.doublesFile_.i_);
    fileExtend(&f.intsFile_, 16, sizeof *f.intsFile_.i_);

    // todo add sourceFileName to strings

    f.chunksFile_.n_ = 1;
    packConstants(chunk, includeLines, &f);

    // todo remove duplicates
    // todo remove non global, non returned method/property

    f.chunksFile_.n_ = 1;
    packCode(chunk, includeLines, &f);

    PackageFileHeader h;
    assert(sizeof h == 24);

    // generate

    memcpy(&h.magic_, &(int8_t [PACKAGE_MAGIC_LEN]){0x79, 0x0a, 0x72, 0x67, 0x00, 0x42}, 6);
    h.version_ = 0x2601;
    h.numChunks_ = f.chunksFile_.n_;
    h.stringHashMod_ = f.stringsFile_.n_ < 765 ? (uint8_t) f.stringsFile_.n_ / 3 : 255;
    h.codeLen_ = (f.codeFile_.n_ + 3) / 4;
    h.stringsLen_ = (f.stringsFile_.totalStringLength_ + 3) / 4;
    h.intsLen_= (f.intsFile_.totalIntsLength_ + 3) / 4;
    h.numStrings_ = f.stringsFile_.n_;
    h.numInts_ = f.intsFile_.n_;
    h.numDoubles_ = f.doublesFile_.n_;
    h.numLines_ = f.linesFile_.n_;

    size_t written = fwrite(&h, sizeof h, 1, file);
    if (written != 1) {
        r = EX_SOFTWARE;
        goto exit;
    }

exit:
    free(f.intsFile_.i_);
    free(f.doublesFile_.i_);
    free(f.stringsFile_.i_);
    free(f.linesFile_.i_);
    free(f.codeFile_.i_);
    free(f.chunksFile_.i_);

    return r;
}

/////  todo  file and sort strings first
//////            mark constants as used when parsing byte code
//////            methods + properties of returned struct or class, global var names, 1st level function names are exported, op_const are not
///////            returned includes copied to var parameters
//////            remove non-global consts

void packConstants(Chunk const *chunk, bool includeLines, PackageFiles *f) {
    for (int i = 0; i < chunk->constants.count; i++) {
        Value *v = &chunk->constants.values[i];
        if (IS_DOUBLE(*v)) {
            fileExtend(&f->doublesFile_, 4, sizeof *f->doublesFile_.i_);
            f->doublesFile_.i_[f->doublesFile_.n_++].f_ = AS_DOUBLE(*v);
            f->doublesFile_.i_[f->doublesFile_.n_++].chunkIndex_ =  f->chunksFile_.n_ - 1;
            f->doublesFile_.i_[f->doublesFile_.n_++].constNumber_ =  i;
        } else if (IS_OBJ(*v)) {
            switch (AS_OBJ(*v)->type) {
            case OBJ_FUNCTION:
                break;
            case OBJ_INT: {
                fileExtend(&f->intsFile_, 16, sizeof *f->intsFile_.i_);
                f->intsFile_.i_[f->intsFile_.n_++].i_ = AS_INTOBJ(*v);
                f->intsFile_.i_[f->intsFile_.n_++].chunkIndex_ =  f->chunksFile_.n_ - 1;
                f->intsFile_.i_[f->intsFile_.n_++].constNumber_ =  i;
                Int *from = AS_INT(*v);
                int len = (int)((char *) from->w_ - (char *) from);
                assert(len % 4 == 0);
                len += sizeof (uint16_t) * from->d_;
                len += len % 4;
                f->intsFile_.totalIntsLength_ += len;
                break;
            }
            case OBJ_STRING: {
                fileExtend(&f->stringsFile_, 16, sizeof *f->stringsFile_.i_);
                ObjString *from = AS_STRING(*v);
                f->stringsFile_.i_[f->stringsFile_.n_++].s_ = from;
                f->stringsFile_.i_[f->stringsFile_.n_++].chunkIndex_ =  f->chunksFile_.n_ - 1;
                f->stringsFile_.i_[f->stringsFile_.n_++].constNumber_ =  i;
                int len = from->length;
                f->stringsFile_.totalStringLength_ += len;
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
                fileExtend(&f->chunksFile_, 4, sizeof *f->chunksFile_.i_);
                f->chunksFile_.n_++;
                packConstants(&AS_FUNCTION(*v)->chunk, includeLines, f);
            default:
                break;
            }
        }
    }
}

static void packCode(Chunk const *chunk, bool includeLines, PackageFiles *f) {
    int chunkIndex = f->chunksFile_.n_ - 1;

    fileExtend(&f->codeFile_, 512, sizeof *f->codeFile_.i_);

    for (int offset = 0; offset < chunk->count;) {
        offset = packInstruction(chunk, offset, f);
    }

    for (int i = 0; i < chunk->constants.count; i++) {
        Value *v = &chunk->constants.values[i];
        if (IS_OBJ(*v)) {
            switch (AS_OBJ(*v)->type) {
            case OBJ_FUNCTION:
                f->chunksFile_.n_++;
                packCode(&AS_FUNCTION(*v)->chunk, includeLines, f);
            default:
                break;
            }
        }
    }
}

static char const *valueType(Value *v) {
    switch (v->type) {
    case VAL_BOOL: return "bool";
    case VAL_NIL: return "";
    case VAL_DOUBLE: return "double";
    case VAL_I8: return "i8";
    case VAL_UI8: return "ui8";
    case VAL_I16: return "i16";
    case VAL_UI16: return "ui16";
    case VAL_I32: return "i32";
    case VAL_UI32: return "ui32";
    case VAL_UI64: return "ui64";
    case VAL_I64: return "i64";
    case VAL_ADDRESS: return "address";
    case VAL_OBJ:
        switch (AS_OBJ(*v)->type) {
        case OBJ_INT: return "int";
        case OBJ_STRING: return "string";
        default: return "valueType.Obj?";
        }
    default: return "valueType?";
    }
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d %s:'", name, constant, valueType(&chunk->constants.values[constant]));
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

static int invokeInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    uint8_t argCount = chunk->code[offset + 2];
    printf("%-16s (%d args) %4d:'", name, argCount, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 3;
}

static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int byteInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int twoByteInstruction(const char* name, Chunk* chunk, int offset) {
    uint16_t slot = chunk->code[offset + 1];
    slot += chunk->code[offset + 2] * 256;
    printf("%-16s %6d\n", name, slot);
    return offset + 3;
}

static int threeByteInstruction(const char* name, Chunk* chunk, int offset) {
    uint32_t slot = chunk->code[offset + 1];
    slot += chunk->code[offset + 2] * 256;
    slot += chunk->code[offset + 3] * 65536;
    printf("%-16s %9d\n", name, slot);
    return offset + 4;
}

static int jumpInstruction(const char* name, int sign, Chunk* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static int builtinInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s ", name);
    switch (slot) {
        case BUILTIN_PEEK: printf("peek"); break;
        case BUILTIN_IMPORT: printf("import"); break;
        case BUILTIN_READ_SOURCE: printf("read_source"); break;
        case BUILTIN_EXEC: printf("exec"); break;
        case BUILTIN_COMPILE: printf("compile"); break;
        case BUILTIN_MAKE_ROUTINE: printf("make_routine"); break;
        case BUILTIN_MAKE_CHANNEL: printf("make_channel"); break;
        case BUILTIN_MAKE_SYNCGROUP: printf("make_sync_group"); break;
        case BUILTIN_RESUME: printf("resume"); break;
        case BUILTIN_START: printf("start"); break;
        case BUILTIN_SEND: printf("send"); break;
        case BUILTIN_RECEIVE: printf("receive"); break;
        case BUILTIN_SHARE: printf("share"); break;
        case BUILTIN_CPEEK: printf("cpeek"); break;
        case BUILTIN_LEN: printf("len"); break;
        case BUILTIN_PIN: printf("pin"); break;
        case BUILTIN_NEW: printf("new"); break;
        case BUILTIN_INT8: printf("int8"); break;
        case BUILTIN_UINT8: printf("uint8"); break;
        case BUILTIN_INT16: printf("int16"); break;
        case BUILTIN_UINT16: printf("uint16"); break;
        case BUILTIN_INT32: printf("int32"); break;
        case BUILTIN_UINT32: printf("uint32"); break;
        case BUILTIN_INT64: printf("int64"); break;
        case BUILTIN_UINT64: printf("uint64"); break;
        case BUILTIN_TS_SET: printf("test_set"); break;
        case BUILTIN_TS_READ: printf("test_read"); break;
        case BUILTIN_TS_WRITE: printf("test_write"); break;
        case BUILTIN_TS_INTERRUPT: printf("test_interrupt"); break;
        case BUILTIN_TS_SYNC: printf("test_sync"); break;
        case BUILTIN_INT: printf("int"); break;
        case BUILTIN_MFLOAT64:  printf("mfloat64"); break;
        case BUILTIN_STRING: printf("string"); break;
        default: printf("<unknown %4d>", slot); break;
    }
    printf("\n");
    return offset + 2;
}

static int typeLiteralInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t type = chunk->code[offset + 1];
    printf("%-16s ", name);
    switch (type) {
        case TYPE_MODIFIER_CONST: printf("const"); break;
        case TYPE_LITERAL_BOOL: printf("bool"); break;
        case TYPE_LITERAL_INT8: printf("int8"); break;
        case TYPE_LITERAL_UINT8: printf("uint8"); break;
        case TYPE_LITERAL_INT16: printf("int16"); break;
        case TYPE_LITERAL_UINT16: printf("uint16"); break;
        case TYPE_LITERAL_INT32: printf("int32"); break;
        case TYPE_LITERAL_UINT32: printf("uint32"); break;
        case TYPE_LITERAL_INT64: printf("int64"); break;
        case TYPE_LITERAL_UINT64: printf("uint64"); break;
        case TYPE_LITERAL_MACHINE_FLOAT64: printf("mfloat64"); break;
        case TYPE_LITERAL_STRING: printf("string"); break;
        case TYPE_LITERAL_INT: printf("int"); break;
        default: printf("<unknown %4d>", type); break;
    }
    printf("\n");
    return offset + 2;
}

int packInstruction(Chunk const *chunk, int offset, PackageFiles *f) {
    printf("%04d ", offset);
    for (int s = 0;; s++) {
        if (chunk->lines != 0 && chunk->lines[s].address == offset) {
            printf("%4d ", chunk->lines[s].line);
            break;
        } else if (s == chunk->numLines || chunk->lines[s].address > offset) {
            printf("   | ");
            break;
        }
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_GET_BUILTIN:
            return builtinInstruction("OP_GET_BUILTIN", chunk, offset);
        case OP_GET_LOCAL:
            return byteInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return byteInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_INITIALISE:
            return simpleInstruction("OP_INITIALISE", offset);
        case OP_GET_UPVALUE:
            return byteInstruction("OP_GET_UPVALUE", chunk, offset);
        case OP_SET_UPVALUE:
            return byteInstruction("OP_SET_UPVALUE", chunk, offset);
        case OP_GET_PROPERTY:
            return constantInstruction("OP_GET_PROPERTY", chunk, offset);
        case OP_SET_PROPERTY:
            return constantInstruction("OP_SET_PROPERTY", chunk, offset);
        case OP_GET_SUPER:
            return constantInstruction("OP_GET_SUPER", chunk, offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_LEFT_SHIFT:
            return simpleInstruction("OP_LEFT_SHIFT", offset);
        case OP_RIGHT_SHIFT:
            return simpleInstruction("OP_RIGHT_SHIFT", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_BITOR:
            return simpleInstruction("OP_BITOR", offset);
        case OP_BITAND:
            return simpleInstruction("OP_BITAND", offset);
        case OP_BITXOR:
            return simpleInstruction("OP_BITXOR", offset);
        case OP_MODULO:
            return simpleInstruction("OP_MODULO", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_POKE:
            return simpleInstruction("OP_POKE", offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", -1, chunk, offset);
        case OP_CALL:
            return byteInstruction("OP_CALL", chunk, offset);
        case OP_INVOKE:
            return invokeInstruction("OP_INVOKE", chunk, offset);
        case OP_SUPER_INVOKE:
            return invokeInstruction("OP_SUPER_INVOKE", chunk, offset);
        case OP_CLOSURE: {
            offset++;
            uint8_t constant = chunk->code[offset++];
            printf("%-16s %4d ", "OP_CLOSURE", constant);
            printValue(chunk->constants.values[constant]);
            printf("\n");

            ObjFunction* function = AS_FUNCTION(
                chunk->constants.values[constant]);
            for (int j = 0; j < function->upvalueCount; j++) {
                int isLocal = chunk->code[offset++];
                int index = chunk->code[offset++];
                printf("%04d       |                    %s %d\n",
                       offset - 2, isLocal ? "local" : "upvalue", index);
            }

            return offset;
        }
        case OP_CLOSE_UPVALUE:
            return simpleInstruction("OP_CLOSE_UPVALUE", offset);
        case OP_YIELD:
            return simpleInstruction("OP_YIELD", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_CLASS:
            return constantInstruction("OP_CLASS", chunk, offset);
        case OP_INHERIT:
            return simpleInstruction("OP_INHERIT", offset);
        case OP_METHOD:
            return constantInstruction("OP_METHOD", chunk, offset);
        case OP_ELEMENT:
            return simpleInstruction("OP_ELEMENT", offset);
        case OP_SET_ELEMENT:
            return simpleInstruction("OP_SET_ELEMENT", offset);
        case OP_IMMEDIATE_N8:
            return byteInstruction("OP_IMMEDIATE_N8", chunk, offset);
        case OP_IMMEDIATE_P8:
            return byteInstruction("OP_IMMEDIATE_P8", chunk, offset);
        case OP_IMMEDIATE_N16:
            return twoByteInstruction("OP_IMMEDIATE_N16", chunk, offset);
        case OP_IMMEDIATE_P16:
            return twoByteInstruction("OP_IMMEDIATE_P16", chunk, offset);
        case OP_IMMEDIATE_N24:
            return threeByteInstruction("OP_IMMEDIATE_N24", chunk, offset);
        case OP_IMMEDIATE_P24:
            return threeByteInstruction("OP_IMMEDIATE_P24", chunk, offset);
        case OP_TYPE_LITERAL:
            return typeLiteralInstruction("OP_TYPE_LITERAL", chunk, offset);
        case OP_TYPE_MODIFIER:
            return typeLiteralInstruction("OP_TYPE_MODIFIER", chunk, offset);
        case OP_TYPE_STRUCT:
            return byteInstruction("OP_TYPE_STRUCT", chunk, offset);
        case OP_TYPE_ARRAY:
            return simpleInstruction("OP_TYPE_ARRAY", offset);
        case OP_SET_CELL_TYPE:
            return simpleInstruction("OP_SET_CELL_TYPE", offset);
        case OP_DEREF_PTR:
            return simpleInstruction("OP_DEREF_PTR", offset);
        case OP_SET_PTR_TARGET:
            return simpleInstruction("OP_SET_PTR_TARGET", offset);
        case OP_PLACE:
            return simpleInstruction("OP_PLACE", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

void xprintValueStack(ObjRoutine* routine, const char* message) {
    size_t stackSize = routine->stackTopIndex;
    printf("%6s", message);
    printf("%3zu:", stackSize);
    for (int i = (int)(stackSize - 1); i >= 0; i--) {
        ValueCell* slot = peekCell(routine, i);
        printf("[ ");
        printValue(slot->value);
        printf(" | ");
        printType(stdout, slot->cellType);
        printf(" ]");
    }
    printf("\n");
}

void fileExtend(void *f, int n, size_t i)
{
    CodeFile *file = (CodeFile *) f;

    if (n > file->extent_ - file->n_) {
        file->extent_ += n * 2;
        file->i_ = realloc(file->i_, file->extent_ * i);
    }
}
