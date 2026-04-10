#include <stdlib.h>
#include <sysexits.h>
#include <assert.h>

#include "common.h"
#include "hosted.h"
#include "object.h"
#include "memory.h"
#include "debug.h"
#include "pack.h"
#include "vm.h"

Host vmHost;

static char* libraryNameFor(const char* importname, const char* libraryPath) {
    size_t namelen = strlen(importname);
    size_t pathlen = 0;
    if (libraryPath) {
        pathlen = strlen(libraryPath);
    }
    char* filename = malloc(pathlen + 1 + namelen + 1);
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
    }
    return filename;
}

int runFile(const char* libraryPath, const char* path) {

    char* replPath = libraryNameFor(path, libraryPath);
    ObjString* replPathString = copyString(replPath, (int) strlen(replPath));
    tempRootPush(OBJ_VAL(replPathString));
    free(replPath);

    InterpretResult result = bootScript(replPathString->chars, replPathString->length);

    tempRootPop();
    if (result == INTERPRET_COMPILE_ERROR) {
        return EX_DATAERR;
    } else if (result == INTERPRET_RUNTIME_ERROR) {
        return EX_SOFTWARE;
    } else {
        return EX_OK;
    }
}

int compileFile(const char* path, Value* compileResult) {

    ObjString* pathString = copyString(path, (int) strlen(path));
    tempRootPush(OBJ_VAL(pathString));

    InterpretResult result = bootstrapVM(compile_bootstrap, compileResult, pathString);

    tempRootPop();
    if (result == INTERPRET_COMPILE_ERROR) {
        return EX_DATAERR;
    } else if (result == INTERPRET_RUNTIME_ERROR) {
        return EX_SOFTWARE;
    } else {
        return EX_OK;
    }
}

int disassembleFile(const char* path) {

    Value result;
    int returnCode = compileFile(path, &result);
    if (returnCode != EX_OK) {
        return returnCode;
    }
    tempRootPush(result);

    ObjClosure* closure = AS_CLOSURE(result);
    ObjFunction* function = closure->function;

    char const *file = strrchr(path, '/');
    if (file == 0) {
        file = path;
    } else {
        file++;
    }
    disassembleChunk(&function->chunk, file);

    tempRootPop();
    return returnCode;
}

int packageBinary(const char *path, Value const *script)
{
    int r = EX_SOFTWARE;
    if (IS_CLOSURE(*script)) {
        char const *scriptFileName = strrchr(path, '/');
        if (scriptFileName == 0) {
            scriptFileName = path;
        } else {
            scriptFileName++;
        }

        size_t len = strlen(path);
        char *packagePath = malloc(len + 1);
        strcpy(packagePath, path);
        assert(packagePath[len - 1] == 'a');
        packagePath[len - 1] = 'b';

        FILE *file = fopen(packagePath, "wb");
        r = packChunks(scriptFileName, &AS_CLOSURE(*script)->function->chunk, true, file);
        fclose(file);

        if (r != EX_OK)
        {
            (void) remove(packagePath);
        }
        free(packagePath);
    }
    return r;
}
