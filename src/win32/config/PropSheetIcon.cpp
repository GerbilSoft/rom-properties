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

// One-time initialization.
#include "threads/pthread_once.h"

class PropSheetIconPrivate
{
	private:
		PropSheetIconPrivate();
		~PropSheetIconPrivate();
		RP_DISABLE_COPY(PropSheetIconPrivate)

	public:
		// pthread_once() control variable.
		static pthread_once_t once_control;

		// Property sheet icons.
		static HICON hIcon;
		static HICON hIconSmall;

		// 96x96 icon for the About tab.
		static HICON hIcon96;

		/**
		 * Get the property sheet icons.
		 * NOTE: This function should be called with pthread_once().
		 */
		static void getPropSheetIcons(void);
};

/** PropSheetIconPrivate **/

// pthread_once() control variable.
pthread_once_t PropSheetIconPrivate::once_control = PTHREAD_ONCE_INIT;

// Property sheet icons.
HICON PropSheetIconPrivate::hIcon = nullptr;
HICON PropSheetIconPrivate::hIconSmall = nullptr;

// 96x96 icon for the About tab.
HICON PropSheetIconPrivate::hIcon96 = nullptr;

/**
 * Get the property sheet icons.
 * NOTE: This function should be called with pthread_once().
 */
void PropSheetIconPrivate::getPropSheetIcons(void)
{
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

			// Windows 7 has a 256x256 icon, so it will automatically
			// select that and downscale.
			// Windows XP does not, so it will upscale the 48x48 icon.
			hIcon96 = static_cast<HICON>(LoadImage(
				hDll, iconDllData[i].pszIcon, IMAGE_ICON,
				96, 96, 0));

			FreeLibrary(hDll);
			return;
		}

		// Icon not found in this DLL.
		FreeLibrary(hDll);
	}

	// No usable icon...
	hIcon = nullptr;
	hIconSmall = nullptr;
	hIcon96 = nullptr;

	// NOTE: pthread_once() has no way to indicate
	// an error occurred, but that isn't important
	// here because if we couldn't load the icons
	// the first time, we won't be able to load the
	// icons the second time.
}

/** PropSheetIcon **/

/**
 * Get the large property sheet icon.
 * @return Large property sheet icon, or nullptr on error.
 */
HICON PropSheetIcon::getLargeIcon(void)
{
	// TODO: Handle errors.
	pthread_once(&PropSheetIconPrivate::once_control,
		PropSheetIconPrivate::getPropSheetIcons);
	return PropSheetIconPrivate::hIcon;
}

/**
 * Get the small property sheet icon.
 * @return Small property sheet icon, or nullptr on error.
 */
HICON PropSheetIcon::getSmallIcon(void)
{
	// TODO: Handle errors.
	pthread_once(&PropSheetIconPrivate::once_control,
		PropSheetIconPrivate::getPropSheetIcons);
	return PropSheetIconPrivate::hIconSmall;
}

/**
 * Get the 96x96 icon.
 * @return 96x96 icon, or nullptr on error.
 */
HICON PropSheetIcon::get96Icon(void)
{
	// TODO: Handle errors.
	pthread_once(&PropSheetIconPrivate::once_control,
		PropSheetIconPrivate::getPropSheetIcons);
	return PropSheetIconPrivate::hIcon96;
}
