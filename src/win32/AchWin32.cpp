/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * AchWin32.hpp: Win32 notifications for achievements.                     *
 *                                                                         *
 * Copyright (c) 2020-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchWin32.hpp"

#include "AchSpriteSheet.hpp"
#include "RpImageWin32.hpp"
#include "dll-macros.h"

// Other rom-properties libraries
#include "librpbase/Achievements.hpp"
#include "librptext/wchar.hpp"
using namespace LibRpBase;

// ROM icon
#include "config/PropSheetIcon.hpp"

// C++ STL classes
#include <mutex>
using std::string;
using std::tstring;
using std::unordered_map;

class AchWin32Private
{
public:
	AchWin32Private();
	~AchWin32Private();

private:
	RP_DISABLE_COPY(AchWin32Private)

public:
	// Static AchWin32 instance.
	// TODO: Q_GLOBAL_STATIC() equivalent, though we
	// may need special initialization if the compiler
	// doesn't support thread-safe statics.
	static AchWin32 instance;
	bool hasRegistered;

	// GetWindowLongPtr() offsets
	static constexpr int GWLP_ACHWIN32_D = 0;
	static constexpr int GWLP_ACHWIN32_NID_UID = static_cast<int>(sizeof(LONG_PTR));

	// Timeout for the achievement popup. (in ms)
	static constexpr unsigned int ACHWIN32_TIMEOUT = 10U * 1000U;

	// Window message for NOTIFYICONDATA.
	static constexpr unsigned int WM_ACHWIN32_NOTIFY = WM_USER + 69;	// nice

	// Icon ID high word.
	static constexpr DWORD ACHWIN32_NID_UID_HI = 0x19840000;

public:
	/**
	 * Notification function. (static)
	 * @param user_data	[in] User data (this)
	 * @param id		[in] Achievement ID
	 * @return 0 on success; negative POSIX error code on error.
	 */
	static int RP_C_API notifyFunc(void *user_data, Achievements::ID id);

	/**
	 * Notification function. (non-static)
	 * @param id	[in] Achievement ID.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int notifyFunc(Achievements::ID id);

private:
	/**
	 * Register the window class.
	 * NOTE: Must be called from std::call_once().
	 */
	static void registerWindowClass(void);

	/**
	 * Remove a window from tracking.
	 * This also removes the notification icon.
	 * @param hWnd
	 */
	static void removeWindowFromTracking(HWND hWnd);

	/**
	 * RpAchNotifyWnd window procedure.
	 * @param hWnd
	 * @param uMsg
	 * @param wParam
	 * @param lParam
	 */
	static LRESULT CALLBACK RpAchNotifyWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	// Window class (registered once)
	ATOM atomWindowClass;
	std::once_flag once_flag;

	// NOTE: Windows Explorer appears to create a new thread per
	// properties dialog, and the thread (and this window) disappears
	// when the associated properties dialog is closed. Hence, we'll
	// need to use a map with thread IDs.
	unordered_map<DWORD, HWND> map_tidToHWND;
	unordered_map<HWND, DWORD> map_hWndToTID;
};

/** AchWin32Private **/

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
AchWin32 AchWin32Private::instance;

AchWin32Private::AchWin32Private()
	: hasRegistered(false)
	, atomWindowClass(0)
{
	// NOTE: Cannot register with the Achievements class here because the
	// static Achievements instance might not be fully initialized yet.
}

AchWin32Private::~AchWin32Private()
{
	if (hasRegistered) {
		Achievements *const pAch = Achievements::instance();
		pAch->clearNotifyFunction(notifyFunc, this);
	}

	// TODO: Verify that the threads are still valid.
	for (const auto &pair : map_tidToHWND) {
		// Zero out the user data to prevent WM_NCDESTROY from
		// attempting to modify the maps.
		HWND hWnd = pair.second;
		SetWindowLongPtr(hWnd, GWLP_ACHWIN32_D, 0);

		// Now destroy the window.
		DestroyWindow(hWnd);
	}

	if (atomWindowClass > 0) {
		UnregisterClass(MAKEINTATOM(atomWindowClass), HINST_THISCOMPONENT);
	}
}

/**
 * Notification function. (static)
 * @param user_data	[in] User data (this)
 * @param id		[in] Achievement ID
 * @return 0 on success; negative POSIX error code on error.
 */
int RP_C_API AchWin32Private::notifyFunc(void *user_data, Achievements::ID id)
{
	auto *const pAchWinP = static_cast<AchWin32Private*>(user_data);
	return pAchWinP->notifyFunc(id);
}

/**
 * Notification function. (non-static)
 * @param id	[in] Achievement ID.
 * @return 0 on success; negative POSIX error code on error.
 */
