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

static int ifunc_method_impl4(int a, long b, char c)
{
	return (int)(a * b + c) * 4;
}

static __typeof__(&ifunc_method_impl1) ifunc_method_resolve(void)
{
	time_t s = time(NULL);

	// NOTE: Since libromdata is a shared library now, IFUNC resolvers
	// cannot call PLT functions. Otherwise, it will crash.
	// We'll use gcc's built-in CPU ID functions instead.
	// Requires gcc-4.8 or later, or clang-6.0 or later.
	// Also, this only works on i386/amd64.
#if defined(__i386__) || defined(__amd64__) || defined(_M_IX86) || defined(_M_X64)
	__builtin_cpu_init();
	if (__builtin_cpu_supports("sse2")) s ^= 3;
#endif

	switch (s & 3) {
		default:
		case 0:	return &ifunc_method_impl1;
		case 1:	return &ifunc_method_impl2;
		case 2:	return &ifunc_method_impl3;
		case 3:	return &ifunc_method_impl4;
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
	if (ret == 38 || ret == 38*2 || ret == 38*3 || ret == 38*4)
		return EXIT_SUCCESS;
	return EXIT_FAILURE;
}
