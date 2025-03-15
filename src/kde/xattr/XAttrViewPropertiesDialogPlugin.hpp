/*******************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                             *
 * XAttrViewPropertiesDialogPlugin.hpp: KPropertiesDialogPlugin implementation *
 *                                                                             *
 * Copyright (c) 2016-2025 by David Korth.                                     *
 * SPDX-License-Identifier: GPL-2.0-or-later                                   *
 *******************************************************************************/

#pragma once

#include <QtCore/qglobal.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  include <kpropertiesdialogplugin.h>
#else /* QT_VERSION < QT_VERSION_CHECK(6, 0, 0) */
#  include <kpropertiesdialog.h>
#endif /* QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) */

class XAttrView;

class XAttrViewPropertiesDialogPlugin : public KPropertiesDialogPlugin
{
Q_OBJECT

public:
	/**
	 * Instantiate properties pages for the given KPropertiesDialog.
	 * @param parent KPropertiesDialog (NOTE: QObject* is used for registerPlugin() compatibility.)
	 * @param args
	 */
	explicit XAttrViewPropertiesDialogPlugin(QObject *parent, const QVariantList &args = QVariantList());

private:
	// Disable these constructors.
	explicit XAttrViewPropertiesDialogPlugin(QObject *parent);
	explicit XAttrViewPropertiesDialogPlugin(KPropertiesDialog *_props);

private:
	typedef KPropertiesDialogPlugin super;
	Q_DISABLE_COPY(XAttrViewPropertiesDialogPlugin)

protected:
	/**
	 * Instantiate an XAttrView object for the given KFileItem.
	 * @param fileItem KFileItem
	 * @param props KPropertiesDialog
	 * @return XAttrView object, or nullptr if the file is not supported.
	 */
	XAttrView *createXAttrView(const KFileItem &fileItem, KPropertiesDialog *props = nullptr);
};
