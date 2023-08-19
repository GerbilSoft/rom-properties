/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DragImageLabel.cpp: Drag & Drop image label.                            *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DragImageLabel.hpp"
#include "RpImageWin32.hpp"

// Other rom-properties libraries
#include "librpbase/img/IconAnimData.hpp"
#include "librpbase/img/IconAnimHelper.hpp"
#include "librpbase/img/RpPngWriter.hpp"
using namespace LibRpBase;
using namespace LibRpTexture;

// Gdiplus for image drawing.
// NOTE: Gdiplus requires min/max.
#include <algorithm>
namespace Gdiplus {
	using std::min;
	using std::max;
}
#include <gdiplus.h>

class DragImageLabelPrivate
{
	public:
		explicit DragImageLabelPrivate(HWND hwndParent);
		~DragImageLabelPrivate();

	private:
		RP_DISABLE_COPY(DragImageLabelPrivate)

	public:
		HWND hwndParent;

		// Position
		POINT position;

		// Icon sizes
		SIZE requiredSize;	// Required size.
		SIZE actualSize;	// Actual size.

		// Calculated RECT based on position and size
		RECT rect;

		bool ecksBawks;
		HMENU hMenuEcksBawks;

		// rp_image
		rp_image_const_ptr img;
		HBITMAP hbmpImg;	// for non-animated only

		// Animated icon data.
		struct anim_vars {
			const LibRpBase::IconAnimData *iconAnimData;
			std::array<HBITMAP, IconAnimData::MAX_FRAMES> iconFrames;
			IconAnimHelper iconAnimHelper;
			HWND m_hwndParent;
			UINT_PTR animTimerID;
			int last_frame_number;		// Last frame number.

			explicit anim_vars(HWND hwndParent)
				: iconAnimData(nullptr)
				, m_hwndParent(hwndParent)
				, animTimerID(0)
				, last_frame_number(0)
			{
				iconFrames.fill(nullptr);
			}
			~anim_vars()
			{
				if (animTimerID) {
					KillTimer(m_hwndParent, animTimerID);
				}
				for (HBITMAP hbmp : iconFrames) {
					if (hbmp) {
						DeleteBitmap(hbmp);
					}
				}
				UNREF(iconAnimData);
			}
		};
		anim_vars *anim;

		// Use nearest-neighbor scaling?
		bool useNearestNeighbor;

	public:
		/**
		 * Rescale an image to be as close to the required size as possible.
		 * @param req_sz	[in] Required size.
		 * @param sz		[in/out] Image size.
		 * @return True if nearest-neighbor scaling should be used (size was kept the same or enlarged); false if shrunken (so use interpolation).
		 */
		static bool rescaleImage(SIZE req_sz, SIZE &sz);

		/**
		 * Update the bitmap(s).
		 * @return True on success; false on error.
		 */
		bool updateBitmaps(void);

		/**
		 * Update the bitmap rect.
		 * Called when position and/or size changes.
		 */
		void updateRect(void);

		/**
		 * Animated icon timer.
		 * @param hWnd
		 * @param uMsg
		 * @param idEvent
		 * @param dwTime
		 */
		static void CALLBACK AnimTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
};

/** DragImageLabelPrivate **/

DragImageLabelPrivate::DragImageLabelPrivate(HWND hwndParent)
	: hwndParent(hwndParent)
	, ecksBawks(false)
	, hMenuEcksBawks(nullptr)
	, hbmpImg(nullptr)
	, anim(nullptr)
	, useNearestNeighbor(false)
{
	// TODO: Set rect/size as parameters?
	requiredSize.cx = DIL_REQ_IMAGE_SIZE;
	requiredSize.cy = DIL_REQ_IMAGE_SIZE;
	actualSize = requiredSize;

	position.x = 0;
	position.y = 0;

	rect.left = 0;
	rect.right = actualSize.cx;
	rect.top = 0;
	rect.bottom = actualSize.cy;
}

DragImageLabelPrivate::~DragImageLabelPrivate()
{
	delete anim;

	if (hbmpImg) {
		DeleteBitmap(hbmpImg);
	}

	if (hMenuEcksBawks) {
		DestroyMenu(hMenuEcksBawks);
	}
}

/**
 * Rescale an image to be as close to the required size as possible.
 * @param req_sz	[in] Required size.
 * @param sz		[in/out] Image size.
 * @return True if nearest-neighbor scaling should be used (size was kept the same or enlarged); false if shrunken (so use interpolation).
 */
bool DragImageLabelPrivate::rescaleImage(SIZE req_sz, SIZE &sz)
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
	const SIZE orig_sz = sz;
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
 * Update the bitmap(s).
 * @return True on success; false on error.
 */
