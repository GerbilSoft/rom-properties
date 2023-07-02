/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * force_inline.h: FORCEINLINE macro.                                      *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// NOTE: MinGW's __forceinline macro has an extra 'extern' when compiling as C code.
// This breaks "static FORCEINLINE".
// Reference: https://sourceforge.net/p/mingw-w64/mailman/message/32882927/
#if !defined(__cplusplus) && defined(__forceinline) && defined(__GNUC__) && defined(_WIN32)
#  undef __forceinline
#  define __forceinline inline __attribute__((always_inline, __gnu_inline__))
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
