/***************************************************************************
 * ROM Properties Page shell extension. (librpsecure)                      *
 * os-secure_dummy.c: OS security functions. (dummy implementation)        *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "os-secure.h"

/**
 * Enable OS-specific security functionality.
 * @param param OS-specific parameter.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_secure_enable(rp_secure_param_t param)
{
	// Dummy implementation does nothing.
	((void)param);
	return 0;
}
