/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CacheTab.cpp: Thumbnail Cache tab for rp-config.                        *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "CacheTab.hpp"
#include "RpConfigTab.hpp"
#include "../MessageSound.hpp"
#include "CacheCleaner.hpp"

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
#else /* !GTK_CHECK_VERSION(3,0,0) */
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif /* GTK_CHECK_VERSION(3,0,0) */

// CacheTab class
struct _CacheTabClass {
	superclass __parent__;
};

// CacheTab instance
struct _CacheTab {
	super __parent__;

	GtkWidget	*lblSysCache;
	GtkWidget	*btnSysCache;
	GtkWidget	*lblRpCache;
	GtkWidget	*btnRpCache;

	GtkWidget	*lblStatus;
	GtkWidget	*pbStatus;

	CacheCleaner	*ccCleaner;
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

// Widget signal handlers
static void	cache_tab_on_btnSysCache_clicked	(GtkButton	*button,
							 CacheTab	*tab);
static void	cache_tab_on_btnRpCache_clicked		(GtkButton	*button,
							 CacheTab	*tab);

// CacheCleaner signal handlers
static void	ccCleaner_progress			(CacheCleaner	*cleaner,
							 int		 pg_cur,
							 int		 pg_max,
							 gboolean	 hasError,
							 CacheTab	*tab);
static void	ccCleaner_error				(CacheCleaner	*cleaner,
							 const char	*error,
							 CacheTab	*tab);
static void	ccCleaner_cacheIsEmpty			(CacheCleaner	*cleaner,
							 RpCacheDir	 cache_dir,
							 CacheTab	*tab);
static void	ccCleaner_cacheCleared			(CacheCleaner	*cleaner,
							 RpCacheDir	 cacheDir,
							 unsigned int	 dirErrs,
							 unsigned int	 fileErrs,
							 CacheTab	*tab);
static void	ccCleaner_finished			(CacheCleaner	*cleaner,
							 CacheTab	*tab);

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

#if GTK_CHECK_VERSION(3,0,0)
	// Add a CSS class for a GtkProgressBar "error" state.
	GtkCssProvider *const provider = gtk_css_provider_new();
	GdkDisplay *const display = gdk_display_get_default();
#  if GTK_CHECK_VERSION(4,0,0)
	// GdkScreen no longer exists in GTK4.
	// Style context providers are added directly to GdkDisplay instead.
	gtk_style_context_add_provider_for_display(display,
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
#  else /* !GTK_CHECK_VERSION(4,0,0) */
	GdkScreen *const screen = gdk_display_get_default_screen(display);
	gtk_style_context_add_provider_for_screen(screen,
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
#  endif /* !GTK_CHECK_VERSION(4,0,0) */

	static const char css_ProgressBar[] =
		"@define-color gsrp_color_pb_error rgb(144,24,24);\n"
		"progressbar.gsrp_pb_error > trough > progress {\n"
		"\tbackground-image: none;\n"
		"\tbackground-color: lighter(@gsrp_color_pb_error);\n"
		"\tborder: solid @gsrp_color_info;\n"
		"}\n";

	GTK_CSS_PROVIDER_LOAD_FROM_DATA(GTK_CSS_PROVIDER(provider), css_ProgressBar, -1);
	g_object_unref(provider);
#endif /* GTK_CHECK_VERSION(3,0,0) */
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
	tab->lblSysCache = gtk_label_new(reinterpret_cast<const char*>(C_("CacheTab",
		"If any image type settings were changed, you will need\n"
		"to clear the system thumbnail cache.")));
	gtk_widget_set_name(tab->lblSysCache, "lblSysCache");
	GTK_LABEL_XALIGN_LEFT(tab->lblSysCache);
	gtk_label_set_wrap(GTK_LABEL(tab->lblSysCache), TRUE);

	tab->btnSysCache = gtk_button_new_with_label(reinterpret_cast<const char*>(
		C_("CacheTab", "Clear the System Thumbnail Cache")));
	gtk_widget_set_name(tab->btnSysCache, "btnSysCache");

	tab->lblRpCache = gtk_label_new(reinterpret_cast<const char*>(C_("CacheTab",
		"ROM Properties Page maintains its own download cache for external images.\n"
		"Clearing this cache will force external images to be redownloaded.")));
	gtk_widget_set_name(tab->lblRpCache, "lblRpCache");
	GTK_LABEL_XALIGN_LEFT(tab->lblRpCache);
	gtk_label_set_wrap(GTK_LABEL(tab->lblRpCache), TRUE);

	tab->btnRpCache  = gtk_button_new_with_label(reinterpret_cast<const char*>(
		C_("CacheTab", "Clear the ROM Properties Page Download Cache")));
	gtk_widget_set_name(tab->btnRpCache, "btnRpCache");

	tab->lblStatus = gtk_label_new(nullptr);
	gtk_widget_set_name(tab->lblStatus, "lblStatus");
	GTK_LABEL_XALIGN_LEFT(tab->lblStatus);
	tab->pbStatus = gtk_progress_bar_new();
	gtk_widget_set_name(tab->pbStatus, "pbStatus");
#if GTK_CHECK_VERSION(3,0,0)
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(tab->pbStatus), TRUE);
#endif /* !GTK_CHECK_VERSION(3,0,0) */

	// Connect the signal handlers for the buttons.
	g_signal_connect(tab->btnSysCache, "clicked", G_CALLBACK(cache_tab_on_btnSysCache_clicked), tab);
	g_signal_connect(tab->btnRpCache,  "clicked", G_CALLBACK(cache_tab_on_btnRpCache_clicked),  tab);

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
	gtk_box_pack_start(GTK_BOX(tab), tab->lblSysCache, false, false, 0);
	gtk_box_pack_start(GTK_BOX(tab), tab->btnSysCache, false, false, 0);
	gtk_box_pack_start(GTK_BOX(tab), tab->lblRpCache, false, false, 0);
	gtk_box_pack_start(GTK_BOX(tab), tab->btnRpCache, false, false, 0);

	// TODO: Spacer and/or alignment?
	gtk_box_pack_end(GTK_BOX(tab), tab->pbStatus, false, false, 0);
	gtk_box_pack_end(GTK_BOX(tab), tab->lblStatus, false, false, 0);

	// NOTE: GTK4 defaults to visible; GTK2 and GTK3 defaults to invisible.
	// Hiding unconditionally just in case.
	gtk_widget_hide(tab->lblStatus);
	gtk_widget_hide(tab->pbStatus);
	gtk_widget_set_no_show_all(tab->lblStatus, TRUE);
	gtk_widget_set_no_show_all(tab->pbStatus, TRUE);

	gtk_widget_show(tab->lblSysCache);
	gtk_widget_show(tab->btnSysCache);
	gtk_widget_show(tab->lblRpCache);
	gtk_widget_show(tab->btnRpCache);
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
	CacheTab *const tab = CACHE_TAB(object);

	if (tab->ccCleaner) {
		g_object_unref(tab->ccCleaner);
	}

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
	g_return_val_if_fail(IS_CACHE_TAB(tab), FALSE);
	return FALSE;
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
	gtk_widget_set_cursor_from_name(widget, (enable ? nullptr : "wait"));
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GdkWindow *const gdkWindow = gtk_widget_get_window(widget);
	if (gdkWindow) {
		if (enable) {
			// Regular cursor.
			gdk_window_set_cursor(gdkWindow, nullptr);
		} else {
			// Busy cursor.
			GdkCursor *const curBusy = gdk_cursor_new_from_name(gtk_widget_get_display(widget), "wait");
			gdk_window_set_cursor(gdkWindow, curBusy);
#if GTK_CHECK_VERSION(3,0,0)
			g_object_unref(curBusy);
#else /* !GTK_CHECK_VERSION(3,0,0) */
			gdk_cursor_unref(curBusy);
#endif /* GTK_CHECK_VERSION(3,0,0) */
		}
	}
#endif /* GTK_CHECK_VERSION(4,0,0) */
}

static inline
void gtk_progress_bar_set_error(GtkProgressBar *pb, gboolean error)
{
#if GTK_CHECK_VERSION(3,0,0)
	// If error, add our CSS class.
	// Otherwise, remove our CSS class.
	GtkStyleContext *const context = gtk_widget_get_style_context(GTK_WIDGET(pb));
	if (error) {
		gtk_style_context_add_class(context, "gsrp_pb_error");
	} else {
		gtk_style_context_remove_class(context, "gsrp_pb_error");
	}
#else /* !GTK_CHECK_VERSION(3,0,0) */
	// Reference: https://www.mail-archive.com/gtk-app-devel-list@gnome.org/msg20073.html
	// TODO: Probably not optimal, but GTK2 is obsolete, so meh.
	// NOTE: It causes the theming to break. I suspect the theming
	// is *why* the usual methods aren't working here.
	GtkStyle *const style = gtk_style_new();
	if (error) {
		gdk_color_parse("#901818", &style->bg[GTK_STATE_PRELIGHT]);
	} else {
		gdk_color_parse("#336E8D", &style->bg[GTK_STATE_PRELIGHT]);
	}
	gtk_widget_set_style(GTK_WIDGET(pb), style);
	g_object_unref(style);
#endif
}

/**
 * Clear the specified cache directory.
 * @param tab CacheTab
 * @param cache_dir Cache directory
 */
static void
cache_tab_clear_cache_dir(CacheTab *tab, RpCacheDir cache_dir)
{
	// Reset the progress bar.
	// FIXME: No "error" option in GtkProgressBar.
	gtk_progress_bar_set_error(GTK_PROGRESS_BAR(tab->pbStatus), FALSE);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(tab->pbStatus), 0.0);

