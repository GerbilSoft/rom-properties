/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DragImageLabel.cpp: Drag & Drop image label.                            *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DragImageLabel.hpp"
#include "RpImageWin32.hpp"

// libwin32common
#include "libwin32common/WinUI.hpp"

// librpbase
#include "librpbase/img/IconAnimData.hpp"
#include "librpbase/img/IconAnimHelper.hpp"
#include "librpbase/img/RpPngWriter.hpp"
using LibRpBase::IconAnimData;
using LibRpBase::IconAnimHelper;
using LibRpBase::RpPngWriter;

// librptexture
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;

// C includes. (C++ namespace)
#include <cassert>

// Gdiplus for image drawing.
// NOTE: Gdiplus requires min/max.
#include <algorithm>
namespace Gdiplus {
	using std::min;
	using std::max;
}
#include <gdiplus.h>

DragImageLabel::DragImageLabel(HWND hwndParent)
	: m_hwndParent(hwndParent)
	, m_useNearestNeighbor(false)
	, m_hUxTheme_dll(nullptr)
	, m_pfnIsThemeActive(nullptr)
	, m_img(nullptr)
	, m_hbmpImg(nullptr)
	, m_anim(nullptr)
{
	// TODO: Set rect/size as parameters?
	m_requiredSize.cx = DIL_REQ_IMAGE_SIZE;
	m_requiredSize.cy = DIL_REQ_IMAGE_SIZE;
	m_actualSize = m_requiredSize;

	m_position.x = 0;
	m_position.y = 0;

	m_rect.left = 0;
	m_rect.right = m_actualSize.cx;
	m_rect.top = 0;
	m_rect.bottom = m_actualSize.cy;

	// Attempt to get IsThemeActive() from uxtheme.dll.
	// TODO: Only do this once? (in ComBase)
	m_hUxTheme_dll = LoadLibrary(_T("uxtheme.dll"));
	if (m_hUxTheme_dll) {
		m_pfnIsThemeActive = (PFNISTHEMEACTIVE)GetProcAddress(m_hUxTheme_dll, "IsThemeActive");
	}
}

DragImageLabel::~DragImageLabel()
{
	delete m_anim;
	if (m_hbmpImg) {
		DeleteObject(m_hbmpImg);
	}

	// Close uxtheme.dll.
	if (m_hUxTheme_dll) {
		FreeLibrary(m_hUxTheme_dll);
	}
}

/**
 * Set the rp_image for this label.
 *
 * NOTE: The rp_image pointer is stored and used if necessary.
 * Make sure to call this function with nullptr before deleting
 * the rp_image object.
 *
 * NOTE 2: If animated icon data is specified, that supercedes
 * the individual rp_image.
 *
 * @param img rp_image, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool DragImageLabel::setRpImage(const rp_image *img)
{
	if (!img) {
		m_img = nullptr;
		if (m_hbmpImg) {
			DeleteBitmap(m_hbmpImg);
			m_hbmpImg = nullptr;
		}

		if (m_anim && m_anim->iconAnimData) {
			return updateBitmaps();
		}
		return false;
	}

	// Don't check if the image pointer matches the
	// previously stored image, since the underlying
	// image may have changed.
	m_img = img;
	return updateBitmaps();
}

/**
 * Set the icon animation data for this label.
 *
 * NOTE: The iconAnimData pointer is stored and used if necessary.
 * Make sure to call this function with nullptr before deleting
 * the IconAnimData object.
 *
 * NOTE 2: If animated icon data is specified, that supercedes
 * the individual rp_image.
 *
 * @param iconAnimData IconAnimData, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool DragImageLabel::setIconAnimData(const IconAnimData *iconAnimData)
{
	if (!m_anim) {
		m_anim = new anim_vars(m_hwndParent);
	}

	if (!iconAnimData) {
		if (m_anim->animTimerID) {
			KillTimer(m_hwndParent, m_anim->animTimerID);
			m_anim->animTimerID = 0;
		}
		m_anim->iconAnimData = nullptr;

		if (!m_img) {
			if (m_hbmpImg) {
				DeleteBitmap(m_hbmpImg);
				m_hbmpImg = nullptr;
			}
		} else {
			return updateBitmaps();
		}
		return false;
	}

	// Don't check if the data pointer matches the
	// previously stored data, since the underlying
	// data may have changed.
	m_anim->iconAnimData = iconAnimData;
	return updateBitmaps();
}

/**
 * Clear the rp_image and iconAnimData.
 * This will stop the animation timer if it's running.
 */
