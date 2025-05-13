#include <stdlib.h>
#include <stdio.h>

#include "ast_compiler.h"
#include "memory.h"
#include "vm.h"
#include "yargtype.h"
#include "ast.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    vm.bytesAllocated += newSize - oldSize;
    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif

        if (vm.bytesAllocated > vm.nextGC) {
            collectGarbage();
        }
    }

    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) {
        printf("help! no memory.");
        exit(1);
    }
    return result;
}

void tempRootPush(Value value) {
    *vm.tempRootsTop = value;
    vm.tempRootsTop++;

    if (vm.tempRootsTop - &vm.tempRoots[0] >= TEMP_ROOTS_MAX) {
        fatalVMError("Allocation Stash Max Exeeded.");
    }
}

Value tempRootPop() {
    vm.tempRootsTop--;
    return *vm.tempRootsTop;
}

void markObject(Obj* object) {
    if (object == NULL) return;
    if (object->isMarked) return;

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    object->isMarked = true;

    if (vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.grayStack = (Obj**)realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);

        if (vm.grayStack == NULL) exit(1);
    }

    vm.grayStack[vm.grayCount++] = object;
}

void markValue(Value value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

static void markArray(ValueArray* array) {
    for (int i = 0; i < array->count; i++) {
        markValue(array->values[i]);
    }
}

static void blackenObject(Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    switch (object->type) {
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod* bound = (ObjBoundMethod*)object;
            markValue(bound->reciever);
            markObject((Obj*)bound->method);
            break;
        }
        case OBJ_CLASS: {
            ObjClass* klass = (ObjClass*)object;
            markObject((Obj*)klass->name);
            markTable(&klass->methods);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            markObject((Obj*)closure->function);
            for (int i = 0; i < closure->upvalueCount; i++) {
                markObject((Obj*)closure->upvalues[i]);
            }
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            markObject((Obj*)function->name);
            markArray(&function->chunk.constants);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            markObject((Obj*)instance->klass);
            markTable(&instance->fields);
            break;
        }
        case OBJ_UPVALUE:
            markValue(((ObjUpvalue*)object)->closed);
            break;
        case OBJ_ROUTINE: {
            ObjRoutine* stack = (ObjRoutine*)object;
            markRoutine(stack);
            break;
        }
        case OBJ_VALARRAY: {
            ObjValArray* array = (ObjValArray*)object;
            markArray(&array->array);
            break;
        }
        case OBJ_UNIFORMARRAY: {
            ObjUniformArray* array = (ObjUniformArray*)object;
            if (is_obj_type(array->element_type)) {
                for (int i = 0; i < array->count; i++) {
                    Obj** elements = (Obj**) array->array;
                    if (elements[i]) {
                        markObject(elements[i]);
                    }
                }
            }
            markObject((Obj*)array->element_type);
            break;
        }
        case OBJ_YARGTYPE: {
            ObjYargType* type = (ObjYargType*)object;
            if (type->yt == TypeInstance) {
                markObject((Obj*)type->instanceKlass);
            }
            break;
        }
        case OBJ_STMT_EXPRESSION: {
            ObjStmtExpression* stmt = (ObjStmtExpression*)object;
            markObject((Obj*)stmt->stmt.nextStmt);
            markObject((Obj*)stmt->expression);
            break;
        }
        case OBJ_STMT_PRINT: {
            ObjStmtPrint* stmt = (ObjStmtPrint*)object;
            markObject((Obj*)stmt->stmt.nextStmt);
            markObject((Obj*)stmt->expression);
            break;
        }
        case OBJ_STMT_VARDECLARATION: {
            ObjStmtVarDeclaration* stmt = (ObjStmtVarDeclaration*)object;
            markObject((Obj*)stmt->stmt.nextStmt);
            markObject((Obj*)stmt->name);
            markObject((Obj*)stmt->initialiser);
            break;
        }
        case OBJ_STMT_BLOCK: {
            ObjStmtBlock* block = (ObjStmtBlock*)object;
            markObject((Obj*)block->stmt.nextStmt);
            markObject((Obj*)block->statements);
            break;
        }
        case OBJ_STMT_IF: {
            ObjStmtIf* ctrl = (ObjStmtIf*)object;
            markObject((Obj*)ctrl->stmt.nextStmt);
            markObject((Obj*)ctrl->test);
            markObject((Obj*)ctrl->ifStmt);
            markObject((Obj*)ctrl->elseStmt);
            break;
        }
        case OBJ_EXPR_NUMBER: {
            ObjExprNumber* expr = (ObjExprNumber*)object;
            markObject((Obj*)expr->expr.nextExpr);
            break;
        }       
        case OBJ_EXPR_OPERATION: {
            ObjExprOperation* expr = (ObjExprOperation*)object;
            markObject((Obj*)expr->expr.nextExpr);
            markObject((Obj*)expr->rhs);
            break;

        }
        case OBJ_EXPR_GROUPING: {
            ObjExprGrouping* expr = (ObjExprGrouping*)object;
            markObject((Obj*)expr->expr.nextExpr);
            markObject((Obj*)expr->expression);
            break;
        }
        case OBJ_EXPR_NAMEDVARIABLE: {
            ObjExprNamedVariable* var = (ObjExprNamedVariable*)object;
            markObject((Obj*)var->expr.nextExpr);
            markObject((Obj*)var->assignment);
            markObject((Obj*)var->name);
            break;
        }
        case OBJ_EXPR_LITERAL: {
            ObjExprLiteral* lit = (ObjExprLiteral*)object;
            markObject((Obj*)lit->expr.nextExpr);
            break;
        }
        case OBJ_EXPR_STRING: {
            ObjExprString* str = (ObjExprString*)object;
            markObject((Obj*)str->expr.nextExpr);
            markObject((Obj*)str->string);
            break;
        }
        case OBJ_NATIVE:
        case OBJ_BLOB:
        case OBJ_CHANNEL:
        case OBJ_STRING:
            break;
    }
}

