default: all

CFLAGS+=     \
  -std=gnu11 \
  -O4        \

LDFLAGS+=   \
  -lpthread \
  -lc		\

INCLUDES=        \
  sharedMemory.h \
  canMessage.h   \

TARGETS=  \
  create  \
  write   \
  fetch   \

EXTRA_FILES=  \
  Makefile    \
  README.md   \


all:  $(TARGETS)

create: create.c $(INCLUDES)
	gcc $(CFLAGS) -o create create.c $(LDFLAGS)

write : write.c sharedMemory.c $(INCLUDES)
	gcc $(CFLAGS) -o write write.c sharedMemory.c $(LDFLAGS)

fetch : fetch.c sharedMemory.c $(INCLUDES)
	gcc $(CFLAGS) -o fetch fetch.c sharedMemory.c $(LDFLAGS)

tar:
	make all;                                              \
	tar -cvzf sviPrototype.tz *.c *.h $(EXTRA_FILES) $(TARGETS); \

clean:
	rm -f *.o *~ $(TARGETS) sviPrototype.tz
