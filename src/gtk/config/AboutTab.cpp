/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * AboutTab.cpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "AboutTab.hpp"
#include "RpConfigTab.h"

#include "RpGtk.hpp"
#include "gtk-compat.h"

// librpbase
#include "librpbase/config/AboutTabText.hpp"
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

// AboutTab class
struct _AboutTabClass {
	superclass __parent__;
};

// AboutTab instance
struct _AboutTab {
	super __parent__;

	GtkWidget	*imgLogo;
	GtkWidget	*lblTitle;

	GtkWidget	*lblCredits;
	GtkWidget	*lblLibraries;
	GtkWidget	*lblSupport;
};

// Interface initialization
static void	about_tab_rp_config_tab_interface_init	(RpConfigTabInterface *iface);
static gboolean	about_tab_has_defaults			(AboutTab	*tab);
static void	about_tab_reset				(AboutTab	*tab);
static void	about_tab_load_defaults			(AboutTab	*tab);
static void	about_tab_save				(AboutTab	*tab,
							 GKeyFile       *keyFile);

// Label initialization
static void	about_tab_init_program_title_text	(GtkImage	*imgLogo,
							 GtkLabel	*lblTitle);
static void	about_tab_init_credits_tab		(GtkLabel	*lblCredits);

// NOTE: Pango doesn't recognize "&nbsp;". Use U+00A0 instead.
static const char sIndent[] = "\xC2\xA0\xC2\xA0\xC2\xA0\xC2\xA0\xC2\xA0\xC2\xA0\xC2\xA0\xC2\xA0";
static const char chrBullet[] = "\xE2\x80\xA2";	// U+2022: BULLET

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(AboutTab, about_tab,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0),
		G_IMPLEMENT_INTERFACE(RP_CONFIG_TYPE_TAB,
			about_tab_rp_config_tab_interface_init));

static void
about_tab_class_init(AboutTabClass *klass)
{ }

static void
about_tab_rp_config_tab_interface_init(RpConfigTabInterface *iface)
{
	iface->has_defaults = (__typeof__(iface->has_defaults))about_tab_has_defaults;
	iface->reset = (__typeof__(iface->reset))about_tab_reset;
	iface->load_defaults = (__typeof__(iface->load_defaults))about_tab_load_defaults;
	iface->save = (__typeof__(iface->save))about_tab_save;
}

static void
about_tab_init(AboutTab *tab)
{
#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(tab), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3,0,0) */
	gtk_box_set_spacing(GTK_BOX(tab), 8);

	// HBox for the logo and title.
	GtkWidget *const hboxTitle = RP_gtk_hbox_new(6);
	GTK_WIDGET_HALIGN_CENTER(hboxTitle);
	// Logo and title labels. (Will be filled in later.)
	tab->imgLogo = gtk_image_new();
	tab->lblTitle = gtk_label_new(nullptr);
	GTK_WIDGET_HALIGN_CENTER(tab->imgLogo);
	GTK_WIDGET_HALIGN_CENTER(tab->lblTitle);
	gtk_label_set_justify(GTK_LABEL(tab->lblTitle), GTK_JUSTIFY_CENTER);

	// Create the GtkNotebook for the three tabs.
	GtkWidget *const tabWidget = gtk_notebook_new();
	//gtk_widget_set_margin_bottom(tabWidget, 8);

	// Each tab contains a scroll area and a label.

	// Credits tab: Scroll area
#if GTK_CHECK_VERSION(4,0,0)
	GtkWidget *const scrlCredits = gtk_scrolled_window_new();
	gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrlCredits), true);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GtkWidget *const scrlCredits = gtk_scrolled_window_new(nullptr, nullptr);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrlCredits), GTK_SHADOW_IN);
#endif /* GTK_CHECK_VERSION */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrlCredits),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	// Credits tab: Label
	tab->lblCredits = gtk_label_new(nullptr);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrlCredits), tab->lblCredits);
	GTK_WIDGET_HALIGN_LEFT(tab->lblCredits);
	GTK_WIDGET_VALIGN_TOP(tab->lblCredits);
	gtk_widget_set_margin(tab->lblCredits, 8);

	// Libraries tab: Scroll area
