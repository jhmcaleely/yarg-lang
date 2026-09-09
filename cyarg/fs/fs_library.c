#include "fs_library.h"
#include <assert.h>

extern unsigned const char cyarg_test_ylib[];
extern unsigned int cyarg_test_ylib_len;

struct RomHeader {
    uint32_t version;
    uint32_t length;
    uint32_t nodeZeroOffset;
};


const struct RomHeader *const romHeader = (struct RomHeader*)&cyarg_test_ylib[0];
const uint8_t* const romData = &cyarg_test_ylib[0];

const uint8_t* const nodeZero() {
    const uint8_t* const node = (const uint8_t*)((const uintptr_t)romHeader + romHeader->nodeZeroOffset);
    const uint8_t* const nodeAlt = &romData[romHeader->nodeZeroOffset];

    assert(node == nodeAlt);
    return node;
}

struct nodeIndex {
    uint32_t offset;
    uint32_t length;
};

const struct nodeIndex* nodeIndex(uint16_t node) {
    const struct nodeIndex* index = (const struct nodeIndex*)nodeZero();
    return &index[node];
}

size_t nodeSize(uint16_t node) {
    const struct nodeIndex* index = nodeIndex(node);
    return index->length;
}

uint16_t nodeCount() {
    size_t length = nodeSize(0);
    return (uint16_t)(length / sizeof(struct nodeIndex));
}

const uint8_t* nodeData(uint16_t node) {
    const struct nodeIndex* index = nodeIndex(node);
    return &romData[index->offset];
}

struct directoryEntry {
    uint16_t fileNode;
    uint16_t nameNode;
};

const struct directoryEntry* directoryEntryRoot() {
    const uint8_t* indexNode = nodeData(2);
    return (const struct directoryEntry*)indexNode;
}

size_t directoryEntryCount() {
    const struct nodeIndex* index = nodeIndex(2);
    return index->length / sizeof(struct directoryEntry);
}

const struct directoryEntry* directoryEntryForFile(const char* filename) {
    size_t entries = directoryEntryCount();
    for (size_t i = 0; i < entries; i++) {
        const struct directoryEntry* entry = &directoryEntryRoot()[i];
        const uint8_t* nameNode = nodeData(entry->nameNode);
        const char* name = (const char*)nameNode;
        if (strcmp(name, filename) == 0) {
            return entry;
        }
    }

    return NULL;
}

size_t romOffsetForFile(const char* filename) {

    const struct directoryEntry* entry = directoryEntryForFile(filename);
    if (!entry) {
        return 0;
    } else {
        const struct nodeIndex* fileIndex = nodeIndex(entry->fileNode);
        return fileIndex->offset;
    }
}

size_t romFileSize(const char* filename) {

    const struct directoryEntry* entry = directoryEntryForFile(filename);
    if (!entry) {
        return 0;
    } else {
        const struct nodeIndex* fileIndex = nodeIndex(entry->fileNode);
        return fileIndex->length;
    }
}

void ROMInvariantChecks() {
    assert(romHeader->version == 1);
    assert(romHeader->length == cyarg_test_ylib_len);

    size_t length = 0;
    uint16_t count = nodeCount();
    printf("Node Count: %u\n", count);
    for (uint16_t i = 0; i < nodeCount(); i++) {
        const struct nodeIndex* index = nodeIndex(i);
        length += index->length;
        printf("Node %u: offset %zu, length %zu\n", i, index->offset, index->length);
    }
    assert(length <= romHeader->length);

    const struct directoryEntry* dirEntries = directoryEntryRoot();
    size_t dirEntryCount = directoryEntryCount();
    printf("Directory Entry Count: %zu\n", dirEntryCount);
    for (uint16_t i = 0; i < dirEntryCount; i++) {
        const struct directoryEntry* entry = &dirEntries[i];
        const uint8_t* nameNode = nodeData(entry->nameNode);
        const char* name = (const char*)nameNode;
        printf("Directory Entry %s, data %d\n", name, entry->fileNode);
    }
}

bool romReadRomFile(const char* filename, const uint8_t** data, size_t* size) {
    ROMInvariantChecks();

    const struct directoryEntry* entry = directoryEntryForFile(filename);
    if (!entry) {
        return false;
    }
    *data = nodeData(entry->fileNode);
    *size = nodeSize(entry->fileNode);
    return true;
}

bool romReadNode(uint16_t node, const uint8_t** data, size_t* size) {
    ROMInvariantChecks();
    if (node >= nodeCount()) {
        return false;
    } else {
        *data = nodeData(node);
        *size = nodeSize(node);
        return true;
    }
}