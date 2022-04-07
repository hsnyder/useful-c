#pragma once
#include <stdlib.h>
#include <complex.h>
#include <stdbool.h>
#include <stdio.h>

#include "dict_config.h"


/* 
	Check adjustable parameters from dict_config.h
*/
#ifndef EXTRA_DICT_TYPES
#define EXTRA_DICT_TYPES(X)
#endif 

#ifndef DICT_KEY_ERROR_CALLBACK
#define DICT_KEY_ERROR_CALLBACK 0
#endif

#ifndef DICT_MAX_SHORTKEY 
#define DICT_MAX_SHORTKEY 35
#endif




#ifdef DICT_SELF_TEST
typedef struct exttyp {int x; int y;} exttyp;
static const char * 
repr_exttyp(exttyp x) {
	static char buf[128];
	snprintf(buf,128,"{%i %i}", x.x, x.y);
	return buf;
}
#undef EXTRA_DICT_TYPES
#define EXTRA_DICT_TYPES(X) \
	X(T_MYEXT,   exttyp ,     ext,   "%s", repr_exttyp   ) 
#endif




#define CONCAT2(a,b) a ## b
#define CONCAT(a,b) CONCAT2(a,b)
typedef float complex       cfloat;
typedef double complex      cdouble;
typedef long double         ldouble;
typedef signed char         schar;
typedef long long           llong;
typedef unsigned char       uchar;
typedef unsigned short      ushort;
typedef unsigned long       ulong;
typedef unsigned long long  ullong;

typedef float*               floatptr;
typedef double*              doubleptr;
typedef short*               shortptr;
typedef int*                 intptr;
typedef long*                longptr;
typedef unsigned*            uintptr;

typedef float complex*       cfloatptr;
typedef double complex*      cdoubleptr;
typedef long double*         ldoubleptr;
typedef signed char*         scharptr;
typedef long long*           llongptr;
typedef unsigned char*       ucharptr;
typedef unsigned short*      ushortptr;
typedef unsigned long*       ulongptr;
typedef unsigned long long*  ullongptr;


#define TYPELIST(X) \
	X(T_FLOAT,       float,       flt,          "%g",    )              \
	X(T_DOUBLE,      double,      dbl,          "%g",    )              \
	X(T_LDOUBLE,     ldouble,     ldbl,         "%Lg",   )              \
	X(T_CFLOAT,      cfloat,      cflt,         "%s",    repr_cfloat)   \
	X(T_CDOUBLE,     cdouble,     cdbl,         "%s",    repr_cdouble)  \
	X(T_SCHAR,       schar,       c,            "%hhi",  )              \
	X(T_SHORT,       short,       s,            "%hi",   )              \
	X(T_INT,         int,         i,            "%i",    )              \
	X(T_LONG,        long,        l,            "%li",   )              \
	X(T_LLONG,       llong,       ll,           "%lli",  )              \
	X(T_UCHAR,       uchar,       uc,           "%hhu",  )              \
	X(T_USHORT,      ushort,      us,           "%hu",   )              \
	X(T_UINT,        unsigned,    ui,           "%u",    )              \
	X(T_ULONG,       ulong,       ul,           "%lu",   )              \
	X(T_ULLONG,      ullong,      ull,          "%llu",  )              \
	X(T_FLOATPTR,    floatptr,    mfloatptr,    "%p",    )              \
	X(T_DOUBLEPTR,   doubleptr,   mdoubleptr,   "%p",    )              \
	X(T_SHORTPTR,    shortptr,    mshortptr,    "%p",    )              \
	X(T_INTPTR,      intptr,      mintptr,      "%p",    )              \
	X(T_LONGPTR,     longptr,     mlongptr,     "%p",    )              \
	X(T_UINTPTR,     uintptr,     muintptr,     "%p",    )              \
	X(T_CFLOATPTR,   cfloatptr,   mcfloatptr,   "%p",    )              \
	X(T_CDOUBLEPTR,  cdoubleptr,  mcdoubleptr,  "%p",    )              \
	X(T_LDOUBLEPTR,  ldoubleptr,  mldoubleptr,  "%p",    )              \
	X(T_SCHARPTR,    scharptr,    mscharptr,    "%p",    )              \
	X(T_LLONGPTR,    llongptr,    mllongptr,    "%p",    )              \
	X(T_UCHARPTR,    ucharptr,    mucharptr,    "%p",    )              \
	X(T_USHORTPTR,   ushortptr,   mushortptr,   "%p",    )              \
	X(T_ULONGPTR,    ulongptr,    mulongptr,    "%p",    )              \
	X(T_ULLONGPTR,   ullongptr,   mullongptr,   "%p",    )              \
	EXTRA_DICT_TYPES(X)

/*
	<API>
*/

int    mkdict (void);
void   rmdict (int handle);
void   dict_clear_key (int handle, const char * key);

const char * repr_cfloat(float complex z);
const char * repr_cdouble(float complex z);
void   dict_dump      (int handle, FILE* where);

#define X(typesymbol,ctype,varname,_a,_b)  \
void CONCAT(dict_set_,ctype)(int handle, const char * key, ctype x) ;
TYPELIST(X)
#undef X

