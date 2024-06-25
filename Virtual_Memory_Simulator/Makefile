TARGET	= vm
CFLAGS	= -g -c -D_POSIX_C_SOURCE -D_GNU_SOURCE
CFLAGS += -std=c99 -Wimplicit-function-declaration -Werror
CFLAGS += # Add your own cflags here if necessary

LDFLAGS	=

.PHONY: all
all: vm

vm: vm.o parser.o pa3.o
	gcc $^ -o $@ $(LDFLAGS)

%.o: %.c
	gcc $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf $(TARGET) *.o *.dSYM
