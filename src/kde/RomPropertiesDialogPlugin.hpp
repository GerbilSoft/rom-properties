/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomPropertiesDialogPlugin.hpp: KPropertiesDialogPlugin.                 *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_ROMPROPERTIESDIALOGPLUGIN_HPP__
#define __ROMPROPERTIES_KDE_ROMPROPERTIESDIALOGPLUGIN_HPP__

#include <kpropertiesdialog.h>

class RomPropertiesDialogPlugin : public KPropertiesDialogPlugin
{
	Q_OBJECT

	public:
		/**
		 * Instantiate a RomDataView for the given KPropertiesDialog.
		 * @param parent KPropertiesDialog (NOTE: QObject* is used for registerPlugin() compatibility.)
		 * @param args
		 */
		explicit RomPropertiesDialogPlugin(QObject *parent, const QVariantList &args = QVariantList());

	private:
		// Disable these constructors.
		explicit RomPropertiesDialogPlugin(QObject *parent);
		explicit RomPropertiesDialogPlugin(KPropertiesDialog *_props);

	private:
		typedef KPropertiesDialogPlugin super;
		Q_DISABLE_COPY(RomPropertiesDialogPlugin)
};

#endif /* __ROMPROPERTIES_KDE_ROMPROPERTIESDIALOGPLUGIN_HPP__ */
