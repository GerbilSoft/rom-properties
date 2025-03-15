/***************************************************************************
 * ROM Properties Page shell extension. (KF6)                              *
 * ExtractorPluginKF6.hpp: KFileMetaData extractor plugin.                 *
 *                                                                         *
 * NOTE: This file is compiled as a separate .so file. Originally, a       *
 * forwarder plugin was used, since Qt's plugin system prevents a single   *
 * shared library from exporting multiple plugins, but as of RP 2.0,       *
 * most of the important code is split out into libromdata.so, so the      *
 * forwarder version is unnecessary.                                       *
 *                                                                         *
 * Copyright (c) 2018-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <kcoreaddons_version.h>
#include <kfilemetadata/extractorplugin.h>

#include "RpQtNS.hpp"

// librpbase
namespace LibRpBase {
	class RomData;
}

namespace RomPropertiesKDE {

class ExtractorPlugin : public ::KFileMetaData::ExtractorPlugin
{
Q_OBJECT
Q_INTERFACES(KFileMetaData::ExtractorPlugin)
Q_PLUGIN_METADATA(IID "org.kde.kf5.kfilemetadata.ExtractorPlugin" FILE "../kf6/ExtractorPlugin.json")

public:
	explicit ExtractorPlugin(QObject *parent = nullptr);

private:
	typedef KFileMetaData::ExtractorPlugin super;
	Q_DISABLE_COPY(ExtractorPlugin);

public:
	QStringList mimetypes(void) const final;

private:
	static void extract_properties(KFileMetaData::ExtractionResult *result, LibRpBase::RomData *romData);
#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5, 76, 0)
	static void extract_image(KFileMetaData::ExtractionResult *result, LibRpBase::RomData *romData);
#endif /* KCOREADDONS_VERSION <= QT_VERSION_CHECK(5, 76, 0) */
public:
	void extract(KFileMetaData::ExtractionResult *result) final;
};

} //namespace RomPropertiesKDE
