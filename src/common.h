/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * common.h: Common types and macros.                                      *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// RP_FORCEINLINE
#include "force_inline.h"

// Alignment macros
#include "alignment_macros.h"

// Compiler attributes
#include "compiler-attrs.h"

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

// RP_ALIGNED struct attribute.
// Use in conjunction with #pragma pack(n).
#ifdef __GNUC__
#  define RP_ALIGNED(n) __attribute__((aligned(n)))
#else
#  define RP_ALIGNED(n)
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
#  define ASSERT_STRUCT(st, sz) static_assert(sizeof(st) == (sz), #st " is not " #sz " bytes.")
#elif defined(HAVE_STATIC_ASSERT_C)
#  define ASSERT_STRUCT(st, sz) _Static_assert(sizeof(st) == (sz), #st " is not " #sz " bytes.")
#else
#  define ASSERT_STRUCT(st, sz)
#endif

/**
 * static_asserts offset of a structure member
 * Also defines a constant of form StructName_MemberName_OFFSET
 */
#if defined(HAVE_STATIC_ASSERT_CXX)
#  define ASSERT_STRUCT_OFFSET(st, mb, of) static_assert(offsetof(st, mb) == (of), #mb " is not at offset " #of " in struct " #st ".")
#elif defined(HAVE_STATIC_ASSERT_C)
#  define ASSERT_STRUCT_OFFSET(st, mb, of) _Static_assert(offsetof(st, mb) == (of), #mb " is not at offset " #of " in struct " #st ".")
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
