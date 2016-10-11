/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ThumbnailProvider.hpp: IThumbnailProvider implementation.            *
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
#include "RP_ThumbnailProvider.hpp"
#include "RegKey.hpp"
#include "RpImageWin32.hpp"

// libromdata
#include "libromdata/RomData.hpp"
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/RpWin32.hpp"
#include "libromdata/file/RpFile.hpp"
#include "libromdata/img/rp_image.hpp"
#include "libromdata/img/RpImageLoader.hpp"
using namespace LibRomData;

// RpFile_IStream
#include "RpFile_IStream.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
using std::unique_ptr;
using std::wstring;

// CLSID
const CLSID CLSID_RP_ThumbnailProvider =
	{0x4723df58, 0x463e, 0x4590, {0x8f, 0x4a, 0x8d, 0x9d, 0xd4, 0xf4, 0x35, 0x5a}};

RP_ThumbnailProvider::RP_ThumbnailProvider()
	: m_file(nullptr)
{ }

RP_ThumbnailProvider::~RP_ThumbnailProvider()
{
	delete m_file;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ThumbnailProvider::QueryInterface(REFIID riid, LPVOID *ppvObj)
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
	if (riid == IID_IUnknown || riid == IID_IInitializeWithStream) {
		*ppvObj = static_cast<IInitializeWithStream*>(this);
	} else if (riid == IID_IThumbnailProvider) {
		*ppvObj = static_cast<IThumbnailProvider*>(this);
	} else {
		// Interface is not supported.
		return E_NOINTERFACE;
	}

	// Make sure we count this reference.
	AddRef();
	return NOERROR;
}

