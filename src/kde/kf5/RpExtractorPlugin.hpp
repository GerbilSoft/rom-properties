/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * RpExtractorPlugin.hpp: KFileMetaData extractor plugin.                  *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KFileMetaData ExtractorPlugin, *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_RPEXTRACTORPLUGIN_HPP__
#define __ROMPROPERTIES_KDE_RPEXTRACTORPLUGIN_HPP__

// FIXME: Test on KDE4.
#include <QtCore/qglobal.h>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <kfilemetadata/extractorplugin.h>

namespace RomPropertiesKDE {

class RpExtractorPlugin final : public KFileMetaData::ExtractorPlugin
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
# define PFN_CREATEEXTRACTORPLUGINKDE_FN createExtractorPluginKF5
# define PFN_CREATEEXTRACTORPLUGINKDE_NAME "createExtractorPluginKF5"
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
# define PFN_CREATEEXTRACTORPLUGINKDE_FN createExtractorPluginKDE4
# define PFN_CREATEEXTRACTORPLUGINKDE_NAME "createExtractorPluginKDE4"
#endif

}

#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

#endif /* __ROMPROPERTIES_KDE_RPEXTRACTORPLUGIN_HPP__ */
