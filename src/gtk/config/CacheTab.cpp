/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CacheTab.cpp: Thumbnail Cache tab for rp-config.                        *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CacheTab.hpp"
#include "RpConfigTab.h"

#include "RpGtk.hpp"
#include "gtk-compat.h"

// librpbase
using namespace LibRpBase;

// C++ STL classes
using std::string;

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#define GTK_TYPE_SUPER GTK_TYPE_BOX
#else
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif

// CacheTab class
struct _CacheTabClass {
	superclass __parent__;
};

// CacheTab instance
struct _CacheTab {
	super __parent__;

	GtkWidget *lblSysCache;
	GtkWidget *btnSysCache;
	GtkWidget *lblRpCache;
	GtkWidget *btnRpCache;

	GtkWidget *lblStatus;
	GtkWidget *pbStatus;

#if !GTK_CHECK_VERSION(4,0,0)
	GdkCursor *curBusy;
#endif /* !GTK_CHECK_VERSION(4,0,0) */
};

static void	cache_tab_dispose			(GObject	*object);
static void	cache_tab_finalize			(GObject	*object);

// Interface initialization
static void	cache_tab_rp_config_tab_interface_init	(RpConfigTabInterface *iface);
static gboolean	cache_tab_has_defaults			(CacheTab	*tab);
static void	cache_tab_reset				(CacheTab	*tab);
static void	cache_tab_load_defaults			(CacheTab	*tab);
static void	cache_tab_save				(CacheTab	*tab,
							 GKeyFile       *keyFile);

// Enable/disable the UI controls.
static void	cache_tab_enable_ui_controls		(CacheTab	*tab,
							 gboolean	enable);

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(CacheTab, cache_tab,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0),
		G_IMPLEMENT_INTERFACE(RP_CONFIG_TYPE_TAB,
			cache_tab_rp_config_tab_interface_init));

static void
cache_tab_class_init(CacheTabClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = cache_tab_dispose;
	gobject_class->finalize = cache_tab_finalize;
}

static void
cache_tab_rp_config_tab_interface_init(RpConfigTabInterface *iface)
{
	iface->has_defaults = (__typeof__(iface->has_defaults))cache_tab_has_defaults;
	iface->reset = (__typeof__(iface->reset))cache_tab_reset;
	iface->load_defaults = (__typeof__(iface->load_defaults))cache_tab_load_defaults;
	iface->save = (__typeof__(iface->save))cache_tab_save;
}

static void
cache_tab_init(CacheTab *tab)
{
#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(tab), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3,0,0) */
	gtk_box_set_spacing(GTK_BOX(tab), 8);

	// FIXME: Better wrapping that doesn't require manual newlines.
	tab->lblSysCache = gtk_label_new(
		C_("CacheTab", "If any image type settings were changed, you will need\nto clear the system thumbnail cache."));
	GTK_LABEL_XALIGN_LEFT(tab->lblSysCache);
	gtk_label_set_wrap(GTK_LABEL(tab->lblSysCache), TRUE);

	tab->lblRpCache = gtk_label_new(
		C_("CacheTab", "ROM Properties Page maintains its own download cache for\n"
		               "external images.\n"
			       "Clearing this cache will force external images to be\n"
			       "redownloaded."));
	GTK_LABEL_XALIGN_LEFT(tab->lblRpCache);
	gtk_label_set_wrap(GTK_LABEL(tab->lblRpCache), TRUE);

	tab->btnSysCache = gtk_button_new_with_label(C_("CacheTab", "Clear the System Thumbnail Cache"));
	tab->btnRpCache  = gtk_button_new_with_label(C_("CacheTab", "Clear the ROM Properties Page Download Cache"));

	tab->lblStatus = gtk_label_new(nullptr);
	tab->pbStatus = gtk_progress_bar_new();
#if GTK_CHECK_VERSION(3,0,0)
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(tab->pbStatus), TRUE);
#endif /* !GTK_CHECK_VERSION(3,0,0) */

