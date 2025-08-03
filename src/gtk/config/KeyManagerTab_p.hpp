/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * KeyManagerTab_p.hpp: Key Manager tab for rp-config. (PRIVATE CLASS)     *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "KeyManagerTab.hpp"
#include "RpConfigTab.h"

#include "gtk-compat.h"
#include "gtk-i18n.h"
#include "RpGtk.h"

#include "KeyStoreGTK.hpp"
#include "MessageWidget.h"

#ifdef __cplusplus
extern "C" {
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_BOX
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

// GtkMenuButton was added in GTK 3.6.
// GMenuModel is also implied by this, since GMenuModel
// support was added to GTK+ 3.4.
// NOTE: GtkMenu was removed from GTK4.
#if GTK_CHECK_VERSION(3, 5, 6)
#  define USE_GTK_MENU_BUTTON 1
#  define USE_G_MENU_MODEL 1
#endif /* GTK_CHECK_VERSION(3, 5, 6) */

// Uncomment to use the new GtkColumnView on GTK4.
// FIXME: Editing is broken...
#if GTK_CHECK_VERSION(4, 0, 0)
//#  define RP_KEY_MANAGER_USE_GTK_COLUMN_VIEW 1
#endif /* GTK_CHECK_VERSION(4, 0, 0) */

// KeyManagerTab class
struct _RpKeyManagerTabClass {
	superclass __parent__;
};

// KeyManagerTab instance
struct _RpKeyManagerTab {
	super __parent__;
	bool changed;	// If true, an option was changed.

	RpKeyStoreGTK *keyStore;
	GtkWidget *scrolledWindow;

#ifdef RP_KEY_MANAGER_USE_GTK_COLUMN_VIEW
	// GTK4: GtkColumnView and GtkListModel
	GListStore *rootListStore;
	GtkTreeListModel *treeListModel;
	GtkWidget *columnView;

	// Vector of GListStore objects, one per section.
	std::vector<GListStore*> *vSectionListStore;
#else /* !RP_KEY_MANAGER_USE_GTK_COLUMN_VIEW */
	// GTK2/GTK3: GtkTreeView and GtkTreeStore
	GtkTreeStore *treeStore;
	GtkWidget *treeView;
#endif /* RP_KEY_MANAGER_USE_GTK_COLUMN_VIEW */

	GtkWidget *btnImport;
	gchar *prevOpenDir;
#ifdef USE_G_MENU_MODEL
	GMenu *menuModel;
	GSimpleActionGroup *actionGroup;
#else /* !USE_G_MENU_MODEL */
	GtkWidget *menuImport;	// GtkMenu
#endif /* USE_G_MENU_MODEL */

	// MessageWidget for key import.
	GtkWidget *messageWidget;
};

/**
 * RpKeyManagerTab: GTK version-specific class initialization.
 * @param klass RpKeyManagerTabClass
 */
void rp_key_manager_tab_class_init_gtkver(RpKeyManagerTabClass *klass);

/**
 * Create the GtkTreeStore and GtkTreeView. (GTK2/GTK3)
 * @param tab RpKeyManagerTab
 */
void rp_key_manager_tab_create_GtkTreeView(RpKeyManagerTab *tab);

/**
 * Initialize keys in the GtkTreeView.
 * This initializes sections and key names.
 * Key values and "Valid?" are initialized by reset().
 * @param tab KeyManagerTab
 */
void rp_key_manager_tab_init_keys(RpKeyManagerTab *tab);

/** KeyStoreGTK signal handlers **/

/**
 * A key in the KeyStore has changed.
 * @param keyStore KeyStoreGTK
 * @param sectIdx Section index
 * @param keyIdx Key index
 * @param tab KeyManagerTab
 */
void keyStore_key_changed_signal_handler(RpKeyStoreGTK *keyStore, int sectIdx, int keyIdx, RpKeyManagerTab *tab);

/**
 * All keys in the KeyStore have changed.
 * @param keyStore KeyStoreGTK
 * @param tab KeyManagerTab
 */
void keyStore_all_keys_changed_signal_handler(RpKeyStoreGTK *keyStore, RpKeyManagerTab *tab);

#ifdef __cplusplus
}
#endif
