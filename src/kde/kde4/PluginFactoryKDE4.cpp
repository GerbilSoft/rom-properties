/***************************************************************************
 * ROM Properties Page shell extension. (KDE4)                             *
 * PluginFactoryKDE4.cpp: Plugin factory class.                            *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.h
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.cpp
 * - https://github.com/KDE/calligra-history/blob/master/libs/main/KoDocInfoPropsFactory.cpp
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/kodocinfopropspage.desktop
 */

#include "config.kde.h"
#include "kde_register_backends.hpp"

// Plugins
#include "../plugins/RomPropertiesDialogPlugin.hpp"

// KDE
#include <kpluginfactory.h>

K_PLUGIN_FACTORY(RomPropertiesDialogFactory,
	kde_register_backends();
	registerPlugin<RomPropertiesDialogPlugin>();
)
K_EXPORT_PLUGIN(RomPropertiesDialogFactory("rom-properties-kde4"))

// automoc4 works correctly without any special handling.
// automoc5 doesn't notice that K_PLUGIN_FACTORY() has a
// Q_OBJECT macro, so it needs a manual .moc include.
// That .moc include trips up automoc4, even if it's #ifdef'd.
// Hence, we need separate files for KDE4, KF5, and KF6.
//#include "PluginFactoryKDE4.moc"