	// Set the label text.
	const char8_t *s_label;
	switch (cache_dir) {
		default:
			assert(!"Invalid cache directory specified.");
			ccCleaner_error(tab->ccCleaner,
				reinterpret_cast<const char*>(C_("CacheTab", "Invalid cache directory specified.")), tab);
			return;
		case RP_CD_System:
			s_label = C_("CacheTab", "Clearing the system thumbnail cache...");
			break;
		case RP_CD_RomProperties:
			s_label = C_("CacheTab", "Clearing the ROM Properties Page cache...");
			break;
	}
	gtk_label_set_text(GTK_LABEL(tab->lblStatus), reinterpret_cast<const char*>(s_label));

#if !GTK_CHECK_VERSION(3,0,0)
	// GTK2: Clear the styling immediately to prevent shenanigans.
	gtk_progress_bar_set_error(GTK_PROGRESS_BAR(tab->pbStatus), FALSE);
#endif /* !GTK_CHECK_VERSION(3,0,0) */

	// Show the progress controls.
	gtk_widget_show(tab->lblStatus);
	gtk_widget_show(tab->pbStatus);

	// Disable the buttons until we're done.
	cache_tab_enable_ui_controls(tab, FALSE);

	// Create the CacheCleaner if necessary.
	if (!tab->ccCleaner) {
		tab->ccCleaner = cache_cleaner_new(cache_dir);
		g_object_ref_sink(tab->ccCleaner);

		// Connect the signal handlers.
		g_signal_connect(tab->ccCleaner, "progress",       G_CALLBACK(ccCleaner_progress),     tab);
		g_signal_connect(tab->ccCleaner, "error",          G_CALLBACK(ccCleaner_error),        tab);
		g_signal_connect(tab->ccCleaner, "cache-is-empty", G_CALLBACK(ccCleaner_cacheIsEmpty), tab);
		g_signal_connect(tab->ccCleaner, "cache-cleared",  G_CALLBACK(ccCleaner_cacheCleared), tab);
		g_signal_connect(tab->ccCleaner, "finished",       G_CALLBACK(ccCleaner_finished),     tab);
	}

