/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellIconOverlayIdentifier_Register.cpp: IShellIconOverlayIdentifier *
 * COM registration functions.                                             *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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

#include "stdafx.h"
#include "RP_ShellIconOverlayIdentifier.hpp"
#include "RP_ShellIconOverlayIdentifier_p.hpp"

#include "libwin32common/RegKey.hpp"
using LibWin32Common::RegKey;

#define CLSID_RP_ShellIconOverlayIdentifier_String	TEXT("{02C6AF01-3C99-497D-B3FC-E38CE526786B}")
extern const TCHAR RP_ProgID[];

// Overlay handler name.
static const TCHAR RP_OverlayHandler[] = _T("RpDangerousPermissionsOverlay");

/**
 * Register the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ShellIconOverlayIdentifier::RegisterCLSID(void)
{
	static const TCHAR description[] = _T("ROM Properties Page - Shell Icon Overlay Identifier");

	// Register the COM object.
	LONG lResult = RegKey::RegisterComObject(__uuidof(RP_ShellIconOverlayIdentifier), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_ShellIconOverlayIdentifier), description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Register as a shell icon overlay handler.
	RegKey hklm_SIOI(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers"), KEY_READ, false);
	if (!hklm_SIOI.isOpen())
		return hklm_SIOI.lOpenRes();

	// Delete the old handler in case it's present.
	hklm_SIOI.deleteSubKey(RP_ProgID);

	// Create the handler for rom-properties.
	RegKey hklm_rpi(hklm_SIOI, RP_OverlayHandler, KEY_READ|KEY_WRITE, true);
	if (!hklm_rpi.isOpen())
		return hklm_rpi.lOpenRes();

	lResult = hklm_rpi.write(nullptr, CLSID_RP_ShellIconOverlayIdentifier_String);
	return lResult;
}

/**
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ShellIconOverlayIdentifier::UnregisterCLSID(void)
{
	// Unegister the COM object.
	LONG lResult = RegKey::UnregisterComObject(__uuidof(RP_ShellIconOverlayIdentifier), RP_ProgID);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Remove the shell icon overlay handler.
	RegKey hklm_SIOI(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers"), KEY_READ, false);
	if (hklm_SIOI.isOpen()) {
		hklm_SIOI.deleteSubKey(RP_OverlayHandler);

		// Delete the old one in case it's present.
		hklm_SIOI.deleteSubKey(RP_ProgID);
	}

	return ERROR_SUCCESS;
}
