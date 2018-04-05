/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * AboutTab.hpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "config.librpbase.h"

#include "AboutTab.hpp"
#include "RpQt.hpp"

// librpbase
#include "librpbase/config/AboutTabText.hpp"
#include "librpbase/TextFuncs.hpp"
using namespace LibRpBase;

// libi18n
#include "libi18n/i18n.h"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>

// C++ includes.
#include <string>
using std::string;

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
// KIO version.
// NOTE: Only available as a compile-time constant.
#include <kio_version.h>
#else
// kdelibs version.
#include <kdeversion.h>
#endif

// Other libraries.
#ifdef HAVE_ZLIB
# include <zlib.h>
#endif
#ifdef HAVE_PNG
# include "librpbase/img/APNG_dlopen.h"
# include <png.h>
#endif
#ifdef ENABLE_DECRYPTION
# ifdef HAVE_NETTLE_VERSION_H
#  include "nettle/version.h"
# endif
#endif
#ifdef ENABLE_XML
# include "tinyxml2.h"
#endif

#include "ui_AboutTab.h"
class AboutTabPrivate
{
	public:
		AboutTabPrivate() { };

	private:
		Q_DISABLE_COPY(AboutTabPrivate)

	public:
		Ui::AboutTab ui;

	protected:
		// Useful strings.
		static const char br[];
		static const char brbr[];
		static const char b_start[];
		static const char b_end[];
		static const char sIndent[];
		static const char chrBullet[];

	protected:
		/**
		 * Initialize the program title text.
		 */
		void initProgramTitleText(void);

		/**
		 * Initialize the "Credits" tab.
		 */
		void initCreditsTab(void);

		/**
		 * Initialize the "Libraries" tab.
		 */
		void initLibrariesTab(void);

		/**
		 * Initialize the "Support" tab.
		 */
		void initSupportTab(void);

	public:
		/**
		 * Initialize the dialog.
		 */
		void init(void);
};

/** AboutTabPrivate **/

// Useful strings.
const char AboutTabPrivate::br[] = "<br/>\n";
const char AboutTabPrivate::brbr[] = "<br/>\n<br/>\n";
const char AboutTabPrivate::b_start[] = "<b>";
const char AboutTabPrivate::b_end[] = "</b>";
const char AboutTabPrivate::sIndent[] = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
const char AboutTabPrivate::chrBullet[] = "\xE2\x80\xA2";	// U+2022: BULLET

/**
 * Initialize the program title text.
 */
void AboutTabPrivate::initProgramTitleText(void)
{
	// lblTitle is RichText.

	// Program icon.
	// TODO: Make a custom icon instead of reusing the system icon.
	// TODO: Fallback for older Qt?
#if QT_VERSION >= QT_VERSION_CHECK(4,6,0)
	QIcon icon = QIcon::fromTheme(QLatin1String("media-flash"));
	if (!icon.isNull()) {
		// Get the 128x128 icon.
		// TODO: Determine the best size.
		ui.lblLogo->setPixmap(icon.pixmap(128, 128));
	} else {
		// No icon...
		ui.lblLogo->hide();
	}
#endif /* QT_VERSION >= QT_VERSION_CHECK(4,6,0) */

	string sPrgTitle;
	sPrgTitle.reserve(1024);
	// tr: Uses Qt's HTML subset for formatting.
	sPrgTitle += C_("AboutTab", "<b>ROM Properties Page</b><br>Shell Extension");
	sPrgTitle += br;
	sPrgTitle += br;
	sPrgTitle += rp_sprintf(C_("AboutTab", "Version %s"),
		AboutTabText::prg_version);
	if (AboutTabText::git_version[0] != 0) {
		sPrgTitle += br;
		sPrgTitle += AboutTabText::git_version;
		if (AboutTabText::git_describe[0] != 0) {
			sPrgTitle += br;
			sPrgTitle += AboutTabText::git_describe;
		}
	}

	ui.lblTitle->setText(U82Q(sPrgTitle));
}

/**
 * Initialize the "Credits" tab.
 */
