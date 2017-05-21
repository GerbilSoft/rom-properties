/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * stdafx.h: Common definitions and includes for COM.                      *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_STDAFX_H__
#define __ROMPROPERTIES_WIN32_STDAFX_H__

#ifndef _WIN32
#error stdafx.h is Windows only.
#endif

// Windows SDK defines and includes.
#include "libwin32common/RpWin32_sdk.h"

// Additional Windows headers.
#include <windows.h>
#include <windowsx.h>
#include <olectl.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <comdef.h>
#include <shlwapi.h>

// IEmptyVolumeCache and related.
#include <emptyvc.h>

// NOTE: Since we're targetting XP/2003 minimum, some progress bar
// functionality isn't defined in the headers.
#ifndef PBM_SETSTATE
#define PBM_SETSTATE            (WM_USER+16) // wParam = PBST_[State] (NORMAL, ERROR, PAUSED)
#endif
#ifndef PBM_GETSTATE
#define PBM_GETSTATE            (WM_USER+17)
#endif

#ifndef PBST_NORMAL
#define PBST_NORMAL             0x0001
#endif
#ifndef PBST_ERROR
#define PBST_ERROR              0x0002
#endif
#ifndef PBST_PAUSED
#define PBST_PAUSED             0x0003
#endif

#endif /* __ROMPROPERTIES_WIN32_STDAFX_H__ */
