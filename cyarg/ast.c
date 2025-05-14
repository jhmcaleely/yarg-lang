#include <stdlib.h>
#include <stdio.h>

#include "ast.h"
#include "memory.h"

ObjStmtExpression* newStmtExpression(ObjExpr* expr) {
    ObjStmtExpression* stmt = ALLOCATE_OBJ(ObjStmtExpression, OBJ_STMT_EXPRESSION);
    stmt->stmt.nextStmt = NULL;
    stmt->expression = expr;

    return stmt;
}

ObjStmtPrint* newStmtPrint(ObjExpr* expr) {
    ObjStmtPrint* stmt = ALLOCATE_OBJ(ObjStmtPrint, OBJ_STMT_PRINT);
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

ObjStmtBlock* newStmtBlock() {
    ObjStmtBlock* block = ALLOCATE_OBJ(ObjStmtBlock, OBJ_STMT_BLOCK);
    block->stmt.nextStmt = NULL;
    block->statements = NULL;
    return block;
}

ObjStmtIf* newStmtIf() {
    ObjStmtIf* ctrl = ALLOCATE_OBJ(ObjStmtIf, OBJ_STMT_IF);
    ctrl->stmt.nextStmt = NULL;
    ctrl->test = NULL;
    ctrl->ifStmt = NULL;
    ctrl->elseStmt = NULL;
    return ctrl;
}

ObjStmtFunDeclaration* newStmtFunDeclaration(const char* name, int nameLength) {
    ObjStmtFunDeclaration* fun = ALLOCATE_OBJ(ObjStmtFunDeclaration, OBJ_STMT_FUNDECLARATION);
    fun->stmt.nextStmt = NULL;
    fun->name = NULL;
    fun->function = NULL;
    tempRootPush(OBJ_VAL(fun));
    fun->name = copyString(name, nameLength);
    tempRootPop();
    return fun;
}

ObjFunctionDeclaration* newObjFunctionDeclaration() {
    ObjFunctionDeclaration* fun = ALLOCATE_OBJ(ObjFunctionDeclaration, OBJ_FUNDECLARATION);
    fun->arity = 0;
    fun->body = NULL;
    return fun;
}

ObjExprOperation* newExprOperation(ObjExpr* rhs, ExprOp op) {
    ObjExprOperation* operation = ALLOCATE_OBJ(ObjExprOperation, OBJ_EXPR_OPERATION);
    operation->expr.nextExpr = NULL;
    operation->rhs = rhs;
    operation->operation = op;

    return operation;
}

ObjExprGrouping* newExprGrouping(ObjExpr* expression) {
    ObjExprGrouping* grp = ALLOCATE_OBJ(ObjExprGrouping, OBJ_EXPR_GROUPING);
    grp->expr.nextExpr = NULL;
    grp->expression = expression;
    return grp;
}


ObjExprNumber* newExprNumberDouble(double value) {
    ObjExprNumber* num = ALLOCATE_OBJ(ObjExprNumber, OBJ_EXPR_NUMBER);
    num->expr.nextExpr = NULL;
    num->type = NUMBER_DOUBLE;
    num->val.dbl = value;
    return num;
}

ObjExprNumber* newExprNumberInteger(int value) {
    ObjExprNumber* num = ALLOCATE_OBJ(ObjExprNumber, OBJ_EXPR_NUMBER);
    num->expr.nextExpr = NULL;
    num->type = NUMBER_INTEGER;
    num->val.integer = value;
    return num;
}

ObjExprNumber* newExprNumberUInteger32(uint32_t value) {
    ObjExprNumber* num = ALLOCATE_OBJ(ObjExprNumber, OBJ_EXPR_NUMBER);
    num->expr.nextExpr = NULL;
    num->type = NUMBER_UINTEGER32;
    num->val.uinteger32 = value;
    return num;
}

ObjExprNamedVariable* newExprNamedVariable(const char* name, int nameLength, ObjExpr* expr) {
    ObjExprNamedVariable* var = ALLOCATE_OBJ(ObjExprNamedVariable, OBJ_EXPR_NAMEDVARIABLE);
    var->expr.nextExpr = NULL;
    var->assignment = expr;
    tempRootPush(OBJ_VAL(var));
    var->name = copyString(name, nameLength);
    tempRootPop();
    return var;
}

ObjExprLiteral* newExprLiteral(ExprLiteral literal) {
    ObjExprLiteral* lit = ALLOCATE_OBJ(ObjExprLiteral, OBJ_EXPR_LITERAL);
    lit->expr.nextExpr = NULL;
    lit->literal = literal;
    return lit;
}

ObjExprString* newExprString(const char* str, int strLength) {
    ObjExprString* string = ALLOCATE_OBJ(ObjExprString, OBJ_EXPR_STRING);
    string->expr.nextExpr = NULL;
    string->string = NULL;
    tempRootPush(OBJ_VAL(string));
    string->string = copyString(str, strLength);
    tempRootPop();
    return string;
}

static void printExprOperation(ObjExprOperation* opexpr) {
    switch (opexpr->operation) {
        case EXPR_OP_EQUAL: printf("=="); break;
        case EXPR_OP_GREATER: printf(">"); break;
        case EXPR_OP_RIGHT_SHIFT: printf(">>"); break;
        case EXPR_OP_LESS: printf("<"); break;
        case EXPR_OP_LEFT_SHIFT: printf("<<"); break;
        case EXPR_OP_ADD: printf("+"); break;
        case EXPR_OP_SUBTRACT: printf("-"); break;
        case EXPR_OP_MULTIPLY: printf("*"); break;
        case EXPR_OP_DIVIDE: printf("/"); break;
        case EXPR_OP_BIT_OR: printf("|"); break;
        case EXPR_OP_BIT_AND: printf("&"); break;
        case EXPR_OP_BIT_XOR: printf("^"); break;
        case EXPR_OP_MODULO: printf("%%"); break;
        case EXPR_OP_NOT_EQUAL: printf("!="); break;
        case EXPR_OP_GREATER_EQUAL: printf(">="); break;
        case EXPR_OP_LESS_EQUAL: printf("<="); break;
        case EXPR_OP_NOT: printf("!"); break;
        case EXPR_OP_NEGATE: printf("-"); break;
        case EXPR_OP_LOGICAL_AND: printf(" and "); break;
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
                printExprOperation((ObjExprOperation*)cursor);
                break;
            case OBJ_EXPR_GROUPING: {
                ObjExprGrouping* expr = (ObjExprGrouping*)cursor;
                printf("(");
                printExpr(expr->expression);
                printf(")");
                break;
            }
            case OBJ_EXPR_NUMBER: {
                ObjExprNumber* num = (ObjExprNumber*)cursor;
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
            case OBJ_EXPR_NAMEDVARIABLE: {
                ObjExprNamedVariable* var = (ObjExprNamedVariable*)cursor;
                printObject(OBJ_VAL(var->name));
                if (var->assignment) {
                    printf(" = ");
                    printExpr(var->assignment);
                }
                break;
            }
            case OBJ_EXPR_LITERAL: {
                ObjExprLiteral* lit = (ObjExprLiteral*)cursor;
                switch (lit->literal) {
                    case EXPR_LITERAL_FALSE: printf("false"); break;
                    case EXPR_LITERAL_TRUE: printf("true"); break;
                    case EXPR_LITERAL_NIL: printf("nil"); break;
                }
                break;
            }
            case OBJ_EXPR_STRING: {
                ObjExprString* str = (ObjExprString*)cursor;
                printf("\"");
                printObject(OBJ_VAL(str->string));
                printf("\"");
                break;
            }
            default:
                printf("<unknown>");
        }
        cursor = cursor->nextExpr;
    }
}

void printStmtIf(ObjStmtIf* ctrl) {
    printf("if (");
    printExpr(ctrl->test);
    printf(")\n");
    printStmts(ctrl->ifStmt);
    if (ctrl->elseStmt) {
        printf("else\n");
        printStmts(ctrl->elseStmt);
    }
}

void printFunDeclaration(ObjStmtFunDeclaration* decl) {
    printf("fun ");
    printObject(OBJ_VAL(decl->name));
    printf("(");
    for (int i = 0; i < decl->function->arity; i++) {
        printExpr(decl->function->params[i]);
        printf(", ");
    }
    printf(")\n");
    printStmts((ObjStmt*)decl->function->body);
}

void printStmts(ObjStmt* stmts) {
    ObjStmt* cursor = stmts;
    while (cursor) {
        switch (cursor->obj.type) {
            case OBJ_STMT_EXPRESSION:
                printf("(");
                printExpr(((ObjStmtExpression*)cursor)->expression);
                printf(");\n");
                break;
            case OBJ_STMT_PRINT:
                printf("print ");
                printExpr(((ObjStmtPrint*)cursor)->expression);
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
            case OBJ_STMT_BLOCK: {
                ObjStmtBlock* block = (ObjStmtBlock*)cursor;
                printf("{\n");
                printStmts(block->statements);
                printf("}\n");
                break;
            }
            case OBJ_STMT_IF: {
                ObjStmtIf* ctrl = (ObjStmtIf*)cursor;
                printStmtIf(ctrl);
                break;
            }
            case OBJ_STMT_FUNDECLARATION: {
                ObjStmtFunDeclaration* decl = (ObjStmtFunDeclaration*)cursor;
                printFunDeclaration(decl);
                break;
            }
            default:
                printf("Unknown stmt;\n");
                break;
        }
        cursor = cursor->nextStmt;
    }
}
