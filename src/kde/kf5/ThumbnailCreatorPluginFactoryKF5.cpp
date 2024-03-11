/*******************************************************************************
 * ROM Properties Page shell extension. (KF5)                                  *
 * ThumbnailCreatorPluginFactoryKF5.cpp: ThumbnailCreator plugin factory class *
 *                                                                             *
 * Copyright (c) 2016-2024 by David Korth.                                     *
 * SPDX-License-Identifier: GPL-2.0-or-later                                   *
 *******************************************************************************/

/**
 * References:
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.h
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.cpp
 * - https://github.com/KDE/calligra-history/blob/master/libs/main/KoDocInfoPropsFactory.cpp
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/kodocinfopropspage.desktop
 */

#include "stdafx.h"

// RpQImageBackend
#include "RpQImageBackend.hpp"
using LibRpTexture::rp_image;

// Plugins
#include "plugins/RomThumbnailCreator.hpp"

// KDE Frameworks
#include <kcoreaddons_version.h>
#include <kpluginfactory.h>

static void register_backends(void)
{
	// Register RpQImageBackend and AchQtDBus.
	rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);
}

K_PLUGIN_FACTORY_WITH_JSON(RomThumbnailCreatorFactory, "RomThumbnailCreator.json",
	register_backends();
	registerPlugin<RomThumbnailCreator>();
)

// automoc4 works correctly without any special handling.
// automoc5 doesn't notice that K_PLUGIN_FACTORY() has a
// Q_OBJECT macro, so it needs a manual .moc include.
// That .moc include trips up automoc4, even if it's #ifdef'd.
// Hence, we need separate files for KDE4, KF5, and KF6.
#include "ThumbnailCreatorPluginFactoryKF5.moc"
