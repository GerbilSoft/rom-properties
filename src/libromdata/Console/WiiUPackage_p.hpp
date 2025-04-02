/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUPackage_p.hpp: Wii U NUS Package reader. (PRIVATE CLASS)            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
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

// TCHAR
#include "tcharx.h"

// C++ STL includes
#include <memory>
#include <vector>

// PugiXML
// NOTE: Cannot forward-declare the PugiXML classes...
#ifdef ENABLE_XML
#  include <pugixml.hpp>
#endif /* ENABLE_XML */

namespace LibRomData {

class WiiUPackagePrivate final : public LibRpBase::RomDataPrivate
{
public:
	WiiUPackagePrivate(const char *path);
#if defined(_WIN32) && defined(_UNICODE)
	WiiUPackagePrivate(const wchar_t *path);
#endif /* _WIN32 && _UNICODE */

private:
	typedef LibRpBase::RomDataPrivate super;
	RP_DISABLE_COPY(WiiUPackagePrivate)

public:
	/** RomDataInfo **/
	static const std::array<const char*, 0+1> exts;
	static const std::array<const char*, 1+1> mimeTypes;
	static const LibRpBase::RomDataInfo romDataInfo;

public:
	// Package type
	enum class PackageType {
		Unknown	= -1,

		NUS		= 0,	// NUS format
		Extracted	= 1,	// Extracted

		Max
	};
	PackageType packageType;

	// Directory path
	std::tstring path;

	// Ticket, TMD, and FST
	std::unique_ptr<WiiTicket> ticket;
	std::unique_ptr<WiiTMD> tmd;
	std::unique_ptr<WiiUFst> fst;

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

#ifdef ENABLE_XML
private:
	/**
	 * Load a Wii U system XML file.
	 *
	 * The XML is loaded and parsed using the specified
	 * PugiXML document.
	 *
	 * NOTE: DelayLoad must be checked by the caller, since it's
	 * passing an xml_document reference to this function.
	 *
	 * @param doc		[in/out] XML document
	 * @param filename	[in] XML filename
	 * @param rootNode	[in] Root node for verification
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadSystemXml(pugi::xml_document &doc, const char *filename, const char *rootNode);

	/**
	 * Parse an "unsignedInt" element.
	 * @param rootNode	[in] Root node
	 * @param name		[in] Node name
	 * @param defval	[in] Default value to return if the node isn't found
	 * @return unsignedInt data (returns 0 on error)
	 */
	unsigned int parseUnsignedInt(pugi::xml_node rootNode, const char *name, unsigned int defval = 0);

	/**
	 * Parse a "hexBinary" element.
	 * NOTE: Some fields are 64-bit hexBinary, so we'll return a 64-bit value.
	 * @param rootNode	[in] Root node
	 * @param name		[in] Node name
	 * @return hexBinary data
	 */
	static uint64_t parseHexBinary(pugi::xml_node rootNode, const char *name);

	/**
	 * Parse a "hexBinary" element.
	 * uint32_t wrapper function; truncates the uint64_t to uint32_t.
	 * @param rootNode	[in] Root node
	 * @param name		[in] Node name
	 * @return hexBinary data
	 */
	static inline uint32_t parseHexBinary32(pugi::xml_node rootNode, const char *name)
	{
		return static_cast<uint32_t>(parseHexBinary(rootNode, name));
	}

	/**
	 * Get the default language code for the multi-string fields.
	 * @return Language code, e.g. 'en' or 'es'.
	 */
	uint32_t getDefaultLC(void) const;

public:
	/**
	 * Add fields from the Wii U System XML files.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int addFields_System_XMLs(void);

	/**
	 * Add metadata from the Wii U System XML files.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int addMetaData_System_XMLs(void);

	/**
	 * Get the product code from meta.xml, and application type from app.xml.
	 * @param pApplType	[out] Pointer to uint32_t for application type
	 * @return Product code, or empty string on error.
	 */
	std::string getProductCodeAndApplType_xml(uint32_t *pApplType);
#endif /* ENABLE_XML */
};

} // namespace LibRomData
