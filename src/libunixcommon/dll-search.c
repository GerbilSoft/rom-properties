/***************************************************************************
 * ROM Properties Page shell extension. (libunixcommon)                    *
 * dll-search.c: Function to search for a usable rom-properties library.   *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.libunixcommon.h"
#include "dll-search.h"

// C includes.
#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// OpenBSD 6.5 doesn't have the static_assert() macro,
// even though it *does* use LLVM/Clang 7.0.1.
#ifndef static_assert
# define static_assert _Static_assert
#endif

// Supported rom-properties frontends.
typedef enum {
	RP_FE_KDE4,
	RP_FE_KDE5,
	RP_FE_XFCE,
	RP_FE_XFCE3,
	RP_FE_GNOME,
	RP_FE_MATE,

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
#ifdef ThunarX3_EXTENSIONS_DIR
	ThunarX3_EXTENSIONS_DIR "/rom-properties-xfce3.so",
#else
	NULL,
#endif
#ifdef LibNautilusExtension_EXTENSION_DIR
	LibNautilusExtension_EXTENSION_DIR "/rom-properties-gnome.so",
#else
	NULL,
#endif
#ifdef LibCajaExtension_EXTENSION_DIR
	LibCajaExtension_EXTENSION_DIR "/rom-properties-mate.so",
#else
	NULL,
#endif
};

// Plugin priority order.
// - Index: Current desktop environment. (RP_Frontend)
// - Value: Plugin to use. (RP_Frontend)
static const uint8_t plugin_prio[6][6] = {
	{RP_FE_KDE4, RP_FE_KDE5, RP_FE_XFCE, RP_FE_XFCE3, RP_FE_GNOME, RP_FE_MATE},	// RP_FE_KDE4
	{RP_FE_KDE5, RP_FE_KDE4, RP_FE_GNOME, RP_FE_XFCE, RP_FE_XFCE3, RP_FE_MATE},	// RP_FE_KDE4
	{RP_FE_XFCE, RP_FE_XFCE3, RP_FE_MATE, RP_FE_GNOME, RP_FE_KDE5, RP_FE_KDE4},	// RP_FE_XFCE
	{RP_FE_XFCE3, RP_FE_XFCE, RP_FE_MATE, RP_FE_GNOME, RP_FE_KDE5, RP_FE_KDE4},	// RP_FE_XFCE3
	{RP_FE_GNOME, RP_FE_MATE, RP_FE_XFCE3, RP_FE_XFCE, RP_FE_KDE5, RP_FE_KDE4},	// RP_FE_GNOME
	{RP_FE_MATE, RP_FE_MATE, RP_FE_XFCE3, RP_FE_XFCE, RP_FE_KDE5, RP_FE_KDE4},	// RP_FE_MATE
};

/**
 * Walk through the process tree to determine the active desktop environment.
 * @return RP_Frontend, or RP_FE_MAX if unknown.
 */
static RP_Frontend walk_proc_tree(void)
{
	RP_Frontend ret = RP_FE_MAX;

#ifdef __linux__
	// Linux-specific: Walk through /proc.
	pid_t ppid = getppid();
	while (ppid > 1) {
		// Open the /proc/$PID/status file.
		char buf[128];
		snprintf(buf, sizeof(buf), "/proc/%d/status", ppid);
		FILE *f = fopen(buf, "r");
		if (!f) {
			// Unable to open the file...
			ppid = 0;
			break;
		}

		// Zero the ppid in case we don't find another one.
		// This prevents infinite loops.
		ppid = 0;

		while (!feof(f) && fgets(buf, sizeof(buf), f) != NULL) {
			if (buf[0] == 0)
				break;

			// "Name:\t" is always the first line.
			// If it matches an expected version of KDE, we're done.
			// Otherwise, continue until we find "PPid:\t".
			if (!strncmp(buf, "Name:\t", 6)) {
				// Found the "Name:" row.
				const char *const s_value = &buf[6];
				const size_t s_value_sz = sizeof(buf)-6;

				const char *const chr = memchr(s_value, '\n', s_value_sz);
				if (!chr)
					continue;

				const int len = (int)(chr - s_value);
				if (len == 8) {
					if (!strncmp(s_value, "kdeinit5", 8)) {
						// Found kdeinit5.
						ret = RP_FE_KDE5;
						ppid = 0;
						break;
					} else if (!strncmp(s_value, "kdeinit4", 8)) {
						// Found kdeinit4.
						ret = RP_FE_KDE4;
						ppid = 0;
						break;
					}
				} else if ((len == 11 && !strncmp(s_value, "gnome-panel", 11)) ||
					   (len == 13 && !strncmp(s_value, "gnome-session", 13)))
				{
					// GNOME session.
					ret = RP_FE_GNOME;
					ppid = 0;
					break;
				} else if ((len == 10 && !strncmp(s_value, "mate-panel", 10)) ||
					   (len == 12 && !strncmp(s_value, "mate-session", 12)))
				{
					// MATE session.
					ret = RP_FE_MATE;
					ppid = 0;
					break;
				}
				// NOTE: Unity and XFCE don't have unique
				// parent processes.
			} else if (!strncmp(buf, "PPid:\t", 6)) {
				// Found the "PPid:" row.
				char *endptr = NULL;
				ppid = (pid_t)strtol(&buf[6], &endptr, 10);
				if (*endptr != 0 && *endptr != '\n') {
					// Invalid numeric value...
					ppid = 0;
				}
				// Check the next process.
				break;
			}
		}
		fclose(f);
	}
#else
# warning walk_proc_tree() is not implemented for this OS.
#endif

	return ret;
}

/**
 * Check an XDG desktop name.
 * @param name XDG desktop name.
 * @return RP_Frontend, or RP_FE_MAX if unrecognized.
 */