#if GTK_CHECK_VERSION(4,0,0)
	gtk_widget_hide(tab->lblStatus);
	gtk_widget_hide(tab->pbStatus);

	gtk_box_append(GTK_BOX(tab), tab->lblSysCache);
	gtk_box_append(GTK_BOX(tab), tab->btnSysCache);
	gtk_box_append(GTK_BOX(tab), tab->lblRpCache);
	gtk_box_append(GTK_BOX(tab), tab->btnRpCache);

	// TODO: Spacer and/or alignment?
	gtk_box_append(GTK_BOX(tab), tab->lblStatus);
	gtk_box_append(GTK_BOX(tab), tab->pbStatus);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	// NOTE: GTK4 defaults to visible; GTK2 and GTK3 defaults to invisible.
	// Hiding unconditionally just in case.
	gtk_widget_hide(tab->lblStatus);
	gtk_widget_hide(tab->pbStatus);

	gtk_widget_show(tab->lblSysCache);
	gtk_widget_show(tab->btnSysCache);
	gtk_widget_show(tab->lblRpCache);
	gtk_widget_show(tab->btnRpCache);

	gtk_box_pack_start(GTK_BOX(tab), tab->lblSysCache, false, false, 0);
	gtk_box_pack_start(GTK_BOX(tab), tab->btnSysCache, false, false, 0);
	gtk_box_pack_start(GTK_BOX(tab), tab->lblRpCache, false, false, 0);
	gtk_box_pack_start(GTK_BOX(tab), tab->btnRpCache, false, false, 0);

	// TODO: Spacer and/or alignment?
	gtk_box_pack_end(GTK_BOX(tab), tab->lblStatus, false, false, 0);
	gtk_box_pack_end(GTK_BOX(tab), tab->pbStatus, false, false, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Load the current configuration.
	cache_tab_reset(tab);
}

static void
cache_tab_dispose(GObject *object)
{
	//CacheTab *const tab = CACHE_TAB(object);

	// NOTE: We can't clear the busy cursor here because
	// the window is being destroyed.

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(cache_tab_parent_class)->dispose(object);
}

static void
cache_tab_finalize(GObject *object)
{
#if !GTK_CHECK_VERSION(4,0,0)
	CacheTab *const tab = CACHE_TAB(object);

	if (tab->curBusy) {
		g_object_unref(tab->curBusy);
	}
#endif /* !GTK_CHECK_VERSION(4,0,0) */

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(cache_tab_parent_class)->finalize(object);
}

GtkWidget*
cache_tab_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(TYPE_CACHE_TAB, nullptr));
}

/** RpConfigTab interface functions **/

static gboolean
cache_tab_has_defaults(CacheTab *tab)
{
	g_return_val_if_fail(IS_CACHE_TAB(tab), false);
	return false;
}

static void
cache_tab_reset(CacheTab *tab)
{
	g_return_if_fail(IS_CACHE_TAB(tab));

	// Not implemented.
	return;
}

static void
cache_tab_load_defaults(CacheTab *tab)
{
	g_return_if_fail(IS_CACHE_TAB(tab));

	// Not implemented.
	return;
}

static void
cache_tab_save(CacheTab *tab, GKeyFile *keyFile)
{
	g_return_if_fail(IS_CACHE_TAB(tab));
	g_return_if_fail(keyFile != nullptr);

	// Not implemented.
	return;
}

/** Miscellaneous **/

/**
 * Enable/disable the UI controls.
 * @param tab CacheTab
 * @param enable True to enable; false to disable.
 */
static void
cache_tab_enable_ui_controls(CacheTab *tab, gboolean enable)
{
	// TODO: Disable the main tab control too?
	gtk_widget_set_sensitive(tab->lblSysCache, enable);
	gtk_widget_set_sensitive(tab->btnSysCache, enable);
	gtk_widget_set_sensitive(tab->lblRpCache, enable);
	gtk_widget_set_sensitive(tab->btnRpCache, enable);

	// Set the busy cursor if needed.
	GtkWidget *const widget = GTK_WIDGET(tab);
#if GTK_CHECK_VERSION(4,0,0)
	gtk_widget_set_cursor_from_name(widget, (enable ? "wait" : nullptr));
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GdkWindow *const gdkWindow = gtk_widget_get_window(widget);
	if (gdkWindow) {
		if (enable) {
			// Regular cursor.
			gdk_window_set_cursor(gdkWindow, nullptr);
		} else {
			// Busy cursor.
			if (!tab->curBusy) {
				// Create the Busy cursor.
				// TODO: Also if the theme changes?
				// TODO: Verify that this doesn't leak.
				tab->curBusy = gdk_cursor_new_from_name(gtk_widget_get_display(widget), "wait");
				g_object_ref_sink(tab->curBusy);
			}
			gdk_window_set_cursor(gdkWindow, tab->curBusy);
		}
	}
#endif /* !GTK_CHECK_VERSION(4,0,0) */
}
