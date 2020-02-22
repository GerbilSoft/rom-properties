/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * OptionsTab.cpp: Options tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "OptionsTab.hpp"

// librpbase
using LibRpBase::Config;

#include "ui_OptionsTab.h"
class OptionsTabPrivate
{
	public:
		explicit OptionsTabPrivate();

	private:
		Q_DISABLE_COPY(OptionsTabPrivate)

	public:
		Ui::OptionsTab ui;

	public:
		// Has the user changed anything?
		bool changed;
};

/** OptionsTabPrivate **/

OptionsTabPrivate::OptionsTabPrivate()
	: changed(false)
{ }

/** OptionsTab **/

OptionsTab::OptionsTab(QWidget *parent)
	: super(parent)
	, d_ptr(new OptionsTabPrivate())
{
	Q_D(OptionsTab);
	d->ui.setupUi(this);

	// Load the current configuration.
	reset();
}

OptionsTab::~OptionsTab()
{
	delete d_ptr;
}

/**
 * Widget state has changed.
 * @param event State change event.
 */
void OptionsTab::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(OptionsTab);
		d->ui.retranslateUi(this);
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/**
 * Reset the configuration.
 */
void OptionsTab::reset(void)
{
	// NOTE: This may re-check the configuration timestamp.
	const Config *const config = Config::instance();

	Q_D(OptionsTab);
	d->ui.chkExtImgDownloadEnabled->setChecked(config->extImgDownloadEnabled());
	d->ui.chkUseIntIconForSmallSizes->setChecked(config->useIntIconForSmallSizes());
	d->ui.chkDownloadHighResScans->setChecked(config->downloadHighResScans());
	d->ui.chkStoreFileOriginInfo->setChecked(config->storeFileOriginInfo());
	d->ui.chkShowDangerousPermissionsOverlayIcon->setChecked(config->showDangerousPermissionsOverlayIcon());
	d->ui.chkEnableThumbnailOnNetworkFS->setChecked(config->enableThumbnailOnNetworkFS());
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void OptionsTab::loadDefaults(void)
{
	// TODO: Get the defaults from Config.
	// For now, hard-coding everything here.
	static const bool extImgDownloadEnabled_default = true;
	static const bool useIntIconForSmallSizes_default = true;
	static const bool downloadHighResScans_default = true;
	static const bool storeFileOriginInfo_default = true;
	static const bool showDangerousPermissionsOverlayIcon_default = true;
	static const bool enableThumbnailOnNetworkFS = false;
	bool isDefChanged = false;

	Q_D(OptionsTab);
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
	if (d->ui.chkStoreFileOriginInfo->isChecked() != storeFileOriginInfo_default) {
		d->ui.chkStoreFileOriginInfo->setChecked(storeFileOriginInfo_default);
		isDefChanged = true;
	}
	if (d->ui.chkShowDangerousPermissionsOverlayIcon->isChecked() !=
		showDangerousPermissionsOverlayIcon_default)
	{
		d->ui.chkShowDangerousPermissionsOverlayIcon->setChecked(
			showDangerousPermissionsOverlayIcon_default);
		isDefChanged = true;
	}
	if (d->ui.chkEnableThumbnailOnNetworkFS->isChecked() != enableThumbnailOnNetworkFS) {
		d->ui.chkEnableThumbnailOnNetworkFS->setChecked(enableThumbnailOnNetworkFS);
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
void OptionsTab::save(QSettings *pSettings)
{
	assert(pSettings != nullptr);
	if (!pSettings)
		return;

	// Save the configuration.
	Q_D(const OptionsTab);
	pSettings->beginGroup(QLatin1String("Downloads"));
	pSettings->setValue(QLatin1String("ExtImageDownload"),
		d->ui.chkExtImgDownloadEnabled->isChecked());
	pSettings->setValue(QLatin1String("UseIntIconForSmallSizes"),
		d->ui.chkUseIntIconForSmallSizes->isChecked());
	pSettings->setValue(QLatin1String("DownloadHighResScans"),
		d->ui.chkDownloadHighResScans->isChecked());
	pSettings->setValue(QLatin1String("StoreFileOriginInfo"),
		d->ui.chkStoreFileOriginInfo->isChecked());
	pSettings->endGroup();

	pSettings->beginGroup(QLatin1String("Options"));
	pSettings->setValue(QLatin1String("ShowDangerousPermissionsOverlayIcon"),
		d->ui.chkShowDangerousPermissionsOverlayIcon->isChecked());
	pSettings->setValue(QLatin1String("EnableThumbnailOnNetworkFS"),
		d->ui.chkEnableThumbnailOnNetworkFS->isChecked());
	pSettings->endGroup();
}

/**
 * A checkbox was clicked.
 */
void OptionsTab::checkBox_clicked(void)
{
	// Configuration has been changed.
	Q_D(OptionsTab);
	d->changed = true;
	printf("MOO\n");
	emit modified();
}
