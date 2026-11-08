/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ZXAY.cpp: ZX Spectrum AY audio reader.                                  *
 *                                                                         *
 * Copyright (c) 2018-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "ZXAY.hpp"
#include "RomData_p.hpp"

#include "zxay_structs.h"

// Other rom-properties libraries
#include "librptext/conversion.hpp"
#include "librptext/fourCC.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;
using std::vector;

// Uninitialized vector class
#include "uvector.h"

namespace LibRomData {

class ZXAYPrivate final : public RomDataPrivate
{
public:
	explicit ZXAYPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
public:
	RP_DISABLE_COPY(ZXAYPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// NOTE: Strings in the ZXAY format are accessed using
	// 16-bit relative pointers and are NULL-terminated.
	// In order to handle this decently, we'll use a buffer to
	// read data in as necessary.
	// NOTE 2: Assuming cp1252 encoding.
	rp::uvector<uint8_t> zxay_buf;

public:
	/**
	 * Expand zxay_buf to the specified size.
	 * This is done by reading more data from the file.
	 * @param req_size Requested size
	 * @return Actual size
	 */
	size_t expand_zxay_buf(size_t req_size);

	/**
	 * Read a C string from the file.
	 * @param p_address Offset in the file that contains the 16-bit pointer
	 * @return C string (as std::string), or empty string on error.
	 */
	string readCString(off64_t p_address);
};

ROMDATA_IMPL(ZXAY)

/** ZXAYPrivate **/

/* RomDataInfo */
const array<const char*, 1+1> ZXAYPrivate::exts = {{
	".ay",

	nullptr
}};
const array<const char*, 1+1> ZXAYPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"audio/x-ay",

	nullptr
}};
const RomDataInfo ZXAYPrivate::romDataInfo = {
	"ZXAY", exts.data(), mimeTypes.data()
};

ZXAYPrivate::ZXAYPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{}

/**
 * Expand zxay_buf to the specified size.
 * This is done by reading more data from the file.
 * @param req_size Requested size
 * @return Actual size
 */
size_t ZXAYPrivate::expand_zxay_buf(size_t req_size)
{
	if (!file || !file->isOpen()) {
		// File isn't open...
		return zxay_buf.size();
	}

	if (req_size <= zxay_buf.size()) {
		// Not going to shrink the buffer...
		return zxay_buf.size();
	}

	// Resize zxay_buf.
	const size_t cur_size = zxay_buf.size();
	zxay_buf.resize(req_size);

	// Read more data from the file.
	size_t to_read = req_size - cur_size;
	size_t sz_read = file->seekAndRead(
		static_cast<off64_t>(cur_size),
		&zxay_buf[cur_size], to_read);
	if (sz_read != to_read) {
		// Short read? May still be able to continue.
		zxay_buf.resize(cur_size + sz_read);
	}

	// Buffer resized.
	return zxay_buf.size();
}

/**
 * Read a C string from the file.
 * @param p_address Offset in the file that contains the 16-bit pointer
 * @return C string (as std::string), or empty string on error.
 */
string ZXAYPrivate::readCString(off64_t p_address)
{
	string s_ret;

	// Check if the address is available in the buffer.
	size_t req_size = static_cast<size_t>(p_address) + 2;
	if (zxay_buf.size() < req_size) {
		size_t new_size = expand_zxay_buf(req_size + 256);
		if (new_size < req_size) {
			// Still unable to read data...
			return s_ret;
		}
	}

	// Read the pointer and adjust it for relative offsets.
	// TODO: Optimize byteswapping?
	off64_t p_data = (zxay_buf[p_address] << 8) | zxay_buf[p_address + 1];
	p_data += p_address;

	// Check if the data address is available in the buffer.
	// NOTE: Assuming up to 256 bytes for the string.
	req_size = static_cast<size_t>(p_data) + 256;
	if (zxay_buf.size() < req_size) {
		// Need to read more data into the buffer!
		// Read the required size.
		size_t new_size = expand_zxay_buf(req_size + 256);
		if (new_size < req_size) {
			// Still unable to read data...
			return s_ret;
		}
	}

	// Search the buffer for a NULL terminator, starting at p_data.
	// NOTE: Assuming cp1252 encoding.
	size_t remain = zxay_buf.size() - p_data;
	const char *p = reinterpret_cast<const char*>(&zxay_buf[p_data]);
	const char *p_nul = static_cast<const char*>(memchr(p, '\0', remain));
	if (p_nul) {
		// Found the NULL terminator.
		s_ret = cp1252_to_utf8(p, (p_nul - p));
	} else {
		// Did not find a NULL terminator...
		// TODO: Expand the buffer again?
		s_ret = cp1252_to_utf8(p, remain);
	}

	return s_ret;
}

