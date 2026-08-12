/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DragImageLabel.hpp: Drag & Drop image label.                            *
 *                                                                         *
 * Copyright (c) 2019-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "libwin32common/RpWin32_sdk.h"

#include "stdboolx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WC_DRAGIMAGELABEL		_T("rp-DragImageLabel")

/** WNDCLASS registration functions **/
void DragImageLabelRegister(void);
void DragImageLabelUnregister(void);

// NOTE: DragImageLabel height is determined by the height set via
// CreateWindowEx() and/or SetWindowPos(). Width is calculated based
// on the height and aspect ratio.

#define WM_DIL_SET_RP_IMAGE		(WM_APP + 1)	// lParam == address of rp_image_const_ptr; return == true on success, false on error
#define WM_DIL_SET_ICON_ANIM_DATA	(WM_APP + 2)	// lParam == address of IconAnimDataConstPtr; return == true on success, false on error
#define WM_DIL_CLEAR_IMAGE		(WM_APP + 3)

#define WM_DIL_ANIM_TIMER_CTRL		(WM_APP + 4)	// wParam == 0 to stop; non-zero to start
#define WM_DIL_IS_ANIM_TIMER_RUNNING	(WM_APP + 5)	// return == 0 if animation timer is not running; non-zero if it is
#define WM_DIL_RESET_ANIM_FRAME		(WM_APP + 6)

#define WM_DIL_SET_DRAG_FILENAMEW	(WM_APP + 7)	// lParam = LPCWSTR of drag filename (e.g. ROM filename but with a .png extension) for dropped images
#define WM_DIL_SET_DRAG_FILENAMEA	(WM_APP + 8)	// lParam = LPCSTR of drag filename (e.g. ROM filename but with a .png extension) for dropped images
#define WM_DIL_SET_DRAG_MTIME		(WM_APP + 9)	// lParam = pointer to modification time (last write time) [FILETIME struct]

#define WM_DIL_INVALIDATE_BITMAPS	(WM_APP + 10)

#define WM_DIL_SET_ECKS_BAWKS		(WM_APP + 11)	// wParam == 0 to clear Ecks Bawks mode; non-zero to set
#define WM_DIL_GET_ECKS_BAWKS		(WM_APP + 12)	// return == 0 if Ecks Bawks mode is not set; non-zero if set

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

// Other rom-properties libraries
#include "librpbase/img/IconAnimData.hpp"
#include "librptexture/img/rp_image.hpp"

static inline bool DragImageLabel_SetRpImage(HWND hWnd, const LibRpTexture::rp_image_const_ptr &img)
{
	return static_cast<bool>(SendMessage(hWnd, WM_DIL_SET_RP_IMAGE, 0, reinterpret_cast<LPARAM>(&img)));
}

static inline bool DragImageLabel_SetIconAnimData(HWND hWnd, const LibRpBase::IconAnimDataConstPtr &iconAnimData)
{
	return static_cast<bool>(SendMessage(hWnd, WM_DIL_SET_ICON_ANIM_DATA, 0, reinterpret_cast<LPARAM>(&iconAnimData)));
}

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

static inline void DragImageLabel_ClearImage(HWND hWnd)
{
	SendMessage(hWnd, WM_DIL_CLEAR_IMAGE, 0, 0);
}

static inline void DragImageLabel_AnimTimerCtrl(HWND hWnd, bool ctrl)
{
	SendMessage(hWnd, WM_DIL_ANIM_TIMER_CTRL, static_cast<WPARAM>(ctrl), 0);
}

static inline bool DragImageLabel_IsAnimTimerRunning(HWND hWnd)
{
	return (bool)SendMessage(hWnd, WM_DIL_IS_ANIM_TIMER_RUNNING, 0, 0);
}

static inline void DragImageLabel_ResetAnimFrame(HWND hWnd)
{
	SendMessage(hWnd, WM_DIL_RESET_ANIM_FRAME, 0, 0);
}

static inline void DragImageLabel_SetDragFileNameW(HWND hWnd, LPCWSTR dragFilename)
{
	SendMessage(hWnd, WM_DIL_SET_DRAG_FILENAMEW, 0, reinterpret_cast<LPARAM>(dragFilename));
}

static inline void DragImageLabel_SetDragFileNameA(HWND hWnd, LPCSTR dragFilename)
{
	SendMessage(hWnd, WM_DIL_SET_DRAG_FILENAMEA, 0, reinterpret_cast<LPARAM>(dragFilename));
}

static inline void DragImageLabel_SetDragMTime(HWND hWnd, const FILETIME *mtime)
{
	SendMessage(hWnd, WM_DIL_SET_DRAG_MTIME, 0, reinterpret_cast<LPARAM>(mtime));
}

static inline void DragImageLabel_InvalidateBitmaps(HWND hWnd)
{
	SendMessage(hWnd, WM_DIL_INVALIDATE_BITMAPS, 0, 0);
}

static inline void DragImageLabel_SetEcksBawks(HWND hWnd, bool ecksBawks)
{
	SendMessage(hWnd, WM_DIL_SET_ECKS_BAWKS, static_cast<WPARAM>(ecksBawks), 0);
}

static inline bool DragImageLabel_GetEcksBawks(HWND hWnd)
{
	return (bool)SendMessage(hWnd, WM_DIL_GET_ECKS_BAWKS, 0, 0);
}

#ifdef __cplusplus
}
#endif

// Unicode macros
#ifdef UNICODE
#  define WM_DIL_SET_DRAG_FILENAME WM_DIL_SET_DRAG_FILENAMEW
#  define DragImageLabel_SetDragFileName(hWnd, dragFilename) DragImageLabel_SetDragFileNameW((hWnd), (dragFilename))
#else /* !UNICODE */
#  define WM_DIL_SET_DRAG_FILENAME WM_DIL_SET_DRAG_FILENAMEA
#  define DragImageLabel_SetDragFileName(hWnd, dragFilename) DragImageLabel_SetDragFileNameA((hWnd), (dragFilename))
#endif /* UNICODE */
