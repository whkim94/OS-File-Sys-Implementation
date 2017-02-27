CC := gcc
CFLAGS := -Werror
CFLAGS := -g

targets := libuthread.a
objs := queue.o context.o uthread.o semaphore.o preempt.o bitmap.o palloc.o tls.o

all: $(targets)

libuthread.a: $(objs)
	@echo "AR $@"
	@ar rcs libuthread.a $^

%.o: %.c %.h
	@echo "CC $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

clean :
	@echo "CLEAN"
	@rm -f $(targets) $(objs)
