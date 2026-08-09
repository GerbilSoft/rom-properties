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

#define WM_DIL_SET_RP_IMAGE		(WM_APP + 1)	// lParam == address of rp_image_const_ptr
#define WM_DIL_SET_ICON_ANIM_DATA	(WM_APP + 2)	// lParam == address of IconAnimDataConstPtr
#define WM_DIL_CLEAR_IMAGE		(WM_APP + 3)

#define WM_DIL_ANIM_TIMER_CTRL		(WM_APP + 4)	// wParam == 0 to stop; non-zero to start
#define WM_DIL_IS_ANIM_TIMER_RUNNING	(WM_APP + 5)	// return == 0 if animation timer is not running; non-zero if it is
#define WM_DIL_RESET_ANIM_FRAME		(WM_APP + 6)

#define WM_DIL_INVALIDATE_BITMAPS	(WM_APP + 7)

#define WM_DIL_SET_ECKS_BAWKS		(WM_APP + 8)	// wParam == 0 to clear Ecks Bawks mode; non-zero to set
#define WM_DIL_GET_ECKS_BAWKS		(WM_APP + 9)	// return == 0 if Ecks Bawks mode is not set; non-zero if set
#define WM_DIL_TRY_POPUP_ECKS_BAWKS	(WM_APP + 10)	// lParam == WM_RBUTTONUP lParam

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

// Other rom-properties libraries
#include "librpbase/img/IconAnimData.hpp"
#include "librptexture/img/rp_image.hpp"

static inline void DragImageLabel_SetRpImage(HWND hWnd, const LibRpTexture::rp_image_const_ptr &img)
{
	SendMessage(hWnd, WM_DIL_SET_RP_IMAGE, 0, reinterpret_cast<LPARAM>(&img));
}

static inline void DragImageLabel_SetIconAnimData(HWND hWnd, const LibRpBase::IconAnimDataConstPtr &img)
{
	SendMessage(hWnd, WM_DIL_SET_ICON_ANIM_DATA, 0, reinterpret_cast<LPARAM>(&img));
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

static inline void DragImageLabel_TryPopupEcksBawks(HWND hWnd, LPARAM lParam)
{
	SendMessage(hWnd, WM_DIL_TRY_POPUP_ECKS_BAWKS, 0, lParam);
}

#ifdef __cplusplus
}
#endif

#if 0
class DragImageLabelPrivate;
class DragImageLabel
{
public:
	explicit DragImageLabel(HWND hwndParent);
	~DragImageLabel();

private:
	DragImageLabelPrivate *const d_ptr;
	RP_DISABLE_COPY(DragImageLabel)

public:
	SIZE requiredSize(void) const;
	void setRequiredSize(SIZE requiredSize);
	void setRequiredSize(int width, int height);

	SIZE actualSize(void) const;

	POINT position(void) const;
	void setPosition(POINT position);
	void setPosition(int x, int y);

	/**
	 * Set the rp_image for this label.
	 * This will replace any previously set rp_image or IconAnimData.
	 *
	 * @param img rp_image, or nullptr to clear.
	 * @return True on success; false on error or if clearing.
	 */
	bool setRpImage(const LibRpTexture::rp_image_const_ptr &img);

	/**
	 * Set the icon animation data for this label.
	 * This will replace any previously set rp_image or IconAnimData.
	 *
	 * @param iconAnimData IconAnimData, or nullptr to clear.
	 * @return True on success; false on error or if clearing.
	 */
	bool setIconAnimData(const LibRpBase::IconAnimDataConstPtr &iconAnimData);

	/**
	 * Clear the rp_image and/or iconAnimData.
	 * This will stop the animation timer if it's running.
	 */
	void clearRp(void);

	bool ecksBawks(void) const;
	void setEcksBawks(bool newEcksBawks);

	void tryPopupEcksBawks(LPARAM lParam);

public:
	/**
	 * Start the animation timer.
	 */
	void startAnimTimer(void);

	/**
	 * Stop the animation timer.
	 */
	void stopAnimTimer(void);

	/**
	 * Is the animation timer running?
	 * @return True if running; false if not.
	 */
	bool isAnimTimerRunning(void) const;

	/**
	 * Reset the animation frame.
	 * This does NOT update the animation frame.
	 */
	void resetAnimFrame(void);

public:
	/**
	 * Get the current bitmap frame.
	 * @return HBITMAP.
	 */
	HBITMAP currentFrame(void) const;

	/**
	 * Draw the image.
	 * @param hdc Device context of the parent window.
	 */
	void draw(HDC hdc);

	/**
	 * Invalidate the bitmap rect.
	 * @param bErase Erase the background.
	 */
	void invalidateRect(bool bErase = false);

	/**
	 * Does a given rectangle intersect this control's rectangle?
	 * Typically used for WM_PAINT.
	 *
	 * @param lprcOther Rectangle to check
	 * @return True if it does; false if it doesn't.
	 */
	bool intersects(const RECT *lprcOther) const;
};
#endif
