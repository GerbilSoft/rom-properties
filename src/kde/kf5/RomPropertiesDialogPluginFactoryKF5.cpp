/***************************************************************************
 * ROM Properties Page shell extension. (KF5)                              *
 * RomPropertiesDialogPluginFactoryKF5.cpp: Factory class.                 *
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
	Q_UNUSED(w)
	KPropertiesDialog *props = qobject_cast<KPropertiesDialog*>(parent);
	Q_ASSERT(props);
	return new RomPropertiesDialogPlugin(props, args);
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
#include "RomPropertiesDialogPluginFactoryKF5.moc"
