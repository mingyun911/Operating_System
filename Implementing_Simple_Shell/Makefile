TARGET	= mash
CFLAGS	= -g -c -D_POSIX_C_SOURCE -D_GNU_SOURCE -D_XOPEN_SOURCE=700
CFLAGS += -std=c99 -Wall -Wextra -Wno-unused-parameter -Werror
LDFLAGS	=

all: mash toy pipe

mash: pa1.o mash.o parser.o
	gcc $(LDFLAGS) $^ -o $@

toy: toy.o
	gcc $(LDFLAGS) $^ -o $@

pipe: pipe.o
	gcc $(LDFLAGS) $^ -o $@

%.o: %.c
	gcc $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf $(TARGET) toy pipe *.o *.dSYM


.PHONY: test-run
test-run: $(TARGET) toy testcases/test-run
	./$< -q < testcases/test-run

.PHONY: test-cd
test-cd: $(TARGET) testcases/test-cd
	./$< -q < testcases/test-cd

.PHONY: test-alias
test-alias: $(TARGET) testcases/test-alias
	./$< -q < testcases/test-alias

.PHONY: test-pipe
test-pipe: $(TARGET) pipe testcases/test-pipe
	./$< -q < testcases/test-pipe

.PHONY: test-combined
test-combined: $(TARGET) testcases/test-combined
	./$< -q < testcases/test-combined


.PHONY: test-all
test-all: test-run test-cd test-alias test-pipe test-combined
