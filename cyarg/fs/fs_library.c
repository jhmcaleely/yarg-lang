#include "fs_library.h"
#include <assert.h>

// interim hosting of the XIP library in .rodata.
#ifdef CYARG_SELF_HOSTED
extern unsigned const char cyarg_ylib[];
extern unsigned int cyarg_ylib_len;
#else
extern unsigned const char cyarg_hosted_ylib[];
extern unsigned int cyarg_hosted_ylib_len;
#endif

// the library is linearised a set of 'nodes', all concatenated in memory.
// the tool creating the library will pad nodes as needed for alignment.
// the first node (0) contains an index offset and length of all nodes, including itself.

struct XIPLibHeader {
    uint32_t version;
    uint32_t length;
    uint32_t nodeZeroOffset;
};

#ifdef CYARG_SELF_HOSTED
const struct XIPLibHeader *const xipLibHeader = (const struct XIPLibHeader*)&cyarg_ylib[0];
const uint8_t* const xipLibraryBytes = &cyarg_ylib[0];
#else
const struct XIPLibHeader *const xipLibHeader = (const struct XIPLibHeader*)&cyarg_hosted_ylib[0];
const uint8_t* const xipLibraryBytes = &cyarg_hosted_ylib[0];
#endif

struct nodeIndex {
    uint32_t offset;
    uint32_t length;
};

const struct nodeIndex* nodeIndex(uint16_t node) {
    const uint8_t* const nodeZero = &xipLibraryBytes[xipLibHeader->nodeZeroOffset];
    const struct nodeIndex* index = (const struct nodeIndex*)nodeZero;
    assert(index[0].offset == xipLibHeader->nodeZeroOffset);
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
    return &xipLibraryBytes[index->offset];
}

const uint16_t bootstrap_node = 1;
const uint16_t root_directory_node = 2;

struct directoryEntry {
    uint16_t fileNode;
    uint16_t nameNode;
};

const struct directoryEntry* directoryEntryRoot() {
    const uint8_t* indexNode = nodeData(root_directory_node);
    return (const struct directoryEntry*)indexNode;
}

size_t directoryEntryCount() {
    const struct nodeIndex* index = nodeIndex(root_directory_node);
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

void xipLibraryInvariant() {
    assert(xipLibHeader->version == 1);
#ifdef CYARG_SELF_HOSTED
    assert(xipLibHeader->length == cyarg_ylib_len);
#else
    assert(xipLibHeader->length == cyarg_hosted_ylib_len);
#endif

#if 0
    size_t length = 0;
    uint16_t count = nodeCount();
    printf("Node Count: %u\n", count);
    for (uint16_t i = 0; i < nodeCount(); i++) {
        const struct nodeIndex* index = nodeIndex(i);
        length += index->length;
        printf("Node %u: offset %d, length %d\n", i, index->offset, index->length);
    }
    assert(length <= xipLibHeader->length);

    const struct directoryEntry* dirEntries = directoryEntryRoot();
    size_t dirEntryCount = directoryEntryCount();
    printf("Directory Entry Count: %zu\n", dirEntryCount);
    for (uint16_t i = 0; i < dirEntryCount; i++) {
        const struct directoryEntry* entry = &dirEntries[i];
        const uint8_t* nameNode = nodeData(entry->nameNode);
        const char* name = (const char*)nameNode;
        printf("Directory Entry %s, data %d\n", name, entry->fileNode);
    }
#endif
}

bool xipLibraryReadFilename(const char* filename, const uint8_t** data, size_t* size) {
    xipLibraryInvariant();

    const struct directoryEntry* entry = directoryEntryForFile(filename);
    if (!entry) {
        return false;
    }
    *data = nodeData(entry->fileNode);
    *size = nodeSize(entry->fileNode);
    return true;
}

bool xipLibraryReadNode(uint16_t node, const uint8_t** data, size_t* size) {
    xipLibraryInvariant();
    if (node >= nodeCount()) {
        return false;
    } else {
        *data = nodeData(node);
        *size = nodeSize(node);
        return true;
    }
}