int AchWin32Private::notifyFunc(Achievements::ID id)
{
	assert((int)id >= 0);
	assert(id < Achievements::ID::Max);
	if ((int)id < 0 || id >= Achievements::ID::Max) {
		// Invalid achievement ID.
		return -EINVAL;
	}

	// Get the notification window.
	HWND hNotifyWnd;
	const DWORD tid = GetCurrentThreadId();
	auto iter = map_tidToHWND.find(tid);
	if (iter != map_tidToHWND.end()) {
		// FIXME: Multiple achievements at once.
		// On Win7, this doesn't work and we end up
		// showing *no* achievements.
		//hNotifyWnd = iter->second;
		return 0;
	} else {
		// No notification window. We'll need to create it.
		std::call_once(once_flag, registerWindowClass);
		hNotifyWnd = CreateWindow(
			MAKEINTATOM(atomWindowClass),	// lpClassName
			_T("RpAchNotifyWnd"),		// lpWindowName
			0,				// dwStyle
			0, 0,				// x, y
			0, 0,				// nWidth, nHeight
			nullptr, nullptr,		// hWndParent, hMenu
			nullptr, this);			// hInstance, lpParam
		SetWindowLongPtr(hNotifyWnd, GWLP_ACHWIN32_D, reinterpret_cast<LONG_PTR>(this));
		map_tidToHWND.emplace(tid, hNotifyWnd);
		map_hWndToTID.emplace(hNotifyWnd, tid);
	}

	// FIXME: Notification window procedure.
	// TODO: Use older notify icon versions for older shell32.
	// https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/8ccef628-7620-400a-8cb5-e8761de8c5fc/shellnotifyicon-fails-error-is-errornotoken?forum=windowsuidevelopment

	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hNotifyWnd;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
	nid.uCallbackMessage = WM_ACHWIN32_NOTIFY;
	_tcscpy(nid.szTip, _T("rom-properties"));
	nid.dwState = NIS_HIDDEN;
	nid.dwStateMask = NIS_HIDDEN;
	nid.uVersion = NOTIFYICON_VERSION_4;

	// FIXME: DPI-aware scaling for the icon size.
	const PropSheetIcon *const psi = PropSheetIcon::instance();
	nid.hIcon = psi->getSmallIcon();

	// FIXME: NIF_GUID returns error 1008...
	// Win7: Use guidItem.
	// Older: Use uID.
	const DWORD nid_uID = ACHWIN32_NID_UID_HI | tid;
	nid.uID = nid_uID;
	memset(&nid.guidItem, 0, sizeof(nid.guidItem));

	BOOL bRet = Shell_NotifyIcon(NIM_ADD, &nid);
	if (!bRet) {
		// Error creating the shell icon.
		// Delete the window and forget anything happened.
		// WM_NCDESTROY will handle cleaning up the maps.
		DestroyWindow(hNotifyWnd);
		return -EIO;
	}

	// Set the notification uID property.
	SetWindowLongPtr(hNotifyWnd, GWLP_ACHWIN32_NID_UID, static_cast<LONG_PTR>(nid_uID));

	// uVersion must be set after the icon is added.
	Shell_NotifyIcon(NIM_SETVERSION, &nid);

	Achievements *const pAch = Achievements::instance();

	// Description text.
	// TODO: Formatting?
	string info = pAch->getName(id);
	info += '\n';
	info += pAch->getDescUnlocked(id);

	// Show the balloon tip.
	// TODO: Remove the icon after it disappears.
	// TODO: Program name?
	nid.uFlags = NIF_INFO;
	nid.dwInfoFlags = NIIF_USER;
	nid.uTimeout = ACHWIN32_TIMEOUT;	// NOTE: Only Win2000/XP.
	nid.hIcon = nullptr;

	// Check the OS version to determine which icon size to use.
	// TODO: DPI awareness.
	const UINT dpi = rp_GetDpiForWindow(hNotifyWnd);
	int iconSize;
	if (IsWindowsVistaOrGreater()) {
		// Windows Vista or later. Use the large icon.
		nid.dwInfoFlags |= NIIF_LARGE_ICON;
		iconSize = 32;
	} else {
		// Windows XP or earlier. Use the small icon.
		iconSize = 16;
	}

	// Load the achievements sprite sheet.
	HICON hBalloonIcon = nullptr;
	AchSpriteSheet achSpriteSheet(iconSize);

	// Get the icon.
	HBITMAP hbmIcon = achSpriteSheet.getIcon(id, false, dpi);
	assert(hbmIcon != nullptr);
	if (hbmIcon) {
		hBalloonIcon = RpImageWin32::toHICON(hbmIcon);
		DeleteBitmap(hbmIcon);
	}
	nid.hBalloonIcon = hBalloonIcon;

	const tstring ts_summary = TC_("Achievements", "Achievement Unlocked");
	_tcsncpy(nid.szInfoTitle, ts_summary.c_str(), _countof(nid.szInfoTitle));
	nid.szInfoTitle[_countof(nid.szInfoTitle)-1] = _T('\0');

	_tcsncpy(nid.szInfo, U82T_s(info), _countof(nid.szInfo));
	nid.szInfo[_countof(nid.szInfo)-1] = _T('\0');

	bRet = Shell_NotifyIcon(NIM_MODIFY, &nid);
	if (!bRet) {
		// Error modifying the shell icon.
		// Delete the shell icon and window, and forget anything happened.
		// WM_NCDESTROY will handle cleaning these up.
		DestroyWindow(hNotifyWnd);
		return -EIO;
	}

	if (hBalloonIcon) {
		DestroyIcon(hBalloonIcon);
	}

	// NOTE: Not waiting for a response.
	return 0;
}

