#include <stdlib.h>

#include "ast.h"

ObjExpressionStatement* newExpressionStatement(ObjExpr* expr) {
    ObjExpressionStatement* stmt = ALLOCATE_OBJ(ObjExpressionStatement, OBJ_STMT_EXPRESSION);
    stmt->stmt.nextStmt = NULL;
    stmt->expression = expr;

    return stmt;
}

ObjPrintStatement* newPrintStatement(ObjExpr* expr) {
    ObjPrintStatement* stmt = ALLOCATE_OBJ(ObjPrintStatement, OBJ_STMT_PRINT);
    stmt->stmt.nextStmt = NULL;
    stmt->expression = expr;

    return stmt;
}


ObjOperationExpr* newOperationExpr(ObjExpr* rhs, ExprOp op) {
    ObjOperationExpr* operation = ALLOCATE_OBJ(ObjOperationExpr, OBJ_EXPR_OPERATION);
    operation->expr.nextExpr = NULL;
    operation->rhs = rhs;
    operation->operation = op;

    return operation;
}

ObjGroupingExpr* newGroupingExpr(ObjExpr* expression) {
    ObjGroupingExpr* grp = ALLOCATE_OBJ(ObjGroupingExpr, OBJ_EXPR_GROUPING);
    grp->expr.nextExpr = NULL;
    grp->expression = expression;
    return grp;
}


ObjNumber* newNumberDouble(double value) {
    ObjNumber* num = ALLOCATE_OBJ(ObjNumber, OBJ_EXPR_NUMBER);
    num->expr.nextExpr = NULL;
    num->type = NUMBER_DOUBLE;
    num->val.dbl = value;
    return num;
}

ObjNumber* newNumberInteger(int value) {
    ObjNumber* num = ALLOCATE_OBJ(ObjNumber, OBJ_EXPR_NUMBER);
    num->expr.nextExpr = NULL;
    num->type = NUMBER_INTEGER;
    num->val.integer = value;
    return num;
}

ObjNumber* newNumberUInteger32(uint32_t value) {
    ObjNumber* num = ALLOCATE_OBJ(ObjNumber, OBJ_EXPR_NUMBER);
    num->expr.nextExpr = NULL;
    num->type = NUMBER_UINTEGER32;
    num->val.uinteger32 = value;
    return num;
}