CC := gcc
CFLAGS := -Werror
CFLAGS := -g

targets := libuthread.a
objs := queue.o context.o uthread.o disk.o fs.o

all: $(targets)

deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)

libuthread.a: $(objs)
	@echo "AR $@"
	@ar rcs libuthread.a $^

%.o: %.c %.h
	@echo "CC $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

clean :
	@echo "CLEAN"
	@rm -f $(targets) $(objs)