bool DragImageLabelPrivate::updateBitmaps(void)
{
	// Window background color.
	// Static controls don't support alpha transparency (?? test),
	// so we have to fake it.
	// TODO: Get the actual background color of the window.
	// TODO: Use DrawThemeBackground:
	// - http://www.codeproject.com/Articles/5978/Correctly-drawn-themed-dialogs-in-WinXP
	// - https://docs.microsoft.com/en-us/archive/blogs/dsui_team/using-theme-apis-to-draw-the-border-of-a-control
	// - https://docs.microsoft.com/en-us/archive/blogs/pareshj/draw-the-background-of-static-control-with-gradient-fill-when-theme-is-enabled
	const int colorIndex = LibWin32UI::isThemeActive()
		? COLOR_WINDOW	// active theme
		: COLOR_3DFACE;	// no theme
	const Gdiplus::ARGB gdipBgColor = LibWin32UI::GetSysColor_ARGB32(colorIndex);

	// Return value.
	bool bRet = false;

	// Clear cx so we know if we got a valid icon size.
	actualSize.cx = 0;

	if (anim && anim->iconAnimData) {
		const IconAnimData *const iconAnimData = anim->iconAnimData;

		// Convert the icons to HBITMAP using the window background color.
		// TODO: Rescale the icon. (port rescaleImage())
		for (int i = iconAnimData->count-1; i >= 0; i--) {
			const rp_image_const_ptr &frame = iconAnimData->frames[i];
			if (frame && frame->isValid()) {
				if (actualSize.cx == 0) {
					// Get the icon size and rescale it, if necessary.
					actualSize.cx = frame->width();
					actualSize.cy = frame->height();
					useNearestNeighbor = rescaleImage(requiredSize, actualSize);
				}

				// NOTE: Allowing NULL frames here...
				anim->iconFrames[i] = RpImageWin32::toHBITMAP(frame, gdipBgColor, actualSize, useNearestNeighbor);
			}
		}

		// Set up the IconAnimHelper.
		anim->iconAnimHelper.setIconAnimData(iconAnimData);
		if (anim->iconAnimHelper.isAnimated()) {
			// Initialize the animation.
			anim->last_frame_number = anim->iconAnimHelper.frameNumber();

			// Icon animation timer is set in startAnimTimer().
		}

		// Image data is valid.
		updateRect();
		bRet = true;
	} else if (img && img->isValid()) {
		// Single image.
		// Convert to HBITMAP using the window background color.
		// TODO: Rescale the icon. (port rescaleImage())
		if (hbmpImg) {
			DeleteBitmap(hbmpImg);
		}

		// Get the icon size and rescale it, if necessary.
		actualSize.cx = img->width();
		actualSize.cy = img->height();
		useNearestNeighbor = rescaleImage(requiredSize, actualSize);

		hbmpImg = RpImageWin32::toHBITMAP(img, gdipBgColor, actualSize, useNearestNeighbor);

		// Image data is valid.
		updateRect();
		bRet = true;
	}

	return bRet;
}

/**
 * Update the bitmap rect.
 * Called when position and/or size changes.
 */
void DragImageLabelPrivate::updateRect(void)
{
	// TODO: Add a bErase parameter to this function?

	// Invalidate the old rect.
	// TODO: Not if the new one completely overlaps the old one?
	InvalidateRect(hwndParent, &rect, false);

	// TODO: Optimize by not invalidating if it didn't change.
	rect.left   = position.x;
	rect.right  = position.x + actualSize.cx;
	rect.top    = position.y;
	rect.bottom = position.y + actualSize.cy;
	InvalidateRect(hwndParent, &rect, false);
}

/**
 * Animated icon timer.
 * @param hWnd
 * @param uMsg
 * @param idEvent
 * @param dwTime
 */
void CALLBACK DragImageLabelPrivate::AnimTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	((void)uMsg);
	((void)dwTime);

	if (hWnd == nullptr || idEvent == 0) {
		// Not a valid timer procedure call...
		// - hWnd should not be nullptr.
		// - idEvent should be the 'this' pointer.
		return;
	}

	DragImageLabelPrivate *const d =
		reinterpret_cast<DragImageLabelPrivate*>(idEvent);

	// Sanity checks.
	assert(d->hwndParent == hWnd);

	assert(d->anim != nullptr);
	if (!d->anim) {
		// Should not happen...
		return;
	}

	// Next frame.
	int delay = 0;
	const int frame = d->anim->iconAnimHelper.nextFrame(&delay);
	if (delay <= 0 || frame < 0) {
		// Invalid frame...
		KillTimer(hWnd, idEvent);
		d->anim->animTimerID = 0;
		return;
	}

	if (frame != d->anim->last_frame_number) {
		// New frame number.
		// Update the icon.
		d->anim->last_frame_number = frame;
		InvalidateRect(hWnd, &d->rect, false);
	}

	// Update the timer.
	// TODO: Verify that this affects the next callback.
	SetTimer(hWnd, idEvent, delay, AnimTimerProc);
}

