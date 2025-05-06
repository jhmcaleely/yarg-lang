#ifndef cyarg_ast_h
#define cyarg_ast_h

#include "object.h"

typedef struct ObjStmt ObjStmt;

typedef struct ObjStmt {
    Obj obj;
    ObjStmt* nextStmt;
} ObjStmt;

typedef struct ObjExpr ObjExpr;

typedef struct ObjExpr {
    Obj Obj;
    ObjExpr* nextExpr;
} ObjExpr;

typedef enum {
    EXPR_OP_EQUAL,
    EXPR_OP_GREATER,
    EXPR_OP_RIGHT_SHIFT,
    EXPR_OP_LESS,
    EXPR_OP_LEFT_SHIFT,
    EXPR_OP_ADD,
    EXPR_OP_SUBTRACT,
    EXPR_OP_MULTIPLY,
    EXPR_OP_DIVIDE,
    EXPR_OP_BITOR,
    EXPR_OP_BITAND,
    EXPR_OP_BITXOR,
    EXPR_OP_MODULO,
    EXPR_OP_NOT_EQUAL,
    EXPR_OP_GREATER_EQUAL,
    EXPR_OP_LESS_EQUAL
} ExprOp;

typedef struct {
    ObjExpr expr;
    ExprOp operation;
    ObjExpr* rhs;
} ObjBinaryExpr;

typedef struct ObjExpressionStatement ObjExpressionStatement;

typedef struct ObjExpressionStatement {
    ObjStmt stmt;
    ObjExpr* expression;
} ObjExpressionStatement;

typedef struct ObjPrintStatement {
    ObjStmt stmt;
    ObjExpr* expression;
} ObjPrintStatement;


typedef enum {
    NUMBER_DOUBLE,
    NUMBER_INTEGER,
    NUMBER_UINTEGER32
} NumberType;


typedef struct ObjNumber {
    ObjExpr expr;
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
ObjBinaryExpr* newBinaryExpr(ObjExpr* rhs, ExprOp op);
ObjExpressionStatement* newExpressionStatement(ObjExpr* expr);
ObjPrintStatement* newPrintStatement(ObjExpr* expr);

#endif