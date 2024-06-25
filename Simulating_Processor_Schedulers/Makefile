TARGET	= sched
CFLAGS	= -g -c -D_POSIX_C_SOURCE
CFLAGS += -std=c99 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Werror
CFLAGS += # Add your own cflags here if necessary
LDFLAGS	=

.PHONY: all
all: sched

sched: pa2.o parser.o sched.o
	gcc $(LDFLAGS) $^ -o $@

%.o: %.c
	gcc $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf $(TARGET) *.o *.dSYM
