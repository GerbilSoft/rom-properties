/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * stub-export.cpp: Exported function for the rp-config stub.              *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "check-uid.hpp"
#include "ConfigDialog.hpp"

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

	QApplication *rpApp = qApp;
	if (!rpApp) {
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
		rpApp = new QApplication(argc, argv);

#ifdef ENABLE_NLS
		// Install the translator for Gettext translations.
		rp_i18n_init();
		rpApp->installTranslator(new GettextTranslator());
#endif /* ENABLE_NLS */
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
	return rpApp->exec();
}