void AboutTabPrivate::initCreditsTab(void)
{
	// lblCredits is RichText.
	string sCredits;
	sCredits.reserve(4096);
	// NOTE: Copyright is NOT localized.
	sCredits += "Copyright (c) 2016-2018 by David Korth.<br/>";
	sCredits += C_("AboutTab|Credits",
		"This program is licensed under the "
		"<a href='https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html'>GNU GPL v2</a> or later.");

	AboutTabText::CreditType_t lastCreditType = AboutTabText::CT_CONTINUE;
	for (const AboutTabText::CreditsData_t *creditsData = &AboutTabText::CreditsData[0];
	     creditsData->type < AboutTabText::CT_MAX; creditsData++)
	{
		if (creditsData->type != AboutTabText::CT_CONTINUE &&
		    creditsData->type != lastCreditType)
		{
			// New credit type.
			sCredits += brbr;
			sCredits += b_start;

			switch (creditsData->type) {
				case AboutTabText::CT_DEVELOPER:
					sCredits += C_("AboutTab|Credits", "Developers:");
					break;
				case AboutTabText::CT_CONTRIBUTOR:
					sCredits += C_("AboutTab|Credits", "Contributors:");
					break;
				case AboutTabText::CT_TRANSLATOR:
					sCredits += C_("AboutTab|Credits", "Translators:");
					break;

				case AboutTabText::CT_CONTINUE:
				case AboutTabText::CT_MAX:
				default:
					assert(!"Invalid credit type.");
					break;
			}

			sCredits += b_end;
		}

		// Append the contributor's name.
		sCredits += br;
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
	}

	// We're done building the string.
	ui.lblCredits->setText(U82Q(sCredits));
}

/**
 * Initialize the "Libraries" tab.
 */
