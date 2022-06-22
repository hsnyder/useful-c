/*
	A Fixed-size memory allocator.
	It is fixed-size in two senses. Both the chunks of memory it manages,
	and the size of the overall heap are fixed.

	Not thread safe, but you can have one allocator per thread.

	Limitations:
	- Max number of items is UINT_MAX

	No constructor function provided, you can fill out the struct yourself
	(also, you may want to use static memory, etc).

	All you need to do is:
       	- populate itemsize and maxitems.
	- make sure heap points to a chunk of memory with size at least itemsize*maxitems 
	- initialize other struct entries to null/zero

	Example:
	fs_allocator a = {
		.itemsize   = sizeof(mystruct),
		.maxitems = 100,
		.heap     = calloc(100, sizeof(mystruct)),
	};


	To use:
	#define FSALLOC_IMPLEMENTATION in one C file before you include this file.

	Options:
	#define FSALLOC_PARANOID for extra runtime checks
	#define NDEBUG for no runtime checks
*/

#ifndef FSALLOC_H
#define FSALLOC_H

#ifndef FSALLOC_API
#define FSALLOC_API 
#endif

typedef struct {

	// user sets these to chosen values
	unsigned  itemsize;
	unsigned  maxitems;
	void *    heap;

	// user sets these to null/zero
	unsigned  firstfree;
	unsigned  setup; 

} fs_allocator;

FSALLOC_API void *
fs_alloc(fs_allocator *a);

FSALLOC_API void  
fs_freeptr(fs_allocator *a, void *ptr);

#endif

#ifdef FSALLOC_IMPLEMENTATION

#include <assert.h>
#include <stddef.h>
#include <string.h>

FSALLOC_API void *
fs_alloc(fs_allocator *a) 
{
	assert(a->heap);
	assert(a->itemsize >= sizeof(unsigned));
	assert(a->firstfree <= a->maxitems);

	const size_t itemsize = a->itemsize;
	const size_t maxitems = a->maxitems;

	if (!a->setup) {
		for (size_t i = 0; i < maxitems; i++) {
			unsigned char * slot = a->heap;
			slot += i*itemsize;
			const unsigned next = i+1;
			memcpy(slot,&next,sizeof(next));
		}
		a->setup = 1;
	}

	if (a->firstfree == a->maxitems) return 0;

	unsigned char * heap = a->heap;
	
	unsigned idx = a->firstfree;
	unsigned nextfree = 0;
	memcpy(&nextfree, &heap[idx*itemsize], sizeof(nextfree));
	a->firstfree = nextfree;

	void * ptr = &heap[idx*itemsize];
	memset(ptr, 0, itemsize);
	return ptr;
}

FSALLOC_API void
fs_free(fs_allocator *a, void *item) 
{
	assert(a->heap);
	assert(a->itemsize >= sizeof(unsigned));
	assert(a->firstfree <= a->maxitems);
	assert(a->setup);

	const size_t itemsize = a->itemsize;
	const size_t maxitems = a->maxitems;

	unsigned char * ptr  = item;
	unsigned char * heap = a->heap;

	ptrdiff_t offset = ptr-heap;
	
	assert(offset >= 0);
	assert(offset < maxitems*itemsize);
	assert(offset % itemsize == 0);

#ifdef FSALLOC_PARANOID
	unsigned chk = a->firstfree;
	while (chk != a->maxitems) {
		assert (chk != offset/itemsize);
		memcpy(&chk, &heap[chk*itemsize],sizeof(chk));	
		assert(chk <= a->maxitems);
	}
#endif

	memset(item, 0, itemsize);
	memcpy(item, &a->firstfree, sizeof(a->firstfree));
	a->firstfree = offset/itemsize;
}

#endif