static inline RP_Frontend check_xdg_desktop_name(const char *name)
{
	// References:
	// - https://askubuntu.com/questions/72549/how-to-determine-which-window-manager-is-running
	// - https://askubuntu.com/a/227669

	// TODO: Check other values for $XDG_CURRENT_DESKTOP.
	if (!strcasecmp(name, "KDE")) {
		// KDE.
		// Check parent processes to determine the version.
		// NOTE: Assuming KDE5 if unable to determine the KDE version.
		RP_Frontend ret = walk_proc_tree();
		if (ret >= RP_FE_MAX) {
			// Unknown. Assume KDE5.
			ret = RP_FE_KDE5;
		}
		return ret;
	} else if (!strcasecmp(name, "GNOME") ||
		   !strcasecmp(name, "Unity") ||
		   !strcasecmp(name, "X-Cinnamon"))
	{
		// GTK3-based desktop environment.
		return RP_FE_GNOME;
	} else if (!strcasecmp(name, "XFCE") ||
		   !strcasecmp(name, "LXDE"))
	{
		// This *may* be GTK3, but it may be GTK2.
		return RP_FE_XFCE3;
	} else if (!strcasecmp(name, "MATE")) {
		// MATE desktop.
		return RP_FE_MATE;
	}

	// NOTE: "KDE4" and "KDE5" are not actually used.
	// They're used here for debugging purposes only.
	if (!strcasecmp(name, "KDE5")) {
		// KDE5.
		return RP_FE_KDE5;
	} else if (!strcasecmp(name, "KDE4")) {
		// KDE4.
		return RP_FE_KDE4;
	}

	// Unknown desktop name.
	return RP_FE_MAX;
}

/**
 * Determine the active desktop environment.
 * @return RP_Frontend.
 */
static RP_Frontend get_active_de(void)
{
	// TODO: What's the correct priority order?
	// Ubuntu 14.04 has $XDG_CURRENT_DESKTOP but not $XDG_SESSION_DESKTOP.
	// Kubuntu 17.04 has both.

	// Check $XDG_CURRENT_DESKTOP first.
	// This is a colon-delimited list of desktop names.
	const char *const xdg_current_desktop = getenv("XDG_CURRENT_DESKTOP");
	if (xdg_current_desktop && xdg_current_desktop[0] != 0) {
		// Use strtok_r() to split the string.
		char *buf = strdup(xdg_current_desktop);
		char *saveptr = NULL;
		for (const char *token = strtok_r(buf, ":", &saveptr);
		     token != nullptr; token = strtok_r(NULL, ":", &saveptr))
		{
			RP_Frontend ret = check_xdg_desktop_name(token);
			if (ret < RP_FE_MAX) {
				// Found a match!
				free(buf);
				return ret;
			}
		}
		free(buf);
	}

	// Check $XDG_SESSION_DESKTOP.
	const char *const xdg_session_desktop = getenv("XDG_SESSION_DESKTOP");
	if (xdg_session_desktop) {
		RP_Frontend ret = check_xdg_desktop_name(xdg_session_desktop);
		if (ret < RP_FE_MAX) {
			// Found a match!
			return ret;
		}
	}

	// Walk the process tree to find a known parent process.
	RP_Frontend ret = walk_proc_tree();
	if (ret >= RP_FE_MAX) {
		// No match. Assume RP_FE_GNOME.
		ret = RP_FE_GNOME;
	}

	return ret;
}

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
	RP_Frontend cur_desktop = get_active_de();
	assert(cur_desktop >= 0);
	assert(cur_desktop <= RP_FE_MAX);
	if (cur_desktop < 0 || cur_desktop > RP_FE_MAX) {
		if (pfnDebug) {
			pfnDebug(LEVEL_ERROR, "*** ERROR: get_active_de() failed.");
		}
		return -EIO;
	}

	// Debug: Print the active desktop environment.
	if (pfnDebug) {
		static const char *const de_name_tbl[] = {
			"KDE4", "KDE5",
			"XFCE (GTK+ 2.x)", "XFCE (GTK+ 3.x)",
			"GNOME", "MATE"
		};
		static_assert(sizeof(de_name_tbl)/sizeof(de_name_tbl[0]) == RP_FE_MAX,
			"de_name_tbl[] needs to be updated.");
		if (cur_desktop == RP_FE_MAX) {
			pfnDebug(LEVEL_DEBUG, "*** Could not determine active desktop environment. Defaulting to GNOME.");
		} else {
			pfnDebug(LEVEL_DEBUG, "Active desktop environment: %s", de_name_tbl[cur_desktop]);
		}
	}

	if (cur_desktop == RP_FE_MAX) {
		// Default to GNOME.
		cur_desktop = RP_FE_GNOME;
	}

	const uint8_t *prio = &plugin_prio[cur_desktop][0];
	*ppDll = NULL;
	*ppfn = NULL;
	for (unsigned int i = RP_FE_MAX-1; i > 0; i--, prio++) {
		// Attempt to open this plugin.
		const char *const plugin_path = RP_Extension_Path[*prio];
		if (!plugin_path)
			continue;

		if (pfnDebug) {
			pfnDebug(LEVEL_DEBUG, "Attempting to open: %s", plugin_path);
		}
		*ppDll = dlopen(plugin_path, RTLD_LOCAL|RTLD_LAZY);
		if (!*ppDll) {
			// Library not found.
			continue;
		}

		// Find the requested symbol.
		if (pfnDebug) {
			pfnDebug(LEVEL_DEBUG, "Checking for symbol: %s", symname);
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
			pfnDebug(LEVEL_ERROR, "*** ERROR: Could not find %s() in any installed rom-properties plugin.", symname);
		}
		return -ENOENT;
	}

	return 0;
}
