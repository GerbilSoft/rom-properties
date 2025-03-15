/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * LanguageComboBox.cpp: Language GtkComboBox subclass                     *
 * (PRIVATE CLASS)                                                         *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_SELECTED_LC,
	PROP_FORCE_PAL,

	PROP_LAST
} RpLanguageComboBoxPropID;

extern GParamSpec *rp_language_combo_box_props[PROP_LAST];

#if GTK_CHECK_VERSION(3, 0, 0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_BOX
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
typedef GtkHBoxClass superclass;
typedef GtkHBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_HBOX
#endif

// LanguageComboBox class
struct _RpLanguageComboBoxClass {
	superclass __parent__;
};

// LanguageComboBox instance
struct _RpLanguageComboBox {
	super __parent__;

#ifdef USE_GTK_DROP_DOWN
	GtkWidget *dropDown;
	GListStore *listStore;
#else /* !USE_GTK_DROP_DOWN */
	GtkWidget *comboBox;
	GtkListStore *listStore;
#endif /* USE_GTK_DROP_DOWN */

	gboolean forcePAL;
};

/**
 * Initialize the GTK3/GTK4-specific portion of the LanguageComboBox.
 * @param widget RpLanguageComboBox
 */
void rp_language_combo_box_init_gtkX(struct _RpLanguageComboBox *widget);

/**
 * Rebuild the language icons.
 * @param widget RpLanguageComboBox
 */
void
rp_language_combo_box_rebuild_icons(struct _RpLanguageComboBox *widget);

G_END_DECLS
