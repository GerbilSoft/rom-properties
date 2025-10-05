/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * AboutTab.cpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"
#include "config.gtk.h"

#include "AboutTab.hpp"
#include "RpConfigTab.h"
#ifdef ENABLE_NETWORKING
#  include "UpdateChecker.hpp"
#endif /* ENABLE_NETWORKING */

#include "gtk-compat.h"
#include "RpGtk.h"

// Other rom-properties libraries
#include "librpbase/config/AboutTabText.hpp"
using namespace LibRpBase;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;

/** Libraries **/

// zlib and libpng
#include <zlib.h>
#include <png.h>
#include "librpbase/img/RpPng.hpp"
// TODO: JPEG
#if defined(ENABLE_DECRYPTION) && defined(HAVE_NETTLE)
#  include "librpbase/crypto/AesNettle.hpp"
#endif /* ENABLE_DECRYPTION && HAVE_NETTLE */
#ifdef ENABLE_XML
#  include <pugixml.hpp>
#endif /* ENABLE_XML */

#if GTK_CHECK_VERSION(3, 0, 0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_BOX
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

#if defined(ENABLE_NETWORKING) && GTK_CHECK_VERSION(3, 1, 6)
// NOTE: Update check requires GtkOverlay, which was
// added in GTK+ 3.2.0 (3.1.6).
#  define ENABLE_UPDATE_CHECK 1
#endif /* defined(ENABLE_NETWORKING) && GTK_CHECK_VERSION(3, 1, 6) */

// RpAboutTab class
struct _RpAboutTabClass {
	superclass __parent__;
};

// RpAboutTab instance
struct _RpAboutTab {
	super __parent__;

	GtkWidget	*imgLogo;	// GtkImage (GTK2/GTK3); GtkPicture (GTK4)
	GtkWidget	*lblTitle;

	GtkWidget	*lblCredits;
	GtkWidget	*lblLibraries;
	GtkWidget	*lblSupport;

#ifdef ENABLE_UPDATE_CHECK
	GtkWidget	*lblUpdateCheck;
	RpUpdateChecker	*updChecker;
	gboolean	checkedForUpdates;
#endif /* ENABLE_UPDATE_CHECK */
};

static void	rp_about_tab_dispose			(GObject	*object);

// Interface initialization
static void	rp_about_tab_rp_config_tab_interface_init	(RpConfigTabInterface *iface);
static gboolean	rp_about_tab_has_defaults			(RpAboutTab	*tab);
static void	rp_about_tab_reset				(RpAboutTab	*tab);
static void	rp_about_tab_save				(RpAboutTab	*tab,
								 GKeyFile       *keyFile);

// Label initialization
static void	rp_about_tab_init_program_title_text	(GtkWidget	*imgLogo,
							 GtkLabel	*lblTitle);
static void	rp_about_tab_init_credits_tab		(GtkLabel	*lblCredits);
static void	rp_about_tab_init_libraries_tab		(GtkLabel	*lblLibraries);
static void	rp_about_tab_init_support_tab		(GtkLabel	*lblSupport);

// Signal handlers
#ifdef ENABLE_UPDATE_CHECK
static void	rp_about_tab_realize_event		(GtkWidget		*self,
							 gpointer		 user_data);
static void	updChecker_error			(RpUpdateChecker	*updChecker,
							 const gchar		*error,
							 RpAboutTab		*tab);
static void	updChecker_retrieved			(RpUpdateChecker	*updChecker,
							 uint64_t		 updateVersion,
							 RpAboutTab		*tab);
#endif /* ENABLE_UPDATE_CHECK */

// NOTE: Pango doesn't recognize "&nbsp;". Use U+00A0 instead.
#define INDENT "\xC2\xA0\xC2\xA0\xC2\xA0\xC2\xA0\xC2\xA0\xC2\xA0\xC2\xA0\xC2\xA0"
#define BULLET "\xE2\x80\xA2"	/* U+2022: BULLET */

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpAboutTab, rp_about_tab,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0),
		G_IMPLEMENT_INTERFACE(RP_TYPE_CONFIG_TAB,
			rp_about_tab_rp_config_tab_interface_init));

static void
rp_about_tab_class_init(RpAboutTabClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = rp_about_tab_dispose;
}

static void
rp_about_tab_rp_config_tab_interface_init(RpConfigTabInterface *iface)
{
	iface->has_defaults = (__typeof__(iface->has_defaults))rp_about_tab_has_defaults;
	iface->reset = (__typeof__(iface->reset))rp_about_tab_reset;
	iface->load_defaults = nullptr;
	iface->save = (__typeof__(iface->save))rp_about_tab_save;
}

