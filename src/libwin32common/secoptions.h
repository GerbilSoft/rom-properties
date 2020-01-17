/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * secoptions.h: Security options for executables.                         *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_SECOPTIONS_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_SECOPTIONS_H__

#include "RpWin32_sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * rom-properties Windows executable initialization.
 * This sets various security options.
 * @param bHighSec If non-zero, enable high security for unprivileged processes.
 * @return 0 on success; non-zero on error.
 */
int rp_secoptions_init(BOOL bHighSec);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_SECOPTIONS_H__ */
