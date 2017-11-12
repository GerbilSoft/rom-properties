/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * AboutTab.hpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
using LibRpBase::AboutTabText;

// C includes. (C++ namespace)
#include <cassert>

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
		AboutTabPrivate();

	private:
		Q_DISABLE_COPY(AboutTabPrivate)

	public:
		Ui::AboutTab ui;

	protected:
		// Useful strings.
		const QString br;
		const QString brbr;
		const QString b_start;
		const QString b_end;
		const QString sIndent;
		static const QChar chrBullet;

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
const QChar AboutTabPrivate::chrBullet = 0x2022;  // U+2022: BULLET

AboutTabPrivate::AboutTabPrivate()
	: br(QLatin1String("<br/>\n"))
	, brbr(QLatin1String("<br/>\n<br/>\n"))
	, b_start(QLatin1String("<b>"))
	, b_end(QLatin1String("</b>"))
	, sIndent(QLatin1String("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"))
{ }

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

	QString sPrgTitle;
	sPrgTitle.reserve(4096);
	sPrgTitle = b_start +
		AboutTab::tr("ROM Properties Page") + b_end + br +
		AboutTab::tr("Shell Extension") + br + br +
		AboutTab::tr("Version %1")
			.arg(QString::fromUtf8(AboutTabText::prg_version));
	if (AboutTabText::git_version[0] != 0) {
		sPrgTitle += br + QString::fromUtf8(AboutTabText::git_version);
		if (AboutTabText::git_describe[0] != 0) {
			sPrgTitle += br + QString::fromUtf8(AboutTabText::git_describe);
		}
	}

	ui.lblTitle->setText(sPrgTitle);
}

/**
 * Initialize the "Credits" tab.
 */