/**
 * Register the window class.
 * NOTE: Must be called from std::call_once().
 */
void AchWin32Private::registerWindowClass(void)
{
	// TODO: Maybe atomWindowClass should be static?
	if (AchWin32Private::instance.d_ptr->atomWindowClass != 0)
		return;

	static const WNDCLASSEX wndClass = {
		sizeof(WNDCLASSEX),		// cbSize
		CS_HREDRAW | CS_VREDRAW,	// style
		RpAchNotifyWndProc,		// lpfnWndProc
		0,				// cbClsExtra
		sizeof(LONG_PTR) * 2,		// cbWndExtra [this, nid_uID]
		HINST_THISCOMPONENT,		// hInstance
		nullptr,			// hIcon
		nullptr,			// hCursor
		nullptr,			// hbrBackground
		nullptr,			// lpszMenuName
		_T("RpAchNotifyWnd"),		// lpszClassName
		nullptr				// hIconSm
	};

	// Register the window class.
	AchWin32Private::instance.d_ptr->atomWindowClass = RegisterClassEx(&wndClass);
}

/**
 * Remove a window from tracking.
 * This also removes the notification icon.
 * @param hWnd
 */
void AchWin32Private::removeWindowFromTracking(HWND hWnd)
{
	const DWORD nid_uID = static_cast<DWORD>(GetWindowLongPtr(hWnd, GWLP_ACHWIN32_NID_UID));
	if (nid_uID > 0) {
		// Notification icon was set.
		SetWindowLongPtr(hWnd, GWLP_ACHWIN32_NID_UID, 0);

		// Make sure the notification icon is destroyed.
		const DWORD tid = GetCurrentThreadId();
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = hWnd;
		nid.uFlags = 0;
		nid.uID = ACHWIN32_NID_UID_HI | tid;
		memset(&nid.guidItem, 0, sizeof(nid.guidItem));
		nid.dwState = 0;
		nid.dwStateMask = 0;
		nid.uVersion = NOTIFYICON_VERSION;
		Shell_NotifyIcon(NIM_DELETE, &nid);
	}

	// Remove the window from the maps.
	auto *const d = reinterpret_cast<AchWin32Private*>(GetWindowLongPtr(hWnd, GWLP_ACHWIN32_D));
	if (d) {
		d->map_tidToHWND.erase(GetCurrentThreadId());
		d->map_hWndToTID.erase(hWnd);
	}
}

/**
 * RpAchNotifyWnd window procedure.
 * @param hWnd
 * @param uMsg
 * @param wParam
 * @param lParam
 */
LRESULT CALLBACK AchWin32Private::RpAchNotifyWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_NCDESTROY: {
			// Window is being destroyed.
			removeWindowFromTracking(hWnd);
			break;
		}

		case WM_ACHWIN32_NOTIFY: {
			switch (LOWORD(lParam)) {
				case NIN_BALLOONTIMEOUT:
				case NIN_BALLOONUSERCLICK:
					// Achievement popup was dismissed.
					removeWindowFromTracking(hWnd);
					break;

				default:
					break;
			}
			break;
		}

		default:
			break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/** AchWin32 **/

AchWin32::AchWin32()
	: d_ptr(new AchWin32Private())
{}

AchWin32::~AchWin32()
{
	delete d_ptr;
}

/**
 * Get the AchWin32 instance.
 *
 * This automatically initializes librpbase's Achievement
 * object and reloads the achievements data if it has been
 * modified.
 *
 * @return AchWin32 instance.
 */
AchWin32 *AchWin32::instance(void)
{
	AchWin32 *const q = &AchWin32Private::instance;

	// NOTE: Cannot register in the private class constructor because
	// the Achievements instance might not be fully initialized yet.
	// Registering here instead.
	Achievements *const pAch = Achievements::instance();
	pAch->setNotifyFunction(AchWin32Private::notifyFunc, q->d_ptr);

	return q;
}

/**
 * Are any achievement popups still active?
 * This is needed in order to determine if the DLL can be unloaded.
 * @return True if any popups are still active; false if not.
 */
bool AchWin32::isAnyPopupStillActive(void) const
{
	RP_D(const AchWin32);
	return (!d->map_tidToHWND.empty());
}
