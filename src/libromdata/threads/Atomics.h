/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Atomics.h: Atomic function macros.                                      *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __LIBROMDATA_THREADS_ATOMICS_H__
#define __LIBROMDATA_THREADS_ATOMICS_H__

// Atomic function macros.
// TODO: C++11 atomic support; test all of this.
#if defined(__clang__)
# if 0 && (__has_feature(c_atomic) || __has_extension(c_atomic))
   /* Clang: Use prefixed C11-style atomics. */
   /* FIXME: Not working... (clang-3.9.0 complains that it's not declared.) */
#  define ATOMIC_INC_FETCH(ptr) __c11_atomic_add_fetch(ptr, 1, __ATOMIC_SEQ_CST)
#  define ATOMIC_OR_FETCH(ptr, val) __c11_atomic_or_fetch(ptr, val, __ATOMIC_SEQ_CST)
# else
   /* Use Itanium-style atomics. */
#  define ATOMIC_INC_FETCH(ptr) __sync_add_and_fetch(ptr, 1)
#  define ATOMIC_OR_FETCH(ptr, val) __sync_or_and_fetch(ptr, val)
# endif
#elif defined(__GNUC__)
# if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
   /* gcc-4.7: Use prefixed C11-style atomics. */
#  define ATOMIC_INC_FETCH(ptr) __atomic_add_fetch(ptr, 1, __ATOMIC_SEQ_CST)
#  define ATOMIC_OR_FETCH(ptr, val) __atomic_or_fetch(ptr, val, __ATOMIC_SEQ_CST)
# else
   /* gcc-4.6 and earlier: Use Itanium-style atomics. */
#  define ATOMIC_INC_FETCH(ptr) __sync_add_and_fetch(ptr, 1)
#  define ATOMIC_OR_FETCH(ptr, val) __sync_or_and_fetch(ptr, val)
# endif
#elif defined(_MSC_VER)
# include <intrin.h>
static inline int ATOMIC_INC_FETCH(volatile int *ptr)
{
	return _InterlockedIncrement(reinterpret_cast<volatile long*>(ptr));
}
static inline int ATOMIC_OR_FETCH(volatile int *ptr, int val)
{
	return _InterlockedOr(reinterpret_cast<volatile long*>(ptr), val);
}
#else
# error Atomic functions not defined for this compiler.
#endif

#endif /* __LIBROMDATA_THREADS_ATOMICS_H__ */
