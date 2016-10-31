/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomDataView.hpp: RomData viewer.                                        *
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

#ifndef __ROMPROPERTIES_KDE_ROMDATAVIEW_HPP__
#define __ROMPROPERTIES_KDE_ROMDATAVIEW_HPP__

#include <QWidget>

namespace LibRomData {
	class RomData;
}

class RomDataViewPrivate;
class RomDataView : public QWidget
{
	Q_OBJECT

	public:
		RomDataView(LibRomData::RomData *romData, QWidget *parent = 0);
		virtual ~RomDataView();

	private:
		typedef QWidget super;
		RomDataViewPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(RomDataView)
	private:
		Q_DISABLE_COPY(RomDataView)

	protected:
		/** QWidget overridden functions. **/

		/**
		 * Window has been hidden.
		 * This means that this tab has been selected.
		 * @param event QShowEvent.
		 */
		virtual void showEvent(QShowEvent *event) final;

		/**
		 * Window has been hidden.
		 * This means that a different tab has been selected.
		 * @param event QHideEvent.
		 */
		virtual void hideEvent(QHideEvent *event) final;

	protected slots:
		/** Widget slots. **/

		/**
		 * Disable user modification of RFT_BITFIELD checkboxes.
		 */
		void bitfield_toggled_slot(bool checked);

		/**
		 * Animated icon timer.
		 */
		void tmrIconAnim_timeout(void);
};

#endif /* __ROMPROPERTIES_KDE_ROMDATAVIEW_HPP__ */
