/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * DownloadsTab.cpp: Downloads tab for rp-config.                          *
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

#include "DownloadsTab.hpp"

// librpbase
#include "librpbase/config/Config.hpp"
using LibRpBase::Config;

// C includes. (C++ namespace)
#include <cassert>

#include "ui_DownloadsTab.h"
class DownloadsTabPrivate
{
	public:
		explicit DownloadsTabPrivate(DownloadsTab *q);

	private:
		DownloadsTab *const q_ptr;
		Q_DECLARE_PUBLIC(DownloadsTab)
		Q_DISABLE_COPY(DownloadsTabPrivate)

	public:
		Ui::DownloadsTab ui;

	public:
		// Has the user changed anything?
		bool changed;
};

/** DownloadsTabPrivate **/

DownloadsTabPrivate::DownloadsTabPrivate(DownloadsTab* q)
	: q_ptr(q)
	, changed(false)
{ }

/** DownloadsTab **/

DownloadsTab::DownloadsTab(QWidget *parent)
	: super(parent)
	, d_ptr(new DownloadsTabPrivate(this))
{
	Q_D(DownloadsTab);
	d->ui.setupUi(this);

	// Load the current configuration.
	reset();
}

DownloadsTab::~DownloadsTab()
{
	delete d_ptr;
}

/**
 * Widget state has changed.
 * @param event State change event.
 */
void DownloadsTab::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(DownloadsTab);
		d->ui.retranslateUi(this);
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/**
 * Reset the configuration.
 */
void DownloadsTab::reset(void)
{
	// NOTE: This may re-check the configuration timestamp.
	const Config *const config = Config::instance();

	Q_D(DownloadsTab);
	d->ui.chkExtImgDownloadEnabled->setChecked(config->extImgDownloadEnabled());
	d->ui.chkUseIntIconForSmallSizes->setChecked(config->useIntIconForSmallSizes());
	d->ui.chkDownloadHighResScans->setChecked(config->downloadHighResScans());
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void DownloadsTab::loadDefaults(void)
{
	// TODO: Get the defaults from Config.
	// For now, hard-coding everything here.
	static const bool extImgDownloadEnabled_default = true;
	static const bool useIntIconForSmallSizes_default = true;
	static const bool downloadHighResScans_default = true;
	bool isDefChanged = false;

	Q_D(DownloadsTab);
	if (d->ui.chkExtImgDownloadEnabled->isChecked() != extImgDownloadEnabled_default) {
		d->ui.chkExtImgDownloadEnabled->setChecked(extImgDownloadEnabled_default);
		isDefChanged = true;
	}
	if (d->ui.chkUseIntIconForSmallSizes->isChecked() != useIntIconForSmallSizes_default) {
		d->ui.chkUseIntIconForSmallSizes->setChecked(useIntIconForSmallSizes_default);
		isDefChanged = true;
	}
	if (d->ui.chkDownloadHighResScans->isChecked() != downloadHighResScans_default) {
		d->ui.chkDownloadHighResScans->setChecked(downloadHighResScans_default);
		isDefChanged = true;
	}

	if (isDefChanged) {
		d->changed = true;
		emit modified();
	}
}

/**
 * Save the configuration.
 * @param pSettings QSettings object.
 */
void DownloadsTab::save(QSettings *pSettings)
{
	assert(pSettings != nullptr);
	if (!pSettings)
		return;

	// Save the configuration.
	Q_D(const DownloadsTab);
	pSettings->beginGroup(QLatin1String("Downloads"));
	pSettings->setValue(QLatin1String("ExtImageDownload"),
		d->ui.chkExtImgDownloadEnabled->isChecked());
	pSettings->setValue(QLatin1String("UseIntIconForSmallSizes"),
		d->ui.chkUseIntIconForSmallSizes->isChecked());
	pSettings->setValue(QLatin1String("DownloadHighResScans"),
		d->ui.chkDownloadHighResScans->isChecked());
	pSettings->endGroup();
}

/**
 * A checkbox was clicked.
 */
void DownloadsTab::checkBox_clicked(void)
{
	// Configuration has been changed.
	Q_D(DownloadsTab);
	d->changed = true;
	emit modified();
}
