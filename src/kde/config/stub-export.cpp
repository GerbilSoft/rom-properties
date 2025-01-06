/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * stub-export.cpp: Exported function for the rp-config stub.              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.kde.h"
#include "check-uid.hpp"

#include "ConfigDialog.hpp"
#include "RomDataView.hpp"
#include "xattr/XAttrView.hpp"

// Program version
#include "librpbase/config/AboutTabText.hpp"
using namespace LibRpBase;

// Other rom-properties libraries
#include "libromdata/RomDataFactory.hpp"
#include "librptexture/img/rp_image.hpp"
using namespace LibRomData;
using namespace LibRpTexture;

#include "RpQImageBackend.hpp"
#include "AchQtDBus.hpp"
#include "RpQUrl.hpp"

// i18n
#ifdef ENABLE_NLS
#  include "../GettextTranslator.hpp"
#endif

// Qt
#include "RpQt.hpp"

// KDE
#include <KAboutData>
#include <KCrash>
#include <KPageWidget>
#include <KPageWidgetModel>
#include <KPropertiesDialog>

// C++ STL classes
using std::string;

/**
 * Initialize the QApplication.
 * @param argc
 * @param argv
 * @param applicationDisplayName
 * @return QApplication
 */
static QApplication *initQApp(int &argc, char *argv[], const QString &applicationDisplayName)
{
	QApplication *app = qApp;
	if (app) {
		// QApplication is already initialized.

		// Initialize base i18n.
		// TODO: Install the translator even if we're reusing the QApplication?
		rp_i18n_init();

		return app;
	}

	// Set high-DPI mode on Qt 5. (not needed on Qt 6)
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0) && QT_VERSION < QT_VERSION_CHECK(6,0,0)
	// Enable High DPI.
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#if QT_VERSION >= QT_VERSION_CHECK(5,6,0)
	// Enable High DPI pixmaps.
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
#else
	// Hardcode the value in case the user upgrades to Qt 5.6 later.
	// http://doc.qt.io/qt-5/qt.html#ApplicationAttribute-enum
	QApplication::setAttribute(static_cast<Qt::ApplicationAttribute>(13), true);
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,6,0) */
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) && QT_VERSION < QT_VERSION_CHECK(6,0,0) */
	// Create the QApplication.
	app = new QApplication(argc, argv);

#ifdef ENABLE_NLS
	// Install the translator for Gettext translations.
	rp_i18n_init();
	app->installTranslator(new GettextTranslator());
#endif /* ENABLE_NLS */

	// Set the application information.
	// TODO: Different info for the RomDataView test program?
	app->setApplicationName(QLatin1String("rp-config"));
	app->setOrganizationDomain(QLatin1String("gerbilsoft.com"));
	app->setOrganizationName(QLatin1String("GerbilSoft"));
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	app->setApplicationDisplayName(applicationDisplayName);
#  if QT_VERSION >= QT_VERSION_CHECK(5,7,0)
	app->setDesktopFileName(QLatin1String("com.gerbilsoft.rom-properties.rp-config"));
#  endif /* QT_VERSION >= QT_VERSION_CHECK(5,7,0) */
#else /* !QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
	RP_UNUSED(applicationDisplayName);
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

	const char *const programVersion =
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::ProgramVersion);
	assert(programVersion != nullptr);
	if (programVersion) {
		app->setApplicationVersion(QLatin1String(programVersion));
	}

	return app;
}

/**
 * Exported function for the rp-config stub.
 * @param argc
 * @param argv
 * @return 0 on success; non-zero on error.
 */
extern "C"
Q_DECL_EXPORT int RP_C_API rp_show_config_dialog(int argc, char *argv[])
{
	CHECK_UID_RET(EXIT_FAILURE);

	const QString applicationDisplayName = QCoreApplication::translate(
		"ConfigDialog", "ROM Properties Page configuration", nullptr);
	QApplication *const app = initQApp(argc, argv, applicationDisplayName);

	// Set up KAboutData.
	const QString displayName = ConfigDialog::tr("ROM Properties Page configuration");
	const char *const copyrightString =
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::Copyright);
	assert(copyrightString != nullptr);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	KAboutData aboutData(
		QLatin1String("rp-config"),	// componentName
		displayName,			// displayName
		app->applicationVersion(),	// version
		displayName,			// shortDescription (TODO: Better value?)
		KAboutLicense::GPL_V2,		// licenseType
		QString::fromUtf8(copyrightString),	// copyrightStatement
		QString(),			// otherText
		QLatin1String("https://github.com/GerbilSoft/rom-properties"),		// homePageAddress
		QLatin1String("https://github.com/GerbilSoft/rom-properties/issues")	// bugAddress
	);
	KAboutData::setApplicationData(aboutData);
