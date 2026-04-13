#ifndef cyarg_pack_h
#define cyarg_pack_h

#include <stdbool.h>

struct Chunk;
struct ObjFunction;

int packScript(char const *sourceFileName, struct Chunk const *chunk, bool includeLines, char const *path);
struct ObjFunction *loadPackage(char const *path);

#endif
