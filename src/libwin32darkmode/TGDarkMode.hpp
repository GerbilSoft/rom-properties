#pragma once

/**
 * Custom subclasses for full Dark Mode functionality.
 *
 * Based on TortoiseGit's dark mode theme:
 * https://gitlab.com/tortoisegit/tortoisegit/-/blob/HEAD/src/Utils/Theme.cpp
 */

#include "libwin32common/RpWin32_sdk.h"

// Subclass ID for TortoiseGit's dark mode subclasses
static const unsigned int TGDarkMode_SubclassID = 0xD8CF;

 /**
 * Subclass procedure for Button controls.
 * @param hWnd
 * @param uMsg
 * @param wParam
 * @param lParam
 * @param uIdSubclass
 * @param dwRefData
 */
LRESULT WINAPI TGDarkMode_ButtonSubclassProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
