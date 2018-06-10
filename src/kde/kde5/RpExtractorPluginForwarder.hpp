/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * RpExtractorPluginForwarder.hpp: KFileMetaData extractor forwarder.      *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KFileMetaData ExtractorPlugin, *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
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
		// rom-properties-kde5.so handle.
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
