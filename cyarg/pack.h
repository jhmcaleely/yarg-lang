#ifndef cyarg_pack_h
#define cyarg_pack_h

#include "chunk.h"

char *packChunk(Chunk* chunk, bool includeLines, int *len);

#endif
