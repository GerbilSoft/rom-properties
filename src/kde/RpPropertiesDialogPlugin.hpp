/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpPropertiesDialogPlugin.hpp: KPropertiesDialogPlugin implementation.   *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_RPPROPERTIESDIALOGPLUGIN_HPP__
#define __ROMPROPERTIES_KDE_RPPROPERTIESDIALOGPLUGIN_HPP__

#include <kpropertiesdialog.h>

class RomDataView;
class XAttrView;

class RpPropertiesDialogPlugin : public KPropertiesDialogPlugin
{
	Q_OBJECT

	public:
		/**
		 * Instantiate properties pages for the given KPropertiesDialog.
		 * @param parent KPropertiesDialog (NOTE: QObject* is used for registerPlugin() compatibility.)
		 * @param args
		 */
		explicit RpPropertiesDialogPlugin(QObject *parent, const QVariantList &args = QVariantList());

	private:
		// Disable these constructors.
		explicit RpPropertiesDialogPlugin(QObject *parent);
		explicit RpPropertiesDialogPlugin(KPropertiesDialog *_props);

	private:
		typedef KPropertiesDialogPlugin super;
		Q_DISABLE_COPY(RpPropertiesDialogPlugin)

	protected:
		/**
		 * Instantiate a RomDataView object for the given QUrl.
		 * @param fileItem KFileItem
		 * @param props KPropertiesDialog
		 * @return RomDataView object, or nullptr if the file is not supported.
		 */
		RomDataView *createRomDataView(const KFileItem &fileItem, KPropertiesDialog *props = nullptr);

		/**
		 * Instantiate an XAttrView object for the given QUrl.
		 * @param fileItem KFileItem
		 * @param props KPropertiesDialog
		 * @return XAttrView object, or nullptr if the file is not supported.
		 */
		XAttrView *createXAttrView(const KFileItem &fileItem, KPropertiesDialog *props = nullptr);
};

#endif /* __ROMPROPERTIES_KDE_RPPROPERTIESDIALOGPLUGIN_HPP__ */
