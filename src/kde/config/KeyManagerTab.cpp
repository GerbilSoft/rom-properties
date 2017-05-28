/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyManagerTab.cpp: Key Manager tab for rp-config.                       *
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

#include "KeyManagerTab.hpp"

#include "KeyStore.hpp"
#include "KeyStoreModel.hpp"
#include "KeyStoreItemDelegate.hpp"

// Qt includes.
#include <QMenu>

#include "ui_KeyManagerTab.h"
class KeyManagerTabPrivate
{
	public:
		explicit KeyManagerTabPrivate(KeyManagerTab *q);

	private:
		KeyManagerTab *const q_ptr;
		Q_DECLARE_PUBLIC(KeyManagerTab)
		Q_DISABLE_COPY(KeyManagerTabPrivate)

	public:
		Ui::KeyManagerTab ui;

	public:
		// KeyStore.
		KeyStore *keyStore;
		// KeyStoreModel.
		KeyStoreModel *keyStoreModel;

		/**
		 * Resize the QTreeView's columns to fit their contents.
		 */
		void resizeColumnsToContents(void);
};

/** KeyManagerTabPrivate **/

KeyManagerTabPrivate::KeyManagerTabPrivate(KeyManagerTab* q)
	: q_ptr(q)
	, keyStore(new KeyStore(q))
	, keyStoreModel(new KeyStoreModel(q))
{
	// Set the KeyStoreModel's KeyStore.
	keyStoreModel->setKeyStore(keyStore);
}

/**
 * Resize the QTreeView's columns to fit their contents.
 */
void KeyManagerTabPrivate::resizeColumnsToContents(void)
{
	const int num_sections = keyStoreModel->columnCount();
	for (int i = num_sections-1; i >= 0; i--) {
		ui.treeKeyStore->resizeColumnToContents(i);
	}
	ui.treeKeyStore->resizeColumnToContents(num_sections);
}

/** KeyManagerTab **/

KeyManagerTab::KeyManagerTab(QWidget *parent)
	: super(parent)
	, d_ptr(new KeyManagerTabPrivate(this))
{
	Q_D(KeyManagerTab);
	d->ui.setupUi(this);

	// Set the QListView's model.
	// TODO: Proxy model for sorting.
	d->ui.treeKeyStore->setModel(d->keyStoreModel);
	d->resizeColumnsToContents();

	// Initialize treeKeyStore's item delegate.
	d->ui.treeKeyStore->setItemDelegate(new KeyStoreItemDelegate(this));

	// Create the dropdown menu for the "Import" button.
	QMenu *menuImport = new QMenu(tr("&Import"), d->ui.btnImport);
	menuImport->addAction(d->ui.actionImportWiiKeysBin);
	menuImport->addAction(d->ui.actionImport3DSboot9firm);
	menuImport->addAction(d->ui.actionImport3DSaeskeydb);
	d->ui.btnImport->setMenu(menuImport);

	// Connect KeyStore's modified() signal to our modified() signal.
	connect(d->keyStore, SIGNAL(modified()), this, SIGNAL(modified()));
}

KeyManagerTab::~KeyManagerTab()
{
	delete d_ptr;
}

/**
 * Widget state has changed.
 * @param event State change event.
 */
void KeyManagerTab::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(KeyManagerTab);
		d->ui.retranslateUi(this);
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/**
 * Reset the configuration.
 */
void KeyManagerTab::reset(void)
{
	Q_D(KeyManagerTab);
	d->keyStore->reset();
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void KeyManagerTab::loadDefaults(void)
{
	// Not implemented for this tab.
}

/**
 * Save the configuration.
 * @param pSettings QSettings object.
 */
void KeyManagerTab::save(QSettings *pSettings)
{
	// TODO: Needs to save to keys.conf, not rom-properties.conf.
	return;
}
