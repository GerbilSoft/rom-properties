/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DragImageLabel.cpp: Drag & Drop image label.                            *
 *                                                                         *
 * Copyright (c) 2019-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "res/resource.h"

#include "DragImageLabel.hpp"
#include "RpImageWin32.hpp"

// Other rom-properties libraries
#include "librpbase/img/IconAnimHelper.hpp"
#include "librpbase/img/RpPngWriter.hpp"
using namespace LibRpBase;
using namespace LibRpTexture;

// libwin32common, libwin32ui
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/sdk/windowsx_ts.h"
#include "libwin32ui/HiDPI.hpp"
#include "libwin32ui/WinUI.hpp"
#include <shellapi.h>	// for ShellExecute()
#include <uxtheme.h>	// for IsThemeActive()

// Gdiplus for image drawing.
// NOTE: Gdiplus requires min/max.
#include <algorithm>
namespace Gdiplus {
	using std::min;
	using std::max;
}
#include <comdef.h>
#include <gdiplus.h>

#ifdef HAVE_STD_VARIANT
#  include <variant>
#else /* !HAVE_STD_VARIANT */
// std::variant<> is not available on this system.
// Use mpark variant instead.
#  include "mpark/variant.hpp"
namespace std {
	using mpark::variant;
	using mpark::holds_alternative;
	using mpark::get;
	using mpark::monostate;
}
#endif /* HAVE_STD_VARIANT */

class DragImageLabelPrivate
{
public:
	explicit DragImageLabelPrivate(HWND hwndParent);
	~DragImageLabelPrivate();

private:
	RP_DISABLE_COPY(DragImageLabelPrivate)

public:
	HWND hwndParent;

	// TODO: Eliminate actualSize()?
	SIZE requiredSize;	// Required icon size
	SIZE actualSize;	// Actual icon size, after rescaling (if necessary)
	RECT rect;		// RECT with specified position and actual icon size

	HMENU hMenuEcksBawks;

	// Non-animated icon data
	struct non_anim_vars_t {
		rp_image_const_ptr img;
		HBITMAP hbmpImg;

		explicit non_anim_vars_t()
			: hbmpImg(nullptr)
		{}
		explicit non_anim_vars_t(const rp_image_const_ptr &img)
			: img(img)
			, hbmpImg(nullptr)
		{}
		explicit non_anim_vars_t(rp_image_const_ptr &&img)
			: img(img)
			, hbmpImg(nullptr)
		{}

		~non_anim_vars_t()
		{
			if (hbmpImg) {
				DeleteBitmap(hbmpImg);
			}
		}
	};

	// Animated icon data
	struct anim_vars_t {
		IconAnimDataConstPtr iconAnimData;
		std::vector<HBITMAP> iconFrames;
		IconAnimHelper iconAnimHelper;
		HWND m_hwndParent;
		UINT_PTR animTimerID;
		int last_frame_number;		// Last frame number.

		explicit anim_vars_t(HWND hwndParent)
			: m_hwndParent(hwndParent)
			, animTimerID(0)
			, last_frame_number(0)
		{}
		explicit anim_vars_t(HWND hwndParent, const IconAnimDataConstPtr &iconAnimData)
			: m_hwndParent(hwndParent)
			, iconAnimData(iconAnimData)
			, animTimerID(0)
			, last_frame_number(0)
		{}
		explicit anim_vars_t(HWND hwndParent, IconAnimDataConstPtr &&iconAnimData)
			: m_hwndParent(hwndParent)
			, iconAnimData(iconAnimData)
			, animTimerID(0)
			, last_frame_number(0)
		{}

		~anim_vars_t()
		{
			if (animTimerID) {
				KillTimer(m_hwndParent, animTimerID);
			}
			for (HBITMAP hbmp : iconFrames) {
				if (hbmp) {
					DeleteBitmap(hbmp);
				}
			}
		}
	};

	std::variant<std::monostate, non_anim_vars_t, anim_vars_t> imgData;

