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

// DIL DataObject (also handles IDropSource)
#include "DILDataObject.hpp"

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

static ATOM atom_dragImageLabel = 0;

class DragImageLabelPrivate
{
public:
	explicit DragImageLabelPrivate(HWND q);
	~DragImageLabelPrivate();

private:
	RP_DISABLE_COPY(DragImageLabelPrivate)
	HWND q_ptr;

public:
	SIZE ourLabelSize;	// Actual label size.

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
		HWND hwndDragImageLabel;
		UINT_PTR animTimerID;
		int last_frame_number;		// Last frame number.

		explicit anim_vars_t(HWND hwndDragImageLabel)
			: hwndDragImageLabel(hwndDragImageLabel)
			, animTimerID(0)
			, last_frame_number(0)
		{}
		explicit anim_vars_t(HWND hwndDragImageLabel, const IconAnimDataConstPtr &iconAnimData)
			: iconAnimData(iconAnimData)
			, hwndDragImageLabel(hwndDragImageLabel)
			, animTimerID(0)
			, last_frame_number(0)
		{}
		explicit anim_vars_t(HWND hwndDragImageLabel, IconAnimDataConstPtr &&iconAnimData)
			: iconAnimData(iconAnimData)
			, hwndDragImageLabel(hwndDragImageLabel)
			, animTimerID(0)
			, last_frame_number(0)
		{}

		~anim_vars_t()
		{
			if (animTimerID) {
				KillTimer(hwndDragImageLabel, animTimerID);
			}
			for (HBITMAP hbmp : iconFrames) {
				if (hbmp) {
					DeleteBitmap(hbmp);
				}
			}
		}