static void
rp_about_tab_init(RpAboutTab *tab)
{
#if GTK_CHECK_VERSION(3, 0, 0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(tab), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3, 0, 0) */
	gtk_box_set_spacing(GTK_BOX(tab), 8);

	// HBox for the logo and title.
	GtkWidget *const hboxTitle = rp_gtk_hbox_new(6);
	gtk_widget_set_name(hboxTitle, "hboxTitle");
	// Logo and title labels. (Will be filled in later.)
#if GTK_CHECK_VERSION(4, 0, 0)
	tab->imgLogo = gtk_picture_new();
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
	tab->imgLogo = gtk_image_new();
#endif /* GTK_CHECK_VERSION(4, 0, 0) */
	gtk_widget_set_name(tab->imgLogo, "imgLogo");
	tab->lblTitle = gtk_label_new(nullptr);
	gtk_widget_set_name(tab->lblTitle, "lblTitle");
	gtk_label_set_justify(GTK_LABEL(tab->lblTitle), GTK_JUSTIFY_CENTER);

#ifdef ENABLE_UPDATE_CHECK
	// FIXME: Figure out a good way to display lblUpdateCheck on GTK2.
	// For GTK+ 3.2 and later, we'll use GtkOverlay.
	// TODO: Qt has layout stretch factors; use that, or make Qt use overlays?
	tab->lblUpdateCheck = gtk_label_new(nullptr);
	gtk_widget_set_name(tab->lblUpdateCheck, "lblUpdateCheck");
	gtk_label_set_justify(GTK_LABEL(tab->lblUpdateCheck), GTK_JUSTIFY_RIGHT);
	GTK_WIDGET_HALIGN_RIGHT(tab->lblUpdateCheck);
	GTK_WIDGET_VALIGN_TOP(tab->lblUpdateCheck);

	GtkWidget *const ovlTitle = gtk_overlay_new();
	gtk_overlay_set_child(GTK_OVERLAY(ovlTitle), hboxTitle);
	gtk_overlay_add_overlay(GTK_OVERLAY(ovlTitle), tab->lblUpdateCheck);
#endif /* ENABLE_UPDATE_CHECK */

	GTK_WIDGET_HALIGN_CENTER(tab->imgLogo);
	GTK_WIDGET_HALIGN_CENTER(tab->lblTitle);

	// Create the GtkNotebook for the three tabs.
	GtkWidget *const tabWidget = gtk_notebook_new();
	gtk_widget_set_name(tabWidget, "tabWidget");
	//gtk_widget_set_margin_bottom(tabWidget, 8);
#if GTK_CHECK_VERSION(2, 91, 1)
	gtk_widget_set_halign(tabWidget, GTK_ALIGN_FILL);
	gtk_widget_set_valign(tabWidget, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand(tabWidget, TRUE);
	gtk_widget_set_vexpand(tabWidget, TRUE);
#endif /* GTK_CHECK_VERSION(2, 91, 1) */

	// Each tab contains a scroll area and a label.
	// FIXME: GtkScrolledWindow seems to start at the label contents,
	// ignoring the top margin...

	// Credits tab: Scroll area
#if GTK_CHECK_VERSION(4, 0, 0)
	GtkWidget *const scrlCredits = gtk_scrolled_window_new();
	gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrlCredits), true);
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
	GtkWidget *const scrlCredits = gtk_scrolled_window_new(nullptr, nullptr);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrlCredits), GTK_SHADOW_IN);
#endif /* GTK_CHECK_VERSION */
	gtk_widget_set_name(scrlCredits, "scrlCredits");
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrlCredits),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(2, 91, 1)
	gtk_widget_set_halign(scrlCredits, GTK_ALIGN_FILL);
	gtk_widget_set_valign(scrlCredits, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand(scrlCredits, TRUE);
	gtk_widget_set_vexpand(scrlCredits, TRUE);
#endif /* GTK_CHECK_VERSION(2, 91, 1) */

	// Credits tab: Label
	tab->lblCredits = gtk_label_new(nullptr);
	gtk_widget_set_name(tab->lblCredits, "lblCredits");
	GTK_WIDGET_HALIGN_LEFT(tab->lblCredits);
	GTK_WIDGET_VALIGN_TOP(tab->lblCredits);
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_set_margin(tab->lblCredits, 8);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrlCredits), tab->lblCredits);
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
	gtk_misc_set_padding(GTK_MISC(tab->lblCredits), 8, 8);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrlCredits), tab->lblCredits);
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

	// Libraries tab: Scroll area
#if GTK_CHECK_VERSION(4, 0, 0)
	GtkWidget *const scrlLibraries = gtk_scrolled_window_new();
	gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrlLibraries), true);
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
	GtkWidget *scrlLibraries = gtk_scrolled_window_new(nullptr, nullptr);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrlLibraries), GTK_SHADOW_IN);