void AboutTabPrivate::initLibrariesTab(void)
{
	// lblLibraries is RichText.

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
	sLibraries.reserve(4096);

	/** Qt **/
	string qtVersion = "Qt ";
	qtVersion += qVersion();
#ifdef QT_IS_STATIC
	sLibraries += rp_sprintf(sIntCopyOf, qtVersion.c_str());
#else
	sLibraries += rp_sprintf(sCompiledWith, "Qt " QT_VERSION_STR) + br;
	sLibraries += rp_sprintf(sUsingDll, qtVersion.c_str());
#endif /* QT_IS_STATIC */
	sLibraries += br;
	sLibraries += "Copyright (C) 1995-2017 The Qt Company Ltd. and/or its subsidiaries.";
	sLibraries += br;
	sLibraries += "<a href='https://www.qt.io/'>https://www.qt.io/</a>";
	// TODO: Check QT_VERSION at runtime?
#if QT_VERSION >= QT_VERSION_CHECK(4,5,0)
	sLibraries += br;
	sLibraries += rp_sprintf(sLicenses, "GNU LGPL v2.1+, GNU GPL v2+");
#else
	sLibraries += br;
	sLibraries += rp_sprintf(sLicense, "GNU GPL v2+");
#endif /* QT_VERSION */

	/** KDE **/
	sLibraries += brbr;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	// NOTE: Can't obtain the runtime version for KDE5 easily...
	sLibraries += rp_sprintf(sCompiledWith, "KDE Frameworks " KIO_VERSION_STRING);
	sLibraries += br;
	sLibraries += "Copyright (C) 1996-2017 KDE contributors.";
	sLibraries += br;
	sLibraries += "<a href='https://www.kde.org/'>https://www.kde.org/</a>";
	sLibraries += br;
	sLibraries += rp_sprintf(sLicense, "GNU LGPL v2.1+");
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
	string kdeVersion = "KDE Libraries ";
	kdeVersion += KDE::versionString();
	sLibraries += rp_sprintf(sCompiledWith, "KDE Libraries " KDE_VERSION_STRING);
	sLibraries += br;
	sLibraries += rp_sprintf(sUsingDll, kdeVersion.c_str());
	sLibraries += br;
	sLibraries += "Copyright (C) 1996-2017 KDE contributors.";
	sLibraries += br;
	sLibraries += rp_sprintf(sLicense, "GNU LGPL v2.1+");
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

	/** zlib **/
#ifdef HAVE_ZLIB
	sLibraries += brbr;
	string sZlibVersion = "zlib ";
	sZlibVersion += zlibVersion();

#if defined(USE_INTERNAL_ZLIB) && !defined(USE_INTERNAL_ZLIB_DLL)
	sLibraries += rp_sprintf(sIntCopyOf, sZlibVersion.c_str());
#else
	sLibraries += rp_sprintf(sCompiledWith, "zlib " ZLIB_VERSION);
	sLibraries += br;
	sLibraries += rp_sprintf(sUsingDll, sZlibVersion.c_str());
#endif
	sLibraries += br;
	sLibraries += "Copyright (C) 1995-2017 Jean-loup Gailly and Mark Adler.";
	sLibraries += br;
	sLibraries += "<a href='https://zlib.net/'>https://zlib.net/</a>";
	sLibraries += br;
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

	sLibraries += brbr;
	const uint32_t png_version_number = png_access_version_number();
	char pngVersion[32];
	snprintf(pngVersion, sizeof(pngVersion), "libpng %u.%u.%u",
		png_version_number / 10000,
		(png_version_number / 100) % 100,
		png_version_number % 100);

	string fullPngVersion;
	if (APNG_is_supported) {
		// PNG version, with APNG support.
		fullPngVersion = rp_sprintf("%s + APNG", pngVersion);
	} else {
		// PNG version, without APNG support.
		fullPngVersion = rp_sprintf("%s (No APNG support)", pngVersion);
	}

#if defined(USE_INTERNAL_PNG) && !defined(USE_INTERNAL_ZLIB_DLL)
	sLibraries += rp_sprintf(sIntCopyOf, fullPngVersion.c_str());
#else
	// NOTE: Gentoo's libpng has "+apng" at the end of
	// PNG_LIBPNG_VER_STRING if APNG is enabled.
	// We have our own "+ APNG", so remove Gentoo's.
	string pngVersionCompiled = "libpng " PNG_LIBPNG_VER_STRING;
	while (!pngVersionCompiled.empty()) {
		size_t idx = pngVersionCompiled.size() - 1;
		char chr = pngVersionCompiled[idx];
		if (isdigit(chr))
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
	sLibraries += br;
	sLibraries += rp_sprintf(sUsingDll, fullPngVersion.c_str());
#endif

	/**
	 * NOTE: MSVC does not define __STDC__ by default.
	 * If __STDC__ is not defined, the libpng copyright
	 * will not have a leading newline, and all newlines
	 * will be replaced with groups of 6 spaces.
	 */
	QString png_copyright = QLatin1String(png_get_copyright(nullptr));
	const QString qs_br = QLatin1String(br);
	if (png_copyright.indexOf(QChar(L'\n')) < 0) {
		// Convert spaces to "<br/>\n".
		// TODO: QString::simplified() to remove other patterns,
		// or just assume all versions of libpng have the same
		// number of spaces?
		png_copyright.replace(QLatin1String("      "), qs_br);
		png_copyright.prepend(qs_br);
		png_copyright.append(qs_br);
	} else {
		// Replace newlines with "<br/>\n".
		png_copyright.replace(QChar(L'\n'), qs_br);
	}
	sLibraries += png_copyright.toUtf8().constData();
	sLibraries += "<a href='http://www.libpng.org/pub/png/libpng.html'>http://www.libpng.org/pub/png/libpng.html</a>";
	sLibraries += br;
	sLibraries += rp_sprintf(sLicense, "libpng license");
#endif /* HAVE_PNG */

	/** nettle **/
#ifdef ENABLE_DECRYPTION
	sLibraries += brbr;
# ifdef HAVE_NETTLE_VERSION_H
	char nettle_build_version[32];
	snprintf(nettle_build_version, sizeof(nettle_build_version),
		"GNU Nettle %u.%u", NETTLE_VERSION_MAJOR, NETTLE_VERSION_MINOR);
	sLibraries += rp_sprintf(sCompiledWith, nettle_build_version);
#  ifdef HAVE_NETTLE_VERSION_FUNCTIONS
	char nettle_runtime_version[32];
	snprintf(nettle_runtime_version, sizeof(nettle_runtime_version),
		"GNU Nettle %u.%u", nettle_version_major(), nettle_version_minor());
	sLibraries += br;
	sLibraries += rp_sprintf(sUsingDll, nettle_runtime_version);
#  endif /* HAVE_NETTLE_VERSION_FUNCTIONS */
	sLibraries += br;
	sLibraries += "Copyright (C) 2001-2016 Niels Möller.";
	sLibraries += br;
	sLibraries += "<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>";
	sLibraries += br;
	sLibraries += rp_sprintf(sLicenses, "GNU LGPL v3+, GNU GPL v2+");
# else /* !HAVE_NETTLE_VERSION_H */
#  ifdef HAVE_NETTLE_3
	sLibraries += rp_sprintf(sCompiledWith, "GNU Nettle 3.0");
	sLibraries += br;
	sLibraries += "Copyright (C) 2001-2014 Niels Möller.";
	sLibraries += br;
	sLibraries += "<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>";
	sLibraries += br;
	sLibraries += rp_sprintf(sLicenses, "GNU LGPL v3+, GNU GPL v2+");
#  else /* !HAVE_NETTLE_3 */
	sLibraries += rp_sprintf(sCompiledWith, "GNU Nettle 2.x");
	sLibraries += br;
	sLibraries += "Copyright (C) 2001-2013 Niels Möller.";
	sLibraries += br;
	sLibraries += "<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>";
	sLibraries += br;
	sLibraries += rp_sprintf(sLicense, "GNU LGPL v2.1+");
#  endif /* HAVE_NETTLE_3 */
# endif /* HAVE_NETTLE_VERSION_H */
#endif /* ENABLE_DECRYPTION */

	/** TinyXML2 **/
#ifdef ENABLE_XML
	sLibraries += brbr;
	char sXmlVersion[32];
	snprintf(sXmlVersion, sizeof(sXmlVersion), "TinyXML2 %u.%u.%u",
		TIXML2_MAJOR_VERSION, TIXML2_MINOR_VERSION,
		TIXML2_PATCH_VERSION);

#if defined(USE_INTERNAL_XML) && !defined(USE_INTERNAL_XML_DLL)
	sLibraries += rp_sprintf(sIntCopyOf, sXmlVersion);
#else
	// FIXME: Runtime version?
	sLibraries += rp_sprintf(sCompiledWith, sXmlVersion);
#endif
	sLibraries += br;
	sLibraries += "Copyright (C) 2000-2017 Lee Thomason";
	sLibraries += br;
	sLibraries += "<a href='http://www.grinninglizard.com/'>http://www.grinninglizard.com/</a>";
	sLibraries += br;
	sLibraries += rp_sprintf(sLicense, "zlib license");
#endif /* ENABLE_XML */

	// We're done building the string.
	ui.lblLibraries->setText(U82Q(sLibraries));
}

/**
 * Initialize the "Support" tab.
 */
void AboutTabPrivate::initSupportTab(void)
{
	// lblSupport is RichText.
	string sSupport;
	sSupport.reserve(4096);
	sSupport = C_("AboutTab|Support",
		"For technical support, you can visit the following websites:");
	sSupport += br;

	for (const AboutTabText::SupportSite_t *supportSite = &AboutTabText::SupportSites[0];
	     supportSite->name != nullptr; supportSite++)
	{
		sSupport += sIndent;
		sSupport += chrBullet;
		sSupport += ' ';
		sSupport += supportSite->name;
		sSupport += " &lt;<a href='";
		sSupport += supportSite->url;
		sSupport += "'>";
		sSupport += supportSite->url;
		sSupport += "</a>&gt;";
		sSupport += br;
	}

	// Email the author.
	sSupport += br;
	sSupport += C_("AboutTab|Support", "You can also email the developer directly:");
	sSupport += br;
	sSupport += sIndent;
	sSupport += chrBullet;
	sSupport += ' ';
	sSupport += "<a href=\"mailto:gerbilsoft@gerbilsoft.com\">"
		"David Korth &lt;gerbilsoft@gerbilsoft.com&gt;</a>";

	// We're done building the string.
	ui.lblSupport->setText(U82Q(sSupport));
}

/**
 * Initialize the dialog.
 */
void AboutTabPrivate::init(void)
{
	initProgramTitleText();
	initCreditsTab();
	initLibrariesTab();
	initSupportTab();
}

/** AboutTab **/

AboutTab::AboutTab(QWidget *parent)
	: super(parent)
	, d_ptr(new AboutTabPrivate())
{
	Q_D(AboutTab);
	d->ui.setupUi(this);

	// Initialize the dialog.
	d->init();
}

AboutTab::~AboutTab()
{
	delete d_ptr;
}

/**
 * Widget state has changed.
 * @param event State change event.
 */
void AboutTab::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(AboutTab);
		d->ui.retranslateUi(this);

		// Reinitialize the dialog.
		d->init();
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/**
 * Reset the configuration.
 */
void AboutTab::reset(void)
{
	// Nothing to do here.
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void AboutTab::loadDefaults(void)
{
	// Nothing to do here.
}

/**
 * Save the configuration.
 * @param pSettings QSettings object.
 */
void AboutTab::save(QSettings *pSettings)
{
	// Nothing to do here.
	Q_UNUSED(pSettings)
}
