/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * OverlayIconPlugin.hpp: KOverlayIconPlugin.                              *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KOverlayIconPlugin,            *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <QtCore/qglobal.h>
#include <KOverlayIconPlugin>

#include "RpQt.hpp"

#define PFN_CREATEOVERLAYICONPLUGINKDE_FN CONCAT_FN(createOverlayIconPlugin, RP_KDE_SUFFIX)
#define PFN_CREATEOVERLAYICONPLUGINKDE_NAME "createOverlayIconPlugin" RP_KDE_UPPER

namespace RomPropertiesKDE {

class OverlayIconPlugin : public ::KOverlayIconPlugin
{
Q_OBJECT
//Q_INTERFACES(KOverlayIconPlugin)

public:
	explicit OverlayIconPlugin(QObject *parent = nullptr);

private:
	typedef KOverlayIconPlugin super;
	Q_DISABLE_COPY(OverlayIconPlugin);

public:
	QStringList getOverlays(const QUrl &item) final;
};

// Exported function pointer to create a new RpExtractorPlugin.
typedef OverlayIconPlugin* (*pfn_createOverlayIconPluginKDE_t)(QObject *parent);

} //namespace RomPropertiesKDE
