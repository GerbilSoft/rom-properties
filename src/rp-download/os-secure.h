/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * os-secure.h: OS security functions.                                     *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RP_DOWNLOAD_OS_SECURE_H__
#define __ROMPROPERTIES_RP_DOWNLOAD_OS_SECURE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enable OS-specific security functionality.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_download_os_secure(void);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_RP_DOWNLOAD_OS_SECURE_H__ */
