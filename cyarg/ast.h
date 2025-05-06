#ifndef cyarg_ast_h
#define cyarg_ast_h

#include "object.h"

typedef struct ObjStmt ObjStmt;

typedef struct ObjStmt {
    Obj obj;
    ObjStmt* nextStmt;
} ObjStmt;

typedef struct ObjExpression ObjExpression;

typedef struct ObjExpression {
    Obj obj;
    ObjExpression* nextItem;
    Obj* expr;
} ObjExpression;

typedef struct ObjExpressionStatement ObjExpressionStatement;

typedef struct ObjExpressionStatement {
    ObjStmt stmt;
    ObjExpression* expression;
} ObjExpressionStatement;

typedef struct ObjPrintStatement {
    ObjStmt stmt;
    ObjExpression* expression;
} ObjPrintStatement;


typedef enum {
    NUMBER_DOUBLE,
    NUMBER_INTEGER,
    NUMBER_UINTEGER32
} NumberType;


typedef struct ObjNumber {
    Obj obj;
    NumberType type;
    union {
        double dbl;
        int integer;
        uint32_t uinteger32;
    } val;
} ObjNumber;

ObjNumber* newNumberDouble(double value);
ObjNumber* newNumberInteger(int value);
ObjNumber* newNumberUInteger32(uint32_t value);
ObjExpression* newExpression(Obj* element);
ObjExpressionStatement* newExpressionStatement(ObjExpression* expr);
ObjPrintStatement* newPrintStatement(ObjExpression* expr);

#endif