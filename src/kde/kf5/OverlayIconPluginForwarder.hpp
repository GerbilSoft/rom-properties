/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * OverlayIconPluginForwarder.hpp: KOverlayIconPlugin forwarder.           *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KOverlayIconPlugin,            *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_KF5_OVERLAYICONPLUGINFORWARDER_HPP__
#define __ROMPROPERTIES_KDE_OVERLAYICONPLUGINFORWARDER_HPP__

#include <QtCore/qglobal.h>
#include <KOverlayIconPlugin>

namespace RomPropertiesKF5 {

class OverlayIconPluginForwarder final : public KOverlayIconPlugin
{
	Q_OBJECT
	// NOTE: KDE doesn't have a standard IID for KOverlayIconPlugin...
	Q_PLUGIN_METADATA(IID "com.gerbilsoft.rom-properties.KOverlayIconPlugin" FILE "OverlayIconPluginForwarder.json")
	//Q_INTERFACES(KOverlayIconPlugin)

	public:
		explicit OverlayIconPluginForwarder(QObject *parent = nullptr);
		virtual ~OverlayIconPluginForwarder();

	private:
		typedef KOverlayIconPlugin super;
		Q_DISABLE_COPY(OverlayIconPluginForwarder);

	public:
		QStringList getOverlays(const QUrl &item) final;

	private:
		// rom-properties-kf5.so handle.
		void *hRpKdeSo;

		// Actual OverlayPlugin.
		KOverlayIconPlugin *fwd_plugin;

	private slots:
		/**
		 * fwd_plugin was destroyed.
		 * @param obj
		 */
		void fwd_plugin_destroyed(QObject *obj = nullptr);
};

}

#endif /* __ROMPROPERTIES_KDE_OVERLAYICONPLUGINFORWARDER_HPP__ */