void AboutTabPrivate::initCreditsTab(void)
{
	// lblCredits is RichText.
	QString sCredits;
	sCredits.reserve(4096);
	sCredits += QLatin1String("Copyright (c) 2016-2017 by David Korth.");
	sCredits += br + AboutTab::tr("This program is licensed under the "
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
					sCredits += AboutTab::tr("Developers:");
					break;

				case AboutTabText::CT_CONTRIBUTOR:
					sCredits += AboutTab::tr("Contributors:");
					break;

				case AboutTabText::CT_TRANSLATOR:
					sCredits += AboutTab::tr("Translators:");
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
		sCredits += br + sIndent + chrBullet + QChar(L' ');
		sCredits += U82Q(creditsData->name);
		if (creditsData->url) {
			sCredits += QLatin1String(" &lt;<a href='") +
				U82Q(creditsData->url) +
				QLatin1String("'>");
			if (creditsData->linkText) {
				sCredits += U82Q(creditsData->linkText);
			} else {
				sCredits += U82Q(creditsData->url);
			}
			sCredits += QLatin1String("</a>&gt;");
		}
		if (creditsData->sub) {
			sCredits += QLatin1String(" (") +
				U82Q(creditsData->sub) +
				QChar(L')');
		}
	}

	// We're done building the string.
	ui.lblCredits->setText(sCredits);
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

	//: Using an internal copy of a library.
	const QString sIntCopyOf = AboutTab::tr("Internal copy of %1.");
	//: Compiled with a specific version of an external library.
	const QString sCompiledWith = AboutTab::tr("Compiled with %1.");
	//: Using an external library, e.g. libpcre.so
	const QString sUsingDll = AboutTab::tr("Using %1.");
	//: License: (libraries with only a single license)
	const QString sLicense = AboutTab::tr("License: %1");
	//: Licenses: (libraries with multiple licenses)
	const QString sLicenses = AboutTab::tr("Licenses: %1");

	// Included libraries string.
	QString sLibraries;
	sLibraries.reserve(4096);

	/** Qt **/
	const QString qtVersion = QLatin1String("Qt ") + QLatin1String(qVersion());
#ifdef QT_IS_STATIC
	sLibraries += sIntCopyOf.arg(qtVersion);
#else
	QString qtVersionCompiled = QLatin1String("Qt " QT_VERSION_STR);
	sLibraries += sCompiledWith.arg(qtVersionCompiled) + br;
	sLibraries += sUsingDll.arg(qtVersion);
#endif /* QT_IS_STATIC */
	sLibraries += br + QLatin1String("Copyright (C) 1995-2017 The Qt Company Ltd. and/or its subsidiaries.");
	sLibraries += br + QLatin1String("<a href='https://www.qt.io/'>https://www.qt.io/</a>");
	// TODO: Check QT_VERSION at runtime?
#if QT_VERSION >= QT_VERSION_CHECK(4,5,0)
	sLibraries += br + sLicenses.arg(QLatin1String("GNU LGPL v2.1+, GNU GPL v2+"));
#else
	sLibraries += br + sLicense.arg(QLatin1String("GNU GPL v2+"));
#endif /* QT_VERSION */

	/** KDE **/
	sLibraries += brbr;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	// NOTE: Can't obtain the runtime version for KDE5 easily...
	sLibraries += sCompiledWith.arg(QLatin1String("KDE Frameworks " KIO_VERSION_STRING));
	sLibraries += br + QLatin1String("Copyright (C) 1996-2017 KDE contributors.");
	sLibraries += br + QLatin1String("<a href='https://www.kde.org/'>https://www.kde.org/</a>");
	sLibraries += br + sLicense.arg(QLatin1String("GNU LGPL v2.1+"));
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
	const QString kdeVersion = QLatin1String("KDE Libraries ") + QLatin1String(KDE::versionString());
	sLibraries += sCompiledWith.arg(QLatin1String("KDE Libraries " KDE_VERSION_STRING));
	sLibraries += br + sUsingDll.arg(kdeVersion);
	sLibraries += br + QLatin1String("Copyright (C) 1996-2017 KDE contributors.");
	sLibraries += br + sLicense.arg(QLatin1String("GNU LGPL v2.1+"));
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

	/** zlib **/
#ifdef HAVE_ZLIB
	sLibraries += brbr;
	QString sZlibVersion = QLatin1String("zlib %1");
	sZlibVersion = sZlibVersion.arg(QLatin1String(zlibVersion()));

#if defined(USE_INTERNAL_ZLIB) && !defined(USE_INTERNAL_ZLIB_DLL)
	sLibraries += sIntCopyOf.arg(sZlibVersion)
#else
	QString sZlibVersionCompiled = QLatin1String("zlib " ZLIB_VERSION);
	sLibraries += sCompiledWith.arg(sZlibVersionCompiled) + br;
	sLibraries += sUsingDll.arg(sZlibVersion);
#endif
	sLibraries += br +
		QLatin1String("Copyright (C) 1995-2017 Jean-loup Gailly and Mark Adler.");
	sLibraries += br + QLatin1String("<a href='https://zlib.net/'>https://zlib.net/</a>");
	sLibraries += br + sLicense.arg(QLatin1String("zlib license"));
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

	const QString pngAPngSuffix = (APNG_is_supported
		? QLatin1String(" + APNG")
		: AboutTab::tr(" (No APNG support)"));

	sLibraries += brbr;
	const uint32_t png_version_number = png_access_version_number();
	QString pngVersion = QString::fromLatin1("libpng %1.%2.%3")
		.arg(png_version_number / 10000)
		.arg((png_version_number / 100) % 100)
		.arg(png_version_number % 100);
	pngVersion += pngAPngSuffix;

#if defined(USE_INTERNAL_PNG) && !defined(USE_INTERNAL_ZLIB_DLL)
	sLibraries += sIntCopyOf.arg(pngVersion);
#else
	// NOTE: Gentoo's libpng has "+apng" at the end of
	// PNG_LIBPNG_VER_STRING if APNG is enabled.
	// We have our own "+ APNG", so remove Gentoo's.
	QString pngVersionCompiled = QLatin1String("libpng " PNG_LIBPNG_VER_STRING);
	while (!pngVersionCompiled.isEmpty()) {
		int idx = pngVersionCompiled.size() - 1;
		const QChar chr = pngVersionCompiled[idx];
		if (chr.isDigit())
			break;
		pngVersionCompiled.resize(idx);
	}
	pngVersionCompiled += pngAPngSuffix;
	sLibraries += sCompiledWith.arg(pngVersionCompiled) + br;
	sLibraries += sUsingDll.arg(pngVersion);
#endif

	/**
	 * NOTE: MSVC does not define __STDC__ by default.
	 * If __STDC__ is not defined, the libpng copyright
	 * will not have a leading newline, and all newlines
	 * will be replaced with groups of 6 spaces.
	 */
	QString png_copyright = QLatin1String(png_get_copyright(nullptr));
	if (png_copyright.indexOf(QChar(L'\n')) < 0) {
		// Convert spaces to "<br/>\n".
		// TODO: QString::simplified() to remove other patterns,
		// or just assume all versions of libpng have the same
		// number of spaces?
		png_copyright.replace(QLatin1String("      "), br);
		png_copyright.prepend(br);
		png_copyright.append(br);
	} else {
		// Replace newlines with "<br/>\n".
		png_copyright.replace(QChar(L'\n'), br);
	}
	sLibraries += png_copyright;
	sLibraries += QLatin1String("<a href='http://www.libpng.org/pub/png/libpng.html'>http://www.libpng.org/pub/png/libpng.html</a>");
	sLibraries += br + sLicense.arg(QLatin1String("libpng license"));
#endif /* HAVE_PNG */

	/** nettle **/
#ifdef ENABLE_DECRYPTION
	sLibraries += brbr;
# ifdef HAVE_NETTLE_VERSION_H
	QString nettle_build_version = QLatin1String("GNU Nettle %1.%2");
	sLibraries += sCompiledWith.arg(
		nettle_build_version
			.arg(NETTLE_VERSION_MAJOR)
			.arg(NETTLE_VERSION_MINOR));
#  ifdef HAVE_NETTLE_VERSION_FUNCTIONS
	QString nettle_runtime_version = QLatin1String("GNU Nettle %1.%2");
	sLibraries += br + sUsingDll.arg(
		nettle_runtime_version
			.arg(nettle_version_major())
			.arg(nettle_version_minor()));
#  endif /* HAVE_NETTLE_VERSION_FUNCTIONS */
	sLibraries += br + QString::fromUtf8("Copyright (C) 2001-2016 Niels Möller.");
	sLibraries += br + QLatin1String("<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>");
	sLibraries += br + sLicenses.arg(QLatin1String("GNU LGPL v3+, GNU GPL v2+"));
# else /* !HAVE_NETTLE_VERSION_H */
#  ifdef HAVE_NETTLE_3
	sLibraries += sCompiledWith.arg(QLatin1String("GNU Nettle 3.0"));
	sLibraries += br + QString::fromUtf8("Copyright (C) 2001-2014 Niels Möller.");
	sLibraries += br + QLatin1String("<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>");
	sLibraries += br + sLicense.arg(QLatin1String("GNU LGPL v3+, GNU GPL v2+"));
#  else /* !HAVE_NETTLE_3 */
	sLibraries += sCompiledWith.arg(QLatin1String("GNU Nettle 2.x"));
	sLibraries += br + QString::fromUtf8("Copyright (C) 2001-2013 Niels Möller.");
	sLibraries += br + QLatin1String("<a href='https://www.lysator.liu.se/~nisse/nettle/'>https://www.lysator.liu.se/~nisse/nettle/</a>");
	sLibraries += br + sLicense.arg(QLatin1String("GNU LGPL v2.1+"));
#  endif /* HAVE_NETTLE_3 */
# endif /* HAVE_NETTLE_VERSION_H */
#endif /* ENABLE_DECRYPTION */

	/** TinyXML2 **/
#ifdef ENABLE_XML
	sLibraries += brbr;
	QString sXmlVersion = QLatin1String("TinyXML2 %1.%2.%3");
	sXmlVersion = sXmlVersion.arg(TIXML2_MAJOR_VERSION)
			.arg(TIXML2_MINOR_VERSION)
			.arg(TIXML2_PATCH_VERSION);

#if defined(USE_INTERNAL_XML) && !defined(USE_INTERNAL_XML_DLL)
	sLibraries += sIntCopyOf.arg(sXmlVersion)
#else
	// FIXME: Runtime version?
	sLibraries += sCompiledWith.arg(sXmlVersion);
#endif
	sLibraries += br + QLatin1String("Copyright (C) 2000-2017 Lee Thomason");
	sLibraries += br + QLatin1String("<a href='http://www.grinninglizard.com/'>http://www.grinninglizard.com/</a>");
	sLibraries += br + sLicense.arg(QLatin1String("zlib license"));
#endif /* ENABLE_XML */

	// We're done building the string.
	ui.lblLibraries->setText(sLibraries);
}

/**
 * Initialize the "Support" tab.
 */
void AboutTabPrivate::initSupportTab(void)
{
	// lblSupport is RichText.
	QString sSupport;
	sSupport.reserve(4096);
	sSupport = AboutTab::tr(
		"For technical support, you can visit the following websites:") + br;

	for (const AboutTabText::SupportSite_t *supportSite = &AboutTabText::SupportSites[0];
	     supportSite->name != nullptr; supportSite++)
	{
		QString qs_url = U82Q(supportSite->url);
		sSupport += sIndent + chrBullet + QChar(L' ') +
			U82Q(supportSite->name) +
			QLatin1String(" &lt;<a href='") +
			qs_url +
			QLatin1String("'>") +
			qs_url +
			QLatin1String("</a>&gt;") + br;
	}

	// Email the author.
	sSupport += br +
		AboutTab::tr("You can also email the developer directly:") + br +
		sIndent + chrBullet + QChar(L' ') +
			QLatin1String("<a href=\"mailto:gerbilsoft@gerbilsoft.com\">"
			"David Korth &lt;gerbilsoft@gerbilsoft.com&gt;</a>");

	// We're done building the string.
	ui.lblSupport->setText(sSupport);
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
