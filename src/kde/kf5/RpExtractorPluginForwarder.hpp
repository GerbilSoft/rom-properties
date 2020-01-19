/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * RpExtractorPluginForwarder.hpp: KFileMetaData extractor forwarder.      *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KFileMetaData ExtractorPlugin, *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_EXTRACTORPLUGINFORWARDER_HPP__
#define __ROMPROPERTIES_KDE_EXTRACTORPLUGINFORWARDER_HPP__

// FIXME: Test on KDE4.
#include <QtCore/qglobal.h>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <kfilemetadata/extractorplugin.h>

namespace RomPropertiesKDE {

class RpExtractorPluginForwarder : public KFileMetaData::ExtractorPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.kde.kf5.kfilemetadata.ExtractorPlugin")
	Q_INTERFACES(KFileMetaData::ExtractorPlugin)

	public:
		explicit RpExtractorPluginForwarder(QObject *parent = nullptr);
		virtual ~RpExtractorPluginForwarder();

	private:
		typedef KFileMetaData::ExtractorPlugin super;
		Q_DISABLE_COPY(RpExtractorPluginForwarder);

	public:
		QStringList mimetypes(void) const final;
		void extract(KFileMetaData::ExtractionResult *result) final;

	private:
		// rom-properties-kf5.so handle.
		void *hRpKdeSo;

		// Actual ExtractorPlugin.
		KFileMetaData::ExtractorPlugin *fwd_plugin;

	private slots:
		/**
		 * fwd_plugin was destroyed.
		 * @param obj
		 */
		void fwd_plugin_destroyed(QObject *obj = nullptr);
};

}

#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

#endif /* __ROMPROPERTIES_KDE_EXTRACTORPLUGINFORWARDER_HPP__ */
