/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * alignment_macros.h: Alignment macros.                                   *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>

/**
 * Alignment macro
 * @param a	Alignment value
 * @param x	Byte count to align
 */
// FIXME: No __typeof__ in MSVC's C mode...
#ifndef ALIGN_BYTES
#  if defined(_MSC_VER) && !defined(__cplusplus)
#    define ALIGN_BYTES(a, x)	(((x)+((a)-1)) & ~((uint64_t)((a)-1)))
#  else
#    define ALIGN_BYTES(a, x)	(((x)+((a)-1)) & ~((__typeof__(x))((a)-1)))
#  endif
#endif /* !ALIGN_BYTES */

/**
 * Alignment assertion macro.
 */
#define ASSERT_ALIGNMENT(a, ptr)	assert(reinterpret_cast<uintptr_t>(ptr) % (a) == 0);

/**
 * Aligned variable macro.
 * @param a Alignment value.
 * @param decl Variable declaration.
 */
#if defined(__GNUC__)
#  define ALIGNED_VAR(a, decl)	decl __attribute__((aligned(a)))
#elif defined(_MSC_VER)
#  define ALIGNED_VAR(a, decl)	__declspec(align(a)) decl
#else
#  error No aligned variable macro for this compiler.
#endif
