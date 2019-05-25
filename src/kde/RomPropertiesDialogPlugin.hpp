/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomPropertiesDialogPlugin.hpp: KPropertiesDialogPlugin.                 *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_ROMPROPERTIESDIALOGPLUGIN_HPP__
#define __ROMPROPERTIES_KDE_ROMPROPERTIESDIALOGPLUGIN_HPP__

#include <kpropertiesdialog.h>

class Q_DECL_EXPORT RomPropertiesDialogPlugin : public KPropertiesDialogPlugin
{
	Q_OBJECT

	public:
		explicit RomPropertiesDialogPlugin(KPropertiesDialog *props, const QVariantList & = QVariantList());
		virtual ~RomPropertiesDialogPlugin();

	private:
		typedef KPropertiesDialogPlugin super;
};

#endif /* __ROMPROPERTIES_KDE_ROMPROPERTIESDIALOGPLUGIN_HPP__ */
