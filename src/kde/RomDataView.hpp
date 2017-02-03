/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomDataView.hpp: RomData viewer.                                        *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include <QtCore/QMetaType>
#include <QWidget>

#include "libromdata/RomData.hpp"
Q_DECLARE_METATYPE(LibRomData::RomData*)

class RomDataViewPrivate;
class RomDataView : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(LibRomData::RomData* romData READ romData WRITE setRomData NOTIFY romDataChanged)

	public:
		explicit RomDataView(QWidget *parent = 0);
		explicit RomDataView(LibRomData::RomData *romData, QWidget *parent = 0);
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
		virtual void showEvent(QShowEvent *event) override final;

		/**
		 * Window has been hidden.
		 * This means that a different tab has been selected.
		 * @param event QHideEvent.
		 */
		virtual void hideEvent(QHideEvent *event) override final;

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

		/** Properties. **/

	public:
		/**
		 * Get the current RomData object.
		 * @return RomData object.
		 */
		LibRomData::RomData *romData(void) const;

	public slots:
		/**
		 * Set the current RomData object.
		 *
		 * If a RomData object is already set, it is unref()'d.
		 * The new RomData object is ref()'d when set.
		 *
		 * @return RomData object.
		 */
		void setRomData(LibRomData::RomData *romData);

	signals:
		/**
		 * The RomData object has been changed.
		 * @param romData New RomData object.
		 */
		void romDataChanged(LibRomData::RomData *romData);
};

#endif /* __ROMPROPERTIES_KDE_ROMDATAVIEW_HPP__ */
