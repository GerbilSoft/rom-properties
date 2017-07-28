/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * pthread_once.h: pthread_once() implementation for systems that don't    *
 * support pthreads natively.                                              *
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

// Based on the InitOnceExecuteOnce() implementation from Chromium:
// - https://chromium.googlesource.com/chromium/src.git/+/18ad5f3a40ceab583961ca5dc064e01900514c57%5E%21/#F0

#ifndef __ROMPROPERTIES_LIBRPBASE_THREADS_PTHREAD_ONCE_H__
#define __ROMPROPERTIES_LIBRPBASE_THREADS_PTHREAD_ONCE_H__

#include "config.librpbase.h"

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

#endif /* __ROMPROPERTIES_LIBRPBASE_THREADS_PTHREAD_ONCE_H__ */
