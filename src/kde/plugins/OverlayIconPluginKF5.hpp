/***************************************************************************
 * ROM Properties Page shell extension. (KF5)                              *
 * OverlayIconPluginKF5.hpp: KOverlayIconPlugin.                           *
 *                                                                         *
 * NOTE: This file is compiled as a separate .so file. Originally, a       *
 * forwarder plugin was used, since Qt's plugin system prevents a single   *
 * shared library from exporting multiple plugins, but as of RP 2.0,       *
 * most of the important code is split out into libromdata.so, so the      *
 * forwarder version is unnecessary.                                       *
 *                                                                         *
 * Copyright (c) 2018-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <QtCore/qglobal.h>
#include <KOverlayIconPlugin>

#include "RpQtNS.hpp"

namespace RomPropertiesKDE {

class OverlayIconPlugin : public ::KOverlayIconPlugin
{
Q_OBJECT
//Q_INTERFACES(KOverlayIconPlugin)

// NOTE: KF5 doesn't have a standard IID for KOverlayIconPlugin...
Q_PLUGIN_METADATA(IID "com.gerbilsoft.rom-properties.KOverlayIconPlugin" FILE "../kf5/OverlayIconPlugin.json")

public:
	explicit OverlayIconPlugin(QObject *parent = nullptr);

private:
	typedef KOverlayIconPlugin super;
	Q_DISABLE_COPY(OverlayIconPlugin);

public:
	QStringList getOverlays(const QUrl &item) final;
};

} //namespace RomPropertiesKDE
