sources=value-zoo.c value.c placed-value.c object.c yargtype.c yargobject.c yarg-runtime.c
headers=object.h placed-value.h value.h yargtype.h yargobject.h yarg-runtime.h
CFLAGS ?= -g -O0

a.out: $(sources) $(headers)
	$(CC) $(CFLAGS) $(sources) -o a.out