/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ContextMenu.hpp: IContextMenu implementation.                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_ContextMenu.hpp"
#include "RpImageWin32.hpp"

// MSVCRT-specific [for _beginthreadex()]
#include <process.h>

// for RPCT_* constants
#include "CreateThumbnail.hpp"

// librpbase, librpfile, librptexture, libromdata
#include "librpbase/img/RpPngWriter.hpp"
#include "librptexture/FileFormatFactory.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using LibRomData::RomDataFactory;
using LibRpTexture::rp_image;
using LibRpTexture::FileFormatFactory;

// libwin32ui
using LibWin32UI::RegKey;

// C++ STL classes.
using std::string;
using std::tstring;
using std::unique_ptr;
using std::vector;

// CLSID
const CLSID CLSID_RP_ContextMenu =
	{0x150715EA, 0x6843, 0x472C, {0x97, 0x09, 0x2C, 0xFA, 0x56, 0x69, 0x05, 0x01}};

#define CTX_VERB_A "rp-convert-to-png"
#define CTX_VERB_W L"rp-convert-to-png"

#define IDM_RP_CONVERT_TO_PNG 0

/** RP_ContextMenu_Private **/
#include "RP_ContextMenu_p.hpp"

RP_ContextMenu_Private::RP_ContextMenu_Private()
	: tfilenames(nullptr)
	, hbmPng(nullptr)
{ }

RP_ContextMenu_Private::~RP_ContextMenu_Private()
{
	clear_tfilenames_vector(this->tfilenames);
	if (hbmPng) {
		DeleteBitmap(hbmPng);
	}
}

/**
 * Clear a tfilenames vector.
 * All filenames will be deleted and the vector will also be deleted.
 * @param tfilenames tilenames vector
 */
void RP_ContextMenu_Private::clear_tfilenames_vector(std::vector<LPTSTR> *tfilenames)
{
	if (unlikely(!tfilenames))
		return;

	for (LPTSTR tfilename : *tfilenames) {
		free(tfilename);
	}
	delete tfilenames;
}

/**
 * Convert a texture file to PNG format.
 * Destination filename will be generated based on the source filename.
 * @param source_file Source filename
 * @return 0 on success; non-zero on error.
 */
