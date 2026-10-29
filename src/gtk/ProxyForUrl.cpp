/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * ProxyForUrl.cpp: proxyForUrl() function for the GTK UI frontend.        *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "ProxyForUrl.hpp"

// C includes (C++ namespace)
#include <cassert>
#include <cstring>

// C++ STL classes
using std::string;
using std::string_view;

// glib includes
#include <glib.h>
#include <gio/gio.h>

/**
 * Get the proxy for the specified URL.
 * @param url URL
 * @return Proxy, or empty string if no proxy is needed.
 */
string proxyForUrl(string_view url)
{
	// TODO: Support multiple proxies?
	string ret;

	GProxyResolver *const proxy_resolver = g_proxy_resolver_get_default();
	assert(proxy_resolver != nullptr);
	if (!proxy_resolver) {
		// Default proxy resolver is not available...
		return ret;
	}

	// NOTE: Need to convert the URL to a NULL-terminated string.
	gchar *const sz_url = g_strndup(url.data(), url.size());
	gchar **proxies = g_proxy_resolver_lookup(proxy_resolver, sz_url, nullptr, nullptr);
	g_free(sz_url);
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
