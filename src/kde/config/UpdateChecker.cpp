/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * UpdateChecker.hpp: Update checker object for AboutTab.                  *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "UpdateChecker.hpp"

// librpbase
#include "librpbase/config/AboutTabText.hpp"
using namespace LibRpBase;

// libromdata
#include "libromdata/img/CacheManager.hpp"
using LibRomData::CacheManager;

// KDE protocol manager.
// Used to find the KDE proxy settings.
#include <kprotocolmanager.h>

// C++ STL classes
using std::string;

UpdateChecker::UpdateChecker(QObject *parent)
	: super(parent)
{ }

/**
 * Run the task.
 * This should be connected to QThread::started().
 */
void UpdateChecker::run(void)
{
	// Download sys/version.txt and compare it to our version.
	// NOTE: Ignoring the fourth decimal (development flag).
	const char *const updateVersionUrl =
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::UpdateVersionUrl);
	const char *const updateVersionCacheKey =
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::UpdateVersionCacheKey);

	assert(updateVersionUrl != nullptr);
	assert(updateVersionCacheKey != nullptr);
	if (!updateVersionUrl || !updateVersionCacheKey) {
		// TODO: Show an error message.
		emit finished();
		return;
	}

	CacheManager cache;
	QString proxy = KProtocolManager::proxyForUrl(QUrl(U82Q(updateVersionUrl)));
	if (!proxy.isEmpty() && proxy != QLatin1String("DIRECT")) {
		// Proxy is required.
		cache.setProxyUrl(proxy.toUtf8().constData());
	}

	// Download the version file.
	string cache_filename = cache.download(updateVersionCacheKey);
	if (cache_filename.empty()) {
		// Unable to download the version file.
		emit error(tr("Failed to download version file."));
		emit finished();
		return;
	}

	// Read the version file.
	QFile file(U82Q(cache_filename));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		// TODO: Error code?
		emit error(tr("Failed to open version file."));
		emit finished();
		return;
	}

	// Read the first line, which should contain a 4-decimal version number.
	QString sVersion = U82Q(file.readLine().constData()).trimmed();
	if (sVersion.isEmpty()) {
		emit error(tr("Version file is invalid."));
		emit finished();
		return;
	}

	QStringList sVersionArray = sVersion.split(QChar(L'.'));
	if (sVersionArray.size() != 4) {
		emit error(tr("Version file is invalid."));
		emit finished();
		return;
	}

	// Convert to a 64-bit version. (ignoring the development flag)
	bool ok = false;
	uint64_t updateVersion = 0;
	for (unsigned int i = 0; i < 3; i++, updateVersion <<= 16U) {
		int x = sVersionArray[i].toInt(&ok);
		if (!ok) {
			emit error(tr("Version file is invalid."));
			emit finished();
			return;
		}
		updateVersion |= ((uint64_t)x & 0xFFFFU);
	}

	// HACK: Always returning 3.1.5 for testing purposes.
	// Remove this when finished debugging!
	updateVersion = 0x0003000100050000;

	// Update version retrieved.
	emit retrieved(updateVersion);
	emit finished();
}
