/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * RpExtractorPlugin.hpp: KFileMetaData extractor plugin.                  *
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

#ifndef __ROMPROPERTIES_KDE_RPEXTRACTORPLUGIN_HPP__
#define __ROMPROPERTIES_KDE_RPEXTRACTORPLUGIN_HPP__

// FIXME: Test on KDE4.
#include <QtCore/qglobal.h>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <kfilemetadata/extractorplugin.h>

namespace RomPropertiesKDE {

class RpExtractorPlugin : public KFileMetaData::ExtractorPlugin
{
	Q_OBJECT
	Q_INTERFACES(KFileMetaData::ExtractorPlugin)

	public:
		explicit RpExtractorPlugin(QObject *parent = nullptr);

	private:
		typedef KFileMetaData::ExtractorPlugin super;
		Q_DISABLE_COPY(RpExtractorPlugin);

	public:
		QStringList mimetypes(void) const final;
		void extract(KFileMetaData::ExtractionResult *result) final;
};

// Exported function pointer to create a new RpExtractorPlugin.
typedef RpExtractorPlugin* (*pfn_createExtractorPluginKDE_t)(QObject *parent);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
# error Qt6 is not supported.
#elif QT_VERSION >= QT_VERSION_CHECK(5,0,0)
# define PFN_CREATEEXTRACTORPLUGINKDE_FN createExtractorPluginKDE5
# define PFN_CREATEEXTRACTORPLUGINKDE_NAME "createExtractorPluginKDE5"
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
# define PFN_CREATEEXTRACTORPLUGINKDE_FN createExtractorPluginKDE4
# define PFN_CREATEEXTRACTORPLUGINKDE_NAME "createExtractorPluginKDE4"
#endif

}

#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

#endif /* __ROMPROPERTIES_KDE_RPEXTRACTORPLUGIN_HPP__ */
