/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ProxyForUrl.cpp: proxyForUrl() function for the KDE UI frontend.        *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ProxyForUrl.hpp"
#include "RpQt.hpp"

// C++ STL classes
using std::string;

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
// KF6 removed KProtocolManager::proxyForUrl() in favor of QNetworkProxyFactory.
#  include <QtNetwork/QNetworkProxyFactory>
#else /* QT_VERSION < QT_VERSION_CHECK(6,0,0) */
// KDE protocol manager
// Used to find the KDE proxy settings.
#  include <kprotocolmanager.h>
#endif /* QT_VERSION >= QT_VERSION_CHECK(6,0,0) */

/**
 * Get the proxy for the specified URL.
 * @param url URL
 * @return Proxy, or empty string if no proxy is needed.
 */
string proxyForUrl(const char *url)
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	QList<QNetworkProxy> proxies = QNetworkProxyFactory::proxyForQuery(
		QNetworkProxyQuery(QUrl(U82Q(url)), QNetworkProxyQuery::UrlRequest));

	if (proxies.isEmpty()) {
		// No proxy.
		return {};
	}

	// FIXME: Not sure if this is correct, especially if the proxy uses https.
	// TODO: Verify this!
	QString proxy;
	const QNetworkProxy &np = proxies[0];
	const QString hostName = np.hostName();
	const qint16 port = np.port();
	if (hostName.isEmpty() || port == 0) {
		// No hostname or invalid port number.
		return {};
	}

	switch (np.type()) {
		case QNetworkProxy::NoProxy:
		case QNetworkProxy::DefaultProxy:
		default:
			// No proxy.
			return {};
		case QNetworkProxy::Socks5Proxy:
			proxy = QLatin1String("socks5://");
			break;
		case QNetworkProxy::HttpProxy:
		case QNetworkProxy::HttpCachingProxy:
		case QNetworkProxy::FtpCachingProxy:	// TODO: Verify
			proxy = QLatin1String("http://");
			break;
	}

	const QString user = np.user();
	const QString password = np.password();
	if (!user.isEmpty() || !password.isEmpty()) {
		// Username/password specified.
		proxy += QString::fromLatin1("%1:%2@").arg(user, password);
	}

	proxy += hostName;
	proxy += QChar(L':');
	proxy += QString::number(port);
	return Q2U8(proxy);
#else /* QT_VERSION < QT_VERSION_CHECK(6,0,0) */
	const QString proxy = KProtocolManager::proxyForUrl(QUrl(U82Q(url)));
	if (proxy.isEmpty() || proxy == QLatin1String("DIRECT")) {
		// No proxy.
		return {};
	}

	// Proxy is required.
	return Q2U8(proxy);
#endif /* QT_VERSION >= QT_VERSION_CHECK(6,0,0) */
}
