/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomDataView.hpp: RomData viewer.                                        *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_ROMDATAVIEW_HPP__
#define __ROMPROPERTIES_KDE_ROMDATAVIEW_HPP__

#include <QtCore/QMetaType>
#include <QWidget>

#include "librpbase/RomData.hpp"
Q_DECLARE_METATYPE(LibRpBase::RomData*)

class RomDataViewPrivate;
class RomDataView : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(LibRpBase::RomData* romData READ romData WRITE setRomData NOTIFY romDataChanged)

	public:
		explicit RomDataView(QWidget *parent = 0);
		explicit RomDataView(LibRpBase::RomData *romData, QWidget *parent = 0);
		virtual ~RomDataView();

	private:
		typedef QWidget super;
		RomDataViewPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(RomDataView)
		Q_DISABLE_COPY(RomDataView)

	protected:
		/** QWidget overridden functions. **/

		/**
		 * Window has been hidden.
		 * This means that this tab has been selected.
		 * @param event QShowEvent.
		 */
		void showEvent(QShowEvent *event) final;

		/**
		 * Window has been hidden.
		 * This means that a different tab has been selected.
		 * @param event QHideEvent.
		 */
		void hideEvent(QHideEvent *event) final;

		/**
		 * Event filter for recalculating RFT_LISTDATA row heights.
		 * @param object QObject.
		 * @param event Event.
		 * @return True to filter the event; false to pass it through.
		 */
		bool eventFilter(QObject *object, QEvent *event) final;

	protected slots:
		/** Widget slots. **/

		/**
		 * Disable user modification of RFT_BITFIELD checkboxes.
		 */
		void bitfield_clicked_slot(bool checked);

		/**
		 * The RFT_MULTI_STRING language was changed.
		 * @param index Index.
		 */
		void cboLanguage_currentIndexChanged_slot(int index);

	public:
		/** Properties. **/

		/**
		 * Get the current RomData object.
		 * @return RomData object.
		 */
		LibRpBase::RomData *romData(void) const;

	public slots:
		/**
		 * Set the current RomData object.
		 *
		 * If a RomData object is already set, it is unref()'d.
		 * The new RomData object is ref()'d when set.
		 */
		void setRomData(LibRpBase::RomData *romData);

	signals:
		/**
		 * The RomData object has been changed.
		 * @param romData New RomData object.
		 */
		void romDataChanged(LibRpBase::RomData *romData);

	private slots:
		/**
		 * An "Options" menu action was triggered.
		 * @param id Options ID.
		 */
		void menuOptions_action_triggered(int id);
};

#endif /* __ROMPROPERTIES_KDE_ROMDATAVIEW_HPP__ */
