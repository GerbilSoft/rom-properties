/**
 * 64-bit time_t test code.
 * Reference: https://github.com/Benjamin-Dobell/Heimdall/blob/master/cmake/LargeFiles.c
 */

#include <stdint.h>
#include <time.h>

/** static_assert() macro copied from c++11-compat.h **/
#define static_assert(expr, msg) switch (0) { case 0: case (expr): ; }

int main(int argc, char *argv[])
{
	static_assert(sizeof(time_t) == sizeof(int64_t), "time_t is the wrong size");
	int64_t tm64;
	time_t tm = time(&tm64);
	return 0;
}