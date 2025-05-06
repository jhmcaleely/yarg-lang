#include <stdlib.h>

#include "ast.h"

ObjExpressionStatement* newExpressionStatement(ObjExpr* expr) {
    ObjExpressionStatement* stmt = ALLOCATE_OBJ(ObjExpressionStatement, OBJ_EXPRESSIONSTMT);
    stmt->stmt.nextStmt = NULL;
    stmt->expression = expr;

    return stmt;
}

ObjPrintStatement* newPrintStatement(ObjExpr* expr) {
    ObjPrintStatement* stmt = ALLOCATE_OBJ(ObjPrintStatement, OBJ_PRINTSTMT);
    stmt->stmt.nextStmt = NULL;
    stmt->expression = expr;

    return stmt;
}


ObjBinaryExpr* newBinaryExpr(ObjExpr* rhs, ExprOp op) {
    ObjBinaryExpr* bin = ALLOCATE_OBJ(ObjBinaryExpr, OBJ_BINARYEXPR);
    bin->expr.nextExpr = NULL;
    bin->rhs = rhs;
    bin->operation = op;

    return bin;
}


ObjNumber* newNumberDouble(double value) {
    ObjNumber* num = ALLOCATE_OBJ(ObjNumber, OBJ_NUMBER);
    num->expr.nextExpr = NULL;
    num->type = NUMBER_DOUBLE;
    num->val.dbl = value;
    return num;
}

ObjNumber* newNumberInteger(int value) {
    ObjNumber* num = ALLOCATE_OBJ(ObjNumber, OBJ_NUMBER);
    num->expr.nextExpr = NULL;
    num->type = NUMBER_INTEGER;
    num->val.integer = value;
    return num;
}

ObjNumber* newNumberUInteger32(uint32_t value) {
    ObjNumber* num = ALLOCATE_OBJ(ObjNumber, OBJ_NUMBER);
    num->expr.nextExpr = NULL;
    num->type = NUMBER_UINTEGER32;
    num->val.uinteger32 = value;
    return num;
}