/***************************************************************************
 * ROM Properties Page shell extension. (libwin32ui)                       *
 * AutoGetDC.hpp: GetDC() RAII wrapper class.                              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cassert>

// Windows SDK
#include "RpWin32_sdk.h"
#include <windowsx.h>

namespace LibWin32UI {

// TODO: There should be a way to remove m_hFontOrig without making two separate classes.
// ("compressed" pair or something)

/**
 * GetDC() RAII wrapper (no font)
 */
class AutoGetDC
{
public:
	explicit inline AutoGetDC(HWND hWnd)
		: m_hWnd(hWnd)
	{
		assert(hWnd != nullptr);
		m_hDC = GetDC(hWnd);
	}

	inline ~AutoGetDC() {
		if (!m_hDC) {
			return;
		}

		ReleaseDC(m_hWnd, m_hDC);
	}

	// Disable copy/assignment constructors.
	AutoGetDC(const AutoGetDC &) = delete;
	AutoGetDC &operator=(const AutoGetDC &) = delete;

	inline operator HDC() {
		return m_hDC;
	}

private:
	HWND m_hWnd;
	HDC m_hDC;
};

/**
 * GetDC() RAII wrapper (with font)
 */
class AutoGetDC_font
{
public:
	explicit inline AutoGetDC_font(HWND hWnd, HFONT hFont)
		: m_hWnd(hWnd)
	{
		assert(hWnd != nullptr);
		assert(hFont != nullptr);
		m_hDC = GetDC(m_hWnd);
		m_hFontOrig = (m_hDC ? SelectFont(m_hDC, hFont) : nullptr);
	}

	inline ~AutoGetDC_font() {
		if (!m_hDC) {
			return;
		}

		SelectFont(m_hDC, m_hFontOrig);
		ReleaseDC(m_hWnd, m_hDC);
	}

	// Disable copy/assignment constructors.
	AutoGetDC_font(const AutoGetDC_font &) = delete;
	AutoGetDC_font &operator=(const AutoGetDC_font &) = delete;

	inline operator HDC() {
		return m_hDC;
	}

private:
	HWND m_hWnd;
	HDC m_hDC;
	HFONT m_hFontOrig;
};

} //namespace LibWin32UI
