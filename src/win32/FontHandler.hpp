/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * FontHandler.hpp: Font handler.                                          *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "libwin32common/RpWin32_sdk.h"

class FontHandlerPrivate;
class FontHandler
{
public:
	explicit FontHandler(HWND hWnd = nullptr);
	~FontHandler();

private:
	RP_DISABLE_COPY(FontHandler)
private:
	friend class FontHandlerPrivate;
	FontHandlerPrivate *d_ptr;

public:
	/**
	 * Get the window being used for the dialog font.
	 * @return Window, or nullptr on error.
	 */
	HWND window(void) const;

	/**
	 * Set the window to use for the dialog font.
	 * This will force all managed controls to be updated.
	 * @param hWnd Window.
	 */
	void setWindow(HWND hWnd);

public:
	/**
	 * Get the bold font.
	 * @return Bold font, or nullptr on error.
	 */
	HFONT boldFont(void) const;

	/**
	 * Get the monospaced font.
	 * Needed in some cases, e.g. for ListView.
	 * @return Monospaced font, or nullptr on error.
	 */
	HFONT monospacedFont(void) const;

public:
	/**
	 * Add a control that should use the monospaced font.
	 * @param hWnd Control.
	 */
	void addMonoControl(HWND hWnd);

	/**
	 * Update fonts.
	 * This should be called in response to:
	 * - WM_NCPAINT (see below)
	 * - WM_THEMECHANGED
	 *
	 * NOTE: This *should* be called in response to WM_SETTINGCHANGE
	 * for SPI_GETFONTSMOOTHING or SPI_GETFONTSMOOTHINGTYPE, but that
	 * isn't sent when previewing ClearType changes, only when applying.
	 * WM_NCPAINT *is* called, though.
	 *
	 * @param force Force update. (Use for WM_THEMECHANGED.)
	 */
	void updateFonts(bool force = false);
};
