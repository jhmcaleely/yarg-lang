//
//  promote.c
//  BigInt
//
//  Created by dlm on 16/02/2026.
//

#include "promote.h"
#include "ast.h"

#include <stdio.h>
#include <assert.h> // should cause runtime error in target

static bool promoteExpression(ObjExpr **); // returns true if argument has reduced to Literal Int

static void promoteTo(ObjExprNumber *, NumberType);

// var uint8 x = 1; => var uint8 x = 1u8;
// var uint8 x; x = 1; => var uint8 x; x = 1u8;
// x[1] = 0; => x[1u8] = 0;
// x[1000] = 0; => x[1000u16] = 0;
// poke @x100, 5 => poke @x100, 5u32
// int64(5) => 5i64

void promoteLitIntStatements(ObjStmt* statements)
{
    ObjStmt *s = statements;
    while (s != 0)
    {
        switch (s->obj.type)
        {
        case OBJ_STMT_RETURN:
        case OBJ_STMT_YIELD:
        case OBJ_STMT_PRINT:
        case OBJ_STMT_EXPRESSION:
            promoteExpression(&((ObjStmtExpression *) s)->expression);
            break;
        case OBJ_STMT_POKE: {
            ObjStmtPoke *poke = (ObjStmtPoke *) s;
            promoteExpression(&poke->location);
            promoteExpression(&poke->offset);
            promoteExpression(&poke->assignment);
            break;
        }
        case OBJ_STMT_VARDECLARATION: { // ------ var uint8 x = 1; -----------------------------------------------------
            ObjStmtVarDeclaration *varDecl = (ObjStmtVarDeclaration *) s;
            if (varDecl->initialiser)
            {
                if (varDecl->initialiser->obj.type == OBJ_EXPR_NUMBER && varDecl->initialiser->nextExpr == 0)
                {
                    ObjExprNumber *n = (ObjExprNumber *) &varDecl->initialiser->obj;
                    if (n->type == NUMBER_INT)
                    {
                        if (varDecl->type->obj.type == OBJ_EXPR_TYPE)
                        {
                            ObjExprTypeLiteral *tl = (ObjExprTypeLiteral *) &varDecl->type->obj;
                            promoteTo(n, tl->type);
                        }
                    }
                }
            }
            break;
        }
        case OBJ_STMT_PLACEDECLARATION:
            break;
        case OBJ_STMT_BLOCK: {
            ObjStmtBlock *block = (ObjStmtBlock *) s;
            promoteLitIntStatements(block->statements);
            break;
        }
        case OBJ_STMT_IF: {
            ObjStmtIf *ifelse = (ObjStmtIf *) s;
            promoteExpression(&ifelse->test);
            promoteLitIntStatements(ifelse->ifStmt);
            promoteLitIntStatements(ifelse->elseStmt);
            break;
        }
        case OBJ_STMT_FUNDECLARATION: {
            ObjStmtFunDeclaration *fun = (ObjStmtFunDeclaration *) s;
            promoteLitIntStatements(fun->body);
            break;
        }
        case OBJ_STMT_WHILE: {
            ObjStmtWhile *whileDone = (ObjStmtWhile *) s;
            promoteExpression(&whileDone->test);
            promoteLitIntStatements(whileDone->loop);
            break;
        }
        case OBJ_STMT_FOR: {
            ObjStmtFor *forNext = (ObjStmtFor *) s;
            promoteLitIntStatements(forNext->initializer);
            promoteExpression(&forNext->condition);
            promoteExpression(&forNext->loopExpression);
            promoteLitIntStatements(forNext->body);
            break;
        }
        case OBJ_STMT_CLASSDECLARATION: {
            ObjStmtClassDeclaration *classDecl = (ObjStmtClassDeclaration *) s;
            for (int i = 0; i < classDecl->methods.objectCount; i++)
            {
                ObjStmtFunDeclaration *fun = (ObjStmtFunDeclaration *) classDecl->methods.objects[i];
                promoteLitIntStatements(fun->body);
            }
            break;
        }
        case OBJ_STMT_FIELDDECLARATION: {
            ObjStmtFieldDeclaration *field = (ObjStmtFieldDeclaration *) s;
            promoteExpression(&field->offset);
            break;
        }
        default:
            assert(!"Statement");
            break;
        }
        s = s->nextStmt;
    }
}

void promoteTo(ObjExprNumber *n, NumberType t)
{
    assert(n->type == NUMBER_INT);
    switch (t)
    {
    default:
        break;
    }
}

bool promoteExpression(ObjExpr **ep)
{
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
            break;
        case OBJ_EXPR_OPERATION: {
            ObjExprOperation *o = (ObjExprOperation *) e;
            promoteExpression(&o->rhs);
            break;
        }
        case OBJ_EXPR_GROUPING: {
            ObjExprGrouping *g = (ObjExprGrouping *) e;
            promoteExpression(&g->expression);
            break;
        }
        case OBJ_EXPR_CALL: {
            ObjExprCall* call = (ObjExprCall *)e;
            DynamicObjArray *a = &call->arguments;

            for (int i = 0; i < a->objectCount; i++)
            {
                promoteExpression((ObjExpr **) &a->objects[i]);
            }
            break;
        }
        case OBJ_EXPR_ARRAYINIT: {
            ObjExprArrayInit* array = (ObjExprArrayInit *)e;
            DynamicObjArray *a = &array->initializers;

            for (int i = 0; i < a->objectCount; i++)
            {
                promoteExpression((ObjExpr **) &a->objects[i]);
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
