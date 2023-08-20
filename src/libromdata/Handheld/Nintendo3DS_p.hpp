/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DS.hpp: Nintendo 3DS ROM reader. (Private class)               *
 * Handles CCI/3DS, CIA, and SMDH files.                                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "n3ds_structs.h"

// librpbase
#include "librpbase/RomData_p.hpp"

// NCCHReader
#include "../disc/NCCHReader.hpp"

namespace LibRpBase {
	class IDiscReader;
	class PartitionFile;
	class RomData;
}
namespace LibRpFile {
	class IRpFile;
}

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData {

class Nintendo3DS_SMDH;
class NintendoDS;

class Nintendo3DSPrivate final : public LibRpBase::RomDataPrivate
{
	public:
		Nintendo3DSPrivate(const LibRpFile::IRpFilePtr &file);
		~Nintendo3DSPrivate() final = default;

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(Nintendo3DSPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const LibRpBase::RomDataInfo romDataInfo;

	public:
		// ROM type.
		enum class RomType {
			Unknown = -1,

			_3DSX	= 0,	// 3DSX (homebrew)
			CCI	= 1,	// CCI/3DS (cartridge dump)
			eMMC	= 2,	// eMMC dump
			CIA	= 3,	// CIA
			NCCH	= 4,	// NCCH

			Max
		};
		RomType romType;

	public:
		// What stuff do we have?
		enum HeadersPresent {
			HEADER_NONE	= 0,

			// The following headers are not exclusive,
			// so one or more can be present.
			HEADER_SMDH	= (1U << 0),	// Includes header and icon.

			// The following headers are mutually exclusive.
			HEADER_3DSX	= (1U << 1),
			HEADER_CIA	= (1U << 2),
			HEADER_TMD	= (1U << 3),	// ticket, tmd_header
			HEADER_NCSD	= (1U << 4),	// ncsd_header, cinfo_header
		};
		uint32_t headers_loaded;	// HeadersPresent

		// Media unit shift.
		// This is usually 9 (512 bytes), though NCSD images
		// can have larger shifts.
		uint8_t media_unit_shift;

		// Mutually-exclusive headers.
		// NOTE: These must be byteswapped on access.
		union {
			N3DS_3DSX_Header_t hb3dsx_header;
			struct {
				N3DS_CIA_Header_t cia_header;
				N3DS_Ticket_t ticket;
				N3DS_TMD_Header_t tmd_header;
				// Content start address.
				uint32_t content_start_addr;
			};
			struct {
				N3DS_NCSD_Header_NoSig_t ncsd_header;
				N3DS_NCSD_Card_Info_Header_t cinfo_header;
			};
		} mxh;

		// Permissions. (cached from headers)
		struct {
			bool isLoaded;		// True if perm is initialized.

			// True if "dangerous" permissions are set.
			// Note that this might not be set if it's a known
			// system title with a non-zero signature.
			bool isDangerous;

			// ARM9 descriptor version.
			uint8_t ioAccessVersion;

			uint32_t fsAccess;		// ARM11 FS access
			uint32_t ioAccess;		// ARM9 descriptors

			// Services.
			// Pointer to character array [34][8].
			// NOTE: This is stored within the ExHeader struct.
			// N3DS_SERVICE_MAX: 34 (number of services)
			// N3DS_SERVICE_LEN: 8 (length of service name)
			const char (*services)[N3DS_SERVICE_LEN];
		} perm;

		// Content chunk records. (CIA only)
		// Loaded by loadTicketAndTMD().
		ao::uvector<N3DS_Content_Chunk_Record_t> content_chunks;

		// TODO: Move the pointers to the union?
		// That requires careful memory management...

	private:
		// Primary NCCH reader.
		// NOTE: Do NOT access this directly!
		// Use loadNCCH() instead.
		NCCHReaderPtr ncch_reader;

	public:
		// Main content object.
		// - If SMDH is present, this is Nintendo3DS_SMDH.
		// - If SRL is present, this is NintendoDS.
		LibRpBase::RomDataPtr mainContent;

		/**
		 * Round a value to the next highest multiple of 64.
		 * @param value Value.
		 * @return Next highest multiple of 64.
		 */
		template<typename T>
		static inline T toNext64(T val)
		{
			return (val + static_cast<T>(63)) & ~(static_cast<T>(63));
		}

		/**
		 * Load the SMDH section.
		 * @return 0 on success; non-zero on error.
		 */
		int loadSMDH(void);

		/**
		 * Load the specified NCCH header.
		 * @param idx			[in] Content/partition index.
		 * @param pOutNcchReader	[out] Output variable for the NCCHReader.
		 * @return 0 on success; negative POSIX error code on error.
		 * NOTE: Caller must check NCCHReader::isOpen().
		 */
		int loadNCCH(int idx, NCCHReaderPtr &pOutNcchReader);

		/**
		 * Create an NCCHReader for the primary content.
		 * An NCCH reader is created as this->ncch_reader.
		 * @return this->ncch_reader on success; nullptr on error.
		 * NOTE: Caller must check NCCHReader::isOpen().
		 */
		const NCCHReaderConstPtr &loadNCCH(void);

		/**
		 * Get the NCCH header from the primary content.
		 * This uses loadNCCH() to get the NCCH reader.
		 * @return NCCH header, or nullptr on error.
		 */
		const N3DS_NCCH_Header_NoSig_t *loadNCCHHeader(void);

		/**
		 * Load the ticket and TMD header. (CIA only)
		 * The ticket is loaded into mxh.ticket.
		 * The TMD header is loaded into mxh.tmd_header.
		 * @return 0 on success; non-zero on error.
		 */
		int loadTicketAndTMD(void);

		/**
		 * Open the SRL if it isn't already opened.
		 * This operation only works for CIAs that contain an SRL.
		 * @return 0 on success; non-zero on error.
		 */
		int openSRL(void);

		/**
		 * Get the SMDH region code.
		 * @return SMDH region code, or 0 if it could not be obtained.
		 */
		uint32_t getSMDHRegionCode(void);

		/**
		 * Add the title ID, product code, and logo fields.
		 * Called by loadFieldData().
		 * @param showContentType If true, show the content type.
		 */
		void addTitleIdAndProductCodeFields(bool showContentType);

		/**
		 * Convert a Nintendo 3DS region value to a GameTDB language code.
		 * @param smdhRegion Nintendo 3DS region. (from SMDH)
		 * @param idRegion Game ID region.
		 *
		 * NOTE: Mulitple GameTDB language codes may be returned, including:
		 * - User-specified fallback language code for PAL.
		 * - General fallback language code.
		 *
		 * @return GameTDB language code(s), or empty vector if the region value is invalid.
		 * NOTE: The language code may need to be converted to uppercase!
		 */
		static std::vector<uint16_t> n3dsRegionToGameTDB(uint32_t smdhRegion, char idRegion);

		/**
		 * Convert a Nintendo 3DS version number field to a string.
		 * @param version Version number.
		 * @return String.
		 */
		static inline std::string n3dsVersionToString(uint16_t version)
		{
			// Reference: https://3dbrew.org/wiki/Titles
			return LibRpText::rp_sprintf("%u.%u.%u (v%u)",
				(static_cast<unsigned int>(version) >> 10),
				(static_cast<unsigned int>(version) >>  4) & 0x1F,
				(static_cast<unsigned int>(version) & 0x0F),
				 static_cast<unsigned int>(version));
		}

		/**
		 * Load the permissions values. (from ExHeader)
		 * @return 0 on success; non-zero on error.
		 */
		int loadPermissions(void);

		/**
		 * Add the Permissions fields. (part of ExHeader)
		 * A separate tab should be created by the caller first.
		 * @return 0 on success; non-zero on error.
		 */
		int addFields_permissions(void);
};

}
