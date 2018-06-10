/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * RpExtractorPlugin.cpp: KFileMetaData forwarder.                         *
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

#include "RpExtractorPlugin.hpp"

// KDE includes.
#include <kfilemetadata/extractorplugin.h>
#include <kfilemetadata/properties.h>
using KFileMetaData::ExtractorPlugin;
using KFileMetaData::ExtractionResult;
using namespace KFileMetaData::Property;

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <kfileitem.h>

namespace RomPropertiesKDE {

/**
 * Factory method.
 * NOTE: Unlike the ThumbCreator version, this one is specific to
 * rom-properties, and is called by a forwarder library.
 */
extern "C" {
	Q_DECL_EXPORT RpExtractorPlugin *PFN_CREATEEXTRACTORPLUGINKDE_FN(QObject *parent)
	{
		return new RpExtractorPlugin(parent);
	}
}

RpExtractorPlugin::RpExtractorPlugin(QObject *parent)
	: super(parent)
{
	// TODO: Create an RpExtractorPlugin class and retrieve
	// it from the rom-properties-kde5.so library.
}

QStringList RpExtractorPlugin::mimetypes(void) const
{
	// TODO: Get a list of MIME types from RomDataFactory.
	// Hard-coding ADX for now.
	QStringList mimeList;
	mimeList += QLatin1String("audio/x-adx");
	return mimeList;
}

void RpExtractorPlugin::extract(ExtractionResult *result)
{
	result->add(Property::Duration, 1234);
	return;

	// TODO: Use KIO to support network URLs.
	// For now, require a local path.
	KFileItem item(QUrl(result->inputUrl()));
	QString filename = item.localPath();
	if (filename.isEmpty())
		return;

	return;
}

}

#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
