/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * CacheCleaner.hpp: Cache cleaner object for CacheTab.                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <QtCore/QObject>

class CacheCleaner : public QObject
{
Q_OBJECT

Q_ENUMS(CacheCleaner::CacheDir)
Q_PROPERTY(CacheCleaner::CacheDir cacheDir READ cacheDir WRITE setCacheDir)

public:
	enum CacheDir {
		CD_System,
		CD_RomProperties,
	};

	explicit CacheCleaner(QObject *parent, CacheCleaner::CacheDir cacheDir = CacheDir::CD_System);

private:
	typedef QObject super;
	Q_DISABLE_COPY(CacheCleaner)

public:
	/**
	 * Get the selected cache directory.
	 * @return Cache directory.
	 */
	inline CacheDir cacheDir(void) const
	{
		return m_cacheDir;
	}

	/**
	 * Set the cache directory.
	 * NOTE: Only do this when the object isn't running!
	 * @param cacheDir New cache directory.
	 */
	inline void setCacheDir(CacheDir cacheDir)
	{
		m_cacheDir = cacheDir;
	}

public slots:
	/**
	 * Run the task.
	 * This should be connected to QThread::started().
	 */
	void run(void);

signals:
	/**
	 * Cache cleaning task progress update.
	 * @param pg_cur Current progress.
	 * @param pg_max Maximum progress.
	 * @param hasError If true, errors have occurred.
	 */
	void progress(int pg_cur, int pg_max, bool hasError);

	/**
	 * An error occurred while clearing the cache.
	 * @param error Error description.
	 */
	void error(const QString &error);

	/**
	 * Cache directory is empty.
	 * @param cacheDir Which cache directory was checked.
	 */
	void cacheIsEmpty(CacheCleaner::CacheDir cacheDir);

	/**
	 * Cache has been cleared.
	 * @param cacheDir Which cache dir was cleared.
	 * @param dirErrs Number of directories that could not be deleted.
	 * @param fileErrs Number of files that could not be deleted.
	 */
	void cacheCleared(CacheCleaner::CacheDir cacheDir, unsigned int dirErrs, unsigned int fileErrs);

	/**
	 * Cache cleaning task has completed.
	 * This is called when run() exits, regardless of status.
	 */
	void finished(void);

protected:
	CacheDir m_cacheDir;
};

Q_DECLARE_METATYPE(CacheCleaner::CacheDir)
