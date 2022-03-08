#include <sys/mman.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

int dirt = 0;
char * page = 0;
size_t pagesz = 0; 

static void
handler(int sig, siginfo_t *si, void *unused)
{
   if (si->si_addr >= page && si->si_addr < page + pagesz) {
	   dirt = 1;
	   int rc = mprotect(page, pagesz, PROT_READ | PROT_WRITE);
	   if(rc == -1) abort();
	   printf("handled segv\n");
   }
}

int main(void) {
	pagesz = sysconf(_SC_PAGESIZE);

	printf("page size: %zu \n", pagesz);

	struct sigaction sa;

	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = handler;
	if (sigaction(SIGSEGV, &sa, NULL) == -1)
		abort();
	

	page = mmap(0, pagesz, PROT_READ | PROT_WRITE, MAP_ANON, -1, 0);
	
	if(!page) abort();

	for (size_t i = 0; i < pagesz; i++) page[i] = 0;

	int rc = mprotect(page, pagesz, PROT_READ) ;
	if(rc == -1) abort();

	for (size_t i = 0; i < pagesz; i++) page[i] = 1;

	for (size_t i = 0; i < pagesz; i++) {
		int x = page[i];
		printf("%02x ", x);
		if (i % 30 == 29)  printf("\n");
	}
	printf("\n");
	printf("dirt: %i\n", dirt);
}