/**
 * Register the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ThumbnailProvider::Register(void)
{
	static const wchar_t description[] = L"ROM Properties Page - Thumbnail Provider";
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ThumbnailProvider), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0)
		return ERROR_INVALID_PARAMETER;

	// Register the COM object.
	lResult = RegKey::RegisterComObject(__uuidof(RP_ThumbnailProvider), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS)
		return lResult;
	// TODO: Set HKCR\CLSID\DisableProcessIsolation=REG_DWORD:1
	// in debug builds. Otherwise, it's not possible to debug
	// the thumbnail handler.

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_ThumbnailProvider), description);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// Create/open the ProgID key.
	RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, RP_ProgID, KEY_WRITE, true);
	if (!hkcr_ProgID.isOpen())
		return hkcr_ProgID.lOpenRes();

	// Set the "Treatment" value.
	// TODO: DWORD write function.
	lResult = hkcr_ProgID.write_dword(L"Treatment", 0);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// Create/open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkcr_ProgID, L"ShellEx", KEY_WRITE, true);
	if (!hkcr_ShellEx.isOpen())
		return hkcr_ShellEx.lOpenRes();
	// Create/open the IExtractImage key.
	RegKey hkcr_IThumbnailProvider(hkcr_ShellEx, L"{E357FCCD-A995-4576-B01F-234630154E96}", KEY_WRITE, true);
	if (!hkcr_IThumbnailProvider.isOpen())
		return hkcr_IThumbnailProvider.lOpenRes();
	// Set the default value to this CLSID.
	lResult = hkcr_IThumbnailProvider.write(nullptr, clsid_str);
	if (lResult != ERROR_SUCCESS)
		return lResult;
	hkcr_IThumbnailProvider.close();
	hkcr_ShellEx.close();

	// COM object registered.
	return ERROR_SUCCESS;
}

/**
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ThumbnailProvider::Unregister(void)
{
	extern const wchar_t RP_ProgID[];

	// Unegister the COM object.
	LONG lResult = RegKey::UnregisterComObject(__uuidof(RP_ThumbnailProvider), RP_ProgID);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// TODO
	return ERROR_SUCCESS;
}

/** IInitializeWithStream **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb761812(v=vs.85).aspx [Initialize()]

IFACEMETHODIMP RP_ThumbnailProvider::Initialize(IStream *pstream, DWORD grfMode)
{
	// Ignoring grfMode for now. (always read-only)
	((void)grfMode);

	// Create an IRpFile wrapper for the IStream.
	IRpFile *file = new RpFile_IStream(pstream);
	if (file->lastError() != 0) {
		// Error initializing the IRpFile.
		delete file;
		return E_FAIL;
	}

	if (m_file) {
		// Delete the old file first.
		IRpFile *old_file = m_file;
		m_file = file;
		delete old_file;
	} else {
		// No old file to delete.
		m_file = file;
	}

	return S_OK;
}

/** IThumbnailProvider **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb774612(v=vs.85).aspx [GetThumbnail()]

IFACEMETHODIMP RP_ThumbnailProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha)
{
	// Verify parameters:
	// - A stream must have been set by calling IInitializeWithStream::Initialize().
	// - phbmp and pdwAlpha must not be nullptr.
	if (!m_file || !phbmp || !pdwAlpha) {
		return E_INVALIDARG;
	}
	*phbmp = nullptr;
	*pdwAlpha = WTSAT_ARGB;

	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	unique_ptr<RomData> romData(RomDataFactory::getInstance(m_file, true));
	if (!romData) {
		// ROM is not supported.
		return S_FALSE;
	}

	// TODO: Customize which ones are used per-system.
	// For now, check EXT MEDIA, then INT ICON.

	bool needs_delete = false;	// External images need manual deletion.
	const rp_image *img = nullptr;
	uint32_t imgpf = 0;

	// ROM is supported. Get the image.
	uint32_t imgbf = romData->supportedImageTypes();
	if (imgbf & RomData::IMGBF_EXT_MEDIA) {
		// External media scan.
		img = RpImageWin32::getExternalImage(romData.get(), RomData::IMG_EXT_MEDIA);
		imgpf = romData->imgpf(RomData::IMG_EXT_MEDIA);
		needs_delete = (img != nullptr);
	}

	if (!img) {
		// No external media scan.
		// Try an internal image.
		if (imgbf & RomData::IMGBF_INT_ICON) {
			// Internal icon.
			img = RpImageWin32::getInternalImage(romData.get(), RomData::IMG_INT_ICON);
			imgpf = romData->imgpf(RomData::IMG_INT_ICON);
		}
	}

	if (img) {
		// Image loaded. Convert it to HBITMAP.

		// TODO: User configuration.
		ResizePolicy resize = RESIZE_HALF;
		bool needs_resize = false;

		switch (resize) {
			case RESIZE_NONE:
				// No resize.
				break;

			case RESIZE_HALF:
			default:
				// Only resize images that are less than or equal to
				// half requested thumbnail size.
				needs_resize = (img->width() <= (int)(cx/2) || img->height() <= (int)(cx/2));
				break;

			case RESIZE_ALL:
				// Resize all images that are smaller than the
				// requested thumbnail size.
				needs_resize = (img->width() < (int)cx || img->height() < (int)cx);
				break;
		}

		if (!needs_resize) {
			// No resize is necessary.
			*phbmp = RpImageWin32::toHBITMAP_alpha(img);
		} else {
			// Windows will *not* enlarge the thumbnail.
			// We'll need to do that ourselves.
			// NOTE: This results in much larger thumbnail files.

			// NOTE 2: GameTDB uses 160px images for disc scans.
			// Windows 7 only seems to request 256px thumbnails,
			// so this will result in 160px being upscaled and
			// then downscaled again.

			SIZE size;
			const int w = img->width();
			const int h = img->height();
			if (w == h) {
				// Aspect ratio matches.
				size.cx = (LONG)cx;
				size.cy = (LONG)cx;
			} else if (w > h) {
				// Image is wider.
				size.cx = (LONG)cx;
				size.cy = (LONG)((float)h / (float)w * (float)cx);
			} else /*if (w < h)*/ {
				// Image is taller.
				size.cx = (LONG)((float)w / (float)h * (float)cx);
				size.cy = (LONG)cx;
			}

			bool nearest = false;
			if (imgpf & RomData::IMGPF_RESCALE_NEAREST) {
				// If the requested thumbnail size is an integer multiple
				// of the image size, use nearest-neighbor scaling.
				if ((size.cx % img->width() == 0) && (size.cy % img->height() == 0)) {
					// Integer multiple.
					nearest = true;
				}
			}
			*phbmp = RpImageWin32::toHBITMAP_alpha(img, size, nearest);
		}

		if (needs_delete) {
			// Delete the image.
			delete const_cast<rp_image*>(img);
		}
	}

	return (*phbmp != nullptr ? S_OK : E_FAIL);
}
