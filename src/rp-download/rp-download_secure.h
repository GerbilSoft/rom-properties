/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * rp-download_secure.h: Security options for rp-download.                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enable security options.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_download_do_security_options(void);

#ifdef __cplusplus
}
#endif
