/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * stub-export.cpp: Exported function for the rp-config stub.              *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ConfigDialog.hpp"
#include "check-uid.hpp"

// Program version
#include "librpbase/config/AboutTabText.hpp"
using namespace LibRpBase;

// i18n
#ifdef ENABLE_NLS
#  include "../GettextTranslator.hpp"
#endif

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

	QApplication *app = qApp;
	if (!app) {
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
		app->setApplicationName(QLatin1String("rp-config"));
		app->setOrganizationDomain(QLatin1String("gerbilsoft.com"));
		app->setOrganizationName(QLatin1String("GerbilSoft"));
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		app->setApplicationDisplayName(QCoreApplication::translate(
			"ConfigDialog", "ROM Properties Page configuration", nullptr));
#  if QT_VERSION >= QT_VERSION_CHECK(5,7,0)
		app->setDesktopFileName(QLatin1String("com.gerbilsoft.rom-properties.rp-config.desktop"));
#  endif /* QT_VERSION >= QT_VERSION_CHECK(5,7,0) */
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

		const char *const programVersion =
			AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::ProgramVersion);
		assert(programVersion != nullptr);
		if (programVersion) {
			app->setApplicationVersion(QLatin1String(programVersion));
		}
	} else {
		// Initialize base i18n.
		// TODO: Install the translator even if we're reusing the QApplication?
		rp_i18n_init();
	}

	// Create and run the ConfigDialog.
	// TODO: Get the return value?
	ConfigDialog *const configDialog = new ConfigDialog();
	configDialog->setObjectName(QLatin1String("configDialog"));
	configDialog->show();

	// Run the Qt UI.
	// FIXME: May need changes if the main loop is already running.
	return app->exec();
}
