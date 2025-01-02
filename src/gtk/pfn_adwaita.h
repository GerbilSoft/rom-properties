/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * pfn_adwaita.h: libadwaita/libhandy function pointer handling.           *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

// libadwaita/libhandy function pointers.
// Only initialized if libadwaita/libhandy is linked into the process.
// NOTE: The function pointers are essentially the same, but
// libhandy was renamed to libadwaita for the GTK4 conversion.
// We'll use libadwaita terminology everywhere.
typedef struct _AdwHeaderBar AdwHeaderBar;
typedef GType (*pfnGlibGetType_t)(void);
typedef void (*pfnAdwHeaderBarPackEnd_t)(AdwHeaderBar *self, GtkWidget *child);

#if GTK_CHECK_VERSION(3,0,0)
#  define RP_MAY_HAVE_ADWAITA 1

extern pfnGlibGetType_t pfn_adw_deck_get_type;
extern pfnGlibGetType_t pfn_adw_header_bar_get_type;
extern pfnAdwHeaderBarPackEnd_t pfn_adw_header_bar_pack_end;

/**
 * Initialize libadwaita/libhandy function pointers.
 * @return TRUE on success; FALSE on failure.
 */
gboolean rp_init_pfn_adwaita(void);

#else /* !GTK_CHECK_VERSION(3,0,0) */

// GTK2: No libadwaita/libhandy.
static inline GType pfn_adw_deck_get_type(void)
{
	return 0;
}
static inline GType pfn_adw_header_bar_get_type(void)
{
	return 0;
}
static inline void pfn_adw_header_bar_pack_end(AdwHeaderBar *self, GtkWidget *child)
{
	((void)self);
	((void)child);
}

/**
 * Initialize libadwaita/libhandy function pointers.
 * @return TRUE on success; FALSE on failure.
 */
static inline gboolean rp_init_pfn_adwaita(void)
{
	return FALSE;
}

#endif /* GTK_CHECK_VERSION(3,0,0) */

G_END_DECLS
