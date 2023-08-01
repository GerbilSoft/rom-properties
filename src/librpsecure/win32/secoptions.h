/***************************************************************************
 * ROM Properties Page shell extension. (librpsecure/win32)                *
 * secoptions.h: Security options for executables.                         *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#define __ROMPROPERTIES_LIBRPSECURE_WIN32_SECOPTIONS_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * rom-properties Windows executable initialization.
 * This sets various security options.
 * @param bHighSec If non-zero, enable high security for unprivileged processes.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_secure_win32_secoptions_init(int bHighSec);

#ifdef __cplusplus
}
#endif
