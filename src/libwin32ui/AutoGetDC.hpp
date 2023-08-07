/***************************************************************************
 * ROM Properties Page shell extension. (libwin32ui)                       *
 * AutoGetDC.hpp: GetDC() RAII wrapper class.                              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cassert>

// Windows SDK
#include "RpWin32_sdk.h"
#include <windowsx.h>

namespace LibWin32UI {

/**
 * GetDC() RAII wrapper.
 */
class AutoGetDC
{
public:
	explicit inline AutoGetDC(HWND hWnd)
		: m_hWnd(hWnd)
		, m_hFontOrig(nullptr)
		, m_bAdjFont(false)
	{
		assert(hWnd != nullptr);
		m_hDC = GetDC(hWnd);
	}

	explicit inline AutoGetDC(HWND hWnd, HFONT hFont)
		: m_hWnd(hWnd)
		, m_bAdjFont(true)
	{
		assert(hWnd != nullptr);
		assert(hFont != nullptr);
		m_hDC = GetDC(m_hWnd);
		m_hFontOrig = (m_hDC ? SelectFont(m_hDC, hFont) : nullptr);
	}

	inline ~AutoGetDC() {
		if (!m_hDC) {
			return;
		}

		if (m_bAdjFont) {
			SelectFont(m_hDC, m_hFontOrig);
		}
		ReleaseDC(m_hWnd, m_hDC);
	}

	inline operator HDC() {
		return m_hDC;
	}

	// Disable copy/assignment constructors.
#if __cplusplus >= 201103L
public:
	AutoGetDC(const AutoGetDC &) = delete;
	AutoGetDC &operator=(const AutoGetDC &) = delete;
#else /* __cplusplus < 201103L */
private:
	AutoGetDC(const AutoGetDC &);
	AutoGetDC &operator=(const AutoGetDC &);
#endif /* __cplusplus */

private:
	HWND m_hWnd;
	HDC m_hDC;
	HFONT m_hFontOrig;
	bool m_bAdjFont;
};

} //namespace LibWin32UI
