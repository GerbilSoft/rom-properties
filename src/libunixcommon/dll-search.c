/***************************************************************************
 * ROM Properties Page shell extension. (libunixcommon)                    *
 * dll-search.c: Function to search for a usable rom-properties library.   *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
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
#  define static_assert _Static_assert
#endif

// Supported rom-properties frontends.
typedef enum {
	RP_FE_KDE4,
	RP_FE_KF5,
	RP_FE_KF6,
	RP_FE_GTK2,	// XFCE (Thunar 1.6)
	RP_FE_GTK3,	// GNOME, MATE, Cinnamon, XFCE (Thunar 1.8)
	RP_FE_GTK4,	// GNOME 43

	RP_FE_MAX
} RP_Frontend;

// Extension paths
// Two paths per DE are supported, since KF5 5.89 and later
// supports namespaced plugins.
static const char *const RP_Extension_Path[RP_FE_MAX][2] = {
#ifdef KDE4_PLUGIN_INSTALL_DIR
	{KDE4_PLUGIN_INSTALL_DIR "/rom-properties-kde4.so", NULL},
#else
	{NULL, NULL},
#endif
#ifdef KF5_PLUGIN_INSTALL_DIR
	{KF5_PRPD_PLUGIN_INSTALL_DIR "/rom-properties-kf5.so",
	 KF5_PLUGIN_INSTALL_DIR "/rom-properties-kf5.so"},
#else
	{NULL, NULL},
#endif
#ifdef KF6_PLUGIN_INSTALL_DIR
	{KF6_PRPD_PLUGIN_INSTALL_DIR "/rom-properties-kf6.so",
	 KF6_PLUGIN_INSTALL_DIR "/rom-properties-kf6.so"},
#else
	{NULL, NULL},
#endif
#ifdef ThunarX2_EXTENSIONS_DIR
	{ThunarX2_EXTENSIONS_DIR "/rom-properties-xfce.so", NULL},
#else
	{NULL, NULL},
#endif
#ifdef LibNautilusExtension3_EXTENSION_DIR
	{LibNautilusExtension3_EXTENSION_DIR "/rom-properties-gtk3.so", NULL},
#else
	{NULL, NULL},
#endif
#ifdef LibNautilusExtension4_EXTENSION_DIR
	{LibNautilusExtension4_EXTENSION_DIR "/rom-properties-gtk4.so", NULL},
#else
	{NULL, NULL},
#endif
};

// Plugin priority order.
// - Index: Current desktop environment. (RP_Frontend)
// - Value: Plugin to use. (RP_Frontend)
static const uint8_t plugin_prio[RP_FE_MAX][RP_FE_MAX] = {
	{RP_FE_KDE4, RP_FE_KF5, RP_FE_KF6, RP_FE_GTK2, RP_FE_GTK3, RP_FE_GTK4},	// RP_FE_KDE4
	{RP_FE_KF5, RP_FE_KF6, RP_FE_KDE4, RP_FE_GTK4, RP_FE_GTK3, RP_FE_GTK2},	// RP_FE_KF5
	{RP_FE_KF6, RP_FE_KF5, RP_FE_KDE4, RP_FE_GTK4, RP_FE_GTK3, RP_FE_GTK2},	// RP_FE_KF6
	{RP_FE_GTK2, RP_FE_GTK3, RP_FE_GTK4, RP_FE_KF6, RP_FE_KF5, RP_FE_KDE4},	// RP_FE_GTK2
	{RP_FE_GTK3, RP_FE_GTK4, RP_FE_GTK2, RP_FE_KF6, RP_FE_KF5, RP_FE_KDE4},	// RP_FE_GTK3
	{RP_FE_GTK4, RP_FE_GTK3, RP_FE_GTK2, RP_FE_KF6, RP_FE_KF5, RP_FE_KDE4},	// RP_FE_GTK4
};

/**
 * Get a process's executable name.
 * @param pid		[in] Process ID.
 * @param pidname	[out] String buffer.
 * @param len		[in] Size of buf.
 * @return 0 on success; non-zero on error.
 */
