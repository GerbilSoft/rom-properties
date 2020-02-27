/***************************************************************************
 * ROM Properties Page shell extension. (librpsecure)                      *
 * seccomp-debug.c: Linux seccomp debug functionality.                     *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPSECURE_SECCOMP_DEBUG_H__
#define __ROMPROPERTIES_LIBRPSECURE_SECCOMP_DEBUG_H__

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

#endif /* __ROMPROPERTIES_LIBRPSECURE_SECCOMP_DEBUG_H__ */
