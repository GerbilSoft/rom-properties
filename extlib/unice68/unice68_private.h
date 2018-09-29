/**
 * @ingroup  lib_unice68
 * @file     unice68_private.h
 * @author   Benjamin Gerard
 * @date     2016/07/29
 * @brief    ICE! depacker private header.
 */

/* Copyright (c) 1998-2016 Benjamin Gerard */

#define BUILD_UNICE68 1

#ifdef UNICE68_H
# error unice68_private.h must be include before unice68.h
#endif

#ifdef UNICE68_PRIVATE_H
# error unice68_private.h must be include once
#endif
#define UNICE68_PRIVATE_H 1

#ifdef HAVE_PRIVATE_H
# include "private.h"
#endif

/* ---------------------------------------------------------------- */

#if defined(DEBUG) && !defined(_DEBUG)
# define _DEBUG DEBUG
#elif !defined(DEBUG) && defined(_DEBUG)
# define DEBUG _DEBUG
#endif

#ifndef UNICE68_WIN32
# if defined(WIN32) || defined(_WIN32) || defined(__SYMBIAN32__)
#  define UNICE68_WIN32 1
# endif
#endif

#if defined(DLL_EXPORT) && !defined(unice68_lib_EXPORTS)
# define unice68_lib_EXPORTS DLL_EXPORT
#endif

#ifndef UNICE68_API
# if defined(UNICE68_WIN32) && defined(unice68_lib_EXPORTS)
#  define UNICE68_API __declspec(dllexport)
# endif
#endif
