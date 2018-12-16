/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellIconOverlayIdentifier.cpp: IShellIconOverlayIdentifier          *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_ShellIconOverlayIdentifier.hpp"

// librpbase
#include "librpbase/RomData.hpp"
#include "librpbase/file/RpFile.hpp"
using namespace LibRpBase;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// C includes. (C++ namespace)
#include <cassert>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <memory>
using std::unique_ptr;

// CLSID
const CLSID CLSID_RP_ShellIconOverlayIdentifier =
	{0x02c6Af01, 0x3c99, 0x497d, {0xb3, 0xfc, 0xe3, 0x8c, 0xe5, 0x26, 0x78, 0x6b}};

/** RP_ShellIconOverlayIdentifier_Private **/
#include "RP_ShellIconOverlayIdentifier_p.hpp"

RP_ShellIconOverlayIdentifier_Private::RP_ShellIconOverlayIdentifier_Private()
	: file(nullptr)
	, romData(nullptr)
{ }

RP_ShellIconOverlayIdentifier_Private::~RP_ShellIconOverlayIdentifier_Private()
{
	if (romData) {
		romData->unref();
	}

	delete file;
}

/** RP_PropertyStore **/

RP_ShellIconOverlayIdentifier::RP_ShellIconOverlayIdentifier()
	: d_ptr(new RP_ShellIconOverlayIdentifier_Private())
{ }

RP_ShellIconOverlayIdentifier::~RP_ShellIconOverlayIdentifier()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ShellIconOverlayIdentifier::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	static const QITAB rgqit[] = {
		QITABENT(RP_ShellIconOverlayIdentifier, IShellIconOverlayIdentifier),
		{ 0, 0 }
	};
	return LibWin32Common::pQISearch(this, rgqit, riid, ppvObj);
}

/** IShellIconOverlayIdentifier **/
// Reference: https://docs.microsoft.com/en-us/windows/desktop/shell/how-to-implement-icon-overlay-handlers

IFACEMETHODIMP RP_ShellIconOverlayIdentifier::IsMemberOf(_In_ PCWSTR pwszPath, DWORD dwAttrib)
{
	if (!pwszPath) {
		return E_POINTER;
	}

	// Allow all files that aren't "slow" or unavailable.
	// TODO: Other attributes?
	//return (dwAttrib & (SFGAO_ISSLOW | SFGAO_GHOSTED))
		//? S_FALSE	/* File is too slow. Don't check it. */
		//: S_OK;		/* Check the file. */
	return S_OK;
}

IFACEMETHODIMP RP_ShellIconOverlayIdentifier::GetOverlayInfo(_Out_writes_(cchMax) PWSTR pwszIconFile, int cchMax, _Out_ int *pIndex, _Out_ DWORD *pdwFlags)
{
	if (!pwszIconFile || !pIndex || !pdwFlags) {
		return E_POINTER;
	}

	// TODO: Actually check the file.
	// For now, assuming it needs the UAC shield.

	// NOTE: SHGetStockIconInfo() is Vista+ only.
	// TODO: Dynamically load the function pointer.
	SHSTOCKICONINFO sii;
	sii.cbSize = sizeof(sii);
	HRESULT hr = SHGetStockIconInfo(SIID_SHIELD, SHGSI_ICONLOCATION, &sii);
	if (SUCCEEDED(hr)) {
		// Copy the returned filename and index.
		wcscpy_s(pwszIconFile, cchMax, sii.szPath);
		*pIndex = sii.iIcon;
		*pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;
	} else {
		// Unable to get the filename.
		*pIndex = 0;
		*pdwFlags = 0;
	}

	return hr;
}

IFACEMETHODIMP RP_ShellIconOverlayIdentifier::GetPriority(_Out_ int *pPriority)
{
	if (!pPriority) {
		return E_POINTER;
	}

	// Use a higher priority for the UAC icon.
	*pPriority = 90;
	return S_OK;
}

