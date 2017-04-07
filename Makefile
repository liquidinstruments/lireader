
SRCS := $(wildcard *.c)
OBJS := ${SRCS:.c=.o}
EXEC := liconvert

CFLAGS += -lm -lz

$(EXEC): $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(CFLAGS)

all:	$(EXEC)
clean:
	rm $(OBJS)
	rm $(EXEC)

.PHONY: all clean
