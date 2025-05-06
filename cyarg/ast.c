#include <stdlib.h>

#include "ast.h"

ObjExpressionStatement* newExpressionStatement(ObjExpression* expr) {
    ObjExpressionStatement* stmt = ALLOCATE_OBJ(ObjExpressionStatement, OBJ_EXPRESSIONSTMT);
    stmt->next = NULL;
    stmt->expression = expr;

    return stmt;
}

ObjExpression* newExpression(Obj* element) {
    ObjExpression* expr = ALLOCATE_OBJ(ObjExpression, OBJ_EXPRESSION);
    expr->nextItem = NULL;
    expr->expr = element;
    return expr;
}

ObjNumber* newNumberDouble(double value) {
    ObjNumber* num = ALLOCATE_OBJ(ObjNumber, OBJ_NUMBER);
    num->type = NUMBER_DOUBLE;
    num->val.dbl = value;
    return num;
}

ObjNumber* newNumberInteger(int value) {
    ObjNumber* num = ALLOCATE_OBJ(ObjNumber, OBJ_NUMBER);
    num->type = NUMBER_INTEGER;
    num->val.integer = value;
    return num;
}

ObjNumber* newNumberUInteger32(uint32_t value) {
    ObjNumber* num = ALLOCATE_OBJ(ObjNumber, OBJ_NUMBER);
    num->type = NUMBER_UINTEGER32;
    num->val.uinteger32 = value;
    return num;
}