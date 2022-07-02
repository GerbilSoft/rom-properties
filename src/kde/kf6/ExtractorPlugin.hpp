/***************************************************************************
 * ROM Properties Page shell extension. (KF6)                              *
 * ExtractorPlugin.hpp: KFileMetaData extractor plugin.                    *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KFileMetaData ExtractorPlugin, *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_KF6_EXTRACTORPLUGIN_HPP__
#define __ROMPROPERTIES_KDE_KF6_EXTRACTORPLUGIN_HPP__

#include <QtCore/qglobal.h>
#include <kfilemetadata/extractorplugin.h>

namespace RomPropertiesKF6 {

class ExtractorPlugin final : public ::KFileMetaData::ExtractorPlugin
{
	Q_OBJECT
	Q_INTERFACES(KFileMetaData::ExtractorPlugin)

	public:
		explicit ExtractorPlugin(QObject *parent = nullptr);

	private:
		typedef KFileMetaData::ExtractorPlugin super;
		Q_DISABLE_COPY(ExtractorPlugin);

	public:
		QStringList mimetypes(void) const final;
		void extract(KFileMetaData::ExtractionResult *result) final;
};

// Exported function pointer to create a new RpExtractorPlugin.
typedef ExtractorPlugin* (*pfn_createExtractorPluginKDE_t)(QObject *parent);
#define PFN_CREATEEXTRACTORPLUGINKDE_FN createExtractorPluginKF6
#define PFN_CREATEEXTRACTORPLUGINKDE_NAME "createExtractorPluginKF6"

}

#endif /* __ROMPROPERTIES_KDE_KF6_EXTRACTORPLUGIN_HPP__ */
