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

// Typesafe inline function wrappers for some Windows headers.
#include "libwin32common/sdk/windowsx_ts.h"
#include "libwin32common/sdk/commctrl_ts.h"

// Additional Windows headers.
#include <olectl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <comdef.h>
#include <shlwapi.h>
#include <commdlg.h>

// IEmptyVolumeCache and related.
#include <emptyvc.h>

// Windows Terminal Services. (Remote Desktop)
#include <wtsapi32.h>

/** Windows Vista functionality. **/
// These aren't available in the Windows headers unless
// _WIN32_WINNT >= 0x0600.
#if _WIN32_WINNT < 0x0600

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

// Split buttons. (for buttons with dropdown menus)
#ifndef BS_SPLITBUTTON
#define BS_SPLITBUTTON	0x0000000CL
#endif
#ifndef BCSIF_GLYPH
#define BCSIF_GLYPH	0x0001
#endif
#ifndef BCSIF_IMAGE
#define BCSIF_IMAGE	0x0002
#endif
#ifndef BCSIF_STYLE
#define BCSIF_STYLE	0x0004
#endif
#ifndef BCSIF_SIZE
#define BCSIF_SIZE	0x0008
#endif
#ifndef BCSS_NOSPLIT
#define BCSS_NOSPLIT	0x0001
#endif
#ifndef BCSS_STRETCH
#define BCSS_STRETCH	0x0002
#endif
#ifndef BCSS_ALIGNLEFT
#define BCSS_ALIGNLEFT	0x0004
#endif
#ifndef BCSS_IMAGE
#define BCSS_IMAGE	0x0008
#endif
#ifndef BCM_SETSPLITINFO
#define BCM_SETSPLITINFO         (BCM_FIRST + 0x0007)
#define Button_SetSplitInfo(hwnd, pInfo) \
    (BOOL)SNDMSG((hwnd), BCM_SETSPLITINFO, 0, (LPARAM)(pInfo))
#define BCM_GETSPLITINFO         (BCM_FIRST + 0x0008)
#define Button_GetSplitInfo(hwnd, pInfo) \
    (BOOL)SNDMSG((hwnd), BCM_GETSPLITINFO, 0, (LPARAM)(pInfo))
typedef struct tagBUTTON_SPLITINFO {
	UINT        mask;
	HIMAGELIST  himlGlyph;
	UINT        uSplitStyle;
	SIZE        size;
} BUTTON_SPLITINFO, *PBUTTON_SPLITINFO;
#endif

#endif /* _WIN32_WINNT < 0x0600 */

#endif /* __ROMPROPERTIES_WIN32_STDAFX_H__ */
