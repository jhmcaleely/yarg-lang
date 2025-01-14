#include "common.h"

#include "channel.h"
#include "memory.h"

ObjChannel* newChannel() {
    ObjChannel* channel = ALLOCATE_OBJ(ObjChannel, OBJ_CHANNEL);
    channel->data = NIL_VAL;
    channel->present = false;
    channel->overflow = false;
    return channel;
}