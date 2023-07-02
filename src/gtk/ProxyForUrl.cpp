/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * ProxyForUrl.cpp: proxyForUrl() function for the GTK UI frontend.        *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ProxyForUrl.hpp"

// C++ STL classes
using std::string;

/**
 * Get the proxy for the specified URL.
 * @param url URL
 * @return Proxy, or empty string if no proxy is needed.
 */
string proxyForUrl(const char *url)
{
	// TODO: Support multiple proxies?
	string ret;

	GProxyResolver *const proxy_resolver = g_proxy_resolver_get_default();
	assert(proxy_resolver != nullptr);
	if (!proxy_resolver) {
		// Default proxy resolver is not available...
		return ret;
	}

	gchar **proxies = g_proxy_resolver_lookup(proxy_resolver, url, nullptr, nullptr);
	gchar *proxy = nullptr;
	if (proxies) {
		// Check if the first proxy is "direct://".
		if (strcmp(proxies[0], "direct://") != 0) {
			// Not direct access. Use this proxy.
			proxy = proxies[0];
		}
	}

	if (proxy) {
		ret.assign(proxy);
	}
	g_strfreev(proxies);
	return ret;
}
