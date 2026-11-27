#include "common.h"
#include "vm.h"
#include "hosted.h"

#include <stdlib.h>
#include <sysexits.h>
#include <string.h>

void usageMessage(FILE* destination) {
    fputs("Usage:\n"
          "\tyargfmt\n"
         , destination);
}

const char* getArgument(int argc, const char* argv[], const char* name) {
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], name) == 0) {
            return argv[i + 1];
        }
    }
    return NULL;
}

int formatSource(FILE* input, FILE* output);

int main(int argc, const char* argv[]) {
    if (argv[1] && strcmp(argv[1], "--help") == 0 && argc == 2) {
        usageMessage(stdout);
        return EX_OK;
    }

    initVMMemory();
    initVMRuntime();
    vmHost.argc = argc;
    vmHost.argv = argv;
    vmHost.exitCode = EX_OK;
    
    int returnCode = EX_OK;
    FILE* input = stdin;
    FILE* output = stdout;
    if (argv[1]) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            perror("Failed to open input file");
            return EX_NOINPUT;
        }
    }
    if (argv[2]) {
        output = fopen(argv[2], "wx");
        if (output == NULL) {
            perror("Failed to open output file");
            return EX_CANTCREAT;
        }
    }

    returnCode = formatSource(input, output);
    if (input != stdin) {
        fclose(input);
    }
    if (output != stdout) {
        fclose(output);
    }

    freeVM();
    return returnCode;
}