int rp_get_process_name(pid_t pid, char *pidname, size_t len, pid_t *ppid)
{
	int ret = -EIO;

	// Zero the ppid initially.
	if (ppid) {
		*ppid = 0;
	}

#ifdef __linux__
	// Open the /proc/$PID/status file.
	char buf[64];
	snprintf(buf, sizeof(buf), "/proc/%d/status", pid);
	errno = 0;
	FILE *f = fopen(buf, "r");
	if (!f) {
		// Unable to open the file...
		int err = errno;
		if (err == 0) {
			err = EIO;
		}
		return err;
	}

	while (!feof(f) && fgets(buf, sizeof(buf), f) != NULL) {
		if (buf[0] == 0)
			break;

		// "Name:\t" is always the first line.
		if (!strncmp(buf, "Name:\t", 6)) {
			// Found the "Name:" row.
			const char *const s_value = &buf[6];
			const size_t s_value_sz = sizeof(buf)-6;

			const char *const chr = memchr(s_value, '\n', s_value_sz);
			if (!chr)
				continue;

			size_t namelen = (int)(chr - s_value);
			if (namelen > len - 1) {
				namelen = len - 1;
			}
			memcpy(pidname, s_value, namelen);
			pidname[namelen] = '\0';

			if (!ppid) {
				// Not searching for ppid, so we can stop here.
				ret = 0;
				break;
			}
		} else if (!strncmp(buf, "PPid:\t", 6)) {
			// Found the "PPid:" row.
			if (ppid) {
				char *endptr = NULL;
				*ppid = (pid_t)strtol(&buf[6], &endptr, 10);
				if (*endptr != 0 && *endptr != '\n') {
					// Invalid numeric value...
					*ppid = 0;
				} else {
					// Valid ppid.
					ret = 0;
				}
				break;
			}
		}
	}
	fclose(f);
#else /* !__linux__ */
	// Not supported.
	ret = -ENOSYS;
#endif

	return ret;
}

/**
 * Walk through the process tree to determine the active desktop environment.
 * @return RP_Frontend, or RP_FE_MAX if unknown.
 */
