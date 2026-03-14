#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef CYARG_FEATURE_HOSTED_REPL
#include <sysexits.h>
#endif

#include "common.h"
#include "platform_hal.h"
#include "vm.h"
#include "fs/fs.h"

#ifdef CYARG_FEATURE_HOSTED_REPL
#include "compiler.h"
#include "chunk.h"
#include "debug.h"
#endif

static char* libraryNameFor(const char* importname, const char* libraryPath) {
    size_t namelen = strlen(importname);
    size_t pathlen = 0;
    if (libraryPath) {
        pathlen = strlen(libraryPath);
    }
    char* filename = malloc(pathlen + 1 +namelen + 3 + 1);
    if (filename) {
        if (libraryPath) {
            strcpy(filename, libraryPath);
            if (libraryPath[pathlen - 1] != '/') {
                strcat(filename, "/");
            }
        } else {
            strcpy(filename, "");
        }
        strcat(filename, importname);
        strcat(filename, ".ya");
    }
    return filename;
}

#ifdef CYARG_FEATURE_HOSTED_REPL
static int runFile(const char* libraryPath, const char* path) {

    char* replPath = libraryNameFor(path, libraryPath);
    ObjString* replPathString = copyString(replPath, strlen(replPath));
    tempRootPush(OBJ_VAL(replPathString));

    ObjFunction* replFunction = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    tempRootPush(OBJ_VAL(replFunction));
    replFunction->name = copyString("boot", 4);

    writeChunk(&replFunction->chunk, OP_GET_BUILTIN, 0);
    writeChunk(&replFunction->chunk, BUILTIN_EXEC, 0);
    writeChunk(&replFunction->chunk, OP_GET_BUILTIN, 0);
    writeChunk(&replFunction->chunk, BUILTIN_READ_SOURCE, 0);
    writeChunk(&replFunction->chunk, OP_CONSTANT, 0);
    writeChunk(&replFunction->chunk, addConstant(&replFunction->chunk, OBJ_VAL(replPathString)), 0);
    writeChunk(&replFunction->chunk, OP_CALL, 0);
    writeChunk(&replFunction->chunk, 1, 0);
    writeChunk(&replFunction->chunk, OP_CALL, 0);
    writeChunk(&replFunction->chunk, 1, 0);
    writeChunk(&replFunction->chunk, OP_RETURN, 0);

    InterpretResult result = startup(replFunction);

    tempRootPop();
    tempRootPop();
    free(replPath);

    if (result == INTERPRET_COMPILE_ERROR) {
        return EX_DATAERR;
    } else if (result == INTERPRET_RUNTIME_ERROR) {
        return EX_SOFTWARE;
    } else {
        return EX_OK;
    }
}

static int compileFile(const char* path) {
    char* source = readFile(path);
    if (!source) {
        return EX_NOINPUT;
    }

    ObjFunction* result = compile(source);
    free(source);
    if (!result) {
        return EX_DATAERR;
    }
    return EX_OK;
}

static int disassembleFile(const char* path) {
    char* source = readFile(path);
    if (!source) {
        return EX_NOINPUT;
    }

    ObjFunction* result = compile(source);
    free(source);
    if (!result) {
        return EX_DATAERR;
    }

    disassembleChunk(&result->chunk, path);
    for (int i = 0; i < result->chunk.constants.count; i++) {
        if (IS_FUNCTION(result->chunk.constants.values[i])) {
            ObjFunction* fun = AS_FUNCTION(result->chunk.constants.values[i]);
            disassembleChunk(&fun->chunk, fun->name->chars);
        }
    }
    return EX_OK;
}
#endif

#if defined(CYARG_FEATURE_SELF_HOSTED_REPL)
int main() {
    plaform_hal_init();

    initVM();
    char* source = readFile("main.ya");
    if (source) {
        interpret(NULL, source);
        free(source);
    }
    repl(NULL);

    freeVM();
    return 0;
}
#elif defined(CYARG_FEATURE_HOSTED_REPL)
void usageMessage(FILE* destination) {
    FPRINTMSG(destination, "Usage:\n");
    FPRINTMSG(destination, "\tcyarg [--library <dir>]\n");      // 1 or 3
    FPRINTMSG(destination, "\tcyarg [--library <dir>] <path>\n"); // 2 or 4
    FPRINTMSG(destination, "\tcyarg --help\n"); // 1
    FPRINTMSG(destination, "\tcyarg --compile <path>\n"); // 3
    FPRINTMSG(destination, "\tcyarg --disassemble <path>\n"); // 3
}

const char* getArgument(int argc, const char* argv[], const char* name) {
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], name) == 0) {
            return argv[i + 1];
        }
    }
    return NULL;
}

int main(int argc, const char* argv[]) {
    plaform_hal_init();
    initVM();

    int returnCode = EX_OK;

    if (argc < 1 && argc > 5) {
        usageMessage(stderr);
        return EX_USAGE;
    }

    const char* libraryPath = getArgument(argc, argv, "--library");
    const char* compilePath = getArgument(argc, argv, "--compile");
    const char* disassemblePath = getArgument(argc, argv, "--disassemble");
    if (compilePath && disassemblePath) {
        usageMessage(stderr);
        return EX_USAGE;
    }

    if (argc == 1 || (argc == 3 && libraryPath)) {
        runFile(libraryPath, "repl");
    } else if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        usageMessage(stdout);
    } else if (argc == 2 || (argc == 4 && libraryPath)) {
        int sourcePosition = argc == 2 ? 1 : 3;
        returnCode = runFile(libraryPath, argv[sourcePosition]);
    } else if (argc == 3 && compilePath) {
        returnCode = compileFile(compilePath);
    } else if (argc == 3 && disassemblePath) {
        returnCode = disassembleFile(disassemblePath);
    }
    else {
        usageMessage(stderr);
        returnCode = EX_USAGE;
    }

    freeVM();
    return returnCode;
}
#endif
