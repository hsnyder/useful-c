#ifndef EVQUEUE_H
#define EVQUEUE_H

#ifndef EVQUEUE_API
#define EVQUEUE_API
#endif
EVQUEUE_API void* 
evqueue (unsigned maxitems, unsigned itemsize) ;

EVQUEUE_API unsigned
evqueue_putevents(void *queue, unsigned n, void *evs, int *types, int timeout_ms);

EVQUEUE_API unsigned
evqueue_getevents(void *queue, unsigned n, void *evs, int *types, unsigned nfilters, int *filters, int timeout_ms);

EVQUEUE_API void
evqueue_free(void *queue);

#endif

#ifdef EVQUEUE_IMPLEMENTATION

#include <pthread.h>
#include <stddef.h>
#include <time.h>
#include "die.h"

typedef struct {

	pthread_mutex_t    m;
	pthread_cond_t     c;

	size_t   itemsize;
	size_t   maxitems;
	size_t   sz_typelist;
	size_t   sz_events;

	unsigned nevents;
	int      data[];

} evqueue_t;


EVQUEUE_API void* 
evqueue (unsigned maxitems, unsigned itemsize)
{

	const size_t sz_typelist = sizeof(int) * maxitems;
	const size_t sz_events   = (size_t)itemsize * (size_t)maxitems;

	evqueue_t *q = malloc(sizeof(*q) + sz_typelist + sz_events);
	if (!q) return 0;

	*q = (evqueue_t) { 
		.itemsize    = itemsize,
		.maxitems    = maxitems,
		.sz_typelist = sz_typelist,
		.sz_events   = sz_events,
	};

	pthread_mutexattr_t a;
	xassert(0 == pthread_mutexattr_init(&a));
	xassert(0 == pthread_mutexattr_settype(&a, PTHREAD_MUTEX_ERRORCHECK));
	xassert(0 == pthread_mutex_init(&q->m, &a));

	pthread_condattr_t b;
	xassert(0 == pthread_condattr_init(&b));
	xassert(0 == pthread_cond_init(&q->c, &b));	

	return q;
}

static struct timespec
time_now(void)
{
	struct timespec now;
	xassert(TIME_UTC == timespec_get(&now, TIME_UTC));
	return now;
}

static struct timespec
get_deadline(int timeout_ms)
{
	struct timespec deadline = time_now();
	if (timeout_ms > 0) {
		size_t ns = deadline.tv_nsec + (size_t)timeout_ms * (size_t)1000000;
		while (ns  >= 1000000000) {
			deadline.tv_sec++;
			ns -= 1000000000;
		}
		deadline.tv_nsec = ns;
	}

	return deadline;
}



static int deadline_hit(struct timespec deadline) {
	struct timespec now = time_now();	
	if (now.tv_sec - deadline.tv_sec > 0) return 1; 
	if (now.tv_sec - deadline.tv_sec == 0 && now.tv_nsec - deadline.tv_nsec > 0) return 1;
	return 0;
}

