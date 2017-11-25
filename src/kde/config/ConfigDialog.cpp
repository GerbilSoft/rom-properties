/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ConfigDialog.cpp: Configuration dialog.                                 *
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

#include "librpbase/config.librpbase.h"
#include "ConfigDialog.hpp"

// librpbase
#include "librpbase/config/Config.hpp"
using LibRpBase::Config;

// libi18n
#include "libi18n/i18n.h"

// U82Q()
#include "RpQt.hpp"

// C includes. (C++ namespace)
#include <cassert>

// Qt includes.
#include <QPushButton>
#include <QSettings>

#ifdef ENABLE_DECRYPTION
#include "KeyManagerTab.hpp"
#include "librpbase/crypto/KeyManager.hpp"
using LibRpBase::KeyManager;
#endif

/** ConfigDialogPrivate **/

#include "ui_ConfigDialog.h"
class ConfigDialogPrivate
{
	public:
		explicit ConfigDialogPrivate(ConfigDialog *q);
		~ConfigDialogPrivate();

	private:
		ConfigDialog *const q_ptr;
		Q_DECLARE_PUBLIC(ConfigDialog)
		Q_DISABLE_COPY(ConfigDialogPrivate)

	public:
		Ui::ConfigDialog ui;
#ifdef ENABLE_DECRYPTION
		KeyManagerTab *tabKeyManager;
		/**
		 * Retranslate parts of the UI that aren't present in the .ui file.
		 */
		void retranslateUi_nonDesigner(void);
#else /* !ENABLE_DECRYPTION */
		/**
		 * Retranslate parts of the UI that aren't present in the .ui file.
		 */
		inline void retranslateUi_nonDesigner(void) { }
#endif /* ENABLE_DECRYPTION */

		// "Apply", "Reset", and "Defaults" buttons.
		QPushButton *btnApply;
		QPushButton *btnReset;
		QPushButton *btnDefaults;

		// Last focused QWidget.
		QWidget *lastFocus;
};

ConfigDialogPrivate::ConfigDialogPrivate(ConfigDialog* q)
	: q_ptr(q)
#ifdef ENABLE_DECRYPTION
	, tabKeyManager(nullptr)
#endif /* ENABLE_DECRYPTION */
	, btnApply(nullptr)
	, btnReset(nullptr)
	, btnDefaults(nullptr)
	, lastFocus(nullptr)
{ }

ConfigDialogPrivate::~ConfigDialogPrivate()
{ }

#ifdef ENABLE_DECRYPTION
/**
 * Retranslate parts of the UI that aren't present in the .ui file.
 */
void ConfigDialogPrivate::retranslateUi_nonDesigner(void)
{
	ui.tabWidget->setTabText(ui.tabWidget->indexOf(tabKeyManager),
		U82Q(C_("ConfigDialog", "&Key Manager")));
}
#endif /* ENABLE_DECRYPTION */

/** ConfigDialog **/

/**
 * Initialize the configuration dialog.
 * @param parent Parent widget.
 */
ConfigDialog::ConfigDialog(QWidget *parent)
	: super(parent,
		Qt::Dialog |
		Qt::CustomizeWindowHint |
		Qt::WindowTitleHint |
		Qt::WindowSystemMenuHint |
		Qt::WindowMinimizeButtonHint |
		Qt::WindowCloseButtonHint)
	, d_ptr(new ConfigDialogPrivate(this))
{
	Q_D(ConfigDialog);
	d->ui.setupUi(this);

#ifdef ENABLE_DECRYPTION
	// Add the Key Manager tab.
	// NOTE: This isn't present in the .ui file because
	// we don't want a hard dependency on Key Manager.
	// Otherwise, no-crypto builds will break.
	d->tabKeyManager = new KeyManagerTab();
	d->tabKeyManager->setObjectName(QLatin1String("tabKeyManager"));
	// NOTE: The last tab is the "About" tab, so we need to
	// use insertTab() so it gets inserted before "About".
	d->ui.tabWidget->insertTab(d->ui.tabWidget->indexOf(d->ui.tabAbout),
		d->tabKeyManager, QString());
#endif /* ENABLE_DECRYPTION */

	// Retranslate non-Designer widgets.
	d->retranslateUi_nonDesigner();

	// Delete the window on close.
	this->setAttribute(Qt::WA_DeleteOnClose, true);

#ifdef Q_OS_MAC
	// Remove the window icon. (Mac "proxy icon")
	this->setWindowIcon(QIcon());
#else /* !Q_OS_MAC */
	// Set the icon from the system theme.
	// TODO: Fallback for older Qt?
#if QT_VERSION >= 0x040600
	QString iconName = QLatin1String("media-flash");
	if (QIcon::hasThemeIcon(iconName)) {
		this->setWindowIcon(QIcon::fromTheme(iconName));
	}
#endif /* QT_VERSION >= 0x040600 */
#endif /* Q_OS_MAC */

	// Cache the "Apply", "Reset", and "Defaults" buttons.
	d->btnApply = d->ui.buttonBox->button(QDialogButtonBox::Apply);
	d->btnReset = d->ui.buttonBox->button(QDialogButtonBox::Reset);
	d->btnDefaults = d->ui.buttonBox->button(QDialogButtonBox::RestoreDefaults);

	// FIXME: Set the "Reset" button's icon to "edit-undo". (Also something for Defaults.)
	// Attmepting to do this using d->btnApply->setIcon() doesn't seem to work...
	// See KDE5's System Settings for the correct icons.

	// Connect slots for "Apply" and "Reset".
	connect(d->btnApply, SIGNAL(clicked()), this, SLOT(apply()));
	connect(d->btnReset, SIGNAL(clicked()), this, SLOT(reset()));
	connect(d->btnDefaults, SIGNAL(clicked()), this, SLOT(loadDefaults()));

	// Disable the "Apply" and "Reset" buttons until we receive a modification signal.
	d->btnApply->setEnabled(false);
	d->btnReset->setEnabled(false);

	// Connect the modification signals.
	// FIXME: Should be doable in Qt Designer...
	connect(d->ui.tabImageTypes, SIGNAL(modified()), this, SLOT(tabModified()));
	connect(d->ui.tabDownloads, SIGNAL(modified()), this, SLOT(tabModified()));
#ifdef ENABLE_DECRYPTION
	connect(d->tabKeyManager, SIGNAL(modified()), this, SLOT(tabModified()));
#endif /* ENABLE_DECRYPTION */

	// Make sure the "Defaults" button has the correct state.
	on_tabWidget_currentChanged();

	// Install the event filter on all child widgets.
	// This is needed in order to track focus in case
	// the "Apply" button is clicked.
	QList<QWidget*> widgets = this->findChildren<QWidget*>();
	foreach (QWidget *widget, widgets) {
		widget->installEventFilter(this);
	}
}

