#ifndef DCACHE_H
#define DCACHE_H

/*

	A very simple disk-cache for write-once data.
	Use case: working dataset exceeds memory size (and fast scratch disks are available)

	Supports store and load. 
	Does not currently support deleting or overwriting existing cache entries.

	Entries are identified both by a numeric id and a short string key (max 39 chars).
	Both the numeric and string IDs need to match for a load to work. So, if you only
	want numeric keys, just use "" for all string keys. If you only want string keys, 
	just use 0 for all numeric keys. You can also use both, but it is the unique 
	combination of a numeric key and a string key that will result in a hit on load.

	To use: 
	define DCACHE_IMPLEMENTATION in one .c file, before you include this header. 

	Error handling: 
	Errors are indicated through return codes. By default, error messages are also
	logged to stderr. If you want to provide a custom logging function, 
	define DCACHE_CUSTOM_LOGERROR and provide a printf-like function called 
	with signature logerror(fmt,...) before including this file, in the same place 
	you define DCACHE_IMPLEMENTATION. If you simply want to change what FILE* stream 
	the messages get printed to, you can define DCACHE_ERRORSTREAM instead.



*/

#ifndef DCACHE_API 
#define DCACHE_API 
#endif

#include <stddef.h>
#include <stdint.h>

/*
	Creates a new cache. A file path must be provided, but it's really just to 
	determine what drive the cache goes on. The actual path gets unlinked immediately,
	so it is deleted when the process exits (even if it exits uncleanly).

	max_entries is the maximum number of things that can be cached, which must be
	determined ahead of time.

	size is the maximum size in bytes that the cache can take up grow to. 

	Returns NULL on error.
*/
DCACHE_API void * 
dcache_mk (int max_entries, const char * path, size_t size);


/*
	Deletes an existing cache.
*/
DCACHE_API void
dcache_rm (void * cache);

/*
	Stores a value in the cache. 
	Returns 1 on success, 0 on failure.

*/
DCACHE_API int 
dcache_store (void * cache, uint64_t id, const char* key, const void * val, size_t valsz);

/*
	Loads an item from the cache.
	Returns 0 on failure, and the number of bytes corresponding to the value indicated by
	(id, key) on success.

	It is safe to call with null val pointer if you simple want to check the size of 
	the value.

*/
DCACHE_API size_t 
dcache_load (void * cache, uint64_t id, const char* key, void * val, size_t valsz);

#endif

#if defined(DCACHE_SELFTEST) && !defined(DCACHE_IMPLEMENTATION)
#define DCACHE_IMPLEMENTATION
#endif

#ifdef DCACHE_IMPLEMENTATION

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#ifndef DCACHE_ERRORSTREAM
#define DCACHE_ERRORSTREAM stderr
#endif

#ifndef DCACHE_CUSTOM_LOGERROR
static void 
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
logerror (char *fmt, ...)
{
	int e = errno;
	va_list args;
	va_start(args, fmt);
	vfprintf(DCACHE_ERRORSTREAM, fmt, args);
	va_end(args);
	if (e!= 0) fprintf(DCACHE_ERRORSTREAM, " (errno %d: %s)", e, strerror(e));
	fputc('\n', DCACHE_ERRORSTREAM);
}

#endif

typedef struct {

	uint64_t  id;
	char      name[40];
	uint64_t  sz;
	uint64_t  off;

} cache_entry;

typedef struct {
	int C;
	int N;
	int fd;
	uint64_t sz;
	uint64_t off;
	cache_entry entries[];
} disk_cache;

DCACHE_API void * 
dcache_mk (int max_entries, const char * path, size_t size)
{
	const size_t tblsz = sizeof(cache_entry)*max_entries;

	int fd = -1;

	if (-1 == (fd = open(path, O_RDWR|O_CREAT|O_EXCL))) {
		logerror("dcache_mk: open('%s')", path);
		return 0;
	}

	if (-1 == unlink(path)) {
		logerror("dcache_mk: unlink('%s')", path);
		close(fd);
		return 0;
	}

	if (-1 == ftruncate(fd, size)) {
		logerror("dcache_mk: ftruncate");
		close(fd);
		return 0;
	}

	disk_cache *c = calloc(1,sizeof(*c) + tblsz);
	if (!c) {
		logerror("dcache_mk: calloc(%zu)",sizeof(*c) + tblsz);
		close(fd);
		return 0;
	}

	*c = (disk_cache) {

		.C   = max_entries,
		.fd  = fd,
		.sz  = size,
	};

	return c;
}

