/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyManagerTab.cpp: Key Manager tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "KeyManagerTab.hpp"
#include "RpQt.hpp"

// librpbase
#include "librpbase/TextFuncs.hpp"
using namespace LibRpBase;

// libi18n
#include "libi18n/i18n.h"

#include "KeyStoreQt.hpp"
#include "KeyStoreModel.hpp"
#include "KeyStoreItemDelegate.hpp"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
using std::string;

// Qt includes.
#include <QFileDialog>
#include <QLocale>
#include <QMenu>

// KDE includes.
#include <kmessagewidget.h>

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

		// Starting directory for importing keys.
		// TODO: Save this in the configuration file?
		QString keyFileDir;

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
	KMessageWidget::MessageType type = KMessageWidget::Information;
	QStyle::StandardPixmap icon = QStyle::SP_MessageBoxInformation;
	const QLocale sysLocale = QLocale::system();
	bool showKeyStats = false;
	string msg;
	msg.reserve(1024);

	// Filename, minus directory.
	const QString fileNoPath = QFileInfo(filename).fileName();

	// TODO: Localize POSIX error messages?
	// TODO: Thread-safe strerror()?

	switch (iret.status) {
		case KeyStoreQt::Import_InvalidParams:
		default:
			msg = C_("KeyManagerTab",
				"An invalid parameter was passed to the key importer.\n"
				"THIS IS A BUG; please report this to the developers!");
			type = KMessageWidget::Error;
			icon = QStyle::SP_MessageBoxCritical;
			break;

		case KeyStoreQt::Import_OpenError:
			if (iret.error_code != 0) {
				msg = rp_sprintf_p(C_("KeyManagerTab",
					// tr: %1$s == filename, %2$s == error message
					"An error occurred while opening '%1$s': %2$s"),
					fileNoPath.toUtf8().constData(),
					strerror(iret.error_code));
			} else {
				msg = rp_sprintf_p(C_("KeyManagerTab",
					// tr: %s == filename
					"An error occurred while opening '%s'."),
					fileNoPath.toUtf8().constData());
			}
			type = KMessageWidget::Error;
			icon = QStyle::SP_MessageBoxCritical;
			break;

		case KeyStoreQt::Import_ReadError:
			// TODO: Error code for short reads.
			if (iret.error_code != 0) {
				msg = rp_sprintf_p(C_("KeyManagerTab",
					// tr: %1$s == filename, %2$s == error message
					"An error occurred while reading '%1$s': %2$s"),
					fileNoPath.toUtf8().constData(),
					strerror(iret.error_code));
			} else {
				msg = rp_sprintf_p(C_("KeyManagerTab",
					// tr: %s == filename
					"An error occurred while reading '%s'."),
					fileNoPath.toUtf8().constData());
			}
			type = KMessageWidget::Error;
			icon = QStyle::SP_MessageBoxCritical;
			break;

		case KeyStoreQt::Import_InvalidFile:
			msg = rp_sprintf_p(C_("KeyManagerTab",
				// tr: %1$s == filename, %2$s == type of file
				"The file '%1$s' is not a valid %2$s file."),
				fileNoPath.toUtf8().constData(),
				keyType.toUtf8().constData());
			type = KMessageWidget::Warning;
			icon = QStyle::SP_MessageBoxWarning;
			break;

		case KeyStoreQt::Import_NoKeysImported:
			msg = rp_sprintf(C_("KeyManagerTab",
				// tr: %s == filename
				"No keys were imported from '%s'."),
				fileNoPath.toUtf8().constData());
			type = KMessageWidget::Information;
			icon = QStyle::SP_MessageBoxInformation;
			showKeyStats = true;
			break;

		case KeyStoreQt::Import_KeysImported: {
			const unsigned int keyCount = iret.keysImportedVerify + iret.keysImportedNoVerify;
			msg = rp_sprintf_p(NC_("KeyManagerTab",
				// tr: %1$s == number of keys (formatted), %2$u == filename
				"%1$s key was imported from '%2$s'.",
				"%1$s keys were imported from '%2$s'.",
				keyCount),
				sysLocale.toString(keyCount).toUtf8().constData(),
				fileNoPath.toUtf8().constData());
			type = KMessageWidget::Positive;
			icon = QStyle::SP_DialogOkButton;
			showKeyStats = true;
			break;
		}
	}

	// U+2022 (BULLET) == \xE2\x80\xA2
	static const char nl_bullet[] = "\n\xE2\x80\xA2 ";

	if (showKeyStats) {
		if (iret.keysExist > 0) {
			msg += nl_bullet;
			msg += rp_sprintf(NC_("KeyManagerTab",
				// tr: %s == number of keys (formatted)
				"%s key already exists in the Key Manager.",
				"%s keys already exist in the Key Manager.",
				iret.keysExist),
				sysLocale.toString(iret.keysExist).toUtf8().constData());
		}
		if (iret.keysInvalid > 0) {
			msg += nl_bullet;
			msg += rp_sprintf(NC_("KeyManagerTab",
				// tr: %s == number of keys (formatted)
				"%s key was not imported because it is incorrect.",
				"%s keys were not imported because they are incorrect.",
				iret.keysInvalid),
				sysLocale.toString(iret.keysInvalid).toUtf8().constData());
		}
		if (iret.keysNotUsed > 0) {
			msg += nl_bullet;
			msg += rp_sprintf(NC_("KeyManagerTab",
				// tr: %s == number of keys (formatted)
				"%s key was not imported because it isn't used by rom-properties.",
				"%s keys were not imported because they aren't used by rom-properties.",
				iret.keysNotUsed),
				sysLocale.toString(iret.keysNotUsed).toUtf8().constData());
		}
		if (iret.keysCantDecrypt > 0) {
			msg += nl_bullet;
			msg += rp_sprintf(NC_("KeyManagerTab",
				// tr: %s == number of keys (formatted)
				"%s key was not imported because it is encrypted and the master key isn't available.",
				"%s keys were not imported because they are encrypted and the master key isn't available.",
				iret.keysCantDecrypt),
				sysLocale.toString(iret.keysCantDecrypt).toUtf8().constData());
		}
		if (iret.keysImportedVerify > 0) {
			msg += nl_bullet;
			msg += rp_sprintf(NC_("KeyManagerTab",
				// tr: %s == number of keys (formatted)
				"%s key has been imported and verified as correct.",
				"%s keys have been imported and verified as correct.",
				iret.keysImportedVerify),
				sysLocale.toString(iret.keysImportedVerify).toUtf8().constData());
		}
		if (iret.keysImportedNoVerify > 0) {
			msg += nl_bullet;
			msg += rp_sprintf(NC_("KeyManagerTab",
				"%s key has been imported without verification.",
				"%s keys have been imported without verification.",
				iret.keysImportedNoVerify),
				sysLocale.toString(iret.keysImportedNoVerify).toUtf8().constData());
		}
	}

	Q_Q(KeyManagerTab);
	KMessageWidget *const widget = new KMessageWidget(q);
	widget->setCloseButtonVisible(true);
	widget->setWordWrap(true);
	widget->setMessageType(type);
	widget->setIcon(qApp->style()->standardIcon(icon, nullptr, widget));
	widget->setText(U82Q(msg));
	QObject::connect(widget, SIGNAL(hideAnimationFinished()),
			 widget, SLOT(deleteLater()));

	ui.vboxMain->insertWidget(0, widget);
	widget->animatedShow();
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
	QMenu *const menuImport = new QMenu(U82Q(C_("KeyManagerTab", "I&mport")), d->ui.btnImport);
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
	Q_D(KeyManagerTab);
	QString filename = QFileDialog::getOpenFileName(this,
		// tr: Wii keys.bin dialog title.
		U82Q(C_("KeyManagerTab", "Select Wii keys.bin File")),
		d->keyFileDir,	// dir
		// tr: Wii keys.bin file filter. (Qt)
		U82Q(C_("KeyManagerTab", "keys.bin (keys.bin);;Binary Files (*.bin);;All Files (*.*)")));
	if (filename.isEmpty())
		return;
	d->keyFileDir = QFileInfo(filename).canonicalPath();

	KeyStoreQt::ImportReturn iret = d->keyStore->importWiiKeysBin(Q2U8(filename));
	d->showKeyImportReturnStatus(filename, QLatin1String("Wii keys.bin"), iret);
}