/**
 * Shut down the save file editor.
 */
ConfigDialog::~ConfigDialog()
{
	delete d_ptr;
}

/**
 * Widget state has changed.
 * @param event State change event.
 */
void ConfigDialog::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(ConfigDialog);
		d->ui.retranslateUi(this);
		d->retranslateUi_nonDesigner();
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/**
 * Event filter for tracking focus.
 * @param watched Object.
 * @param event QEvent.
 * @return True to filter the event; false to allow it to propagate.
 */
bool ConfigDialog::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() == QEvent::FocusIn) {
		Q_D(ConfigDialog);
		QWidget *widget = qobject_cast<QWidget*>(watched);
		if (widget && widget != d->btnApply && widget != d->btnReset) {
			// Save the widget for refocusing if the
			// "Apply" button is clicked.
			d->lastFocus = widget;
		}
	}

	// Allow the event to propagate.
	return false;
}

/** Automatic slots from Qt Designer. **/

/**
 * The current tab has changed.
 * @param index New tab index.
 */
void ConfigDialog::on_tabWidget_currentChanged(void)
{
	// Enable/disable the "Defaults" button.
	Q_D(ConfigDialog);
	ITab *const tab = qobject_cast<ITab*>(d->ui.tabWidget->currentWidget());
	if (tab) {
		d->btnDefaults->setEnabled(tab->hasDefaults());
	} else {
		// Not an ITab...
		d->btnDefaults->setEnabled(false);
	}
}

/** Button slots. **/

/**
 * The "OK" button was clicked.
 */
void ConfigDialog::accept(void)
{
	// Save all tabs and close the dialog.
	// TODO: What if apply() fails?
	apply();
	super::accept();
}

/**
 * The "Apply" button was clicked.
 */
void ConfigDialog::apply(void)
{
	// Open the configuration file using QSettings.
	// TODO: Error handling.
	const Config *const config = Config::instance();
	const char *filename = config->filename();
	assert(filename != nullptr);
	if (!filename) {
		// No configuration filename...
		return;
	}

	QSettings settings(U82Q(filename), QSettings::IniFormat);
	if (!settings.isWritable()) {
		// Can't write to the file...
		return;
	}

	// Save all tabs.
	Q_D(ConfigDialog);
	d->ui.tabImageTypes->save(&settings);
	d->ui.tabDownloads->save(&settings);

#ifdef ENABLE_DECRYPTION
	// KeyManager needs to save to keys.conf.
	const KeyManager *const keyManager = KeyManager::instance();
	filename = keyManager->filename();
	assert(filename != nullptr);
	if (filename) {
		QSettings keys_conf(U82Q(filename), QSettings::IniFormat);
		if (keys_conf.isWritable()) {
			d->tabKeyManager->save(&keys_conf);
		}
	}
#endif /* ENABLE_DECRYPTION */

	// Set the focus to the last-focused widget.
	// Otherwise, it ends up focusing the "Cancel" button.
	if (d->lastFocus) {
		d->lastFocus->setFocus();
	}

	// Disable the "Apply" and "Reset" buttons.
	d->btnApply->setEnabled(false);
	d->btnReset->setEnabled(false);
}

/**
 * The "Reset" button was clicked.
 */
void ConfigDialog::reset(void)
{
	// Reset all tabs.
	Q_D(ConfigDialog);
	d->ui.tabImageTypes->reset();
	d->ui.tabDownloads->reset();
#ifdef ENABLE_DECRYPTION
	d->tabKeyManager->reset();
#endif /* ENABLE_DECRYPTION */

	// Set the focus to the last-focused widget.
	// Otherwise, it ends up focusing the "Cancel" button.
	if (d->lastFocus) {
		d->lastFocus->setFocus();
	}

	// Disable the "Apply" and "Reset" buttons.
	d->btnApply->setEnabled(false);
	d->btnReset->setEnabled(false);
}

/**
 * The "Defaults" button was clicked.
 */
void ConfigDialog::loadDefaults(void)
{
	Q_D(ConfigDialog);
	switch (d->ui.tabWidget->currentIndex()) {
		case 0:
			d->ui.tabImageTypes->loadDefaults();
			break;
		case 1:
			d->ui.tabDownloads->loadDefaults();
			break;
		default:
			assert(!"Unrecognized tab index.");
			break;
	}
}

/**
 * A tab has been modified.
 */
void ConfigDialog::tabModified(void)
{
	// Enable the "Apply" and "Reset" buttons.
	Q_D(ConfigDialog);
	d->btnApply->setEnabled(true);
	d->btnReset->setEnabled(true);
}
