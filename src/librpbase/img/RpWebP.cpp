/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpWebP.cpp: WebP image handler.                                         *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "RpWebP.hpp"
#include "librpfile/IRpFile.hpp"

// Other rom-properties libraries
using namespace LibRpFile;
using namespace LibRpTexture;

// Uninitialized vector class
#include "uvector.h"

// C includes
#include <dlfcn.h>

// C++ STL classes
using std::unique_ptr;

// libwebp declarations
// NOTE: Using dlopen().

int WebPGetInfo(
    const uint8_t* data, size_t data_size, int* width, int* height);

uint8_t* WebPDecodeBGRAInto(
    const uint8_t* data, size_t data_size,
    uint8_t* output_buffer, size_t output_buffer_size, int output_stride);

typedef __typeof__(WebPGetInfo) *pfn_WebPGetInfo_t;
typedef __typeof__(WebPDecodeBGRAInto) *pfn_WebPDecodeBGRAInto_t;

namespace LibRpBase { namespace RpWebP {

// DLL handle
#ifndef _WIN32
typedef void *HMODULE;
#endif

class HMODULE_deleter
{
public:
	typedef HMODULE pointer;

	void operator()(HMODULE hModule)
	{
		if (hModule) {
#ifdef _WIN32
			FreeLibrary(hModule);
#else /* !_WIN32 */
			dlclose(hModule);
#endif /* _WIN32 */
		}
	}
};

/** RpWebP **/

static constexpr off64_t WEBP_MAX_FILESIZE = 2*1024*1024;

/**
 * Load a WebP image from an IRpFile.
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
rp_image_ptr load(IRpFile *file)
{
	if (!file) {
		return {};
	}

	// Check the file size.
	const off64_t fileSize_o64 = file->size();
	if (fileSize_o64 <= 16 || fileSize_o64 > WEBP_MAX_FILESIZE) {
		// File is too big (or too small?).
		return {};
	}
	const size_t fileSize = static_cast<size_t>(fileSize_o64);

	// dlopen() libwebp.
	// TODO: Cache the dlopen()'d library?
	// NOTE: Ubuntu systems don't have an unversioned .so unless the -dev package is installed.
	static const char libwebp_so_filenames[3][16] = {
		"libwebp.so.7",
		"libwebp.so.6",
		"libwebp.so.5",
	};
	unique_ptr<HMODULE, HMODULE_deleter> libwebp_so;
	for (auto filename : libwebp_so_filenames) {
		libwebp_so.reset(dlopen(filename, RTLD_NOW | RTLD_LOCAL));
		if (libwebp_so.get() != nullptr) {
			break;
		}
	}
	if (!libwebp_so) {
		// Not found...
		return {};
	}

	// Attempt to dlsym() the required symbols.
	pfn_WebPGetInfo_t pfn_WebPGetInfo = reinterpret_cast<pfn_WebPGetInfo_t>(
		dlsym(libwebp_so.get(), "WebPGetInfo"));
	pfn_WebPDecodeBGRAInto_t pfn_WebPDecodeBGRAInto = reinterpret_cast<pfn_WebPDecodeBGRAInto_t>(
		dlsym(libwebp_so.get(), "WebPDecodeBGRAInto"));
	assert(pfn_WebPGetInfo != nullptr);
	assert(pfn_WebPDecodeBGRAInto != nullptr);
	if (!pfn_WebPGetInfo || !pfn_WebPDecodeBGRAInto) {
		// One or more symbols were not found...
		return {};
	}

	// Read the entire file into memory.
	// TODO: Does libwebp support streaming?
	rp::uvector<uint8_t> webp_buf;
	webp_buf.resize(fileSize);
	size_t size = file->seekAndRead(0, webp_buf.data(), webp_buf.size());
	if (size != webp_buf.size()) {
		// Seek and/or read error.
		return {};
	}

	// Get the WebP image dimensions.
	// NOTE: WebP returns 0 on *error*.
	int width = 0, height = 0;
	int ret = pfn_WebPGetInfo(webp_buf.data(), webp_buf.size(), &width, &height);
	if (!ret || width <= 0 || height <= 0) {
		// WebP didn't like this image.
		return {};
	}

	// Decode the WebP into an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::ARGB32);
	uint8_t *const pRet = pfn_WebPDecodeBGRAInto(webp_buf.data(), webp_buf.size(),
		static_cast<uint8_t*>(img->bits()), img->data_len(), img->stride());
	if (!pRet) {
		// Failed to decode the image...
		return {};
	}

	// Image decoded successfully.
	return img;
}

} }
