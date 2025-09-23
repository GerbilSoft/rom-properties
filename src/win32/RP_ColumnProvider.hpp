/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ColumnProvider.hpp: IColumnProvider implementation.                  *
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
	extern const CLSID CLSID_RP_ColumnProvider;
}

namespace LibWin32UI {
	class RegKey;
}

class RP_ColumnProvider_Private;

class UUID_ATTR("{126621F9-01E7-45DA-BC4F-CBDFAB9C0E0A}")
RP_ColumnProvider final : public LibWin32Common::ComBase<IColumnProvider>
{
public:
	RP_ColumnProvider();
protected:
	~RP_ColumnProvider() final;

private:
	typedef LibWin32Common::ComBase<IColumnProvider> super;
	RP_DISABLE_COPY(RP_ColumnProvider)
private:
	//friend class RP_ColumnProvider_Private;
	//RP_ColumnProvider_Private *const d_ptr;

public:
	FILETYPE_HANDLER_DECL(RP_ColumnProvider)

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

	// IColumnProvider
	IFACEMETHODIMP Initialize(LPCSHCOLUMNINIT psci) final;
	IFACEMETHODIMP GetColumnInfo(_In_ DWORD dwIndex, _Out_ SHCOLUMNINFO *psci) final;
	//STDMETHOD(GetItemData)(THIS_ LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, __out VARIANT *pvarData) PURE;
	IFACEMETHODIMP GetItemData(_In_ LPCSHCOLUMNID pscid, _In_ LPCSHCOLUMNDATA pscd, _Out_ VARIANT *pvarData) final;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ColumnProvider, __MSABI_LONG(0x126621f9), 0x01e7, 0x45da, 0xbc,0x4f, 0xcb, 0xdf, 0xab, 0x9c, 0x0e, 0x0a)
#endif
