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

#include <threads.h>

#define FSALLOC_IMPLEMENTATION
#include "fsalloc.h"
#include "die.h"

#include <stddef.h>

typedef struct {
	int       evtype;
	unsigned  next_idx;
} evlist_t;

typedef struct {

	mtx_t    m;
	cnd_t    c;

	fs_allocator  alloc_events;
	fs_allocator  alloc_evlist;

	unsigned nevents;
	unsigned evlist_head;
	evlist_t data[];

} evqueue_t;


EVQUEUE_API void* 
evqueue (unsigned maxitems, unsigned itemsize)
{

	const size_t sz_evlist = sizeof(evlist_t) * maxitems;
	const size_t sz_events = (size_t)itemsize * (size_t)maxitems;

	evqueue_t *q = malloc(sizeof(*q) + sz_evlist + sz_events);
	if (!q) return 0;

	*q = (evqueue_t) { 

		.alloc_events = { 
			.itemsize = itemsize,
			.maxitems = maxitems,
			.heap     = ((unsigned char*)q->data) + sz_evlist,
		}, 
		.alloc_evlist = {
			.itemsize = sizeof(evlist_t),
			.maxitems = maxitems,
			.heap     = q->data,
		}
	};

	xassert(thrd_success == mtx_init(&q->m, mtx_timed));	
	xassert(thrd_success == cnd_init(&q->c));	

	return q;
}

#include <time.h>

static struct timespec
get_deadline(int timeout_ms)
{
	struct timespec deadline;
	xassert(TIME_UTC == timespec_get(&deadline, TIME_UTC));
	if (timeout_ms > 0) {
		size_t ns = deadline.tv_nsec + (size_t)timeout_ms * (size_t)1000000;
		while (ns   > 1000000000) {
			deadline.tv_sec++;
			ns -= 1000000000;
		}
		deadline.tv_nsec = ns;
	}

	return deadline;

}

EVQUEUE_API unsigned
evqueue_putevents(void *queue, unsigned n, void *evs, int *types, int timeout_ms) 
{
	evqueue_t * q = queue;
	xassert(q);

	struct timespec deadline = get_deadline(timeout_ms);

	const size_t maxitems  = q->alloc_events.maxitems;
	const size_t evsz      = q->alloc_events.itemsize;
	const size_t sz_evlist = sizeof(evlist_t) * maxitems;
	unsigned char * input_events = evs;

	unsigned nwritten = 0;

	// try to acquire the lock. 
	if (timeout_ms < 0) {
		// infinite wait
		xassert(thrd_success == mtx_lock(&q->m));

		while (q->nevents == maxitems) 
			xassert(thrd_success == cnd_wait(&q->c, &q->m));

	} else if (timeout_ms == 0) {
		// do not wait
		int rc = mtx_trylock(&q->m);
		xassert(rc != thrd_error);
		if (rc == thrd_busy) return 0;

	} else {
		// specified wait time
		int rc = mtx_timedlock(&q->m, &deadline);
		xassert(rc == thrd_success || rc == thrd_timedout);
		if (rc == thrd_timedout) return 0;

		while (q->nevents == maxitems) {
			int rc = cnd_timedwait(&q->c, &q->m, &deadline);
			xassert(rc == thrd_success || rc == thrd_timedout);
			if (rc == thrd_timedout) {
				xassert(thrd_success == mtx_unlock(&q->m));
				return 0;
			}
		}
	}

	unsigned evlist_tail = q->evlist_head;
	for (unsigned i = 0; i < q->nevents; i++) 
		evlist_tail = q->data[evlist_tail].next_idx;

	while (nwritten < n && q->nevents < maxitems) {

		unsigned char * new_event  = fs_alloc(&q->alloc_events);
		evlist_t *      new_evlist = fs_alloc(&q->alloc_evlist);

		xassert(new_event);
		xassert(new_evlist);

		ptrdiff_t off = new_evlist - q->data;
		xassert(off >= 0);
		xassert(off < sz_evlist);

		unsigned newidx = off/sizeof(*new_evlist);

		if (q->nevents == 0) 
			q->evlist_head = newidx;
		else
			q->data[evlist_tail].next_idx = newidx;

		*new_evlist = (evlist_t) {
			.evtype = types[nwritten],
		};

		memcpy (new_event, input_events + evsz*nwritten, evsz);

		evlist_tail = newidx;
		nwritten++;
		q->nevents++;
	}

	xassert(thrd_success == mtx_unlock(&q->m));
	if (nwritten > 0) 
		xassert(thrd_success == cnd_broadcast(&q->c));
	return nwritten;


}

EVQUEUE_API unsigned
evqueue_getevents(void *queue, unsigned n, void *evs, int *types, unsigned nfilters, int *filters, int timeout_ms)
{
	evqueue_t * q = queue;
	xassert(q);

	struct timespec deadline = get_deadline(timeout_ms);

	const size_t maxitems  = q->alloc_events.maxitems;
	const size_t evsz      = q->alloc_events.itemsize;
	const size_t sz_evlist = sizeof(evlist_t) * maxitems;
	unsigned char * input_events = evs;

	unsigned char * output_events = evs;
	unsigned char * stored_events = ((unsigned char *) q->data) + sz_evlist;

	unsigned ngot = 0;

	// try to acquire the lock. 
	if (timeout_ms < 0) {
		// infinite wait
		xassert(thrd_success == mtx_lock(&q->m));

		while (!q->nevents) 
			xassert(thrd_success == cnd_wait(&q->c, &q->m));

	} else if (timeout_ms == 0) {
		// do not wait
		int rc = mtx_trylock(&q->m);
		xassert(rc != thrd_error);
		if (rc == thrd_busy) return 0;

	} else {
		// specified wait time
		int rc = mtx_timedlock(&q->m, &deadline);
		xassert(rc == thrd_success || rc == thrd_timedout);
		if (rc == thrd_timedout) return 0;

		while (!q->nevents) {
			int rc = cnd_timedwait(&q->c, &q->m, &deadline);
			xassert(rc == thrd_success || rc == thrd_timedout);
			if (rc == thrd_timedout) {
				xassert(thrd_success == mtx_unlock(&q->m));
				return 0;
			}
		}
	}

	unsigned nextev = q->evlist_head;
	long long prevev = -1;

	unsigned ntried = 0;
	while (ngot < n && ngot < q->nevents && ntried++ < q->nevents) {
		unsigned i = 0;
		for (; i < nfilters; i++) {
			if (q->data[nextev].evtype == filters[i]) {
				memcpy(output_events + ngot*evsz, stored_events + nextev*evsz, evsz);
				types[ngot] = filters[i];
				break;
			}
		}

		if (i == nfilters) {
			prevev = nextev;
			nextev = q->data[nextev].next_idx;
		} else {
			if (prevev == -1) 
				q->evlist_head = q->data[nextev].next_idx;
			else 
				q->data[prevev].next_idx = q->data[nextev].next_idx;

			fs_free(&q->alloc_evlist, &q->data[nextev]);
			fs_free(&q->alloc_events, stored_events + nextev*evsz);
			ngot++;
			q->nevents--;
		}
	}

	xassert(thrd_success == mtx_unlock(&q->m));

	if (ngot > 0) 
		xassert(thrd_success == cnd_broadcast(&q->c));
	
	return ngot;
}

EVQUEUE_API void
evqueue_free(void *queue)
{
	evqueue_t * q = queue;
	xassert(q);
	mtx_destroy(&q->m);	
	cnd_destroy(&q->c);	
	free(q);
}


#endif