EVQUEUE_API unsigned
evqueue_putevents(void *queue, unsigned n, void *evs, int *types, int timeout_ms) 
{
	evqueue_t * q = queue;
	xassert(q);

	struct timespec deadline = get_deadline(timeout_ms);

	unsigned char * input_events  = evs;
	unsigned char * event_storage = ((unsigned char *)q->data) + q->sz_typelist;

	unsigned nwritten = 0;

	do {	

		// try to acquire the lock. 
		if (timeout_ms < 0) {
			// infinite wait
			int rc = pthread_mutex_lock(&q->m);
			xassert(0 == rc);

			while (q->nevents == q->maxitems) 
				xassert(0 == pthread_cond_wait(&q->c, &q->m));

		} else if (timeout_ms == 0) {
			// do not wait
			int rc = pthread_mutex_trylock(&q->m);
			xassert(rc == 0 || rc == EBUSY);
			if (rc == EBUSY) return 0;

		} else {
			// specified wait time
			int rc = pthread_mutex_timedlock(&q->m, &deadline);
			xassert(rc == 0 || rc == ETIMEDOUT);
			if (rc == ETIMEDOUT) return 0;

			while (q->nevents == q->maxitems) {
				int rc = pthread_cond_timedwait(&q->c, &q->m, &deadline);
				xassert(rc == 0 || rc == ETIMEDOUT);
				if (rc == ETIMEDOUT) {
					xassert(0 == pthread_mutex_unlock(&q->m));
					return 0;
				}
			}
		}

		while (nwritten < n && q->nevents < q->maxitems) {

			q->data[q->nevents] = types[nwritten];

			unsigned char * dst = event_storage + q->nevents * q->itemsize;
			unsigned char * src = input_events  +   nwritten * q->itemsize;
			memcpy(dst,src,q->itemsize);

			nwritten++;
			q->nevents++;
		}

		xassert(0 == pthread_mutex_unlock(&q->m));
		if (nwritten > 0) 
			xassert(0 == pthread_cond_broadcast(&q->c));

	} while (nwritten < n && (timeout_ms < 0 || !deadline_hit(deadline)));

	return nwritten;


}

EVQUEUE_API unsigned
evqueue_getevents(void *queue, unsigned n, void *evs, int *types, unsigned nfilters, int *filters, int timeout_ms)
{
	evqueue_t * q = queue;
	xassert(q);

	struct timespec deadline = get_deadline(timeout_ms);

	unsigned char * output_events = evs;
	unsigned char * event_storage = ((unsigned char *)q->data) + q->sz_typelist;

	unsigned ngot = 0;

	do {

		// try to acquire the lock. 
		if (timeout_ms < 0) {
			// infinite wait
			int rc = pthread_mutex_lock(&q->m);
			xassert(0 == rc);

			while (!q->nevents) 
				xassert(0 == pthread_cond_wait(&q->c, &q->m));

		} else if (timeout_ms == 0) {
			// do not wait
			int rc = pthread_mutex_trylock(&q->m);
			xassert(rc == 0 || rc == EBUSY);
			if (rc == EBUSY) return 0;

		} else {
			// specified wait time
			int rc = pthread_mutex_timedlock(&q->m, &deadline);
			xassert(rc == 0 || rc == ETIMEDOUT);
			if (rc == ETIMEDOUT) return 0;

			while (!q->nevents) {
				int rc = pthread_cond_timedwait(&q->c, &q->m, &deadline);
				xassert(rc == 0 || rc == ETIMEDOUT);
				if (rc == ETIMEDOUT) {
					xassert(0 == pthread_mutex_unlock(&q->m));
					return 0;
				}
			}
		}


		for (unsigned i = 0; i < q->nevents && ngot < n; i++) {
			for (unsigned f = 0; f < nfilters; f++) {
				if (q->data[f] == filters[f]) {

					unsigned char * ourcopy   = event_storage +    i * q->itemsize;
					unsigned char * theircopy = output_events + ngot * q->itemsize;

					types[ngot] = filters[f];
					memcpy(theircopy, ourcopy, q->itemsize);

					// compress lists
					memmove(&q->data[i], &q->data[i+1], (q->maxitems-i) * sizeof(q->data[0]));
					memmove(ourcopy, ourcopy+q->itemsize, (q->maxitems-i) * q->itemsize);

					ngot++;
					q->nevents--;
					i--; // since we're decrementing q->nevents
					break;
				}
			}
		}

		xassert(0 == pthread_mutex_unlock(&q->m));
		if (ngot > 0) 
			xassert(0 == pthread_cond_broadcast(&q->c));

	} while (ngot < 1 && (timeout_ms < 0 || !deadline_hit(deadline)));

	return ngot;
}

EVQUEUE_API void
evqueue_free(void *queue)
{
	evqueue_t * q = queue;
	xassert(q);
	xassert(0 == pthread_mutex_destroy(&q->m));
	xassert(0 == pthread_cond_destroy(&q->c));
	free(q);
}


#endif

