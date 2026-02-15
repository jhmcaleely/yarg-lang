//
//  precalc.c
//  BigInt
//
//  Created by dlm on 14/02/2026.
//

#include "precalc.h"
#include "ast.h"

#include <stdio.h>
#include <assert.h> // should cause runtime error in target

static bool precalcExpression(ObjExpr **); // returns true if argument has reduced to Literal Int

// (5) => 5
// 6+7 => 13
// 7+(2*1) => 10
// ...etc.

void precalcStatements(ObjStmt* stmts)
{
    ObjStmt *stmt = stmts;
    while (stmt)
    {
        switch (stmt->obj.type)
        {
        case OBJ_STMT_RETURN:
        case OBJ_STMT_YIELD:
        case OBJ_STMT_PRINT:
        case OBJ_STMT_EXPRESSION:
            precalcExpression(&((ObjStmtExpression *) stmt)->expression);
            break;
        case OBJ_STMT_POKE: {
            ObjStmtPoke *poke = (ObjStmtPoke *) stmt;
            precalcExpression(&poke->location);
            precalcExpression(&poke->offset);
            precalcExpression(&poke->assignment);
            break;
        }
        case OBJ_STMT_VARDECLARATION: {
            ObjStmtVarDeclaration *varDecl = (ObjStmtVarDeclaration *) stmt;
            if (varDecl->initialiser)
            {
                precalcExpression(&varDecl->initialiser);
            }
            break;
        }
        case OBJ_STMT_PLACEDECLARATION:
            break;
        case OBJ_STMT_BLOCK: {
            ObjStmtBlock *block = (ObjStmtBlock *) stmt;
            precalcStatements(block->statements);
            break;
        }
        case OBJ_STMT_IF: {
            ObjStmtIf *ifelse = (ObjStmtIf *) stmt;
            precalcExpression(&ifelse->test);
            precalcStatements(ifelse->ifStmt);
            precalcStatements(ifelse->elseStmt);
            break;
        }
        case OBJ_STMT_FUNDECLARATION: {
            ObjStmtFunDeclaration *fun = (ObjStmtFunDeclaration *) stmt;
            precalcStatements(fun->body);
            break;
        }
        case OBJ_STMT_WHILE: {
            ObjStmtWhile *whileDone = (ObjStmtWhile*) stmt;
            precalcExpression(&whileDone->test);
            precalcStatements(whileDone->loop);
            break;
        }
        case OBJ_STMT_FOR: {
            ObjStmtFor *forNext = (ObjStmtFor *) stmt;
            precalcStatements(forNext->initializer);
            precalcExpression(&forNext->condition);
            precalcExpression(&forNext->loopExpression);
            precalcStatements(forNext->body);
            break;
        }
        case OBJ_STMT_CLASSDECLARATION: {
            ObjStmtClassDeclaration *classDecl = (ObjStmtClassDeclaration*) stmt;
            for (int i = 0; i < classDecl->methods.objectCount; i++)
            {
                ObjStmtFunDeclaration *fun = (ObjStmtFunDeclaration *) classDecl->methods.objects[i];
                precalcStatements(fun->body);
            }
            break;
        }
        case OBJ_STMT_FIELDDECLARATION: {
            ObjStmtFieldDeclaration *field = (ObjStmtFieldDeclaration*) stmt;
            precalcExpression(&field->offset);
            break;
        }
        default: assert(!"Statement"); break;
        }
        stmt = stmt->nextStmt;
    }
}

static bool isReducible(ObjExpr *e);
static Int *intExpr(ObjExpr *e);

