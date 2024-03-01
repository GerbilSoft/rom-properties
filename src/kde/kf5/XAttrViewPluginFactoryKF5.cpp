/***************************************************************************
 * ROM Properties Page shell extension. (KF5)                              *
 * XAttrViewPluginFactoryKF5.cpp: XAttrView plugin factory class           *
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

// Plugins
#include "xattr/XAttrViewPropertiesDialogPlugin.hpp"

// KDE Frameworks
#include <kcoreaddons_version.h>
#include <kpluginfactory.h>

K_PLUGIN_FACTORY_WITH_JSON(XAttrViewPropertiesDialogFactory, "xattrview-kf5.json",
	registerPlugin<XAttrViewPropertiesDialogPlugin>();
)

// automoc4 works correctly without any special handling.
// automoc5 doesn't notice that K_PLUGIN_FACTORY() has a
// Q_OBJECT macro, so it needs a manual .moc include.
// That .moc include trips up automoc4, even if it's #ifdef'd.
// Hence, we need separate files for KDE4, KF5, and KF6.
#include "XAttrViewPluginFactoryKF5.moc"
