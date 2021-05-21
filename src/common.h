/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * common.h: Common types and macros.                                      *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_COMMON_H__
#define __ROMPROPERTIES_COMMON_H__

#ifdef __cplusplus
# include <cstddef>
#else
# include <stddef.h>
#endif

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
# define PACKED __attribute__((packed))
#else
# define PACKED
#endif

/**
 * static_asserts size of a structure
 * Also defines a constant of form StructName_SIZE
 */
// TODO: Check MSVC support for static_assert() in C mode.
#if defined(__cplusplus)
# define ASSERT_STRUCT(st,sz) /*enum { st##_SIZE = (sz), };*/ \
	static_assert(sizeof(st)==(sz),#st " is not " #sz " bytes.")
#elif defined(__GNUC__) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
# define ASSERT_STRUCT(st,sz) /*enum { st##_SIZE = (sz), };*/ \
	_Static_assert(sizeof(st)==(sz),#st " is not " #sz " bytes.")
#else
# define ASSERT_STRUCT(st, sz)
#endif

/**
 * static_asserts offset of a structure member
 * Also defines a constant of form StructName_MemberName_OFFSET
 */
// TODO: Check MSVC support for static_assert() in C mode.
#if defined(__cplusplus)
# define ASSERT_STRUCT_OFFSET(st,mb,of) /*enum { st##_##mb##_OFFSET = (of), };*/ \
	static_assert(offsetof(st,mb)==(of),#mb " is not at offset " #of " in struct " #st ".")
#elif defined(__GNUC__) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
# define ASSERT_STRUCT_OFFSET(st,mb,of) /*enum { st##_##mb##_OFFSET = (of), };*/ \
	_Static_assert(offsetof(st,mb)==(of),#mb " is not at offset " #of " in struct " #st ".")
#else
# define ASSERT_STRUCT_OFFSET(st, mb, of)
#endif

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

// NOTE: MinGW's __forceinline macro has an extra 'extern' when compiling as C code.
// This breaks "static FORCEINLINE".
// Reference: https://sourceforge.net/p/mingw-w64/mailman/message/32882927/
#if !defined(__cplusplus) && defined(__forceinline) && defined(__GNUC__) && defined(_WIN32)
# undef __forceinline
# define __forceinline inline __attribute__((always_inline,__gnu_inline__))
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
// FIXME: Doesn't work in C mode.
#if defined(_MSC_VER) && defined(__cplusplus)
# define __typeof__(x) decltype(x)
#endif

/**
 * Alignment macro.
 * @param a	Alignment value.
 * @param x	Byte count to align.
 */
// FIXME: No __typeof__ in MSVC's C mode...
#if defined(_MSC_VER) && !defined(__cplusplus)
# define ALIGN_BYTES(a, x)	(((x)+((a)-1)) & ~((uint64_t)((a)-1)))
#else
# define ALIGN_BYTES(a, x)	(((x)+((a)-1)) & ~((__typeof__(x))((a)-1)))
#endif

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
# define ALIGNED_VAR(a, decl)	decl __attribute__((aligned(a)))
#elif defined(_MSC_VER)
# define ALIGNED_VAR(a, decl)	__declspec(align(a)) decl
#else
# error No aligned variable macro for this compiler.
#endif

// C API declaration for MSVC.
// Required when using stdcall as the default calling convention.
#ifdef _MSC_VER
# define RP_C_API __cdecl
#else
# define RP_C_API
#endif

// printf()-style function attribute.
#ifndef ATTR_PRINTF
# ifdef __GNUC__
#  define ATTR_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
# else
#  define ATTR_PRINTF(fmt, args)
# endif
#endif /* ATTR_PRINTF */

// gcc-10 adds an "access" attribute to mark pointers as
// read-only, read-write, write-only, or none, and an
// optional object size.
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 10
# define ATTR_ACCESS(access_mode, ref_index) __attribute__((access(access_mode, (ref_index))))
# define ATTR_ACCESS_SIZE(access_mode, ref_index, size_index) __attribute__((access(access_mode, (ref_index), (size_index))))
#else
# define ATTR_ACCESS(access_mode, ref_index)
# define ATTR_ACCESS_SIZE(access_mode, ref_index, size_index)
#endif

#endif /* __ROMPROPERTIES_COMMON_H__ */
