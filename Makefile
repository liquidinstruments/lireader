
SRCS := $(wildcard *.c)
OBJS := ${SRCS:.c=.o}
EXEC := liconvert

CFLAGS ?= -lm -lz
EXTRA_CFLAGS ?= $(CFLAGS)

$(EXEC): $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(EXTRA_CFLAGS)

all:	$(EXEC)
clean:
	rm $(OBJS)
	rm $(EXEC)

.PHONY: all clean
