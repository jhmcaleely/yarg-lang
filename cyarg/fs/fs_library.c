#include "fs_library.h"
#include <assert.h>

extern unsigned char cyarg_test_ylib[];
extern unsigned int cyarg_test_ylib_len;

struct RomHeader {
    uint32_t version;
    uint32_t length;
    uint32_t nodeZeroOffset;
};

typedef uint8_t RomNode;

struct RomHeader *const romHeader = (struct RomHeader*)&cyarg_test_ylib[0];

RomNode* nodeZero() {
    return (RomNode*)((const uintptr_t)romHeader + romHeader->nodeZeroOffset);
}

struct nodeIndex {
    uint32_t offset;
    uint32_t length;
};

struct nodeIndex* nodeIndexForNode(uint16_t nodeIndex) {
    uint8_t* indexBase = (uint8_t*)nodeZero();
    struct nodeIndex* index = (struct nodeIndex*)indexBase;
    return &index[nodeIndex];
}

size_t nodeSize(uint16_t nodeIndex) {
    struct nodeIndex* index = nodeIndexForNode(nodeIndex);
    return index->length;
}

uint16_t nodeCount() {
    size_t length = nodeSize(0);
    return (uint16_t)(length / sizeof(struct nodeIndex));
}

RomNode* indexedRomNode(uint16_t nodeIndex) {
    struct nodeIndex* index = nodeIndexForNode(nodeIndex);
    return (RomNode*)((const uintptr_t)romHeader + index->offset);
}

struct directoryEntry {
    uint16_t fileNode;
    uint16_t nameNode;
};

struct directoryEntry* directoryEntryRoot() {
    RomNode* indexNode = indexedRomNode(1);
    return (struct directoryEntry*)indexNode;
}

size_t directoryEntryCount() {
    struct nodeIndex* index = nodeIndexForNode(1);
    return index->length / sizeof(struct directoryEntry);
}

size_t romOffsetForFile(const char* filename) {
    size_t entries = directoryEntryCount();
    for (size_t i = 0; i < entries; i++) {
        struct directoryEntry* entry = &directoryEntryRoot()[i];
        RomNode* nameNode = indexedRomNode(entry->nameNode);
        char* name = (char*)nameNode;
        if (strcmp(name, filename) == 0) {
            struct nodeIndex* fileIndex = nodeIndexForNode(entry->fileNode);
            return fileIndex->offset;
        }
    }

    return 0;
}
size_t romFileSize(const char* filename) {

    size_t entries = directoryEntryCount();
    for (size_t i = 0; i < entries; i++) {
        struct directoryEntry* entry = &directoryEntryRoot()[i];
        RomNode* nameNode = indexedRomNode(entry->nameNode);
        char* name = (char*)nameNode;
        if (strcmp(name, filename) == 0) {
            struct nodeIndex* fileIndex = nodeIndexForNode(entry->fileNode);
            return fileIndex->length;
        }
    }

    return 0;
}


PackedValue romOffsetAsPackedValue(size_t romOffset) {
    PackedValue pv;
    pv.storedValue = NULL;
    pv.storedType = NULL;
    return pv;
}

void ROMInvariantChecks() {
    assert(romHeader->version == 1);
    assert(romHeader->length == cyarg_test_ylib_len);

    size_t length = 0;
    for (uint16_t i = 0; i < nodeCount(); i++) {
        struct nodeIndex* index = nodeIndexForNode(i);
        length += index->length;
    }
    assert(length <= romHeader->length);

    struct directoryEntry* dirEntries = directoryEntryRoot();
    size_t dirEntryCount = directoryEntryCount();
    printf("Directory Entry Count: %zu\n", dirEntryCount);
    for (uint16_t i = 0; i < dirEntryCount; i++) {
        struct directoryEntry* entry = &dirEntries[i];
        RomNode* nameNode = indexedRomNode(entry->nameNode);
        char* name = (char*)nameNode;
        printf("Directory Entry %s, data %d\n", name, entry->fileNode);
    }
}

unsigned char* romBaseAddress() {
    ROMInvariantChecks();

    return &cyarg_test_ylib[0];
}



void romDataForIndex(uint32_t romFileIndex, uint8_t** data, size_t* size) {
    ROMInvariantChecks();

    struct nodeIndex* fileIndex = nodeIndexForNode(romFileIndex);
    *data = (uint8_t*)((const uintptr_t)romHeader + fileIndex->offset);
    *size = fileIndex->length;

}