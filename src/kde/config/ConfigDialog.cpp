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

#include "ConfigDialog.hpp"

// librpbase
#include "librpbase/config/Config.hpp"
using LibRpBase::Config;

// RP2Q()
#include "RpQt.hpp"

// C includes. (C++ namespace)
#include <cassert>

// Qt includes.
#include <QPushButton>
#include <QSettings>

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

		// "Apply", "Reset", and "Defaults" buttons.
		QPushButton *btnApply;
		QPushButton *btnReset;
		QPushButton *btnDefaults;

		// Last focused QWidget.
		QWidget *lastFocus;
};

ConfigDialogPrivate::ConfigDialogPrivate(ConfigDialog* q)
	: q_ptr(q)
	, btnApply(nullptr)
	, btnReset(nullptr)
	, btnDefaults(nullptr)
	, lastFocus(nullptr)
{ }

ConfigDialogPrivate::~ConfigDialogPrivate()
{ }

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
		if (widget && widget != d->btnApply) {
			// Save the widget for refocusing if the
			// "Apply" button is clicked.
			d->lastFocus = widget;
		}
	}

	// Allow the event to propagate.
	return false;
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
	const rp_char *const filename = config->filename();
	if (!filename) {
		// No configuration filename...
		return;
	}

	QSettings settings(RP2Q(filename), QSettings::IniFormat);
	if (!settings.isWritable()) {
		// Can't write to the file...
		return;
	}

	// Save all tabs.
	Q_D(ConfigDialog);
	d->ui.tabImageTypes->save(&settings);

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
