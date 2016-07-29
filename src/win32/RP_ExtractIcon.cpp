/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon.cpp: IExtractIcon implementation.                        *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_ExtractIcon.hpp"

// libromdata
#include "libromdata/RomData.hpp"
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/rp_image.hpp"
using namespace LibRomData;

// C includes. (C++ namespace)
#include <cassert>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <string>
using std::wstring;

// CLSID
const GUID CLSID_RP_ExtractIcon =
	{0xe51bc107, 0xe491, 0x4b29, {0xa6, 0xa3, 0x2a, 0x43, 0x09, 0x25, 0x98, 0x02}};
const wchar_t CLSID_RP_ExtractIcon_Str[] =
	L"{E51BC107-E491-4B29-A6A3-2A4309259802}";

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

STDMETHODIMP RP_ExtractIcon::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	// Always set out parameter to NULL, validating it first.
	if (!ppvObj)
		return E_INVALIDARG;
	*ppvObj = NULL;

	// Check if this interface is supported.
	// NOTE: static_cast<> is required due to vtable shenanigans.
	// Also, IID_IUnknown must always return the same pointer.
	// References:
	// - http://stackoverflow.com/questions/1742848/why-exactly-do-i-need-an-explicit-upcast-when-implementing-queryinterface-in-a
	// - http://stackoverflow.com/a/2812938
	if (riid == IID_IUnknown || riid == IID_IExtractIcon) {
		*ppvObj = static_cast<IExtractIcon*>(this);
	} else if (riid == IID_IPersistFile) {
		*ppvObj = static_cast<IPersistFile*>(this);
	} else {
		// Interface is not supported.
		return E_NOINTERFACE;
	}

	// Make sure we count this reference.
	AddRef();
	return NOERROR;
}

