/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * PropSheetIcon.cpp: Property sheet icon.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include "stdafx.h"
#include "PropSheetIcon.hpp"

#include "libromdata/threads/InitOnceExecuteOnceXP.h"

class PropSheetIconPrivate
{
	private:
		PropSheetIconPrivate();
		~PropSheetIconPrivate();
		RP_DISABLE_COPY(PropSheetIconPrivate)

	public:
		// InitOnceExecuteOnce() control variable.
		static INIT_ONCE initOnce;

		// Property sheet icons.
		static HICON hIcon;
		static HICON hIconSmall;

		/**
		 * Get the property sheet icons.
		 * NOTE: This function should be called with InitOnceExecuteOnce().
		 * @param InitOnce INIT_ONCE control variable.
		 * @param Parameter Parameter.
		 * @param Context Context.
		 */
		static BOOL CALLBACK getPropSheetIcons(
			_Inout_ PINIT_ONCE once,
			_Inout_opt_ PVOID param,
			_Out_opt_ PVOID *context);
};

/** PropSheetIconPrivate **/

// InitOnceExecuteOnce() control variable.
INIT_ONCE PropSheetIconPrivate::initOnce = INIT_ONCE_STATIC_INIT;

// Property sheet icons.
HICON PropSheetIconPrivate::hIcon = nullptr;
HICON PropSheetIconPrivate::hIconSmall = nullptr;

/**
 * Get the property sheet icons.
 * NOTE: This function should be called with InitOnceExecuteOnce().
 * @param InitOnce INIT_ONCE control variable.
 * @param Parameter Parameter.
 * @param Context Context.
 */
BOOL CALLBACK PropSheetIconPrivate::getPropSheetIcons(
	_Inout_ PINIT_ONCE once,
	_Inout_opt_ PVOID param,
	_Out_opt_ PVOID *context)
{
	// We aren't using any of the InitOnceExecuteOnce() parameters.
	RP_UNUSED(once);
	RP_UNUSED(param);
	RP_UNUSED(context);

	// Check for a DLL containing a usable ROM chip icon.
	struct IconDllData_t {
		const wchar_t *dll_filename;
		LPWSTR pszIcon;
	};

	static const IconDllData_t iconDllData[] = {
		{L"imageres.dll", MAKEINTRESOURCE(34)},	// Vista+
		{L"shell32.dll",  MAKEINTRESOURCE(13)},
	};

	for (unsigned int i = 0; i < ARRAY_SIZE(iconDllData); i++) {
		HMODULE hDll = LoadLibrary(iconDllData[i].dll_filename);
		if (!hDll)
			continue;

		// Check for the specified icon resource.
		HRSRC hRes = FindResource(hDll, iconDllData[i].pszIcon, RT_GROUP_ICON);
		if (hRes) {
			// Found a usable icon resource.
			hIcon = static_cast<HICON>(LoadImage(
				hDll, iconDllData[i].pszIcon, IMAGE_ICON,
				GetSystemMetrics(SM_CXICON),
				GetSystemMetrics(SM_CYICON), 0));
			hIconSmall = static_cast<HICON>(LoadImage(
				hDll, iconDllData[i].pszIcon, IMAGE_ICON,
				GetSystemMetrics(SM_CXSMICON),
				GetSystemMetrics(SM_CYSMICON), 0));

			FreeLibrary(hDll);
			return TRUE;
		}

		// Icon not found in this DLL.
		FreeLibrary(hDll);
	}

	// No usable icon...
	hIcon = nullptr;
	hIconSmall = nullptr;

	// NOTE: Returning TRUE anyway to prevent
	// reinitialization, which will fail because
	// system DLLs shouldn't change at runtime.
	return TRUE;
}

/** PropSheetIcon **/

/**
 * Get the large property sheet icon.
 * @return Large property sheet icon, or nullptr on error.
 */
HICON PropSheetIcon::getLargeIcon(void)
{
	InitOnceExecuteOnce(&PropSheetIconPrivate::initOnce,
		PropSheetIconPrivate::getPropSheetIcons, nullptr, nullptr);
	return PropSheetIconPrivate::hIcon;
}

/**
 * Get the small property sheet icon.
 * @return Small property sheet icon, or nullptr on error.
 */
HICON PropSheetIcon::getSmallIcon(void)
{
	InitOnceExecuteOnce(&PropSheetIconPrivate::initOnce,
		PropSheetIconPrivate::getPropSheetIcons, nullptr, nullptr);
	return PropSheetIconPrivate::hIconSmall;
}
