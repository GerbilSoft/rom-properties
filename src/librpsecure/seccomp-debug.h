/***************************************************************************
 * ROM Properties Page shell extension. (librpsecure)                      *
 * seccomp-debug.c: Linux seccomp debug functionality.                     *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifndef NDEBUG
#  define ENABLE_SECCOMP_DEBUG 1
#endif /* !NDEBUG */

#ifdef ENABLE_SECCOMP_DEBUG

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Install the signal handler for SIGSYS.
 * This will print debugging information for trapped system calls.
 */
void seccomp_debug_install_sigsys(void);

#ifdef __cplusplus
}
#endif

#endif /* !ENABLE_SECCOMP_DEBUG */
