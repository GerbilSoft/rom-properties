/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * AutoGetDC.hpp: GetDC() RAII wrapper class.                              *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
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
		inline AutoGetDC(HWND hWnd, HFONT hFont)
			: hWnd(hWnd)
		{
			assert(hWnd != nullptr);
			assert(hFont != nullptr);
			if (hWnd) {
				hDC = GetDC(hWnd);
				hFontOrig = (hDC ? SelectFont(hDC, hFont) : nullptr);
			} else {
				hDC = nullptr;
				hFontOrig = nullptr;
			}
		}

		inline ~AutoGetDC() {
			if (hDC) {
				SelectFont(hDC, hFontOrig);
				ReleaseDC(hWnd, hDC);
			}
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
};

}

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_AUTOGETDC_HPP__ */
