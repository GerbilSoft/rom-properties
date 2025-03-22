/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * common.h: Common types and macros.                                      *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// FORCEINLINE
#include "force_inline.h"

#ifdef __cplusplus
#  include <cstddef>
#else
#  include <stddef.h>
#endif

/**
 * Number of elements in an array.
 *
 * Includes a static check for pointers to make sure
 * a dynamically-allocated array wasn't specified.
 * Reference: http://stackoverflow.com/questions/8018843/macro-definition-array-size
 */
#define ARRAY_SIZE(x) \
	(((sizeof(x) / sizeof((x)[0]))) / \
		(size_t)(!(sizeof(x) % sizeof((x)[0]))))

/**
 * Number of elements in an array. (cast to int)
 */
#define ARRAY_SIZE_I(x) ((int)(ARRAY_SIZE(x)))

// RP_PACKED struct attribute.
// Use in conjunction with #pragma pack(1).
// (NOTE: Was previously PACKED, but that conflicts with libfmt.)
#ifdef __GNUC__
#  define RP_PACKED __attribute__((packed))
#else
#  define RP_PACKED
#endif

// static_assert() support
#if defined(__cplusplus)
// C++11 supports static_assert() with two arguments.
#  define HAVE_STATIC_ASSERT_CXX 1
#else /* !__cplusplus */
#  if defined(_MSC_VER) && _MSC_VER >= 1900
// MSVC 2015 and later supports _Static_assert() in C.
#    define HAVE_STATIC_ASSERT_C 1
#  elif defined(__GNUC__) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
// gcc supports _Static_assert() if compiling as C11 or later.
#    define HAVE_STATIC_ASSERT_C 1
#  endif
#endif /* __cplusplus */

/**
 * static_asserts size of a structure
 * Also defines a constant of form StructName_SIZE
 */
