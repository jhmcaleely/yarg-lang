#ifndef cyarg_object_gc_impl_h
#define cyarg_object_gc_impl_h

#include "value.h"
#include "object.h"

// API for the memory management system within the VM
void gc_objects(size_t size_requested);

// API for implementers of gc functionality within the VM
void gc_free(void* ptr);
void markValue(Value value);
void markObject(Obj* object);

// functions that are internal to heap management & gc operations.
void blackenObject(Obj* object);
void freeObject(Obj* object);

void markVMRoots();
void markCompilerRoots();
void vmRemoveUnmarkedRoots();

typedef struct ObjSyncGroup ObjSyncGroup;
void markSyncGroup(ObjSyncGroup* group);
void gc_freeSyncGroup(ObjSyncGroup* group);

typedef struct ObjChannelContainer ObjChannelContainer;
void markChannel(ObjChannelContainer* channel);
void gc_freeChannelObject(ObjChannelContainer* channel);

typedef struct ObjScanner ObjScanner;
void markScanner(ObjScanner* scanner);

typedef struct ObjParser ObjParser;
void markParser(ObjParser* parser);
void gc_freeParser(ObjParser* parser);

typedef struct ObjRoutine ObjRoutine;
void markRoutine(ObjRoutine* stack);

typedef struct ObjPackedStorage ObjPackedStorage;
void markPackedStorage(ObjPackedStorage* storage);
void gc_freePackedStorage(ObjPackedStorage* storage);

typedef struct DynamicObjArray DynamicObjArray;
void markDynamicObjArray(DynamicObjArray* array);
void gc_freeDynamicObjArray(DynamicObjArray* array);

typedef struct ValueCell ValueCell;
void markValueCell(ValueCell* value);

typedef struct ObjFunction ObjFunction;
void markFunction(ObjFunction* function);

#endif // cyarg_object_gc_impl_h
