/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ContextMenu.hpp: IContextMenu implementation.                        *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// librpbase
#include "librpbase/config.librpbase.h"
#include "common.h"

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "libwin32common/ComBase.hpp"

// CLSID common macros
#include "clsid_common.hpp"

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_ContextMenu;
}

namespace LibWin32UI {
	class RegKey;
}

class RP_ContextMenu_Private;

class UUID_ATTR("{150715EA-6843-472C-9709-2CFA56690501}")
RP_ContextMenu final : public LibWin32Common::ComBase2<IShellExtInit, IContextMenu>
{
public:
	RP_ContextMenu();
protected:
	~RP_ContextMenu() final;

private:
	typedef LibWin32Common::ComBase2<IPersistFile, IContextMenu> super;
	RP_DISABLE_COPY(RP_ContextMenu)
private:
	friend class RP_ContextMenu_Private;
	RP_ContextMenu_Private *const d_ptr;

public:
	FILETYPE_HANDLER_DECL(RP_ContextMenu)

public:
	// IUnknown
	IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj) final;

	// IShellExtInit
	IFACEMETHODIMP Initialize(_In_ LPCITEMIDLIST pidlFolder, _In_ LPDATAOBJECT pDataObj, _In_ HKEY hKeyProgID) final;

	// IContextMenu
	IFACEMETHODIMP QueryContextMenu(_In_ HMENU hMenu, _In_ UINT indexMenu, _In_ UINT idCmdFirst, _In_ UINT idCmdLast, _In_ UINT uFlags) final;
	IFACEMETHODIMP InvokeCommand(_In_ CMINVOKECOMMANDINFO *pici) final;
	IFACEMETHODIMP GetCommandString(_In_ UINT_PTR idCmd, _In_ UINT uType, _Reserved_ UINT *pReserved, _Out_ CHAR *pszName, _In_  UINT cchMax) final;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ContextMenu, __MSABI_LONG(0x150715EA), 0x6843, 0x472C, 0x97,0x09, 0x2C, 0xFA, 0x56, 0x69, 0x05, 0x01)
#endif