static RP_Frontend walk_proc_tree(void)
{
	RP_Frontend ret_fe = RP_FE_MAX;

#ifdef __linux__
	// Linux-specific: Walk through /proc.
	pid_t ppid = getppid();
	for (unsigned int max_pid_check = 64; ppid > 1 && max_pid_check > 0; max_pid_check--) {
		// Open the /proc/$PID/status file.
		char process_name[32];

		const pid_t pid = ppid;
		int ret = rp_get_process_name(pid, process_name, sizeof(process_name), &ppid);
		if (ret != 0) {
			// Error getting the parent process information.
			break;
		}

		// Check the parent process name.
		// NOTE: Unity and XFCE don't have unique parent processes.
		// FIXME: Handle ksmserver...
		// NOTE: "kdeinit6" is not part of KF6.
		if (!strcmp(process_name, "kdeinit5")) {
			// Found kdeinit5.
			ret_fe = RP_FE_KF5;
			ppid = 0;
			break;
		} else if (!strcmp(process_name, "kdeinit4")) {
			// Found kdeinit4.
			ret_fe = RP_FE_KDE4;
			ppid = 0;
			break;
		} else if (!strcmp(process_name, "gnome-panel") ||
		           !strcmp(process_name, "gnome-session") ||
		           !strcmp(process_name, "mate-panel") ||
		           !strcmp(process_name, "mate-session") ||
		           !strcmp(process_name, "cinnamon-panel") ||
		           !strcmp(process_name, "cinnamon-session"))
		{
			// GNOME, MATE, or Cinnamon session.
			// TODO: Verify the Cinnamon process names.
			ret_fe = RP_FE_GTK3;
			ppid = 0;
			break;
		}
	}
#else
#  warning walk_proc_tree() is not implemented for this OS.
#endif

	return ret_fe;
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
	RP_Frontend ret = RP_FE_MAX;
	if (!strcasecmp(name, "KDE")) {
		// KDE
		// Check if KDE_SESSION_VERSION is set.
		const char *const ksv = getenv("KDE_SESSION_VERSION");
		if (ksv) {
			// Convert the environment variable to a number.
			char *endptr = NULL;
			long ver = strtoul(ksv, &endptr, 10);
			if (endptr && *endptr == '\0') {
				switch (ver) {
					default:
						break;
					case 4:
						ret = RP_FE_KDE4;
						break;
					case 5:
						ret = RP_FE_KF5;
						break;
					case 6:
						ret = RP_FE_KF6;
						break;
				}
			}
		}

		if (ret >= RP_FE_MAX) {
			// KDE_SESSION_VERSION is not set.
			// Check parent processes to determine the version.
			// NOTE: Assuming KF5 if unable to determine the KDE version.
			// TODO: Check for KF6?
			ret = walk_proc_tree();
			if (ret >= RP_FE_MAX) {
				// Not set. Assume KF5.
				ret = RP_FE_KF5;
			}
		}
	} else if (!strcasecmp(name, "GNOME") ||
	           !strcasecmp(name, "Unity") ||
	           !strcasecmp(name, "MATE") ||
	           !strcasecmp(name, "X-Cinnamon") ||
	           !strcasecmp(name, "Cinnamon"))
	{
		// GTK3-based desktop environment. (GNOME or GNOME-like)
		// TODO: Determine if it's actually GTK4.
		ret = RP_FE_GTK3;
	} else if (!strcasecmp(name, "XFCE") ||
		   !strcasecmp(name, "LXDE"))
	{
		// This *might* be GTK3 if it's new enough.
		ret = RP_FE_GTK3;
	}

	// NOTE: The following desktop names are not actually used.
	// They're used here for debugging purposes only.
	if (!strcasecmp(name, "KF6") || !strcasecmp(name, "KDE6")) {
		ret = RP_FE_KF6;
	} else if (!strcasecmp(name, "KF5") || !strcasecmp(name, "KDE5")) {
		ret = RP_FE_KF5;
	} else if (!strcasecmp(name, "KDE4")) {
		ret = RP_FE_KDE4;
	} else if (!strcasecmp(name, "GTK4")) {
		ret = RP_FE_GTK4;
	} else if (!strcasecmp(name, "GTK3")) {
		ret = RP_FE_GTK3;
	} else if (!strcasecmp(name, "GTK2")) {
		ret = RP_FE_GTK2;
	}

	return ret;
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
		     token != NULL; token = strtok_r(NULL, ":", &saveptr))
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
		// No match. Assume RP_FE_GTK3.
		ret = RP_FE_GTK3;
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
		static const char de_name_tbl[][8] = {
			"KDE4", "KF5", "KF6",
			"GTK2", "GTK3", "GTK4",
		};
		static_assert(sizeof(de_name_tbl)/sizeof(de_name_tbl[0]) == RP_FE_MAX,
			"de_name_tbl[] needs to be updated.");
		if (cur_desktop == RP_FE_MAX) {
			pfnDebug(LEVEL_DEBUG, "*** Could not determine active desktop environment. Defaulting to GTK3.");
		} else {
			pfnDebug(LEVEL_DEBUG, "Active desktop environment: %s", de_name_tbl[cur_desktop]);
		}
	}

	if (cur_desktop == RP_FE_MAX) {
		// Default to GTK3.
		cur_desktop = RP_FE_GTK3;
	}

	const uint8_t *const prio = &plugin_prio[cur_desktop][0];
	*ppDll = NULL;
	*ppfn = NULL;
	for (unsigned int i = 0; i < RP_FE_MAX; i++) {
	for (unsigned int j = 0; !*ppfn && j < 2; j++) {
		// Attempt to open this plugin.
		const char *const plugin_path = RP_Extension_Path[prio[i]][j];
		if (!plugin_path)
			continue;

		if (pfnDebug) {
			pfnDebug(LEVEL_DEBUG, "Attempting to open: %s", plugin_path);
		}
		*ppDll = dlopen(plugin_path, RTLD_LOCAL|RTLD_LAZY);
		if (!*ppDll) {
			// Library not found, or unable to open the library.
			if (pfnDebug) {
				pfnDebug(LEVEL_DEBUG, "*** dlopen() failed: %s", dlerror());
			}
			continue;
		}

		// Find the requested symbol.
		if (pfnDebug) {
			pfnDebug(LEVEL_DEBUG, "Checking for symbol: %s", symname);
		}
		*ppfn = dlsym(*ppDll, symname);
		if (!*ppfn) {
			// Symbol not found.
			if (pfnDebug) {
				pfnDebug(LEVEL_DEBUG, "*** dlsym() failed: %s", dlerror());
			}
			dlclose(*ppDll);
			*ppDll = NULL;
			continue;
		}

		// Found the symbol.
		break;
	} }

	if (!*ppfn) {
		if (pfnDebug) {
			pfnDebug(LEVEL_ERROR, "*** ERROR: Could not find %s() in any installed rom-properties plugin.", symname);
		}
		return -ENOENT;
	}

	return 0;
}
