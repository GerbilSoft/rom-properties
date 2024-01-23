/***************************************************************************
 * ROM Properties Page shell extension. (KF6)                              *
 * OverlayIconPluginForwarder.cpp: KOverlayIconPlugin forwarder.           *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KOverlayIconPlugin,            *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.kf6.h"
#include "../RpQt.hpp"
#include "../check-uid.hpp"

#include "OverlayIconPluginForwarder.hpp"
#include "../OverlayIconPlugin.hpp"

// C includes.
#include <dlfcn.h>

// KDE includes.
#include <kfileitem.h>
#include <KOverlayIconPlugin>

#ifndef KF6_PRPD_PLUGIN_INSTALL_DIR
#  error KF6_PRPD_PLUGIN_INSTALL_DIR is not set.
#endif /* KF6_PRPD_PLUGIN_INSTALL_DIR */

namespace RomPropertiesKF6 {

OverlayIconPluginForwarder::OverlayIconPluginForwarder(QObject *parent)
	: super(parent)
	, hRpKdeSo(nullptr)
	, fwd_plugin(nullptr)
{
	CHECK_UID();

	QString pluginPath(QString::fromUtf8(KF6_PRPD_PLUGIN_INSTALL_DIR));
	pluginPath += QLatin1String("/rom-properties-kf6.so");

	// Attempt to load the plugin.
	hRpKdeSo = dlopen(pluginPath.toUtf8().constData(), RTLD_LOCAL|RTLD_LAZY);
	if (!hRpKdeSo) {
		// Unable to open the plugin.
		// NOTE: We can't use mismatched plugins here.
		return;
	}

	// Load the symbol.
	pfn_createOverlayIconPluginKDE_t pfn = reinterpret_cast<pfn_createOverlayIconPluginKDE_t>(
		dlsym(hRpKdeSo, PFN_CREATEOVERLAYICONPLUGINKDE_NAME));
	if (!pfn) {
		// Symbol not found.
		dlclose(hRpKdeSo);
		hRpKdeSo = nullptr;
		return;
	}

	// Create an OverlayIconPlugin object.
	fwd_plugin = pfn(this);
	if (fwd_plugin.isNull()) {
		// Unable to create an OverlayIconPlugin object.
		dlclose(hRpKdeSo);
		hRpKdeSo = nullptr;
		return;
	}
}

OverlayIconPluginForwarder::~OverlayIconPluginForwarder()
{
	delete fwd_plugin;

	// NOTE: dlclose(nullptr) may crash, so we have to check for nullptr.
	if (hRpKdeSo) {
		dlclose(hRpKdeSo);
	}
}

QStringList OverlayIconPluginForwarder::getOverlays(const QUrl &item)
{
	if (!fwd_plugin.isNull()) {
		return fwd_plugin->getOverlays(item);
	}
	return {};
}

} //namespace RomPropertiesKF6
