#define type_isequal(A,B) \
	printf("%-6zu  %-6zu  %-4i  %-15s  %-22s \n", \
			sizeof(A), \
			sizeof(B), \
			_Generic(( (A){0} ), B: 1, default: 0), \
			#A, \
			#B)

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main(void) {

	printf("Size A  Size B  Same  Type A           Type B\n");
	printf("------  ------  ----  ---------------  ---------------------- \n");

	type_isequal(int8_t, char);
	type_isequal(int8_t, signed char);
	type_isequal(int16_t, short);
	type_isequal(int32_t, int);
	type_isequal(int32_t, long);
	type_isequal(int64_t, long);
	type_isequal(int64_t, long long);
	type_isequal(int64_t, ssize_t);
	printf("\n");

	type_isequal(intptr_t, ssize_t);
	type_isequal(ptrdiff_t, ssize_t);
	printf("\n");

	type_isequal(int, long);
	type_isequal(long, long long);
	printf("\n");

	type_isequal(uint8_t,  unsigned char);
	type_isequal(uint16_t, unsigned short);
	type_isequal(uint32_t, unsigned int);
	type_isequal(uint32_t, unsigned long);
	type_isequal(uint64_t, unsigned long);
	type_isequal(uint64_t, unsigned long long);
	type_isequal(uint64_t, size_t);
	printf("\n");

	type_isequal(uintptr_t, size_t);
	printf("\n");

	type_isequal(unsigned int, unsigned long);
	type_isequal(unsigned long, unsigned long long);
	printf("\n");


}
