#ifndef cyarg_fs_library_h
#define cyarg_fs_library_h

#include <stdint.h>
#include <stddef.h>
#include "../value.h"

size_t romOffsetForFile(const char* filename);
size_t romFileSize(const char* filename);
PackedValue romOffsetAsPackedValue(size_t romOffset);
unsigned char* romBaseAddress();

void romDataForIndex(uint32_t romFileIndex, uint8_t** data, size_t* size);

#endif


