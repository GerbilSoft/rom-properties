/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE.cpp: DOS/Windows executable reader. (Private class)                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_EXE_P_HPP__
#define __ROMPROPERTIES_LIBROMDATA_EXE_P_HPP__

#include "librpbase/config.librpbase.h"
#include "librpbase/RomData_p.hpp"

#include "exe_structs.h"

// IResourceReader
#include "disc/NEResourceReader.hpp"
#include "disc/PEResourceReader.hpp"

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData {

class EXE;
class EXEPrivate : public LibRpBase::RomDataPrivate
{
	public:
		EXEPrivate(EXE *q, LibRpBase::IRpFile *file);
		~EXEPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(EXEPrivate)

	public:
		// Executable type.
		enum ExeType {
			EXE_TYPE_UNKNOWN = -1,	// Unknown EXE type.

			EXE_TYPE_MZ = 0,	// DOS MZ
			EXE_TYPE_NE,		// 16-bit New Executable
			EXE_TYPE_LE,		// Mixed 16/32-bit Linear Executable
			EXE_TYPE_W3,		// Collection of LE executables (WIN386.EXE)
			EXE_TYPE_LX,		// 32-bit Linear Executable
			EXE_TYPE_PE,		// 32-bit Portable Executable
			EXE_TYPE_PE32PLUS,	// 64-bit Portable Executable

			EXE_TYPE_LAST
		};
		int exeType;

	public:
		// DOS MZ header.
		IMAGE_DOS_HEADER mz;

		// Secondary header.
		#pragma pack(1)
		union PACKED {
			uint32_t sig32;
			uint16_t sig16;
			struct PACKED {
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
		static const char *const NE_TargetOSes[];

		/**
		 * Load the NE resource table.
		 * @return 0 on success; negative POSIX error code on error. (-ENOENT if not found)
		 */
		int loadNEResourceTable(void);

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
		 * Load the top-level PE resource directory.
		 * @return 0 on success; negative POSIX error code on error. (-ENOENT if not found)
		 */
		int loadPEResourceTypes(void);

		/**
		 * Add fields for PE and PE32+ executables.
		 */
		void addFields_PE(void);

#ifdef ENABLE_XML
		/**
		 * Add fields from the Win32 manifest resource.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int addFields_PE_Manifest(void);
#endif /* ENABLE_XML */
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_EXE_P_HPP__ */
