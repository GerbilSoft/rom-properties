/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * OptionsMenuButton.cpp: Options menu button QPushButton subclass.        *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "OptionsMenuButton.hpp"

// libromdata
using LibRpBase::RomData;

// C++ STL classes
using std::array;
using std::vector;

/** Standard actions. **/
struct option_menu_action_t {
	const char *desc;
	int id;
};
static constexpr array<option_menu_action_t, 4> stdacts = {{
	{NOP_C_("OptionsMenuButton|StdActs", "Export to Text..."),	OPTION_EXPORT_TEXT},
	{NOP_C_("OptionsMenuButton|StdActs", "Export to JSON..."),	OPTION_EXPORT_JSON},
	{NOP_C_("OptionsMenuButton|StdActs", "Copy as Text"),		OPTION_COPY_TEXT},
	{NOP_C_("OptionsMenuButton|StdActs", "Copy as JSON"),		OPTION_COPY_JSON},
}};

OptionsMenuButton::OptionsMenuButton(QWidget *parent)
	: super(parent)
	, menuOptions(nullptr)
#ifndef RP_OMB_USE_LAMBDA_FUNCTIONS
	, mapperOptionsMenu(nullptr)
#endif /* RP_OMB_USE_LAMBDA_FUNCTIONS */
	, romOps_firstActionIndex(-1)
{
	// tr: "Options" button.
	const QString s_options = U82Q(C_("OptionsMenuButton", "&Options"));
	this->setText(s_options);

	// Create the menu.
	menuOptions = new QMenu(s_options, this);
	menuOptions->setObjectName(QLatin1String("menuOptions"));
	this->setMenu(menuOptions);

#ifndef RP_OMB_USE_LAMBDA_FUNCTIONS
	// Qt4: Create the QSignalMapper.
	mapperOptionsMenu = new QSignalMapper(this);
	mapperOptionsMenu->setObjectName(QLatin1String("mapperOptionsMenu"));
	QObject::connect(mapperOptionsMenu, SIGNAL(mapped(int)),
		this, SIGNAL(triggered(int)));
#endif /* !RP_OMB_USE_LAMBDA_FUNCTIONS */
}

/**
 * Reset the menu items using the specified RomData object.
 * @param widget OptionsMenuButton
 * @param romData RomData object
 */
void OptionsMenuButton::reinitMenu(const LibRpBase::RomData *romData)
{
	// Clear the menu.
	menuOptions->clear();

	// Add the standard actions.
	for (const option_menu_action_t &p : stdacts) {
		QAction *const action = menuOptions->addAction(
			U82Q(pgettext_expr("OptionsMenuButton", p.desc)));
#ifdef RP_OMB_USE_LAMBDA_FUNCTIONS
		// Qt5: Use a lambda function.
		const int id = p.id;	// only capture id, not the whole reference
		QObject::connect(action, &QAction::triggered,
			[this, id] { emit triggered(id); });
#else /* !RP_OMB_USE_LAMBDA_FUNCTIONS */
		// Qt4: Use the QSignalMapper.
		QObject::connect(action, SIGNAL(triggered()),
			mapperOptionsMenu, SLOT(map()));
		mapperOptionsMenu->setMapping(action, p.id);
#endif /* RP_OMB_USE_LAMBDA_FUNCTIONS */
	}

	/** ROM operations. **/
	const vector<RomData::RomOp> ops = romData->romOps();
	if (!ops.empty()) {
		menuOptions->addSeparator();
		// NOTE: We need to save the index because menuOptions has
		// more children than we would otherwise expect.
		romOps_firstActionIndex = menuOptions->children().count();

		int i = 0;
		for (const RomData::RomOp &op : ops) {
			QAction *const action = menuOptions->addAction(U82Q(op.desc));
			action->setEnabled(!!(op.flags & RomData::RomOp::ROF_ENABLED));
#ifdef RP_OMB_USE_LAMBDA_FUNCTIONS
			// Qt5: Use a lambda function.
			QObject::connect(action, &QAction::triggered,
				[this, i] { emit triggered(i); });
#else /* !RP_OMB_USE_LAMBDA_FUNCTIONS */
			// Qt4: Use the QSignalMapper.
			QObject::connect(action, SIGNAL(triggered()),
				mapperOptionsMenu, SLOT(map()));
			mapperOptionsMenu->setMapping(action, i);
#endif /* RP_OMB_USE_LAMBDA_FUNCTIONS */

			// Next operation.
			i++;
		}
	}
}

/**
 * Update a ROM operation menu item.
 * @param widget OptionsMenuButton
 * @param id ROM operation ID
 * @param op ROM operation
 */
void OptionsMenuButton::updateOp(int id, const LibRpBase::RomData::RomOp *op)
{
	assert(id >= 0);
	assert(op != nullptr);
	if (id < 0 || !op)
		return;

	assert(romOps_firstActionIndex >= 0);
	if (romOps_firstActionIndex < 0) {
		// No ROM operations...
		return;
	}

	const QObjectList &objList = menuOptions->children();
	const int actionIndex = id + romOps_firstActionIndex;
	assert(actionIndex < objList.size());
	if (actionIndex >= objList.size())
		return;

	QAction *const action = qobject_cast<QAction*>(objList.at(actionIndex));
	assert(action != nullptr);
	if (action) {
		action->setText(U82Q(op->desc));
		action->setEnabled(!!(op->flags & RomData::RomOp::ROF_ENABLED));
	}
}