int RP_ContextMenu_Private::convert_to_png(LPCTSTR source_file)
{
	const size_t source_len = _tcslen(source_file);
	LPTSTR output_file = static_cast<LPTSTR>(malloc((source_len + 16) * sizeof(TCHAR)));
	_tcscpy(output_file, source_file);

	// Find the current extension and replace it.
	TCHAR *const dotpos = _tcsrchr(output_file, '.');
	if (!dotpos) {
		// No file extension. Add it.
		_tcscat(output_file, _T(".png"));
	} else {
		// If the dot is after the last slash, we already have a file extension.
		// Otherwise, we don't have one, and need to add it.
		TCHAR *const slashpos = _tcsrchr(output_file, _T('\\'));
		if (slashpos < dotpos) {
			// We already have a file extension.
			_tcscpy(dotpos, _T(".png"));
		} else {
			// No file extension.
			_tcscat(output_file, _T(".png"));
		}
	}

	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	// TODO: Use FileFormatFactory from librptexture instead?
	RomData *const romData = RomDataFactory::create(T2U8(source_file).c_str(), RomDataFactory::RDA_HAS_THUMBNAIL);
	if (!romData) {
		// ROM is not supported.
		free(output_file);
		return RPCT_ERROR_SOURCE_FILE_NOT_SUPPORTED;
	}

	// Get the internal image.
	// NOTE: The GTK and KDE implementations use CreateThumbnail.
	// NOTE 2: Image is owned by the RomData object.
	const rp_image *const img = romData->image(RomData::IMG_INT_IMAGE);
	if (!img) {
		// No image.
		romData->unref();
		free(output_file);
		return RPCT_ERROR_SOURCE_FILE_NO_IMAGE;
	}

	// Save the image using RpPngWriter.
	const int height = img->height();

	// tEXt chunks
	RpPngWriter::kv_vector kv;

	unique_ptr<RpPngWriter> pngWriter(new RpPngWriter(T2U8(output_file).c_str(),
		img->width(), height, img->format()));
	free(output_file);
	if (!pngWriter->isOpen()) {
		// Could not open the PNG writer.
		romData->unref();
		return RPCT_ERROR_OUTPUT_FILE_FAILED;
	}

	/** tEXt chunks **/

	// Software
	kv.emplace_back("Software", "ROM Properties Page shell extension (Win32)");

	// Write the tEXt chunks.
	pngWriter->write_tEXt(kv);

	/** IHDR **/

	// If sBIT wasn't found, all fields will be 0.
	// RpPngWriter will ignore sBIT in this case.
	rp_image::sBIT_t sBIT;
	if (img->get_sBIT(&sBIT) != 0) {
		memset(&sBIT, 0, sizeof(sBIT));
	}
	int pwRet = pngWriter->write_IHDR(&sBIT,
		img->palette(), img->palette_len());
	if (pwRet != 0) {
		// Error writing IHDR.
		// TODO: Unlink the PNG image.
		romData->unref();
		return RPCT_ERROR_OUTPUT_FILE_FAILED;
	}

	/** IDAT chunk. **/

	// Initialize the row pointers.
	unique_ptr<const uint8_t*[]> row_pointers(new const uint8_t*[height]);
	const uint8_t *pixels = static_cast<const uint8_t*>(img->bits());
	const int stride = img->stride();
	for (int y = 0; y < height; y++, pixels += stride) {
		row_pointers[y] = pixels;
	}

	// Write the IDAT section.
	pwRet = pngWriter->write_IDAT(row_pointers.get());
	if (pwRet != 0) {
		// Error writing IDAT.
		// TODO: Unlink the PNG image.
		romData->unref();
		return RPCT_ERROR_OUTPUT_FILE_FAILED;
	}

	// Finished writing the PNG image.
	romData->unref();
	return 0;
}

/**
 * Convert texture file(s) to PNG format.
 * This function should be created in a separate thread using _beginthreadex().
 * @param lpParameter [in] Pointer to vector of tfilenames. Will be freed by this function afterwards.
 * @return 0 on success; non-zero on error.
 */
unsigned int WINAPI RP_ContextMenu_Private::convert_to_png_ThreadProc(std::vector<LPTSTR> *tfilenames)
{
	if (unlikely(!tfilenames)) {
		// No filenames...
		return ERROR_INVALID_PARAMETER;
	}

	// Process the filenames.
	for (LPTSTR tfilename : *tfilenames) {
		convert_to_png(tfilename);
	}

	// Delete the tfilenames vector.
	clear_tfilenames_vector(tfilenames);

	// We're done here.
	return 0;
}

/**
 * Get the icon index from an icon resource specification,
 * e.g. "C:\\Windows\\Some.DLL,1" .
 * @param szIconSpec Icon resource specification
 * @return Icon index, or 0 (default) if unknown.
 */
int RP_ContextMenu_Private::getIconIndexFromSpec(LPCTSTR szIconSpec)
{
	// DefaultIcon format: "C:\\Windows\\Some.DLL,1"
	// TODO: Can the filename be quoted?
	// TODO: Better error codes?
	int nIconIndex;
	const TCHAR *const comma = _tcsrchr(szIconSpec, _T(','));
	if (!comma) {
		// No comma. Assume the default icon index.
		return 0;
	}

	// Found the comma.
	if (comma > szIconSpec && comma[1] != _T('\0')) {
		TCHAR *endptr = nullptr;
		errno = 0;
		nIconIndex = (int)_tcstol(&comma[1], &endptr, 10);
		if (errno == ERANGE || *endptr != 0) {
			// _tcstol() failed.
			// DefaultIcon is invalid, but we'll assume the index is 0.
			nIconIndex = 0;
		}
	} else {
		// Comma is the last character.
		// We'll assume the index is 0.
		nIconIndex = 0;
	}

	return nIconIndex;
}