/**
 * Convert an rp_image to HBITMAP.
 * @return image rp_image.
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP RP_ExtractIcon::rpToHBITMAP(const rp_image *image)
{
	if (!image || !image->isValid())
		return nullptr;

	// FIXME: Only 256-color images are supported right now.
	// FIXME: Alpha-transparency doesn't seem to work in 256-color icons on Windows XP.
	if (image->format() != rp_image::FORMAT_CI8)
		return nullptr;

	// References:
	// - http://stackoverflow.com/questions/2886831/win32-c-c-load-image-from-memory-buffer
	// - http://stackoverflow.com/a/2901465

	// BITMAPINFO with 256-color palette.
	uint8_t bmi[sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD)*256)];
	BITMAPINFOHEADER *bmiHeader = reinterpret_cast<BITMAPINFOHEADER*>(&bmi[0]);
	RGBQUAD *palette = reinterpret_cast<RGBQUAD*>(&bmi[sizeof(BITMAPINFOHEADER)]);

	// Initialize the BITMAPINFOHEADER.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/dd183376%28v=vs.85%29.aspx
	bmiHeader->biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader->biWidth = image->width();
	bmiHeader->biHeight = -image->height();	// negative for top-down
	bmiHeader->biPlanes = 1;
	bmiHeader->biBitCount = 8;
	bmiHeader->biCompression = BI_RGB;
	bmiHeader->biSizeImage = 0;	// TODO?
	bmiHeader->biXPelsPerMeter = 0;	// TODO
	bmiHeader->biYPelsPerMeter = 0;	// TODO
	bmiHeader->biClrUsed = image->palette_len();
	bmiHeader->biClrImportant = bmiHeader->biClrUsed;	// TODO?

	// Copy the palette from the image.
	memcpy(palette, image->palette(), bmiHeader->biClrUsed * sizeof(uint32_t));

	// Create the bitmap.
	uint8_t *pvBits;
	HBITMAP hBitmap = CreateDIBSection(nullptr, reinterpret_cast<BITMAPINFO*>(&bmi),
		DIB_RGB_COLORS, reinterpret_cast<void**>(&pvBits), nullptr, 0);
	if (!hBitmap)
		return nullptr;

	// Copy the data from the rp_image into the bitmap.
	memcpy(pvBits, image->bits(), image->data_len());

	// Return the bitmap.
	return hBitmap;
}

/**
 * Convert an rp_image to an HBITMAP.
 * @return image rp_image.
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP RP_ExtractIcon::rpToHBITMAP_mask(const LibRomData::rp_image *image)
{
	if (!image || !image->isValid())
		return nullptr;

	// FIXME: Only 256-color images are supported right now.
	if (image->format() != rp_image::FORMAT_CI8)
		return nullptr;

	// References:
	// - http://stackoverflow.com/questions/2886831/win32-c-c-load-image-from-memory-buffer
	// - http://stackoverflow.com/a/2901465

	/**
	 * Create a monochrome bitmap twice as tall as the image.
	 * Top half is the AND mask; bottom half is the XOR mask.
	 *
	 * Icon truth table:
	 * AND=0, XOR=0: Black
	 * AND=0, XOR=1: White
	 * AND=1, XOR=0: Screen (transparent)
	 * AND=1, XOR=1: Reverse screen (inverted)
	 *
	 * References:
	 * - https://msdn.microsoft.com/en-us/library/windows/desktop/ms648059%28v=vs.85%29.aspx
	 * - https://msdn.microsoft.com/en-us/library/windows/desktop/ms648052%28v=vs.85%29.aspx
	 */
	const int width8 = (image->width() + 8) & ~8;	// Round up to 8px width.
	const int icon_sz = width8 * image->height() / 8;
	uint8_t *data = (uint8_t*)malloc(icon_sz * 2);

	BITMAPINFO bmi;
	BITMAPINFOHEADER *bmiHeader = &bmi.bmiHeader;

	// Initialize the BITMAPINFOHEADER.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/dd183376%28v=vs.85%29.aspx
	bmiHeader->biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader->biWidth = width8;
	bmiHeader->biHeight = image->height();	// FIXME: Top-down isn't working for monochrome...
	bmiHeader->biPlanes = 1;
	bmiHeader->biBitCount = 1;
	bmiHeader->biCompression = BI_RGB;
	bmiHeader->biSizeImage = 0;	// TODO?
	bmiHeader->biXPelsPerMeter = 0;	// TODO
	bmiHeader->biYPelsPerMeter = 0;	// TODO
	bmiHeader->biClrUsed = 0;
	bmiHeader->biClrImportant = 0;

	// Create the bitmap.
	uint8_t *pvBits;
	HBITMAP hBitmap = CreateDIBSection(nullptr, reinterpret_cast<BITMAPINFO*>(&bmi),
		DIB_RGB_COLORS, reinterpret_cast<void**>(&pvBits), nullptr, 0);
	if (!hBitmap)
		return nullptr;

	// TODO: Convert the image to a mask.
	// For now, assume the entire image is opaque.
	// NOTE: Windows doesn't support top-down for monochrome icons,
	// so this is vertically flipped.

	// AND mask: all 0 to disable inversion.
	memset(&pvBits[icon_sz], 0, icon_sz);

	// XOR mask: Parse the original image.

	// Find the first fully-transparent color.
	const uint32_t *palette = image->palette();
	int palette_len = image->palette_len();
	int tr_idx = -1;
	for (int i = 0; i < palette_len; i++, palette++) {
		if ((*palette & 0xFF000000) == 0) {
			// Found a transparent color.
			tr_idx = i;
			break;
		}
	}

	if (tr_idx >= 0) {
		// Find all pixels matching tr_idx.
		uint8_t *dest = pvBits;
		for (int y = image->height()-1; y >= 0; y--) {
			const uint8_t *src = reinterpret_cast<const uint8_t*>(image->scanLine(y));
			// FIXME: Handle images that aren't a multiple of 8px wide.
			assert(image->width() % 8 == 0);
			for (int x = image->width(); x > 0; x -= 8) {
				uint8_t pxMono = 0;
				for (int px = 8; px > 0; px--, src++) {
					// MSB == left-most pixel.
					pxMono <<= 1;
					pxMono |= (*src != tr_idx);
				}
				*dest++ = pxMono;
			}
		}
	} else {
		// No transparent color.
		memset(pvBits, 0xFF, icon_sz);
	}

	// Return the bitmap.
	return hBitmap;
}

/**
 * Convert an rp_image to HICON.
 * @param image rp_image.
 * @return HICON, or nullptr on error.
 */
