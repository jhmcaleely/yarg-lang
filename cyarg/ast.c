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

ObjArguments* newObjArguments() {
    ObjArguments* args = ALLOCATE_OBJ(ObjArguments, OBJ_ARGUMENTS);
    args->arguments = NULL;
    args->count = 0;
    args->capacity = 0;
    return args;
}

void appendObjArgument(ObjArguments* args, ObjExpr* expr) {
    if (args->capacity < args->count + 1) {
        args->capacity = GROW_CAPACITY(args->capacity);
        args->arguments = (Obj**)realloc(args->arguments, sizeof(Obj*) * args->capacity);

        if (args->arguments == NULL) {
            printf("Out of memory!");
            exit(1);
        }
    }
    args->arguments[args->count++] = (Obj*) expr;
}

ObjExprCall* newExprCall(ObjArguments* args) {
    ObjExprCall* call = ALLOCATE_OBJ(ObjExprCall, OBJ_EXPR_CALL);
    call->args = args;
    return call;
}

ObjExprArrayInit* newExprArrayInit(ObjArguments* args) {
    ObjExprArrayInit* array = ALLOCATE_OBJ(ObjExprArrayInit, OBJ_EXPR_ARRAYINIT);
    array->expr.nextExpr = NULL;
    array->args = args;
    return array;
}

ObjExprArrayElement* newExprArrayElement() {
    ObjExprArrayElement* array = ALLOCATE_OBJ(ObjExprArrayElement, OBJ_EXPR_ARRAYELEMENT);
    array->expr.nextExpr = NULL;
    array->element = NULL;
    array->assignment = NULL;
    return array;
}

ObjExprBuiltin* newExprBuiltin(ExprBuiltin fn, int arity) {
    ObjExprBuiltin* builtin = ALLOCATE_OBJ(ObjExprBuiltin, OBJ_EXPR_BUILTIN);
    builtin->expr.nextExpr = NULL;
    builtin->builtin = fn;
    builtin->arity = arity;
    return builtin;
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
        case EXPR_OP_LOGICAL_OR: printf(" or "); break;
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
            case OBJ_EXPR_CALL: {
                ObjExprCall* call = (ObjExprCall*)cursor;
                printf("(");
                for (int i = 0; i < call->args->count; i++) {
                    printExpr((ObjExpr*)call->args->arguments[i]);
                    printf(", ");
                }
                printf(")");
                break;
            }
            case OBJ_EXPR_ARRAYINIT: {
                ObjExprArrayInit* array = (ObjExprArrayInit*)cursor;
                printf("[");
                for (int i = 0; i < array->args->count; i++) {
                    printExpr((ObjExpr*)array->args->arguments[i]);
                    printf(", ");
                }
                printf("]");
                break;
            }
            case OBJ_EXPR_ARRAYELEMENT: {
                ObjExprArrayElement* array = (ObjExprArrayElement*)cursor;
                printf("[");
                printExpr(array->element);
                printf("]");
                if (array->assignment) {
                    printf(" = ");
                    printExpr(array->assignment);
                }
                break;
            }
            case OBJ_EXPR_BUILTIN: {
                ObjExprBuiltin* fn = (ObjExprBuiltin*)cursor;
                printf("%d:%d", fn->builtin, fn->arity);
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
