#ifndef cyarg_fs_library_h
#define cyarg_fs_library_h

#include <stdint.h>
#include <stddef.h>
#include "../value.h"



bool romReadFilename(const char* filename, const uint8_t** data, size_t* size);
bool romReadNode(uint16_t node, const uint8_t** data, size_t* size);

#endif