HICON RP_ExtractIcon::rpToHICON(const rp_image *image)
{
	if (!image || !image->isValid())
		return nullptr;

	// Convert to HBITMAP first.
	HBITMAP hBitmap = rpToHBITMAP(image);
	if (!hBitmap)
		return nullptr;

	// Convert the image to an icon mask.
	HBITMAP hbmMask = rpToHBITMAP_mask(image);
	if (!hbmMask) {
		DeleteObject(hBitmap);
		return nullptr;
	}

	// Convert to an icon.
	// Reference: http://forums.codeguru.com/showthread.php?441251-CBitmap-to-HICON-or-HICON-from-HBITMAP&p=1661856#post1661856
	ICONINFO ii = {0};
	ii.fIcon = TRUE;
	ii.hbmColor = hBitmap;
	ii.hbmMask = hbmMask;

	// Create the icon.
	HICON hIcon = CreateIconIndirect(&ii);

	// Delete the original bitmaps and we're done.
	DeleteObject(hBitmap);
	DeleteObject(hbmMask);
	return hIcon;
}

/** IExtractIcon **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb761854(v=vs.85).aspx

STDMETHODIMP RP_ExtractIcon::GetIconLocation(UINT uFlags,
	__out_ecount(cchMax) LPTSTR pszIconFile, UINT cchMax,
	__out int *piIndex, __out UINT *pwFlags)
{
	// TODO: If the icon is cached on disk, return a filename.
	*pwFlags = GIL_NOTFILENAME | GIL_DONTCACHE;
	return S_OK;
}

STDMETHODIMP RP_ExtractIcon::Extract(LPCTSTR pszFile, UINT nIconIndex,
	__out_opt HICON *phiconLarge, __out_opt HICON *phiconSmall,
	UINT nIconSize)
{
	// NOTE: pszFile should be nullptr here.
	// TODO: Fail if it's not nullptr?

	// Make sure a filename was set by calling IPersistFile::Load().
	if (m_filename.empty())
		return E_INVALIDARG;

	// Attempt to open the ROM file.
	// TODO: rp_QFile() wrapper?
	// For now, using stdio.
	FILE *file = _wfopen(m_filename.c_str(), L"rb");
	if (!file)
		return E_FAIL;

	// Get the appropriate RomData class for this ROM.
	RomData *romData = RomDataFactory::getInstance(file);
	fclose(file);	// file is dup()'d by RomData.

	if (!romData) {
		// ROM is not supported.
		return S_FALSE;
	}

	// ROM is supported. Get the internal icon.
	// TODO: Customize for internal icon, disc/cart scan, etc.?
	HRESULT ret = S_FALSE;
	const rp_image *icon = romData->image(RomData::IMG_INT_ICON);
	if (icon) {
		// Convert the icon to HICON.
		HICON hIcon = rpToHICON(icon);
		if (hIcon != nullptr) {
			// Icon converted.
			if (phiconLarge) {
				*phiconLarge = hIcon;
			} else {
				DeleteObject(hIcon);
			}

			if (phiconSmall) {
				// FIXME: is this valid?
				*phiconSmall = nullptr;
			}

			ret = S_OK;
		}
	}
	delete romData;

	return ret;
}

/** IPersistFile **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/cc144067(v=vs.85).aspx#unknown_28177

STDMETHODIMP RP_ExtractIcon::GetClassID(__RPC__out CLSID *pClassID)
{
	// 3DS extension returns E_NOTIMPL here.
	// TODO: Set a CLSID for RP_ExtractIcon?
	// TODO: Use separate CLSIDs for the different RP_ classes,
	// instead of one CLSID for everything?
	return E_NOTIMPL;
}

STDMETHODIMP RP_ExtractIcon::IsDirty(void)
{
	return E_NOTIMPL;
}

STDMETHODIMP RP_ExtractIcon::Load(__RPC__in LPCOLESTR pszFileName, DWORD dwMode)
{
	// pszFileName is the file being worked on.
	m_filename = pszFileName;
	return S_OK;
}

STDMETHODIMP RP_ExtractIcon::Save(__RPC__in_opt LPCOLESTR pszFileName, BOOL fRemember)
{
	return E_NOTIMPL;
}

STDMETHODIMP RP_ExtractIcon::SaveCompleted(__RPC__in_opt LPCOLESTR pszFileName)
{
	return E_NOTIMPL;
}

STDMETHODIMP RP_ExtractIcon::GetCurFile(__RPC__deref_out_opt LPOLESTR *ppszFileName)
{
	return E_NOTIMPL;
}
