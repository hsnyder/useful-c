/*
	Generate 10 consecutive lines of a million characters each.
	For testing if utilities have input line length limits.
*/


char tab[] = {
	'a','b','c','d','e','f','g','h','i','j','k',
	'l','m','n','o','p','q','r','s','t','u','v',
	'w','x','y','z','0','1','2','3','4','5','6',
	'7','8','9',' ',' ',' ',' ',' ',' ',' ',' '
};


#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(void)
{
	srand(time(NULL));
	for (int i = 0; i < 10; i++) {

		for (int j = 1; j< 1000000; j++) {
			fputc(tab[rand()%sizeof(tab)], stdout);
		}
		fputc('\n',stdout);

	}
}