/** DragImageLabel **/

DragImageLabel::DragImageLabel(HWND hwndParent)
	: d_ptr(new DragImageLabelPrivate(hwndParent))
{ }

DragImageLabel::~DragImageLabel()
{
	delete d_ptr;
}

SIZE DragImageLabel::requiredSize(void) const
{
	RP_D(const DragImageLabel);
	return d->requiredSize;
}

void DragImageLabel::setRequiredSize(SIZE requiredSize)
{
	RP_D(DragImageLabel);
	if (d->requiredSize.cx != requiredSize.cx ||
	    d->requiredSize.cy != requiredSize.cy)
	{
		d->requiredSize = requiredSize;
		d->updateBitmaps();
	}
}

void DragImageLabel::setRequiredSize(int width, int height)
{
	RP_D(DragImageLabel);
	if (d->requiredSize.cx != width ||
	    d->requiredSize.cy != height)
	{
		d->requiredSize.cx = width;
		d->requiredSize.cy = height;
		d->updateBitmaps();
	}
}

SIZE DragImageLabel::actualSize(void) const
{
	RP_D(const DragImageLabel);
	return d->actualSize;
}

POINT DragImageLabel::position(void) const
{
	RP_D(const DragImageLabel);
	return d->position;
}

void DragImageLabel::setPosition(POINT position)
{
	RP_D(DragImageLabel);
	if (d->position.x != position.x ||
	    d->position.y != position.y)
	{
		d->position = position;
		d->updateRect();
	}
}

void DragImageLabel::setPosition(int x, int y)
{
	RP_D(DragImageLabel);
	if (d->position.x != x ||
	    d->position.y != y)
	{
		d->position.x = x;
		d->position.y = y;
		d->updateRect();
	}
}

bool DragImageLabel::ecksBawks(void) const
{
	RP_D(const DragImageLabel);
	return d->ecksBawks;
}

void DragImageLabel::setEcksBawks(bool newEcksBawks)
{
	RP_D(DragImageLabel);
	d->ecksBawks = newEcksBawks;
	if (d->ecksBawks && d->hMenuEcksBawks) {
		// Already created the Ecks Bawks menu.
		return;
	}

	d->hMenuEcksBawks = CreatePopupMenu();
	AppendMenu(d->hMenuEcksBawks, MF_STRING, IDM_ECKS_BAWKS_MENU_BASE + 1,
		_T("ermahgerd! an ecks bawks ISO!"));
	AppendMenu(d->hMenuEcksBawks, MF_STRING, IDM_ECKS_BAWKS_MENU_BASE + 2,
		_T("Yar, har, fiddle dee dee"));
}

