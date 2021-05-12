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
		static const int pal_lc_idx_def;	// Default index in pal_lc[].
		static const uint32_t pal_lc[];
};

/** OptionsTabPrivate **/

// PAL language codes for GameTDB.
// NOTE: 'au' is technically not a language code, but
// GameTDB handles it as a separate language.
const int OptionsTabPrivate::pal_lc_idx_def = 2;
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
	d->ui.chkExtImgDownloadEnabled->setChecked(config->extImgDownloadEnabled());
	d->ui.chkUseIntIconForSmallSizes->setChecked(config->useIntIconForSmallSizes());
	d->ui.chkDownloadHighResScans->setChecked(config->downloadHighResScans());
	d->ui.chkStoreFileOriginInfo->setChecked(config->storeFileOriginInfo());

	// Options
	d->ui.chkShowDangerousPermissionsOverlayIcon->setChecked(config->showDangerousPermissionsOverlayIcon());
	d->ui.chkEnableThumbnailOnNetworkFS->setChecked(config->enableThumbnailOnNetworkFS());

	// PAL language code
	const uint32_t lc = config->palLanguageForGameTDB();
	int idx = 0;
	for (; idx < ARRAY_SIZE(d->pal_lc); idx++) {
		if (d->pal_lc[idx] == lc)
			break;
	}
	if (idx >= ARRAY_SIZE(d->pal_lc)) {
		// Out of range. Default to 'en'.
		idx = d->pal_lc_idx_def;
	}
	d->ui.cboGameTDBPAL->setCurrentIndex(idx);
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
	static const bool downloadHighResScans_default = true;
	static const bool storeFileOriginInfo_default = true;
	static const int palLanguageForGameTDB_default =
		OptionsTabPrivate::pal_lc_idx_def;	// cboGameTDBPAL index ('en')
	// Options
	static const bool showDangerousPermissionsOverlayIcon_default = true;
	static const bool enableThumbnailOnNetworkFS = false;
	bool isDefChanged = false;

	Q_D(OptionsTab);

	// Downloads
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
	if (d->ui.cboGameTDBPAL->currentIndex() != palLanguageForGameTDB_default) {
		d->ui.cboGameTDBPAL->setCurrentIndex(palLanguageForGameTDB_default);
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

	// Downloads
	pSettings->beginGroup(QLatin1String("Downloads"));
	pSettings->setValue(QLatin1String("ExtImageDownload"),
		d->ui.chkExtImgDownloadEnabled->isChecked());
	pSettings->setValue(QLatin1String("UseIntIconForSmallSizes"),
		d->ui.chkUseIntIconForSmallSizes->isChecked());
	pSettings->setValue(QLatin1String("DownloadHighResScans"),
		d->ui.chkDownloadHighResScans->isChecked());
	pSettings->setValue(QLatin1String("StoreFileOriginInfo"),
		d->ui.chkStoreFileOriginInfo->isChecked());
	// NOTE: QComboBox::currentData() was added in Qt 5.2.
	pSettings->setValue(QLatin1String("PalLanguageForGameTDB"),
		lcToQString(d->ui.cboGameTDBPAL->itemData(d->ui.cboGameTDBPAL->currentIndex()).toUInt()));
	pSettings->endGroup();

	// Options
	pSettings->beginGroup(QLatin1String("Options"));
	pSettings->setValue(QLatin1String("ShowDangerousPermissionsOverlayIcon"),
		d->ui.chkShowDangerousPermissionsOverlayIcon->isChecked());
	pSettings->setValue(QLatin1String("EnableThumbnailOnNetworkFS"),
		d->ui.chkEnableThumbnailOnNetworkFS->isChecked());
	pSettings->endGroup();
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
