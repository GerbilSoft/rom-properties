/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * stub-export.cpp: Exported function for the rp-config stub.              *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ConfigDialog.hpp"

// i18n
#ifdef ENABLE_NLS
# include "../GettextTranslator.hpp"
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
	if (getuid() == 0 || geteuid() == 0) {
		qCritical("*** rom-properties-" RP_KDE_LOWER "%u does not support running as root.", QT_VERSION >> 16);
		return EXIT_FAILURE;
	}

	QApplication *rpApp = qApp;
	if (!rpApp) {
#if QT_VERSION >= 0x050000
		// Enable High DPI.
		QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#if QT_VERSION >= 0x050600
		// Enable High DPI pixmaps.
		QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
#else
		// Hardcode the value in case the user upgrades to Qt 5.6 later.
		// http://doc.qt.io/qt-5/qt.html#ApplicationAttribute-enum
		QApplication::setAttribute(static_cast<Qt::ApplicationAttribute>(13), true);
#endif /* QT_VERSION >= 0x050600 */
#endif /* QT_VERSION >= 0x050000 */
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
	ConfigDialog *cfg = new ConfigDialog();
	cfg->show();

	// Run the Qt UI.
	// FIXME: May need changes if the main loop is already running.
	return rpApp->exec();
}
