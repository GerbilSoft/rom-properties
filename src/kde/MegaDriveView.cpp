/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDriveView.cpp: Sega Mega Drive ROM viewer.                          *
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

#include "MegaDriveView.hpp"
#include "libromdata/MegaDrive.hpp"

#include "ui_MegaDriveView.h"
class MegaDriveViewPrivate
{
	public:
		MegaDriveViewPrivate(MegaDriveView *q, const LibRomData::MegaDrive *rom);
		~MegaDriveViewPrivate();

	private:
		MegaDriveView *const q_ptr;
		Q_DECLARE_PUBLIC(MegaDriveView)
	private:
		Q_DISABLE_COPY(MegaDriveViewPrivate)

	public:
		Ui::MegaDriveView ui;
		const LibRomData::MegaDrive *rom;
};

/** MegaDriveViewPrivate **/

MegaDriveViewPrivate::MegaDriveViewPrivate(MegaDriveView *q, const LibRomData::MegaDrive *rom)
	: q_ptr(q)
	, rom(rom)
{ }

MegaDriveViewPrivate::~MegaDriveViewPrivate()
{
	delete rom;
}

/** MegaDriveView **/

MegaDriveView::MegaDriveView(const LibRomData::MegaDrive *rom, QWidget *parent)
	: super(parent)
	, d_ptr(new MegaDriveViewPrivate(this, rom))
{
	Q_D(MegaDriveView);
	d->ui.setupUi(this);
}

MegaDriveView::~MegaDriveView()
{
	delete d_ptr;
}
