#pragma once

/**
 * Custom subclasses for full Dark Mode functionality.
 *
 * Based on Notepad++'s controls:
 * https://github.com/notepad-plus-plus/notepad-plus-plus/tree/master/PowerEditor/src/WinControls
 */

#include "libwin32common/RpWin32_sdk.h"

// Subclass ID for Notepad++'s dark mode subclasses
static const unsigned int NppDarkMode_SubclassID = 0x0115;

 /**
 * Subclass procedure for Tab controls.
 * @param hWnd
 * @param uMsg
 * @param wParam
 * @param lParam
 * @param uIdSubclass
 * @param dwRefData
 */
LRESULT WINAPI NppDarkMode_TabControlSubclassProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
