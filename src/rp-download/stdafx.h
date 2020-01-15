/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * stdafx.h: Common definitions and includes for COM.                      *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RP_DOWNLOAD_STDAFX_H__
#define __ROMPROPERTIES_RP_DOWNLOAD_STDAFX_H__

#ifdef _WIN32
// Windows SDK defines and includes.
#include "libwin32common/RpWin32_sdk.h"

// Additional Windows headers.
#include <shlobj.h>
#include <tchar.h>
#endif /* _WIN32 */

#endif /* __ROMPROPERTIES_RP_DOWNLOAD_STDAFX_H__ */
