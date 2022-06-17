#ifndef QUEUE_H
#define QUEUE_H

#ifndef QUEUE_API
#define QUEUE_API 
#endif

#include <threads.h>

typedef struct {

	mtx_t           m;
	cnd_t           c;
	unsigned short  r, w, sz;

} queue;

QUEUE_API void
queue_init (queue *q, unsigned short sz);

QUEUE_API void
queue_destroy (queue *q);

QUEUE_API unsigned
queue_begin_put (queue *q);

QUEUE_API void
queue_commit_put (queue *q);

QUEUE_API unsigned
queue_begin_get (queue *q);

QUEUE_API void
queue_commit_get (queue *q);

#endif

#if defined(QUEUE_SELFTEST) && !defined(QUEUE_IMPLEMENTATION) 
#define QUEUE_IMPLEMENTATION
#endif 

#ifdef QUEUE_IMPLEMENTATION

#include "die.h"

QUEUE_API void
queue_init (queue *q, unsigned short sz) {
	*q = (queue) {.sz = sz};
	if (thrd_success != mtx_init(&q->m, mtx_plain )) die("queue_init: mtx_init");
	if (thrd_success != cnd_init(&q->c)) die("queue_init: cnd_init");
}

QUEUE_API void
queue_destroy (queue *q) {
	mtx_destroy(&q->m);
	cnd_destroy(&q->c);
	*q = (queue) {};
}

static inline int 
q_full (queue *q) {
	return (q->w + 1) % q->sz == q->r;
}

static inline int
q_empty (queue *q) {
	return q->r == q->w;
}

QUEUE_API unsigned
queue_begin_put (queue *q)
{
	if (thrd_success != mtx_lock(&q->m)) die("queue_begin_put: mtx_lock");

	while (q_full(q)) {
		if (thrd_success != cnd_wait(&q->c, &q->m)) die("queue_begin_put: cnd_wait");
	}

	return q->w;
}

QUEUE_API void
queue_commit_put (queue *q) 
{

	int wakethds = q_empty(q);

	q->w = (q->w + 1) % q->sz;

	if (thrd_success != mtx_unlock(&q->m)) die("queue_commit_put: mtx_unlock");
	
	if (wakethds) {
		if(thrd_success != cnd_broadcast(&q->c)) die("queue_commit_put: cnd_broadcast");
	}
}

QUEUE_API unsigned
queue_begin_get (queue *q) 
{
	if (thrd_success != mtx_lock(&q->m)) die("queue_begin_get: mtx_lock");

	while (q_empty(q)) {
		if (thrd_success != cnd_wait(&q->c, &q->m)) die("queue_begin_get: cnd_wait");
	}

	return q->r;
}

QUEUE_API void
queue_commit_get (queue *q)
{
	int wakethds = q_full(q);

	q->r = (q->r + 1) % q->sz;

	if (thrd_success != mtx_unlock(&q->m)) die("queue_commit_get: mtx_unlock");
	
	if (wakethds) {
		if(thrd_success != cnd_broadcast(&q->c)) die("queue_commit_get: cnd_broadcast");
	}
}

#endif

#ifdef QUEUE_SELFTEST

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static long 
randu (unsigned long *randus) { 
	return (*randus *= 0x10003) & 0x7fffffff; 
}


float entries[10] = {};
queue q = {};

int producer (void * nothing) {
	unsigned long *_tid = nothing;
	unsigned long tid = *_tid;
	unsigned long _r = tid;


	for (int i = 0; i < 10; i++) {

		float f = randu(&_r);

		unsigned u = queue_begin_put(&q);
		printf("Thread %lu generating %f at slot %u\n", tid, f, u);
		entries[u] = f;
		queue_commit_put(&q);
		usleep(randu(&_r) % 10000000);

	}

	return 0;
}

int consumer (void * nothing) {
	unsigned long *_tid = nothing;
	unsigned long tid = *_tid;
	unsigned long _r = tid;


	while(1) {


		unsigned u = queue_begin_get(&q);
		printf("Thread %lu got %f at slot %u\n", tid, entries[u], u);
		queue_commit_get(&q);
		usleep(randu(&_r) % 10000000);

	}

	return 0;
}

int main (void) {

	queue_init(&q, 10);

	thrd_t ts[6];
	unsigned long ls[6];

	for (int i = 0; i < 6; i++) {
		ls[i] = i;
		thrd_create(ts+i, i < 2 ? producer : consumer, ls+i);
	}


	sleep(60);
	_exit(EXIT_SUCCESS);
}

#endif
