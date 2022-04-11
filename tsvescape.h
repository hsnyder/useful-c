#include <assert.h>
#include <string.h>

void 
tsv_escape_inplace(char * str, size_t capacity)
{
	/*
		Capacity must be twice actual string length
	*/
	const size_t sz = strlen(str)+1;
	assert(sz*2 <= capacity);
	memcpy(str+sz,str,sz);
	char *ostr = str;
	char *istr = str+sz;

	for (int i = 0; i < sz; i++) {
		const char c = istr[i];
		switch(c) {
		case '\\':
			*ostr++ = '\\';
			*ostr++ = '\\';
			break;
		case '\r':
			*ostr++ = '\\';
			*ostr++ = 'r';
			break;
		case '\n':
			*ostr++ = '\\';
			*ostr++ = 'n';
			break;
		case '\t':
			*ostr++ = '\\';
			*ostr++ = 't';
			break;
		default:
			*ostr++ = c;
			break;
		}
	}
}

void
tsv_unescape_inplace(char * str) 
{
	char * iptr = str;
	char * optr = str;
	while (*iptr) {
		const char c = *iptr++;
		if (c != '\\') {
			*optr++ = c;
		} else {
			switch(*iptr++) {
			case '\\':
				*optr++ = '\\';
				break;
			case 'r':
				*optr++ = '\r';
				break;
			case 'n':
				*optr++ = '\n';
				break;
			case 't':
				*optr++ = '\t';
				break;
			default:
				*optr++ = c;
				iptr--;
				break;
			}
		}
	}
	*optr = 0;
}
