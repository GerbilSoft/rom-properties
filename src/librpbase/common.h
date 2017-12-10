/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * common.h: Common types and macros.                                      *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_COMMON_H__
#define __ROMPROPERTIES_LIBRPBASE_COMMON_H__

/**
 * Number of elements in an array.
 *
 * Includes a static check for pointers to make sure
 * a dynamically-allocated array wasn't specified.
 * Reference: http://stackoverflow.com/questions/8018843/macro-definition-array-size
 */
#define ARRAY_SIZE(x) \
	((int)(((sizeof(x) / sizeof(x[0]))) / \
		(size_t)(!(sizeof(x) % sizeof(x[0])))))

// PACKED struct attribute.
// Use in conjunction with #pragma pack(1).
#ifdef __GNUC__
#define PACKED __attribute__((packed))
#else
#define PACKED
#endif

/**
 * static_asserts size of a structure
 * Also defines a constant of form StructName_SIZE
 */
#define ASSERT_STRUCT(st,sz) enum { st##_SIZE = (sz), }; \
	static_assert(sizeof(st)==(sz),#st " is not " #sz " bytes.")

// RP equivalent of Q_UNUSED().
#define RP_UNUSED(x) ((void)x)

#ifdef __cplusplus
// RP equivalents of Q_D() and Q_Q().
#define RP_D(klass) klass##Private *const d = static_cast<klass##Private*>(d_ptr)
#define RP_Q(klass) klass *const q = static_cast<klass*>(q_ptr)

// RP equivalent of Q_DISABLE_COPY().
#if __cplusplus >= 201103L
#define RP_DISABLE_COPY(klass) \
	klass(const klass &) = delete; \
	klass &operator=(const klass &) = delete;
#else
#define RP_DISABLE_COPY(klass) \
	klass(const klass &); \
	klass &operator=(const klass &);
#endif
#endif /* __cplusplus */

// Deprecated function attribute.
#ifndef DEPRECATED
# if defined(__GNUC__)
#  define DEPRECATED __attribute__ ((deprecated))
# elif defined(_MSC_VER)
#  define DEPRECATED __declspec(deprecated)
# else
#  define DEPRECATED
# endif
#endif

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

// gcc branch prediction hints.
// Should be used in combination with profile-guided optimization.
#ifdef __GNUC__
# define likely(x)	__builtin_expect(!!(x), 1)
# define unlikely(x)	__builtin_expect(!!(x), 0)
#else
# define likely(x)	x
# define unlikely(x)	x
#endif

// C99 restrict macro.
// NOTE: gcc only defines restrict in C, not C++,
// so use __restrict on both gcc and MSVC.
#define RESTRICT __restrict

// typeof() for MSVC.
#ifdef _MSC_VER
#define __typeof__(x) decltype(x)
#endif

/**
 * Alignment macro.
 * @param a	Alignment value.
 * @param x	Byte count to align.
 */
#define ALIGN(a, x)	(((x)+((a)-1))&~((__typeof__(x))((a)-1)))

/**
 * Alignment assertion macro.
 */
#define ASSERT_ALIGNMENT(a, ptr)	assert(reinterpret_cast<uintptr_t>(ptr) % 16 == 0);

// C API declaration for MSVC.
// Required when using stdcall as the default calling convention.
#ifdef _MSC_VER
# define RP_C_API __cdecl
#else
# define RP_C_API
#endif

#endif /* __ROMPROPERTIES_LIBRPBASE_COMMON_H__ */
