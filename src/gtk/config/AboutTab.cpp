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

// Libraries
#ifdef HAVE_ZLIB
#  include <zlib.h>
#endif
#ifdef HAVE_PNG
#  include "librpbase/img/APNG_dlopen.h"
#  include <png.h>
#endif
// TODO: JPEG
#if defined(ENABLE_DECRYPTION) && defined(HAVE_NETTLE_VERSION_H)
#  include <nettle/version.h>
#endif
#ifdef ENABLE_XML
#  include <tinyxml2.h>
#endif

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
static void	about_tab_init_libraries_tab		(GtkLabel	*lblLibraries);

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
	// FIXME: GtkScrolledWindow seems to start at the label contents,
	// ignoring the top margin...

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
	GtkWidget *const scrlLibraries = gtk_scrolled_window_new();
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
	GtkWidget *const scrlSupport = gtk_scrolled_window_new();
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
	about_tab_init_libraries_tab(GTK_LABEL(tab->lblLibraries));
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
#if GTK_CHECK_VERSION(4,0,0)
	// TODO: Get text direction from lblTitle instead of imgLogo?
	GtkIconPaintable *const icon = gtk_icon_theme_lookup_icon(
		gtk_icon_theme_get_for_display(gtk_widget_get_display(GTK_WIDGET(imgLogo))),
		"media-flash", nullptr, 128, 1,
		gtk_widget_get_direction(GTK_WIDGET(imgLogo)), (GtkIconLookupFlags)0);

	if (icon) {
		gtk_image_set_from_paintable(imgLogo, GDK_PAINTABLE(icon));
		g_object_unref(icon);
	} else {
		gtk_image_clear(imgLogo);
	}
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GdkPixbuf *const icon = gtk_icon_theme_load_icon(
		gtk_icon_theme_get_default(), "media-flash", 128,
		(GtkIconLookupFlags)0, nullptr);
	if (icon) {
		gtk_image_set_from_pixbuf(imgLogo, icon);
		g_object_unref(icon);
	} else {
		gtk_image_clear(imgLogo);
	}
#endif /* GTK_CHECK_VERSION(4,0,0) */

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

/**
 * Initialize the "Libraries" tab.
 * @param lblLibraries
 */
