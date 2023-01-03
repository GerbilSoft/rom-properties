/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * OptionsTab.cpp: Options tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
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

		// PAL language codes for GameTDB.
		static const uint32_t pal_lc[];
		static const int pal_lc_idx_def;	// Default index in pal_lc[].
};

/** OptionsTabPrivate **/

// PAL language codes for GameTDB.
// NOTE: 'au' is technically not a language code, but
// GameTDB handles it as a separate language.
const uint32_t OptionsTabPrivate::pal_lc[] = {'au', 'de', 'en', 'es', 'fr', 'it', 'nl', 'pt', 'ru'};
const int OptionsTabPrivate::pal_lc_idx_def = 2;

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

	// Initialize the PAL language dropdown.
	d->ui.cboGameTDBPAL->setForcePAL(true);
	d->ui.cboGameTDBPAL->setLCs(OptionsTabPrivate::pal_lc, ARRAY_SIZE(OptionsTabPrivate::pal_lc));

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

	// Downloads
	d->ui.grpExtImgDownloads->setChecked(config->extImgDownloadEnabled());
	d->ui.chkUseIntIconForSmallSizes->setChecked(config->useIntIconForSmallSizes());
	d->ui.chkStoreFileOriginInfo->setChecked(config->storeFileOriginInfo());

	// Image bandwidth options
	d->ui.cboUnmeteredConnection->setCurrentIndex(static_cast<int>(config->imgBandwidthUnmetered()));
	d->ui.cboMeteredConnection->setCurrentIndex(static_cast<int>(config->imgBandwidthMetered()));

	// Options
	d->ui.chkShowDangerousPermissionsOverlayIcon->setChecked(config->showDangerousPermissionsOverlayIcon());
	d->ui.chkEnableThumbnailOnNetworkFS->setChecked(config->enableThumbnailOnNetworkFS());
	d->ui.chkShowXAttrView->setChecked(config->showXAttrView());

	// PAL language code
	const uint32_t lc = config->palLanguageForGameTDB();
	int idx = 0;
	for (; idx < ARRAY_SIZE_I(d->pal_lc); idx++) {
		if (d->pal_lc[idx] == lc)
			break;
	}
	if (idx >= ARRAY_SIZE_I(d->pal_lc)) {
		// Out of range. Default to 'en'.
		idx = d->pal_lc_idx_def;
	}
	d->ui.cboGameTDBPAL->setCurrentIndex(idx);

	d->changed = false;
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

	// Downloads
	static const bool extImgDownloadEnabled_default = true;
	static const bool useIntIconForSmallSizes_default = true;
	static const bool storeFileOriginInfo_default = true;
	static const int palLanguageForGameTDB_default =
		OptionsTabPrivate::pal_lc_idx_def;	// cboGameTDBPAL index ('en')

	// Image bandwidth options
	static const Config::ImgBandwidth imgBandwidthUnmetered_default = Config::ImgBandwidth::HighRes;
	static const Config::ImgBandwidth imgBandwidthMetered_default = Config::ImgBandwidth::NormalRes;

	// Options
	static const bool showDangerousPermissionsOverlayIcon_default = true;
	static const bool enableThumbnailOnNetworkFS_default = false;
	static const bool showXAttrView_default = true;
	bool isDefChanged = false;

	Q_D(OptionsTab);

	// Downloads
	if (d->ui.grpExtImgDownloads->isChecked() != extImgDownloadEnabled_default) {
		d->ui.grpExtImgDownloads->setChecked(extImgDownloadEnabled_default);
		isDefChanged = true;
	}
	if (d->ui.chkUseIntIconForSmallSizes->isChecked() != useIntIconForSmallSizes_default) {
		d->ui.chkUseIntIconForSmallSizes->setChecked(useIntIconForSmallSizes_default);
		isDefChanged = true;
	}
	if (d->ui.chkStoreFileOriginInfo->isChecked() != storeFileOriginInfo_default) {
		d->ui.chkStoreFileOriginInfo->setChecked(storeFileOriginInfo_default);
		isDefChanged = true;
	}
	if (d->ui.cboGameTDBPAL->currentIndex() != palLanguageForGameTDB_default) {
		d->ui.cboGameTDBPAL->setCurrentIndex(palLanguageForGameTDB_default);
		isDefChanged = true;
	}

	// Image bandwidth options
	if (d->ui.cboUnmeteredConnection->currentIndex() != static_cast<int>(imgBandwidthUnmetered_default)) {
		d->ui.cboUnmeteredConnection->setCurrentIndex(static_cast<int>(imgBandwidthUnmetered_default));
		isDefChanged = true;
	}
	if (d->ui.cboMeteredConnection->currentIndex() != static_cast<int>(imgBandwidthMetered_default)) {
		d->ui.cboMeteredConnection->setCurrentIndex(static_cast<int>(imgBandwidthMetered_default));
		isDefChanged = true;
	}

	// Options
	if (d->ui.chkShowDangerousPermissionsOverlayIcon->isChecked() !=
		showDangerousPermissionsOverlayIcon_default)
	{
		d->ui.chkShowDangerousPermissionsOverlayIcon->setChecked(
			showDangerousPermissionsOverlayIcon_default);
		isDefChanged = true;
	}
	if (d->ui.chkEnableThumbnailOnNetworkFS->isChecked() != enableThumbnailOnNetworkFS_default) {
		d->ui.chkEnableThumbnailOnNetworkFS->setChecked(enableThumbnailOnNetworkFS_default);
		isDefChanged = true;
	}
	if (d->ui.chkShowXAttrView->isChecked() != showXAttrView_default) {
		d->ui.chkShowXAttrView->setChecked(showXAttrView_default);
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

	Q_D(OptionsTab);
	if (!d->changed) {
		// Configuration was not changed.
		return;
	}

	// Save the configuration.

	// Downloads
	pSettings->beginGroup(QLatin1String("Downloads"));
	pSettings->setValue(QLatin1String("ExtImageDownload"),
		d->ui.grpExtImgDownloads->isChecked());
	pSettings->setValue(QLatin1String("UseIntIconForSmallSizes"),
		d->ui.chkUseIntIconForSmallSizes->isChecked());
	pSettings->setValue(QLatin1String("StoreFileOriginInfo"),
		d->ui.chkStoreFileOriginInfo->isChecked());
	// NOTE: QComboBox::currentData() was added in Qt 5.2.
	pSettings->setValue(QLatin1String("PalLanguageForGameTDB"),
		lcToQString(d->ui.cboGameTDBPAL->itemData(d->ui.cboGameTDBPAL->currentIndex()).toUInt()));

	// Image bandwidth options
	// TODO: Consolidate this.
	const char *sUnmetered, *sMetered;
	switch (static_cast<Config::ImgBandwidth>(d->ui.cboUnmeteredConnection->currentIndex())) {
		case Config::ImgBandwidth::None:
			sUnmetered = "None";
			break;
		case Config::ImgBandwidth::NormalRes:
			sUnmetered = "NormalRes";
			break;
		case Config::ImgBandwidth::HighRes:
		default:
			sUnmetered = "HighRes";
			break;
	}
	switch (static_cast<Config::ImgBandwidth>(d->ui.cboMeteredConnection->currentIndex())) {
		case Config::ImgBandwidth::None:
			sMetered = "None";
			break;
		case Config::ImgBandwidth::NormalRes:
		default:
			sMetered = "NormalRes";
			break;
		case Config::ImgBandwidth::HighRes:
			sMetered = "HighRes";
			break;
	}
	pSettings->setValue(QLatin1String("ImgBandwidthUnmetered"), QLatin1String(sUnmetered));
	pSettings->setValue(QLatin1String("ImgBandwidthMetered"), QLatin1String(sMetered));
	// Remove the old option
	pSettings->remove(QLatin1String("DownloadHighResScans"));
	pSettings->endGroup();

	// Options
	pSettings->beginGroup(QLatin1String("Options"));
	pSettings->setValue(QLatin1String("ShowDangerousPermissionsOverlayIcon"),
		d->ui.chkShowDangerousPermissionsOverlayIcon->isChecked());
	pSettings->setValue(QLatin1String("EnableThumbnailOnNetworkFS"),
		d->ui.chkEnableThumbnailOnNetworkFS->isChecked());
	pSettings->setValue(QLatin1String("ShowXAttrView"),
		d->ui.chkShowXAttrView->isChecked());
	pSettings->endGroup();

	// Configuration saved.
	d->changed = false;
}

/**
 * An option was changed.
 */
void OptionsTab::optionChanged_slot(void)
{
	// Configuration has been changed.
	Q_D(OptionsTab);
	d->changed = true;
	emit modified();
}
