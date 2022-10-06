/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ClearCache.hpp: Clear Cache object.                                     *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_CONFIG_CLEARCACHE_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_CLEARCACHE_HPP__

// Qt includes
#include <QtCore/QObject>

class ClearCache : public QObject
{
	Q_OBJECT

	Q_PROPERTY(CacheDir cacheDir READ cacheDir WRITE setCacheDir)

	public:
		explicit ClearCache(QObject *parent = 0);

	private:
		typedef QObject super;
		Q_DISABLE_COPY(ClearCache)

	public:
		enum class CacheDir {
			Unknown = 0,

			System,
			RomProperties,
		};
#if QT_VERSION >= QT_VERSION_CHECK(5,5,0)
		Q_ENUM(CacheDir);
#else /* QT_VERSION < QT_VERSION_CHECK(5,5,0) */
		Q_ENUMS(CacheDir);
#endif

		/**
		 * Set the cache directory.
		 * @return True if it was set; false if unable to set.
		 */
		bool setCacheDir(CacheDir cacheDir);

		/**
		 * Get the cache directory.
		 * @return Cache directory.
		 */
		CacheDir cacheDir(void) const
		{
			return m_cacheDir;
		}

	private:
		CacheDir m_cacheDir;
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_CLEARCACHE_HPP__ */
