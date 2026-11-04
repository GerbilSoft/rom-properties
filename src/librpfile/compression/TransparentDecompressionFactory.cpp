/*****************************************************************************
 * ROM Properties Page shell extension. (librpfile)                          *
 * TransparentDecompressionFactory.cpp: Check for transparent decompression. *
 *                                                                           *
 * Copyright (c) 2026 by David Korth.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 *****************************************************************************/

#include "TransparentDecompressionFactory.hpp"

// Transparent decompression handler IRpFile subclasses
#include "GzipFile.hpp"
#include "ZstdFile.hpp"

// for FileSystem::file_ext()
#include "../FileSystem.hpp"

// librpbyteswap
#include "librpbyteswap/byteswap_rp.h"

// C includes
#include "tcharx.h"	// for strcasecmp() or equivalent

// C includes (C++ namespace)
#include <cstdint>

namespace LibRpFile { namespace TransparentDecompressionFactory {

/**
 * Check if a file is compressed using a supported compression scheme.
 * If it is, open a transparent decompression handler IRpFile subclass
 * and return it.
 * @param file IRpFile to check
 * @return Transparent decompression handler IRpFile, or nullptr if not compressed.
 */
IRpFilePtr create(const IRpFilePtr &file)
{
	IRpFilePtr z_file;

	// Read the first 4 bytes in order to check magic numbers.
	union {
		uint8_t u8[4];
		uint16_t u16[2];
		uint32_t u32;
	} magic;
	file->rewind();
	size_t size = file->read(magic.u8, sizeof(magic.u8));
	if (size != sizeof(magic.u8)) {
		// Read error.
		return z_file;
	}

	if (magic.u16[0] == cpu_to_be16(0x1F8B)) {
		// gzip magic number
		z_file = std::make_shared<GzipFile>(file);
		if (static_cast<GzipFile*>(z_file.get())->isGzip()) {
			// It's Gzip!
			return z_file;
		}
	}

	if (magic.u32 == be32_to_cpu(0x28B52FFD)) {
		// zstd magic number
		// NOTE: Need to check that the file extension is *not* ".wua".
		const char *const filename = file->filename();
		const char *ext = nullptr;
		if (filename) {
			ext = FileSystem::file_ext(filename);
		}
		if (!ext || strcasecmp(ext, ".wua") != 0) {
			// Not a ".wua" extension. Try zstd.
			z_file = std::make_shared<ZstdFile>(file);
			if (static_cast<ZstdFile*>(z_file.get())->isZstd()) {
				// It's Zstd!
				return z_file;
			}
		}
	}

	// Not supported...
	z_file.reset();
	return z_file;
}

} } // namespace LibRpFile::TransparentDecompressionFactory
