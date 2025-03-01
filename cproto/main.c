#include <stdio.h>
#include "pico/stdlib.h"
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

#include "../hostproto/littlefs/lfs.h"
#include "pico_lfs_hal.h"
#include "pico_flash_fs.h"

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
        return NULL;
    }
    struct lfs_info info;
    memset(&info, 0, sizeof(info));

    err = lfs_stat(&lfs, path, &info);
    if (err < 0) {
        return NULL;
    }
    int fileSize = info.size;

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        return NULL;
    }

    err = lfs_file_open(&lfs, &file, path, LFS_O_RDONLY);
    if (err < 0) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        return NULL;
    }

    int bytesRead = lfs_file_read(&lfs, &file, buffer, fileSize);
    if (bytesRead < fileSize) {
        fprintf(stderr, "could not read file \"%s\".\n", path);
        return NULL;
    }
    buffer[bytesRead] = '\0';
    
    lfs_file_close(&lfs, &file);    
    lfs_unmount(&lfs);

    return buffer;
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
