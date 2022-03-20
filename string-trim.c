#include <stdio.h>
#include <wctype.h>
#include <wchar.h>
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

