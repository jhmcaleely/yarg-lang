#ifndef VM_H
#define VM_H

typedef struct ObjYarg ObjYarg;
typedef struct ObjYargType ObjYargType;

ObjYarg* builtin_new(const ObjYargType* type);

#endif // VM_H