DCACHE_API void
dcache_rm (void * cache) 
{
	if (!cache) {
		logerror("dcache_rm: NULL argument");
		return;
	}
	disk_cache *c = cache;
	close(c->fd);
	free(c);
}


static cache_entry *
lookup(disk_cache * c, uint64_t id, const char * key) 
{
	cache_entry * e = 0;
	
	for (int i = 0; i < c->N; i++) {

		if (id != c->entries[i].id) continue;
		if (strcmp(key, c->entries[i].name)) continue;

		e = & c->entries[i];
		break;

	}

	return e;
}


DCACHE_API int 
dcache_store (void * cache, uint64_t id, const char* key, const void * val, size_t valsz)
{

	disk_cache *c = cache;

	if (!c) {
		logerror("dcache_store: NULL argument");
		return 0;
	}

	if (strlen(val) +1 >= sizeof(c->entries[0].name)) {
		logerror("dcache_store: key too long");
		return 0;
	}
	
	if (c->N == c->C) {
		logerror("dcache_store: cache table full");
		return 0;
	}

	if (c->off + valsz >= c->sz) {
		logerror("dcache_store: cache file full");
		return 0;
	}

	if (lookup(c, id, key)) {
		logerror("dcache_store: entry %zu %s already in cache", (size_t) id, key);
		return 0;
	}

	if (valsz != pwrite(c->fd, val, valsz, c->off)) {
		logerror("dcache_store: pwrite(%zu bytes)", valsz);
		return 0;
	}

	cache_entry newent = {
			
		.sz   = valsz,
		.off  = c->off,			
		.id  = id,

	};

	snprintf (newent.name, sizeof(newent.name), "%s", key);

	c->entries[c->N++] = newent;
	c->off += valsz;

	return 1;
}

DCACHE_API size_t 
dcache_load (void * cache, uint64_t id, const char* key, void * val, size_t valsz) 
{
	disk_cache *c = cache;

	if (!c) {
		logerror("dcache_load: NULL argument");
		return 0;
	}

	cache_entry * e = lookup(c, id, key);
	
	if (!e) {
		logerror("dcache_load: entry %zu %s not found in cache", (size_t) id, key);
		return 0;
	}


	if (valsz >= e->sz && val) {
		if (e->sz != pread (c->fd, val, e->sz, e->off)) {
			logerror("dcache_load: pread(%zu bytes)", (size_t) e->sz);
			return 0;
		}
	}

	return e->sz;
}

#endif

#ifdef DCACHE_SELFTEST

typedef struct {
	uint64_t id;
	const char * key;
	void * val;
	size_t sz;
} test;

#include <assert.h>

int main(void)
{
	void * c = dcache_mk(100, "/tmp/scrap", 1<<25);

	if (!c) return 0;

	int x = 1;
	int y = 2;

	float z[] = {10.0f, 101.3f};

	dcache_store(c, 1, "x", &x, sizeof(x));
	dcache_store(c, 1, "x", &y, sizeof(y));
	dcache_store(c, 1, "y", &y, sizeof(y));
	dcache_store(c, 0, "y", &y, sizeof(y));
	dcache_store(c, 0, "z", &z, sizeof(z));

	memset(&x, 0xff, sizeof(x));
	memset(&y, 0xff, sizeof(y));
	memset(&z, 0xff, sizeof(z));

	assert(sizeof(x) == dcache_load(c, 1, "x", &x, sizeof(x)));
	printf("x: %i y: %i z: %f %f\n", x, y, z[0], z[1]);

	assert(sizeof(y) == dcache_load(c, 1, "y", &y, sizeof(y)));
	printf("x: %i y: %i z: %f %f\n", x, y, z[0], z[1]);

	assert(sizeof(y) == dcache_load(c, 0, "y", &y, sizeof(y)));
	printf("x: %i y: %i z: %f %f\n", x, y, z[0], z[1]);

	assert(sizeof(z) == dcache_load(c, 0, "z", &z, sizeof(z)));
	printf("x: %i y: %i z: %f %f\n", x, y, z[0], z[1]);

	dcache_load(c, 0, "", 0, 0);

	dcache_rm(c);

}

#endif 
