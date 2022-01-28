/**
 * IFUNC test code
 */
#include <stdlib.h>
#include <time.h>

// Minimum glibc version requirements.
#if defined(__GLIBC__)
#  if __GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 11)
#    error glibc-2.11 is required for IFUNC on most platforms
#  elif defined(__arm__) || defined(__thumb__) || \
        defined(__arm) || defined(_ARM) || \
        defined(_M_ARM) || defined(__aarch64__)
#    if __GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 18)
#      error glibc-2.18 is required for IFUNC on ARM
#    endif
#  endif
#endif

static int ifunc_method_impl1(int a, long b, char c)
{
	return (int)(a * b + c);
}

static int ifunc_method_impl2(int a, long b, char c)
{
	return (int)(a * b + c) * 2;
}

static int ifunc_method_impl3(int a, long b, char c)
{
	return (int)(a * b + c) * 3;
}

static __typeof__(&ifunc_method_impl1) ifunc_method_resolve(void)
{
	time_t s = time(NULL);
	switch (s % 3) {
		default:
		case 0:	return &ifunc_method_impl1;
		case 1:	return &ifunc_method_impl2;
		case 2:	return &ifunc_method_impl3;
	}
	return NULL;
}

int ifunc_method(int a, long b, char c)
	__attribute__((ifunc("ifunc_method_resolve")));

int main(void)
{
	// (5 * 7) + 3 == 38
	int a = 5;
	long b = 7;
	char c = 3;

	int ret = ifunc_method(a, b, c);
	if (ret == 38 || ret == 38*2 || ret == 38*3)
		return EXIT_SUCCESS;
	return EXIT_FAILURE;
}
