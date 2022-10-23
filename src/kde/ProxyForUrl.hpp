/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ProxyForUrl.hpp: proxyForUrl() function for the KDE UI frontend.        *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_PROXYFORURL_HPP__
#define __ROMPROPERTIES_KDE_PROXYFORURL_HPP__

// C++ includes
#include <string>

/**
 * Get the proxy for the specified URL.
 * @param url URL
 * @return Proxy, or empty string if no proxy is needed.
 */
std::string proxyForUrl(const char *url);

#endif /* __ROMPROPERTIES_KDE_PROXYFORURL_HPP__ */
