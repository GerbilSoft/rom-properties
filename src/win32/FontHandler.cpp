/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * FontHandler.cpp: Font handler.                                          *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"
#include "FontHandler.hpp"

// libwin32common
#include "libwin32common/w32err.hpp"

// C++ STL classes
using std::array;
using std::tstring;
using std::unordered_set;
using std::vector;

class FontHandlerPrivate
{
public:
	explicit FontHandlerPrivate(HWND hWnd);
	~FontHandlerPrivate();

private:
	RP_DISABLE_COPY(FontHandlerPrivate)

public:
	// Window used for the dialog font.
	HWND hWnd;

	// Fonts
	HFONT hFontBold;
	HFONT hFontMono;

	// Controls
	vector<HWND> vecMonoControls;	// Controls using the monospaced font.

	// Previous ClearType setting
	bool bPrevIsClearType;

private:
	/**
	 * Monospaced font enumeration procedure.
	 * @param lpelfe Enumerated font information.
	 * @param lpntme Font metrics.
	 * @param FontType Font type.
	 * @param lParam Pointer to RP_ShellPropSheetExt_Private.
	 */
	static int CALLBACK MonospacedFontEnumProc(
		_In_ const LOGFONT *lpelfe,
		_In_ const TEXTMETRIC *lpntme,
		_In_ DWORD FontType,
		_In_ LPARAM lParam);

public:
	/**
	 * Determine the monospaced font to use.
	 * @param plfFontMono Pointer to LOGFONT to store the font name in.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	static int findMonospacedFont(LOGFONT *plfFontMono);

	/**
	 * Get the current ClearType setting.
	 * @return Current ClearType setting.
	 */
	static bool isClearTypeEnabled(void);

	/**
	 * (Re)Initialize the bold font.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int initBoldFont(void);

	/**
	 * (Re)Initialize the monospaced font.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int initMonospacedFont(void);

public:
	/**
	 * Update fonts.
	 * @param force Force update. (Use for WM_THEMECHANGED.)
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int updateFonts(bool force = false);
};

/** FontHandlerPrivate **/

FontHandlerPrivate::FontHandlerPrivate(HWND hWnd)
	: hWnd(hWnd)
	, hFontBold(nullptr)
	, hFontMono(nullptr)
	, bPrevIsClearType(false)
{}

FontHandlerPrivate::~FontHandlerPrivate()
{
	if (hFontBold) {
		DeleteFont(hFontBold);
	}
	if (hFontMono) {
		DeleteFont(hFontMono);
	}
}

/**
 * Monospaced font enumeration procedure.
 * @param lpelfe Enumerated font information.
 * @param lpntme Font metrics.
 * @param FontType Font type.
 * @param lParam Pointer to RP_ShellPropSheetExt_Private.
 */
int CALLBACK FontHandlerPrivate::MonospacedFontEnumProc(
	_In_ const LOGFONT *lpelfe,
	_In_ const TEXTMETRIC *lpntme,
	_In_ DWORD FontType,
	_In_ LPARAM lParam)
{
	((void)lpntme);
	((void)FontType);
	unordered_set<tstring> *const pFonts = reinterpret_cast<unordered_set<tstring>*>(lParam);

	// Check the font attributes:
	// - Must be monospaced.
	// - Must be horizontally-oriented.
	if ((lpelfe->lfPitchAndFamily & FIXED_PITCH) &&
	     lpelfe->lfFaceName[0] != _T('@'))
	{
		pFonts->insert(lpelfe->lfFaceName);
	}

	// Continue enumeration.
	return 1;
}

/**
 * Determine the monospaced font to use.
 * @param plfFontMono Pointer to LOGFONT to store the font name in.
 * @return 0 on success; negative POSIX error code on error.
 */
int FontHandlerPrivate::findMonospacedFont(LOGFONT *plfFontMono)
{
	// Enumerate all monospaced fonts.
	// Reference: http://www.catch22.net/tuts/fixed-width-font-enumeration
	unordered_set<tstring> enum_fonts;
#ifdef HAVE_UNORDERED_SET_RESERVE
	enum_fonts.reserve(64);
#endif /* HAVE_UNORDERED_SET_RESERVE */

	LOGFONT lfEnumFonts = {
		0,	// lfHeight
		0,	// lfWidth
		0,	// lfEscapement
		0,	// lfOrientation
		0,	// lfWeight
		0,	// lfItalic
		0,	// lfUnderline
		0,	// lfStrikeOut
		DEFAULT_CHARSET, // lfCharSet
		0,	// lfOutPrecision
		0,	// lfClipPrecision
		0,	// lfQuality
		FIXED_PITCH | FF_DONTCARE, // lfPitchAndFamily
		_T(""),	// lfFaceName
	};

	HDC hDC = GetDC(nullptr);
	EnumFontFamiliesEx(hDC, &lfEnumFonts, MonospacedFontEnumProc,
		reinterpret_cast<LPARAM>(&enum_fonts), 0);
	ReleaseDC(nullptr, hDC);

	if (enum_fonts.empty()) {
		// No fonts...
		return -ENOENT;
	}

	// Fonts to try.
	static const array<LPCTSTR, 12> mono_font_names = {{
		_T("DejaVu Sans Mono"),
		_T("Consolas"),
		_T("Lucida Console"),
		_T("Fixedsys Excelsior 3.01"),
		_T("Fixedsys Excelsior 3.00"),
		_T("Fixedsys Excelsior 3.0"),
		_T("Fixedsys Excelsior 2.00"),
		_T("Fixedsys Excelsior 2.0"),
		_T("Fixedsys Excelsior 1.00"),
		_T("Fixedsys Excelsior 1.0"),
		_T("Fixedsys"),
		_T("Courier New"),
	}};

	const TCHAR *mono_font = nullptr;
	for (const TCHAR *pFontTest : mono_font_names) {
		if (enum_fonts.find(pFontTest) != enum_fonts.end()) {
			// Found a font.
			mono_font = pFontTest;
			break;
		}
	}

	if (!mono_font) {
		// No monospaced font found.
		return -ENOENT;
	}

	// Found the monospaced font.
	_tcscpy_s(plfFontMono->lfFaceName, _countof(plfFontMono->lfFaceName), mono_font);
	return 0;
}

/**
 * Get the current ClearType setting.
 * @return Current ClearType setting.
 */
bool FontHandlerPrivate::isClearTypeEnabled(void)
{
	// Get the current ClearType setting.
	bool bIsClearType = false;
	BOOL bFontSmoothing;
	BOOL bRet = SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &bFontSmoothing, 0);
	if (bRet) {
		UINT uiFontSmoothingType;
		bRet = SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &uiFontSmoothingType, 0);
		if (bRet) {
			bIsClearType = (bFontSmoothing && (uiFontSmoothingType == FE_FONTSMOOTHINGCLEARTYPE));
		}
	}
	return bIsClearType;
}

