/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * os-secure_posix.c: OS security functions. (dummy implementation)        *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "os-secure.h"

/**
 * Enable OS-specific security functionality.
 * @return 0 on success; negative POSIX error code on error.
 */
int rpcli_os_secure(void)
{
	// Dummy implementation does nothing.
	return 0;
}
