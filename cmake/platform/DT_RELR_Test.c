/**
 * DT_RELR test code
 */
#include <stdlib.h>
#include <stdio.h>

// Minimum glibc version requirements.
#if defined(__GLIBC__)
#  if __GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 36)
#    error glibc-2.36 is required for DT_RELR
#  endif
#else
#  error TODO: Detect DT_RELR on non-glibc platforms
#endif

void test_print(const char *a, const char *b)
{
	printf("%s %s\n", a, b);
}

int main(void)
{
	const char *tbl[] = {"quack", "moo"};
	test_print(tbl[0], tbl[1]);
	return 0;
}