#endif /* GTK_CHECK_VERSION */
	gtk_widget_set_name(scrlLibraries, "scrlLibraries");
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrlLibraries),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(2, 91, 1)
	gtk_widget_set_halign(scrlLibraries, GTK_ALIGN_FILL);
	gtk_widget_set_valign(scrlLibraries, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand(scrlLibraries, TRUE);
	gtk_widget_set_vexpand(scrlLibraries, TRUE);
#endif /* GTK_CHECK_VERSION(2, 91, 1) */

	// Libraries tab: Label
	tab->lblLibraries = gtk_label_new(nullptr);
	gtk_widget_set_name(tab->lblLibraries, "lblLibraries");
	GTK_WIDGET_HALIGN_LEFT(tab->lblLibraries);
	GTK_WIDGET_VALIGN_TOP(tab->lblLibraries);
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_set_margin(tab->lblLibraries, 8);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrlLibraries), tab->lblLibraries);
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
	gtk_misc_set_padding(GTK_MISC(tab->lblLibraries), 8, 8);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrlLibraries), tab->lblLibraries);
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

	// Support tab: Scroll area
#if GTK_CHECK_VERSION(4, 0, 0)
	GtkWidget *const scrlSupport = gtk_scrolled_window_new();
	gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrlSupport), true);
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
	GtkWidget *scrlSupport = gtk_scrolled_window_new(nullptr, nullptr);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrlSupport), GTK_SHADOW_IN);
#endif /* GTK_CHECK_VERSION */
	gtk_widget_set_name(scrlSupport, "scrlSupport");
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrlSupport),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(2, 91, 1)
	gtk_widget_set_halign(scrlSupport, GTK_ALIGN_FILL);
	gtk_widget_set_valign(scrlSupport, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand(scrlSupport, TRUE);
	gtk_widget_set_vexpand(scrlSupport, TRUE);
#endif /* GTK_CHECK_VERSION(2, 91, 1) */

	// Support tab: Label
	tab->lblSupport = gtk_label_new(nullptr);
	gtk_widget_set_name(tab->lblSupport, "lblSupport");
	GTK_WIDGET_HALIGN_LEFT(tab->lblSupport);
	GTK_WIDGET_VALIGN_TOP(tab->lblSupport);
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_set_margin(tab->lblSupport, 8);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrlSupport), tab->lblSupport);
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
	gtk_misc_set_padding(GTK_MISC(tab->lblSupport), 8, 8);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrlSupport), tab->lblSupport);
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

	// Create the tabs.
	GtkWidget *const lblTabCredits = rp_gtk_label_new_with_mnemonic(C_("AboutTab", "C&redits"));
	gtk_notebook_append_page(GTK_NOTEBOOK(tabWidget), scrlCredits, lblTabCredits);
	gtk_widget_set_name(lblTabCredits, "lblCreditsTab");
	GtkWidget *const lblTabLibraries = rp_gtk_label_new_with_mnemonic(C_("AboutTab", "&Libraries"));
	gtk_widget_set_name(lblTabLibraries, "lblLibrariesTab");
	gtk_notebook_append_page(GTK_NOTEBOOK(tabWidget), scrlLibraries, lblTabLibraries);
	GtkWidget *const lblTabSupport = rp_gtk_label_new_with_mnemonic(C_("AboutTab", "&Support"));
	gtk_widget_set_name(lblTabSupport, "lblSupportTab");
	gtk_notebook_append_page(GTK_NOTEBOOK(tabWidget), scrlSupport, lblTabSupport);

#if GTK_CHECK_VERSION(4, 0, 0)
	// GtkNotebook took a reference to the tab labels,
	// so we don't need to keep our references.
	g_object_unref(lblTabCredits);
	g_object_unref(lblTabLibraries);
	g_object_unref(lblTabSupport);
#endif /* GTK_CHECK_VERSION(4, 0, 0) */

#if GTK_CHECK_VERSION(4, 0, 0)
	GTK_WIDGET_HALIGN_CENTER(hboxTitle);
	gtk_box_append(GTK_BOX(hboxTitle), tab->imgLogo);
	gtk_box_append(GTK_BOX(hboxTitle), tab->lblTitle);

#  ifdef ENABLE_UPDATE_CHECK
	gtk_box_append(GTK_BOX(tab), ovlTitle);	// contains hboxTitle
#  else /* !ENABLE_UPDATE_CHECK */
	gtk_box_append(GTK_BOX(tab), hboxTitle);
#  endif /* ENABLE_UPDATE_CHECK */
	gtk_box_append(GTK_BOX(tab), tabWidget);
#else /* !GTK_CHECK_VERSION(4, 0, 0) */

	gtk_box_pack_start(GTK_BOX(hboxTitle), tab->imgLogo, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hboxTitle), tab->lblTitle, false, false, 0);

#  ifndef RP_USE_GTK_ALIGNMENT
	GTK_WIDGET_HALIGN_CENTER(hboxTitle);
#    ifdef ENABLE_UPDATE_CHECK
	gtk_box_pack_start(GTK_BOX(tab), ovlTitle, false, false, 0);	// contains hboxTitle
#    else /* !ENABLE_UPDATE_CHECK */
	gtk_box_pack_start(GTK_BOX(tab), hboxTitle, false, false, 0);
#    endif /* ENABLE_UPDATE_CHECK */
#  else /* RP_USE_GTK_ALIGNMENT */
	GtkWidget *const alignTitle = gtk_alignment_new(0.5f, 0.0f, 0.0f, 0.0f);
	gtk_widget_set_name(alignTitle, "alignTitle");
