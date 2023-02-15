/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * microtar_zstd.h: zstd reader for MicroTAR                               *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <microtar.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open a .tar.zst file using zstd and MicroTAR. (read-only access)
 * @param tar		[out] mtar_t
 * @param filename	[in] Filename
 * @return MTAR_ESUCCESS on success; other on error.
 */
int mtar_zstd_open_ro(mtar_t *tar, const char *filename);

#ifdef __cplusplus
}
#endif