void DragImageLabel::clearRp(void)
{
	if (m_anim) {
		if (m_anim->animTimerID) {
			KillTimer(m_hwndParent, m_anim->animTimerID);
			m_anim->animTimerID = 0;
		}
		m_anim->iconAnimData = nullptr;
	}

	m_img = nullptr;
	if (m_hbmpImg) {
		DeleteBitmap(m_hbmpImg);
		m_hbmpImg = nullptr;
	}
}

/**
 * Rescale an image to be as close to the required size as possible.
 * @param req_sz	[in] Required size.
 * @param sz		[in/out] Image size.
 * @return True if nearest-neighbor scaling should be used (size was kept the same or enlarged); false if shrunken (so use interpolation).
 */
bool DragImageLabel::rescaleImage(const SIZE &req_sz, SIZE &sz)
{
	// TODO: Adjust req_sz for DPI.
	if (sz.cx == req_sz.cx && sz.cy == req_sz.cy) {
		// No resize necessary.
		return true;
	} else if (req_sz.cx == 0 || req_sz.cy == 0) {
		// Required size is 0, which means no rescaling.
		return true;
	} else if (sz.cx == 0 || sz.cy == 0) {
		// Image size is 0, which shouldn't happen...
		assert(!"Zero image size...");
		return true;
	}

	// Check if the image is too big.
	if (sz.cx >= req_sz.cx || sz.cy >= req_sz.cy) {
		// Image is too big. Shrink it.
		// FIXME: Assuming the icon is always a power of two.
		// Move TCreateThumbnail::rescale_aspect() into another file
		// and make use of that.
		sz = req_sz;
		return false;
	}

	// Image is too small.
	// TODO: Ensure dimensions don't exceed req_img_size.
	SIZE orig_sz = sz;
	do {
		// Increase by integer multiples until
		// the icon is at least 32x32.
		// TODO: Constrain to 32x32?
		sz.cx += orig_sz.cx;
		sz.cy += orig_sz.cy;
	} while (sz.cx < req_sz.cx && sz.cy < req_sz.cy);
	return true;
}

/**
 * Update the pixmap(s).
 * @return True on success; false on error.
 */
bool DragImageLabel::updateBitmaps(void)
{
	// Window background color.
	// Static controls don't support alpha transparency (?? test),
	// so we have to fake it.
	// TODO: Get the actual background color of the window.
	// TODO: Use DrawThemeBackground:
	// - http://www.codeproject.com/Articles/5978/Correctly-drawn-themed-dialogs-in-WinXP
	// - https://blogs.msdn.microsoft.com/dsui_team/2013/06/26/using-theme-apis-to-draw-the-border-of-a-control/
	// - https://blogs.msdn.microsoft.com/pareshj/2011/11/03/draw-the-background-of-static-control-with-gradient-fill-when-theme-is-enabled/
	int colorIndex;
	if (m_pfnIsThemeActive && m_pfnIsThemeActive()) {
		// Theme is active.
		colorIndex = COLOR_WINDOW;
	} else {
		// Theme is not active.
		colorIndex = COLOR_3DFACE;
	}
	const Gdiplus::ARGB gdipBgColor = LibWin32Common::GetSysColor_ARGB32(colorIndex);

	// Return value.
	bool bRet = false;

	// Clear cx so we know if we got a valid icon size.
	m_actualSize.cx = 0;

	if (m_anim && m_anim->iconAnimData) {
		const IconAnimData *const iconAnimData = m_anim->iconAnimData;

		// Convert the icons to HBITMAP using the window background color.
		// TODO: Rescale the icon. (port rescaleImage())
		for (int i = iconAnimData->count-1; i >= 0; i--) {
			const rp_image *const frame = iconAnimData->frames[i];
			if (frame && frame->isValid()) {
				if (m_actualSize.cx == 0) {
					// Get the icon size and rescale it, if necessary.
					m_actualSize.cx = frame->width();
					m_actualSize.cy = frame->height();
					m_useNearestNeighbor = rescaleImage(m_requiredSize, m_actualSize);
				}

				// NOTE: Allowing NULL frames here...
				m_anim->iconFrames[i] = RpImageWin32::toHBITMAP(frame, gdipBgColor, m_actualSize, m_useNearestNeighbor);
			}
		}

		// Set up the IconAnimHelper.
		m_anim->iconAnimHelper.setIconAnimData(iconAnimData);
		if (m_anim->iconAnimHelper.isAnimated()) {
			// Initialize the animation.
			m_anim->last_frame_number = m_anim->iconAnimHelper.frameNumber();

			// Icon animation timer is set in startAnimTimer().
		}

		// Image data is valid.
		updateRect();
		bRet = true;
	} else if (m_img && m_img->isValid()) {
		// Single image.
		// Convert to HBITMAP using the window background color.
		// TODO: Rescale the icon. (port rescaleImage())
		if (m_hbmpImg) {
			DeleteObject(m_hbmpImg);
		}

		// Get the icon size and rescale it, if necessary.
		m_actualSize.cx = m_img->width();
		m_actualSize.cy = m_img->height();
		m_useNearestNeighbor = rescaleImage(m_requiredSize, m_actualSize);

		m_hbmpImg = RpImageWin32::toHBITMAP(m_img, gdipBgColor, m_actualSize, m_useNearestNeighbor);

		// Image data is valid.
		updateRect();
		bRet = true;
	}

	// TODO: InvalidateRect()?
	return bRet;
}

