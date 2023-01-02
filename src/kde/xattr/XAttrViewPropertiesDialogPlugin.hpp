/*******************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                             *
 * XAttrViewPropertiesDialogPlugin.hpp: KPropertiesDialogPlugin implementation *
 *                                                                             *
 * Copyright (c) 2016-2022 by David Korth.                                     *
 * SPDX-License-Identifier: GPL-2.0-or-later                                   *
 *******************************************************************************/

#ifndef __ROMPROPERTIES_KDE_XATTRS_XATTRVIEWPROPERTIESDIALOGPLUGIN_HPP__
#define __ROMPROPERTIES_KDE_XATTRS_XATTRVIEWPROPERTIESDIALOGPLUGIN_HPP__

#include <kpropertiesdialog.h>

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

#endif /* __ROMPROPERTIES_KDE_XATTRS_XATTRVIEWPROPERTIESDIALOGPLUGIN_HPP__ */