static void freeObject(Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
#endif

    switch (object->type) {
        case OBJ_BOUND_METHOD:
            FREE(ObjBoundMethod, object);
            break;
        case OBJ_CLASS: {
            ObjClass* klass = (ObjClass*)object;
            freeTable(&klass->methods);
            FREE(ObjClass, object);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            FREE(ObjClosure, object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            freeChunk(&function->chunk);
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            freeTable(&instance->fields);
            FREE(ObjInstance, object);
            break;
        }
        case OBJ_NATIVE:
            FREE(ObjNative, object);
            break;
        case OBJ_BLOB: {
            ObjBlob* blob = (ObjBlob*)object;
            free(blob->blob);
            FREE(ObjBlob, object);
            break;
        }
        case OBJ_ROUTINE:
            FREE(ObjRoutine, object);
            break;
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
        case OBJ_UPVALUE:
            FREE(ObjUpvalue, object);
            break;
        case OBJ_CHANNEL:
            FREE(ObjChannel, object);
            break;
        case OBJ_VALARRAY: {
            ObjValArray* array = (ObjValArray*)object;
            freeValueArray(&array->array);
            FREE(ObjValArray, object);
            break;
        }
        case OBJ_UNIFORMARRAY: {
            ObjUniformArray* array = (ObjUniformArray*)object;
            reallocate(array->array, array->count * array->element_size, 0);
            array->array = NULL;    
            FREE(ObjUniformArray, object);
            break;
        }
        case OBJ_YARGTYPE: {
            FREE(ObjYargType, object);
            break;
        }
        case OBJ_STMT_EXPRESSION: FREE(ObjStmtExpression, object); break;
        case OBJ_STMT_PRINT: FREE(ObjStmtPrint, object); break;
        case OBJ_STMT_VARDECLARATION: FREE(ObjStmtVarDeclaration, object); break;
        case OBJ_STMT_BLOCK: FREE(ObjStmtBlock, object); break;
        case OBJ_STMT_IF: FREE(ObjStmtIf, object); break;
        case OBJ_EXPR_NUMBER: FREE(ObjExprNumber, object); break;
        case OBJ_EXPR_OPERATION: FREE(ObjExprOperation, object); break;
        case OBJ_EXPR_GROUPING: FREE(ObjExprGrouping, object); break;
        case OBJ_EXPR_NAMEDVARIABLE: FREE(ObjExprNamedVariable, object); break;
        case OBJ_EXPR_LITERAL: FREE(ObjExprLiteral, object); break;
        case OBJ_EXPR_STRING: FREE(ObjExprString, object); break;
    }
}

static void markRoots() {

    // Don't use markObject, as this is not on the heap.
    markRoutine(&vm.core0);
    
    markObject((Obj*)vm.core1);

    for (Value* slot = vm.tempRoots; slot < vm.tempRootsTop; slot++) {
        markValue(*slot);
    }

    markTable(&vm.globals);
    markAstCompilerRoots();
    markObject((Obj*)vm.initString);
}

static void traceReferences() {
    while (vm.grayCount > 0) {
        Obj* object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    }
}

static void sweep() {
    Obj* previous = NULL;
    Obj* object = vm.objects;
    while (object != NULL) {
        if (object->isMarked) {
            object->isMarked = false;
            previous = object;
            object = object->next;
        } else {
            Obj* unreached = object;
            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            } else {
                vm.objects = object;
            }

            freeObject(unreached);
        }
    }
}

void collectGarbage() {

    platform_mutex_enter(&vm.heap);

#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm.bytesAllocated;
#endif

    markRoots();
    traceReferences();
    tableRemoveWhite(&vm.strings);
    sweep();

    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
           before - vm.bytesAllocated, before, vm.bytesAllocated,
           vm.nextGC);
#endif

    platform_mutex_leave(&vm.heap);
}

void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }

    free(vm.grayStack);
}