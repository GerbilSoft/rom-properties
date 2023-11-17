/***************************************************************************
 * ROM Properties Page shell extension. (KF5)                              *
 * PluginFactoryKF5.cpp: Plugin factory class.                             *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.h
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.cpp
 * - https://github.com/KDE/calligra-history/blob/master/libs/main/KoDocInfoPropsFactory.cpp
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/kodocinfopropspage.desktop
 */

#include "stdafx.h"
#include "config.kde.h"

// RpQImageBackend
#include "RpQImageBackend.hpp"
using LibRpTexture::rp_image;

// Achievements backend
#include "AchQtDBus.hpp"

// Plugins
#include "RomPropertiesDialogPlugin.hpp"
#include "RomThumbCreator.hpp"

// KDE Frameworks
#include <kpluginfactory.h>

static void register_backends(void)
{
	// Register RpQImageBackend and AchQtDBus.
	rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);
#if defined(ENABLE_ACHIEVEMENTS) && defined(HAVE_QtDBus_NOTIFY)
	AchQtDBus::instance();
#endif /* ENABLE_ACHIEVEMENTS && HAVE_QtDBus_NOTIFY */
}

K_PLUGIN_FACTORY_WITH_JSON(RomPropertiesDialogFactory, "rom-properties-kf5.json",
	register_backends();
	registerPlugin<RomPropertiesDialogPlugin>();
#ifdef HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H
	registerPlugin<RomThumbnailCreator>();
#endif /* HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H */
)

// automoc4 works correctly without any special handling.
// automoc5 doesn't notice that K_PLUGIN_FACTORY() has a
// Q_OBJECT macro, so it needs a manual .moc include.
// That .moc include trips up automoc4, even if it's #ifdef'd.
// Hence, we need separate files for KDE4, KF5, and KF6.
#include "PluginFactoryKF5.moc"
