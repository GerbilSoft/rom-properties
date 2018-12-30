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

// Make sure STRICT is defined for better type safety.
#ifndef STRICT
#define STRICT
#endif

// Windows SDK defines and includes.
#include "libwin32common/RpWin32_sdk.h"
#include <windows.h>

#if _WIN32_WINNT < 0x0600
# error Windows Vista SDK or later is required.
#endif

// Typesafe inline function wrappers for some Windows headers.
#include "libwin32common/sdk/windowsx_ts.h"
#include "libwin32common/sdk/commctrl_ts.h"

// Additional Windows headers.
#include <olectl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlwapi.h>
#include <commdlg.h>

// Native COM support. (C++ only!)
#ifdef __cplusplus
# include <comdef.h>
#endif

// IEmptyVolumeCache and related.
#include <emptyvc.h>

// Windows Terminal Services. (Remote Desktop)
#include <wtsapi32.h>

// Common Controls COM declarations.
#include <commoncontrols.h>

#endif /* __ROMPROPERTIES_WIN32_STDAFX_H__ */
