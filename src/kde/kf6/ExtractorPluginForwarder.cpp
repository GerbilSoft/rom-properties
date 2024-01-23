/***************************************************************************
 * ROM Properties Page shell extension. (KF6)                              *
 * ExtractorPluginForwarder.cpp: KFileMetaData extractor forwarder.        *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KFileMetaData ExtractorPlugin, *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.kf6.h"
#include "../RpQt.hpp"
#include "../check-uid.hpp"

#include "ExtractorPluginForwarder.hpp"
#include "../ExtractorPlugin.hpp"

// C includes.
#include <dlfcn.h>

// KDE includes.
#define SO_FILENAME "rom-properties-kf6.so"
#include <kfileitem.h>
#include <kfilemetadata/extractorplugin.h>
using KFileMetaData::ExtractionResult;

#ifndef KF6_PRPD_PLUGIN_INSTALL_DIR
#  error KF6_PRPD_PLUGIN_INSTALL_DIR is not set.
#endif

namespace RomPropertiesKF6 {

ExtractorPluginForwarder::ExtractorPluginForwarder(QObject *parent)
	: super(parent)
	, hRpKdeSo(nullptr)
{
	CHECK_UID();

	QString pluginPath(QString::fromUtf8(KF6_PRPD_PLUGIN_INSTALL_DIR));
	pluginPath += QLatin1String("/" SO_FILENAME);

	// Attempt to load the plugin.
	hRpKdeSo = dlopen(pluginPath.toUtf8().constData(), RTLD_LOCAL|RTLD_LAZY);
	if (!hRpKdeSo) {
		// Unable to open the plugin.
		// NOTE: We can't use mismatched plugins here.
		return;
	}

	// Load the symbol.
	pfn_createExtractorPluginKDE_t pfn = reinterpret_cast<pfn_createExtractorPluginKDE_t>(
		dlsym(hRpKdeSo, PFN_CREATEEXTRACTORPLUGINKDE_NAME));
	if (!pfn) {
		// Symbol not found.
		dlclose(hRpKdeSo);
		hRpKdeSo = nullptr;
		return;
	}

	// Create an ExtractorPlugin object.
	fwd_plugin = pfn(this);
	if (fwd_plugin.isNull()) {
		// Unable to create an ExtractorPlugin object.
		dlclose(hRpKdeSo);
		hRpKdeSo = nullptr;
		return;
	}
}

ExtractorPluginForwarder::~ExtractorPluginForwarder()
{
	fwd_plugin.clear();

	// NOTE: dlclose(nullptr) may crash, so we have to check for nullptr.
	if (hRpKdeSo) {
		dlclose(hRpKdeSo);
	}
}

QStringList ExtractorPluginForwarder::mimetypes(void) const
{
	if (!fwd_plugin.isNull()) {
		return fwd_plugin->mimetypes();
	}
	return {};
}

void ExtractorPluginForwarder::extract(ExtractionResult *result)
{
	if (!fwd_plugin.isNull()) {
		fwd_plugin->extract(result);
	}
}

} //namespace RomPropertiesKF6
