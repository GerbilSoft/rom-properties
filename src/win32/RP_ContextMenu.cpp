/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ContextMenu.hpp: IContextMenu implementation.                        *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_ContextMenu.hpp"
#include "RpImageWin32.hpp"

// librpbase, librpfile, librptexture, libromdata
using namespace LibRpBase;
using namespace LibRpFile;
using LibRpTexture::rp_image;
using LibRomData::RomDataFactory;

// C++ STL classes.
using std::string;
using std::tstring;

// CLSID
const CLSID CLSID_RP_ContextMenu =
	{0x150715EA, 0x6843, 0x472C, {0x97, 0x09, 0x2C, 0xFA, 0x56, 0x69, 0x05, 0x01}};

#define CTX_VERB_A "rp-convert-to-png"
#define CTX_VERB_W L"rp-convert-to-png"

/** RP_ContextMenu_Private **/
#include "RP_ContextMenu_p.hpp"

RP_ContextMenu_Private::RP_ContextMenu_Private()
	: idConvertToPng(0)
{ }

RP_ContextMenu_Private::~RP_ContextMenu_Private()
{
	clear_filenames();
}

/**
 * Clear the filenames vector.
 */
void RP_ContextMenu_Private::clear_filenames(void)
{
	for (LPTSTR filename : filenames) {
		free(filename);
	}
	filenames.clear();
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
# pragma warning(push)
# pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(RP_ContextMenu, IShellExtInit),
		QITABENT(RP_ContextMenu, IContextMenu),
		{ 0, 0 }
	};
#ifdef _MSC_VER
# pragma warning(pop)
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
	d->clear_filenames();

	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	if (!pDataObj) {
		return E_INVALIDARG;
	}

	FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
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

	HRESULT hr = E_FAIL;

	// Determine how many files are involved in this operation. This
	// code sample displays the custom context menu item when only
	// one file is selected.
	UINT nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
	if (nFiles == 0) {
		// No files?
		//goto cleanup;
		GlobalUnlock(stm.hGlobal);
		return E_FAIL;
	}

	// Save the file paths.
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

		// Check if the extension is supported.
		// TODO: Combine with DllRegisterServer and/or FileFormatFactory.
		const TCHAR *ext = FileSystem::file_ext(tfilename);
		static const TCHAR texture_exts[][8] = {
			_T(".astc"), _T(".dds"), _T(".gvr"), _T(".ktx"),
			_T(".ktx2"), _T(".pvr"), _T(".stex"), _T(".svr"),
			_T(".tex"), _T(".texs"), _T(".tga"), _T(".vtf"),
			_T(".xpr"), _T(".xbx"),
		};
		// TODO: Rework into a binary search.
		bool is_texture = false;
		for (auto texture_ext : texture_exts) {
			if (!_tcscmp(texture_ext, ext)) {
				is_texture = true;
				break;
			}
		}

		if (is_texture) {
			// It's a supported texture. Save the filename.
			d->filenames.emplace_back(tfilename);
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
	// TODO: Figure out when the context menu is added.
	if ((LOWORD(uFlags) & 3) != CMF_NORMAL) {
		// Not a "normal" context menu. Don't add anything.
		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));
	}

	// Add "Convert to PNG".
	// TODO: Verify that it can be converted to PNG first.
	RP_D(RP_ContextMenu);
	d->idConvertToPng = idCmdFirst;
	InsertMenu(hMenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmdFirst,
		U82T_c(C_("ServiceMenu", "Convert to PNG")));
	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(1));
}

IFACEMETHODIMP RP_ContextMenu::InvokeCommand(_In_ CMINVOKECOMMANDINFO *pici)
{
	// TODO: Implement this.
	MessageBoxA(NULL, "invoke RP Convert to PNG!", "invoke RP Convert to PNG!", 0);
	return E_FAIL;
}

IFACEMETHODIMP RP_ContextMenu::GetCommandString(_In_ UINT_PTR idCmd, _In_ UINT uType, _Reserved_ UINT *pReserved, _Out_ CHAR *pszName, _In_  UINT cchMax)
{
	// NOTE: Using snprintf()/_snwprintf() because strncpy()
	// clears the buffer, which can be slow.

	RP_D(const RP_ContextMenu);
	if (idCmd == d->idConvertToPng) {
		switch (uType) {
			case GCS_VERBA:
				snprintf(pszName, cchMax, "rp-convert-to-png");
				return S_OK;
			case GCS_VERBW:
				_snwprintf(reinterpret_cast<LPWSTR>(pszName), cchMax, L"rp-convert-to-png");
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
		const char *const msg = NC_("ServiceMenu",
			"Convert the selected texture file to PNG format.",
			"Convert the selected texture files to PNG format.",
			static_cast<int>(d->filenames.size()));

		if (likely(uType == GCS_VERBW)) {
			_snwprintf(reinterpret_cast<LPWSTR>(pszName), cchMax, L"%s", U82T_c(msg));
		} else /*if (uType == GCS_VERBA)*/ {
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
