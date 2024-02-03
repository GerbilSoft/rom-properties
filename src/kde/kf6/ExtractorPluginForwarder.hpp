/***************************************************************************
 * ROM Properties Page shell extension. (KF6)                              *
 * ExtractorPluginForwarder.hpp: KFileMetaData extractor forwarder.        *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KFileMetaData ExtractorPlugin, *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <QtCore/qglobal.h>
#include <QtCore/QObject>	// missing in KF6 extractorplugin.h?
#include <kfilemetadata/extractorplugin.h>

namespace RomPropertiesKF6 {

class ExtractorPluginForwarder final : public ::KFileMetaData::ExtractorPlugin
{
Q_OBJECT
// FIXME: "kf5" -> "kf6" once it's updated upstream.
// https://invent.kde.org/frameworks/kfilemetadata/-/blob/master/src/extractorplugin.h
Q_PLUGIN_METADATA(IID "org.kde.kf5.kfilemetadata.ExtractorPlugin" FILE "ExtractorPluginForwarder.json")
Q_INTERFACES(KFileMetaData::ExtractorPlugin)

public:
	explicit ExtractorPluginForwarder(QObject *parent = nullptr);
	~ExtractorPluginForwarder() final;

private:
	typedef KFileMetaData::ExtractorPlugin super;
	Q_DISABLE_COPY(ExtractorPluginForwarder);

public:
	QStringList mimetypes(void) const final;
	void extract(KFileMetaData::ExtractionResult *result) final;

private:
	// rom-properties-kf5.so handle
	void *hRpKdeSo;

	// Actual ExtractorPlugin
	KFileMetaData::ExtractorPlugin *fwd_plugin;
};

} //namespace RomPropertiesKF6