/**
 * Get the PNG icon for the menu.
 * (Technically an HBITMAP for menus.)
 * @return PNG icon, or nullptr if unavailable.
 */
HBITMAP RP_ContextMenu_Private::getPngIcon(void)
{
	if (this->hbmPng) {
		// We already have the icon.
		return this->hbmPng;
	}

	// Get the icon used for PNG files.
	// NOTE: Assuming it's ".png" -> "pngfile". Not handling other cases.
	RegKey hkcr_png(HKEY_CLASSES_ROOT, _T(".png"), KEY_READ, false);
	if (!hkcr_png.isOpen()) {
		return nullptr;
	}

	tstring ts_class_name = hkcr_png.read(nullptr);
	if (ts_class_name.empty()) {
		return nullptr;
	}
	hkcr_png.close();

	ts_class_name += _T("\\DefaultIcon");
	RegKey hkcr_defaultIcon(HKEY_CLASSES_ROOT, ts_class_name.c_str(), KEY_READ, false);
	if (!hkcr_defaultIcon.isOpen()) {
		return nullptr;
	}

	const tstring ts_icon_name = hkcr_defaultIcon.read_expand(nullptr);
	if (ts_icon_name.empty()) {
		return nullptr;
	}
	hkcr_defaultIcon.close();

	// We have an icon and index.
	// TODO: Only load the small icon?
	const UINT nIconSize = MAKELONG(
		GetSystemMetrics(SM_CXICON),	// LOWORD: Large icon size
		GetSystemMetrics(SM_CXSMICON)	// HIWORD: Small icon size
	);
	HICON hLargeIcon = nullptr, hSmallIcon = nullptr;
	int ret = LibWin32UI::loadIconFromFilenameAndIndex(ts_icon_name.c_str(), &hLargeIcon, &hSmallIcon, nIconSize);
	if (ret != ERROR_SUCCESS) {
		// Error loading an icon.
		if (hLargeIcon) {
			DestroyIcon(hLargeIcon);
		}
		if (hSmallIcon) {
			DestroyIcon(hSmallIcon);
		}
		return nullptr;
	}

	// Prefer the small icon.
	if (!hSmallIcon) {
		hSmallIcon = hLargeIcon;
		hLargeIcon = nullptr;
	}
	if (!hSmallIcon) {
		// No icon...
		return nullptr;
	}

	// Get the icon bitmaps.
	ICONINFO iconInfo;
	BOOL bRet = GetIconInfo(hSmallIcon, &iconInfo);
	DestroyIcon(hSmallIcon);
	if (hLargeIcon) {
		DestroyIcon(hLargeIcon);
	}
	if (!bRet) {
		// Failed to retrieve the icon bitmaps.
		return nullptr;
	}

	if (iconInfo.hbmMask) {
		// TODO: Do we need to use the mask?
		// Ignore it for now.
		DeleteBitmap(iconInfo.hbmMask);
	}

	// We have the bitmap.
	this->hbmPng = iconInfo.hbmColor;
	return iconInfo.hbmColor;
}

/** RP_ContextMenu **/

RP_ContextMenu::RP_ContextMenu()
	: d_ptr(new RP_ContextMenu_Private())
{ }

RP_ContextMenu::~RP_ContextMenu()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/implementing-iunknown-in-c-plus-plus

IFACEMETHODIMP RP_ContextMenu::QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj)
{
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(RP_ContextMenu, IShellExtInit),
		QITABENT(RP_ContextMenu, IContextMenu),
		{ 0, 0 }
	};
#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::rp_QISearch(this, rgqit, riid, ppvObj);
}

