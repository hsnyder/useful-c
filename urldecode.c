/*
	Decode URL-escaped string (i.e. '+' becomes ' ' and '%NN' becomes whatever character is '\xNN'
*/
#include <stdio.h>
#include <wchar.h>
#include <locale.h>

int 
main(int argc, char ** argv)
{
	const size_t N = 4096;
	wchar_t buf[N];

	setlocale(LC_ALL, "");

	while(fgetws(buf,N,stdin)) {
		
		wchar_t *p = buf;
		while(*p != 0) {
			if (L'+' == *p) fputwc(L' ', stdout);

			else if (L'%' == *p) {
				if(p[1]==0) {
					ungetwc(*p,stdin);
					break;
				}
				if(p[2]==0) {
					ungetwc(*p,stdin);
					ungetwc(p[1],stdin);
					break;
				}
				wchar_t number[3] = {};
				number[0] = p[1];
				number[1] = p[2];
				int x = wcstol(number, 0, 16);
				putwc(btowc(x), stdout);
				p+=2;
			}

			else fputwc(*p, stdout);

			p++;
		}

	}
}
