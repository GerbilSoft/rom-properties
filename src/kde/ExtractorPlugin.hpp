/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ExtractorPlugin.hpp: KFileMetaData extractor plugin.                    *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KFileMetaData ExtractorPlugin, *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <QtCore/qglobal.h>
#include <kcoreaddons_version.h>
#include <kfilemetadata/extractorplugin.h>

#include "RpQt.hpp"

#define PFN_CREATEEXTRACTORPLUGINKDE_FN CONCAT_FN(createExtractorPlugin, RP_KDE_SUFFIX)
#define PFN_CREATEEXTRACTORPLUGINKDE_NAME "createExtractorPlugin" RP_KDE_UPPER

namespace LibRpBase {
	class RomData;
}

namespace RomPropertiesKDE {

class ExtractorPlugin : public ::KFileMetaData::ExtractorPlugin
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

	private:
		static void extract_properties(KFileMetaData::ExtractionResult *result, LibRpBase::RomData *romData);
#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5,76,0)
		static void extract_image(KFileMetaData::ExtractionResult *result, LibRpBase::RomData *romData);
#endif /* KCOREADDONS_VERSION <= QT_VERSION_CHECK(5,76,0) */
	public:
		void extract(KFileMetaData::ExtractionResult *result) final;
};

// Exported function pointer to create a new RpExtractorPlugin.
typedef ExtractorPlugin* (*pfn_createExtractorPluginKDE_t)(QObject *parent);

}