	// Convenience functions to check both if the correct type is
	// set in the variant and if the shared_ptr is not nullptr.
	inline bool isAnim(void) const
	{
		if (std::holds_alternative<anim_vars_t>(imgData)) {
			const anim_vars_t &anim = std::get<anim_vars_t>(imgData);
			return static_cast<bool>(anim.iconAnimData);
		}
		return false;
	}
	inline bool isNonAnim(void) const
	{
		if (std::holds_alternative<non_anim_vars_t>(imgData)) {
			const non_anim_vars_t &non_anim = std::get<non_anim_vars_t>(imgData);
			return (non_anim.img && non_anim.img->isValid());
		}
		return false;
	}

	// Use nearest-neighbor scaling?
	bool useNearestNeighbor;
	bool ecksBawks;

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
	, hMenuEcksBawks(nullptr)
	, useNearestNeighbor(false)
	, ecksBawks(false)
{
	// TODO: Set rect/size as parameters?
	// TODO: Adjust on DPI change?
	const LONG iconSizeForDpi = rp_AdjustSizeForDpi(32, rp_GetDpiForWindow(hwndParent));

	requiredSize.cx = iconSizeForDpi;
	requiredSize.cy = iconSizeForDpi;
	actualSize.cx = iconSizeForDpi;
	actualSize.cy = iconSizeForDpi;

	rect.left = 0;
	rect.right = iconSizeForDpi;
	rect.top = 0;
	rect.bottom = iconSizeForDpi;
}

DragImageLabelPrivate::~DragImageLabelPrivate()
{
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
	const int colorIndex = IsThemeActive()
		? COLOR_WINDOW	// active theme
		: COLOR_3DFACE;	// no theme
	const Gdiplus::ARGB gdipBgColor = LibWin32UI::GetSysColor_ARGB32(colorIndex);

	// Return value.
	bool bRet = false;

	// Clear cx so we know if we got a valid icon size.
	actualSize.cx = 0;

	if (isAnim()) {
		anim_vars_t &anim = std::get<anim_vars_t>(imgData);
		const IconAnimDataConstPtr &iconAnimData = anim.iconAnimData;

		assert(iconAnimData->count > 0);
		assert(iconAnimData->count <= IconAnimData::MAX_FRAMES);
		if (iconAnimData->count <= 0 || iconAnimData->count > IconAnimData::MAX_FRAMES) {
			// Icon frame count is out of range...
			for (HBITMAP hbmp : anim.iconFrames) {
				if (hbmp) {
					DeleteBitmap(hbmp);
				}
			}
			anim.iconFrames.clear();
			return false;
		}
		// NOTE: Only increasing the vector size, not shrinking, because
		// shrinking would require deleting HBITMAPs.
		if (iconAnimData->count > static_cast<int>(anim.iconFrames.size())) {
			anim.iconFrames.resize(iconAnimData->count);
		}

		// Convert the icons to HBITMAP using the window background color.
		// TODO: Rescale the icon. (port rescaleImage())
		for (int i = iconAnimData->count-1; i >= 0; i--) {
			// Delete the existing HBITMAP first.
			// COMMIT NOTE: Do this before the rest of the vectorization.
			if (anim.iconFrames[i]) {
				DeleteBitmap(anim.iconFrames[i]);
				anim.iconFrames[i] = nullptr;
			}

			const rp_image_const_ptr &frame = iconAnimData->frames[i];
			if (frame && frame->isValid()) {
				if (actualSize.cx == 0) {
					// Get the icon size and rescale it, if necessary.
					actualSize.cx = frame->width();
					actualSize.cy = frame->height();
					useNearestNeighbor = rescaleImage(requiredSize, actualSize);
				}

				// NOTE: Allowing NULL frames here...
				anim.iconFrames[i] = RpImageWin32::toHBITMAP(frame, gdipBgColor, actualSize, useNearestNeighbor);
			}
		}

		// Set up the IconAnimHelper.
		IconAnimHelper &iconAnimHelper = anim.iconAnimHelper;
		iconAnimHelper.setIconAnimData(iconAnimData);
		if (iconAnimHelper.isAnimated()) {
			// Initialize the animation.
			anim.last_frame_number = iconAnimHelper.frameNumber();

			// Icon animation timer is set in startAnimTimer().
		}

		// Image data is valid.
		updateRect();
		bRet = true;
	} else if (isNonAnim()) {
		// Single image.
		// Convert to HBITMAP using the window background color.
		// TODO: Rescale the icon. (port rescaleImage())
		non_anim_vars_t &non_anim = std::get<non_anim_vars_t>(imgData);
		if (non_anim.hbmpImg) {
			DeleteBitmap(non_anim.hbmpImg);
		}

		// Get the icon size and rescale it, if necessary.
		actualSize.cx = non_anim.img->width();
		actualSize.cy = non_anim.img->height();
		useNearestNeighbor = rescaleImage(requiredSize, actualSize);

		non_anim.hbmpImg = RpImageWin32::toHBITMAP(non_anim.img, gdipBgColor, actualSize, useNearestNeighbor);

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

	// rect.left/rect.top already contains the actual position.
	// TODO: Optimize by not invalidating if it didn't change.
	rect.right  = rect.left + actualSize.cx;
	rect.bottom = rect.top  + actualSize.cy;
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

	assert(d->isAnim());
	if (!d->isAnim()) {
		// Should not happen...
		return;
	}

	// Next frame.
	DragImageLabelPrivate::anim_vars_t &anim = std::get<DragImageLabelPrivate::anim_vars_t>(d->imgData);
	int delay = 0;
	const int frame = anim.iconAnimHelper.nextFrame(&delay);
	if (delay <= 0 || frame < 0) {
		// Invalid frame...
		KillTimer(hWnd, idEvent);
		anim.animTimerID = 0;
		return;
	}

	if (frame != anim.last_frame_number) {
		// New frame number.
		// Update the icon.
		anim.last_frame_number = frame;
		InvalidateRect(hWnd, &d->rect, false);
	}

	// Update the timer.
	// TODO: Verify that this affects the next callback.
	SetTimer(hWnd, idEvent, delay, AnimTimerProc);
}

/** DragImageLabel **/

DragImageLabel::DragImageLabel(HWND hwndParent)
	: d_ptr(new DragImageLabelPrivate(hwndParent))
{}

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
	return { d->rect.left, d->rect.top };
}

void DragImageLabel::setPosition(POINT position)
{
	RP_D(DragImageLabel);
	if (d->rect.left != position.x ||
	    d->rect.top != position.y)
	{
		d->rect.left = position.x;
		d->rect.top = position.y;
		d->updateRect();
	}
}

void DragImageLabel::setPosition(int x, int y)
{
	RP_D(DragImageLabel);
	if (d->rect.left != x ||
	    d->rect.top != y)
	{
		d->rect.left = x;
		d->rect.top = y;
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
	if (!d->ecksBawks)
		return;
	if (d->hMenuEcksBawks)
		return;

	// NOTE: Need to get the submenu of this menu.
	d->hMenuEcksBawks = LoadMenu(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDR_ECKS_BAWKS));
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

	HMENU hSubMenu = GetSubMenu(d->hMenuEcksBawks, 0);
	assert(hSubMenu != nullptr);
	int id = TrackPopupMenu(hSubMenu,
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
		case IDM_ECKS_BAWKS_1:
			url = _T("https://twitter.com/DeaThProj/status/1684469412978458624");
			break;
		case IDM_ECKS_BAWKS_2:
			url = _T("https://github.com/xenia-canary/xenia-canary/pull/180");
			break;
	}

	if (url) {
		ShellExecute(nullptr, _T("open"), url, nullptr, nullptr, SW_SHOW);
	}
}

/**
 * Set the rp_image for this label.
 * This will replace any previously set rp_image or IconAnimData.
 *
 * @param img rp_image, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool DragImageLabel::setRpImage(const rp_image_const_ptr &img)
{
	// NOTE: We're not checking if the image pointer matches the
	// previously stored image, since the underlying image may
	// have changed.
	RP_D(DragImageLabel);
	d->imgData.emplace<DragImageLabelPrivate::non_anim_vars_t>(img);
	if (!img) {
		return false;
	}
	return d->updateBitmaps();
}

/**
 * Set the icon animation data for this label.
 * This will replace any previously set rp_image or IconAnimData.
 *
 * @param iconAnimData IconAnimData, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool DragImageLabel::setIconAnimData(const IconAnimDataConstPtr &iconAnimData)
{
	// NOTE: We're not checking if the image pointer matches the
	// previously stored image, since the underlying image may
	// have changed.
	RP_D(DragImageLabel);
	d->imgData.emplace<DragImageLabelPrivate::anim_vars_t>(d->hwndParent, iconAnimData);
	if (!iconAnimData) {
		return false;
	}
	return d->updateBitmaps();
}

/**
 * Clear the rp_image and/or iconAnimData.
 * This will stop the animation timer if it's running.
 */
void DragImageLabel::clearRp(void)
{
	RP_D(DragImageLabel);
	d->imgData.emplace<std::monostate>();
}

/**
 * Start the animation timer.
 */
void DragImageLabel::startAnimTimer(void)
{
	RP_D(DragImageLabel);
	if (!d->isAnim()) {
		// Not an animated icon.
		return;
	}

	DragImageLabelPrivate::anim_vars_t &anim = std::get<DragImageLabelPrivate::anim_vars_t>(d->imgData);
	const IconAnimHelper &iconAnimHelper = anim.iconAnimHelper;
	if (!iconAnimHelper.isAnimated()) {
		// Not an animated icon.
		return;
	}

	if (anim.animTimerID) {
		// Timer is already running.
		return;
	}

	// Get the current frame information.
	anim.last_frame_number = iconAnimHelper.frameNumber();
	const int delay = iconAnimHelper.frameDelay();
	assert(delay > 0);
	if (delay <= 0) {
		// Invalid delay value.
		return;
	}

	// Set a timer for the current frame.
	// We're using the 'd' pointer as nIDEvent.
	anim.animTimerID = SetTimer(d->hwndParent,
		reinterpret_cast<UINT_PTR>(d),
		delay, d->AnimTimerProc);
}

/**
 * Stop the animation timer.
 */
void DragImageLabel::stopAnimTimer(void)
{
	RP_D(DragImageLabel);
	if (!d->isAnim()) {
		// Not an animated icon.
		return;
	}

	DragImageLabelPrivate::anim_vars_t &anim = std::get<DragImageLabelPrivate::anim_vars_t>(d->imgData);
	if (anim.animTimerID) {
		KillTimer(d->hwndParent, anim.animTimerID);
		anim.animTimerID = 0;
	}
}

/**
 * Is the animation timer running?
 * @return True if running; false if not.
 */
bool DragImageLabel::isAnimTimerRunning(void) const
{
	RP_D(const DragImageLabel);
	if (!d->isAnim()) {
		// Not an animated icon.
		return false;
	}

	const DragImageLabelPrivate::anim_vars_t &anim = std::get<DragImageLabelPrivate::anim_vars_t>(d->imgData);
	return (anim.animTimerID != 0);
}

/**
 * Reset the animation frame.
 * This does NOT update the animation frame.
 */
void DragImageLabel::resetAnimFrame(void)
{
	RP_D(DragImageLabel);
	if (!d->isAnim()) {
		// Not an animated icon.
		return;
	}

	DragImageLabelPrivate::anim_vars_t &anim = std::get<DragImageLabelPrivate::anim_vars_t>(d->imgData);
	anim.last_frame_number = 0;
}

/**
 * Get the current bitmap frame.
 * @return HBITMAP.
 */
HBITMAP DragImageLabel::currentFrame(void) const
{
	RP_D(const DragImageLabel);
	if (d->isAnim()) {
		const DragImageLabelPrivate::anim_vars_t &anim = std::get<DragImageLabelPrivate::anim_vars_t>(d->imgData);
		const int frame = anim.iconAnimHelper.frameNumber();
		assert(frame >= 0);
		assert(frame < static_cast<int>(anim.iconFrames.size()));
		if (frame >= 0 && frame < static_cast<int>(anim.iconFrames.size())) {
			return anim.iconFrames[anim.last_frame_number];
		}
	} else if (d->isNonAnim()) {
		const DragImageLabelPrivate::non_anim_vars_t &non_anim = std::get<DragImageLabelPrivate::non_anim_vars_t>(d->imgData);
		return non_anim.hbmpImg;
	}

	return nullptr;
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
	BitBlt(hdc, d->rect.left, d->rect.top,
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

/**
 * Does a given rectangle intersect this control's rectangle?
 * Typically used for WM_PAINT.
 *
 * @param lprcOther Rectangle to check
 * @return True if it does; false if it doesn't.
 */
bool DragImageLabel::intersects(const RECT *lprcOther) const
{
	RP_D(const DragImageLabel);
	RECT rcIntersect;
	return (IntersectRect(&rcIntersect, &d->rect, lprcOther) != 0);
}