	// Set the cache directory.
	cache_cleaner_set_cache_dir(tab->ccCleaner, cache_dir);

	// Run the CacheCleaner object.
	// NOTE: Sending signals from a GObject to a GtkWidget
	// and updating the UI can cause the program to crash.
	// Instead, we'll just run gtk_main_iteration() within
	// cache_cleaner_run().
	// Everything else works just like the KDE version.
	cache_cleaner_run(tab->ccCleaner);
}

static inline void
gtk_process_main_event_loop(void)
{
	// FIXME: This causes flickering...
#if GTK_CHECK_VERSION(4,0,0)
	while (g_main_context_pending(nullptr)) {
		g_main_context_iteration(nullptr, TRUE);
	}
#else /* !GTK_CHECK_VERSION(4,0,0) */
	while (gtk_events_pending()) {
		gtk_main_iteration();
	}
#endif /* GTK_CHECK_VERSION(4,0,0) */
}

/** CacheCleaner signal handlers **/

/**
 * Cache cleaning task progress update.
 * @param cleaner CacheCleaner
 * @param pg_cur Current progress
 * @param pg_max Maximum progress
 * @param hasError If true, errors have occurred
 * @param tab CacheTab
 */
static void
ccCleaner_progress(CacheCleaner *cleaner, int pg_cur, int pg_max, gboolean hasError, CacheTab *tab)
{
	g_return_if_fail(IS_CACHE_TAB(tab));
	RP_UNUSED(cleaner);

	// GtkProgressBar uses a gdouble fraction in the range [0.0,1.0].
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(tab->pbStatus),
		(gdouble)pg_cur / (gdouble)pg_max);
	gtk_progress_bar_set_error(GTK_PROGRESS_BAR(tab->pbStatus), hasError);
	gtk_process_main_event_loop();
}

/**
 * An error occurred while clearing the cache.
 * @param cleaner CacheCleaner
 * @param error Error description
 * @param tab CacheTab
 */
static void
ccCleaner_error(CacheCleaner *cleaner, const char *error, CacheTab *tab)
{
	g_return_if_fail(IS_CACHE_TAB(tab));
	RP_UNUSED(cleaner);

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(tab->pbStatus), 1.0);
	gtk_progress_bar_set_error(GTK_PROGRESS_BAR(tab->pbStatus), TRUE);

	// FIXME: U8STRFIX - rp_sprintf()
	const string s_msg = rp_sprintf(reinterpret_cast<const char*>(
		C_("CacheTab", "<b>ERROR:</b> %s")), error);
	gtk_label_set_markup(GTK_LABEL(tab->lblStatus), s_msg.c_str());
	// FIXME: Causes crashes...
	//MessageSound::play(GTK_MESSAGE_WARNING, s_msg.c_str(), GTK_WIDGET(tab));
	gtk_process_main_event_loop();
}

