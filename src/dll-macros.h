/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * dll-macros.h: DLL visibility macros.                                    *
 *                                                                         *
 * Copyright (c) 2022-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C API declaration for MSVC.
// Required when using stdcall as the default calling convention.
#ifdef _MSC_VER
#  define RP_C_API __cdecl
#else
#  define RP_C_API
#endif

// Visibility macros for plugin DLLs. (always dllexport)
// Reference: https://gcc.gnu.org/wiki/Visibility
#if defined _WIN32 || defined __CYGWIN__
#  ifdef __GNUC__
#    define RP_PLUGIN_PUBLIC __attribute__((dllexport))
#  else
#    define RP_PLUGIN_PUBLIC __declspec(dllexport)
#  endif
#  define RP_PLUGIN_LOCAL
#else
#  if __GNUC__ >= 4
#    define RP_PLUGIN_PUBLIC __attribute__((visibility("default")))
#    define RP_PLUGIN_LOCAL __attribute__((visibility("hidden")))
#  else
#    define RP_PLUGIN_PUBLIC
#    define RP_PLUGIN_LOCAL
#  endif
#endif

// Visibility macros for libromdata.
// Reference: https://gcc.gnu.org/wiki/Visibility
#if defined(RP_LIBROMDATA_IS_DLL)
#  if (defined(_WIN32) || defined (__CYGWIN__)) && defined(RP_BUILDING_FOR_DLL)
#    ifdef RP_BUILDING_LIBROMDATA
#      ifdef __GNUC__
#        define RP_LIBROMDATA_PUBLIC __attribute__((dllexport))
#      else
#        define RP_LIBROMDATA_PUBLIC __declspec(dllexport)
#      endif
#    else
#      ifdef __GNUC__
#        define RP_LIBROMDATA_PUBLIC __attribute__((dllimport))
#      else
#        define RP_LIBROMDATA_PUBLIC __declspec(dllimport)
#      endif
#    endif
#    define RP_LIBROMDATA_LOCAL
#  else /* !defined(_WIN32) && !defined (__CYGWIN__) */
#    if __GNUC__ >= 4
#      define RP_LIBROMDATA_PUBLIC __attribute__((visibility("default")))
#      define RP_LIBROMDATA_LOCAL  __attribute__((visibility("hidden")))
#    else
#      define RP_LIBROMDATA_PUBLIC
#      define RP_LIBROMDATA_LOCAL
#    endif
#  endif
#else /* !defined(RP_LIBROMDATA_IS_DLL) */
#  define RP_LIBROMDATA_PUBLIC
#  define RP_LIBROMDATA_LOCAL
#endif