void DragImageLabel::tryPopupEcksBawks(LPARAM lParam)
{
	RP_D(const DragImageLabel);
	if (!d->ecksBawks || !d->hMenuEcksBawks)
		return;

	POINT pt = { LOWORD(lParam), HIWORD(lParam) };
	if (!PtInRect(&d->rect, pt))
		return;

	// Convert from local coordinates to screen coordinates.
	MapWindowPoints(d->hwndParent, HWND_DESKTOP, &pt, 1);

	int id = TrackPopupMenu(d->hMenuEcksBawks,
		TPM_LEFTALIGN | TPM_TOPALIGN | TPM_VERNEGANIMATION |
			TPM_NONOTIFY | TPM_RETURNCMD,
		pt.x, pt.y, 0, d->hwndParent, nullptr);

	LPCTSTR url = nullptr;
	switch (id) {
		default:
			assert(!"Invalid ecksbawks URL ID.");
			break;
		case 0:		// No item selected
			break;
		case IDM_ECKS_BAWKS_MENU_BASE + 1:
			url = _T("https://twitter.com/DeaThProj/status/1684469412978458624");
			break;
		case IDM_ECKS_BAWKS_MENU_BASE + 2:
			url = _T("https://github.com/xenia-canary/xenia-canary/pull/180");
			break;
	}

	if (url) {
		ShellExecute(nullptr, _T("open"), url, nullptr, nullptr, SW_SHOW);
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
bool DragImageLabel::setRpImage(const rp_image_const_ptr &img)
{
	RP_D(DragImageLabel);

	d->img = img;
	if (!img) {
		if (d->hbmpImg) {
			DeleteBitmap(d->hbmpImg);
			d->hbmpImg = nullptr;
		}

		if (d->anim && d->anim->iconAnimData) {
			return d->updateBitmaps();
		}
		return false;
	}
	return d->updateBitmaps();
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
	RP_D(DragImageLabel);

	if (!d->anim) {
		d->anim = new DragImageLabelPrivate::anim_vars(d->hwndParent);
	}
	auto *const anim = d->anim;

	// NOTE: We're not checking if the image pointer matches the
	// previously stored image, since the underlying image may
	// have changed.
	UNREF_AND_NULL(anim->iconAnimData);

	if (!iconAnimData) {
		if (anim->animTimerID) {
			KillTimer(d->hwndParent, anim->animTimerID);
			anim->animTimerID = 0;
		}
		anim->iconAnimData = nullptr;

		if (!d->img) {
			if (d->hbmpImg) {
				DeleteBitmap(d->hbmpImg);
				d->hbmpImg = nullptr;
			}
		} else {
			return d->updateBitmaps();
		}
		return false;
	}

	anim->iconAnimData = iconAnimData->ref();
	return d->updateBitmaps();
}

/**
 * Clear the rp_image and iconAnimData.
 * This will stop the animation timer if it's running.
 */
void DragImageLabel::clearRp(void)
{
	RP_D(DragImageLabel);
	if (d->anim) {
		if (d->anim->animTimerID) {
			KillTimer(d->hwndParent, d->anim->animTimerID);
			d->anim->animTimerID = 0;
		}
		d->anim->iconAnimData = nullptr;
	}

	d->img.reset();
	if (d->hbmpImg) {
		DeleteBitmap(d->hbmpImg);
		d->hbmpImg = nullptr;
	}
}

/**
 * Start the animation timer.
 */
void DragImageLabel::startAnimTimer(void)
{
	RP_D(DragImageLabel);
	if (!d->anim || !d->anim->iconAnimHelper.isAnimated()) {
		// Not an animated icon.
		return;
	}

	if (d->anim->animTimerID) {
		// Timer is already running.
		return;
	}

	// Get the current frame information.
	d->anim->last_frame_number = d->anim->iconAnimHelper.frameNumber();
	const int delay = d->anim->iconAnimHelper.frameDelay();
	assert(delay > 0);
	if (delay <= 0) {
		// Invalid delay value.
		return;
	}

	// Set a timer for the current frame.
	// We're using the 'd' pointer as nIDEvent.
	d->anim->animTimerID = SetTimer(d->hwndParent,
		reinterpret_cast<UINT_PTR>(d),
		delay, d->AnimTimerProc);
}

/**
 * Stop the animation timer.
 */
void DragImageLabel::stopAnimTimer(void)
{
	RP_D(DragImageLabel);
	if (d->anim && d->anim->animTimerID) {
		KillTimer(d->hwndParent, d->anim->animTimerID);
		d->anim->animTimerID = 0;
	}
}

/**
 * Is the animation timer running?
 * @return True if running; false if not.
 */
bool DragImageLabel::isAnimTimerRunning(void) const
{
	RP_D(const DragImageLabel);
	return (d->anim && d->anim->animTimerID);
}

/**
 * Reset the animation frame.
 * This does NOT update the animation frame.
 */
void DragImageLabel::resetAnimFrame(void)
{
	RP_D(DragImageLabel);
	if (d->anim) {
		d->anim->last_frame_number = 0;
	}
}

/**
 * Get the current bitmap frame.
 * @return HBITMAP.
 */
HBITMAP DragImageLabel::currentFrame(void) const
{
	RP_D(const DragImageLabel);
	if (d->anim && d->anim->iconAnimData) {
		return d->anim->iconFrames[d->anim->last_frame_number];
	}
	return d->hbmpImg;
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

	RP_D(DragImageLabel);
	SelectBitmap(hdcMem, hbmp);
	BitBlt(hdc, d->position.x, d->position.y,
		d->actualSize.cx, d->actualSize.cy,
		hdcMem, 0, 0, SRCCOPY);

	DeleteDC(hdcMem);
}

/**
 * Invalidate the bitmap rect.
 * @param bErase Erase the background.
 */
void DragImageLabel::invalidateRect(bool bErase)
{
	RP_D(DragImageLabel);
	InvalidateRect(d->hwndParent, &d->rect, bErase);
}