#if GTK_CHECK_VERSION(4,0,0)
	GtkWidet *const scrlLibraries = gtk_scrolled_window_new();
	gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrlLibraries), true);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GtkWidget *scrlLibraries = gtk_scrolled_window_new(nullptr, nullptr);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrlLibraries), GTK_SHADOW_IN);
#endif /* GTK_CHECK_VERSION */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrlLibraries),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	// Libraries tab: Label
	tab->lblLibraries = gtk_label_new(nullptr);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrlLibraries), tab->lblLibraries);
	GTK_WIDGET_HALIGN_LEFT(tab->lblLibraries);
	GTK_WIDGET_VALIGN_TOP(tab->lblLibraries);
	gtk_widget_set_margin(tab->lblLibraries, 8);

	// Support tab: Scroll area
#if GTK_CHECK_VERSION(4,0,0)
	GtkWidet *const scrlSupport = gtk_scrolled_window_new();
	gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrlSupport), true);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GtkWidget *scrlSupport = gtk_scrolled_window_new(nullptr, nullptr);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrlSupport), GTK_SHADOW_IN);
#endif /* GTK_CHECK_VERSION */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrlSupport),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	// Support tab: Label
	tab->lblSupport = gtk_label_new(nullptr);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrlSupport), tab->lblSupport);
	GTK_WIDGET_HALIGN_LEFT(tab->lblSupport);
	GTK_WIDGET_VALIGN_TOP(tab->lblSupport);
	gtk_widget_set_margin(tab->lblSupport, 8);

	// Create the tabs.
	GtkWidget *lblTab = gtk_label_new_with_mnemonic(
		convert_accel_to_gtk(C_("AboutTab", "C&redits")).c_str());
	gtk_notebook_append_page(GTK_NOTEBOOK(tabWidget), scrlCredits, lblTab);
	lblTab = gtk_label_new_with_mnemonic(
		convert_accel_to_gtk(C_("AboutTab", "&Libraries")).c_str());
	gtk_notebook_append_page(GTK_NOTEBOOK(tabWidget), scrlLibraries, lblTab);
	lblTab = gtk_label_new_with_mnemonic(
		convert_accel_to_gtk(C_("AboutTab", "&Support")).c_str());
	gtk_notebook_append_page(GTK_NOTEBOOK(tabWidget), scrlSupport, lblTab);

#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(hboxTitle), tab->imgLogo);
	gtk_box_append(GTK_BOX(hboxTitle), tab->lblTitle);

	gtk_box_append(GTK_BOX(tab), hboxTitle);
	gtk_box_append(GTK_BOX(tab), tabWidget);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_widget_show(hboxTitle);
	gtk_widget_show(tab->imgLogo);
	gtk_widget_show(tab->lblTitle);
	gtk_widget_show(tabWidget);
	gtk_widget_show(scrlCredits);
	gtk_widget_show(tab->lblCredits);
	gtk_widget_show(scrlLibraries);
	gtk_widget_show(tab->lblLibraries);
	gtk_widget_show(scrlSupport);
	gtk_widget_show(tab->lblSupport);

	gtk_box_pack_start(GTK_BOX(hboxTitle), tab->imgLogo, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hboxTitle), tab->lblTitle, false, false, 0);

	gtk_box_pack_start(GTK_BOX(tab), hboxTitle, false, false, 0);
	gtk_box_pack_start(GTK_BOX(tab), tabWidget, true, true, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Initialize the various text fields.
	about_tab_init_program_title_text(GTK_IMAGE(tab->imgLogo), GTK_LABEL(tab->lblTitle));
	about_tab_init_credits_tab(GTK_LABEL(tab->lblCredits));
}

GtkWidget*
about_tab_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(TYPE_ABOUT_TAB, nullptr));
}