#    ifdef ENABLE_UPDATE_CHECK
	gtk_container_add(GTK_CONTAINER(alignTitle), ovlTitle);	// contains hboxTitle
#    else /* !ENABLE_UPDATE_CHECK */
	gtk_container_add(GTK_CONTAINER(alignTitle), hboxTitle);
#    endif /* ENABLE_UPDATE_CHECK */
	gtk_box_pack_start(GTK_BOX(tab), alignTitle, false, false, 0);
	gtk_widget_show(alignTitle);
#  endif /* RP_USE_GTK_ALIGNMENT */
	gtk_box_pack_start(GTK_BOX(tab), tabWidget, true, true, 0);

#  ifdef ENABLE_UPDATE_CHECK
	gtk_widget_show_all(ovlTitle);
#  else /* !ENABLE_UPDATE_CHECK */
	gtk_widget_show_all(hboxTitle);
#  endif /* ENABLE_UPDATE_CHECK */
	gtk_widget_show_all(tabWidget);
#endif /* GTK_CHECK_VERSION(4, 0, 0) */

	// Initialize the various text fields.
	rp_about_tab_init_program_title_text(tab->imgLogo, GTK_LABEL(tab->lblTitle));
	rp_about_tab_init_credits_tab(GTK_LABEL(tab->lblCredits));
	rp_about_tab_init_libraries_tab(GTK_LABEL(tab->lblLibraries));
	rp_about_tab_init_support_tab(GTK_LABEL(tab->lblSupport));

	// Connect signals
#ifdef ENABLE_UPDATE_CHECK
	g_signal_connect(tab, "realize", G_CALLBACK(rp_about_tab_realize_event), 0);
#endif /* ENABLE_UPDATE_CHECK */
}

static void
rp_about_tab_dispose(GObject *object)
{
	RpAboutTab *const tab = RP_ABOUT_TAB(object);
	RP_UNUSED(tab);	// maybe unused in some builds (XFCE/GTK2)...

#ifdef ENABLE_UPDATE_CHECK
	if (tab->updChecker) {
		g_clear_object(&tab->updChecker);
	}
#endif /* ENABLE_UPDATE_CHECK */

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_about_tab_parent_class)->dispose(object);
}

GtkWidget*
rp_about_tab_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(RP_TYPE_ABOUT_TAB, nullptr));
}

/** RpConfigTab interface functions **/

static gboolean
rp_about_tab_has_defaults(RpAboutTab *tab)
{
	g_return_val_if_fail(RP_IS_ABOUT_TAB(tab), FALSE);
	return FALSE;
}

static void
rp_about_tab_reset(RpAboutTab *tab)
{
	g_return_if_fail(RP_IS_ABOUT_TAB(tab));
	// TODO
}

static void
rp_about_tab_save(RpAboutTab *tab, GKeyFile *keyFile)
{
	// Not implemented.
	g_return_if_fail(RP_IS_ABOUT_TAB(tab));
	g_return_if_fail(keyFile != nullptr);
}

/** Label initialization **/

/**
 * Initialize the program title text.
 * @param imgLogo GtkImage (GTK2/GTK3); GtkPicture (GTK4)
 * @param lblTitle
 */
static void
rp_about_tab_init_program_title_text(GtkWidget *imgLogo, GtkLabel *lblTitle)
{
	// Program icon.
	// TODO: Make a custom icon instead of reusing the system icon.

	// Get the 128x128 icon.
	// TODO: Determine the best size.
	static constexpr int icon_size = 128;
#if GTK_CHECK_VERSION(4, 0, 0)
	// TODO: Get text direction from lblTitle instead of imgLogo?
	// FIXME: This is loading a 32x32 icon...
	GtkIconPaintable *const icon = gtk_icon_theme_lookup_icon(
		gtk_icon_theme_get_for_display(gtk_widget_get_display(GTK_WIDGET(imgLogo))),
		"media-flash", nullptr, icon_size, 1,
		gtk_widget_get_direction(GTK_WIDGET(imgLogo)), (GtkIconLookupFlags)0);

	if (icon) {
		gtk_picture_set_paintable(GTK_PICTURE(imgLogo), GDK_PAINTABLE(icon));
		g_object_unref(icon);
	} else {
		gtk_picture_set_paintable(GTK_PICTURE(imgLogo), nullptr);
	}
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
	GdkPixbuf *const icon = gtk_icon_theme_load_icon(
		gtk_icon_theme_get_default(), "media-flash", icon_size,
		(GtkIconLookupFlags)0, nullptr);
	if (icon) {
		gtk_image_set_from_pixbuf(GTK_IMAGE(imgLogo), icon);
		g_object_unref(icon);
	} else {
		gtk_image_clear(GTK_IMAGE(imgLogo));
	}
#endif /* GTK_CHECK_VERSION(4, 0, 0) */

	const char *const programVersion =
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::ProgramVersion);
	const char *const gitVersion =
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::GitVersion);

	string sPrgTitle;
	sPrgTitle.reserve(1024);
	// tr: Uses Pango's HTML subset for formatting.
	sPrgTitle += C_("AboutTab", "<b>ROM Properties Page</b>\nShell Extension");
	sPrgTitle += "\n\n";
	sPrgTitle += fmt::format(FRUN(C_("AboutTab", "Version {:s}")), programVersion);
	if (gitVersion) {
		sPrgTitle += '\n';
		sPrgTitle += gitVersion;
		const char *const gitDescription =
			AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::GitDescription);
		if (gitDescription) {
			sPrgTitle += '\n';
			sPrgTitle += gitDescription;
		}
	}

	gtk_label_set_markup(lblTitle, sPrgTitle.c_str());
}

