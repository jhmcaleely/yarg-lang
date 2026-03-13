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

static void repl() {
    char line[4096];
    printf("> ");
    for (;;) {
        if (fgets(line, sizeof(line), stdin) != 0) {
            interpret(line);
            printf("> ");
        }
        else {
            int feofi = feof(stdin); // fgets returns null when pause/continue in lldb
            if (feofi) { // only exit if EOF
                break;
            }
        }
    }
}

#ifdef CYARG_FEATURE_HOSTED_REPL
static int runFile(const char* path) {
    char* source = readFile(path);
    if (source) {
        InterpretResult result = interpret(source);
        free(source);

        if (result == INTERPRET_COMPILE_ERROR) {
            return EX_DATAERR;
        } else if (result == INTERPRET_RUNTIME_ERROR) {
            return EX_SOFTWARE;
        } else {
            return EX_OK;
        }
    } else {
        return EX_NOINPUT;
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
        interpret(source);
        free(source);
    }
    repl();

    freeVM();
    return 0;
}
#elif defined(CYARG_FEATURE_HOSTED_REPL)
void usageMessage(FILE* destination) {
    FPRINTMSG(destination, "Usage:\n");
    FPRINTMSG(destination, "\tcyarg <path>\n");
    FPRINTMSG(destination, "\tcyarg --help\n");
    FPRINTMSG(destination, "\tcyarg --compile <path>\n");
    FPRINTMSG(destination, "\tcyarg --disassemble <path>\n");
}

int main(int argc, const char* argv[]) {
    plaform_hal_init();
    initVM();

    int returnCode = EX_OK;

    if (argc == 1) {
        repl();
    } else if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        usageMessage(stdout);
    } else if (argc == 2) {
        returnCode = runFile(argv[1]);
    } else if (argc == 3 && strcmp(argv[1], "--compile") == 0) {
        returnCode = compileFile(argv[2]);
    } else if (argc == 3 && strcmp(argv[1], "--disassemble") == 0) {
        returnCode = disassembleFile(argv[2]);
    }
    else {
        usageMessage(stderr);
        returnCode = EX_USAGE;
    }

    freeVM();
    return returnCode;
}
#endif
