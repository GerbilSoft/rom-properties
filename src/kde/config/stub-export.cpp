/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * stub-export.cpp: Exported function for the rp-config stub.              *
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

#include "ConfigDialog.hpp"

// Qt includes.
#include <QApplication>

// i18n
#include "libi18n/i18n.h"
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
Q_DECL_EXPORT int rp_show_config_dialog(int argc, char *argv[])
{
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
		QApplication::setAttribute((Qt::ApplicationAttribute)13, true);
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
