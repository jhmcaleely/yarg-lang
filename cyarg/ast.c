#include <stdlib.h>

#include "ast.h"

ObjExpressionStatement* newExpressionStatement(ObjExpression* expr) {
    ObjExpressionStatement* stmt = ALLOCATE_OBJ(ObjExpressionStatement, OBJ_EXPRESSIONSTMT);
    stmt->expression = expr;

    return stmt;
}