/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomPropertiesDialogPlugin.hpp: KPropertiesDialogPlugin.                 *
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
