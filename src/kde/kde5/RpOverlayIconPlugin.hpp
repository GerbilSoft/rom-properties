/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * RpOverlayIconPlugin.hpp: KOverlayIconPlugin.                            *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KOverlayIconPlugin,            *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_KDE_RPOVERLAYICONPLUGIN_HPP__
#define __ROMPROPERTIES_KDE_RPOVERLAYICONPLUGIN_HPP__

// FIXME: Test on KDE4.
#include <QtCore/qglobal.h>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <KOverlayIconPlugin>

namespace RomPropertiesKDE {

class RpOverlayIconPlugin : public KOverlayIconPlugin
{
	Q_OBJECT
	//Q_INTERFACES(KOverlayIconPlugin)

	public:
		explicit RpOverlayIconPlugin(QObject *parent = nullptr);

	private:
		typedef KOverlayIconPlugin super;
		Q_DISABLE_COPY(RpOverlayIconPlugin);

	public:
		QStringList getOverlays(const QUrl &item) final;
};

// Exported function pointer to create a new RpExtractorPlugin.
typedef RpOverlayIconPlugin* (*pfn_createOverlayIconPluginKDE_t)(QObject *parent);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
# error Qt6 is not supported.
#elif QT_VERSION >= QT_VERSION_CHECK(5,0,0)
# define PFN_CREATEOVERLAYICONPLUGINKDE_FN createOverlayIconPluginKDE5
# define PFN_CREATEOVERLAYICONPLUGINKDE_NAME "createOverlayIconPluginKDE5"
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
# define PFN_CREATEOVERLAYICONPLUGINKDE_FN createOverlayIconPluginKDE4
# define PFN_CREATEOVERLAYICONPLUGINKDE_NAME "createOverlayIconPluginKDE4"
#endif

}

#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

#endif /* __ROMPROPERTIES_KDE_RPOVERLAYICONPLUGIN_HPP__ */
