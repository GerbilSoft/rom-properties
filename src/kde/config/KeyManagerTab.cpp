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
#include "RpQt.hpp"

#include "KeyStoreQt.hpp"
#include "KeyStoreModel.hpp"
#include "KeyStoreItemDelegate.hpp"

// C includes. (C++ namespace)
#include <cassert>

// Qt includes.
#include <QFileDialog>
#include <QMenu>

#include "ui_KeyManagerTab.h"
class KeyManagerTabPrivate
{
	public:
		explicit KeyManagerTabPrivate(KeyManagerTab *q);
		~KeyManagerTabPrivate();

	private:
		KeyManagerTab *const q_ptr;
		Q_DECLARE_PUBLIC(KeyManagerTab)
		Q_DISABLE_COPY(KeyManagerTabPrivate)

	public:
		Ui::KeyManagerTab ui;

	public:
		// KeyStore.
		KeyStoreQt *keyStore;
		// KeyStoreModel.
		KeyStoreModel *keyStoreModel;

		/**
		 * Resize the QTreeView's columns to fit their contents.
		 */
		void resizeColumnsToContents(void);

		/**
		 * Show key import return status.
		 * @param filename Filename.
		 * @param keyType Key type.
		 * @param iret ImportReturn.
		 */
		void showKeyImportReturnStatus(const QString &filename,
			const QString &keyType, const KeyStoreQt::ImportReturn &iret);
};

/** KeyManagerTabPrivate **/

KeyManagerTabPrivate::KeyManagerTabPrivate(KeyManagerTab* q)
	: q_ptr(q)
	, keyStore(new KeyStoreQt(q))
	, keyStoreModel(new KeyStoreModel(q))
{
	// Set the KeyStoreModel's KeyStore.
	keyStoreModel->setKeyStore(keyStore);
}

