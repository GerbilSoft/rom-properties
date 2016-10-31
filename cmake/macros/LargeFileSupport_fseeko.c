/**
 * Large File Support test code: fseeko(), ftello() [LFS]
 * Reference: https://github.com/Benjamin-Dobell/Heimdall/blob/master/cmake/LargeFiles.c
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/** static_assert() macro copied from c++11-compat.h **/
#define static_assert(expr, msg) switch (0) { case 0: case (expr): ; }

int main(int argc, const char **argv)
{
	static_assert(sizeof(off_t) == sizeof(int64_t), "off_t is the wrong size");
	off_t offset = ftello(NULL);
	fseeko(NULL, offset, SEEK_SET);
	return 0;
}