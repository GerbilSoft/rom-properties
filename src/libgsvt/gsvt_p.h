/***************************************************************************
 * ROM Properties Page shell extension. (libgsvt)                          *
 * gsvt_p.h: Virtual Terminal wrapper functions. (PRIVATE functions)       *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gsvt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Send a terminal query command and retrieve a response string.
 * @param cmd Query command
 * @param buf Response buffer
 * @param size Size of buf
 * @param endchr Expected end character (Windows only!)
 * @return 0 on success; negative POSIX error code on error.
 */
ATTR_ACCESS_SIZE(read_write, 2, 3)
int gsvt_query_tty(const char *cmd, TCHAR *buf, size_t size, TCHAR endchr);

#ifdef __cplusplus
}
#endif /* __cplusplus */
