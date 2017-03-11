#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define _UTHREAD_PRIVATE
#include "context.h"
//#include "palloc.h"
//#include "preempt.h"
#include "queue.h"
//#include "tls.h"
#include "uthread.h"

enum thread_state {
	READY,
	RUNNING,
	BLOCKED,
	ZOMBIE
};

queue_t ready;

struct uthread_tcb* current_thread;

struct uthread_tcb {
    int state;
    uthread_ctx_t context;
    void* tstack;
    //void* threadtls;
};

void schedule(void)
{
	struct uthread_tcb* prev; //create a temporary variable to hold thread
	prev = current_thread;
	queue_dequeue(ready,(void**) &current_thread); //remove the thread from waiting queue
	current_thread->state = RUNNING;
	uthread_ctx_switch(&prev->context, &current_thread->context);
}

void uthread_yield(void)
{
	current_thread->state = READY;
	queue_enqueue(ready, current_thread);
	schedule();
}


void uthread_exit(void)
{
	current_thread->state = ZOMBIE;
	//palloc_destroy();
	schedule();
}

void uthread_block(void)
{
}

void uthread_unblock(struct uthread_tcb *uthread)
{
}

void *uthread_get_tls(void)
{
	/* TODO: PART 2 - Phase 3 */
	/*
 * uthread_get_tls - Get TLS pointer of currently running thread
 *
 * Return: Pointer to current thread's TLS
 */
	//return current_thread->threadtls;
}

void uthread_set_tls(void *tls)
{
	/* TODO: PART 2 - Phase 3 */
	/*
 	* uthread_set_tls - Set TLS pointer for currently running thread
 	*/
 	//current_thread->threadtls = tls;
}

struct uthread_tcb *uthread_current(void)
{
	return current_thread;
}

int uthread_create(uthread_func_t func, void *arg)
{
	struct uthread_tcb* new_thread;

	new_thread = malloc(sizeof(struct uthread_tcb));
	new_thread->state = READY;
	new_thread->tstack = uthread_ctx_alloc_stack();
	uthread_ctx_init(&new_thread->context, new_thread->tstack, func, arg);

	queue_enqueue(ready, new_thread); //let new_thread be ready 
	return 0;
}


void uthread_start(uthread_func_t start, void *arg)
{
	ready = queue_create();
	struct uthread_tcb* idleThread;
	idleThread = malloc(sizeof(struct uthread_tcb));
	idleThread->state = RUNNING;

	current_thread = idleThread;

	if(uthread_create(start,arg) != 0) {
		printf("Error\n");
		exit(1);
	}
	
	while(1) {
		if(queue_length(ready) == 0) {
			//palloc_destroy();
			break;
		}
		uthread_yield();
	}
}

void uthread_mem_config(size_t npages)
{
	//palloc_configure(npages);
}
