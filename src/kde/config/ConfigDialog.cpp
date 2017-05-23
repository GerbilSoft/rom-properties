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

// Qt includes.
#include <QPushButton>

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
};

ConfigDialogPrivate::ConfigDialogPrivate(ConfigDialog* q)
	: q_ptr(q)
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

	// Connect slots for "Reset" and "Apply".
	connect(d->ui.buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()),
		this, SLOT(apply()));
	connect(d->ui.buttonBox->button(QDialogButtonBox::Reset), SIGNAL(clicked()),
		this, SLOT(reset()));

	// Disable the "Apply" button until we receive a modification signal.
	d->ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

	// Connect the modification signals.
	// FIXME: Should be doable in Qt Designer...
	connect(d->ui.tabImageTypes, SIGNAL(modified()),
		this, SLOT(tabModified()));
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

/** Button slots. **/

/**
 * The "OK" button was clicked.
 */
void ConfigDialog::accept(void)
{
	// Save all tabs and close the dialog.
	Q_D(ConfigDialog);
	d->ui.tabImageTypes->save();
	super::accept();
}

/**
 * The "Apply" button was clicked.
 */
void ConfigDialog::apply(void)
{
	// Save all tabs.
	Q_D(ConfigDialog);
	d->ui.tabImageTypes->save();

	// Disable the "Apply" button.
	d->ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

/**
 * The "Reset" button was clicked.
 */
void ConfigDialog::reset(void)
{
	// Reset all tabs.
	Q_D(ConfigDialog);
	d->ui.tabImageTypes->reset();

	// Disable the "Apply" button.
	d->ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

/**
 * A tab has been modified.
 */
void ConfigDialog::tabModified(void)
{
	// Enable the "Apply" button.
	Q_D(ConfigDialog);
	d->ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}