#define X(typesymbol,ctype,varname,_a,_b)  \
bool CONCAT(dict_get_,ctype)(int handle, const char * key, ctype * val) ;
TYPELIST(X)
#undef X


/*
	</API>
*/


/* 
	ugly hack alert: to make the X-Macros work with generic we need to add something after the last comma
	so create a useless type that nobody should use just so we're syntactically legal
*/
typedef struct uselesstype {int x;} uselesstype;
static void DICT_TYPE_NOT_SUPPORTED(uselesstype x){(void)x;abort();}


#define MACRO_GENERIC_DICT_SET(_a,ctype,_b,_c,_d) ctype: CONCAT(dict_set_,ctype), 
#define dict_set(handle, key, val) _Generic((val), TYPELIST(MACRO_GENERIC_DICT_SET) uselesstype: DICT_TYPE_NOT_SUPPORTED)(handle, key,val)

#define MACRO_GENERIC_DICT_GET(_a,ctype,_b,_c,_d) ctype: CONCAT(dict_get_,ctype), 
#define dict_get(handle, key, val) _Generic((val), TYPELIST(MACRO_GENERIC_DICT_GET) uselesstype: DICT_TYPE_NOT_SUPPORTED)(handle, key, val)

/*
	IMPLEMENTATION  -------------------------------------------------------------------------------
*/


#ifdef DICT_IMPLEMENTATION

#include <string.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

enum dicttype {
#define X(typesymbol,ctype,varname,_a,_b) typesymbol,
TYPELIST(X)
#undef X
};

/*
	For scalar entries in the dict, we just store the value directly, as seen in struct dictentry.
	This union is just a way to avoid wasting memory. 
*/
union dictval {
#define X(typesymbol,ctype,varname,_a,_b) ctype varname;
TYPELIST(X)
#undef X
};

/*
	A single entry in our dictionary. 
	Stores the key, along with the datatype indicator, plus the actual value.
	The key is "small size optimized" meaning there's no memory allocation if it's smaller than DICT_MAX_SHORTKEY
*/
struct dictentry {
	char            key[DICT_MAX_SHORTKEY+1]; 
	enum dicttype     type;
	char            *longkey;
	union dictval     val;
};


static const char *
helper_get_key(const struct dictentry * de)
{
	return de->longkey ? de->longkey : de->key;
}

/*
	Comparison function to compare keys of dictentry structs.
	API compatible with qsort and bsearch.
static int 
cmp_dictentry(const void * p1, const void * p2) {
	const struct dictentry * dicten1 = p1;
	const struct dictentry * dicten2 = p2;
	const char * str1 = helper_get_key(dicten1);
	const char * str2 = helper_get_key(dicten2);
	return strcmp(str1, str2);
}
*/

/*
	Represents an entire dictionary.
	Capacity is how much memory is allocated.
	N_entries is how many entries there actually are. 
*/
struct dict {
	size_t          capacity;
	size_t          n_entries;
	struct dictentry *entries;
};


/*
	Fixed number of actual dictionary objects. 
	If you need more than 4096 in one process... probably doing it wrong

*/
#define MAX_DICTS 4096 
bool dict_inuse[MAX_DICTS] = {};
struct dict dicts[MAX_DICTS] = {}; 


/* 
	Place a debug break point in here if needed.
*/
void debug_invalid_handle(int handle) {
	fprintf(stderr, "Warning: dict passed nonexistent handle %i\n",handle);
}

/*
	Make sure that a handle provided actually exists
*/
bool handle_check(int handle) 
{
	if (handle >= 0 && handle < MAX_DICTS && dict_inuse[handle]) 
		return true;
	debug_invalid_handle(handle);
	return false;
}

_Static_assert(sizeof *dicts[0].entries == sizeof (struct dictentry), "Sanity check failed: clearly I don't know how C works.");
_Static_assert(sizeof dicts[0].entries[0] == sizeof (struct dictentry), "Sanity check failed: clearly I don't know how C works.");

int mkdict(void)
{
	for (int i = 0; i < MAX_DICTS; i++) {
		if (!dict_inuse[i]) {
			dict_inuse[i] = true;
			return i;
		}
	}
	return -1;
}

static void 
helper_dict_free_all_longkeys(int handle)
{
	for (size_t i = 0; i < dicts[handle].n_entries; i++) {
		struct dictentry * entry = &(dicts[handle].entries[i]);
		if(entry->longkey){
			free (entry->longkey);
			entry->longkey = 0;
		}
	}
		
}

void rmdict(int handle)
{
	if (!handle_check(handle)) return;
	helper_dict_free_all_longkeys(handle);
	free(dicts[handle].entries);
	dicts[handle] = (struct dict){};
	dict_inuse[handle] = false;
}

/*
	Look up a key and return a pointer to the entry
	NULL result means not found.
*/
struct dictentry * 
dict_lookup(int handle, const char * key)
{
	for (size_t i = 0; i < dicts[handle].n_entries; i++) {
		const char * cmpkey = helper_get_key(&dicts[handle].entries[i]);
		if(!strcmp(key,cmpkey))	return &dicts[handle].entries[i];
	}

	return 0;
}