#if defined(HAVE_STATIC_ASSERT_CXX)
#  define ASSERT_STRUCT(st,sz) /*enum { st##_SIZE = (sz), };*/ \
	static_assert(sizeof(st)==(sz),#st " is not " #sz " bytes.")
#elif defined(HAVE_STATIC_ASSERT_C)
#  define ASSERT_STRUCT(st,sz) /*enum { st##_SIZE = (sz), };*/ \
	_Static_assert(sizeof(st)==(sz),#st " is not " #sz " bytes.")
#else
#  define ASSERT_STRUCT(st, sz)
#endif

/**
 * static_asserts offset of a structure member
 * Also defines a constant of form StructName_MemberName_OFFSET
 */
#if defined(HAVE_STATIC_ASSERT_CXX)
#  define ASSERT_STRUCT_OFFSET(st,mb,of) /*enum { st##_##mb##_OFFSET = (of), };*/ \
	static_assert(offsetof(st,mb)==(of),#mb " is not at offset " #of " in struct " #st ".")
#elif defined(HAVE_STATIC_ASSERT_C)
#  define ASSERT_STRUCT_OFFSET(st,mb,of) /*enum { st##_##mb##_OFFSET = (of), };*/ \
	_Static_assert(offsetof(st,mb)==(of),#mb " is not at offset " #of " in struct " #st ".")
#else
#  define ASSERT_STRUCT_OFFSET(st, mb, of)
#endif

// RP equivalent of Q_UNUSED().
#define RP_UNUSED(x) ((void)(x))

// NOLINTBEGIN(bugprone-macro-parentheses, cppcoreguidelines-pro-type-static-cast-downcast)
#ifdef __cplusplus
// RP equivalents of Q_D() and Q_Q().
#  define RP_D(klass) klass##Private *const d = static_cast<klass##Private*>(d_ptr)
#  define RP_Q(klass) klass *const q = static_cast<klass*>(q_ptr)

// RP equivalent of Q_DISABLE_COPY().
#  define RP_DISABLE_COPY(klass) \
public: \
	klass(const klass &) = delete; \
	klass &operator=(const klass &) = delete;
#endif /* __cplusplus */
// NOLINTEND(bugprone-macro-parentheses, cppcoreguidelines-pro-type-static-cast-downcast)

// Deprecated function attribute.
#ifndef DEPRECATED
#  if defined(__GNUC__)
#    define DEPRECATED __attribute__((deprecated))
#  elif defined(_MSC_VER)
#    define DEPRECATED __declspec(deprecated)
#  else
#    define DEPRECATED
#  endif
#endif

// gcc branch prediction hints.
// Should be used in combination with profile-guided optimization.
#ifdef __GNUC__
#  define likely(x)	__builtin_expect(!!(x), 1)
#  define unlikely(x)	__builtin_expect(!!(x), 0)
#else
#  define likely(x)	(x)
#  define unlikely(x)	(x)
#endif

// C99 restrict macro.
// NOTE: gcc only defines restrict in C, not C++,
// so use __restrict on both gcc and MSVC.
#define RESTRICT __restrict

/**
 * Alignment macro.
 * @param a	Alignment value.
 * @param x	Byte count to align.
 */
// FIXME: No __typeof__ in MSVC's C mode...
#if defined(_MSC_VER) && !defined(__cplusplus)
#  define ALIGN_BYTES(a, x)	(((x)+((a)-1)) & ~((uint64_t)((a)-1)))
#else
#  define ALIGN_BYTES(a, x)	(((x)+((a)-1)) & ~((__typeof__(x))((a)-1)))
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
#  define ALIGNED_VAR(a, decl)	decl __attribute__((aligned(a)))
#elif defined(_MSC_VER)
#  define ALIGNED_VAR(a, decl)	__declspec(align(a)) decl
#else
#  error No aligned variable macro for this compiler.
#endif

// printf()-style function attribute.
#ifndef ATTR_PRINTF
#  if defined(__clang__) || defined(__llvm__)
#    define ATTR_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#  elif defined(__GNUC__)
#    if !defined(_WIN32) || (defined(_UCRT) || __USE_MINGW_ANSI_STDIO)
#      define ATTR_PRINTF(fmt, args) __attribute__((format(gnu_printf, fmt, args)))
#    else
#      define ATTR_PRINTF(fmt, args) __attribute__((format(ms_printf, fmt, args)))
#    endif
#  else
#    define ATTR_PRINTF(fmt, args)
#  endif
#endif /* ATTR_PRINTF */

// gcc-10 adds an "access" attribute to mark pointers as
// read-only, read-write, write-only, or none, and an
// optional object size.
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 10
#  define ATTR_ACCESS(access_mode, ref_index) __attribute__((access(access_mode, (ref_index))))
#  define ATTR_ACCESS_SIZE(access_mode, ref_index, size_index) __attribute__((access(access_mode, (ref_index), (size_index))))
#else
#  define ATTR_ACCESS(access_mode, ref_index)
#  define ATTR_ACCESS_SIZE(access_mode, ref_index, size_index)
#endif

// gcc-12 enables automatic vectorization using SSE2.
// This results in massive code bloat compared to the manually-optimized
// versions in some cases, e.g. SMD decoding, so we'll explicitly disable
// vectorization for some of the non-optimized functions.
// (clang and MSVC do not have this issue.)
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))
#  define ATTR_GCC_NO_VECTORIZE __attribute__((optimize("no-tree-vectorize")))
#else
#  define ATTR_GCC_NO_VECTORIZE
#endif

// novtable: suppress vtable generation for abstract base classes.
// NOTE: MSVC and Clang (MS ABI) only!
#if defined(_WIN32) && (defined(_MSC_VER) || defined(__clang__))
#  define NOVTABLE __declspec(novtable)
#else
#  define NOVTABLE
#endif

// no_address_sanitize: Disable AddressSanitizer on a given function.
// Needed for IFUNC functions on gcc due to a bug.
// Reference: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110442
#ifdef __SANITIZE_ADDRESS__
#  ifdef _MSC_VER
#    define NO_SANITIZE_ADDRESS __declspec(no_sanitize_address)
#  else /* !_MSC_VER */
#    define NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#  endif /* _MSC_VER */
#else /* !__SANITIZE_ADDRESS__ */
#  define NO_SANITIZE_ADDRESS
#endif /* __SANITIZE_ADDRESS__ */

// pure: Function does not have any effects except on the return value,
// and it only depends on the input parameters and/or global variables.
#ifndef ATTR_PURE
#  ifdef __GNUC__
#    define ATTR_PURE __attribute__((pure))
#  else /* !__GNUC__ */
#    define ATTR_PURE
#  endif /* __GNUC__ */
#endif /* ATTR_PURE */

// const: Function does not have any effects except on the return value,
// and it only depends on the input parameters. (similar to constexpr)
#ifndef ATTR_CONST
#  ifdef __GNUC__
#    define ATTR_CONST __attribute__((const))
#  else /* !__GNUC__ */
#    define ATTR_CONST
#  endif /* __GNUC__ */
#endif /* ATTR_CONST */