#else /* QT_VERSION < QT_VERSION_CHECK(5, 0, 0) */
	KAboutData aboutData(
		QByteArray("rp-config"),		// appName
		QByteArray("rom-properties"),		// catalogName
		KLocalizedString() /*displayName*/,	// programName
		app->applicationVersion().toUtf8(),	// version
		KLocalizedString() /*displayName*/,	// shortDescription (TODO: Better value?)
		KAboutData::License_GPL_V2,		// licenseType
		KLocalizedString(), /*QString::fromUtf8(copyrightString)*/	// copyrightStatement
		KLocalizedString(),			// otherText
		QByteArray("https://github.com/GerbilSoft/rom-properties"),		// homePageAddress
		QByteArray("https://github.com/GerbilSoft/rom-properties/issues")	// bugAddress
	);
	// FIXME: KAboutData::setAboutData() equivalent on KDE4.
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) */

	// Initialize KCrash.
	// FIXME: It shows bugs.kde.org as the bug reporting address, which isn't wanted...
	//KCrash::initialize();

	// Create and run the ConfigDialog.
	// TODO: Get the return value?
	ConfigDialog *const configDialog = new ConfigDialog();
	configDialog->setObjectName(QLatin1String("configDialog"));
	configDialog->show();

	// Run the Qt UI.
	// FIXME: May need changes if the main loop is already running.
	return app->exec();
}

/**
 * Exported function for the RomDataView test program stub.
 * @param argc
 * @param argv
 * @return 0 on success; non-zero on error.
 */
extern "C"
Q_DECL_EXPORT int RP_C_API rp_show_RomDataView_dialog(int argc, char *argv[])
{
	CHECK_UID_RET(EXIT_FAILURE);

	// TODO: argv[] needs to be updated such that [0] == argv[0] and [1] == URI.
	// For now, assuming the last element is the URI.
	if (argc < 2) {
		// Not enough parameters...
		fputs("*** " RP_KDE_UPPER " rp_show_RomDataView_dialog(): ERROR: No URI specified.\n", stderr);
		return EXIT_FAILURE;
	}
	const char *const uri = argv[argc-1];

	const QString applicationDisplayName = QLatin1String("RomDataView " RP_KDE_UPPER " test program");
	QApplication *const app = initQApp(argc, argv, applicationDisplayName);

	// Register RpQImageBackend and AchQtDBus.
	rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);
#if defined(ENABLE_ACHIEVEMENTS) && defined(HAVE_QtDBus_NOTIFY)
	AchQtDBus::instance();
#endif /* ENABLE_ACHIEVEMENTS && HAVE_QtDBus_NOTIFY */

	// Parse the specified URI and localize it.
	const QString qs_uri(QString::fromUtf8(uri));
#if QT_VERSION >= QT_VERSION_CHECK(5,4,0)
	const QUrl localUrl(QUrl::fromUserInput(qs_uri, QDir::current().absolutePath()));
#else /* QT_VERSION < QT_VERSION_CHECK(5,4,0) */
	// QUrl::fromUserInput()'s workingDirectory parameter was added in Qt 5.4.
	// For older versions, we'll have to check the CWD ourselves.
	const QFileInfo fileInfo(QDir::current(), qs_uri);
	const QUrl localUrl = (fileInfo.exists())
		? QUrl::fromLocalFile(fileInfo.absoluteFilePath())
		: QUrl::fromUserInput(qs_uri);
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,4,0) */
	if (localUrl.isEmpty()) {
		fprintf(stderr, "*** " RP_KDE_UPPER " rp_show_RomDataView_dialog(): URI '%s' is invalid.\n", uri);
		return EXIT_FAILURE;
	}
	fprintf(stderr, "*** " RP_KDE_UPPER " rp_show_RomDataView_dialog(): Opening URI: '%s'\n", uri);

	// Create a KPropertiesDialog.
	// FIXME: Remove the default "General" and "Permissions" tabs.
	// NOTE: Assuming we have a valid URL, KDE will automatically load
	// the rom-properties KPropertiesDialogPlugins.
	KPropertiesDialog *const dialog = new KPropertiesDialog(localUrl);
	dialog->setObjectName(QLatin1String("propertiesDialog"));
	dialog->show();

	// Set the current tab to one of our tabs (whichever shows up first).
	// FIXME: Removing tabs causes random SIGSEGV...
	bool hasPlugins = false;
	KPageWidget *const kpw = findDirectChild<KPageWidget*>(dialog);
	if (kpw) {
		KPageWidgetModel *const model = findDirectChild<KPageWidgetModel*>(kpw);

		// Assuming a single "column".
		assert(model->columnCount() == 1);
		const int rowCount = model->rowCount();
		for (int row = 0; row < rowCount; row++) {
			KPageWidgetItem *const item = model->item(model->index(row, 0));
			QWidget *const widget = item->widget();
			const char *const className = widget->metaObject()->className();

			if (!strcmp(className, "RomDataView") ||
			    !strcmp(className, "XAttrView"))
			{
				// Found one of our tabs.
				hasPlugins = true;
				kpw->setCurrentPage(item);
				break;
			}
		}

		hasPlugins = (model->rowCount() > 0);
	}

	if (!hasPlugins) {
		fputs("*** " RP_KDE_UPPER " rp_show_RomDataView_dialog(): No tabs were created; exiting.\n", stderr);
		return EXIT_FAILURE;
	}

	// Run the Qt UI.
	// FIXME: May need changes if the main loop is already running.
	fputs("*** " RP_KDE_UPPER " rp_show_RomDataView_dialog(): Starting main loop.\n", stderr);
	return app->exec();
}
