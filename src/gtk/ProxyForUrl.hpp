/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * ProxyForUrl.hpp: proxyForUrl() function for the GTK UI frontend.        *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C++ includes
#include <string>

/**
 * Get the proxy for the specified URL.
 * @param url URL
 * @return Proxy, or empty string if no proxy is needed.
 */
std::string proxyForUrl(const char *url);
