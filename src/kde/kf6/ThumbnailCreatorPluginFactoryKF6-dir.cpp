/***********************************************************************************
 * ROM Properties Page shell extension. (KF6)                                      *
 * ThumbnailCreatorPluginFactoryKF6-dir.cpp: ThumbnailCreator plugin factory class *
 * "inode/directory" version.                                                      *
 *                                                                                 *
 * Copyright (c) 2016-2026 by David Korth.                                         *
 * SPDX-License-Identifier: GPL-2.0-or-later                                       *
 ***********************************************************************************/

/**
 * References:
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.h
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.cpp
 * - https://github.com/KDE/calligra-history/blob/master/libs/main/KoDocInfoPropsFactory.cpp
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/kodocinfopropspage.desktop
 */

#define RP_KDE_DISABLE_REGISTER_ACHQTDBUS 1
#include "kde_register_backends.hpp"

// Plugins
#include "plugins/RomThumbnailCreator.hpp"

// KDE Frameworks
#include <kpluginfactory.h>

K_PLUGIN_FACTORY_WITH_JSON(RomThumbnailCreatorDirFactory, "RomThumbnailCreator-dir.json",
	kde_register_backends();
	registerPlugin<RomThumbnailCreatorDir>();
)

// automoc4 works correctly without any special handling.
// automoc5 doesn't notice that K_PLUGIN_FACTORY() has a
// Q_OBJECT macro, so it needs a manual .moc include.
// That .moc include trips up automoc4, even if it's #ifdef'd.
// Hence, we need separate files for KDE4, KF5, and KF6.
#include "ThumbnailCreatorPluginFactoryKF6-dir.moc"
