/* 
	This "library" is "STB style" (see https://github.com/nothings/stb)
	Short version: the header includes both the declarations and the implementation.	
	The implementation is compiled out unless STRING_TRIM_IMPLEMENTATION is defined.
	So to use this library, you need to define STRING_TRIM_IMPLEMENTATION in ONE .c file
	before you include this header. 
*/

#ifndef STRING_TRIM_H
#define STRING_TRIM_H
#include <wchar.h>

/*
	Trim leading and trailing whitespace from (wide) character string, in-place.
*/

void
wstr_trim_inplace(wchar_t * str);

void
str_trim_inplace(char * str);

#endif

#ifdef STRING_TRIM_IMPLEMENTATION

#include <wctype.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

void
wstr_trim_inplace(wchar_t * str)
{
	/*
		Trim leading and trailing whitespace from wide character string, in-place.
	*/
	const size_t len = wcslen(str);
	size_t first = 0;
	while(first < len && iswspace(str[first])) first++;
	size_t last = len-1;
	while(last > first && iswspace(str[last])) last--;
	wmemmove(str,&str[first],last-first+1);
	str[last-first+1] = 0;
}

void
str_trim_inplace(char * str)
{
	/*
		Trim leading and trailing whitespace from string, in-place.
	*/
	const size_t len = strlen(str);
	size_t first = 0;
	while(first < len && isspace(str[first])) first++;
	size_t last = len-1;
	while(last > first && isspace(str[last])) last--;
	memmove(str,&str[first],last-first+1);
	str[last-first+1] = 0;
}

#endif
#ifdef STRING_TRIM_SELF_TEST

int main(void)
{
	{
		wchar_t test[100] = L" Hello   \n\r  ";
		wprintf(L"Before: '%ls'\n", test);
		wstr_trim_inplace(test);
		wprintf(L"After:  '%ls'\n", test);
	}
	{
		wchar_t test[100] = L"Hello   \n\r  ";
		wprintf(L"Before: '%ls'\n", test);
		wstr_trim_inplace(test);
		wprintf(L"After:  '%ls'\n", test);
	}
	{
		wchar_t test[100] = L" Hello";
		wprintf(L"Before: '%ls'\n", test);
		wstr_trim_inplace(test);
		wprintf(L"After:  '%ls'\n", test);
	}
	{
		wchar_t test[100] = L"Hello";
		wprintf(L"Before: '%ls'\n", test);
		wstr_trim_inplace(test);
		wprintf(L"After:  '%ls'\n", test);
	}


	{
		char test[100] = " Hello   \n\r  ";
		printf("Before: '%s'\n", test);
		str_trim_inplace(test);
		printf("After:  '%s'\n", test);
	}
	{
		char test[100] = "Hello   \n\r  ";
		printf("Before: '%s'\n", test);
		str_trim_inplace(test);
		printf("After:  '%s'\n", test);
	}
	{
		char test[100] = " Hello";
		printf("Before: '%s'\n", test);
		str_trim_inplace(test);
		printf("After:  '%s'\n", test);
	}
	{
		char test[100] = "Hello";
		printf("Before: '%s'\n", test);
		str_trim_inplace(test);
		printf("After:  '%s'\n", test);
	}
}

#endif
