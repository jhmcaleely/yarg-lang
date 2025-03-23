#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform_hal.h"

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"
#include "files.h"

const char* defaultScript = "main.ya";

static void repl() {
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

static void runFile(const char* path) {
    char* source = readFile(path);
    if (source) {
        InterpretResult result = interpret(source);
        free(source);

        if (result == INTERPRET_COMPILE_ERROR) exit(65);
        if (result == INTERPRET_RUNTIME_ERROR) exit(70);
    }
}

#ifdef CLOX_PICO_TARGET
int main() {
    plaform_hal_init();

    initVM();

    runFile(defaultScript);
    repl();

    freeVM();
    return 0;
}
#else
int main(int argc, const char* argv[]) {
    plaform_hal_init();
    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: cyarg [path]\n");
        exit(64);
    }

    freeVM();
    return 0;
}
#endif