/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDriveView.hpp: Sega Mega Drive ROM viewer.                          *
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

#ifndef __ROMPROPERTIES_KDE_MEGADRIVEVIEW_HPP__
#define __ROMPROPERTIES_KDE_MEGADRIVEVIEW_HPP__

#include <QWidget>

namespace LibRomData {
	class MegaDrive;
}

class MegaDriveViewPrivate;
class MegaDriveView : public QWidget
{
	Q_OBJECT

	public:
		MegaDriveView(const LibRomData::MegaDrive *rom, QWidget *parent = 0);
		virtual ~MegaDriveView();

	private:
		typedef QWidget super;
		MegaDriveViewPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(MegaDriveView)
	private:
		Q_DISABLE_COPY(MegaDriveView)
};

#endif /* __ROMPROPERTIES_KDE_MEGADRIVEVIEW_HPP__ */