/** IShellExtInit **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ishellextinit-initialize [Initialize()]

IFACEMETHODIMP RP_ContextMenu::Initialize(
	_In_ LPCITEMIDLIST pidlFolder, _In_ LPDATAOBJECT pDataObj, _In_ HKEY hKeyProgID)
{
	((void)pidlFolder);
	((void)hKeyProgID);

	// Clear the stored filenames on Initialize(),
	// even if the rest of the function fails.
	RP_D(RP_ContextMenu);
	d->clear_tfilenames_vector(d->tfilenames);
	d->tfilenames = nullptr;

	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	if (!pDataObj) {
		return E_INVALIDARG;
	}

	FORMATETC fe = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stm;

	// The pDataObj pointer contains the objects being acted upon. In this 
	// example, we get an HDROP handle for enumerating the selected files and 
	// folders.
	if (FAILED(pDataObj->GetData(&fe, &stm)))
		return E_FAIL;

	// Get an HDROP handle.
	HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
	if (!hDrop) {
		ReleaseStgMedium(&stm);
		return E_FAIL;
	}

	// Determine how many files are involved in this operation. This
	// code sample displays the custom context menu item when only
	// one file is selected.
	UINT nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
	if (nFiles == 0) {
		// No files?
		//goto cleanup;
		GlobalUnlock(stm.hGlobal);
		return E_FAIL;
	}

	// Get the vector of supported file extensions.
	const vector<const char*> &texture_exts = FileFormatFactory::supportedFileExtensions();

	// Save the tfilenames.
	d->tfilenames = new std::vector<LPTSTR>();
	d->tfilenames->reserve(nFiles);
	for (UINT i = 0; i < nFiles; i++) {
		UINT cchFilename = DragQueryFile(hDrop, i, nullptr, 0);
		if (cchFilename == 0) {
			// No filename.
			continue;
		}
		LPTSTR tfilename = static_cast<LPTSTR>(malloc((cchFilename+1) * sizeof(TCHAR)));
		cchFilename = DragQueryFile(hDrop, i, tfilename, cchFilename+1);
		if (cchFilename == 0) {
			// No filename.
			free(tfilename);
			continue;
		}

		// The list of supported file extensions is in UTF-8.
		// Convert the file extension from TCHAR to UTF-8.
		const string s_ext = T2U8(FileSystem::file_ext(tfilename));

		bool is_texture = false;
		for (const char *texture_ext : texture_exts) {
			if (!strcasecmp(texture_ext, s_ext.c_str())) {
				is_texture = true;
				break;
			}
		}

		if (is_texture) {
			// It's a supported texture. Save the filename.
			d->tfilenames->emplace_back(tfilename);
		} else {
			// Not a supported texture.
			free(tfilename);
		}
	}

	// File paths saved.
	GlobalUnlock(stm.hGlobal);
	return S_OK;
}

/** IContextMenu **/
// References:
// - https://learn.microsoft.com/en-us/windows/win32/shell/how-to-implement-the-icontextmenu-interface
// - https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-icontextmenu

IFACEMETHODIMP RP_ContextMenu::QueryContextMenu(_In_ HMENU hMenu, _In_ UINT indexMenu, _In_ UINT idCmdFirst, _In_ UINT idCmdLast, _In_ UINT uFlags)
{
	RP_UNUSED(idCmdLast);

	// TODO: Figure out when the context menu is added.
	if ((LOWORD(uFlags) & 3) != CMF_NORMAL) {
		// Not a "normal" context menu. Don't add anything.
		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));
	}

	// Get the icon used for PNG files.
	RP_D(RP_ContextMenu);
	HBITMAP hbmPng = d->getPngIcon();

	// Menu item text
	const tstring ts_text = U82T_c(C_("ServiceMenu", "Convert to PNG"));

	// Add "Convert to PNG".
	// TODO: Verify that it can be converted to PNG first.
	// FIXME: Icon transparency seems to be broken.
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_BITMAP;
	mii.wID = idCmdFirst + IDM_RP_CONVERT_TO_PNG;
	mii.fType = MFT_STRING;
	mii.dwTypeData = const_cast<LPTSTR>(ts_text.c_str());
	mii.hbmpItem = hbmPng;
	mii.fState = MFS_ENABLED;

	if (!InsertMenuItem(hMenu, indexMenu, TRUE, &mii)) {
		return HRESULT_FROM_WIN32(GetLastError());
	}
	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(IDM_RP_CONVERT_TO_PNG + 1));
}

