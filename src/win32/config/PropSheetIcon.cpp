/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * PropSheetIcon.cpp: Property sheet icon.                                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "PropSheetIcon.hpp"

// C++ STL classes
using std::array;

class PropSheetIconPrivate
{
public:
	PropSheetIconPrivate();
	~PropSheetIconPrivate();

private:
	RP_DISABLE_COPY(PropSheetIconPrivate)

public:
	// Static PropSheetIcon instance.
	// TODO: Q_GLOBAL_STATIC() equivalent, though we
	// may need special initialization if the compiler
	// doesn't support thread-safe statics.
	static PropSheetIcon instance;

public:
	// Property sheet icons
	HICON hIcon;
	HICON hIconSmall;

	// 96x96 icon for the About tab.
	HICON hIcon96;

	/**
	 * Get the property sheet icons.
	 * NOTE: This function should be called with pthread_once().
	 */
	void getPropSheetIcons(void);
};

/** PropSheetIconPrivate **/

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
PropSheetIcon PropSheetIconPrivate::instance;

PropSheetIconPrivate::PropSheetIconPrivate()
	: hIcon(nullptr)
	, hIconSmall(nullptr)
	, hIcon96(nullptr)
{
	// Check for a DLL containing a usable ROM chip icon.
	struct IconDllData_t {
		LPCTSTR dll_filename;
		LPCTSTR pszIcon;
	};

	// FIXME: constexpr isn't working here for some reason...
	static const array<IconDllData_t, 2> iconDllData = {{
		{_T("imageres.dll"), MAKEINTRESOURCE(34)},	// Vista+
		{_T("shell32.dll"),  MAKEINTRESOURCE(13)},
	}};

	for (const auto &p : iconDllData) {
		HMODULE hDll = LoadLibraryEx(p.dll_filename, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (!hDll)
			continue;

		// Check for the specified icon resource.
		HRSRC hRes = FindResource(hDll, p.pszIcon, RT_GROUP_ICON);
		if (hRes) {
			// Found a usable icon resource.
			hIcon = static_cast<HICON>(LoadImage(
				hDll, p.pszIcon, IMAGE_ICON,
				GetSystemMetrics(SM_CXICON),
				GetSystemMetrics(SM_CYICON), 0));
			hIconSmall = static_cast<HICON>(LoadImage(
				hDll, p.pszIcon, IMAGE_ICON,
				GetSystemMetrics(SM_CXSMICON),
				GetSystemMetrics(SM_CYSMICON), 0));

			// Windows 7 has a 256x256 icon, so it will automatically
			// select that and downscale.
			// Windows XP does not, so it will upscale the 48x48 icon.
			hIcon96 = static_cast<HICON>(LoadImage(
				hDll, p.pszIcon, IMAGE_ICON,
				96, 96, 0));

			FreeLibrary(hDll);
			break;
		}

		// Icon not found in this DLL.
		FreeLibrary(hDll);
	}
}

PropSheetIconPrivate::~PropSheetIconPrivate()
{
	if (hIcon) {
		DestroyIcon(hIcon);
	}
	if (hIconSmall) {
		DestroyIcon(hIconSmall);
	}
	if (hIcon96) {
		DestroyIcon(hIcon96);
	}
}

/** PropSheetIcon **/

PropSheetIcon::PropSheetIcon()
	: d_ptr(new PropSheetIconPrivate())
{}

PropSheetIcon::~PropSheetIcon()
{
	delete d_ptr;
}

/**
 * Get the PropSheetIcon instance.
 * @return PropSheetIcon instance.
 */
PropSheetIcon *PropSheetIcon::instance(void)
{
	return &PropSheetIconPrivate::instance;
}

/**
 * Get the large property sheet icon.
 * @return Large property sheet icon, or nullptr on error.
 */
HICON PropSheetIcon::getLargeIcon(void) const
{
	RP_D(const PropSheetIcon);
	return d->hIcon;
}

/**
 * Get the small property sheet icon.
 * @return Small property sheet icon, or nullptr on error.
 */
HICON PropSheetIcon::getSmallIcon(void) const
{
	RP_D(const PropSheetIcon);
	return d->hIconSmall;
}

/**
 * Get the 96x96 icon.
 * @return 96x96 icon, or nullptr on error.
 */
HICON PropSheetIcon::get96Icon(void) const
{
	RP_D(const PropSheetIcon);
	return d->hIcon96;
}