/**
 * (Re)Initialize the bold font.
 * @return 0 on success; negative POSIX error code on error.
 */
int FontHandlerPrivate::initBoldFont(void)
{
	if (!hWnd) {
		// No window. Delete the font.
		if (hFontBold) {
			DeleteFont(hFontBold);
			hFontBold = nullptr;
		}
		return -EBADF;
	}

	// TODO: Keep track of controls that use this font?

	// Get the current window font.
	HFONT hFont = GetWindowFont(hWnd);
	assert(hFont != nullptr);
	if (!hFont) {
		// Unable to get the window font.
		return -ENOENT;
	}

	LOGFONT lfFontBold;
	if (GetObject(hFont, sizeof(lfFontBold), &lfFontBold) == 0) {
		// Unable to obtain the LOGFONT.
		return -EIO;
	}

	// Adjust the font weight and create the new font.
	lfFontBold.lfWeight = FW_BOLD;
	HFONT hFontBoldNew = CreateFontIndirect(&lfFontBold);
	if (!hFontBoldNew) {
		// Unable to create the new font.
		return -w32err_to_posix(GetLastError());
	}

	// Delete the old font and save the new one.
	HFONT hFontBoldOld = hFontBold;
	hFontBold = hFontBoldNew;
	if (hFontBoldOld) {
		DeleteFont(hFontBoldOld);
	}

	// Font created.
	return 0;
}

/**
 * (Re)Initialize the monospaced font.
 * @return 0 on success; negative POSIX error code on error.
 */
