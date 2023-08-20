/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomDataView.hpp: RomData viewer.                                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <QtCore/QMetaType>
#include <QWidget>

#include "librpbase/RomData.hpp"
//Q_DECLARE_METATYPE(LibRpBase::RomData*)

class RomDataViewPrivate;
class RomDataView : public QWidget
{
	Q_OBJECT

	// FIXME: Not compatible with std::shared_ptr<>.
	//Q_PROPERTY(LibRpBase::RomData* romData READ romData WRITE setRomData NOTIFY romDataChanged)

	public:
		explicit RomDataView(QWidget *parent = nullptr);
		explicit RomDataView(const LibRpBase::RomDataPtr &romData, QWidget *parent = nullptr);
		~RomDataView() override;

	private:
		typedef QWidget super;
		RomDataViewPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(RomDataView)
		Q_DISABLE_COPY(RomDataView)

	protected:
		/** QWidget overridden functions. **/

		/**
		 * Window is now visible.
		 * This means that this tab has been selected.
		 * @param event QShowEvent
		 */
		void showEvent(QShowEvent *event) final;

		/**
		 * Window has been hidden.
		 * This means that a different tab has been selected.
		 * @param event QHideEvent
		 */
		void hideEvent(QHideEvent *event) final;

		/**
		 * Paint event.
		 *
		 * The window is technically "shown" and hidden at least once
		 * before the tab is selected, which causes the achievement
		 * notification to be triggered too early. Wait for an actual
		 * paint event before checking for achievements instead.
		 *
		 * @param event QPaintEvent
		 */
		void paintEvent(QPaintEvent *event) final;

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
		 * @param lc Language code.
		 */
		void cboLanguage_lcChanged_slot(uint32_t lc);

	public:
		/** Properties. **/

		/**
		 * Get the current RomData object.
		 * @return RomData object
		 */
		LibRpBase::RomDataPtr romData(void) const;

	public slots:
		/**
		 * Set the current RomData object.
		 *
		 * If a RomData object is already set, it is unref()'d.
		 * The new RomData object is ref()'d when set.
		 *
		 * @param romData New RomData object
		 */
		void setRomData(const LibRpBase::RomDataPtr &romData);

#if 0
	/* FIXME: Not compatible with std::shared_ptr<>. */
	signals:
		/**
		 * The RomData object has been changed.
		 * @param romData New RomData object
		 */
		void romDataChanged(LibRpBase::RomData *romData);
#endif

	private slots:
		/**
		 * An "Options" menu action was triggered.
		 * @param id Options ID.
		 */
		void btnOptions_triggered(int id);
};
