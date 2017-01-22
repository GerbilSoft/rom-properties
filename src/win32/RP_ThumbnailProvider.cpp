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
#include "libromdata/common.h"
#include "libromdata/RomData.hpp"
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/RpWin32.hpp"
#include "libromdata/file/RpFile.hpp"
#include "libromdata/img/rp_image.hpp"
#include "libromdata/img/RpImageLoader.hpp"
using namespace LibRomData;

// TCreateThumbnail is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/img/TCreateThumbnail.cpp"

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

/** RP_ExtractIcon_Private **/
// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ThumbnailProviderPrivate RP_ThumbnailProvider_Private

class RP_ThumbnailProvider_Private : public TCreateThumbnail<HBITMAP>
{
	public:
		RP_ThumbnailProvider_Private();
		virtual ~RP_ThumbnailProvider_Private();

	private:
		typedef TCreateThumbnail<HBITMAP> super;
		RP_ThumbnailProvider_Private(const RP_ThumbnailProvider &other);
		RP_ThumbnailProvider_Private &operator=(const RP_ThumbnailProvider &other);

	public:
		// IRpFile IInitializeWithStream::Initialize().
		IRpFile *file;

		/** TCreateThumbnail functions. **/

		/**
		 * Wrapper function to convert rp_image* to ImgClass.
		 * @param img rp_image
		 * @return ImgClass
		 */
		virtual HBITMAP rpImageToImgClass(const rp_image *img) const final;

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		virtual bool isImgClassValid(const HBITMAP &imgClass) const final;

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		virtual HBITMAP getNullImgClass(void) const final;

		/**
		 * Free an ImgClass object.
		 * This may be no-op for e.g. HBITMAP.
		 * @param imgClass ImgClass object.
		 */
		virtual void freeImgClass(const HBITMAP &imgClass) const final;

		/**
		 * Get an ImgClass's size.
		 * @param imgClass ImgClass object.
		 * @retrun Size.
		 */
		virtual ImgSize getImgSize(const HBITMAP &imgClass) const final;

		/**
		 * Rescale an ImgClass using nearest-neighbor scaling.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @return Rescaled ImgClass.
		 */
		virtual HBITMAP rescaleImgClass(const HBITMAP &imgClass, const ImgSize &sz) const final;

		/**
		 * Get the proxy for the specified URL.
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		virtual rp_string proxyForUrl(const rp_string &url) const final;
};

/** RP_ThumbnailProvider_Private **/

RP_ThumbnailProvider_Private::RP_ThumbnailProvider_Private()
	: file(nullptr)
{ }

RP_ThumbnailProvider_Private::~RP_ThumbnailProvider_Private()
{
	delete file;
}

/**
 * Wrapper function to convert rp_image* to ImgClass.
 * @param img rp_image
 * @return ImgClass.
 */
HBITMAP RP_ThumbnailProvider_Private::rpImageToImgClass(const rp_image *img) const
{
	return RpImageWin32::toHBITMAP_alpha(img);
}

/**
 * Wrapper function to check if an ImgClass is valid.
 * @param imgClass ImgClass
 * @return True if valid; false if not.
 */
bool RP_ThumbnailProvider_Private::isImgClassValid(const HBITMAP &imgClass) const
{
	return (imgClass != nullptr);
}

/**
 * Wrapper function to get a "null" ImgClass.
 * @return "Null" ImgClass.
 */
HBITMAP RP_ThumbnailProvider_Private::getNullImgClass(void) const
{
	return nullptr;
}

/**
 * Free an ImgClass object.
 * This may be no-op for e.g. HBITMAP.
 * @param imgClass ImgClass object.
 */
void RP_ThumbnailProvider_Private::freeImgClass(const HBITMAP &imgClass) const
{
	DeleteObject(imgClass);
}

/**
 * Get an ImgClass's size.
 * @param imgClass ImgClass object.
 * @retrun Size.
 */
RP_ThumbnailProvider_Private::ImgSize RP_ThumbnailProvider_Private::getImgSize(const HBITMAP &imgClass) const
{
	BITMAP bm;
	if (GetObject(imgClass, sizeof(bm), &bm) == 0) {
		// Error retrieving the bitmap information.
		static const ImgSize sz = {0, 0};
		return sz;
	}

	const ImgSize sz = {bm.bmWidth, bm.bmHeight};
	return sz;
}

/**
 * Rescale an ImgClass using nearest-neighbor scaling.
 * @param imgClass ImgClass object.
 * @param sz New size.
 * @return Rescaled ImgClass.
 */
HBITMAP RP_ThumbnailProvider_Private::rescaleImgClass(const HBITMAP &imgClass, const ImgSize &sz) const
{
	// TODO!!!
	return nullptr;
#if 0
	return imgClass.scaled(sz.width, sz.height,
		Qt::KeepAspectRatio, Qt::FastTransformation);
#endif
}

/**
 * Get the proxy for the specified URL.
 * @return Proxy, or empty string if no proxy is needed.
 */
rp_string RP_ThumbnailProvider_Private::proxyForUrl(const rp_string &url) const
{
	// libcachemgr uses urlmon on Windows, which
	// always uses the system proxy.
	return rp_string();
}

/** RP_ThumbnailProvider **/

RP_ThumbnailProvider::RP_ThumbnailProvider()
	: d_ptr(new RP_ThumbnailProvider_Private())
{ }

RP_ThumbnailProvider::~RP_ThumbnailProvider()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ThumbnailProvider::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	// Always set out parameter to NULL, validating it first.
	if (!ppvObj)
		return E_INVALIDARG;

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
		*ppvObj = nullptr;
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
LONG RP_ThumbnailProvider::RegisterCLSID(void)
{
	static const wchar_t description[] = L"ROM Properties Page - Thumbnail Provider";
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ThumbnailProvider), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Register the COM object.
	lResult = RegKey::RegisterComObject(__uuidof(RP_ThumbnailProvider), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// TODO: Set HKCR\CLSID\DisableProcessIsolation=REG_DWORD:1
	// in debug builds. Otherwise, it's not possible to debug
	// the thumbnail handler.

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_ThumbnailProvider), description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// COM object registered.
	return ERROR_SUCCESS;
}