/**
 * Initialize the "Credits" tab.
 * @param lblCredits
 */
static void
rp_about_tab_init_credits_tab(GtkLabel *lblCredits)
{
	// License name, with HTML formatting.
	const string sPrgLicense = fmt::format(FSTR("<a href='https://www.gnu.org/licenses/gpl-2.0.html'>{:s}</a>"),
		C_("AboutTabl|Credits", "GNU GPL v2"));

	// lblCredits is RichText.
	string sCredits;
	sCredits.reserve(4096);
	// NOTE: Copyright is NOT localized.
	sCredits += AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::Copyright);
	sCredits += '\n';
	sCredits += fmt::format(
		// tr: {:s} is the name of the license.
		FRUN(C_("AboutTab|Credits", "This program is licensed under the {:s} or later.")), sPrgLicense);

	AboutTabText::CreditType lastCreditType = AboutTabText::CreditType::Continue;
	for (const AboutTabText::CreditsData_t *creditsData = AboutTabText::getCreditsData();
	     creditsData->type < AboutTabText::CreditType::Max; creditsData++)
	{
		if (creditsData->type != AboutTabText::CreditType::Continue &&
		    creditsData->type != lastCreditType)
		{
			// New credit type
			sCredits += "\n\n<b>";

			switch (creditsData->type) {
				case AboutTabText::CreditType::Developer:
					sCredits += C_("AboutTab|Credits", "Developers:");
					break;
				case AboutTabText::CreditType::Contributor:
					sCredits += C_("AboutTab|Credits", "Contributors:");
					break;
				case AboutTabText::CreditType::Translator:
					sCredits += C_("AboutTab|Credits", "Translators:");
					break;

				case AboutTabText::CreditType::Continue:
				case AboutTabText::CreditType::Max:
				default:
					assert(!"Invalid credit type.");
					break;
			}

			sCredits += "</b>";
		}

		// Append the contributor's name.
		sCredits += "\n" INDENT BULLET " ";
		sCredits += creditsData->name;
		if (creditsData->url) {
			sCredits += " &lt;<a href='";
			sCredits += creditsData->url;
			sCredits += "'>";
			if (creditsData->linkText) {
				sCredits += creditsData->linkText;
			} else {
				sCredits += creditsData->url;
			}
			sCredits += "</a>&gt;";
		}
		if (creditsData->sub) {
			// tr: Sub-credit
			sCredits += fmt::format(FRUN(C_("AboutTab|Credits", " ({:s})")),
				creditsData->sub);
		}

		lastCreditType = creditsData->type;
	}

	// We're done building the string.
	gtk_label_set_markup(lblCredits, sCredits.c_str());
}

/**
 * Initialize the "Libraries" tab.
 * @param lblLibraries
 */