		/**
		 * Get frame 0.
		 * @return Frame 0, or nullptr on error.
		 */
		HBITMAP frame0(void) const
		{
			if (!iconAnimData || iconAnimData->seq_count <= 0) {
				// No animation sequence.
				// We might still have a static icon, though...
				if (iconFrames.size() >= 1) {
					return iconFrames[0];
				}
				// No icons at all.
				return nullptr;
			}

			const int frame0_idx = iconAnimData->seq_index[0];
			if (frame0_idx < 0 || frame0_idx >= static_cast<int>(iconFrames.size())) {
				return nullptr;
			}

			return iconFrames[frame0_idx];
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
	 * Update the bitmap(s).
	 * @return True on success; false on error.
	 */
	bool updateBitmaps(void);

	/**
	 * Get the current bitmap frame.
	 * @return HBITMAP.
	 */
	HBITMAP currentFrame(void) const;

	/**
	 * Animated icon timer.
	 * @param hWnd
	 * @param uMsg
	 * @param idEvent
	 * @param dwTime
	 */
	static void CALLBACK AnimTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

public:
	/** Internal message handlers **/

	/**
	 * WM_PAINT handler
	 */
	void on_WM_PAINT(void);

	/**
	 * WM_LBUTTONDOWN handler
	 * @param wParam
	 * @param lParam
	 */
	void on_WM_LBUTTONDOWN(WPARAM wParam, LPARAM lParam);

public:
	/** External message handlers **/

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

	void setEcksBawks(bool newEcksBawks);
	void tryPopupEcksBawks(LPARAM lParam);
};

/** DragImageLabelPrivate **/

DragImageLabelPrivate::DragImageLabelPrivate(HWND q)
	: q_ptr(q)
	, hMenuEcksBawks(nullptr)
	, useNearestNeighbor(false)
	, ecksBawks(false)
{}

DragImageLabelPrivate::~DragImageLabelPrivate()
{
	if (hMenuEcksBawks) {
		DestroyMenu(hMenuEcksBawks);
	}
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

	// Get the DragImageLabel size.
	RECT rectDragImageLabel;
	GetWindowRect(q_ptr, &rectDragImageLabel);
	MapWindowPoints(HWND_DESKTOP, GetParent(q_ptr), (LPPOINT)&rectDragImageLabel, 2);

	// Icon size (= 0x0 if not determined yet)
	SIZE iconSize = {0, 0};
	SIZE labelSize = {rectDragImageLabel.right, rectDragImageLabel.bottom};

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

			const rp_image_ptr &frame = iconAnimData->frames[i];
			if (frame && frame->isValid()) {
				// Get the icon size and rescale it, if necessary.
				if (iconSize.cx == 0) {
					iconSize.cx = frame->width();
					iconSize.cy = frame->height();

					// Calculate the new label width.
					labelSize.cx = static_cast<LONG>(rintf(
						static_cast<float>(labelSize.cy) * (static_cast<float>(iconSize.cx) /
							static_cast<float>(iconSize.cy))));

					// Use nearest-neighbor scaling if the label size is
					// an integer multiple of the icon size.
					useNearestNeighbor = ((labelSize.cx % iconSize.cx == 0) &&
					                      (labelSize.cy % iconSize.cy) == 0);
				}

				// NOTE: Allowing NULL frames here...
				anim.iconFrames[i] = RpImageWin32::toHBITMAP(frame, gdipBgColor, labelSize, useNearestNeighbor);
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
		//updateRect();
		bRet = true;
	} else if (isNonAnim()) {
		// Single image.
		// Convert to HBITMAP using the window background color.
		// TODO: Rescale the icon. (port rescaleImage())
		non_anim_vars_t &non_anim = std::get<non_anim_vars_t>(imgData);
		if (non_anim.hbmpImg) {
			DeleteBitmap(non_anim.hbmpImg);
		}

		iconSize.cx = non_anim.img->width();
		iconSize.cy = non_anim.img->height();

		// Calculate the new label width.
		labelSize.cx = static_cast<LONG>(rintf(
			static_cast<float>(labelSize.cy) * (static_cast<float>(iconSize.cx) /
				static_cast<float>(iconSize.cy))));

		// Use nearest-neighbor scaling if the label size is
		// an integer multiple of the icon size.
		useNearestNeighbor = ((labelSize.cx % iconSize.cx == 0) &&
		                      (labelSize.cy % iconSize.cy) == 0);

		non_anim.hbmpImg = RpImageWin32::toHBITMAP(non_anim.img, gdipBgColor, labelSize, useNearestNeighbor);

		// Image data is valid.
		//updateRect();
		bRet = true;
	}

	// Resize the DragImageLabel to match the required size.
	ourLabelSize = labelSize;
	SetWindowPos(q_ptr, nullptr, 0, 0, labelSize.cx, labelSize.cy,
		SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	return bRet;
}

/**
 * Start the animation timer.
 */
void DragImageLabelPrivate::startAnimTimer(void)
{
	if (!isAnim()) {
		// Not an animated icon.
		return;
	}

	DragImageLabelPrivate::anim_vars_t &anim = std::get<DragImageLabelPrivate::anim_vars_t>(imgData);
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
	anim.animTimerID = SetTimer(q_ptr,
		reinterpret_cast<UINT_PTR>(this),
		delay, AnimTimerProc);
}

/**
 * Stop the animation timer.
 */
void DragImageLabelPrivate::stopAnimTimer(void)
{
	if (!isAnim()) {
		// Not an animated icon.
		return;
	}

	DragImageLabelPrivate::anim_vars_t &anim = std::get<DragImageLabelPrivate::anim_vars_t>(imgData);
	if (anim.animTimerID) {
		KillTimer(q_ptr, anim.animTimerID);
		anim.animTimerID = 0;
	}
}

/**
 * Is the animation timer running?
 * @return True if running; false if not.
 */
bool DragImageLabelPrivate::isAnimTimerRunning(void) const
{
	if (!isAnim()) {
		// Not an animated icon.
		return false;
	}

	const DragImageLabelPrivate::anim_vars_t &anim = std::get<DragImageLabelPrivate::anim_vars_t>(imgData);
	return (anim.animTimerID != 0);
}

/**
 * Reset the animation frame.
 * This does NOT update the animation frame.
 */
void DragImageLabelPrivate::resetAnimFrame(void)
{
	if (!isAnim()) {
		// Not an animated icon.
		return;
	}

	DragImageLabelPrivate::anim_vars_t &anim = std::get<DragImageLabelPrivate::anim_vars_t>(imgData);
	anim.last_frame_number = 0;
}

/**
 * Get the current bitmap frame.
 * @return HBITMAP.
 */
HBITMAP DragImageLabelPrivate::currentFrame(void) const
{
	if (isAnim()) {
		const DragImageLabelPrivate::anim_vars_t &anim = std::get<DragImageLabelPrivate::anim_vars_t>(imgData);
		const int frame = anim.iconAnimHelper.frameNumber();
		assert(frame >= 0);
		assert(frame < static_cast<int>(anim.iconFrames.size()));
		if (frame >= 0 && frame < static_cast<int>(anim.iconFrames.size())) {
			return anim.iconFrames[anim.last_frame_number];
		}
	} else if (isNonAnim()) {
		const DragImageLabelPrivate::non_anim_vars_t &non_anim = std::get<DragImageLabelPrivate::non_anim_vars_t>(imgData);
		return non_anim.hbmpImg;
	}

	return nullptr;
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

	// Sanity checks
	assert(d->q_ptr == hWnd);
	if (d->q_ptr != hWnd) {
		// Should not happen...
		return;
	}

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
		InvalidateRect(hWnd, nullptr, false);
	}

	// Update the timer.
	// TODO: Verify that this affects the next callback.
	SetTimer(hWnd, idEvent, delay, AnimTimerProc);
}

/**
 * WM_PAINT handler
 */
void DragImageLabelPrivate::on_WM_PAINT(void)
{
	HBITMAP hbmp = currentFrame();
	if (!hbmp) {
		// Nothing to draw...
		return;
	}

	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(q_ptr, &ps);

	// Memory DC for BitBlt.
	HDC hdcMem = CreateCompatibleDC(hDC);
	SelectBitmap(hdcMem, hbmp);
	BitBlt(hDC, 0, 0, ourLabelSize.cx, ourLabelSize.cy, hdcMem, 0, 0, SRCCOPY);
	DeleteDC(hdcMem);

	EndPaint(q_ptr, &ps);
}

/**
 * WM_LBUTTONDOWN handler
 * @param wParam
 * @param lParam
 */
void DragImageLabelPrivate::on_WM_LBUTTONDOWN(WPARAM wParam, LPARAM lParam)
{
	// Check for a drag.
	const POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	if (!DragDetect(q_ptr, pt)) {
		// Not a drag.
		return;
	}

	// Start the drag operation.
	// TODO: Handle IconAnimData.
	DILDataObject *dataObj = nullptr;
	if (isAnim()) {
		const anim_vars_t &anim = std::get<anim_vars_t>(imgData);
		dataObj = new DILDataObject(anim.iconAnimData);
	} else if (isNonAnim()) {
		const non_anim_vars_t &non_anim = std::get<non_anim_vars_t>(imgData);
		dataObj = new DILDataObject(non_anim.img);
	} else {
		// No icon...
		return;
	}

	// TODO: Allow more than COPY? (everything will be handled as COPY regardless)
	DWORD effect = 0;
	HRESULT hr = DoDragDrop(dataObj, dataObj, DROPEFFECT_COPY, &effect);
	if (FAILED(hr)) {
		// DoDragDrop() failed...
		dataObj->Release();
		return;
	}
	dataObj->Release();
}

void DragImageLabelPrivate::setEcksBawks(bool newEcksBawks)
{
	ecksBawks = newEcksBawks;
	if (!ecksBawks) {
		return;
	}
	if (hMenuEcksBawks) {
		return;
	}

	// NOTE: Need to get the submenu of this menu.
	hMenuEcksBawks = LoadMenu(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDR_ECKS_BAWKS));
}

