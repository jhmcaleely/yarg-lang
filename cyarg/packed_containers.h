#ifndef cyarg_packed_containers_h
#define cyarg_packed_containers_h

#include "value.h"

// the underlying storage for pointers, structs and arrays (packed containers that contain potentially placed values)

typedef struct ObjPackedStorage ObjPackedStorage;

typedef struct ObjPackedContainer {
    Obj obj;
    void* loc;
} ObjPackedContainer;

typedef struct ObjOwningPackedContainer {
    ObjPackedContainer container;
    ObjPackedStorage* storage;
} ObjOwningPackedContainer;

#define AS_PACKEDCONTAINER(value) ((ObjPackedContainer*)AS_OBJ(value))

bool is_uniformarray(Value value);
bool is_pointer(Value value);
bool is_struct(Value value);
bool isUint32Pointer(Value val);

#define IS_UNIFORMARRAY(value) is_uniformarray(value)
#define IS_POINTER(value)      is_pointer(value)
#define IS_STRUCT(value)       is_struct(value)

typedef struct ObjPackedStorage {
    Obj obj;
    size_t storage_size;
    size_t value_elements;
    Value** values;
    uint8_t storage[];
} ObjPackedStorage;


ObjPackedStorage* newPackedStorage(const ObjConcreteYargType* type);
void MarkPackedStorage(ObjPackedStorage* storage);

Value newPackedValueContainer(const ObjConcreteYargType* type);
Value createPackedValueContainerAt(void* location, const ObjConcreteYargType* type);

Value pinPackedValueContainer(Value packedValueContainer);

void setDefaultPackedContainerValues(void* location, const ObjConcreteYargType* type);

Value defaultArrayValue(const ObjConcreteYargType* type);
Value defaultStructValue(const ObjConcreteYargType* type);
Value defaultPointerValue(const ObjConcreteYargType* type);

Value duplicateStruct(Value structVal);
Value duplicateArray(Value arrayVal);
Value duplicatePointer(Value pointer);

// Two useful coveniences - a byte array and a value array.
// byte arrays can create a buffer, or be placed over an existing memory location.
Value allocByteArray(size_t length);
Value createByteArrayAt(uint8_t* location, size_t length);
bool  isByteArray(Value value);
// value arrays can contain any value.
Value allocValueArray(size_t length);

Value arrayElement(Value array, size_t index);
Value pointerDestination(Value pointer);
Value structField(Value structVal, size_t fieldIndex);

Value arrayElementPointer(Value array, size_t index);
Value structFieldPointer(Value structVal, size_t fieldIndex);

bool setArrayElement(Value array, size_t index, Value value);
bool setStructField(Value structVal, size_t fieldIndex, Value value);
bool setPointerDestination(Value pointer, Value value);

void offsetPointerDestination(Value pointer, Value offset);

bool isArrayPointer(Value value);
bool isStructPointer(Value value);

bool arraysEqual(Value a, Value b);
bool structsEqual(Value a, Value b);
bool pointersEqual(Value a, Value b);

bool yt_packs_asValue(const ObjConcreteYargType* type);


#endif // cyarg_packed_containers_h