static void
rp_about_tab_init_libraries_tab(GtkLabel *lblLibraries)
{
	// lblLibraries is RichText.

	// NOTE: These strings can NOT be static.
	// Otherwise, they won't be retranslated if the UI language
	// is changed at runtime.

	// tr: Using an internal copy of a library.
	const char *const sIntCopyOf = C_("AboutTab|Libraries", "Internal copy of {:s}.");
	// tr: Compiled with a specific version of an external library.
	const char *const sCompiledWith = C_("AboutTab|Libraries", "Compiled with {:s}.");
	// tr: Using an external library, e.g. libpcre.so
	const char *const sUsingDll = C_("AboutTab|Libraries", "Using {:s}.");
	// tr: License: (libraries with only a single license)
	const char *const sLicense = C_("AboutTab|Libraries", "License: {:s}");
	// tr: Licenses: (libraries with multiple licenses)
	const char *const sLicenses = C_("AboutTab|Libraries", "Licenses: {:s}");

	// Suppress "unused variable" warnings.
	// sIntCopyOf isn't used if no internal copies of libraries are needed.
	RP_UNUSED(sIntCopyOf);
	RP_UNUSED(sCompiledWith);
	RP_UNUSED(sUsingDll);

	// Included libraries string.
	string sLibraries;
	sLibraries.reserve(8192);

	/** GTK+ **/
	const string gtkVersionCompiled = fmt::format(FSTR("GTK{:s} {:d}.{:d}.{:d}"),
		(GTK_MAJOR_VERSION >= 4 ? "" : "+"),
		(guint)GTK_MAJOR_VERSION, (guint)GTK_MINOR_VERSION,
		(guint)GTK_MICRO_VERSION);
	sLibraries += fmt::format(FRUN(sCompiledWith), gtkVersionCompiled) + '\n';

	// NOTE: Although the GTK+ 2.x headers export variables,
	// the shared libraries for 2.24.33 do *not* export them,
	// which results in undefined symbols at at runtime.
#if GTK_CHECK_VERSION(2, 90, 7)
	const guint gtk_major = gtk_get_major_version();
	const string gtkVersionUsing = fmt::format(FSTR("GTK{:s} {:d}.{:d}.{:d}"),
		(gtk_major >= 4 ? "" : "+"),
		gtk_major, gtk_get_minor_version(),
		gtk_get_micro_version());
	sLibraries += fmt::format(FRUN(sUsingDll), gtkVersionUsing);
#endif /* GTK_CHECK_VERSION(2, 90, 7) */
	sLibraries += "\n"
		"Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald.\n"
		"Copyright (C) 1995-2022 the GTK+ Team and others.\n"
		"<a href='https://www.gtk.org/'>https://www.gtk.org/</a>\n";
	sLibraries += fmt::format(FRUN(sLicenses), "GNU LGPL v2.1+");

	/** zlib **/
#ifdef HAVE_ZLIB
	sLibraries += "\n\n";
	const bool zlib_is_ng = RpPng::zlib_is_ng();
	string sZlibVersion = (zlib_is_ng ? "zlib-ng " : "zlib ");
	sZlibVersion += RpPng::zlib_version_string();

#if defined(USE_INTERNAL_ZLIB) && !defined(USE_INTERNAL_ZLIB_DLL)
	sLibraries += fmt::format(FRUN(sIntCopyOf), sZlibVersion);
#else
#  ifdef ZLIBNG_VERSION
	sLibraries += fmt::format(FRUN(sCompiledWith), "zlib-ng " ZLIBNG_VERSION);
#  else /* !ZLIBNG_VERSION */
	sLibraries += fmt::format(FRUN(sCompiledWith), "zlib " ZLIB_VERSION);
#  endif /* ZLIBNG_VERSION */
	sLibraries += '\n';
	sLibraries += fmt::format(FRUN(sUsingDll), sZlibVersion);
#endif
	sLibraries += "\n"
		"Copyright (C) 1995-2022 Jean-loup Gailly and Mark Adler.\n"
		"<a href='https://zlib.net/'>https://zlib.net/</a>\n";
#  ifdef ZLIBNG_VERSION
	// TODO: Also if zlibVersion() contains "zlib-ng"?
	sLibraries += "<a href='https://github.com/zlib-ng/zlib-ng'>https://github.com/zlib-ng/zlib-ng</a>\n";
#  endif /* ZLIBNG_VERSION */
	sLibraries += fmt::format(FRUN(sLicense), "zlib license");
#endif /* HAVE_ZLIB */

	/** libpng **/
#ifdef HAVE_PNG
	const bool APNG_is_supported = RpPng::libpng_has_APNG();
	const uint32_t png_version_number = RpPng::libpng_version_number();
	const string pngVersion = fmt::format(FSTR("libpng {:d}.{:d}.{:d}{:s}"),
		png_version_number / 10000,
		(png_version_number / 100) % 100,
		png_version_number % 100,
		(APNG_is_supported ? " + APNG" : " (No APNG support)"));

	sLibraries += "\n\n";
#if defined(USE_INTERNAL_PNG) && !defined(USE_INTERNAL_ZLIB_DLL)
	sLibraries += fmt::format(FRUN(sIntCopyOf), pngVersion);
#else
	// NOTE: Gentoo's libpng has "+apng" at the end of
	// PNG_LIBPNG_VER_STRING if APNG is enabled.
	// We have our own "+ APNG", so remove Gentoo's.
	string pngVersionCompiled = "libpng " PNG_LIBPNG_VER_STRING;
	for (size_t i = pngVersionCompiled.size()-1; i > 6; i--) {
		const char chr = pngVersionCompiled[i];
		if (isdigit_ascii(chr)) {
			break;
		}
		pngVersionCompiled.resize(i);
	}

	string fullPngVersionCompiled;
	if (APNG_is_supported) {
		// PNG version, with APNG support.
		fullPngVersionCompiled = fmt::format(FSTR("{:s} + APNG"), pngVersionCompiled);
	} else {
		// PNG version, without APNG support.
		fullPngVersionCompiled = fmt::format(FSTR("{:s} (No APNG support)"), pngVersionCompiled);
	}

	sLibraries += fmt::format(FRUN(sCompiledWith), fullPngVersionCompiled);
	sLibraries += '\n';
	sLibraries += fmt::format(FRUN(sUsingDll), pngVersion);
#endif

	sLibraries += RpPng::libpng_copyright_string();
	sLibraries += "<a href='http://www.libpng.org/pub/png/libpng.html'>http://www.libpng.org/pub/png/libpng.html</a>\n"
		"<a href='https://github.com/glennrp/libpng'>https://github.com/glennrp/libpng</a>\n";
	if (APNG_is_supported) {
		sLibraries += C_("AboutTab|Libraries", "APNG patch:");
		sLibraries += " <a href='https://sourceforge.net/projects/libpng-apng/'>https://sourceforge.net/projects/libpng-apng/</a>\n";
	}
	sLibraries += fmt::format(FRUN(sLicense), "libpng license");
#endif /* HAVE_PNG */

	/** nettle **/
#if defined(ENABLE_DECRYPTION) && defined(HAVE_NETTLE)
	sLibraries += "\n\n";
	int nettle_major, nettle_minor;
	int ret = AesNettle::get_nettle_compile_time_version(&nettle_major, &nettle_minor);
	if (ret == 0) {
		if (nettle_major >= 3) {
			sLibraries += fmt::format(FRUN(sCompiledWith),
				fmt::format(FSTR("GNU Nettle {:d}.{:d}"),
					nettle_major, nettle_minor));
		} else {
			sLibraries += fmt::format(FRUN(sCompiledWith), "GNU Nettle 2.x");
		}
		sLibraries += '\n';
	}

	ret = AesNettle::get_nettle_runtime_version(&nettle_major, &nettle_minor);
	if (ret == 0) {
		sLibraries += fmt::format(FRUN(sUsingDll),
			fmt::format(FSTR("GNU Nettle {:d}.{:d}"),
				nettle_major, nettle_minor));
		sLibraries += '\n';
	}

	if (nettle_major >= 3) {
		if (nettle_minor >= 1) {
			sLibraries += "Copyright (C) 2001-2025 Niels Möller.\n"
				"<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>\n";
		} else {
			sLibraries += "Copyright (C) 2001-2014 Niels Möller.\n"
				"<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>\n";
		}
		sLibraries += fmt::format(FRUN(sLicenses), "GNU LGPL v3+, GNU GPL v2+");
	} else {
		sLibraries +=
			"Copyright (C) 2001-2013 Niels Möller.\n"
			"<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>\n";
		sLibraries += fmt::format(FRUN(sLicense), "GNU LGPL v2.1+");
	}
#endif /* ENABLE_DECRYPTION && HAVE_NETTLE */

	/** PugiXML **/
#ifdef ENABLE_XML
	// PugiXML 1.10 and later uses this format: 1140 == 1.14.0
	// PugiXML 1.9 and earlier uses this format: 190 == 1.9.0
	unsigned int pugixml_major, pugixml_minor, pugixml_patch;
	if (PUGIXML_VERSION >= 1000) {
		pugixml_major = PUGIXML_VERSION / 1000;
		pugixml_minor = (PUGIXML_VERSION % 1000) / 10;
		pugixml_patch = PUGIXML_VERSION  % 10;
	} else {
		pugixml_major = PUGIXML_VERSION / 100;
		pugixml_minor = (PUGIXML_VERSION % 100) / 10;
		pugixml_patch = PUGIXML_VERSION % 10;
	}

	sLibraries += "\n\n";
	string pugiXmlVersion = fmt::format(FSTR("PugiXML {:d}.{:d}"),
		pugixml_major, pugixml_minor);
	if (pugixml_patch > 0) {
		pugiXmlVersion += fmt::format(FSTR(".{:d}"), pugixml_patch);
	}

#  if defined(USE_INTERNAL_XML) && !defined(USE_INTERNAL_XML_DLL)
	sLibraries += fmt::format(FRUN(sIntCopyOf), pugiXmlVersion);
#  else
	// FIXME: Runtime version?
	sLibraries += fmt::format(FRUN(sCompiledWith), pugiXmlVersion);
#  endif
	sLibraries += "\n"
		"Copyright (C) 2006-2025, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)\n"
		"<a href='https://pugixml.org/'>https://pugixml.org/</a>\n";
	sLibraries += fmt::format(FRUN(sLicense), "MIT license");
#endif /* ENABLE_XML */

	/** GNU gettext **/
	// NOTE: glibc's libintl.h doesn't have the version information,
	// so we're only printing this if we're using GNU gettext's version.
#if defined(HAVE_GETTEXT) && defined(LIBINTL_VERSION)
	string gettextVersion;
	if (LIBINTL_VERSION & 0xFF) {
		gettextVersion = fmt::format(FSTR("GNU gettext {:d}.{:d}.{:d}"),
			static_cast<unsigned int>( LIBINTL_VERSION >> 16),
			static_cast<unsigned int>((LIBINTL_VERSION >>  8) & 0xFF),
			static_cast<unsigned int>( LIBINTL_VERSION        & 0xFF));
	} else {
		gettextVersion = fmt::format(FSTR("GNU gettext {:d}.{:d}"),
			static_cast<unsigned int>( LIBINTL_VERSION >> 16),
			static_cast<unsigned int>((LIBINTL_VERSION >>  8) & 0xFF));
	}
	// FIXME: Runtime version?
	sLibraries += fmt::format(FRUN(sCompiledWith), gettextVersion);
	sLibraries += "\n"
		"Copyright (C) 1995-1997, 2000-2016, 2018-2020 Free Software Foundation, Inc.\n"
		"<a href='https://www.gnu.org/software/gettext/'>https://www.gnu.org/software/gettext/</a>\n";
	sLibraries += fmt::format(FRUN(sLicense), "GNU LGPL v2.1+");
#endif /* HAVE_GETTEXT && LIBINTL_VERSION */

	// We're done building the string.
	gtk_label_set_markup(lblLibraries, sLibraries.c_str());
}