static void
about_tab_init_libraries_tab(GtkLabel *lblLibraries)
{
	// lblLibraries is RichText.
	char sVerBuf[64];

	// NOTE: These strings can NOT be static.
	// Otherwise, they won't be retranslated if the UI language
	// is changed at runtime.

	// tr: Using an internal copy of a library.
	const char *const sIntCopyOf = C_("AboutTab|Libraries", "Internal copy of %s.");
	// tr: Compiled with a specific version of an external library.
	const char *const sCompiledWith = C_("AboutTab|Libraries", "Compiled with %s.");
	// tr: Using an external library, e.g. libpcre.so
	const char *const sUsingDll = C_("AboutTab|Libraries", "Using %s.");
	// tr: License: (libraries with only a single license)
	const char *const sLicense = C_("AboutTab|Libraries", "License: %s");
	// tr: Licenses: (libraries with multiple licenses)
	const char *const sLicenses = C_("AboutTab|Libraries", "Licenses: %s");

	// Suppress "unused variable" warnings.
	// sIntCopyOf isn't used if no internal copies of libraries are needed.
	RP_UNUSED(sIntCopyOf);
	RP_UNUSED(sCompiledWith);
	RP_UNUSED(sUsingDll);

	// Included libraries string.
	string sLibraries;
	sLibraries.reserve(8192);

	/** GTK+ **/
	string gtkVersionCompiled = rp_sprintf("GTK%s %u.%u.%u",
		(GTK_MAJOR_VERSION >= 4 ? "" : "+"),
		(guint)GTK_MAJOR_VERSION, (guint)GTK_MINOR_VERSION,
		(guint)GTK_MICRO_VERSION);
#ifdef QT_IS_STATIC
	sLibraries += rp_sprintf(sIntCopyOf, gtkVersion.c_str());
#else
	const guint gtk_major = gtk_get_major_version();
	string gtkVersionUsing = rp_sprintf("GTK%s %u.%u.%u",
		(gtk_major >= 4 ? "" : "+"),
		gtk_major, gtk_get_minor_version(),
		gtk_get_minor_version());
	sLibraries += rp_sprintf(sCompiledWith, gtkVersionCompiled.c_str()) + '\n';
	sLibraries += rp_sprintf(sUsingDll, gtkVersionUsing.c_str());
#endif /* QT_IS_STATIC */
	sLibraries += '\n';
	sLibraries +=
		"Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald.\n"
		"Copyright (C) 1995-2022 the GTK+ Team and others.\n"
		"<a href='https://www.gtk.org/'>https://www.gtk.org/</a>\n";
	sLibraries += rp_sprintf(sLicenses, "GNU LGPL v2.1+");

	/** zlib **/
#ifdef HAVE_ZLIB
	sLibraries += "\n\n";
#  ifdef ZLIBNG_VERSION
	string sZlibVersion = "zlib-ng ";
	sZlibVersion += zlibngVersion();
#  else /* !ZLIBNG_VERSION */
	string sZlibVersion = "zlib ";
	sZlibVersion += zlibVersion();
#  endif /* ZLIBNG_VERSION */

#if defined(USE_INTERNAL_ZLIB) && !defined(USE_INTERNAL_ZLIB_DLL)
	sLibraries += rp_sprintf(sIntCopyOf, sZlibVersion.c_str());
#else
#  ifdef ZLIBNG_VERSION
	sLibraries += rp_sprintf(sCompiledWith, "zlib-ng " ZLIBNG_VERSION);
#  else /* !ZLIBNG_VERSION */
	sLibraries += rp_sprintf(sCompiledWith, "zlib " ZLIB_VERSION);
#  endif /* ZLIBNG_VERSION */
	sLibraries += '\n';
	sLibraries += rp_sprintf(sUsingDll, sZlibVersion.c_str());
#endif
	sLibraries += '\n';
	sLibraries +=
		"Copyright (C) 1995-2022 Jean-loup Gailly and Mark Adler.\n"
		"<a href='https://zlib.net/'>https://zlib.net/</a>\n";
#  ifdef ZLIBNG_VERSION
	// TODO: Also if zlibVersion() contains "zlib-ng"?
	sLibraries += "<a href='https://github.com/zlib-ng/zlib-ng'>https://github.com/zlib-ng/zlib-ng</a>\n";
#  endif /* ZLIBNG_VERSION */
	sLibraries += rp_sprintf(sLicense, "zlib license");
#endif /* HAVE_ZLIB */

	/** libpng **/
#ifdef HAVE_PNG
	// APNG suffix.
	const bool APNG_is_supported = (APNG_ref() == 0);
	if (APNG_is_supported) {
		// APNG is supported.
		// Unreference it to prevent leaks.
		APNG_unref();
	}

	const uint32_t png_version_number = png_access_version_number();
	char pngVersion[48];
	snprintf(pngVersion, sizeof(pngVersion), "libpng %u.%u.%u%s",
		png_version_number / 10000,
		(png_version_number / 100) % 100,
		png_version_number % 100,
		(APNG_is_supported ? " + APNG" : " (No APNG support)"));

	sLibraries += "\n\n";
#if defined(USE_INTERNAL_PNG) && !defined(USE_INTERNAL_ZLIB_DLL)
	sLibraries += rp_sprintf(sIntCopyOf, pngVersion);
#else
	// NOTE: Gentoo's libpng has "+apng" at the end of
	// PNG_LIBPNG_VER_STRING if APNG is enabled.
	// We have our own "+ APNG", so remove Gentoo's.
	string pngVersionCompiled = "libpng " PNG_LIBPNG_VER_STRING;
	while (!pngVersionCompiled.empty()) {
		size_t idx = pngVersionCompiled.size() - 1;
		char chr = pngVersionCompiled[idx];
		if (ISDIGIT(chr))
			break;
		pngVersionCompiled.resize(idx);
	}

	string fullPngVersionCompiled;
	if (APNG_is_supported) {
		// PNG version, with APNG support.
		fullPngVersionCompiled = rp_sprintf("%s + APNG", pngVersionCompiled.c_str());
	} else {
		// PNG version, without APNG support.
		fullPngVersionCompiled = rp_sprintf("%s (No APNG support)", pngVersionCompiled.c_str());
	}

	sLibraries += rp_sprintf(sCompiledWith, fullPngVersionCompiled.c_str());
	sLibraries += '\n';
	sLibraries += rp_sprintf(sUsingDll, pngVersion);
#endif

	/**
	 * NOTE: MSVC does not define __STDC__ by default.
	 * If __STDC__ is not defined, the libpng copyright
	 * will not have a leading newline, and all newlines
	 * will be replaced with groups of 6 spaces.
	 */
	// NOTE: Ignoring this for the GTK+ build, since it's
	// only built for Linux systems.
	sLibraries += png_get_copyright(nullptr);
	sLibraries += "<a href='http://www.libpng.org/pub/png/libpng.html'>http://www.libpng.org/pub/png/libpng.html</a>\n";
	sLibraries += "<a href='https://github.com/glennrp/libpng'>https://github.com/glennrp/libpng</a>\n";
	if (APNG_is_supported) {
		sLibraries += C_("AboutTab|Libraries", "APNG patch:");
		sLibraries += " <a href='https://sourceforge.net/projects/libpng-apng/'>https://sourceforge.net/projects/libpng-apng/</a>\n";
	}
	sLibraries += rp_sprintf(sLicense, "libpng license");
#endif /* HAVE_PNG */

	/** nettle **/
#ifdef ENABLE_DECRYPTION
	sLibraries += "\n\n";
#  ifdef HAVE_NETTLE_VERSION_H
	snprintf(sVerBuf, sizeof(sVerBuf),
		"GNU Nettle %u.%u",
			static_cast<unsigned int>(NETTLE_VERSION_MAJOR),
			static_cast<unsigned int>(NETTLE_VERSION_MINOR));
	sLibraries += rp_sprintf(sCompiledWith, sVerBuf);
#    ifdef HAVE_NETTLE_VERSION_FUNCTIONS
	snprintf(sVerBuf, sizeof(sVerBuf),
		"GNU Nettle %u.%u",
			static_cast<unsigned int>(nettle_version_major()),
			static_cast<unsigned int>(nettle_version_minor()));
	sLibraries += '\n';
	sLibraries += rp_sprintf(sUsingDll, sVerBuf);
#    endif /* HAVE_NETTLE_VERSION_FUNCTIONS */
	sLibraries += '\n';
	sLibraries +=
		"Copyright (C) 2001-2022 Niels Möller.\n"
		"<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>\n";
	sLibraries += rp_sprintf(sLicenses, "GNU LGPL v3+, GNU GPL v2+");
#  else /* !HAVE_NETTLE_VERSION_H */
#    ifdef HAVE_NETTLE_3
	sLibraries += rp_sprintf(sCompiledWith, "GNU Nettle 3.0");
	sLibraries += '\n';
	sLibraries +=
		"Copyright (C) 2001-2014 Niels Möller.\n"
		"<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>\n";
	sLibraries += rp_sprintf(sLicenses, "GNU LGPL v3+, GNU GPL v2+");
#    else /* !HAVE_NETTLE_3 */
	sLibraries += rp_sprintf(sCompiledWith, "GNU Nettle 2.x");
	sLibraries += '\n';
	sLibraries +=
		"Copyright (C) 2001-2013 Niels Möller.\n"
		"<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>\n";
	sLibraries += rp_sprintf(sLicense, "GNU LGPL v2.1+");
#    endif /* HAVE_NETTLE_3 */
#  endif /* HAVE_NETTLE_VERSION_H */
#endif /* ENABLE_DECRYPTION */

	/** TinyXML2 **/
#ifdef ENABLE_XML
	sLibraries += "\n\n";
	snprintf(sVerBuf, sizeof(sVerBuf), "TinyXML2 %u.%u.%u",
		static_cast<unsigned int>(TIXML2_MAJOR_VERSION),
		static_cast<unsigned int>(TIXML2_MINOR_VERSION),
		static_cast<unsigned int>(TIXML2_PATCH_VERSION));

#  if defined(USE_INTERNAL_XML) && !defined(USE_INTERNAL_XML_DLL)
	sLibraries += rp_sprintf(sIntCopyOf, sVerBuf);
#  else
	// FIXME: Runtime version?
	sLibraries += rp_sprintf(sCompiledWith, sVerBuf);
#  endif
	sLibraries += '\n';
	sLibraries +=
		"Copyright (C) 2000-2021 Lee Thomason\n"
		"<a href='http://www.grinninglizard.com/'>http://www.grinninglizard.com/</a>\n";
	sLibraries += rp_sprintf(sLicense, "zlib license");
#endif /* ENABLE_XML */

	/** GNU gettext **/
	// NOTE: glibc's libintl.h doesn't have the version information,
	// so we're only printing this if we're using GNU gettext's version.
#if defined(HAVE_GETTEXT) && defined(LIBINTL_VERSION)
	if (LIBINTL_VERSION & 0xFF) {
		snprintf(sVerBuf, sizeof(sVerBuf), "GNU gettext %u.%u.%u",
			static_cast<unsigned int>( LIBINTL_VERSION >> 16),
			static_cast<unsigned int>((LIBINTL_VERSION >>  8) & 0xFF),
			static_cast<unsigned int>( LIBINTL_VERSION        & 0xFF));
	} else {
		snprintf(sVerBuf, sizeof(sVerBuf), "GNU gettext %u.%u",
			static_cast<unsigned int>( LIBINTL_VERSION >> 16),
			static_cast<unsigned int>((LIBINTL_VERSION >>  8) & 0xFF));
	}
#  ifdef _WIN32
	sLibraries += rp_sprintf(sIntCopyOf, sVerBuf);
#  else /* _WIN32 */
	// FIXME: Runtime version?
	sLibraries += rp_sprintf(sCompiledWith, sVerBuf);
#  endif /* _WIN32 */
	sLibraries += '\n';
	sLibraries +=
		"Copyright (C) 1995-1997, 2000-2016, 2018-2020 Free Software Foundation, Inc.\n"
		"<a href='https://www.gnu.org/software/gettext/'>https://www.gnu.org/software/gettext/</a>\n";
	sLibraries += rp_sprintf(sLicense, "GNU LGPL v2.1+");
#endif /* HAVE_GETTEXT && LIBINTL_VERSION */

	// We're done building the string.
	gtk_label_set_markup(lblLibraries, sLibraries.c_str());
}
