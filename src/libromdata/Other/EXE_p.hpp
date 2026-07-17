/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE_p.hpp: DOS/Windows executable reader. (Private class)               *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/config.librpbase.h"
#include "librpbase/RomData_p.hpp"

#include "libromdata/config.libromdata.h"

#include "exe_mz_structs.h"
#include "exe_pe_structs.h"
#include "exe_ne_structs.h"
#include "exe_le_structs.h"

#include "disc/PEResourceReader.hpp"
using LibRpBase::IResourceReader;
using LibRpBase::IResourceReaderPtr;

// Uninitialized vector class
#include "uvector.h"

#include "span.hh"

#ifdef HAVE_STD_VARIANT
#  include <variant>
#else /* !HAVE_STD_VARIANT */
// std::variant<> is not available on this system.
// Use mpark variant instead.
#  include "mpark/variant.hpp"
namespace std {
	using mpark::variant;
	using mpark::holds_alternative;
	using mpark::get;
	using mpark::monostate;
}
#endif /* HAVE_STD_VARIANT */

// PugiXML
// NOTE: Cannot forward-declare the PugiXML classes...
#ifdef ENABLE_XML
#  include <pugixml.hpp>
#endif /* ENABLE_XML */

// C includes (C++ namespace)
#include <cstdint>

