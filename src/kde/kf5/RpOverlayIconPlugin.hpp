/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * RpOverlayIconPlugin.hpp: KOverlayIconPlugin.                            *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KOverlayIconPlugin,            *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
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
# define PFN_CREATEOVERLAYICONPLUGINKDE_FN createOverlayIconPluginKF5
# define PFN_CREATEOVERLAYICONPLUGINKDE_NAME "createOverlayIconPluginKF5"
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
# define PFN_CREATEOVERLAYICONPLUGINKDE_FN createOverlayIconPluginKDE4
# define PFN_CREATEOVERLAYICONPLUGINKDE_NAME "createOverlayIconPluginKDE4"
#endif

}

#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

#endif /* __ROMPROPERTIES_KDE_RPOVERLAYICONPLUGIN_HPP__ */
