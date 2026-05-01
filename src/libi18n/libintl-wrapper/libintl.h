/* libgnuintl.h wrapper file */
#include "libgnuintl.h"

// Undefine libgnuintl's wrapper functions.
// These cause problems when using delay-load on Windows because
// if libgnuintl isn't available, printf() won't be available,
// so printing an error message will crash the program.
// The other functions are unnecessary as well.
#ifdef fprintf
#undef fprintf
#endif
#ifdef vfprintf
#undef vfprintf
#endif
#ifdef printf
#undef printf
#endif
#ifdef vprintf
#undef vprintf
#endif
#ifdef sprintf
#undef sprintf
#endif
#ifdef vsprintf
#undef vsprintf
#endif
#ifdef snprintf
#undef snprintf
#endif
#ifdef vsnprintf
#undef vsnprintf
#endif
#ifdef asprintf
#undef asprintf
#endif
#ifdef vasprintf
#undef vasprintf
#endif
#ifdef fwprintf
#undef fwprintf
#endif
#ifdef vfwprintf
#undef vfwprintf
#endif
#ifdef wprintf
#undef wprintf
#endif
#ifdef vwprintf
#undef vwprintf
#endif
#ifdef swprintf
#undef swprintf
#endif
#ifdef vswprintf
#undef vswprintf
#endif
#ifdef setlocale
#undef setlocale
#endif
#ifdef newlocale
#undef newlocale
#endif

#ifdef vasnprintf
#undef vasnprintf
#endif
#ifdef vasnwprintf
#undef vasnwprintf
#endif

// Fix snprintf() on older MSVC.
// See c99-compat.msvcrt.h.
#ifdef _MSC_VER
# if _MSC_VER < 1400
/* MSVC 2003 and older. Don't use variadic macros. */
#  define snprintf  _snprintf
# elif _MSC_VER < 1900
/* MSVC 2005 through MSVC 2013. Use variadic macros. */
#  define snprintf(str, size, format, ...)  _snprintf(str, size, format, __VA_ARGS__)
# endif
#endif /* _MSC_VER */
