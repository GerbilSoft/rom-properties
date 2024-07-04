/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * UpdateChecker.hpp: Update checker object for AboutTab.                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "UpdateChecker.hpp"
#include "ProxyForUrl.hpp"

// librpbase
#include "librpbase/config/AboutTabText.hpp"
using namespace LibRpBase;

// libromdata
#include "libromdata/img/CacheManager.hpp"
using LibRomData::CacheManager;

// C++ STL classes
using std::string;

UpdateChecker::UpdateChecker(QObject *parent)
	: super(parent)
{}

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
	const string proxy = ::proxyForUrl(updateVersionUrl);
	if (!proxy.empty()) {
		// Proxy is required.
		cache.setProxyUrl(proxy);
	}

	// Download the version file.
	const string cache_filename = cache.download(updateVersionCacheKey);
	if (cache_filename.empty()) {
		// Unable to download the version file.
		emit error(U82Q(C_("UpdateChecker", "Failed to download version file.")));
		emit finished();
		return;
	}

	// Read the version file.
	QFile file(U82Q(cache_filename));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		// TODO: Error code?
		emit error(U82Q(C_("UpdateChecker", "Failed to open version file.")));
		emit finished();
		return;
	}

	// Read the first line, which should contain a 4-decimal version number.
	const QString sVersion = U82Q(file.readLine().constData()).trimmed();
	const QStringList sVersionArray = sVersion.split(QChar(L'.'));
	if (sVersion.isEmpty() || sVersionArray.size() != 4) {
		emit error(U82Q(C_("UpdateChecker", "Version file is invalid.")));
		emit finished();
		return;
	}

	// Convert to a 64-bit version. (ignoring the development flag)
	bool ok = false;
	uint64_t updateVersion = 0;
	for (unsigned int i = 0; i < 3; i++, updateVersion <<= 16U) {
		const int x = sVersionArray[i].toInt(&ok);
		if (!ok || x < 0) {
			emit error(U82Q(C_("UpdateChecker", "Version file is invalid.")));
			emit finished();
			return;
		}
		updateVersion |= ((uint64_t)x & 0xFFFFU);
	}

	// Update version retrieved.
	emit retrieved(updateVersion);
	emit finished();
}