IFACEMETHODIMP RP_ContextMenu::InvokeCommand(_In_ CMINVOKECOMMANDINFO *pici)
{
	// Check for a matching "Convert to PNG" verb.
	bool isConvertToPNG = false;
	if (pici->cbSize == sizeof(CMINVOKECOMMANDINFOEX) && (pici->fMask & CMIC_MASK_UNICODE)) {
		// Unicode version
		CMINVOKECOMMANDINFOEX *const piciex = reinterpret_cast<CMINVOKECOMMANDINFOEX*>(pici);
		if (HIWORD(piciex->lpVerbW)) {
			// Verb is specified
			isConvertToPNG = !wcscmp(piciex->lpVerbW, CTX_VERB_W);
		} else {
			// Offset is specified
			isConvertToPNG = (LOWORD(pici->lpVerb) == IDM_RP_CONVERT_TO_PNG);
		}
	} else {
		// ANSI version
		if (HIWORD(pici->lpVerb)) {
			// Verb is specified
			isConvertToPNG = !strcmp(pici->lpVerb, CTX_VERB_A);
		} else {
			// Offset is specified
			isConvertToPNG = (LOWORD(pici->lpVerb) == IDM_RP_CONVERT_TO_PNG);
		}
	}

	if (!isConvertToPNG) {
		return E_FAIL;
	}

	// Start the PNG conversion thread.
	RP_D(RP_ContextMenu);
	std::vector<LPTSTR> *param_tfilenames = nullptr;
	std::swap(param_tfilenames, d->tfilenames);
	HANDLE hThread = reinterpret_cast<HANDLE>(_beginthreadex(
		nullptr, 0, (_beginthreadex_proc_type)d->convert_to_png_ThreadProc,
		param_tfilenames, 0, nullptr));
	if (!hThread) {
		// Couldn't start the worker thread.

		// Copy the tfilenames vector pointer back so we still own it.
		d->tfilenames = param_tfilenames;

		// TODO: Better error code?
		return E_FAIL;
	}

	// We don't need to hold on to the thread handle.
	CloseHandle(hThread);
	return S_OK;
}

IFACEMETHODIMP RP_ContextMenu::GetCommandString(_In_ UINT_PTR idCmd, _In_ UINT uType, _Reserved_ UINT *pReserved, _Out_ CHAR *pszName, _In_  UINT cchMax)
{
	// NOTE: Using snprintf()/_snwprintf() because strncpy()
	// clears the buffer, which can be slow.
	RP_UNUSED(pReserved);

	RP_D(const RP_ContextMenu);
	if (idCmd == IDM_RP_CONVERT_TO_PNG) {
		switch (uType) {
			case GCS_VERBA:
				snprintf(pszName, cchMax, CTX_VERB_A);
				return S_OK;
			case GCS_VERBW:
				_snwprintf(reinterpret_cast<LPWSTR>(pszName), cchMax, CTX_VERB_W);
				return S_OK;

			case GCS_HELPTEXTA:
			case GCS_HELPTEXTW:
				// This will be handled outside of switch/case.
				// NOTE: Not used by Windows Vista or later.
				break;

			case GCS_VALIDATEA:
			case GCS_VALIDATEW:
				return S_OK;

			default:
				return E_FAIL;
		}

		// GCS_HELPTEXT(A|W)
		const int nc = d->tfilenames ? static_cast<int>(d->tfilenames->size()) : 0;
		const char *const msg = NC_("ServiceMenu",
			"Convert the selected texture file to PNG format.",
			"Convert the selected texture files to PNG format.",
			nc);

		if (likely(uType == GCS_HELPTEXTW)) {
			_snwprintf(reinterpret_cast<LPWSTR>(pszName), cchMax, L"%s", U82T_c(msg));
		} else /*if (uType == GCS_HELPTEXTA)*/ {
			_snprintf(pszName, cchMax, "%s", U82A_c(msg));
		}
		return S_OK;
	} else {
		switch (uType) {
			case GCS_VALIDATEA:
			case GCS_VALIDATEW:
				return S_FALSE;

			default:
				return E_FAIL;
		}
	}
}
