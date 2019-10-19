/***************************************************************************
 * ROM Properties Page shell extension. (librpthreads)                     *
 * pthread_once.h: pthread_once() implementation for systems that don't    *
 * support pthreads natively.                                              *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Based on the InitOnceExecuteOnce() implementation from Chromium:
// - https://chromium.googlesource.com/chromium/src.git/+/18ad5f3a40ceab583961ca5dc064e01900514c57%5E%21/#F0

#ifndef __ROMPROPERTIES_LIBRPTHREADS_PTHREAD_ONCE_H__
#define __ROMPROPERTIES_LIBRPTHREADS_PTHREAD_ONCE_H__

#include "config.librpthreads.h"

#ifdef HAVE_PTHREADS
// System has pthreads. Use it directly.
#include <pthread.h>
#else /* !HAVE_PTHREADS */

// System does not have pthreads.
// Provide our own pthread_once().
#ifdef __cplusplus
extern "C" {
#endif

typedef int pthread_once_t;
#define PTHREAD_ONCE_INIT 0

/**
 * pthread_once() implementation.
 * Based on the InitOnceExecuteOnce() implementation from Chromium.
 * @param once_control
 * @param init_routine
 * @return
 */
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

#ifdef __cplusplus
}
#endif

#endif /* HAVE_PTHREADS */

#endif /* __ROMPROPERTIES_LIBRPTHREADS_PTHREAD_ONCE_H__ */
