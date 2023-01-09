/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * check-uid.h: CHECK_UID() macro.                                         *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <unistd.h>
#include <glib.h>

// Prevent running as root.
#define CHECK_UID() do { \
	if (getuid() == 0 || geteuid() == 0) { \
		g_critical("*** " G_LOG_DOMAIN " does not support running as root."); \
		return; \
	} \
} while (0)

// Prevent running as root.
// If running as root, returns errval.
#define CHECK_UID_RET(errval) do { \
	if (getuid() == 0 || geteuid() == 0) { \
		g_critical("*** " G_LOG_DOMAIN " does not support running as root."); \
		return (errval); \
	} \
} while (0)
