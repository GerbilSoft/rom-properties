/***************************************************************************
 * ROM Properties Page shell extension. (rp-stub)                          *
 * dll-search.c: Function to search for a usable rom-properties library.   *
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

#include "rp-stub/config.rp-stub.h"
#include "dll-search.h"

// C includes.
#include <dlfcn.h>
#include <errno.h>
#include <string.h>

// Supported rom-properties frontends.
typedef enum {
	RP_FE_KDE4,
	RP_FE_KDE5,
	RP_FE_XFCE,
	RP_FE_GNOME,

	RP_FE_MAX
} RP_Frontend;

// Extension paths.
static const char *const RP_Extension_Path[RP_FE_MAX] = {
#ifdef KDE4_PLUGIN_INSTALL_DIR
	KDE4_PLUGIN_INSTALL_DIR "/rom-properties-kde4.so",
#else
	NULL,
#endif
#ifdef KDE5_PLUGIN_INSTALL_DIR
	KDE5_PLUGIN_INSTALL_DIR "/rom-properties-kde5.so",
#else
	NULL,
#endif
#ifdef ThunarX2_EXTENSIONS_DIR
	ThunarX2_EXTENSIONS_DIR "/rom-properties-xfce.so",
#else
	NULL,
#endif
#ifdef LibNautilusExtension_EXTENSION_DIR
	LibNautilusExtension_EXTENSION_DIR "/rom-properties-gnome.so",
#else
	NULL,
#endif
};

// Plugin priority order.
// - Index: Current desktop environment. (RP_Frontend)
// - Value: Plugin to use. (RP_Frontend)
static const uint8_t plugin_prio[4][4] = {
	{RP_FE_KDE4, RP_FE_KDE5, RP_FE_XFCE, RP_FE_GNOME},	// RP_FE_KDE4
	{RP_FE_KDE5, RP_FE_KDE4, RP_FE_GNOME, RP_FE_XFCE},	// RP_FE_KDE5
	{RP_FE_XFCE, RP_FE_GNOME, RP_FE_KDE5, RP_FE_KDE4},	// RP_FE_XFCE
	{RP_FE_GNOME, RP_FE_XFCE, RP_FE_KDE5, RP_FE_KDE4},	// RP_FE_GNOME
};

/**
 * Search for a rom-properties library.
 * @param symname	[in] Symbol name to look up.
 * @param ppDll		[out] Handle to opened library.
 * @param ppfn		[out] Pointer to function.
 * @param pfnDebug	[in,opt] Pointer to debug logging function. (printf-style) (may be NULL)
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_dll_search(const char *symname, void **ppDll, void **ppfn, PFN_RP_DLL_DEBUG pfnDebug)
{
	// Attempt to open all available plugins.
	// TODO: Determine the current desktop. Assuming XFCE for now.
	const uint8_t cur_desktop = RP_FE_XFCE;

	const uint8_t *prio = &plugin_prio[cur_desktop][0];
	*ppDll = NULL;
	*ppfn = NULL;
	for (unsigned int i = RP_FE_MAX; i > 0; i--, prio++) {
		// Attempt to open this plugin.
		const char *const plugin_path = RP_Extension_Path[*prio];
		if (!plugin_path)
			continue;

		if (pfnDebug) {
			pfnDebug(LEVEL_DEBUG, "Attempting to open: %s\n", plugin_path);
		}
		*ppDll = dlopen(plugin_path, RTLD_LOCAL|RTLD_LAZY);
		if (!*ppDll) {
			// Library not found.
			continue;
		}

		// Find the requested symbol.
		if (pfnDebug) {
			pfnDebug(LEVEL_DEBUG, "Checking for symbol: %s\n", symname);
		}
		*ppfn = dlsym(*ppDll, symname);
		if (!*ppfn) {
			// Symbol not found.
			dlclose(*ppDll);
			*ppDll = NULL;
			continue;
		}

		// Found the symbol.
		break;
	}

	if (!*ppfn) {
		if (pfnDebug) {
			pfnDebug(LEVEL_ERROR, "*** ERROR: Could not find %s() in any installed rom-properties plugin.\n", symname);
		}
		return -ENOENT;
	}

	return 0;
}
