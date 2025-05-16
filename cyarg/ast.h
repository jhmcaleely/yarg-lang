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
    ExprLiteral literal;
} ObjExprLiteral;

typedef struct  {
    ObjStmt stmt;
    ObjExpr* expression;
} ObjStmtExpression;

typedef struct {
    ObjStmt stmt;
    ObjExpr* expression;
} ObjStmtPrint;

typedef struct {
    ObjStmt stmt;
    ObjString* name;
    ObjExpr* initialiser;
} ObjStmtVarDeclaration;

typedef struct {
    ObjStmt stmt;
    ObjStmt* statements;
} ObjStmtBlock;

typedef struct {
    ObjStmt stmt;
    ObjExpr* test;
    ObjStmt* ifStmt;
    ObjStmt* elseStmt;
} ObjStmtIf;

typedef struct {
    Obj obj;
    unsigned int arity;
    ObjExpr* params[UINT8_MAX];
    ObjStmtBlock* body;
} ObjFunctionDeclaration;

typedef struct {
    ObjStmt stmt;
    ObjString* name;
    ObjFunctionDeclaration* function;
} ObjStmtFunDeclaration;

typedef struct {
    ObjStmt stmt;
    ObjExpr* test;
    ObjStmt* loop;
} ObjStmtWhile;

typedef struct {
    ObjStmt stmt;
    ObjExpr* value;
} ObjStmtReturnOrYield;

typedef struct {
    ObjStmt stmt;
    ObjStmt* initializer;
    ObjExpr* condition;
    ObjExpr* loopExpression;
    ObjStmt* body;
} ObjStmtFor;

typedef struct {
    ObjStmt stmt;
    ObjString* name;
    ObjExpr* superclass;
    Obj** methods;
    int methodCapacity;
    int methodCount;
} ObjStmtClassDeclaration;

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
    Obj obj;
    int count;
    int capacity;
    Obj** arguments;
} ObjArguments;

typedef struct {
    ObjExpr expr;
    ObjArguments* args;
} ObjExprCall;

typedef struct {
    ObjExpr expr;
    ObjArguments* args;
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
    ObjExpr* assignment;
    ObjArguments* callArgs;
} ObjExprDot;

typedef struct {
    ObjExpr expr;
    ObjString* name;
    ObjArguments* callArgs;
} ObjExprSuper;

ObjExprNumber* newExprNumberDouble(double value);
ObjExprNumber* newExprNumberInteger(int value);
ObjExprNumber* newExprNumberUInteger32(uint32_t value);
ObjExprOperation* newExprOperation(ObjExpr* rhs, ExprOp op);
ObjExprGrouping* newExprGrouping(ObjExpr* expression);
ObjExprNamedVariable* newExprNamedVariable(const char* name, int nameLength, ObjExpr* expr);
ObjExprLiteral* newExprLiteral(ExprLiteral literal);
ObjExprString* newExprString(const char* str, int strLength);
ObjArguments* newObjArguments();
ObjExprCall* newExprCall(ObjArguments* args);
ObjExprArrayInit* newExprArrayInit(ObjArguments* args);
ObjExprArrayElement* newExprArrayElement();
ObjExprBuiltin* newExprBuiltin(ExprBuiltin fn, int arity);
ObjExprDot* newExprDot(const char* name, int nameLength);
ObjExprSuper* newExprSuper(const char* name, int nameLength);

void appendObjArgument(ObjArguments* args, ObjExpr* expr);
void appendMethod(ObjStmtClassDeclaration* class_, ObjStmtFunDeclaration* method);

ObjStmtExpression* newStmtExpression(ObjExpr* expr);
ObjStmtPrint* newStmtPrint(ObjExpr* expr);
ObjStmtVarDeclaration* newStmtVarDeclaration(char* name, int nameLength, ObjExpr* expr);
ObjStmtBlock* newStmtBlock();
ObjStmtIf* newStmtIf();
ObjStmtFunDeclaration* newStmtFunDeclaration(const char* name, int nameLength);
ObjStmtWhile* newStmtWhile();
ObjStmtReturnOrYield* newStmtReturnOrYield(bool ret);
ObjStmtFor* newStmtFor();
ObjStmtClassDeclaration* newStmtClassDeclaration(const char* name, int nameLength);

ObjFunctionDeclaration* newObjFunctionDeclaration();

void printStmts(ObjStmt* stmts);
void printExpr(ObjExpr* expr);

#endif