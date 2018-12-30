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

#ifndef __ROMPROPERTIES_WIN32_RP_SHELLICONOVERLAYIDENTIFIER_HPP__
#define __ROMPROPERTIES_WIN32_RP_SHELLICONOVERLAYIDENTIFIER_HPP__

// librpbase
#include "librpbase/config.librpbase.h"
#include "librpbase/common.h"

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "libwin32common/ComBase.hpp"

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_ShellIconOverlayIdentifier;
}

namespace LibWin32Common {
	class RegKey;
}

class RP_ShellIconOverlayIdentifier_Private;

class UUID_ATTR("{02C6AF01-3C99-497D-B3FC-E38CE526786B}")
RP_ShellIconOverlayIdentifier : public LibWin32Common::ComBase<IShellIconOverlayIdentifier>
{
	public:
		RP_ShellIconOverlayIdentifier();
		virtual ~RP_ShellIconOverlayIdentifier();

	private:
		typedef LibWin32Common::ComBase<IShellIconOverlayIdentifier> super;
		RP_DISABLE_COPY(RP_ShellIconOverlayIdentifier)
	private:
		friend class RP_ShellIconOverlayIdentifier_Private;
		RP_ShellIconOverlayIdentifier_Private *const d_ptr;

	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) final;

	public:
		/**
		 * Register the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG RegisterCLSID(void);

		/**
		 * Unregister the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterCLSID(void);

	public:
		// IShellIconOverlayIdentifier
		IFACEMETHODIMP IsMemberOf(_In_ PCWSTR pwszPath, DWORD dwAttrib) final;
		IFACEMETHODIMP GetOverlayInfo(_Out_writes_(cchMax) PWSTR pwszIconFile, int cchMax, _Out_ int *pIndex, _Out_ DWORD *pdwFlags);
		IFACEMETHODIMP GetPriority(_Out_ int *pPriority);
};

#ifdef __CRT_UUID_DECL
// Required for MinGw-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ShellIconOverlayIdentifier, 0x02c6Af01, 0x3c99, 0x497d, 0xb3, 0xfc, 0xe3, 0x8c, 0xe5, 0x26, 0x78, 0x6b);
#endif

#endif /* __ROMPROPERTIES_WIN32_RP_SHELLICONOVERLAYIDENTIFIER_HPP__ */