/*
	See if we still have enough free space allocated to add one element to the dict.
	If not, reallocate to make more space.
*/
static void 
dict_grow_if_needed(int handle)
{
	if (dicts[handle].n_entries == dicts[handle].capacity) {
		size_t newcap = MAX(512, 2*dicts[handle].capacity);
		dicts[handle].entries = realloc(dicts[handle].entries, newcap * sizeof dicts[handle].entries[0]);
		if (!dicts[handle].entries) {
			perror ("dict_grow_if_needed");
			exit   (EXIT_FAILURE);
		}
		dicts[handle].capacity = newcap;
	}
}


#define X(typesymbol,ctype,varname,_a,_b)  \
void CONCAT(dict_set_,ctype)(int handle, const char * key, ctype x) \
{                                                      \
	if (!handle_check(handle)) return;             \
	struct dictentry * entry = dict_lookup(handle,key);   \
	if (!entry) {                                  \
		dict_grow_if_needed(handle);               \
	        entry = dicts[handle].entries + dicts[handle].n_entries++; \
	}                                              \
	*entry = (struct dictentry) { .type = typesymbol, .val.varname = x }; \
	if(strlen(key) > DICT_MAX_SHORTKEY) {                                     \
		entry->longkey = strdup(key);                                \
	} else {                                                             \
		memcpy(entry->key, key, MIN(DICT_MAX_SHORTKEY,strlen(key)));      \
	}                                                                    \
}
TYPELIST(X)
#undef X


typedef void (*dict_key_error_callback)(const char * notfound_key, int dict_handle, const char * type_name);

dict_key_error_callback keyerror_callback = DICT_KEY_ERROR_CALLBACK;

#define X(typesymbol,ctype,varname,_a,_b)  \
bool CONCAT(dict_get_,ctype)(int handle, const char * key, ctype * val) \
{                                                      \
	if (!handle_check(handle)) { \
		if(keyerror_callback)keyerror_callback(key, handle, #typesymbol ); \
		return false;       \
	}                                 \
	struct dictentry * entry = dict_lookup(handle,key);   \
	if (entry && entry->type == typesymbol) {        \
		*val = entry->val.varname;      \
		return true;		               \
	}                                              \
	return false;                                  \
}
TYPELIST(X)
#undef X


void dict_clear_key (int handle, const char * key)
{
	if (!handle_check(handle)) return;
	struct dictentry * entry = dict_lookup(handle,key);
	if (entry) {
		if(entry->longkey) {
			free(entry->longkey);
			entry->longkey = 0;
		}
		struct dictentry * lastentry = &dicts[handle].entries[--dicts[handle].n_entries];
		*entry = *lastentry;
	}
}

const char * repr_cfloat(float complex z)
{
	static char repr[512];
	snprintf(repr, 512, "%g%+gi", crealf(z), cimagf(z));
	return repr;
}

const char * repr_cdouble(float complex z)
{
	static char repr[512];
	snprintf(repr, 512, "%g%+gi", creal(z), cimag(z));
	return repr;
}

void helper_scalar_printf(const struct dictentry * entry, FILE * where)
{
	#define X(typesymbol,ctype,varname,pf,repr) else if (entry->type == typesymbol) { fprintf(where, pf, repr(entry->val.varname) );} 

	if(0){}
	TYPELIST(X)
	else {
		fprintf(where, "[UNPRINTABLE]");
	}
	#undef X

}


const char * helper_type_tostring(enum dicttype t)
{
	#define X(typesymbol,_a,_b,_c,_d) if(t == typesymbol) return #typesymbol ;
	TYPELIST(X)
	#undef X
	return "[INVAL]";
}


void dict_dump (int handle, FILE * where)
{
	if (!handle_check(handle)) return ;
	for (size_t i = 0; i < dicts[handle].n_entries; i++)
	{
		const struct dictentry * entry = &(dicts[handle].entries[i]);
		const char * key = helper_get_key(entry);
		char is_longkey = key == entry->key ? ' ' : '*';
		fprintf(where, "%c   %s (%s): ", is_longkey, key, helper_type_tostring(entry->type));
		helper_scalar_printf(entry, where);
		fprintf(where, "\n");
	}
}


/*
	SELF TEST -------------------------------------------------------------------------------
*/

#ifdef DICT_SELF_TEST


int main (void) 
{
	printf("size of struct dictentry on this platform: %zu\n", sizeof(struct dictentry));
	printf("size of union dictval on this platform: %zu\n", sizeof(union dictval));

	int d = mkdict();

	dict_set(d, "int thing", 14);
	dict_set(d, "another int thing", 16);
	dict_set(d, "complex thing", CMPLX(1.0, 9.9) );
	dict_set(d, "very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long key thing", 19ULL);

	dict_dump(d, stdout);

	dict_clear_key(d, "int thing");
	
	printf("\n");

	dict_dump(d, stdout);

	rmdict(d);

	printf("\n");
	d = mkdict();
	const exttyp  myx = {3,4};
	dict_set(d, "extension", myx);
	dict_dump(d,stdout);
	rmdict(d);

}


#endif
#endif