/**
 * Register the file type handler.
 * @param hkey_Assoc File association key to register under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ThumbnailProvider::RegisterFileType(RegKey &hkey_Assoc)
{
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ThumbnailProvider), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Register as the thumbnail handler for this file association.

	// Set the "Treatment" value.
	// TODO: DWORD write function.
	lResult = hkey_Assoc.write_dword(L"Treatment", 0);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Create/open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkey_Assoc, L"ShellEx", KEY_WRITE, true);
	if (!hkcr_ShellEx.isOpen()) {
		return hkcr_ShellEx.lOpenRes();
	}
	// Create/open the IExtractImage key.
	RegKey hkcr_IThumbnailProvider(hkcr_ShellEx, L"{E357FCCD-A995-4576-B01F-234630154E96}", KEY_WRITE, true);
	if (!hkcr_IThumbnailProvider.isOpen()) {
		return hkcr_IThumbnailProvider.lOpenRes();
	}
	// Set the default value to this CLSID.
	lResult = hkcr_IThumbnailProvider.write(nullptr, clsid_str);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// File type handler registered.
	return ERROR_SUCCESS;
}

/**
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ThumbnailProvider::UnregisterCLSID(void)
{
	extern const wchar_t RP_ProgID[];

	// Unegister the COM object.
	LONG lResult = RegKey::UnregisterComObject(__uuidof(RP_ThumbnailProvider), RP_ProgID);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// TODO
	return ERROR_SUCCESS;
}

/**
 * Unregister the file type handler.
 * @param hkey_Assoc File association key to register under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ThumbnailProvider::UnregisterFileType(RegKey &hkey_Assoc)
{
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ThumbnailProvider), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Unregister as the thumbnail handler for this file association.

	// Open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkey_Assoc, L"ShellEx", KEY_READ, false);
	if (!hkcr_ShellEx.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_ShellEx.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_ShellEx.lOpenRes();
	}
	// Open the IThumbnailProvider key.
	RegKey hkcr_IThumbnailProvider(hkcr_ShellEx, L"{E357FCCD-A995-4576-B01F-234630154E96}", KEY_READ, false);
	if (!hkcr_IThumbnailProvider.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_IThumbnailProvider.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_IThumbnailProvider.lOpenRes();
	}

	// Check if the default value matches the CLSID.
	wstring wstr_IThumbnailProvider = hkcr_IThumbnailProvider.read(nullptr);
	if (wstr_IThumbnailProvider == clsid_str) {
		// Default value matches.
		// Remove the subkey.
		hkcr_IThumbnailProvider.close();
		lResult = hkcr_ShellEx.deleteSubKey(L"{E357FCCD-A995-4576-B01F-234630154E96}");
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}

		// Remove "Treatment" if it's present.
		lResult = hkey_Assoc.deleteValue(L"Treatment");
		if (lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
		}
	} else {
		// Default value doesn't match.
		// We're done here.
		return hkcr_IThumbnailProvider.lOpenRes();
	}

	// File type handler unregistered.
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

	RP_D(RP_ThumbnailProvider);
	if (d->file) {
		// Delete the old file first.
		IRpFile *old_file = d->file;
		d->file = file;
		delete old_file;
	} else {
		// No old file to delete.
		d->file = file;
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
	RP_D(RP_ThumbnailProvider);
	if (!d->file || !phbmp || !pdwAlpha) {
		return E_INVALIDARG;
	}
	*phbmp = nullptr;
	*pdwAlpha = WTSAT_ARGB;

	int ret = d->getThumbnail(d->file, cx, *phbmp);
	return (ret == 0 ? S_OK : S_FALSE);
#if 0
	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	RomData *romData = RomDataFactory::getInstance(d->file, true);
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
		img = RpImageWin32::getExternalImage(romData, RomData::IMG_EXT_MEDIA);
		imgpf = romData->imgpf(RomData::IMG_EXT_MEDIA);
		needs_delete = (img != nullptr);
	}

	if (!img) {
		// No external media scan.
		// Try an internal image.
		if (imgbf & RomData::IMGBF_INT_ICON) {
			// Internal icon.
			img = RpImageWin32::getInternalImage(romData, RomData::IMG_INT_ICON);
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

	romData->unref();
	return (*phbmp != nullptr ? S_OK : E_FAIL);
#endif
}
