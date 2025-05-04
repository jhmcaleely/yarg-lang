#ifndef cyarg_ast_h
#define cyarg_ast_h

#include "object.h"

typedef struct {
    Obj obj;
} ObjExpression;

typedef struct ObjExpressionStatement ObjExpressionStatement;

typedef struct ObjExpressionStatement {
    Obj obj;
    ObjExpressionStatement* next;
    ObjExpression* expression;
} ObjExpressionStatement;


ObjExpression* newExpression();
ObjExpressionStatement* newExpressionStatement(ObjExpression* expr);


#endif