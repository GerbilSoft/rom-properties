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
// TODO: Also localize rp-stub?
// (Will require moving gettext out of librpbase.)
#include "librpbase/i18n.hpp"

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
	}

#if defined(ENABLE_NLS) && defined(HAVE_GETTEXT)
	// TODO: Better location for this?
#ifdef DIR_INSTALL_LOCALE
	rp_i18n_init(DIR_INSTALL_LOCALE);
#else
	// Use the current directory.
	// TODO: On Windows, use the DLL directory.
	rp_i18n_init("locale");
#endif
#endif /* ENABLE_NLS && HAVE_GETTEXT */

	// Create and run the ConfigDialog.
	// TODO: Get the return value?
	ConfigDialog *cfg = new ConfigDialog();
	cfg->show();

	// Run the Qt UI.
	// FIXME: May need changes if the main loop is already running.
	return rpApp->exec();
}