/**
 * Initialize the "Support" tab.
 * @param lblSupport
 */
static void
rp_about_tab_init_support_tab(GtkLabel *lblSupport)
{
	string sSupport;
	sSupport.reserve(4096);
	sSupport = C_("AboutTab|Support",
		"For technical support, you can visit the following websites:");
	sSupport += '\n';

	for (const AboutTabText::SupportSite_t *supportSite = AboutTabText::getSupportSites();
	     supportSite->name != nullptr; supportSite++)
	{
		sSupport += INDENT BULLET " ";
		sSupport += supportSite->name;
		sSupport += " &lt;<a href='";
		sSupport += supportSite->url;
		sSupport += "'>";
		sSupport += supportSite->url;
		sSupport += "</a>&gt;\n";
	}

	// Email the author.
	sSupport += '\n';
	sSupport += C_("AboutTab|Support", "You can also email the developer directly:");
	sSupport += "\n" INDENT BULLET " "
		"David Korth "
		"&lt;<a href=\"mailto:gerbilsoft@gerbilsoft.com\">"
		"gerbilsoft@gerbilsoft.com</a>&gt;";

	// We're done building the string.
	gtk_label_set_markup(lblSupport, sSupport.c_str());
}

/** Signal handlers **/

#ifdef ENABLE_UPDATE_CHECK
static void
rp_about_tab_realize_event(GtkWidget	*self,
			   gpointer	 user_data)
{
	RP_UNUSED(user_data);

	RpAboutTab *const tab = RP_ABOUT_TAB(self);
	if (tab->checkedForUpdates) {
		// Already checked for updates.
		return;
	}

	tab->checkedForUpdates = TRUE;
	gtk_label_set_text(GTK_LABEL(tab->lblUpdateCheck), C_("AboutTab", "Checking for updates..."));

	// Run the update checker.
	if (!tab->updChecker) {
		tab->updChecker = rp_update_checker_new();
		g_signal_connect(tab->updChecker, "error", G_CALLBACK(updChecker_error), tab);
		g_signal_connect(tab->updChecker, "retrieved", G_CALLBACK(updChecker_retrieved), tab);
	}
	rp_update_checker_run(tab->updChecker);
}

