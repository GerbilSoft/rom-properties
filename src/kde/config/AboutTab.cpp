/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * AboutTab.hpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "AboutTab.hpp"

// librpbase
#include "librpbase/config/AboutTabText.hpp"
using namespace LibRpBase;

// C++ STL classes.
using std::string;

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
// KIO version.
// NOTE: Only available as a compile-time constant.
#  include <kio_version.h>
#else
// kdelibs version.
#  include <kdeversion.h>
#endif

// Other libraries.
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
#define BR "<br/>\n"
const char AboutTabPrivate::br[] = BR;
const char AboutTabPrivate::brbr[] = BR BR;
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
	sPrgTitle += brbr;
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
	// License name, with HTML formatting.
	const string sPrgLicense = rp_sprintf("<a href='https://www.gnu.org/licenses/gpl-2.0.html'>%s</a>",
		C_("AboutTabl|Credits", "GNU GPL v2"));

	// lblCredits is RichText.
	string sCredits;
	sCredits.reserve(4096);
	// NOTE: Copyright is NOT localized.
	sCredits += "Copyright (c) 2016-2022 by David Korth." BR;
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
			sCredits += brbr;
			sCredits += b_start;

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

		lastCreditType = creditsData->type;
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

	/** Qt **/
	string qtVersion = "Qt ";
	qtVersion += qVersion();
#ifdef QT_IS_STATIC
	sLibraries += rp_sprintf(sIntCopyOf, qtVersion.c_str());
#else
	sLibraries += rp_sprintf(sCompiledWith, "Qt " QT_VERSION_STR) + br;
	sLibraries += rp_sprintf(sUsingDll, qtVersion.c_str());
#endif /* QT_IS_STATIC */
	sLibraries += BR
		"Copyright (C) 1995-2021 The Qt Company Ltd. and/or its subsidiaries." BR
		"<a href='https://www.qt.io/'>https://www.qt.io/</a>" BR;
	// TODO: Check QT_VERSION at runtime?
#if QT_VERSION >= QT_VERSION_CHECK(4,5,0)
	sLibraries += rp_sprintf(sLicenses, "GNU LGPL v2.1+, GNU GPL v2+");
#else
	sLibraries += rp_sprintf(sLicense, "GNU GPL v2+");
#endif /* QT_VERSION */

	/** KDE **/
	sLibraries += brbr;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	// NOTE: Can't obtain the runtime version for KF5 easily...
	sLibraries += rp_sprintf(sCompiledWith, "KDE Frameworks " KIO_VERSION_STRING);
	sLibraries += BR
		"Copyright (C) 1996-2021 KDE contributors." BR
		"<a href='https://www.kde.org/'>https://www.kde.org/</a>" BR;
	sLibraries += rp_sprintf(sLicense, "GNU LGPL v2.1+");
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
	string kdeVersion = "KDE Libraries ";
	kdeVersion += KDE::versionString();
	sLibraries += rp_sprintf(sCompiledWith, "KDE Libraries " KDE_VERSION_STRING);
	sLibraries += br;
	sLibraries += rp_sprintf(sUsingDll, kdeVersion.c_str());
	sLibraries += BR
		"Copyright (C) 1996-2017 KDE contributors." BR;
	sLibraries += rp_sprintf(sLicense, "GNU LGPL v2.1+");
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

	/** zlib **/
#ifdef HAVE_ZLIB
	sLibraries += brbr;
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
	sLibraries += br;
	sLibraries += rp_sprintf(sUsingDll, sZlibVersion.c_str());
#endif
	sLibraries += BR
		"Copyright (C) 1995-2022 Jean-loup Gailly and Mark Adler." BR
		"<a href='https://zlib.net/'>https://zlib.net/</a>" BR;
#  ifdef ZLIBNG_VERSION
	// TODO: Also if zlibVersion() contains "zlib-ng"?
	sLibraries += "<a href='https://github.com/zlib-ng/zlib-ng'>https://github.com/zlib-ng/zlib-ng</a>" BR;
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

	sLibraries += brbr;
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
	sLibraries += br;
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
	sLibraries += "<a href='http://www.libpng.org/pub/png/libpng.html'>http://www.libpng.org/pub/png/libpng.html</a>" BR;
	sLibraries += "<a href='https://github.com/glennrp/libpng'>https://github.com/glennrp/libpng</a>\n";
	if (APNG_is_supported) {
		sLibraries += C_("AboutTab|Libraries", "APNG patch:");
		sLibraries += " <a href='https://sourceforge.net/projects/libpng-apng/'>https://sourceforge.net/projects/libpng-apng/</a>" BR;
	}
	sLibraries += rp_sprintf(sLicense, "libpng license");
#endif /* HAVE_PNG */

	/** nettle **/
#ifdef ENABLE_DECRYPTION
	sLibraries += brbr;
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
	sLibraries += br;
	sLibraries += rp_sprintf(sUsingDll, sVerBuf);
#    endif /* HAVE_NETTLE_VERSION_FUNCTIONS */
	sLibraries += BR
		"Copyright (C) 2001-2022 Niels Möller." BR
		"<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>" BR;
	sLibraries += rp_sprintf(sLicenses, "GNU LGPL v3+, GNU GPL v2+");
#  else /* !HAVE_NETTLE_VERSION_H */
#    ifdef HAVE_NETTLE_3
	sLibraries += rp_sprintf(sCompiledWith, "GNU Nettle 3.0");
	sLibraries += BR
		"Copyright (C) 2001-2014 Niels Möller." BR
		"<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>" BR;
	sLibraries += rp_sprintf(sLicenses, "GNU LGPL v3+, GNU GPL v2+");
#    else /* !HAVE_NETTLE_3 */
	sLibraries += rp_sprintf(sCompiledWith, "GNU Nettle 2.x");
	sLibraries += BR
		"Copyright (C) 2001-2013 Niels Möller." BR
		"<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>" BR;
	sLibraries += rp_sprintf(sLicense, "GNU LGPL v2.1+");
#    endif /* HAVE_NETTLE_3 */
#  endif /* HAVE_NETTLE_VERSION_H */
#endif /* ENABLE_DECRYPTION */

	/** TinyXML2 **/
#ifdef ENABLE_XML
	sLibraries += brbr;
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
	sLibraries += BR
		"Copyright (C) 2000-2021 Lee Thomason" BR
		"<a href='http://www.grinninglizard.com/'>http://www.grinninglizard.com/</a>" BR;
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
	sLibraries += BR
		"Copyright (C) 1995-1997, 2000-2016, 2018-2020 Free Software Foundation, Inc." BR
		"<a href='https://www.gnu.org/software/gettext/'>https://www.gnu.org/software/gettext/</a>" BR;
	sLibraries += rp_sprintf(sLicense, "GNU LGPL v2.1+");
#endif /* HAVE_GETTEXT && LIBINTL_VERSION */

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
	sSupport += "David Korth "
		"&lt;<a href=\"mailto:gerbilsoft@gerbilsoft.com\">"
		"gerbilsoft@gerbilsoft.com</a>&gt;";

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
