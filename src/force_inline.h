/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * force_inline.h: RP_FORCEINLINE macro.                                   *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Force inline attribute
#ifndef RP_FORCEINLINE
#  if (!defined(_DEBUG) || defined(NDEBUG))
#    if defined(__GNUC__)
#      define RP_FORCEINLINE inline __attribute__((always_inline))
#    elif defined(_MSC_VER)
#      define RP_FORCEINLINE __forceinline
#    else
#      define RP_FORCEINLINE inline
#    endif
#  else
#    ifdef _MSC_VER
#      define RP_FORCEINLINE __inline
#    else
#      define RP_FORCEINLINE inline
#    endif
#  endif
#endif /* !RP_FORCEINLINE */