static void
updChecker_error(RpUpdateChecker	*updChecker,
		 const gchar		*error,
		 RpAboutTab		*tab)
{
	RP_UNUSED(updChecker);

	gtk_label_set_markup(GTK_LABEL(tab->lblUpdateCheck),
		// tr: Error message template. (GTK version, with C++20 formatting)
		fmt::format(FRUN(C_("ConfigDialog", "<b>ERROR:</b> {:s}")), error).c_str());
}

static void
updChecker_retrieved(RpUpdateChecker	*updChecker,
		     uint64_t	 	 updateVersion,
		     RpAboutTab		*tab)
{
	RP_UNUSED(updChecker);

	// Our version. (ignoring the development flag)
	const uint64_t ourVersion = RP_PROGRAM_VERSION_NO_DEVEL(AboutTabText::getProgramVersion());

	// Format the latest version string.
	string sUpdVersion;
	const array<unsigned int, 3> upd = {{
		RP_PROGRAM_VERSION_MAJOR(updateVersion),
		RP_PROGRAM_VERSION_MINOR(updateVersion),
		RP_PROGRAM_VERSION_REVISION(updateVersion)
	}};

	if (upd[2] == 0) {
		sUpdVersion = fmt::format(FSTR("{:d}.{:d}"), upd[0], upd[1]);
	} else {
		sUpdVersion = fmt::format(FSTR("{:d}.{:d}.{:d}"), upd[0], upd[1], upd[2]);
	}

	string sVersionLabel;
	sVersionLabel.reserve(512);

	sVersionLabel = fmt::format(FRUN(C_("AboutTab", "Latest version: {:s}")), sUpdVersion);
	if (updateVersion > ourVersion) {
		sVersionLabel += "\n\n";
		sVersionLabel += C_("AboutTab", "<b>New version available!</b>");
		sVersionLabel += '\n';
		sVersionLabel += "<a href='https://github.com/GerbilSoft/rom-properties/releases'>";
		sVersionLabel += C_("AboutTab", "Download at GitHub");
		sVersionLabel += "</a>";
	}

	gtk_label_set_markup(GTK_LABEL(tab->lblUpdateCheck), sVersionLabel.c_str());
}
#endif /* ENABLE_UPDATE_CHECK */
