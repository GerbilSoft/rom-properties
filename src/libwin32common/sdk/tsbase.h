/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * tsbase.h: Base header for the typesafe inline function headers.         *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_SDK_TSBASE_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_SDK_TSBASE_H__

#ifdef _MAC
#error Old MSVC for Mac compilers are not supported.
#endif

// Make sure STRICT is defined for better type safety.
#ifndef STRICT
#define STRICT
#endif

#include "../RpWin32_sdk.h"
#include <windows.h>

// Force inline attribute.
#if !defined(FORCEINLINE)
# if (!defined(_DEBUG) || defined(NDEBUG))
#  if defined(__GNUC__)
#   define FORCEINLINE inline __attribute__((always_inline))
#  elif defined(_MSC_VER)
#   define FORCEINLINE __forceinline
#  else
#   define FORCEINLINE inline
#  endif
# else
#  ifdef _MSC_VER
#   define FORCEINLINE __inline
#  else
#   define FORCEINLINE inline
#  endif
# endif
#endif /* !defined(FORCEINLINE) */

#ifndef FORCEINLINE
# error FORCEINLINE not defined for this compiler.
#endif

// STATIC_CAST() macro that matches shlwapi.h's version.
#ifndef STATIC_CAST
# ifdef __cplusplus
#  define STATIC_CAST(typ) static_cast<typ>
# else
#  define STATIC_CAST(typ) (typ)
# endif
#endif /* STATIC_CAST */

// REINTERPRET_CAST() macro that matches shlwapi.h's STATIC_CAST().
#ifndef REINTERPRET_CAST
# ifdef __cplusplus
#  define REINTERPRET_CAST(typ) reinterpret_cast<typ>
# else
#  define REINTERPRET_CAST(typ) (typ)
# endif
#endif /* REINTERPRET_CAST */

// CONST_CAST() macro that matches shlwapi.h's STATIC_CAST().
#ifndef CONST_CAST
# ifdef __cplusplus
#  define CONST_CAST(typ) const_cast<typ>
# else
#  define CONST_CAST(typ) (typ)
# endif
#endif /* CONST_CAST */

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_SDK_TSBASE_H__ */