namespace LibRomData {

class EXE;
class EXEPrivate final : public LibRpBase::RomDataPrivate
{
public:
	EXEPrivate(const LibRpFile::IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(EXEPrivate)

public:
	/** RomDataInfo **/
	static const std::array<const char*, (9*2)+1> exts;
	static const std::array<const char*, 6+1> mimeTypes;
	static const LibRpBase::RomDataInfo romDataInfo;

public:
	// Executable type
	enum class ExeType {
		Unknown = -1,	// Unknown EXE type.

		MZ = 0,		// DOS MZ
		NE,		// 16-bit New Executable
		COM_NE,		// 16-bit COM/NE hybrid
				// (IBMDOS.COM from European DOS 4.0)
		LE,		// Mixed 16/32-bit Linear Executable
		W3,		// Collection of LE executables (WIN386.EXE)
		LX,		// 32-bit Linear Executable
		PE,		// 32-bit Portable Executable
		PE32PLUS,	// 64-bit Portable Executable

		Max
	};
	ExeType exeType;

public:
	// DOS MZ header
	IMAGE_DOS_HEADER mz;

	// Secondary header
	#pragma pack(1)
	union {
		uint32_t sig32;
		uint16_t sig16;
		struct {
			uint32_t Signature;
			IMAGE_FILE_HEADER FileHeader;
			union {
				uint16_t Magic;
				IMAGE_OPTIONAL_HEADER32 opt32;
				IMAGE_OPTIONAL_HEADER64 opt64;
			} OptionalHeader;
		} pe;
		NE_Header ne;
		LE_Header le;
	} hdr;
	#pragma pack()

	// PE-specific data
	struct PE_data_t {
		PE_data_t(uint16_t pe_subsystem) {
			this->pe_subsystem = pe_subsystem;
			ilcd.Size = 0;
		}

		// PE subsystem
		uint16_t pe_subsystem;

		// IMAGE_LOAD_CONFIG_DIRECTORY
		union ImageLoadConfigDirectory_t {
			uint32_t Size;
			IMAGE_LOAD_CONFIG_DIRECTORY32 ilcd32;
			IMAGE_LOAD_CONFIG_DIRECTORY64 ilcd64;
		};
		ImageLoadConfigDirectory_t ilcd;

		// PE section headers
		rp::uvector<IMAGE_SECTION_HEADER> sections;

		// PE Import Directory
		std::vector<IMAGE_IMPORT_DIRECTORY> peImportDir;
		// PE Import DLL Names (same order as the directory)
		std::vector<std::string> peImportNames;
	};

	// NE-specific data
	struct NE_data_t {
		rp::uvector<uint8_t> resident;
		vhvc::span<const NE_Segment> segment_table;
		vhvc::span<const uint8_t> resource_table;
		vhvc::span<const char> resident_name_table;
		vhvc::span<const uint16_t> modref_table;
		vhvc::span<const char> imported_name_table;
		vhvc::span<const uint8_t> entry_table;

		// Contents of the non-resident name table
		rp::uvector<char> nonresident_name_table;
	};

	// Data for specific executable types.
	std::variant<std::monostate, PE_data_t, NE_data_t> EXE_data;

	// Resource reader
	IResourceReaderPtr rsrcReader;

	/**
	 * Make sure the resource reader is loaded.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadResourceReader(void);

	/**
	 * Add VS_VERSION_INFO fields.
	 *
	 * NOTE: A subtab is NOT created here; if one is desired,
	 * create and set it before calling this function.
	 *
	 * @param pVsFfi	[in] VS_FIXEDFILEINFO
	 * @param pVsSfi	[in,opt] IResourceReader::StringFileInfo
	 */
	void addFields_VS_VERSION_INFO(const VS_FIXEDFILEINFO *pVsFfi, const IResourceReader::StringFileInfo *pVsSfi);

	// Icon
	LibRpTexture::rp_image_const_ptr img_icon;

	/**
	 * Load a specific icon by index.
	 * @param iconindex Icon index (positive for zero-based index; negative for resource ID)
	 * @return Icon, or nullptr if not found.
	 */
	LibRpTexture::rp_image_const_ptr loadSpecificIcon(int iconindex);

	/**
	 * Load the icon.
	 * @return Icon, or nullptr on error.
	 */
	LibRpTexture::rp_image_const_ptr loadIcon(void);

	/**
	 * Get the raw resource data for a specific icon.
	 * The highest color-depth icon is selected.
	 * @param iconindex	[in] Icon index (positive for zero-based index; negative for resource ID)
	 * @param width		[in,opt] Icon width (if 0, gets the largest icon)
	 * @param height	[in,opt] Icon height (if 0, gets the largest icon)
	 * @param pIconResID	[out,opt] Resource ID of the RT_ICON that was loaded
	 * @return Raw resource data, or nullptr if not found.
	 */
	rp::uvector<uint8_t> loadIconResourceData(int iconindex, int width = 0, int height = 0, uint32_t *pIconResID = nullptr);

	/** MZ-specific **/

	/**
	 * Add fields for MZ executables.
	 */
	void addFields_MZ(void);

	/** NE-specific **/

	// NE target OSes
	// Also used for LE.
	static const std::array<const char*, 6> NE_TargetOSes;

	/**
	 * Load the resident portion of NE header.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadNEResident(void);

	/**
	 * Load the non-resident name table. (NE)
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadNENonResidentNames(void);

	/**
	 * Load the NE resource table.
	 * @return 0 on success; negative POSIX error code on error. (-ENOENT if not found)
	 */
	int loadNEResourceTable(void);

	/**
	 * Find the runtime DLL. (NE version)
	 * @param refDesc String to store the description.
	 * @param refLink String to store the download link.
	 * @param refHasKernel Set to true if KERNEL is present in the import table.
	 *                     Used to distinguish between old Windows and OS/2 executables.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int findNERuntimeDLL(std::string &refDesc, std::string &refLink, bool &refHasKernel);

	/**
	 * Add fields for NE executables.
	 */
	void addFields_NE(void);

	/**
	 * Add fields for NE entry table.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int addFields_NE_Entry(void);

	/**
	 * Add fields for NE import table.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int addFields_NE_Import(void);

	/** LE/LX-specific **/

	/**
	 * Add fields for LE/LX executables.
	 */
	void addFields_LE(void);

	/** PE-specific **/

	/**
	 * Format the PE subsystem name.
	 * @param pe_subsystem PE subsystem
	 * @param subsystem_ver_major Subsystem major version
	 * @param subsystem_ver_minor Subsystem minor version
	 * @return Formatted PE subsystem name
	 */
	static std::string formatPESubsystemName(uint16_t pe_subsystem, uint32_t subsystem_ver_major, uint32_t subsystem_ver_minor);

	/**
	 * Load the PE section table.
	 *
	 * NOTE: If the table was read successfully, but no section
	 * headers were found, -ENOENT is returned.
	 *
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadPESectionTable(void);

	/**
	 * Convert a PE virtual address to a physical address.
	 * @param vaddr Virtual address.
	 * @param size Size of the virtual section.
	 * @return Physical address, or 0 if not mappable.
	 */
	uint32_t pe_vaddr_to_paddr(uint32_t vaddr, uint32_t size);

	/**
	 * Load the top-level PE resource directory.
	 * @return 0 on success; negative POSIX error code on error. (-ENOENT if not found)
	 */
	int loadPEResourceTypes(void);

private:
	/**
	 * Read PE import/export directory
	 * @param dataDir RVA/Size of directory will be stored here.
	 * @param type One of IMAGE_DATA_DIRECTORY_{EXPORT,IMPORT}_TABLE
	 * @param minSize Minimum direcrory size
	 * @param maxSize Maximum directory size
	 * @param dirTbl Vector into which the directory will be read.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int readPEImpExpDir(IMAGE_DATA_DIRECTORY &dataDir, int type,
		size_t minSize, size_t maxSize, rp::uvector<uint8_t> &dirTbl);

	/**
	 * Read a block of null-terminated strings, where the length of the
	 * last one isn't known in advance.
	 *
	 * The amount that will be read is:
	 * (high - low) + minExtra + maxExtra
	 * Which must be in the range [minMax; minMax + maxExtra]
	 *
	 * @param low RVA of first string.
	 * @param high RVA of last string.
	 * @param minExtra Last string must be at least this long.
	 * @param minMax The minimum size of the block must be smaller than this.
	 * @param maxExtra How many extra bytes can be read.
	 * @param outPtr Resulting array.
	 * @param outSize How much data was read.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int readPENullBlock(uint32_t low, uint32_t high, uint32_t minExtra,
		uint32_t minMax, uint32_t maxExtra, std::unique_ptr<char[]> &outPtr,
		size_t &outSize);

	/**
	 * Read PE Import Directory (peImportDir) and DLL names (peImportNames).
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int readPEImportDir(void);

public:
	/**
	 * Safely read data from a PE virtual address.
	 * @param vaddr	[in] Virtual address
	 * @param ptr	[out] Output data buffer
	 * @param size	[in] Amount of data to read, in bytes
	 * @return Number of bytes read on success; 0 on seek or read error.
	 */
	size_t readFromPEVAddr(uint32_t vaddr, void *ptr, size_t size);

	/**
	 * Find the runtime DLL. (PE version)
	 * @param refDesc String to store the description.
	 * @param refLink String to store the download link.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int findPERuntimeDLL(std::string &refDesc, std::string &refLink);

	/**
	 * Add fields for PE and PE32+ executables.
	 */
	void addFields_PE(void);

#ifdef ENABLE_XML
private:
	/**
	 * Load the Win32 manifest resource.
	 *
	 * The XML is loaded and parsed using the specified
	 * PugiXML document.
	 *
	 * NOTE: DelayLoad must be checked by the caller, since it's
	 * passing an xml_document reference to this function.
	 *
	 * @param doc		[in/out] XML document
	 * @param ppResName	[out,opt] Pointer to receive the loaded resource name. (statically-allocated string)
	 * @return 0 on success; negative POSIX error code on error.
	 */
	ATTR_ACCESS(read_write, 2)
	ATTR_ACCESS(write_only, 3)
	int loadWin32ManifestResource(pugi::xml_document &doc, const char **ppResName = nullptr) const;

public:
	/**
	 * Add fields from the Win32 manifest resource.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int addFields_PE_Manifest(void);

	/**
	 * Is the requestedExecutionLevel set to requireAdministrator?
	 * @return True if set; false if not or unable to determine.
	 */
	bool doesExeRequireAdministrator(void) const;
#endif /* ENABLE_XML */

public:
	/**
	 * Add fields for PE export table.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int addFields_PE_Export(void);

	/**
	 * Add fields for PE import table.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int addFields_PE_Import(void);

	/**
	 * Add fields for PE Codeview PDB info data.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int addFields_PE_PDB(void);


private:
	/**
	 * Load the IMAGE_LOAD_CONFIG_DIRECTORY.
	 * @return 0 on success; negative POSIX error code on error. (-ENOENT if not found)
	 */
	int loadPEImageLoadConfigDirectory(void);

public:
	/**
	 * Get the hybrid metadata pointer, if present.
	 * @return Hybrid metadata pointer, or 0 if not present.
	 */
	uint64_t getHybridMetadataPointer(void);

	/**
	 * Get the Dependent Load Flags, if present.
	 * @return Dependent Load Flags, or 0 if not present.
	 */
	uint16_t getDependentLoadFlags(void);
};

} // namespace LibRomData
