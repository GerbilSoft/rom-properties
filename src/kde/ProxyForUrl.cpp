/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ProxyForUrl.cpp: proxyForUrl() function for the KDE UI frontend.        *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include <stdafx.h>
#include "ProxyForUrl.hpp"
#include "RpQt.hpp"

// C++ STL classes
using std::string;

// KDE protocol manager
// Used to find the KDE proxy settings.
#include <kprotocolmanager.h>

/**
 * Get the proxy for the specified URL.
 * @param url URL
 * @return Proxy, or empty string if no proxy is needed.
 */
string proxyForUrl(const char *url)
{
	const QString proxy = KProtocolManager::proxyForUrl(QUrl(U82Q(url)));
	if (proxy.isEmpty() || proxy == QLatin1String("DIRECT")) {
		// No proxy.
		return string();
	}

	// Proxy is required.
	return Q2U8(proxy);
}