bool precalcExpression(ObjExpr **ep)
{
    ObjExpr **h = ep;
    ObjExpr *e;
    bool r = true;

    while (ep != 0 && (e = *ep) != 0)
    {
        ObjType t = e->obj.type;
        switch (t)
        {
        case OBJ_EXPR_NUMBER:
        case OBJ_EXPR_NAMEDVARIABLE:
        case OBJ_EXPR_STRING:
        case OBJ_EXPR_LITERAL:
            assert(e == *h);
            r = isReducible(e);
            if (e->nextExpr == 0)
            {
                return r;
            }
            break;
        case OBJ_EXPR_OPERATION: {
            ObjExprOperation *o = (ObjExprOperation *) e;
            bool rhsIsReducecible = precalcExpression(&o->rhs);
            r = r & rhsIsReducecible;
            if (r)
            {
                switch (o->operation)
                {
                    // todo - could all be done if precalc handled machine type literals
                    // EXPR_OP_EQUAL, EXPR_OP_GREATER, EXPR_OP_RIGHT_SHIFT, EXPR_OP_LESS,  EXPR_OP_LEFT_SHIFT,
                    // EXPR_OP_BIT_OR, EXPR_OP_BIT_AND, EXPR_OP_BIT_XOR,
                    // EXPR_OP_NOT_EQUAL, EXPR_OP_GREATER_EQUAL, EXPR_OP_LESS_EQUAL, EXPR_OP_NOT,
                    // EXPR_OP_LOGICAL_AND, EXPR_OP_LOGICAL_OR, EXPR_OP_DEREF_PTR
                case EXPR_OP_ADD:
                    int_add(intExpr(*h), intExpr(o->rhs), intExpr(*h));
                    (*h)->nextExpr = e->nextExpr;
                    ep = h;
                    break;
                case EXPR_OP_SUBTRACT:
                    int_sub(intExpr(*h), intExpr(o->rhs), intExpr(*h));
                    (*h)->nextExpr = e->nextExpr;
                    ep = h;
                    break;
                case EXPR_OP_MULTIPLY: {
                    Int p;
                    int_mul(intExpr(*h), intExpr(o->rhs), &p);
                    int_set_t(&p, intExpr(*h));
                    (*h)->nextExpr = e->nextExpr;
                    ep = h;
                    break;
                }
                case EXPR_OP_DIVIDE:
                    int_div(intExpr(*h), intExpr(o->rhs), intExpr(*h), 0);
                    (*h)->nextExpr = e->nextExpr;
                    ep = h;
                    break;
                case EXPR_OP_MODULO: {
                    Int q;
                    int_div(intExpr(*h), intExpr(o->rhs), &q, intExpr(*h)); // todo allow q == 0
                    (*h)->nextExpr = e->nextExpr;
                    ep = h;
                    break;
                }
                case EXPR_OP_NEGATE:
                    int_neg(intExpr(o->rhs));
                    o->rhs->nextExpr = e->nextExpr;
                    *ep = o->rhs;
                    break;
                default:
                    break;
                }
            }
            else
            {
                r = false;
            }
            break;
        }
        case OBJ_EXPR_GROUPING: {
            ObjExprGrouping *g = (ObjExprGrouping *) e;
            bool expressionIsReducecible = precalcExpression(&g->expression);
            r = r & expressionIsReducecible;
            if (expressionIsReducecible)
            {
                g->expression->nextExpr = e->nextExpr;
                *ep = g->expression; // no need to have a group of a number
            }
            break;
        }
        case OBJ_EXPR_CALL: {
            ObjExprCall* call = (ObjExprCall *)e;
            DynamicObjArray *a = &call->arguments;

            for (int i = 0; i < a->objectCount; i++)
            {
                precalcExpression((ObjExpr **) &a->objects[i]);
            }
            break;
        }
        case OBJ_EXPR_ARRAYINIT: {
            ObjExprArrayInit* array = (ObjExprArrayInit *)e;
            DynamicObjArray *a = &array->initializers;

            for (int i = 0; i < a->objectCount; i++)
            {
                precalcExpression((ObjExpr **) &a->objects[i]);
            }
            break;
        }
        default:
            printf("%d\n", t);
            break;
        }
        ep = &(*ep)->nextExpr;
    }
    return r;
}


bool isReducible(ObjExpr *e)
{
    return (e != 0 && e->obj.type == OBJ_EXPR_NUMBER && ((ObjExprNumber *) e)->type == NUMBER_INT);
}

Int *intExpr(ObjExpr *e)
{
    assert(isReducible(e));
    return &((ObjExprNumber *) e)->val.bigInt;
}