/**
 * Cache directory is empty.
 * @param cleaner CacheCleaner
 * @param cache_dir Which cache directory was checked
 * @param tab CacheTab
 */
static void
ccCleaner_cacheIsEmpty(CacheCleaner *cleaner, RpCacheDir cache_dir, CacheTab *tab)
{
	g_return_if_fail(IS_CACHE_TAB(tab));
	RP_UNUSED(cleaner);

	const char8_t *s_msg;
	switch (cache_dir) {
		default:
			assert(!"Invalid cache directory specified.");
			s_msg = C_("CacheTab", "Invalid cache directory specified.");
			break;
		case RP_CD_System:
			s_msg = C_("CacheTab", "System thumbnail cache is empty. Nothing to do.");
			break;
		case RP_CD_RomProperties:
			s_msg = C_("CacheTab", "rom-properties cache is empty. Nothing to do.");
			break;
	}

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(tab->pbStatus), 1.0);
	gtk_label_set_text(GTK_LABEL(tab->lblStatus), reinterpret_cast<const char*>(s_msg));
	// FIXME: Causes crashes...
	//MessageSound::play(GTK_MESSAGE_WARNING, s_msg, GTK_WIDGET(tab));
	gtk_process_main_event_loop();
}

/**
 * Cache was cleared.
 * @param cleaner CacheCleaner
 * @param cache_dir Which cache dir was cleared
 * @param dirErrs Number of directories that could not be deleted
 * @param fileErrs Number of files that could not be deleted
 * @param tab CacheTab
 */
static void
ccCleaner_cacheCleared(CacheCleaner *cleaner, RpCacheDir cache_dir, unsigned int dirErrs, unsigned int fileErrs, CacheTab *tab)
{
	RP_UNUSED(cleaner);

	if (dirErrs > 0 || fileErrs > 0) {
		// FIXME: U8STRFIX - rp_sprintf()
		const string s_msg = rp_sprintf(
			reinterpret_cast<const char*>(C_("CacheTab", "<b>ERROR:</b> %s")),
			rp_sprintf_p(
				reinterpret_cast<const char*>(C_("CacheTab", "Unable to delete %1$u file(s) and/or %2$u dir(s).")),
				fileErrs, dirErrs).c_str());
		gtk_label_set_markup(GTK_LABEL(tab->lblStatus), s_msg.c_str());
		// FIXME: Causes crashes...
		//MessageSound::play(GTK_MESSAGE_WARNING, s_msg.c_str(), GTK_WIDGET(tab));
		return;
	}

	const char8_t *s_msg;
	GtkMessageType icon;
	switch (cache_dir) {
		default:
			assert(!"Invalid cache directory specified.");
			s_msg = C_("CacheTab", "Invalid cache directory specified.");
			icon = GTK_MESSAGE_WARNING;
			break;
		case RP_CD_System:
			s_msg = C_("CacheTab", "System thumbnail cache cleared successfully.");
			icon = GTK_MESSAGE_INFO;
			break;
		case RP_CD_RomProperties:
			s_msg = C_("CacheTab", "rom-properties cache cleared successfully.");
			icon = GTK_MESSAGE_INFO;
			break;
	}

	gtk_label_set_text(GTK_LABEL(tab->lblStatus), reinterpret_cast<const char*>(s_msg));
	// FIXME: Causes crashes...
	//MessageSound::play(icon, s_msg, GTK_WIDGET(tab));
	RP_UNUSED(icon);
	gtk_process_main_event_loop();
}

/**
 * Cache cleaning task has completed.
 * This is called when run() exits, regardless of status.
 * @param cleaner CacheCleaner
 * @param tab CacheTab
 */
static void
ccCleaner_finished(CacheCleaner *cleaner, CacheTab *tab)
{
	RP_UNUSED(cleaner);
	cache_tab_enable_ui_controls(tab, TRUE);
}

/** Widget signal handlers **/

/**
 * "Clear the System Thumbnail Cache" button was clicked.
 */
static void
cache_tab_on_btnSysCache_clicked(GtkButton *button, CacheTab *tab)
{
	RP_UNUSED(button);
	cache_tab_clear_cache_dir(tab, RP_CD_System);
}

/**
 * "Clear the ROM Properties Page Download Cache" button was clicked.
 */
static void
cache_tab_on_btnRpCache_clicked(GtkButton *button, CacheTab *tab)
{
	RP_UNUSED(button);
	cache_tab_clear_cache_dir(tab, RP_CD_RomProperties);
}
