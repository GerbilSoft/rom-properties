/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * AutoGetDC.hpp: GetDC() RAII wrapper class.                              *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_AUTOGETDC_HPP__
#define __ROMPROPERTIES_LIBWIN32COMMON_AUTOGETDC_HPP__

// C includes. (C++ namespace)
#include <cassert>

// Windows SDK.
#include "RpWin32_sdk.h"
#include <windowsx.h>

namespace LibWin32Common {

/**
 * GetDC() RAII wrapper.
 */
class AutoGetDC
{
	public:
		inline AutoGetDC(HWND hWnd)
			: hWnd(hWnd)
			, bAdjFont(false)
		{
			assert(hWnd != nullptr);
			hDC = GetDC(hWnd);
		}

		inline AutoGetDC(HWND hWnd, HFONT hFont)
			: hWnd(hWnd)
			, bAdjFont(true)
		{
			assert(hWnd != nullptr);
			assert(hFont != nullptr);
			hDC = GetDC(hWnd);
			hFontOrig = (hDC ? SelectFont(hDC, hFont) : nullptr);
		}

		inline ~AutoGetDC() {
			if (!hDC) {
				return;
			}

			if (bAdjFont) {
				SelectFont(hDC, hFontOrig);
			}
			ReleaseDC(hWnd, hDC);
		}

		inline operator HDC() {
			return hDC;
		}

	private:
		// Disable copying.
		AutoGetDC(const AutoGetDC &);
		AutoGetDC &operator=(const AutoGetDC &);

	private:
		HWND hWnd;
		HDC hDC;
		HFONT hFontOrig;
		bool bAdjFont;
};

}

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_AUTOGETDC_HPP__ */
