#include <stdlib.h>
#include <stdio.h>

#include "ast.h"
#include "memory.h"

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

ObjStmtVarDeclaration* newStmtVarDeclaration(char* name, int nameLength, ObjExpr* expr) {
    ObjStmtVarDeclaration* stmt = ALLOCATE_OBJ(ObjStmtVarDeclaration, OBJ_STMT_VARDECLARATION);
    tempRootPush(OBJ_VAL(stmt));
    stmt->stmt.nextStmt = NULL;
    stmt->name = copyString(name, nameLength);
    stmt->initialiser = expr;
    tempRootPop();
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

static void printExprOperation(ObjOperationExpr* opexpr) {
    switch (opexpr->operation) {
        case EXPR_OP_EQUAL: printf("="); break;
        case EXPR_OP_GREATER: printf(">"); break;
        case EXPR_OP_RIGHT_SHIFT: printf(">>"); break;
        case EXPR_OP_LESS: printf("<"); break;
        case EXPR_OP_LEFT_SHIFT: printf("<<"); break;
        case EXPR_OP_ADD: printf("+"); break;
        case EXPR_OP_SUBTRACT: printf("-"); break;
        case EXPR_OP_MULTIPLY: printf("*"); break;
        case EXPR_OP_DIVIDE: printf("/"); break;
        case EXPR_OP_BITOR: printf("|"); break;
        case EXPR_OP_BITAND: printf("&"); break;
        case EXPR_OP_BITXOR: printf("^"); break;
        case EXPR_OP_MODULO: printf("%%"); break;
        case EXPR_OP_NOT_EQUAL: printf("!="); break;
        case EXPR_OP_GREATER_EQUAL: printf(">="); break;
        case EXPR_OP_LESS_EQUAL: printf("<="); break;
        case EXPR_OP_NOT: printf("!"); break;
        case EXPR_OP_NEGATE: printf("-"); break;
    }
    printf("(");
    printExpr(opexpr->rhs);
    printf(")");
}

void printExpr(ObjExpr* expr) {
    ObjExpr* cursor = expr;
    while (cursor) {
        switch (cursor->obj.type) {
            case OBJ_EXPR_OPERATION:
                printExprOperation((ObjOperationExpr*)cursor);
                break;
            case OBJ_EXPR_GROUPING: {
                ObjGroupingExpr* expr = (ObjGroupingExpr*)cursor;
                printf("(");
                printExpr(expr->expression);
                printf(")");
                break;
            }
            case OBJ_EXPR_NUMBER: {
                ObjNumber* num = (ObjNumber*)cursor;
                switch (num->type) {
                    case NUMBER_DOUBLE:
                        printf("%f", num->val.dbl);
                        break;
                    case NUMBER_INTEGER:
                        printf("%d", num->val.integer);
                        break;
                    case NUMBER_UINTEGER32:
                        printf("u%d", num->val.uinteger32);
                        break;
                }
                break;
            }
            default:
                printf("<unknown>");
        }
        cursor = cursor->nextExpr;
    }
}

void printStmts(ObjStmt* stmts) {
    ObjStmt* cursor = stmts;
    while (cursor) {
        switch (cursor->obj.type) {
            case OBJ_STMT_EXPRESSION:
                printf("(");
                printExpr(((ObjExpressionStatement*)cursor)->expression);
                printf(");\n");
                break;
            case OBJ_STMT_PRINT:
                printf("print ");
                printExpr(((ObjPrintStatement*)cursor)->expression);
                printf(";\n");
                break;
            case OBJ_STMT_VARDECLARATION: {
                ObjStmtVarDeclaration* decl = (ObjStmtVarDeclaration*)cursor;
                printf("var ");
                printObject(OBJ_VAL(decl->name));
                if (decl->initialiser) {
                    printf(" = ");
                    printExpr(decl->initialiser);
                }
                printf(";\n");
                break;
            }
            default:
                printf("Unknown stmt\n");
                break;
        }
        cursor = cursor->nextStmt;
    }
}