/**
 * Start the animation timer.
 */
void DragImageLabel::startAnimTimer(void)
{
	if (!m_anim || !m_anim->iconAnimHelper.isAnimated()) {
		// Not an animated icon.
		return;
	}

	if (m_anim->animTimerID) {
		// Timer is already running.
		return;
	}

	// Get the current frame information.
	m_anim->last_frame_number = m_anim->iconAnimHelper.frameNumber();
	const int delay = m_anim->iconAnimHelper.frameDelay();
	assert(delay > 0);
	if (delay <= 0) {
		// Invalid delay value.
		return;
	}

	// Set a timer for the current frame.
	// We're using the 'this' pointer as nIDEvent.
	m_anim->animTimerID = SetTimer(m_hwndParent,
		reinterpret_cast<UINT_PTR>(this),
		delay, AnimTimerProc);
}

/**
 * Stop the animation timer.
 */
void DragImageLabel::stopAnimTimer(void)
{
	if (m_anim && m_anim->animTimerID) {
		KillTimer(m_hwndParent, m_anim->animTimerID);
		m_anim->animTimerID = 0;
	}
}

/**
 * Draw the image.
 * @param hdc Device context of the parent window.
 */
void DragImageLabel::draw(HDC hdc)
{
	HBITMAP hbmp = currentFrame();
	if (!hbmp) {
		// Nothing to draw...
		return;
	}

	// Memory DC for BitBlt.
	HDC hdcMem = CreateCompatibleDC(hdc);

	SelectBitmap(hdcMem, hbmp);
	BitBlt(hdc, m_position.x, m_position.y,
		m_actualSize.cx, m_actualSize.cy,
		hdcMem, 0, 0, SRCCOPY);

	DeleteDC(hdcMem);
}

/**
 * Invalidate the bitmap rect.
 * @param bErase Erase the background.
 */
void DragImageLabel::invalidateRect(bool bErase)
{
	InvalidateRect(m_hwndParent, &m_rect, bErase);
}

/**
 * Update the bitmap rect.
 * Called when position and/or size changes.
 */
void DragImageLabel::updateRect(void)
{
	// TODO: Add a bErase parameter to this function?

	// Invalidate the old rect.
	// TODO: Not if the new one completely overlaps the old one?
	InvalidateRect(m_hwndParent, &m_rect, false);

	// TODO: Optimize by not invalidating if it didn't change.
	m_rect.left   = m_position.x;
	m_rect.right  = m_position.x + m_actualSize.cx;
	m_rect.top    = m_position.y;
	m_rect.bottom = m_position.y + m_actualSize.cy;
	InvalidateRect(m_hwndParent, &m_rect, false);
}

/**
 * Animated icon timer.
 * @param hWnd
 * @param uMsg
 * @param idEvent
 * @param dwTime
 */
void CALLBACK DragImageLabel::AnimTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (hWnd == nullptr || idEvent == 0) {
		// Not a valid timer procedure call...
		// - hWnd should not be nullptr.
		// - idEvent should be the 'this' pointer.
		return;
	}

	DragImageLabel *const q =
		reinterpret_cast<DragImageLabel*>(idEvent);

	// Sanity checks.
	assert(q->m_hwndParent == hWnd);

	assert(q->m_anim != nullptr);
	if (!q->m_anim) {
		// Should not happen...
		return;
	}

	// Next frame.
	int delay = 0;
	int frame = q->m_anim->iconAnimHelper.nextFrame(&delay);
	if (delay <= 0 || frame < 0) {
		// Invalid frame...
		KillTimer(hWnd, idEvent);
		q->m_anim->animTimerID = 0;
		return;
	}

	if (frame != q->m_anim->last_frame_number) {
		// New frame number.
		// Update the icon.
		q->m_anim->last_frame_number = frame;
		InvalidateRect(hWnd, &q->m_rect, false);
	}

	// Update the timer.
	// TODO: Verify that this affects the next callback.
	SetTimer(hWnd, idEvent, delay, AnimTimerProc);
}
