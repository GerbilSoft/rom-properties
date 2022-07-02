/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * check-uid.h: CHECK_UID() macro.                                         *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_CHECK_UID_HPP__
#define __ROMPROPERTIES_KDE_CHECK_UID_HPP__

#include <unistd.h>
#include <qglobal.h>

// Prevent running as root.
#define CHECK_UID() do { \
	if (getuid() == 0 || geteuid() == 0) { \
		qCritical("*** rom-properties-" RP_KDE_LOWER " does not support running as root."); \
		return; \
	} \
} while (0)

// Prevent running as root.
// If running as root, returns errval.
#define CHECK_UID_RET(errval) do { \
	if (getuid() == 0 || geteuid() == 0) { \
		qCritical("*** rom-properties-" RP_KDE_LOWER " does not support running as root."); \
	} \
} while (0)

#endif /* __ROMPROPERTIES_KDE_CHECK_UID_HPP__ */
