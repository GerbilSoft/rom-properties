/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * OptionsMenuButton.hpp: Options menu button QPushButton subclass.        *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// librpbase
#include "librpbase/RomData.hpp"

// NOTE: Using QT_VERSION_CHECK causes errors on moc-qt4 due to CMAKE_AUTOMOC.
// Reference: https://bugzilla.redhat.com/show_bug.cgi?id=1396755
// QT_VERSION_CHECK(5,0,0) -> 0x50000
#include <qglobal.h>
#if QT_VERSION >= 0x50000
#  define RP_OMB_USE_LAMBDA_FUNCTIONS 1
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

// RpQt.hpp for rp_qsizetype
#include "RpQt.hpp"

// Qt classes and includes
#include <QPushButton>
class QMenu;
#ifndef RP_OMB_USE_LAMBDA_FUNCTIONS
class QSignalMapper;
#endif /* !RP_OMB_USE_LAMBDA_FUNCTIONS */

enum StandardOptionID {
	OPTION_EXPORT_TEXT = -1,
	OPTION_EXPORT_JSON = -2,
	OPTION_COPY_TEXT = -3,
	OPTION_COPY_JSON = -4,
};

class OptionsMenuButton : public QPushButton
{
	Q_OBJECT

public:
	explicit OptionsMenuButton(QWidget *parent = nullptr);

private:
	typedef QPushButton super;
	Q_DISABLE_COPY(OptionsMenuButton)

public:
	/**
	 * Reset the menu items using the specified RomData object.
	 * @param widget OptionsMenuButton
	 * @param romData RomData object
	 */
	void reinitMenu(const LibRpBase::RomData *romData);

	/**
	 * Update a ROM operation menu item.
	 * @param widget OptionsMenuButton
	 * @param id ROM operation ID
	 * @param op ROM operation
	 */
	void updateOp(int id, const LibRpBase::RomData::RomOp *op);

signals:
	/**
	 * Menu item was triggered.
	 * @param id ROM operation ID
	 */
	void triggered(int id);

private:
	QMenu *menuOptions;
#ifndef RP_OMB_USE_LAMBDA_FUNCTIONS
	QSignalMapper *mapperOptionsMenu;
#endif /* RP_OMB_USE_LAMBDA_FUNCTIONS */
	rp_qsizetype romOps_firstActionIndex;
};
