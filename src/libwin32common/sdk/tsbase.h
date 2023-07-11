/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * tsbase.h: Base header for the typesafe inline function headers.         *
 ***************************************************************************/

#pragma once

#ifdef _MAC
#  error Old MSVC for Mac compilers are not supported.
#endif

// Make sure STRICT is defined for better type safety.
#ifndef STRICT
#  define STRICT
#endif

#include "../RpWin32_sdk.h"
#include <windows.h>

// NOTE: MinGW's __forceinline macro has an extra 'extern' when compiling as C code.
// This breaks "static FORCEINLINE".
// Reference: https://sourceforge.net/p/mingw-w64/mailman/message/32882927/
#if !defined(__cplusplus) && defined(__forceinline) && defined(__GNUC__) && defined(_WIN32)
#  undef __forceinline
#  define __forceinline inline __attribute__((always_inline,__gnu_inline__))
#endif

// Force inline attribute.
#if !defined(FORCEINLINE)
#  if (!defined(_DEBUG) || defined(NDEBUG))
#    if defined(__GNUC__)
#      define FORCEINLINE inline __attribute__((always_inline))
#    elif defined(_MSC_VER)
#      define FORCEINLINE __forceinline
#    else
#      define FORCEINLINE inline
#    endif
#  else
#    ifdef _MSC_VER
#      define FORCEINLINE __inline
#    else
#      define FORCEINLINE inline
#    endif
#  endif
#endif /* !defined(FORCEINLINE) */

#ifndef FORCEINLINE
#  error FORCEINLINE not defined for this compiler.
#endif

// STATIC_CAST() macro that matches shlwapi.h's version.
#ifndef STATIC_CAST
#  ifdef __cplusplus
#    define STATIC_CAST(typ) static_cast<typ>
#  else
#    define STATIC_CAST(typ) (typ)
#  endif
#endif /* STATIC_CAST */

// REINTERPRET_CAST() macro that matches shlwapi.h's STATIC_CAST().
#ifndef REINTERPRET_CAST
#  ifdef __cplusplus
#    define REINTERPRET_CAST(typ) reinterpret_cast<typ> /* NOLINT(performance-no-int-to-ptr) */
#  else
#    define REINTERPRET_CAST(typ) (typ)
#  endif
#endif /* REINTERPRET_CAST */

// CONST_CAST() macro that matches shlwapi.h's STATIC_CAST().
#ifndef CONST_CAST
#  ifdef __cplusplus
#    define CONST_CAST(typ) const_cast<typ>
#  else
#    define CONST_CAST(typ) (typ)
#  endif
#endif /* CONST_CAST */
