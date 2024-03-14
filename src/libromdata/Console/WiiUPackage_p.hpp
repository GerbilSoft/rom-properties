/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUPackage_p.hpp: Wii U NUS Package reader. (PRIVATE CLASS)            *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.librpbase.h"

// RomData subclasses
#include "WiiTicket.hpp"
#include "WiiTMD.hpp"

// Wii U FST
#include "disc/WiiUFst.hpp"

// librpbase
#ifdef ENABLE_DECRYPTION
#  include "librpbase/disc/CBCReader.hpp"
#  include "disc/WiiUH3Reader.hpp"
#endif /* ENABLE_DECRYPTION */

// librpfile
#include "librpfile/IRpFile.hpp"

// Uninitialized vector class
#include "uvector.h"

// C++ STL includes
#include <vector>

namespace LibRomData {

class WiiUPackagePrivate final : public LibRpBase::RomDataPrivate
{
public:
	WiiUPackagePrivate(const char *path);
	~WiiUPackagePrivate();

private:
	typedef LibRpBase::RomDataPrivate super;
	RP_DISABLE_COPY(WiiUPackagePrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const LibRpBase::RomDataInfo romDataInfo;

public:
	// Directory path (strdup()'d)
	char *path;

	// Ticket, TMD, and FST
	WiiTicket *ticket;
	WiiTMD *tmd;
	WiiUFst *fst;

	// Icon (loaded from "/meta/iconTex.tga")
	LibRpTexture::rp_image_const_ptr img_icon;

#ifdef ENABLE_DECRYPTION
	// Title key
	uint8_t title_key[16];
#endif /* ENABLE_DECRYPTION */

	// Contents table
	rp::uvector<WUP_Content_Entry> contentsTable;

	// Contents readers (index is the TMD index)
	std::vector<LibRpBase::IDiscReaderPtr> contentsReaders;

public:
	/**
	 * Clear everything.
	 */
	void reset(void);

	/**
	 * Open a content file.
	 * @param idx Content index (TMD index)
	 * @return Content file, or nullptr on error.
	 */
	LibRpBase::IDiscReaderPtr openContentFile(unsigned int idx);

	/**
	 * Open a file from the contents using the FST.
	 * @param filename Filename
	 * @return IRpFile, or nullptr on error.
	 */
	LibRpFile::IRpFilePtr open(const char *filename);

	/**
	 * Load the icon.
	 * @return Icon, or nullptr on error.
	 */
	LibRpTexture::rp_image_const_ptr loadIcon(void);
};

}
