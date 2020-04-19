/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * DMG.cpp: Game Boy tab for rp-config.                                    *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DMGTab.hpp"

// librpbase
using LibRpBase::Config;

#include "ui_DMGTab.h"
class DMGTabPrivate
{
	public:
		explicit DMGTabPrivate();

	private:
		Q_DISABLE_COPY(DMGTabPrivate)

	public:
		Ui::DMGTab ui;

	public:
		// Has the user changed anything?
		bool changed;
};

/** DMGTabPrivate **/

DMGTabPrivate::DMGTabPrivate()
	: changed(false)
{ }

/** DMGTab **/

DMGTab::DMGTab(QWidget *parent)
	: super(parent)
	, d_ptr(new DMGTabPrivate())
{
	Q_D(DMGTab);
	d->ui.setupUi(this);

	// Load the current configuration.
	reset();
}

DMGTab::~DMGTab()
{
	delete d_ptr;
}

/**
 * Widget state has changed.
 * @param event State change event.
 */
void DMGTab::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(DMGTab);
		d->ui.retranslateUi(this);
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/**
 * Reset the configuration.
 */
void DMGTab::reset(void)
{
	// NOTE: This may re-check the configuration timestamp.
	const Config *const config = Config::instance();

	Q_D(DMGTab);

	// Block signals while reloading.
	const bool blockDMG = d->ui.cboDMG->blockSignals(true);
	const bool blockSGB = d->ui.cboSGB->blockSignals(true);
	const bool blockCGB = d->ui.cboCGB->blockSignals(true);

	// Special handling: DMG as SGB doesn't really make sense,
	// so handle it as DMG.
	const Config::DMG_TitleScreen_Mode tsMode =
		config->dmgTitleScreenMode(Config::DMG_TitleScreen_Mode::DMG_TS_DMG);
	switch (tsMode) {
		case Config::DMG_TitleScreen_Mode::DMG_TS_DMG:
		case Config::DMG_TitleScreen_Mode::DMG_TS_SGB:
		default:
			d->ui.cboDMG->setCurrentIndex(0);
			break;
		case Config::DMG_TitleScreen_Mode::DMG_TS_CGB:
			d->ui.cboDMG->setCurrentIndex(1);
			break;
	}

	// The SGB and CGB dropdowns have all three.
	d->ui.cboSGB->setCurrentIndex((int)config->dmgTitleScreenMode(Config::DMG_TitleScreen_Mode::DMG_TS_SGB));
	d->ui.cboCGB->setCurrentIndex((int)config->dmgTitleScreenMode(Config::DMG_TitleScreen_Mode::DMG_TS_CGB));

	// Restore the signal block state.
	d->ui.cboDMG->blockSignals(blockDMG);
	d->ui.cboSGB->blockSignals(blockSGB);
	d->ui.cboCGB->blockSignals(blockCGB);
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void DMGTab::loadDefaults(void)
{
	// TODO: Get the defaults from Config.
	// For now, hard-coding everything here.
	static const int8_t idxDMG_default = 0;
	static const int8_t idxSGB_default = 1;
	static const int8_t idxCGB_default = 2;
	bool isDefChanged = false;

	Q_D(DMGTab);

	// Block signals while reloading.
	const bool blockDMG = d->ui.cboDMG->blockSignals(true);
	const bool blockSGB = d->ui.cboSGB->blockSignals(true);
	const bool blockCGB = d->ui.cboCGB->blockSignals(true);

	if (d->ui.cboDMG->currentIndex() != idxDMG_default) {
		d->ui.cboDMG->setCurrentIndex(idxDMG_default);
		isDefChanged = true;
	}
	if (d->ui.cboSGB->currentIndex() != idxSGB_default) {
		d->ui.cboSGB->setCurrentIndex(idxSGB_default);
		isDefChanged = true;
	}
	if (d->ui.cboCGB->currentIndex() != idxCGB_default) {
		d->ui.cboDMG->setCurrentIndex(idxCGB_default);
		isDefChanged = true;
	}

	if (isDefChanged) {
		d->changed = true;
		emit modified();
	}

	// Restore the signal block state.
	d->ui.cboDMG->blockSignals(blockDMG);
	d->ui.cboSGB->blockSignals(blockSGB);
	d->ui.cboCGB->blockSignals(blockCGB);
}

/**
 * Save the configuration.
 * @param pSettings QSettings object.
 */
void DMGTab::save(QSettings *pSettings)
{
	assert(pSettings != nullptr);
	if (!pSettings)
		return;

	// Save the configuration.
	Q_D(const DMGTab);
	pSettings->beginGroup(QLatin1String("DMGTitleScreenMode"));

	const char s_dmg_dmg[][4] = {"DMG", "CGB"};
	const int idxDMG = d->ui.cboDMG->currentIndex();
	assert(idxDMG >= 0);
	assert(idxDMG < ARRAY_SIZE(s_dmg_dmg));
	if (idxDMG >= 0 && idxDMG < ARRAY_SIZE(s_dmg_dmg)) {
		pSettings->setValue(QLatin1String("DMG"), QLatin1String(s_dmg_dmg[idxDMG]));
	}

	const char s_dmg_other[][4] = {"DMG", "SGB", "CGB"};
	const int idxSGB = d->ui.cboSGB->currentIndex();
	const int idxCGB = d->ui.cboCGB->currentIndex();

	assert(idxSGB >= 0);
	assert(idxSGB < ARRAY_SIZE(s_dmg_other));
	if (idxSGB >= 0 && idxSGB < ARRAY_SIZE(s_dmg_other)) {
		pSettings->setValue(QLatin1String("SGB"), QLatin1String(s_dmg_other[idxSGB]));
	}
	assert(idxCGB >= 0);
	assert(idxCGB < ARRAY_SIZE(s_dmg_other));
	if (idxCGB >= 0 && idxCGB < ARRAY_SIZE(s_dmg_other)) {
		pSettings->setValue(QLatin1String("CGB"), QLatin1String(s_dmg_other[idxCGB]));
	}

	pSettings->endGroup();
}

/**
 * A combobox was changed.
 */
void DMGTab::comboBox_changed(void)
{
	// Configuration has been changed.
	printf("QUACK\n");
	Q_D(DMGTab);
	d->changed = true;
	emit modified();
}