/** ZXAY **/

/**
 * Read a ZXAY audio file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
ZXAY::ZXAY(const IRpFilePtr &file)
	: super(new ZXAYPrivate(file))
{
	RP_D(ZXAY);
	d->fileType = FileType::AudioFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ZXAY header.
	// NOTE: Using a resizable buffer in order to handle string pointers.
	d->zxay_buf.resize(256);
	d->file->rewind();
	size_t size = d->file->read(d->zxay_buf.data(), d->zxay_buf.size());
	if (size < sizeof(ZXAY_Header)) {
		d->file.reset();
		return;
	} else if (size < d->zxay_buf.size()) {
		// Probably won't work, but we'll keep going for now.
		d->zxay_buf.resize(size);
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, static_cast<uint32_t>(d->zxay_buf.size()), d->zxay_buf.data()},
		nullptr,	// ext (not needed for ZXAY)
		0		// szFile (not needed for ZXAY)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int ZXAY::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(ZXAY_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const ZXAY_Header *const zxayHeader =
		reinterpret_cast<const ZXAY_Header*>(info->header.pData);

	// Check for the ZXAY magic number.
	if (zxayHeader->magic != cpu_to_be32(ZXAY_MAGIC)) {
		// Not found.
		return -1;
	}

	// Check the ZXAY type.
	if (zxayHeader->type == cpu_to_be32(ZXAY_TYPE_EMUL) ||
	    zxayHeader->type == cpu_to_be32(ZXAY_TYPE_AMAD))
	{
		// Found a valid ZXAY type.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *ZXAY::systemName(unsigned int type) const
{
	RP_D(const ZXAY);
	if (!d->isValid || !isSystemNameTypeValid(type)) {
		return nullptr;
	}

	// ZXAY has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"ZXAY::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"ZX Spectrum AY-file Format", "ZXAY", "ZXAY", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int ZXAY::loadFieldData(void)
{
	RP_D(ZXAY);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// ZXAY header
	// NOTE: It's located at the start of a data buffer.
	// NOTE: zxay_buf may be reallocated when expanded, so we can't
	// keep this pointer!
	const ZXAY_Header *zxayHeader;
#define UPDATE_ZXAYHEADER_PTR()  do { \
	zxayHeader = reinterpret_cast<const ZXAY_Header*>(d->zxay_buf.data()); \
} while (0)
	UPDATE_ZXAYHEADER_PTR();
	const bool isEMUL = (zxayHeader->type == cpu_to_be32(ZXAY_TYPE_EMUL));

	d->fields.reserve(5);	// Maximum of 5 fields.

	// Type
	d->fields.addField_string(C_("RomData", "Type"),
		fourCCtoString(be32_to_cpu(zxayHeader->type)));

	// File version
	d->fields.addField_string_numeric(C_("ZXAY", "File Version"),
		zxayHeader->file_version);

	// Player version
	d->fields.addField_string_numeric(C_("ZXAY", "Player Version"),
		zxayHeader->player_version);

	// Author
	string s_author = d->readCString(offsetof(ZXAY_Header, p_author));
	if (!s_author.empty()) {
		d->fields.addField_string(C_("RomData|Audio", "Author"), s_author);
	}

	// Description
	string s_description = d->readCString(offsetof(ZXAY_Header, p_misc));
	if (!s_description.empty()) {
		d->fields.addField_string(C_("RomData", "Description"), s_description);
	}

	// Song list
	UPDATE_ZXAYHEADER_PTR();
	const unsigned int song_count = zxayHeader->song_count + 1;

	// Ensure the song structs are in zxay_buf.
	unsigned int song_struct_addr = be16_to_cpu(zxayHeader->p_first_song_struct) + offsetof(ZXAY_Header, p_first_song_struct);
	const size_t req_size = song_struct_addr + (sizeof(ZXAY_Song_Struct) * song_count);
	size_t new_size = d->expand_zxay_buf(req_size);
	if (new_size >= req_size) {
		unsigned int song_num = 0;
		auto *const v_song_list = new RomFields::ListData_t(song_count);
		auto dest_iter = v_song_list->begin();
		const auto song_list_end = v_song_list->end();
		for ( ; dest_iter != song_list_end; ++dest_iter, song_num++, song_struct_addr += sizeof(ZXAY_Song_Struct)) {
			vector<string> &data_row = *dest_iter;
			data_row.reserve(2);	// 2 fields per row.

			// Song number
			data_row.push_back(fmt::format_int(song_num).str());

			// Song name
			data_row.push_back(d->readCString(song_struct_addr + offsetof(ZXAY_Song_Struct, p_song_name)));

			if (isEMUL) {
				// Get the ZXAY_EMUL_Song_Data.
				const ZXAY_Song_Struct *const pSongStruct =
					reinterpret_cast<const ZXAY_Song_Struct*>(d->zxay_buf.data() + song_struct_addr);
				unsigned int emul_song_data_addr = be16_to_cpu(pSongStruct->p_song_data) +
					offsetof(ZXAY_Song_Struct, p_song_data) + song_struct_addr;
				const size_t req_size_emul = emul_song_data_addr + sizeof(ZXAY_EMUL_Song_Data);
				// NOTE: Including +256 for read-ahead.
				new_size = d->expand_zxay_buf(req_size_emul + 256);
				if (new_size >= req_size_emul) {
					const ZXAY_EMUL_Song_Data *pEmulSongData =
						reinterpret_cast<const ZXAY_EMUL_Song_Data*>(d->zxay_buf.data() + emul_song_data_addr);

					// song_length is in units of 1/50th of a second. (PAL)
					const unsigned int duration = be16_to_cpu(pEmulSongData->song_length);
					const unsigned int min = (duration / 50) / 60;
					const unsigned int sec = (duration / 50) % 60;
					const unsigned int cs =  (duration % 50) * 2;
					data_row.push_back(fmt::format(FSTR("{:d}:{:0>2d}.{:0>2d}"), min, sec, cs));
				} else {
					// Unable to read ZXAY_EMUL_Song_Data...
					data_row.push_back("<?>");
				}
			}
		}

		// TODO: Duration for EMUL?
		static const array<const char*, 3> song_list_hdr = {{
			NOP_C_("ZXAY|SongList", "#"),
			NOP_C_("ZXAY|SongList", "Name"),
			NOP_C_("ZXAY|SongList", "Duration"),	// 'EMUL' type only
		}};
		vector<string> *const v_song_list_hdr = RomFields::strArrayToVector_i18n("ZXAY|SongList",
			song_list_hdr.data(), (isEMUL) ? 3 : 2);

		RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_SEPARATE_ROW, 8);
		params.headers = v_song_list_hdr;
		params.data.single = v_song_list;
		params.col_attrs.align_headers = AFLD_ALIGN3(TXA_D, TXA_D, TXA_D);
		params.col_attrs.align_data = AFLD_ALIGN3(TXA_R, TXA_D, TXA_R);
		params.col_attrs.sizing = AFLD_ALIGN3(COLSZ_R, COLSZ_S, COLSZ_R);
		params.col_attrs.sorting = AFLD_ALIGN3(COLSORT_NUM, COLSORT_NC, COLSORT_NUM);
		params.col_attrs.sort_col = 0; // #
		params.col_attrs.sort_dir = RomFields::COLSORTORDER_ASCENDING;
		d->fields.addField_listData(C_("ZXAY", "Song List"), &params);
	}

	// Finished reading the field data.
	return d->fields.count();
}

} // namespace LibRomData
