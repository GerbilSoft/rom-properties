/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * CacheTab.hpp: Thumbnail Cache tab for rp-config.                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ITab.hpp"
#include "CacheCleaner.hpp"

class CacheTabPrivate;
class CacheTab : public ITab
{
Q_OBJECT

public:
	explicit CacheTab(QWidget *parent = nullptr);
	~CacheTab() override;

private:
	typedef ITab super;
	CacheTabPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(CacheTab);
	Q_DISABLE_COPY(CacheTab)

public:
	/**
	 * Does this tab have defaults available?
	 * If so, the "Defaults" button will be enabled.
	 * Otherwise, it will be disabled.
	 *
	 * CacheTab sets this to false.
	 *
	 * @return True to enable; false to disable.
	 */
	bool hasDefaults(void) const final
	{
		return false;
	}

protected:
	// State change event. (Used for switching the UI language at runtime.)
	void changeEvent(QEvent *event) final;

public slots:
	/**
	 * Reset the configuration.
	 */
	void reset(void) final {}		// Nothing to do here.

	/**
	 * Save the configuration.
	 * @param pSettings QSettings object.
	 */
	void save(QSettings *pSettings) final
	{
		// Nothing to do here.
		Q_UNUSED(pSettings)
	}

private slots:
	/**
	 * "Clear the System Thumbnail Cache" button was clicked.
	 */
	void on_btnSysCache_clicked(void);

	/**
	 * "Clear the ROM Properties Page Download Cache" button was clicked.
	 */
	void on_btnRpCache_clicked(void);

private slots:
	/** CacheCleaner slots **/

	/**
	 * Cache cleaning task progress update.
	 * @param pg_cur Current progress.
	 * @param pg_max Maximum progress.
	 * @param hasError If true, errors have occurred.
	 */
	void ccCleaner_progress(int pg_cur, int pg_max, bool hasError);

	/**
	 * An error occurred while clearing the cache.
	 * @param error Error description.
	 */
	void ccCleaner_error(const QString &error);

	/**
	 * Cache directory is empty.
	 * @param cacheDir Which cache directory was checked.
	 */
	void ccCleaner_cacheIsEmpty(CacheCleaner::CacheDir cacheDir);

	/**
	 * Cache was cleared.
	 * @param cacheDir Which cache dir was cleared.
	 * @param dirErrs Number of directories that could not be deleted.
	 * @param fileErrs Number of files that could not be deleted.
	 */
	void ccCleaner_cacheCleared(CacheCleaner::CacheDir cacheDir, unsigned int dirErrs, unsigned int fileErrs);

	/**
	 * Cache cleaning task has completed.
	 * This is called when run() exits, regardless of status.
	 */
	void ccCleaner_finished(void);
};
