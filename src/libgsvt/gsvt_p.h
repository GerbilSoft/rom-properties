/***************************************************************************
 * ROM Properties Page shell extension. (libgsvt)                          *
 * gsvt_p.h: Virtual Terminal wrapper functions. (PRIVATE functions)       *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gsvt.h"
#include "common.h"
#include "tcharx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Send a terminal query command and retrieve a response string.
 * Response string should be a numeric list and end with a single lowercase letter.
 * @param cmd Query command
 * @param buf Response buffer
 * @param size Size of buf
 * @return 0 on success; negative POSIX error code on error.
 */
ATTR_ACCESS_SIZE(read_write, 2, 3)
int gsvt_query_tty(const char *cmd, TCHAR *buf, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
