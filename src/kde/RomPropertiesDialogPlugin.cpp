/***************************************************************************
 * ROM Properties Page extension. (KDE4/KDE5)                              *
 * RomPropertiesDialogPlugin.cpp: KPropertiesDialogPlugin.                 *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

/**
 * References:
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.h
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.cpp
 * - https://github.com/KDE/calligra-history/blob/master/libs/main/KoDocInfoPropsFactory.cpp
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/kodocinfopropspage.desktop
 */

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
K_EXPORT_PLUGIN(RomPropertiesDialogFactory("rom-properties-kde"))

RomPropertiesDialogPlugin::RomPropertiesDialogPlugin(KPropertiesDialog *props, const QVariantList&)
	: KPropertiesDialogPlugin(props)
{
	// TODO: Check if a single file is specified,
	// and if the file is supported.
	props->addPage(new QWidget(props), QLatin1String("ROM Properties"));
}

RomPropertiesDialogPlugin::~RomPropertiesDialogPlugin()
{ }
