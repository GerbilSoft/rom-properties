/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * pfn_adwaita.c: libadwaita/libhandy function pointer handling.           *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.gtk.h"
#include "pfn_adwaita.h"

// libdl
#ifdef HAVE_DLVSYM
#  ifndef _GNU_SOURCE
#    define _GNU_SOURCE 1
#  endif /* _GNU_SOURCE */
#else /* !HAVE_DLVSYM */
#  define dlvsym(handle, symbol, version) dlsym((handle), (symbol))
#endif /* HAVE_DLVSYM */
#include <dlfcn.h>

// libadwaita/libhandy function pointers.
// Only initialized if libadwaita/libhandy is linked into the process.
// NOTE: The function pointers are essentially the same, but
// libhandy was renamed to libadwaita for the GTK4 conversion.
// We'll use libadwaita terminology everywhere.

static gsize has_checked_adw = 0;
pfnGlibGetType_t pfn_adw_deck_get_type = NULL;
pfnGlibGetType_t pfn_adw_header_bar_get_type = NULL;
pfnAdwHeaderBarPackEnd_t pfn_adw_header_bar_pack_end = NULL;

#if GTK_CHECK_VERSION(4,0,0)
   // GTK4: libadwaita
#  define ADW_SYM_PREFIX "adw_"
#  define ADW_SYM_VERSION "LIBADWAITA_1_0"
#else /* !GTK_CHECK_VERSION(4,0,0) */
   // GTK3: libhandy
#  define ADW_SYM_PREFIX "hdy_"
#  define ADW_SYM_VERSION "LIBHANDY_1_0"
#endif

/**
 * Initialize libadwaita/libhandy function pointers.
 * @return TRUE on success; FALSE on failure.
 */
gboolean rp_init_pfn_adwaita(void)
{
	if (g_once_init_enter(&has_checked_adw)) {
		// Check if libadwaita-1 is loaded in the process.
		// TODO: Verify that it is in fact 1.x if symbol versioning isn't available.
		pfn_adw_deck_get_type = (pfnGlibGetType_t)dlvsym(
			RTLD_DEFAULT, ADW_SYM_PREFIX "deck_get_type", ADW_SYM_VERSION);
		if (pfn_adw_deck_get_type) {
			pfn_adw_header_bar_get_type = (pfnGlibGetType_t)dlvsym(
				RTLD_DEFAULT, ADW_SYM_PREFIX "header_bar_get_type", ADW_SYM_VERSION);
			pfn_adw_header_bar_pack_end = (pfnAdwHeaderBarPackEnd_t)dlvsym(
				RTLD_DEFAULT, ADW_SYM_PREFIX "header_bar_pack_end", ADW_SYM_VERSION);
		}

		g_once_init_leave(&has_checked_adw, 1);
	}

	return (pfn_adw_deck_get_type != NULL);
}