int FontHandlerPrivate::initMonospacedFont(void)
{
	if (!hWnd) {
		// No window. Delete the font.
		if (hFontMono) {
			DeleteFont(hFontMono);
			hFontMono = nullptr;
		}
		return -EBADF;
	}

	// Get the current window font.
	HFONT hFont = GetWindowFont(hWnd);
	assert(hFont != nullptr);
	if (!hFont) {
		// Unable to get the window font.
		return -ENOENT;
	}

	LOGFONT lfFontMono;
	if (GetObject(hFont, sizeof(lfFontMono), &lfFontMono) == 0) {
		// Unable to obtain the LOGFONT.
		return -EIO;
	}

	// Find a monospaced font.
	int ret = findMonospacedFont(&lfFontMono);
	if (ret != 0) {
		// Monospaced font not found.
		return -ENOENT;
	}

	// Create the monospaced font.
	// If ClearType is enabled, use DEFAULT_QUALITY;
	// otherwise, use NONANTIALIASED_QUALITY.
	const bool bIsClearType = isClearTypeEnabled();
	lfFontMono.lfQuality = (bIsClearType ? DEFAULT_QUALITY : NONANTIALIASED_QUALITY);
	HFONT hFontMonoNew = CreateFontIndirect(&lfFontMono);
	if (!hFontMonoNew) {
		// Unable to create the new font.
		return -w32err_to_posix(GetLastError());
	}

	// Update all monospaced controls to use the new font.
	for (HWND hWnd : vecMonoControls) {
		SetWindowFont(hWnd, hFontMonoNew, false);
	}

	// Delete the old font and save the new one.
	HFONT hFontMonoOld = hFontMono;
	hFontMono = hFontMonoNew;
	if (hFontMonoOld) {
		DeleteFont(hFontMonoOld);
	}

	// Font created.
	return 0;
}

/**
 * Update fonts.
 * @param force Force update. (Use for WM_THEMECHANGED.)
 * @return 0 on success; negative POSIX error code on error.
 */
int FontHandlerPrivate::updateFonts(bool force)
{
	if (!hWnd) {
		// No window. Delete the fonts.
		if (hFontBold) {
			DeleteFont(hFontBold);
			hFontBold = nullptr;
		}
		if (hFontMono) {
			DeleteFont(hFontMono);
			hFontMono = nullptr;
		}
		return -EBADF;
	}

	// We need to update the fonts if the ClearType state was changed.
	const bool bIsClearType = isClearTypeEnabled();
	const bool bCreateFonts = force || (bIsClearType != bPrevIsClearType);
	if (!bCreateFonts) {
		// Nothing to do here.
		return 0;
	}

	// Update fonts if they were previously created.
	if (hFontBold) {
		initBoldFont();
	}
	if (hFontMono || !vecMonoControls.empty()) {
		initMonospacedFont();
	}

	// Update the ClearType state.
	bPrevIsClearType = bIsClearType;
	return 0;
}

/** FontHandler **/

FontHandler::FontHandler(HWND hWnd)
	: d_ptr(new FontHandlerPrivate(hWnd))
{}

FontHandler::~FontHandler()
{
	delete d_ptr;
}

/**
 * Get the window being used for the dialog font.
 * @return Window, or nullptr on error.
 */
HWND FontHandler::window(void) const
{
	RP_D(const FontHandler);
	return d->hWnd;
}

/**
 * Set the window to use for the dialog font.
 * This will force all managed controls to be updated.
 * @param hWnd Window.
 */
void FontHandler::setWindow(HWND hWnd)
{
	RP_D(FontHandler);
	d->hWnd = hWnd;
	d->updateFonts();
}

/**
 * Get the bold font.
 * @return Bold font, or nullptr on error.
 */
HFONT FontHandler::boldFont(void) const
{
	RP_D(FontHandler);
	if (!d->hFontBold) {
		d->initBoldFont();
	}
	assert(d->hFontBold != nullptr);
	return d->hFontBold;
}

/**
 * Get the monospaced font.
 * Needed in some cases, e.g. for ListView.
 * @return Monospaced font, or nullptr on error.
 */
HFONT FontHandler::monospacedFont(void) const
{
	RP_D(FontHandler);
	if (!d->hFontMono) {
		d->initMonospacedFont();
	}
	assert(d->hFontMono != nullptr);
	return d->hFontMono;
}

/**
 * Add a control that should use the monospaced font.
 * @param hWnd Control.
 */
void FontHandler::addMonoControl(HWND hWnd)
{
	RP_D(FontHandler);
	assert(d->hWnd != nullptr);
	if (!d->hFontMono) {
		d->initMonospacedFont();
	}
	d->vecMonoControls.emplace_back(hWnd);
	SetWindowFont(hWnd, d->hFontMono, false);
}

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
void FontHandler::updateFonts(bool force)
{
	RP_D(FontHandler);
	assert(d->hWnd != nullptr);
	if (d->hWnd) {
		d->updateFonts(force);
	}
}
