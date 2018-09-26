/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SNDH.hpp: Atari ST SNDH audio reader.                                   *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

// Reference: http://sndh.atari.org/fileformat.php
// NOTE: The header format consists of tags that may be in any order,
// so we don't have a structs file.

#include "SNDH.hpp"
#include "librpbase/RomData_p.hpp"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include "librpbase/ctypex.h"
#include <cassert>
#include <cerrno>
#include <cstring>
#include <ctime>

// C++ includes.
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
using std::pair;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(SNDH)

class SNDHPrivate : public RomDataPrivate
{
	public:
		SNDHPrivate(SNDH *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(SNDHPrivate)

	public:
		// Parsed tags.
		struct TagData {
			bool tags_read;		// True if tags were read successfully.

			string title;		// Song title.
			string composer;	// Composer name.
			string ripper;		// Ripper name.
			string converter;	// Converter name.

			unsigned int subtunes;	// Subtune count.
			struct {
				// Timer frequencies.
				// 0 if not specified.
				unsigned int A;
				unsigned int B;
				unsigned int C;
				unsigned int D;
			} timer;
			unsigned int vblank;		// VBlank frequency. (50/60)
			unsigned int year;		// Year of release.
			unsigned int def_subtune;	// Default subtune.

			// TODO: Use std::pair<>?
			// The SNDH format uses separate tags for each, though...
			vector<string> subtune_names;		// Subtune names.
			vector<unsigned int> subtune_lengths;	// Subtune lengths, in seconds.

			TagData() : tags_read(false), subtunes(0), vblank(0), year(0), def_subtune(0)
			{
				// Clear the timer values.
				memset(&timer, 0, sizeof(timer));
			}
		};

		/**
		 * Read a NULL-terminated ASCII string from an arbitrary binary buffer.
		 * @param p	[in/out] String pointer. (will be adjusted to point after the NULL terminator)
		 * @param p_end	[in] End of buffer pointer.
		 * @param p_err	[out] Set to true if the string is out of bounds.
		 * @return ASCII string.
		 */
		static string readStrFromBuffer(const uint8_t **p, const uint8_t *p_end, bool *p_err);

		/**
		 * Read a NULL-terminated unsigned ASCII number from an arbitrary binary buffer.
		 * @param p	[in/out] String pointer. (will be adjusted to point after the NULL terminator)
		 * @param p_end	[in] End of buffer pointer.
		 * @param p_err	[out] Set to true if the string is out of bounds.
		 * @return Unsigned ASCII number.
		 */
		static unsigned int readAsciiNumberFromBuffer(const uint8_t **p, const uint8_t *p_end, bool *p_err);

		/**
		 * Parse the tags from the open SNDH file.
		 * @return TagData object.
		 */
		TagData parseTags(void);
};

/** SNDHPrivate **/

SNDHPrivate::SNDHPrivate(SNDH *q, IRpFile *file)
	: super(q, file)
{ }

/**
 * Read a NULL-terminated ASCII string from an arbitrary binary buffer.
 * @param p	[in/out] String pointer. (will be adjusted to point after the NULL terminator)
 * @param p_end	[in] End of buffer pointer.
 * @param p_err	[out] Set to true if the string is out of bounds.
 * @return ASCII string.
 */
string SNDHPrivate::readStrFromBuffer(const uint8_t **p, const uint8_t *p_end, bool *p_err)
{
	if (*p >= p_end) {
		// Out of bounds.
		*p_err = true;
		return string();
	}

	const uint8_t *const s_end = reinterpret_cast<const uint8_t*>(memchr(*p, 0, p_end-*p));
	if (!s_end) {
		// Out of bounds.
		*p_err = true;
		return string();
	}

	*p_err = false;
	if (s_end > *p) {
		string ret = latin1_to_utf8(reinterpret_cast<const char*>(*p), (int)(s_end-*p));
		// Skip the string, then add one for the NULL terminator.
		*p += ret.size() + 1;
		return ret;
	}

	// Empty string.
	return string();
}

/**
 * Read a NULL-terminated unsigned ASCII number from an arbitrary binary buffer.
 * @param p	[in/out] String pointer. (will be adjusted to point after the NULL terminator)
 * @param p_end	[in] End of buffer pointer.
 * @param p_err	[out] Set to true if the string is out of bounds.
 * @return Unsigned ASCII number.
 */
unsigned int SNDHPrivate::readAsciiNumberFromBuffer(const uint8_t **p, const uint8_t *p_end, bool *p_err)
{
	if (*p >= p_end) {
		// Out of bounds.
		*p_err = true;
		return 0;
	}

	char *endptr;
	unsigned int ret = (unsigned int)strtoul(reinterpret_cast<const char*>(*p), &endptr, 10);
	if (endptr >= reinterpret_cast<const char*>(p_end) || *endptr != '\0') {
		// Invalid value.
		*p_err = true;
		return 0;
	}
	// endptr is the NULL terminator, so go one past that.
	*p = reinterpret_cast<const uint8_t*>(endptr) + 1;
	return ret;
}

/**
 * Parse the tags from the open SNDH file.
 * @return TagData object.
 */
SNDHPrivate::TagData SNDHPrivate::parseTags(void)
{
	TagData tags;

	// FIXME: May be compressed with "Pack-Ice".
	// https://sourceforge.net/projects/sc68/files/unice68/

	// Read up to 4 KB from the beginning of the file.
	// TODO: Support larger headers?
	static const size_t HEADER_SIZE = 4096;
	unique_ptr<uint8_t[]> header(new uint8_t[HEADER_SIZE+1]);
	size_t sz = file->seekAndRead(0, header.get(), HEADER_SIZE);
	if (sz < 16) {
		// Not enough data for "SNDH" and "HDNS".
		return tags;
	}
	header[HEADER_SIZE] = 0;	// ensure NULL-termination

	// Verify the header.
	// NOTE: SNDH is defined as using CRLF line endings,
	// but we'll allow LF line endings too.
	const uint8_t *p = header.get();
	const uint8_t *const p_end = header.get() + HEADER_SIZE;
	if (memcmp(&header[12], "SNDH", 4) != 0) {
		// Not SNDH.
		return tags;
	}
	p += 4;

	// NOTE: Strings are in ASCII.
	// TODO: Add ascii_to_utf8(). For now, using latin1_to_utf8().
	bool err = false;
	while (p < p_end) {
		// Check for 32-bit tags.
		// NOTE: This might not be aligned, so we'll need to handle
		// alignment properly later.
		uint32_t tag = be32_to_cpu(*(reinterpret_cast<const uint32_t*>(p)));
		bool is32 = true;
		switch (tag) {
			case 'TITL':
				// Song title.
				p += 4;
				tags.title = readStrFromBuffer(&p, p_end, &err);
				if (err) {
					// An error occurred.
					p = p_end;
				}
				break;

			case 'COMM':
				// Composer.
				p += 4;
				tags.composer = readStrFromBuffer(&p, p_end, &err);
				if (err) {
					// An error occurred.
					p = p_end;
				}
				break;

			case 'RIPP':
				// Ripper.
				p += 4;
				tags.ripper = readStrFromBuffer(&p, p_end, &err);
				if (err) {
					// An error occurred.
					p = p_end;
				}
				break;

			case 'CONV':
				// Converter.
				p += 4;
				tags.converter = readStrFromBuffer(&p, p_end, &err);
				if (err) {
					// An error occurred.
					p = p_end;
				}
				break;

			case 'YEAR': {
				// Year of release.
				// String uses ASCII digits, so use strtoul().
				p += 4;
				tags.year = readAsciiNumberFromBuffer(&p, p_end, &err);
				if (err) {
					// An error occurred.
 					p = p_end;
 				}
 				break;
 			}

			case 'HDNS':
				// End of SNDH header.
				p = p_end;
				break;

			default:
				// Need to check for 16-bit tags next.
				is32 = false;
				break;
		}

		if (is32) {
			// A 32-bit tag was parsed.
			// Check the next tag.
			continue;
		}

		// Check for 16-bit tags.
		tag = be16_to_cpu(*(reinterpret_cast<const uint16_t*>(p)));
		switch (tag) {
			case '##':
				// # of subtunes.
				// String uses ASCII digits, so use strtoul().
				p += 2;
				tags.subtunes = readAsciiNumberFromBuffer(&p, p_end, &err);
				if (err) {
					// An error occurred.
					p = p_end;
				}
				break;

			default:
				// Unsupported tag...
				p++;
				break;
		}
	}

	// Tags parsed.
	tags.tags_read = true;
	return tags;
}

/** SNDH **/

/**
 * Read an SNDH audio file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
SNDH::SNDH(IRpFile *file)
	: super(new SNDHPrivate(this, file))
{
	RP_D(SNDH);
	d->className = "SNDH";
	d->fileType = FTYPE_AUDIO_FILE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the SNDH header.
	uint8_t buf[16];
	d->file->rewind();
	size_t size = d->file->read(buf, sizeof(buf));
	if (size != sizeof(buf)) {
		delete d->file;
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(buf);
	info.header.pData = buf;
	info.ext = nullptr;	// Not needed for SNDH.
	info.szFile = 0;	// Not needed for SNDH.
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		delete d->file;
		d->file = nullptr;
		return;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int SNDH::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 16)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for "SNDH" at offset 12.
	const uint32_t *const pData32 = reinterpret_cast<const uint32_t*>(info->header.pData);
	if (pData32[3] == cpu_to_be32('SNDH')) {
		// Found the SNDH magic number.
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
const char *SNDH::systemName(unsigned int type) const
{
	RP_D(const SNDH);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// SNDH has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"SNDH::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Atari ST SNDH Audio",
		"SNDH",
		"SNDH",
		nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *SNDH::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".sndh",

		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *SNDH::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"audio/x-sndh",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int SNDH::loadFieldData(void)
{
	RP_D(SNDH);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// Get the tags.
	SNDHPrivate::TagData tags = d->parseTags();
	if (!tags.tags_read) {
		// No tags.
		return 0;
	}

	// SNDH header.
	d->fields->reserve(4);	// Maximum of 4 fields.

	// NOTE: Some strings have trailing spaces.

	// Song title.
	if (!tags.title.empty()) {
		d->fields->addField_string(C_("SNDH", "Song Title"),
			tags.title, RomFields::STRF_TRIM_END);
	}

	// Composer.
	if (!tags.composer.empty()) {
		d->fields->addField_string(C_("SNDH", "Composer"),
			tags.composer, RomFields::STRF_TRIM_END);
	}

	// Ripper.
	if (!tags.ripper.empty()) {
		d->fields->addField_string(C_("SNDH", "Ripper"),
			tags.ripper, RomFields::STRF_TRIM_END);
	}

	// Converter.
	if (!tags.converter.empty()) {
		d->fields->addField_string(C_("SNDH", "Converter"),
			tags.converter, RomFields::STRF_TRIM_END);
	}

	// Year of release.
	if (tags.year != 0) {
		d->fields->addField_string_numeric(C_("SNDH", "Year of Release"), tags.year);
	}

	// Number of subtunes.
	if (tags.subtunes != 0) {
		d->fields->addField_string_numeric(C_("SNDH", "# of Subtunes"), tags.subtunes);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

}
