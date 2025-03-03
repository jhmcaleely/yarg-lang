#include <stdio.h>
#include "pico/stdlib.h"
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"
#include "files.h"

const char* defaultScript = "main.lox";

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

int main() {
    stdio_init_all();

    initVM();

    runFile(defaultScript);
    repl();

    freeVM();
    return 0;
}