KeyManagerTabPrivate::~KeyManagerTabPrivate()
{
	// Make sure keyStoreModel is deleted before keyStore.
	delete keyStoreModel;
	delete keyStore;
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

/**
 * Show key import return status.
 * @param filename Filename.
 * @param keyType Key type.
 * @param iret ImportReturn.
 */
void KeyManagerTabPrivate::showKeyImportReturnStatus(
	const QString &filename,
	const QString &keyType,
	const KeyStoreQt::ImportReturn &iret)
{
	QString msg;
	MessageWidget::MsgIcon icon;
	bool showKeyStats = false;
	switch (iret.status) {
		case KeyStoreQt::Import_InvalidParams:
		default:
			msg = KeyManagerTab::tr("An invalid parameter was passed to the key importer.\nTHIS IS A BUG; please report this to the developers!");
			icon = MessageWidget::ICON_CRITICAL;
			break;

		case KeyStoreQt::Import_OpenError:
			// TODO: Show error.
			msg = KeyManagerTab::tr("An error occurred while opening '%1'.")
				.arg(QFileInfo(filename).fileName());
			icon = MessageWidget::ICON_CRITICAL;
			break;

		case KeyStoreQt::Import_ReadError:
			// TODO: Show error.
			msg = KeyManagerTab::tr("An error occurred while reading '%1'.")
				.arg(QFileInfo(filename).fileName());
			icon = MessageWidget::ICON_CRITICAL;
			break;

		case KeyStoreQt::Import_InvalidFile:
			msg = KeyManagerTab::tr("The file '%1' is not a %2 file.")
				.arg(QFileInfo(filename).fileName())
				.arg(keyType);
			icon = MessageWidget::ICON_WARNING;
			break;

		case KeyStoreQt::Import_NoKeysImported:
			msg = KeyManagerTab::tr("No keys were imported from '%1'.")
				.arg(QFileInfo(filename).fileName());
			icon = MessageWidget::ICON_WARNING;
			showKeyStats = true;
			break;

		case KeyStoreQt::Import_KeysImported:
			msg = KeyManagerTab::tr("%Ln key(s) were imported from '%1'.",
				 nullptr, iret.keysImportedVerify + iret.keysImportedNoVerify)
				.arg(QFileInfo(filename).fileName());
			icon = MessageWidget::ICON_INFORMATION;
			showKeyStats = true;
			break;
	}

	if (showKeyStats) {
		if (iret.keysExist > 0) {
			msg += QChar(L'\n') + QChar(0x2022) + QChar(L' ') +
				KeyManagerTab::tr("%Ln key(s) already exist in the Key Manager.",
					nullptr, iret.keysExist);
		}
		if (iret.keysInvalid > 0) {
			msg += QChar(L'\n') + QChar(0x2022) + QChar(L' ') +
				KeyManagerTab::tr("%Ln key(s) were not imported because they are incorrect.",
					nullptr, iret.keysInvalid);
		}
		if (iret.keysNotUsed > 0) {
			msg += QChar(L'\n') + QChar(0x2022) + QChar(L' ') +
				KeyManagerTab::tr("%Ln key(s) were not imported because they aren't used by rom-properties.",
					nullptr, iret.keysNotUsed);
		}
		if (iret.keysCantDecrypt > 0) {
			msg += QChar(L'\n') + QChar(0x2022) + QChar(L' ') +
				KeyManagerTab::tr("%Ln key(s) were not imported because they are encrypted and the key isn't available.",
					nullptr, iret.keysNotUsed);
		}
		if (iret.keysImportedVerify > 0) {
			msg += QChar(L'\n') + QChar(0x2022) + QChar(L' ') +
				KeyManagerTab::tr("%Ln key(s) have been imported and verified as correct.",
					nullptr, iret.keysImportedVerify);
		}
		if (iret.keysImportedNoVerify > 0) {
			msg += QChar(L'\n') + QChar(0x2022) + QChar(L' ') +
				KeyManagerTab::tr("%Ln key(s) have been imported without verification.",
					nullptr, iret.keysImportedNoVerify);
		}
	}

	ui.msgWidget->showMessage(msg, icon);
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
	d->ui.treeKeyStore->expandAll();

	// Make the first column "spanned" for all section headers.
	for (int sectIdx = d->keyStore->sectCount(); sectIdx >= 0; sectIdx--) {
		d->ui.treeKeyStore->setFirstColumnSpanned(sectIdx, QModelIndex(), true);
	}
	// Must be done *after* marking the first column as "spanned".
	d->resizeColumnsToContents();

	// Initialize treeKeyStore's item delegate.
	d->ui.treeKeyStore->setItemDelegate(new KeyStoreItemDelegate(this));

	// Create the dropdown menu for the "Import" button.
	QMenu *menuImport = new QMenu(tr("I&mport"), d->ui.btnImport);
	menuImport->addAction(d->ui.actionImportWiiKeysBin);
	menuImport->addAction(d->ui.actionImportWiiUOtpBin);
	menuImport->addAction(d->ui.actionImport3DSboot9bin);
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
	// pSettings is keys.conf.
	assert(pSettings != nullptr);
	if (!pSettings)
		return;

	Q_D(KeyManagerTab);
	if (!d->keyStore->hasChanged())
		return;

	// [Keys]
	pSettings->beginGroup(QLatin1String("Keys"));

	// Save the keys.
	const int totalKeyCount = d->keyStore->totalKeyCount();
	for (int i = 0; i < totalKeyCount; i++) {
		const KeyStoreQt::Key *const pKey = d->keyStore->getKey(i);
		assert(pKey != nullptr);
		if (!pKey || !pKey->modified)
			continue;

		// Save this key.
		pSettings->setValue(U82Q(pKey->name), U82Q(pKey->value));
	}

	// End of [Keys]
	pSettings->endGroup();

	// Clear the modified status.
	d->keyStore->allKeysSaved();
}

/** Actions. **/

/**
 * Import keys from Wii keys.bin. (BootMii format)
 */
void KeyManagerTab::on_actionImportWiiKeysBin_triggered(void)
{
	QString filename = QFileDialog::getOpenFileName(this,
		tr("Select Wii keys.bin File"),	// caption
		QString(),			// dir (TODO)
		tr("keys.bin (keys.bin);;Binary Files (*.bin);;All Files (*.*)"));
	if (filename.isEmpty())
		return;

	Q_D(KeyManagerTab);
	KeyStoreQt::ImportReturn iret = d->keyStore->importWiiKeysBin(Q2U8(filename));
	d->showKeyImportReturnStatus(filename, QLatin1String("Wii keys.bin"), iret);
}

/**
 * Import keys from Wii U otp.bin.
 */
void KeyManagerTab::on_actionImportWiiUOtpBin_triggered(void)
{
	QString filename = QFileDialog::getOpenFileName(this,
		tr("Select Wii U otp.bin File"),	// caption
		QString(),				// dir (TODO)
		tr("otp.bin (otp.bin);;Binary Files (*.bin);;All Files (*.*)"));
	if (filename.isEmpty())
		return;

	Q_D(KeyManagerTab);
	KeyStoreQt::ImportReturn iret = d->keyStore->importWiiUOtpBin(Q2U8(filename));
	d->showKeyImportReturnStatus(filename, QLatin1String("Wii U otp.bin"), iret);
}

/**
 * Import keys from 3DS boot9.bin.
 */
void KeyManagerTab::on_actionImport3DSboot9bin_triggered(void)
{
	QString filename = QFileDialog::getOpenFileName(this,
		tr("Select 3DS boot9.bin File"),	// caption
		QString(),			// dir (TODO)
		tr("boot9.bin (boot9.bin);;Binary Files (*.bin);;All Files (*.*)"));
	if (filename.isEmpty())
		return;

	Q_D(KeyManagerTab);
	KeyStoreQt::ImportReturn iret = d->keyStore->import3DSboot9bin(Q2U8(filename));
	d->showKeyImportReturnStatus(filename, QLatin1String("3DS boot9.bin"), iret);
}

/**
 * Import keys from 3DS aeskeydb.bin.
 */
void KeyManagerTab::on_actionImport3DSaeskeydb_triggered(void)
{
	QString filename = QFileDialog::getOpenFileName(this,
		tr("Select 3DS aeskeydb.bin File"),	// caption
		QString(),				// dir (TODO)
		tr("aeskeydb.bin (aeskeydb.bin);;Binary Files (*.bin);;All Files (*.*)"));
	if (filename.isEmpty())
		return;

	Q_D(KeyManagerTab);
	KeyStoreQt::ImportReturn iret = d->keyStore->import3DSaeskeydb(Q2U8(filename));
	d->showKeyImportReturnStatus(filename, QLatin1String("3DS aeskeydb.bin"), iret);
}
