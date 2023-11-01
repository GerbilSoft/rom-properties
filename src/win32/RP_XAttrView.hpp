/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_XAttrView.hpp: Extended attribute viewer property page.              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// librpbase
#include "librpbase/config.librpbase.h"
#include "common.h"

// References:
// - http://www.codeproject.com/Articles/338268/COM-in-C
// - https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
// - https://docs.microsoft.com/en-us/windows/win32/ad/implementing-the-property-page-com-object
#include "libwin32common/ComBase.hpp"

// CLSID common macros
#include "clsid_common.hpp"

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_XAttrView;
}

namespace LibWin32UI {
	class RegKey;
}

// C++ includes.
#include <string>

class RP_XAttrView_Private;

class UUID_ATTR("{B0503F2E-C4AE-48DF-A880-E2B122B58571}")
RP_XAttrView final : public LibWin32Common::ComBase2<IShellExtInit, IShellPropSheetExt>
{
public:
	RP_XAttrView();
protected:
	~RP_XAttrView() final;

private:
	typedef LibWin32Common::ComBase2<IShellExtInit, IShellPropSheetExt> super;
	RP_DISABLE_COPY(RP_XAttrView)
private:
	friend class RP_XAttrView_Private;
	RP_XAttrView_Private *d_ptr;

public:
	CLSID_DECL(RP_XAttrView)
	FILETYPE_HANDLER_DECL(RP_XAttrView)

private:
	/**
	 * Register the file type handler.
	 *
	 * Internal version; this only registers for a single Classes key.
	 * Called by the public version multiple times if a ProgID is registered.
	 *
	 * @param hkey_Assoc File association key to register under.
	 * @return ERROR_SUCCESS on success; Win32 error code on error.
	 */
	static LONG RegisterFileType_int(LibWin32UI::RegKey &hkey_Assoc);

	/**
	 * Unregister the file type handler.
	 *
	 * Internal version; this only unregisters for a single Classes key.
	 * Called by the public version multiple times if a ProgID is registered.
	 *
	 * @param hkey_Assoc File association key to unregister under.
	 * @return ERROR_SUCCESS on success; Win32 error code on error.
	 */
	static LONG UnregisterFileType_int(LibWin32UI::RegKey &hkey_Assoc);

public:
	// IUnknown
	IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj) final;

	// IShellExtInit
	IFACEMETHODIMP Initialize(_In_ LPCITEMIDLIST pidlFolder, _In_ LPDATAOBJECT pDataObj, _In_ HKEY hKeyProgID) final;

	// IShellPropSheetExt
	IFACEMETHODIMP AddPages(_In_ LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam) final;
	IFACEMETHODIMP ReplacePage(UINT uPageID, _In_ LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam) final;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_XAttrView, __MSABI_LONG(0xB0503F2E), 0xC4AE, 0x48DF, 0xA8,0x80, 0xE2, 0xB1, 0x22, 0xB5, 0x85, 0x71)
#endif
