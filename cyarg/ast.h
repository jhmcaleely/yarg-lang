#ifndef cyarg_ast_h
#define cyarg_ast_h

#include "object.h"

typedef struct ObjStmt ObjStmt;

typedef struct ObjStmt {
    Obj obj;
    int line;
    ObjStmt* nextStmt;
} ObjStmt;

typedef struct ObjExpr ObjExpr;

typedef struct ObjExpr {
    Obj obj;
    ObjExpr* nextExpr;
} ObjExpr;

typedef struct ObjAst {
    Obj obj;
    ObjStmt* statements;
} ObjAst;

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
    EXPR_OP_LOGICAL_AND,
    EXPR_OP_LOGICAL_OR
} ExprOp;

typedef enum {
    EXPR_LITERAL_TRUE,
    EXPR_LITERAL_FALSE,
    EXPR_LITERAL_NIL
} ExprLiteral;

typedef enum {
    EXPR_BUILTIN_IMPORT,
    EXPR_BUILTIN_MAKE_ARRAY,
    EXPR_BUILTIN_MAKE_ROUTINE,
    EXPR_BUILTIN_MAKE_CHANNEL,
    EXPR_BUILTIN_RESUME,
    EXPR_BUILTIN_START,
    EXPR_BUILTIN_RECEIVE,
    EXPR_BUILTIN_SEND,
    EXPR_BUILTIN_PEEK,
    EXPR_BUILTIN_SHARE,
    EXPR_BUILTIN_RPEEK,
    EXPR_BUILTIN_RPOKE,
    EXPR_BUILTIN_LEN
} ExprBuiltin;

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
    ObjString* name;
    ObjExpr* value;
} ObjExprNamedConstant;

typedef struct {
    ObjExpr expr;
    ExprLiteral literal;
} ObjExprLiteral;

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

typedef struct {
    ObjExpr expr;
    ObjString* string;
} ObjExprString;

typedef struct {
    ObjExpr expr;
    DynamicObjArray arguments;
} ObjExprCall;

typedef struct {
    ObjExpr expr;
    DynamicObjArray initializers;
} ObjExprArrayInit;

typedef struct {
    ObjExpr expr;
    ObjExpr* element;
    ObjExpr* assignment;
} ObjExprArrayElement;

typedef struct {
    ObjExpr expr;
    ExprBuiltin builtin;
    int arity;
} ObjExprBuiltin;

typedef struct {
    ObjExpr expr;
    ObjString* name;
    ObjExpr* offset;
    ObjExpr* assignment;
    ObjExprCall* call;
} ObjExprDot;

typedef struct {
    ObjExpr expr;
    ObjString* name;
    ObjExprCall* call;
} ObjExprSuper;

typedef enum {
    EXPR_TYPE_MUINT32
} ExprTypeType;

typedef struct {
    ObjExpr expr;
    ExprTypeType type;
} ObjExprTypeBuiltin;

typedef struct {
    ObjExpr expr;
    Table fields;
    size_t size;
} ObjExprTypeStruct;

typedef struct {
    ObjExpr expr;
    ObjString* name;
} ObjExprTypeKnown;

typedef struct {
    ObjStmt stmt;
    ObjStmt* statements;
} ObjStmtBlock;

typedef struct  {
    ObjStmt stmt;
    ObjExpr* expression;
} ObjStmtExpression;

typedef struct {
    ObjStmt stmt;
    ObjString* name;
    ObjExpr* initialiser;
} ObjStmtVarDeclaration;

typedef struct {
    ObjStmt stmt;
    ObjString* name;
    DynamicObjArray parameters;
    ObjStmt* body;
} ObjStmtFunDeclaration;

typedef struct {
    ObjStmt stmt;
    ObjString* name;
    ObjExpr* superclass;
    DynamicObjArray methods;
} ObjStmtClassDeclaration;

typedef enum {
    ACCESS_RW,
    ACCESS_RO,
    ACCESS_WO,
} AccessRule;

typedef struct {
    ObjStmt stmt;
    ObjString* name;
    AccessRule access;
    ObjExpr* offset;
    ObjExpr* array_cardinality;
    ObjExpr* type;
} ObjStmtFieldDeclaration;

typedef struct {
    ObjStmt stmt;
    ObjString* name;
    AccessRule access;
    ObjExpr* address;
    ObjExpr* type;
} ObjStmtMapDeclaration;

typedef struct {
    ObjStmt stmt;
    ObjString* name;
    ObjExpr* type;
} ObjStmtTypeDeclaration;

typedef struct {
    ObjStmt stmt;
    ObjExpr* test;
    ObjStmt* ifStmt;
    ObjStmt* elseStmt;
} ObjStmtIf;

typedef struct {
    ObjStmt stmt;
    ObjExpr* test;
    ObjStmt* loop;
} ObjStmtWhile;

typedef struct {
    ObjStmt stmt;
    ObjStmt* initializer;
    ObjExpr* condition;
    ObjExpr* loopExpression;
    ObjStmt* body;
} ObjStmtFor;

ObjAst* newObjAst();

ObjExprNumber* newExprNumberDouble(double value);
ObjExprNumber* newExprNumberInteger(int value);
ObjExprNumber* newExprNumberUInteger32(uint32_t value);
ObjExprLiteral* newExprLiteral(ExprLiteral literal);
ObjExprString* newExprString(const char* str, int strLength);
ObjExprOperation* newExprOperation(ObjExpr* rhs, ExprOp op);
ObjExprGrouping* newExprGrouping(ObjExpr* expression);
ObjExprNamedVariable* newExprNamedVariable(const char* name, int nameLength);
ObjExprNamedConstant* newExprNamedConstant(const char* name, int nameLength);
ObjExprCall* newExprCall();
ObjExprArrayInit* newExprArrayInit();
ObjExprArrayElement* newExprArrayElement();
ObjExprBuiltin* newExprBuiltin(ExprBuiltin fn, int arity);
ObjExprDot* newExprDot(const char* name, int nameLength);
ObjExprSuper* newExprSuper(const char* name, int nameLength);
ObjExprTypeBuiltin* newExprTypeBuiltin(ExprTypeType type);
ObjExprTypeStruct* newExprTypeStruct();
ObjExprTypeKnown* newExprTypeKnown(const char* name, int nameLength);

ObjStmtExpression* newStmtExpression(ObjExpr* expr, ObjType statement, int line);
ObjStmtBlock* newStmtBlock(int line);

ObjStmtVarDeclaration* newStmtVarDeclaration(const char* name, int nameLength, ObjExpr* expr, int line);
ObjStmtFunDeclaration* newStmtFunDeclaration(const char* name, int nameLength, int line);
ObjStmtClassDeclaration* newStmtClassDeclaration(const char* name, int nameLength, int line);
ObjStmtTypeDeclaration* newStmtTypeDeclaration(const char* name, int nameLength, int line);
ObjStmtFieldDeclaration* newStmtFieldDeclaration(const char* name, int nameLength, int line);
ObjStmtMapDeclaration* newStmtMapDeclaration(const char* name, int nameLength, int line);
 
ObjStmtIf* newStmtIf(int line);
ObjStmtWhile* newStmtWhile(int line);
ObjStmtFor* newStmtFor(int line);

void printStmts(ObjStmt* stmts);
void printExpr(ObjExpr* expr);
void printSourceValue(FILE* op, Value value);

#endif