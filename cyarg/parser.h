#ifndef cyarg_parser_h
#define cyarg_parser_h

#include <stdbool.h>

typedef struct ObjStmt ObjStmt;

bool parse(ObjStmt** ast_root);


#endif