/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * tsbase.h: Base header for the typesafe inline function headers.         *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_SDK_TSBASE_H__
#define __ROMPROPERTIES_WIN32_SDK_TSBASE_H__

#ifdef _MAC
#error Old MSVC for Mac compilers are not supported.
#endif

// Make sure STRICT is defined for better type safety.
#ifndef STRICT
#define STRICT
#endif

#include "libwin32common/RpWin32_sdk.h"
#include <windows.h>

#ifndef FORCEINLINE
# if defined(_MSC_VER)
#  define FORCEINLINE __forceinline
# elif defined(__GNUC__)
#  define FORCEINLINE __attribute__((always_inline))
# else
#  error FORCEINLINE not defined for this compiler.
# endif
#endif /* FORCEINLINE */

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

#endif /* __ROMPROPERTIES_WIN32_SDK_TSBASE_H__ */
