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
};

/** OptionsTabPrivate **/

// PAL language codes for GameTDB.
// NOTE: 'au' is technically not a language code, but
// GameTDB handles it as a separate language.
const uint32_t OptionsTabPrivate::pal_lc[] = {'au', 'de', 'en', 'es', 'fr', 'it', 'nl', 'pt', 'ru'};

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
	d->ui.grpExtImgDownloads->setChecked(config->getBoolConfigOption(Config::BoolConfig::Downloads_ExtImgDownloadEnabled));
	d->ui.chkUseIntIconForSmallSizes->setChecked(config->getBoolConfigOption(Config::BoolConfig::Downloads_UseIntIconForSmallSizes));
	d->ui.chkStoreFileOriginInfo->setChecked(config->getBoolConfigOption(Config::BoolConfig::Downloads_StoreFileOriginInfo));

	// Image bandwidth options
	d->ui.cboUnmeteredConnection->setCurrentIndex(static_cast<int>(config->imgBandwidthUnmetered()));
	d->ui.cboMeteredConnection->setCurrentIndex(static_cast<int>(config->imgBandwidthMetered()));

	// Options
	d->ui.chkShowDangerousPermissionsOverlayIcon->setChecked(config->getBoolConfigOption(Config::BoolConfig::Options_ShowDangerousPermissionsOverlayIcon));
	d->ui.chkEnableThumbnailOnNetworkFS->setChecked(config->getBoolConfigOption(Config::BoolConfig::Options_EnableThumbnailOnNetworkFS));
	d->ui.chkThumbnailDirectoryPackages->setChecked(config->getBoolConfigOption(Config::BoolConfig::Options_ThumbnailDirectoryPackages));
	d->ui.chkShowXAttrView->setChecked(config->getBoolConfigOption(Config::BoolConfig::Options_ShowXAttrView));

	// PAL language code
	d->ui.cboGameTDBPAL->setSelectedLC(config->palLanguageForGameTDB());

	d->changed = false;
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void OptionsTab::loadDefaults(void)
{
	// Has any value changed due to resetting to defaults?
	bool isDefChanged = false;

	Q_D(OptionsTab);

	// Downloads
	bool bdef = Config::getBoolConfigOption_default(Config::BoolConfig::Downloads_ExtImgDownloadEnabled);
	if (d->ui.grpExtImgDownloads->isChecked() != bdef) {
		d->ui.grpExtImgDownloads->setChecked(bdef);
		isDefChanged = true;
	}
	bdef = Config::getBoolConfigOption_default(Config::BoolConfig::Downloads_UseIntIconForSmallSizes);
	if (d->ui.chkUseIntIconForSmallSizes->isChecked() != bdef) {
		d->ui.chkUseIntIconForSmallSizes->setChecked(bdef);
		isDefChanged = true;
	}
	bdef = Config::getBoolConfigOption_default(Config::BoolConfig::Downloads_StoreFileOriginInfo);
	if (d->ui.chkStoreFileOriginInfo->isChecked() != bdef) {
		d->ui.chkStoreFileOriginInfo->setChecked(bdef);
		isDefChanged = true;
	}
	const uint32_t u32def = Config::palLanguageForGameTDB_default();
	if (d->ui.cboGameTDBPAL->selectedLC() != u32def) {
		d->ui.cboGameTDBPAL->setSelectedLC(u32def);
		isDefChanged = true;
	}

	// Image bandwidth options
	int idef = static_cast<int>(Config::imgBandwidthUnmetered_default());
	if (d->ui.cboUnmeteredConnection->currentIndex() != idef) {
		d->ui.cboUnmeteredConnection->setCurrentIndex(idef);
		isDefChanged = true;
	}
	idef = static_cast<int>(Config::imgBandwidthMetered_default());
	if (d->ui.cboMeteredConnection->currentIndex() != idef) {
		d->ui.cboMeteredConnection->setCurrentIndex(idef);
		isDefChanged = true;
	}

	// Options
	bdef = Config::getBoolConfigOption_default(Config::BoolConfig::Options_ShowDangerousPermissionsOverlayIcon);
	if (d->ui.chkShowDangerousPermissionsOverlayIcon->isChecked() != bdef) {
		d->ui.chkShowDangerousPermissionsOverlayIcon->setChecked(bdef);
		isDefChanged = true;
	}
	bdef = Config::getBoolConfigOption_default(Config::BoolConfig::Options_EnableThumbnailOnNetworkFS);
	if (d->ui.chkEnableThumbnailOnNetworkFS->isChecked() != bdef) {
		d->ui.chkEnableThumbnailOnNetworkFS->setChecked(bdef);
		isDefChanged = true;
	}
	bdef = Config::getBoolConfigOption_default(Config::BoolConfig::Options_ThumbnailDirectoryPackages);
	if (d->ui.chkThumbnailDirectoryPackages->isChecked() != bdef) {
		d->ui.chkThumbnailDirectoryPackages->setChecked(bdef);
		isDefChanged = true;
	}
	bdef = Config::getBoolConfigOption_default(Config::BoolConfig::Options_ShowXAttrView);
	if (d->ui.chkShowXAttrView->isChecked() != bdef) {
		d->ui.chkShowXAttrView->setChecked(bdef);
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
	pSettings->setValue(QLatin1String("ThumbnailDirectoryPackages"),
		d->ui.chkThumbnailDirectoryPackages->isChecked());
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