void DragImageLabelPrivate::tryPopupEcksBawks(LPARAM lParam)
{
	if (!ecksBawks || !hMenuEcksBawks) {
		return;
	}

	// Convert from local coordinates to screen coordinates.
	POINT pt = { LOWORD(lParam), HIWORD(lParam) };
	MapWindowPoints(q_ptr, HWND_DESKTOP, &pt, 1);

	HMENU hSubMenu = GetSubMenu(hMenuEcksBawks, 0);
	assert(hSubMenu != nullptr);
	int id = TrackPopupMenu(hSubMenu,
		TPM_LEFTALIGN | TPM_TOPALIGN | TPM_VERNEGANIMATION |
			TPM_NONOTIFY | TPM_RETURNCMD,
		pt.x, pt.y, 0, q_ptr, nullptr);

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

static LRESULT CALLBACK
DragImageLabelWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// FIXME: Don't use GWLP_USERDATA; use extra window bytes?
	switch (uMsg) {
		default:
			break;

		case WM_NCCREATE: {
			DragImageLabelPrivate *const d = new DragImageLabelPrivate(hWnd);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(d));
			break;
		}

		case WM_NCDESTROY: {
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			delete d;
			break;
		}

		case WM_SIZE: {
			// Label has been resized.
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!d) {
				// No DragImageLabelPrivate. Can't do anything...
				return false;
			}
			if (d->ourLabelSize.cx != LOWORD(lParam) ||
			    d->ourLabelSize.cy != HIWORD(lParam))
			{
				// Control size was changed.
				d->updateBitmaps();
			}
			break;
		}

		case WM_PAINT: {
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!d) {
				// No DragImageLabelPrivate. Can't do anything...
				return false;
			}

			d->on_WM_PAINT();
			break;
		}

		case WM_RBUTTONUP: {
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!d) {
				// No DragImageLabelPrivate. Can't do anything...
				return false;
			}

			d->tryPopupEcksBawks(lParam);

			// Don't bother running DefWindowProc here.
			return 0;
		}

		case WM_SYSCOLORCHANGE:
		case WM_THEMECHANGED: {
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			// TODO: Only schedule an update, and update it in the next WM_PAINT?
			d->updateBitmaps();
		}

		case WM_LBUTTONDOWN: {
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			d->on_WM_LBUTTONDOWN(wParam, lParam);

			// Don't bother running DefWindowProc here.
			return 0;
		}

		// Custom messages

		case WM_DIL_SET_RP_IMAGE: {
			// NOTE: We're not checking if the image pointer matches the
			// previously stored image, since the underlying image may
			// have changed.
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!d) {
				// No DragImageLabelPrivate. Can't do anything...
				return false;
			}

			bool ok = false;
			const rp_image_const_ptr *const pImg = reinterpret_cast<const rp_image_const_ptr*>(lParam);
			if (pImg) {
				d->imgData.emplace<DragImageLabelPrivate::non_anim_vars_t>(*pImg);
				if (*pImg) {
					ok = d->updateBitmaps();
				}
			}

			// Custom message; don't bother running DefWindowProc.
			return ok;
		}

		case WM_DIL_SET_ICON_ANIM_DATA: {
			// NOTE: We're not checking if the image pointer matches the
			// previously stored image, since the underlying image may
			// have changed.
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!d) {
				// No DragImageLabelPrivate. Can't do anything...
				return false;
			}

			bool ok = false;
			const IconAnimDataConstPtr *const pIconAnimData =
				reinterpret_cast<const IconAnimDataConstPtr*>(lParam);
			if (pIconAnimData) {
				d->imgData.emplace<DragImageLabelPrivate::anim_vars_t>(hWnd, *pIconAnimData);
				if (*pIconAnimData) {
					ok = d->updateBitmaps();
				}
			}

			// Custom message; don't bother running DefWindowProc.
			return ok;
		}

		case WM_DIL_CLEAR_IMAGE: {
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!d) {
				// No DragImageLabelPrivate. Can't do anything...
				return false;
			}

			d->imgData.emplace<std::monostate>();

			// Custom message; don't bother running DefWindowProc.
			return 0;
		}

		case WM_DIL_ANIM_TIMER_CTRL: {
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!d) {
				// No DragImageLabelPrivate. Can't do anything...
				return false;
			}

			if (wParam) {
				d->startAnimTimer();
			} else {
				d->stopAnimTimer();
			}

			// Custom message; don't bother running DefWindowProc.
			return 0;
		}

		case WM_DIL_IS_ANIM_TIMER_RUNNING: {
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!d) {
				// No DragImageLabelPrivate. Can't do anything...
				return false;
			}

			return d->isAnimTimerRunning();
		}

		case WM_DIL_RESET_ANIM_FRAME: {
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!d) {
				// No DragImageLabelPrivate. Can't do anything...
				return false;
			}

			d->resetAnimFrame();

			// Custom message; don't bother running DefWindowProc.
			return 0;
		}

		case WM_DIL_INVALIDATE_BITMAPS: {
			// Invalidate the bitmaps, possibly due to a theme change.
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!d) {
				// No DragImageLabelPrivate. Can't do anything...
				return false;
			}

			// TODO: Only schedule an update, and update it in the next WM_PAINT?
			d->updateBitmaps();

			// Custom message; don't bother running DefWindowProc.
			return 0;
		}

		case WM_DIL_SET_ECKS_BAWKS: {
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!d) {
				// No DragImageLabelPrivate. Can't do anything...
				return false;
			}

			d->setEcksBawks(static_cast<bool>(wParam));

			// Custom message; don't bother running DefWindowProc.
			return 0;
		}

		case WM_DIL_GET_ECKS_BAWKS: {
			DragImageLabelPrivate *const d = reinterpret_cast<DragImageLabelPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!d) {
				// No DragImageLabelPrivate. Can't do anything...
				return false;
			}

			return d->ecksBawks;
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/** WNDCLASS registration functions **/

void DragImageLabelRegister(void)
{
	if (atom_dragImageLabel != 0) {
		return;
	}

	WNDCLASS wndClass = {
		0,			// style
		DragImageLabelWndProc,	// lpfnWndProc
		0,			// cbClsExtra
		0,			// cbWndExtra
		HINST_THISCOMPONENT,	// hInstance
		nullptr,		// hIcon
		nullptr,		// hCursor
		nullptr,		// hbrBackground
		nullptr,		// lpszMenuName
		WC_DRAGIMAGELABEL	// lpszClassName
	};

	atom_dragImageLabel = RegisterClass(&wndClass);
}

void DragImageLabelUnregister(void)
{
	if (atom_dragImageLabel != 0) {
		UnregisterClass(MAKEINTATOM(atom_dragImageLabel), HINST_THISCOMPONENT);
		atom_dragImageLabel = 0;
	}
}
