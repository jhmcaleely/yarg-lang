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
    Obj obj;
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
    EXPR_OP_BIT_OR,
    EXPR_OP_BIT_AND,
    EXPR_OP_BIT_XOR,
    EXPR_OP_MODULO,
    EXPR_OP_NOT_EQUAL,
    EXPR_OP_GREATER_EQUAL,
    EXPR_OP_LESS_EQUAL,
    EXPR_OP_NOT,
    EXPR_OP_NEGATE,
    EXPR_OP_LOGICAL_AND
} ExprOp;

typedef enum {
    EXPR_LITERAL_TRUE,
    EXPR_LITERAL_FALSE,
    EXPR_LITERAL_NIL
} ExprLiteral;

typedef struct {
    ObjExpr expr;
    ExprOp operation;
    ObjExpr* rhs;
} ObjExprOperation;

typedef struct {
    ObjExpr expr;
    ObjExpr* expression;
} ObjExprGrouping;

typedef struct {
    ObjExpr expr;
    ObjString* name;
    ObjExpr* assignment;
} ObjExprNamedVariable;

typedef struct {
    ObjExpr expr;
    ExprLiteral literal;
} ObjExprLiteral;

typedef struct  {
    ObjStmt stmt;
    ObjExpr* expression;
} ObjStmtExpression;

typedef struct {
    ObjStmt stmt;
    ObjExpr* expression;
} ObjStmtPrint;

typedef struct {
    ObjStmt stmt;
    ObjString* name;
    ObjExpr* initialiser;
} ObjStmtVarDeclaration;

typedef enum {
    NUMBER_DOUBLE,
    NUMBER_INTEGER,
    NUMBER_UINTEGER32
} NumberType;

typedef struct {
    ObjExpr expr;
    NumberType type;
    union {
        double dbl;
        int integer;
        uint32_t uinteger32;
    } val;
} ObjExprNumber;

ObjExprNumber* newExprNumberDouble(double value);
ObjExprNumber* newExprNumberInteger(int value);
ObjExprNumber* newExprNumberUInteger32(uint32_t value);
ObjExprOperation* newExprOperation(ObjExpr* rhs, ExprOp op);
ObjExprGrouping* newExprGrouping(ObjExpr* expression);
ObjExprNamedVariable* newExprNamedVariable(const char* name, int nameLength, ObjExpr* expr);
ObjExprLiteral* newExprLiteral(ExprLiteral literal);

ObjStmtExpression* newStmtExpression(ObjExpr* expr);
ObjStmtPrint* newStmtPrint(ObjExpr* expr);
ObjStmtVarDeclaration* newStmtVarDeclaration(char* name, int nameLength, ObjExpr* expr);

void printStmts(ObjStmt* stmts);
void printExpr(ObjExpr* expr);

#endif