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

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

typedef struct ObjString ObjString;

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)        isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value)      isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
#define IS_BUILTIN(value)      isObjType(value, OBJ_BUILTIN)
#define IS_BLOB(value)         isObjType(value, OBJ_BLOB)
#define IS_ROUTINE(value)      isObjType(value, OBJ_ROUTINE)
#define IS_CHANNEL(value)      isObjType(value, OBJ_CHANNELCONTAINER)
#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define IS_UNIFORMARRAY(value) (isObjType(value, OBJ_PACKEDUNIFORMARRAY)|| isObjType(value, OBJ_UNOWNED_UNIFORMARRAY))
#define IS_YARGTYPE(value)     (isObjType(value, OBJ_YARGTYPE) || isObjType(value, OBJ_YARGTYPE_ARRAY) || isObjType(value, OBJ_YARGTYPE_STRUCT) || isObjType(value, OBJ_YARGTYPE_POINTER) || isObjType(value, OBJ_YARGTYPE_MAP))
#define IS_POINTER(value)      (isObjType(value, OBJ_PACKEDPOINTER) || isObjType(value, OBJ_UNOWNED_PACKEDPOINTER))
#define IS_STRUCT(value)       (isObjType(value, OBJ_PACKEDSTRUCT) || isObjType(value, OBJ_UNOWNED_PACKEDSTRUCT))
#define IS_SYNCGROUP(value)    isObjType(value, OBJ_SYNCGROUP)
#define IS_MAP(value)          isObjType(value, OBJ_MAP)

#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value)        ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value) \
    (((ObjNative*)AS_OBJ(value))->function)
#define AS_BUILTIN(value) \
    (((ObjBuiltin*)AS_OBJ(value))->function)
#define AS_ROUTINE(value)      ((ObjRoutine*)AS_OBJ(value))
#define AS_CHANNEL(value)      ((ObjChannelContainer*)AS_OBJ(value))
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)
#define AS_UNIFORMARRAY(value) ((ObjPackedUniformArray*)AS_OBJ(value))
#define AS_YARGTYPE(value)     ((ObjConcreteYargType*)AS_OBJ(value))
#define AS_POINTER(value)      ((ObjPackedPointer*)AS_OBJ(value))
#define AS_STRUCT(value)       ((ObjPackedStruct*)AS_OBJ(value))
#define AS_SYNCGROUP(value)    ((ObjSyncGroup*)AS_OBJ(value))
#define AS_INTOBJ(value)       ((ObjInt*)AS_OBJ(value))
#define AS_INT(value)          (&(AS_INTOBJ(value)->bigInt))
#define AS_MAP(value)          ((ObjMap*)AS_OBJ(value))

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
    OBJ_INT
} ObjType;

struct Obj {
    ObjType type;
    bool isMarked;
    struct Obj* next;
};

typedef struct {
    Obj* stash;
    Obj** objects;
    int objectCapacity;
    int objectCount;
} DynamicObjArray;


#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

Obj* allocateObject(size_t size, ObjType type);

void initDynamicObjArray(DynamicObjArray* array);
void freeDynamicObjArray(DynamicObjArray* array);
void appendToDynamicObjArray(DynamicObjArray* array, Obj* obj);
Obj* removeLastFromDynamicObjArray(DynamicObjArray* array);

Obj* destinationObject(Value pointer);
ObjString* objectToString(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
