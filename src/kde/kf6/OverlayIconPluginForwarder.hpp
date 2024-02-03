/***************************************************************************
 * ROM Properties Page shell extension. (KF6)                              *
 * OverlayIconPluginForwarder.hpp: KOverlayIconPlugin forwarder.           *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KOverlayIconPlugin,            *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <QtCore/qglobal.h>
#include <KOverlayIconPlugin>

namespace RomPropertiesKF6 {

class OverlayIconPluginForwarder final : public KOverlayIconPlugin
{
Q_OBJECT
// NOTE: KF5 doesn't have a standard IID for KOverlayIconPlugin...
// TODO: Check KF6.
Q_PLUGIN_METADATA(IID "com.gerbilsoft.rom-properties.KOverlayIconPlugin" FILE "OverlayIconPluginForwarder.json")
//Q_INTERFACES(KOverlayIconPlugin)

public:
	explicit OverlayIconPluginForwarder(QObject *parent = nullptr);
	~OverlayIconPluginForwarder() final;

private:
	typedef KOverlayIconPlugin super;
	Q_DISABLE_COPY(OverlayIconPluginForwarder);

public:
	QStringList getOverlays(const QUrl &item) final;

private:
	// rom-properties-kf6.so handle
	void *hRpKdeSo;

	// Actual OverlayPlugin
	KOverlayIconPlugin *fwd_plugin;
};

} //namespace RomPropertiesKF6
