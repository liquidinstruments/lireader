
SRCS := $(wildcard *.c)
OBJS := ${SRCS:.c=.o}
EXEC := liconvert

CFLAGS ?= -lm -lz -std=gnu99

$(EXEC): $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(CFLAGS)

all:	$(EXEC)
clean:
	rm $(OBJS) 2>/dev/null || exit 0
	rm $(EXEC) 2>/dev/null || exit 0

.PHONY: all clean
