/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomPropertiesDialogPlugin.hpp: KPropertiesDialogPlugin.                 *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_ROMPROPERTIESDIALOGPLUGIN_HPP__
#define __ROMPROPERTIES_KDE_ROMPROPERTIESDIALOGPLUGIN_HPP__

#include <kpropertiesdialog.h>

class Q_DECL_EXPORT RomPropertiesDialogPlugin final : public KPropertiesDialogPlugin
{
	Q_OBJECT

	public:
		explicit RomPropertiesDialogPlugin(KPropertiesDialog *props, const QVariantList & = QVariantList());

	private:
		typedef KPropertiesDialogPlugin super;
};

#endif /* __ROMPROPERTIES_KDE_ROMPROPERTIESDIALOGPLUGIN_HPP__ */