/**
 * Import keys from Wii U otp.bin.
 */
void KeyManagerTab::on_actionImportWiiUOtpBin_triggered(void)
{
	Q_D(KeyManagerTab);
	QString filename = QFileDialog::getOpenFileName(this,
		// tr: Wii U otp.bin dialog title.
		U82Q(C_("KeyManagerTab", "Select Wii U otp.bin File")),
		d->keyFileDir,	// dir
		// tr: Wii U otp.bin file filter. (Qt)
		U82Q(C_("KeyManagerTab", "otp.bin (otp.bin);;Binary Files (*.bin);;All Files (*.*)")));
	if (filename.isEmpty())
		return;
	d->keyFileDir = QFileInfo(filename).canonicalPath();

	KeyStoreQt::ImportReturn iret = d->keyStore->importWiiUOtpBin(Q2U8(filename));
	d->showKeyImportReturnStatus(filename, QLatin1String("Wii U otp.bin"), iret);
}

/**
 * Import keys from 3DS boot9.bin.
 */
void KeyManagerTab::on_actionImport3DSboot9bin_triggered(void)
{
	Q_D(KeyManagerTab);
	QString filename = QFileDialog::getOpenFileName(this,
		// tr: 3DS boot9.bin dialog title.
		U82Q(C_("KeyManagerTab", "Select 3DS boot9.bin File")),
		d->keyFileDir,	// dir
		// tr: 3DS boot9.bin file filter. (Qt)
		U82Q(C_("KeyManagerTab", "boot9.bin (boot9.bin);;Binary Files (*.bin);;All Files (*.*)")));
	if (filename.isEmpty())
		return;
	d->keyFileDir = QFileInfo(filename).canonicalPath();

	KeyStoreQt::ImportReturn iret = d->keyStore->import3DSboot9bin(Q2U8(filename));
	d->showKeyImportReturnStatus(filename, QLatin1String("3DS boot9.bin"), iret);
}

/**
 * Import keys from 3DS aeskeydb.bin.
 */
void KeyManagerTab::on_actionImport3DSaeskeydb_triggered(void)
{
	Q_D(KeyManagerTab);
	QString filename = QFileDialog::getOpenFileName(this,
		// tr: 3DS aeskeydb.bin dialog title.
		U82Q(C_("KeyManagerTab", "Select 3DS aeskeydb.bin File")),
		d->keyFileDir,	// dir
		// tr: 3DS aeskeydb.bin file filter. (Qt)
		U82Q(C_("KeyManagerTab", "aeskeydb.bin (aeskeydb.bin);;Binary Files (*.bin);;All Files (*.*)")));
	if (filename.isEmpty())
		return;
	d->keyFileDir = QFileInfo(filename).canonicalPath();

	KeyStoreQt::ImportReturn iret = d->keyStore->import3DSaeskeydb(Q2U8(filename));
	d->showKeyImportReturnStatus(filename, QLatin1String("3DS aeskeydb.bin"), iret);
}
