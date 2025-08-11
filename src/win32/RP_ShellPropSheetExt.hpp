/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt.hpp: IShellPropSheetExt implementation.            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
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
	extern const CLSID CLSID_RP_ShellPropSheetExt;
}

namespace LibWin32UI {
	class RegKey;
}

class RP_ShellPropSheetExt_Private;

class UUID_ATTR("{2443C158-DF7C-4352-B435-BC9F885FFD52}")
RP_ShellPropSheetExt final : public LibWin32Common::ComBase2<IShellExtInit, IShellPropSheetExt>
{
public:
	RP_ShellPropSheetExt();
protected:
	~RP_ShellPropSheetExt() final;

private:
	typedef LibWin32Common::ComBase2<IShellExtInit, IShellPropSheetExt> super;
	RP_DISABLE_COPY(RP_ShellPropSheetExt)
private:
	friend class RP_ShellPropSheetExt_Private;
	RP_ShellPropSheetExt_Private *d_ptr;

public:
	FILETYPE_HANDLER_DECL(RP_ShellPropSheetExt)

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
__CRT_UUID_DECL(RP_ShellPropSheetExt, __MSABI_LONG(0x2443c158), 0xdf7c, 0x4352, 0xb4,0x35, 0xbc, 0x9f, 0x88, 0x5f, 0xfd, 0x52)
#endif
