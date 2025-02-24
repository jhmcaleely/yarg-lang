#include <stdio.h>
#ifdef LOX_PICO_SDK
    #include "pico/stdlib.h"
#endif
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

#include "../hostproto/littlefs/lfs.h"
#include "pico_lfs_hal.h"
#include "pico_flash_fs.h"

const char* defaultScript = "coffee.lox";
#define USE_SCRIPT

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

static struct lfs_config cfg = {
    .read = pico_read_flash_block,
    .prog  = pico_prog_flash_block,
    .erase = pico_erase_flash_block,
    .sync  = pico_sync_flash_block,
    
    // block device configuration

    // device is memory mapped for reading, so reading can be per byte
    .read_size = 1,
    
    .prog_size = PICO_PROG_PAGE_SIZE,
    .block_size = PICO_ERASE_PAGE_SIZE,

    // the number of blocks we use for a flash fs.
    .block_count = FLASHFS_BLOCK_COUNT,

    // cache needs to be a multiple of the programming page size.
    .cache_size = PICO_PROG_PAGE_SIZE * 1,

    .lookahead_size = 16,
    .block_cycles = 500,
};

static void lsDir(const char* path) {
    lfs_t lfs;
    lfs_dir_t dir;
    memset(&lfs, 0, sizeof(lfs));
    memset(&dir, 0, sizeof(dir));
    
    // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);
    if (err < 0) {
        fprintf(stderr, "Could not mount filesystem (%d).\n", err);
        exit(74);
    }

    err = lfs_dir_open(&lfs, &dir, path);
    if (err < 0) {
        fprintf(stderr, "Could not open dir \"%s\" (%d).\n", path, err);
        exit(74);
    }
    struct lfs_info info;
    memset(&info, 0, sizeof(info));

    while ((err = lfs_dir_read(&lfs, &dir, &info)) > 0) {

        if (info.type == LFS_TYPE_DIR) {
            printf("'%s' (dir)\n", info.name);
        }
        else if (info.type == LFS_TYPE_REG) {
            printf("'%s' (%d)\n", info.name, info.size);
        }
        memset(&info, 0, sizeof(info));
    }
    if (err < 0) {
        fprintf(stderr, "Could not read dir \"%s\" (%d).\n", path, err);
        exit(74);
    }

    lfs_dir_close(&lfs, &dir);
    lfs_unmount(&lfs);

}

static char* readFile(const char* path) {

    lfs_t lfs;
    lfs_file_t file;
    memset(&lfs, 0, sizeof(lfs));
    memset(&file, 0, sizeof(file));
    
    // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);
    if (err < 0) {
        fprintf(stderr, "Could not mount filesystem (%d).\n", err);
        exit(74);
    }
    struct lfs_info info;
    memset(&info, 0, sizeof(info));

    err = lfs_stat(&lfs, path, &info);
    if (err < 0) {
        fprintf(stderr, "Could not stat file \"%s\" (%d).\n", path, err);
        exit(74);
    }
    int fileSize = info.size;

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    err = lfs_file_open(&lfs, &file, path, LFS_O_RDONLY);
    if (err < 0) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    int bytesRead = lfs_file_read(&lfs, &file, buffer, fileSize);
    if (bytesRead < fileSize) {
        fprintf(stderr, "could not read file \"%s\".\n", path);
        exit(74);

    }
    buffer[bytesRead] = '\0';
    
    lfs_file_close(&lfs, &file);    
    lfs_unmount(&lfs);

    return buffer;
}

static void runFile(const char* path) {
    char* source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

#ifdef HOST_REPL
int main() {
#ifdef LOX_PICO_SDK
    stdio_init_all();
#endif
    initVM();

#ifdef USE_SCRIPT
    lsDir("/");
    runFile(defaultScript);
#else
    repl();
#endif
    freeVM();
    return 0;
}
#else
int main(int argc, const char* argv[]) {
    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    freeVM();
    return 0;
}
#endif