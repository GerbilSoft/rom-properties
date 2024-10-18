/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * CacheKeyVerify.cpp: Cache key verifier.                                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CacheKeyVerify.hpp"

// libcachecommon
#include "libcachecommon/CacheKeys.hpp"

// C++ STL classes
using std::tstring;

namespace RpDownload {

/**
 * Verify a cache key and convert it to a source URL.
 * @param outURL	[out] tstring for the source URL
 * @param check_newer	[out] If true, always check, but only download if newer (for [sys])
 * @param cache_key	[in] Cache key
 * @return CacheKeyError
 */
CacheKeyError verifyCacheKey(tstring &outURL, bool &check_newer, const TCHAR *cache_key)
{
	// Check the cache key prefix. The prefix indicates the system
	// and identifies the online database used.
	// [key] indicates the cache key without the prefix.
	// - wii:    https://art.gametdb.com/wii/[key]
	// - wiiu:   https://art.gametdb.com/wiiu/[key]
	// - 3ds:    https://art.gametdb.com/3ds/[key]
	// - ds:     https://art.gametdb.com/3ds/[key]
	// - amiibo: https://amiibo.life/[key]/image
	// - gba:    https://rpdb.gerbilsoft.com/gba/[key]
	// - gb:     https://rpdb.gerbilsoft.com/gb/[key]
	// - snes:   https://rpdb.gerbilsoft.com/snes/[key]
	// - ngp:    https://rpdb.gerbilsoft.com/ngp/[key]
	// - ngpc:   https://rpdb.gerbilsoft.com/ngpc/[key]
	// - ws:     https://rpdb.gerbilsoft.com/ws/[key]
	// - c64:    https://rpdb.gerbilsoft.com/c64/[key]
	// - c128:   https://rpdb.gerbilsoft.com/c128/[key]
	// - cbmII:  https://rpdb.gerbilsoft.com/cbmII/[key]
	// - vic20:  https://rpdb.gerbilsoft.com/vic20/[key]
	// - plus4:  https://rpdb.gerbilsoft.com/plus4/[key]
	// - ps1:    https://rpdb.gerbilsoft.com/ps1/[key]
	// - ps2:    https://rpdb.gerbilsoft.com/ps2/[key]
	// - sys:    https://rpdb.gerbilsoft.com/sys/[key] [system info, e.g. update version]
	const TCHAR *slash_pos = _tcschr(cache_key, _T('/'));
	if (slash_pos == nullptr || slash_pos == cache_key ||
		slash_pos[1] == '\0')
	{
		// Invalid cache key:
		// - Does not contain any slashes.
		// - First slash is either the first or the last character.
		return CacheKeyError::Invalid;
	}

	const ptrdiff_t prefix_len = (slash_pos - cache_key);
	if (prefix_len <= 0) {
		// Empty prefix.
		return CacheKeyError::Invalid;
	}

	// Cache key must include a lowercase file extension.
	const TCHAR *const lastdot = _tcsrchr(cache_key, _T('.'));
	if (!lastdot) {
		// No dot...
		return CacheKeyError::Invalid;
	}
	if ((!_tcscmp(lastdot, _T(".png"))) != 0 ||
	    (!_tcscmp(lastdot, _T(".jpg"))) != 0)
	{
		// Image file extension is supported.
	}
	else if (!_tcscmp(lastdot, _T(".txt")))
	{
		// .txt is supported for sys/ only.
		if (_tcsncmp(cache_key, _T("sys/"), 4) != 0) {
			return CacheKeyError::Invalid;
		}
	} else {
		// Not a supported file extension.
		return CacheKeyError::Invalid;
	}

	// urlencode the cache key.
	const tstring cache_key_urlencode = LibCacheCommon::urlencode(cache_key);
	// Update the slash position based on the urlencoded string.
	slash_pos = _tcschr(cache_key_urlencode.data(), _T('/'));
	assert(slash_pos != nullptr);
	if (!slash_pos) {
		// Shouldn't happen, since a slash was found earlier...
		return CacheKeyError::Invalid;
	}

	// Determine the full URL based on the cache key.
	check_newer = false;	// for [sys]: always check, but only download if newer
	bool ok = false;
	outURL.reserve(64);
	if ((prefix_len == 3 && (!_tcsncmp(cache_key, _T("wii"), 3) || !_tcsncmp(cache_key, _T("3ds"), 3))) ||
	    (prefix_len == 4 && !_tcsncmp(cache_key, _T("wiiu"), 4)) ||
	    (prefix_len == 2 && !_tcsncmp(cache_key, _T("ds"), 2)))
	{
		// GameTDB: Wii, Wii U, Nintendo 3DS, Nintendo DS
		ok = true;
		outURL = _T("https://art.gametdb.com/");
		outURL += cache_key_urlencode;
	} else if (prefix_len == 6 && !_tcsncmp(cache_key, _T("amiibo"), 6)) {
		// amiibo.life: amiibo images
		// NOTE: We need to remove the file extension.
		size_t filename_len = _tcslen(slash_pos+1);
		if (filename_len <= 4) {
			// Can't remove the extension...
			return CacheKeyError::Invalid;
		}
		filename_len -= 4;

		ok = true;
		outURL = _T("https://amiibo.life/nfc/");
		outURL.append(slash_pos + 1, filename_len);
		outURL += "/image";
	} else {
		// RPDB: Title screen images for various systems.
		switch (prefix_len) {
			default:
				break;
			case 2:
				if (!_tcsncmp(cache_key, _T("gb"), 2) ||
				    !_tcsncmp(cache_key, _T("ws"), 2) ||
				    !_tcsncmp(cache_key, _T("md"), 2))
				{
					ok = true;
				}
				break;
			case 3:
				if (!_tcsncmp(cache_key, _T("gba"), 3) ||
				    !_tcsncmp(cache_key, _T("mcd"), 3) ||
				    !_tcsncmp(cache_key, _T("32x"), 3) ||
				    !_tcsncmp(cache_key, _T("c64"), 3) ||
				    !_tcsncmp(cache_key, _T("ps1"), 3) ||
				    !_tcsncmp(cache_key, _T("ps2"), 3))
				{
					ok = true;
				}
				else if (!_tcsncmp(cache_key, _T("sys"), 3))
				{
					ok = true;
					check_newer = true;
				}
				break;
			case 4:
				if (!_tcsncmp(cache_key, _T("snes"), 4) ||
				    !_tcsncmp(cache_key, _T("ngpc"), 4) ||
				    !_tcsncmp(cache_key, _T("pico"), 4) ||
				    !_tcsncmp(cache_key, _T("tera"), 4) ||
				    !_tcsncmp(cache_key, _T("c128"), 4))
				{
					ok = true;
				}
				break;
			case 5:
				if (!_tcsncmp(cache_key, _T("cbmII"), 5) ||
				    !_tcsncmp(cache_key, _T("vic20"), 5) ||
				    !_tcsncmp(cache_key, _T("plus4"), 5))
				{
					ok = true;
				}
				break;
			case 6:
				if (!_tcsncmp(cache_key, _T("mcd32x"), 6)) {
					ok = true;
				}
				break;
		}

		if (ok) {
			outURL = _T("https://rpdb.gerbilsoft.com/");
			outURL += cache_key_urlencode;
		}
	}

	return (ok) ? CacheKeyError::OK : CacheKeyError::PrefixNotSupported;
}

} // namespace RpDownload
