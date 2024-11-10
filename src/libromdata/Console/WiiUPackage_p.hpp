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

// TCHAR
#include "tcharx.h"

// C++ STL includes
#include <memory>
#include <vector>

// TinyXML2
namespace tinyxml2 {
	class XMLDocument;
	class XMLElement;
}

namespace LibRomData {

class WiiUPackagePrivate final : public LibRpBase::RomDataPrivate
{
public:
	WiiUPackagePrivate(const char *path);
#if defined(_WIN32) && defined(_UNICODE)
	WiiUPackagePrivate(const wchar_t *path);
#endif /* _WIN32 && _UNICODE */
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
	TCHAR *path;

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

public:
	/**
	 * Is a directory supported by this class?
	 * @tparam T Character type (char for UTF-8; wchar_t for Windows UTF-16)
	 * @param path Directory to check
	 * @param filenames_to_check Array of filenames to check
	 * @return Class-specific system ID (>= 0) if supported; -1 if not.
	 */
	template<typename T>
	static int T_isDirSupported_static(const T *path, const std::array<const T*, 3> &filenames_to_check);

#ifdef ENABLE_XML
private:
	/**
	 * Load a Wii U system XML file.
	 *
	 * The XML is loaded and parsed using the specified
	 * TinyXML document.
	 *
	 * NOTE: DelayLoad must be checked by the caller, since it's
	 * passing an XMLDocument reference to this function.
	 *
	 * @param doc		[in/out] XML document
	 * @param filename	[in] XML filename
	 * @param rootNode	[in] Root node for verification
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadSystemXml(tinyxml2::XMLDocument &doc, const char *filename, const char *rootNode);

	/**
	 * Parse an "unsignedInt" element.
	 * @param rootNode	[in] Root node
	 * @param name		[in] Node name
	 * @param defval	[in] Default value to return if the node isn't found
	 * @return unsignedInt data (returns 0 on error)
	 */
	unsigned int parseUnsignedInt(const tinyxml2::XMLElement *rootNode, const char *name, unsigned int defval = 0);

	/**
	 * Parse a "hexBinary" element.
	 * NOTE: Some fields are 64-bit hexBinary, so we'll return a 64-bit value.
	 * @param rootNode	[in] Root node
	 * @param name		[in] Node name
	 * @return hexBinary data
	 */
	static uint64_t parseHexBinary(const tinyxml2::XMLElement *rootNode, const char *name);

	/**
	 * Get text from an XML element.
	 * @param rootNode	[in] Root node
	 * @param name		[in] Node name
	 * @return Node text, or nullptr if not found or empty.
	 */
	static inline const char *getText(const tinyxml2::XMLElement *rootNode, const char *name);

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

}