/** RpConfigTab interface functions **/

static gboolean
about_tab_has_defaults(AboutTab *tab)
{
	g_return_val_if_fail(IS_ABOUT_TAB(tab), false);
	return false;
}

static void
about_tab_reset(AboutTab *tab)
{
	g_return_if_fail(IS_ABOUT_TAB(tab));
	// TODO
}

static void
about_tab_load_defaults(AboutTab *tab)
{
	g_return_if_fail(IS_ABOUT_TAB(tab));

	// Not implemented.
	return;
}

static void
about_tab_save(AboutTab *tab, GKeyFile *keyFile)
{
	g_return_if_fail(IS_ABOUT_TAB(tab));
	g_return_if_fail(keyFile != nullptr);

	// Not implemented.
	return;
}

/** Label initialization **/

/**
 * Initialize the program title text.
 * @param imgLogo
 * @param lblTitle
 */
static void
about_tab_init_program_title_text(GtkImage *imgLogo, GtkLabel *lblTitle)
{
	// Program icon.
	// TODO: Make a custom icon instead of reusing the system icon.

	// Get the 128x128 icon.
	// TODO: Determine the best size.
	GdkPixbuf *const icon = gtk_icon_theme_load_icon(
		gtk_icon_theme_get_default(), "media-flash", 128,
		(GtkIconLookupFlags)0, nullptr);
	if (icon) {
		gtk_image_set_from_pixbuf(imgLogo, icon);
		g_object_unref(icon);
	} else {
		gtk_image_clear(imgLogo);
	}

	string sPrgTitle;
	sPrgTitle.reserve(1024);
	// tr: Uses Pango's HTML subset for formatting.
	sPrgTitle += C_("AboutTab", "<b>ROM Properties Page</b>\nShell Extension");
	sPrgTitle += "\n\n";
	sPrgTitle += rp_sprintf(C_("AboutTab", "Version %s"),
		AboutTabText::prg_version);
	if (AboutTabText::git_version[0] != 0) {
		sPrgTitle += '\n';
		sPrgTitle += AboutTabText::git_version;
		if (AboutTabText::git_describe[0] != 0) {
			sPrgTitle += '\n';
			sPrgTitle += AboutTabText::git_describe;
		}
	}

	gtk_label_set_markup(lblTitle, sPrgTitle.c_str());
}

/**
 * Initialize the "Credits" tab.
 * @param lblCredits
 */
static void
about_tab_init_credits_tab(GtkLabel *lblCredits)
{
	// License name, with HTML formatting.
	const string sPrgLicense = rp_sprintf("<a href='https://www.gnu.org/licenses/gpl-2.0.html'>%s</a>",
		C_("AboutTabl|Credits", "GNU GPL v2"));

	// lblCredits is RichText.
	string sCredits;
	sCredits.reserve(4096);
	// NOTE: Copyright is NOT localized.
	sCredits += "Copyright (c) 2016-2022 by David Korth.\n";
	sCredits += rp_sprintf(
		// tr: %s is the name of the license.
		C_("AboutTab|Credits", "This program is licensed under the %s or later."),
			sPrgLicense.c_str());

	AboutTabText::CreditType lastCreditType = AboutTabText::CreditType::Continue;
	for (const AboutTabText::CreditsData_t *creditsData = &AboutTabText::CreditsData[0];
	     creditsData->type < AboutTabText::CreditType::Max; creditsData++)
	{
		if (creditsData->type != AboutTabText::CreditType::Continue &&
		    creditsData->type != lastCreditType)
		{
			// New credit type.
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
		sCredits += '\n';
		sCredits += sIndent;
		sCredits += chrBullet;
		sCredits += ' ';
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
			// tr: Sub-credit.
			sCredits += rp_sprintf(C_("AboutTab|Credits", " (%s)"),
				creditsData->sub);
		}

		lastCreditType = creditsData->type;
	}

	// We're done building the string.
	gtk_label_set_markup(lblCredits, sCredits.c_str());
}
