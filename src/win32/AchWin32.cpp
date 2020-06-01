/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * AchWin32.hpp: Win32 notifications for achievements.                     *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchWin32.hpp"

// librpbase
#include "librpbase/TextFuncs_wchar.hpp"
using LibRpBase::Achievements;

// ROM icon.
#include "config/PropSheetIcon.hpp"

// C++ STL classes.
using std::string;

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

	public:
		/**
		 * Notification function. (static)
		 * @param user_data User data. (this)
		 * @param name Achievement name.
		 * @param desc Achievement description.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int notifyFunc(intptr_t user_data, const char *name, const char *desc);

		/**
		 * Notification function. (non-static)
		 * @param name Achievement name.
		 * @param desc Achievement description.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int notifyFunc(const char *name, const char *desc);

	public:
		ATOM atomWindowClass;
		HWND hNotifyWnd;
};

/** AchWin32Private **/

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
AchWin32 AchWin32Private::instance;

AchWin32Private::AchWin32Private()
	: hasRegistered(false)
	, hNotifyWnd(nullptr)
{
	// NOTE: Cannot register here because the static Achievements
	// instance might not be fully initialized yet.

	WNDCLASSEX wndClass;
	wndClass.cbSize = sizeof(wndClass);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = DefWindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = HINST_THISCOMPONENT;
	wndClass.hIcon = nullptr;
	wndClass.hCursor = nullptr;
	wndClass.hbrBackground = nullptr;
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = _T("RpAchNotifyWnd");
	wndClass.hIconSm = nullptr;

	// Register the window class.
	atomWindowClass = RegisterClassEx(&wndClass);
	DWORD dwErr = GetLastError();
}

AchWin32Private::~AchWin32Private()
{
	if (hasRegistered) {
		Achievements *const pAch = Achievements::instance();
		pAch->clearNotifyFunction(notifyFunc, reinterpret_cast<intptr_t>(this));
	}

	if (hNotifyWnd) {
		DestroyWindow(hNotifyWnd);
	}

	if (atomWindowClass > 0) {
		UnregisterClass(_T("RpAchNotifyWnd"), HINST_THISCOMPONENT);
	}
}

/**
 * Notification function. (static)
 * @param user_data User data. (this)
 * @param name Achievement name.
 * @param desc Achievement description.
 * @return 0 on success; negative POSIX error code on error.
 */
int AchWin32Private::notifyFunc(intptr_t user_data, const char *name, const char *desc)
{
	AchWin32Private *const pAchWinP = reinterpret_cast<AchWin32Private*>(user_data);
	return pAchWinP->notifyFunc(name, desc);
}

/**
 * Notification function. (non-static)
 * @param name Achievement name.
 * @param desc Achievement description.
 * @return 0 on success; negative POSIX error code on error.
 */
int AchWin32Private::notifyFunc(const char *name, const char *desc)
{
	// Create the notification window if it hasn't already been created.
	if (!hNotifyWnd) {
		hNotifyWnd = CreateWindow(
			_T("RpAchNotifyWnd"),	// lpClassName
			_T("RpAchNotifyWnd"),	// lpWindowName
			0,			// dwStyle
			0, 0,			// x, y
			0, 0,			// nWidth, nHeight
			nullptr, nullptr,	// hWndParent, hMenu
			nullptr, this);		// hInstance, lpParam
		DWORD dwErr = GetLastError();
	}

	// FIXME: Notification window procedure.
	// TODO: Use older notify icon versions for older shell32.
	// https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/8ccef628-7620-400a-8cb5-e8761de8c5fc/shellnotifyicon-fails-error-is-errornotoken?forum=windowsuidevelopment

	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hNotifyWnd;
	nid.uFlags = NIF_ICON | NIF_TIP /*| NIF_MESSAGE*/ | NIF_SHOWTIP;
	nid.uCallbackMessage = 0;
	_tcscpy(nid.szTip, _T("rom-properties"));
	nid.dwState = 0;
	nid.dwStateMask = 0;
	nid.uVersion = NOTIFYICON_VERSION_4;

	// FIXME: DPI-aware scaling for the icon size.
	const PropSheetIcon *const psi = PropSheetIcon::instance();
	nid.hIcon = psi->getSmallIcon();

	// FIXME: NIF_GUID returns error 1008...
	// Win7: Use guidItem.
	// Older: Use uID.
	nid.uID = 1234;
	memset(&nid.guidItem, 0, sizeof(nid.guidItem));

	BOOL bRet = Shell_NotifyIcon(NIM_ADD, &nid);
	DWORD dwErr = GetLastError();

	// uVersion must be set after the icon is added.
	Shell_NotifyIcon(NIM_SETVERSION, &nid);

	// Description text.
	// TODO: Formatting?
	string info = name;
	info += '\n';
	info += desc;

	// Show the balloon tip.
	// TODO: Remove the icon after it disappears.
	// TODO: Program name?
	nid.uFlags = NIF_INFO;
	nid.dwInfoFlags = NIIF_USER;
	nid.uTimeout = 5000;	// NOTE: Only Win2000/XP.

	// Check the OS version to determine which icon to use.
	// TODO: DPI awareness.
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	if (GetVersionEx(&osvi) && osvi.dwMajorVersion >= 6) {
		// Windows Vista or later. Use the large icon.
		nid.dwInfoFlags |= NIIF_LARGE_ICON;
		nid.hIcon = nullptr;
		nid.hBalloonIcon = psi->getLargeIcon();
	} else {
		// Windows XP or earlier. Use the small icon.
		nid.hIcon = psi->getSmallIcon();
		nid.hBalloonIcon = nullptr;
	}

	_tcsncpy(nid.szInfoTitle, _T("Achievement Unlocked"), _countof(nid.szInfoTitle));
	nid.szInfoTitle[_countof(nid.szInfoTitle)-1] = _T('\0');
	_tcsncpy(nid.szInfo, U82W_s(info), _countof(nid.szInfo));
	nid.szInfo[_countof(nid.szInfo)] = _T('\0');

	Shell_NotifyIcon(NIM_MODIFY, &nid);

	// NOTE: Not waiting for a response.
	return 0;
}

/** AchWin32 **/

AchWin32::AchWin32()
	: d_ptr(new AchWin32Private())
{ }

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
	pAch->setNotifyFunction(AchWin32Private::notifyFunc, reinterpret_cast<intptr_t>(q->d_ptr));

	return q;
}
