/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * CacheTab.cpp: Cache tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "CacheTab.hpp"

// NOTE: librpbase doesn't cache the main cache directory;
// it only caches the rom-properties cache directory.
// We'll get the main cache directory from libunixcommon.
#include "librpbase/file/FileSystem.hpp"
#include "libunixcommon/userdirs.hpp"

#include "RpQt.hpp"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
using std::string;

#include "ui_CacheTab.h"
class CacheTabPrivate
{
	public:
		CacheTabPrivate() { }

	private:
		Q_DISABLE_COPY(CacheTabPrivate)

	public:
		Ui::CacheTab ui;

		/**
		 * Clear a directory using a worker thread.
		 * @param dir Directory to clear.
		 * TODO: Directory title?
		 */
		void clearDirectory(const QString &dir);
};

/** CacheTabPrivate **/

/**
 * Clear a directory using a worker thread.
 * @param dir Directory to clear.
 * TODO: Directory title?
 */
void CacheTabPrivate::clearDirectory(const QString &dir)
{
	// TODO: Implement this and add Q pointer!
	Q_UNUSED(dir);
}

/** CacheTab **/

CacheTab::CacheTab(QWidget *parent)
	: super(parent)
	, d_ptr(new CacheTabPrivate())
{
	Q_D(CacheTab);
	d->ui.setupUi(this);

	// Hide the status label and progress bar initially.
	d->ui.lblStatus->hide();
	d->ui.pbStatus->hide();
}

CacheTab::~CacheTab()
{
	delete d_ptr;
}

/**
 * Widget state has changed.
 * @param event State change event.
 */
void CacheTab::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(CacheTab);
		d->ui.retranslateUi(this);
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/**
 * Reset the configuration.
 */
void CacheTab::reset(void)
{
	// Nothing to do here...
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void CacheTab::loadDefaults(void)
{
	// Nothing to do here...
}

/**
 * Save the configuration.
 * @param pSettings QSettings object.
 */
void CacheTab::save(QSettings *pSettings)
{
	// Nothing to do here...
	Q_UNUSED(pSettings);
}

/**
 * "Clear System Thumbnail Cache" was clicked.
 */
void CacheTab::on_btnClearSysThumbnailCache_clicked(void)
{
	Q_D(CacheTab);
	const string cacheDir = LibUnixCommon::getCacheDirectory();
	if (cacheDir.empty()) {
		// TODO: Error!
		return;
	}
	QString qCacheDir = U82Q(cacheDir) + QLatin1String("/thumbnails");
	d->clearDirectory(qCacheDir);
}

/**
 * "Clear ROM Properties Cache" was clicked.
 */
void CacheTab::on_btnClearRomPropertiesCache_clicked(void)
{
	Q_D(CacheTab);
	d->clearDirectory(U82Q(LibRpBase::FileSystem::getCacheDirectory()));
}
