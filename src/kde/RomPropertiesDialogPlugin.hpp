/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomPropertiesDialogPlugin.hpp: KPropertiesDialogPlugin implementation   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <kpropertiesdialog.h>

class RomDataView;

class RomPropertiesDialogPlugin : public KPropertiesDialogPlugin
{
Q_OBJECT

public:
	/**
	 * Instantiate RomDataView for the given KPropertiesDialog.
	 * @param parent KPropertiesDialog (NOTE: QObject* is used for registerPlugin() compatibility.)
	 * @param args
	 */
	explicit RomPropertiesDialogPlugin(QObject *parent, const QVariantList &args = QVariantList());

private:
	// Disable these constructors.
	explicit RomPropertiesDialogPlugin(QObject *parent);
	explicit RomPropertiesDialogPlugin(KPropertiesDialog *_props);

private:
	typedef KPropertiesDialogPlugin super;
	Q_DISABLE_COPY(RomPropertiesDialogPlugin)

private:
	/**
	 * Instantiate a RomDataView object for the given KFileItem.
	 * @param fileItem KFileItem
	 * @param props KPropertiesDialog
	 * @return RomDataView object, or nullptr if the file is not supported.
	 */
	RomDataView *createRomDataView(const KFileItem &fileItem, KPropertiesDialog *props = nullptr);
};
