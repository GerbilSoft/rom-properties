/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE_p.hpp: DOS/Windows executable reader. (Private class)               *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_EXE_P_HPP__
#define __ROMPROPERTIES_LIBROMDATA_EXE_P_HPP__

#include "librpbase/config.librpbase.h"
#include "librpbase/RomData_p.hpp"

#include "exe_structs.h"
#include "disc/PEResourceReader.hpp"

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

// TinyXML2
namespace tinyxml2 {
	class XMLDocument;
}

namespace LibRomData {

class EXE;
class EXEPrivate final : public LibRpBase::RomDataPrivate
{
	public:
		EXEPrivate(EXE *q, LibRpFile::IRpFile *file);
		~EXEPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(EXEPrivate)

	public:
		/** RomDataInfo **/
		static const char8_t *const exts[];
		static const char *const mimeTypes[];
		static const LibRpBase::RomDataInfo romDataInfo;

	public:
		// Executable type.
		enum class ExeType {
			Unknown = -1,	// Unknown EXE type.

			MZ = 0,		// DOS MZ
			NE,		// 16-bit New Executable
			LE,		// Mixed 16/32-bit Linear Executable
			W3,		// Collection of LE executables (WIN386.EXE)
			LX,		// 32-bit Linear Executable
			PE,		// 32-bit Portable Executable
			PE32PLUS,	// 64-bit Portable Executable

			Max
		};
		ExeType exeType;

	public:
		// DOS MZ header.
		IMAGE_DOS_HEADER mz;

		// Secondary header.
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

		// Resource reader.
		IResourceReader *rsrcReader;

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

		/** MZ-specific **/

		/**
		 * Add fields for MZ executables.
		 */
		void addFields_MZ(void);

		/** NE-specific **/

		// NE target OSes.
		// Also used for LE.
		static const char *const NE_TargetOSes[6];

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
		int findNERuntimeDLL(std::u8string &refDesc, std::u8string &refLink, bool &refHasKernel);

		/**
		 * Add fields for NE executables.
		 */
		void addFields_NE(void);

		/** LE/LX-specific **/

		/**
		 * Add fields for LE/LX executables.
		 */
		void addFields_LE(void);

		/** PE-specific **/

		// PE subsystem.
		uint16_t pe_subsystem;

		// PE section headers.
		ao::uvector<IMAGE_SECTION_HEADER> pe_sections;

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

		/**
		 * Find the runtime DLL. (PE version)
		 * @param refDesc String to store the description.
		 * @param refLink String to store the download link.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int findPERuntimeDLL(std::u8string &refDesc, std::u8string &refLink);

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
		 * TinyXML document.
		 *
		 * NOTE: DelayLoad must be checked by the caller, since it's
		 * passing an XMLDocument reference to this function.
		 *
		 * @param doc		[in/out] XML document
		 * @param ppResName	[out,opt] Pointer to receive the loaded resource name (statically-allocated string)
		 * @return 0 on success; negative POSIX error code on error.
		 */
		ATTR_ACCESS(read_write, 2)
		ATTR_ACCESS(write_only, 3)
		int loadWin32ManifestResource(tinyxml2::XMLDocument &doc, const char8_t **ppResName = nullptr) const;

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
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_EXE_P_HPP__ */
