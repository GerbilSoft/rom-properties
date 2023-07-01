/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * NetworkStatus.h: Get network status.                                    *
 *                                                                         *
 * Copyright (c) 2022-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "stdboolx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Can we identify if this system has a metered network connection?
 * @return True if we can; false if we can't.
 */
bool rp_win32_can_identify_if_metered(void);

/**
 * Is this system using a metered network connection?
 * NOTE: If we can't identify it, this will always return false (unmetered).
 * @return True if metered; false if unmetered.
 */
bool rp_win32_is_metered(void);

#ifdef __cplusplus
}
#endif
