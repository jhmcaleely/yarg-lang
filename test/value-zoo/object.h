#ifndef cyarg_object_h
#define cyarg_object_h

/* Obj memory management
 *
 * memory in cyarg is managed through reallocate() and assorted free functions in memory.h
 *
 * These functions will call the underlying malloc & free functions from the C standard library
 * or other allocator.
 *
 * For all memory allocated to an Obj instance, free will be called automatically when the object
 * is no longer referenced.
 *
 * Obj instances are tracked in two places. All instances can be traversed, by the linked
 * list formed from the next member of Obj.
 *
 * If the Obj instance should not be deleted, it should be present in a global visible to markRoots()
 *
 * The globals are notably the language stack, some VM state, and the 'temporary root'
 * stack accessible throughout the VM.
 *
 * Since any call to reallocate() can trigger the GC, objects constructed via multiple calls to reallocate()
 * often need care in storage during this phase. Commonly either push()'d onto the
 * language stack, or tempRootPush()'d onto the temporary root stack. Note that a partially constructed object
 * must be safe to free, and object allocation zero's the whole memory cell as a convenience for this
 *
 */

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_BUILTIN,
    OBJ_ROUTINE,
    OBJ_CHANNELCONTAINER,
    OBJ_STRING,
    OBJ_UPVALUE,
    OBJ_UNOWNED_UNIFORMARRAY,
    OBJ_PACKEDUNIFORMARRAY,
    OBJ_YARGTYPE,
    OBJ_YARGTYPE_ARRAY,
    OBJ_YARGTYPE_STRUCT,
    OBJ_YARGTYPE_POINTER,
    OBJ_YARGTYPE_MAP,
    OBJ_PACKEDPOINTER,
    OBJ_UNOWNED_PACKEDPOINTER,
    OBJ_UNOWNED_PACKEDSTRUCT,
    OBJ_PACKEDSTRUCT,
    OBJ_SYNCGROUP,
    OBJ_MAP,
    OBJ_STACKSLICE,
    OBJ_AST,
    OBJ_PLACEALIAS,
    OBJ_STMT_EXPRESSION,
    OBJ_STMT_PRINT,
    OBJ_STMT_POKE,
    OBJ_STMT_VARDECLARATION,
    OBJ_STMT_FIELDDECLARATION,
    OBJ_STMT_PLACEDECLARATION,
    OBJ_STMT_BLOCK,
    OBJ_STMT_IF,
    OBJ_STMT_FUNDECLARATION,
    OBJ_STMT_WHILE,
    OBJ_STMT_RETURN,
    OBJ_STMT_YIELD,
    OBJ_STMT_FOR,
    OBJ_STMT_CLASSDECLARATION,
    OBJ_EXPR_NUMBER,
    OBJ_EXPR_ADDRESS,
    OBJ_EXPR_OPERATION,
    OBJ_EXPR_GROUPING,
    OBJ_EXPR_NAMEDVARIABLE,
    OBJ_EXPR_LITERAL,
    OBJ_EXPR_STRING,
    OBJ_EXPR_CALL,
    OBJ_EXPR_COLLECTION_INITIALIZER,
    OBJ_EXPR_COLLECTION_ELEMENT,
    OBJ_EXPR_PAIR,
    OBJ_EXPR_BUILTIN,
    OBJ_EXPR_DOT,
    OBJ_EXPR_SUPER,
    OBJ_EXPR_TYPE,
    OBJ_EXPR_TYPE_STRUCT,
    OBJ_EXPR_TYPE_INDEXED_COLLECTION,
    OBJ_INT,
    OBJ_ANYVALUE,
    OBJ_LOCATION,
} ObjType;

typedef struct Obj {
    ObjType type;
    bool isMarked;
    struct Obj* next;
} Obj;

Obj* allocateObject(size_t size, ObjType objectType);

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

void freeObject(Obj* object);

#endif
