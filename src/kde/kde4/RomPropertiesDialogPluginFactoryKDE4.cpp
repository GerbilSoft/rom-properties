/***************************************************************************
 * ROM Properties Page shell extension. (KDE4)                             *
 * RomPropertiesDialogPluginFactoryKDE4.cpp: Factory class.                *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
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
#include "RomPropertiesDialogPlugin.hpp"
#include <kpluginfactory.h>

static QObject *createRomPropertiesPage(QWidget *w, QObject *parent, const QVariantList &args)
{
	// NOTE: RomPropertiesDialogPlugin will verify that parent is an
	// instance of KPropertiesDialog*, so we don't have to do that here.
	Q_UNUSED(w)
	return new RomPropertiesDialogPlugin(parent, args);
}

K_PLUGIN_FACTORY(RomPropertiesDialogFactory, registerPlugin<RomPropertiesDialogPlugin>(QString(), createRomPropertiesPage);)
#if QT_VERSION < 0x050000
K_EXPORT_PLUGIN(RomPropertiesDialogFactory("rom-properties-kde"))
#endif

// automoc4 works correctly without any special handling.
// automoc5 doesn't notice that K_PLUGIN_FACTORY() has a
// Q_OBJECT macro, so it needs a manual .moc include.
// That .moc include trips up automoc4, even if it's #ifdef'd.
// Hence, we need separate files for KDE4 and KF5.
//#include "RomPropertiesDialogPluginFactoryKDE4.moc"
