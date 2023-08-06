/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * KeyManagerTab_p.hpp: Key Manager tab for rp-config. (PRIVATE CLASS)     *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "KeyManagerTab.hpp"
#include "RpConfigTab.h"

#include "gtk-compat.h"
#include "gtk-i18n.h"
#include "RpGtk.hpp"

#include "KeyStoreGTK.hpp"
#include "MessageWidget.h"

#ifdef __cplusplus
extern "C" {
#endif

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_BOX
#  define USE_GTK_GRID 1	// Use GtkGrid instead of GtkTable.
#else /* !GTK_CHECK_VERSION(3,0,0) */
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif /* GTK_CHECK_VERSION(3,0,0) */

// GtkMenuButton was added in GTK 3.6.
// GMenuModel is also implied by this, since GMenuModel
// support was added to GTK+ 3.4.
// NOTE: GtkMenu was removed from GTK4.
#if GTK_CHECK_VERSION(3,5,6)
#  define USE_GTK_MENU_BUTTON 1
#  define USE_G_MENU_MODEL 1
#endif /* GTK_CHECK_VERSION(3,5,6) */

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

	// GTK2/GTK3: TreeView and backing store
	GtkTreeStore *treeStore;
	GtkWidget *treeView;

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

/**
 * KeyStore has been modified.
 * This signal is forwarded to the parent ConfigDialog.
 * @param keyStore KeyStoreGTK
 * @param tab KeyManagerTab
 */
void keyStore_modified_signal_handler(RpKeyStoreGTK *keyStore, RpKeyManagerTab *tab);

#ifdef __cplusplus
}
#endif
