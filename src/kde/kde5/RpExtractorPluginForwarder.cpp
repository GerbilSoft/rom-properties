/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * RpExtractorPluginForwarder.cpp: KFileMetaData extractor forwarder.      *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KFileMetaData ExtractorPlugin, *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpExtractorPluginForwarder.hpp"
#include "RpExtractorPlugin.hpp"

// C includes.
#include <dlfcn.h>

// KDE includes.
#include <kfilemetadata/extractorplugin.h>
using KFileMetaData::ExtractorPlugin;
using KFileMetaData::ExtractionResult;

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <kfileitem.h>

namespace RomPropertiesKDE {

RpExtractorPluginForwarder::RpExtractorPluginForwarder(QObject *parent)
	: super(parent)
	, hRpKdeSo(nullptr)
	, fwd_plugin(nullptr)
{
#ifndef PLUGIN_INSTALL_DIR
# error PLUGIN_INSTALL_DIR is not set.
#endif
	// FIXME: Check the .desktop file?
	QString pluginPath(QString::fromUtf8(PLUGIN_INSTALL_DIR));
	pluginPath += QLatin1String("/rom-properties-kde5.so");

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

	// Create an RpExtractorPlugin object.
	fwd_plugin = pfn(this);
	if (!fwd_plugin) {
		// Unable to create an RpExtractorPlugin object.
		dlclose(hRpKdeSo);
		hRpKdeSo = nullptr;
		return;
	}

	// Make sure we know if the ExtractorPlugin gets deleted.
	// This *shouldn't* happen, but it's possible that our parent
	// object enumerates child objects and does weird things.
	connect(fwd_plugin, SIGNAL(destroyed(QObject*)),
		this, SLOT(fwd_plugin_destroyed(QObject*)));
}

RpExtractorPluginForwarder::~RpExtractorPluginForwarder()
{
	delete fwd_plugin;

	// FIXME: What does dlclose(nullptr) do?
	if (hRpKdeSo) {
		dlclose(hRpKdeSo);
	}
}

QStringList RpExtractorPluginForwarder::mimetypes(void) const
{
	if (fwd_plugin) {
		return fwd_plugin->mimetypes();
	}
	return QStringList();
}

void RpExtractorPluginForwarder::extract(ExtractionResult *result)
{
	if (fwd_plugin) {
		fwd_plugin->extract(result);
	}
}

/**
 * fwd_plugin was destroyed.
 * @param obj
 */
void RpExtractorPluginForwarder::fwd_plugin_destroyed(QObject *obj)
{
	if (obj == fwd_plugin) {
		// Object matches.
		// NULL it out so we don't have problems later.
		fwd_plugin = nullptr;
	}
}

}

#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
