/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SNDH.hpp: Atari ST SNDH audio reader.                                   *
 *                                                                         *
 * Copyright (c) 2018-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://sndh.atari.org/fileformat.php
// NOTE: The header format consists of tags that may be in any order,
// so we don't have a structs file.
#include "stdafx.h"
#include "libromdata/config.libromdata.h"

#include "SNDH.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// unice68
#ifdef ENABLE_UNICE68
#  include "unice68.h"
#endif

// for memmem() if it's not available in <string.h>
#include "librptext/libc.h"

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class SNDHPrivate final : public RomDataPrivate
{
public:
	explicit SNDHPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(SNDHPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// Parsed tags
	struct TagData {
		bool tags_read;		// True if tags were read successfully.

		string title;		// Song title
		string composer;	// Composer name
		string ripper;		// Ripper name
		string converter;	// Converter name

		unsigned int subtunes;			// Subtune count (If 0 or 1, entire file is one song.)
							// NOTE: 0 (or missing) means SNDHv1; 1 means SNDHv2.
		unsigned int vblank_freq;		// VBlank frequency (50/60)
		array<unsigned int, 4> timer_freq;	// Timer frequencies (A, B, C, D) [0 if not specified]
		unsigned int year;			// Year of release
		unsigned int def_subtune;		// Default subtune

		// TODO: Use std::pair<>?
		// The SNDH format uses separate tags for each, though...
		vector<string> subtune_names;		// Subtune names.
		vector<unsigned int> subtune_lengths;	// Subtune lengths, in seconds.

		TagData() : tags_read(false), subtunes(0), vblank_freq(0), year(0), def_subtune(0)
		{
			// Clear the timer frequencies.
			timer_freq.fill(0);
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

ROMDATA_IMPL(SNDH)

/** SNDHPrivate **/

/* RomDataInfo */
const array<const char*, 1+1> SNDHPrivate::exts = {{
	".sndh",

	nullptr
}};
const array<const char*, 1+1> SNDHPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"audio/x-sndh",

	nullptr
}};
const RomDataInfo SNDHPrivate::romDataInfo = {
	"SNDH", exts.data(), mimeTypes.data()
};

SNDHPrivate::SNDHPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
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
		return {};
	}

	const uint8_t *const s_end = reinterpret_cast<const uint8_t*>(memchr(*p, 0, p_end-*p));
	if (!s_end) {
		// Out of bounds.
		*p_err = true;
		return {};
	}

	*p_err = false;
	if (s_end > *p) {
		// NOTE: Strings are encoded using an Atari ST-specific character set.
		// It's ASCII-compatible, but control characters and high-bit characters
		// are different compared to Latin-1 and other code pages.
		// Reference: https://en.wikipedia.org/wiki/Atari_ST_character_set
		const uint8_t *const p_old = *p;
		*p = s_end + 1;
		return cpN_to_utf8(CP_RP_ATARIST, reinterpret_cast<const char*>(p_old), (int)(s_end-p_old));
	}

	// Empty string.
	return {};
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
		// NOTE: Return the parsed number anyway, in case it's still useful.
		// For exmaple, 'YEAR' tags might have "1995/2013".
		// See: Modmate/almoST_real_(ENtRACte).sndh
		*p_err = true;
		return ret;
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

	// Read up to 4 KB from the beginning of the file.
	// TODO: Support larger headers?
	size_t headerSize = 4096;
	unique_ptr<uint8_t[]> header(new uint8_t[headerSize+1]);
	size_t sz = file->seekAndRead(0, header.get(), headerSize);
	if (sz < 16) {
		// Not enough data for "SNDH" and "HDNS".
		return tags;
	} else if (sz < headerSize) {
		headerSize = sz;
	}
	header[headerSize] = 0;	// ensure NULL-termination

	// Check if this is packed with ICE.
	// https://sourceforge.net/projects/sc68/files/unice68/
	const uint32_t *const pData32 = reinterpret_cast<const uint32_t*>(header.get());
	if (pData32[0] == cpu_to_be32('ICE!') || pData32[0] == cpu_to_be32('Ice!')) {
		// Packed with ICE.
		// FIXME: Return an error if unpacking fails.
#ifdef ENABLE_UNICE68
		// Decompress the data.
		// FIXME: unice68_depacker() only supports decompressing the entire file.
		// Add a variant that supports buffer sizes.
		const off64_t fileSize_o = file->size();
		if (fileSize_o < 16) {
			return tags;
		}

		const size_t fileSize = static_cast<size_t>(fileSize_o);
		unique_ptr<uint8_t[]> inbuf(new uint8_t[fileSize]);
		sz = file->seekAndRead(0, inbuf.get(), fileSize);
		if (sz != (size_t)fileSize) {
			return tags;
		}

		// unice68 uses a margin of 16 bytes for input size vs. file size,
		// and a maximum of 16 MB for the output size.
		static constexpr int SNDH_SIZE_MARGIN = 16;
		static constexpr int SNDH_SIZE_MAX = (1 << 24);
		int csize = 0;
		int reqSize = unice68_depacked_size(inbuf.get(), &csize);
		assert(reqSize > 0);
		assert(abs(csize - static_cast<int>(fileSize)) < SNDH_SIZE_MARGIN);
		if (reqSize <= 0 || reqSize > SNDH_SIZE_MAX ||
		    abs(csize - static_cast<int>(fileSize)) >= SNDH_SIZE_MARGIN)
		{
			// Size is out of range.
			return tags;
		}
		header.reset(new uint8_t[reqSize+1]);
		int ret = unice68_depacker(header.get(), reqSize, inbuf.get(), fileSize);
		if (ret != 0) {
			return tags;
		}
		headerSize = std::min(4096, reqSize);
		header[headerSize] = 0;	// ensure NULL-termination
#else /* !ENABLE_UNICE68 */
		// unice68 is disabled.
		return tags;
#endif /* ENABLE_UNICE68 */
	}

	// Verify the header.
	// NOTE: SNDH is defined as using CRLF line endings,
	// but we'll allow LF line endings too.
	const uint8_t *p = header.get();
	const uint8_t *const p_end = header.get() + headerSize;
	if (memcmp(&header[12], "SNDH", 4) != 0) {
		// Not SNDH.
		return tags;
	}
	p += 16;

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
				assert(!err);
				if (err) {
					// An error occurred.
					p = p_end;
				}
				break;

			case 'COMM':
				// Composer.
				p += 4;
				tags.composer = readStrFromBuffer(&p, p_end, &err);
				assert(!err);
				if (err) {
					// An error occurred.
					p = p_end;
				}
				break;

			case 'RIPP':
			case 'ripp':	// header corruption in: Marcer/Bellanotte_Chip.sndh
				// Ripper.
				p += 4;
				tags.ripper = readStrFromBuffer(&p, p_end, &err);
				assert(!err);
				if (err) {
					// An error occurred.
					p = p_end;
				}
				break;

			case 'CONV':
				// Converter.
				p += 4;
				tags.converter = readStrFromBuffer(&p, p_end, &err);
				assert(!err);
				if (err) {
					// An error occurred.
					p = p_end;
				}
				break;

			case 'YEAR': {
				// Year of release.
				// String uses ASCII digits, so use strtoul().
				// FIXME: Modmate/almoST_real_(ENtRACte).sndh has "1995/2013".
				// Ignoring errors for now.
				p += 4;
				tags.year = readAsciiNumberFromBuffer(&p, p_end, &err);
				if (err) {
					// An error occurred.
					if (tags.year != 0) {
						// FIXME: Might be two years, e.g. "1995/2013".
						// This might also be "198x".
						// Ignore the non-numeric portion for now.
						const uint8_t *const p_nul = reinterpret_cast<const uint8_t*>(memchr(p, 0, p_end - p));
						if (p_nul) {
							err = false;	// Ignore this error for now.
							p = p_nul + 1;
						} else {
							p = p_end;
						}
					} else {
						// Invalid year, probably.
						p = p_end;
					}
 				}
 				break;
 			}

			case '!#SN':
			case '!#ST': {
				// Subtune names.

				// NOTE: If subtune count is 0 (no '##' tag), this is SNDHv1,
				// which doesn't support subtunes. Handle it as a single subtune.
				const unsigned int subtunes = (tags.subtunes > 0 ? tags.subtunes : 1);

				// The following WORDs are offsets from the tag,
				// and they point to NULL-terminated strings.
				// The next tag is immediately after the last string.
				assert(tags.subtune_names.empty());
				if (!tags.subtune_names.empty()) {
					// We already have subtune names.
					// This means there's a duplicate '!#SN' tag.
					p = p_end;
					break;
				}

				// NOTE: Some SNDH files are incorrect and assume offset 0
				// is the start of the text area.
				// - MotionRide/K0mar.sndh [NOTE: Seems to be working now...]
				// - Mr_Saigon/MSI_Sound_Demo.sndh
				// - The_Archmage/MSI_Sound_Demo.sndh (WHICH ONE?)
				// - Edd_the_Duck/Sonixx.sndh
				// - Roggie/Sonixx.sndh (WHICH ONE?)
				// - Povey_Rob/Quartet_1_0.sndh
				const uint16_t *p_tbl = reinterpret_cast<const uint16_t*>(p + 4);
				const unsigned int offset = (be16_to_cpu(*p_tbl) == 0 ? (4 + (subtunes * 2)) : 0);

				const uint8_t *p_next = nullptr;
				for (unsigned int i = 0; i < subtunes; i++, p_tbl++) {
					const uint8_t *p_str = p + be16_to_cpu(*p_tbl) + offset;
					string str = readStrFromBuffer(&p_str, p_end, &err);
					//assert(!err);	// FIXME: Breaks Johansen_Benny/Yahtzee.sndh.
					if (err) {
						// An error occured.
						break;
					}
					tags.subtune_names.emplace_back(std::move(str));

					if (p_str > p_next) {
						// This string is the farthest ahead so far.
						p_next = p_str;
					}
				}

				//assert(!err);	// FIXME: Breaks Johansen_Benny/Yahtzee.sndh.
				if (err) {
					// An error occurred.
					p = p_end;
					tags.subtune_names.clear();
				} else {
					// p_next is the next byte to read.
					// NOTE: http://sndh.atari.org/fileformat.php says it should be 16-bit aligned.
					p = p_next;
				}
				break;
			}

			case 'TIME': {
				// Subtune lengths, in seconds.
				// NOTE: This field is OPTIONAL.
				// Count_Zero/Decade_Demo_Quartet.sndh has '!#SN', but not 'TIME'.

				// NOTE: If subtune count is 0, this is SNDHv1,
				// which only supports one subtune.
				const unsigned int subtunes = (tags.subtunes > 0 ? tags.subtunes : 1);

				// Immediately following the tag is a table of WORDs,
				// with one element per subtune.
				const uint8_t *const p_next = p + 4 + (subtunes * 2);
				assert(p_next <= p_end);
				if (p_next > p_end) {
					// Out of bounds.
					p = p_end;
					break;
				}

				const uint16_t *p_tbl = reinterpret_cast<const uint16_t*>(p + 4);
				tags.subtune_lengths.resize(subtunes);
				for (unsigned int &pSubtuneLength : tags.subtune_lengths) {
					pSubtuneLength = be16_to_cpu(*p_tbl++);
				}

				p = p_next;
				break;
			}

			case 'FLAG': {
				// TODO: This is non-standard.
				// Observed variants: (after the tag)
				// - Two bytes, and a NULL terminator.
				// - Three bytes, and a NULL terminator.
				// - Five bytes, and maybe a NULL terminator.
				// NOTE: The data format for some of these seems to be
				// two bytes per subtune, two more bytes, then two NULL bytes.
				bool handled = false;
				if (p < p_end - 4+2) {
					if (p[4+2] == 0 && ISUPPER(p[4+2+1])) {
						p += 4+2+1;
						handled = true;
					}
				}

				if (!handled && p < p_end - 4+3) {
					if (p[4+3] == 0 && ISUPPER(p[4+3+1])) {
						p += 4+3+1;
						handled = true;
					}
				}

				// Check for the 5-byte version.
				if (!handled && (p < p_end - 4+5+1)) {
					// Might not have a NULL terminator.
					if (p[4+5] != 0 && ISUPPER(p[4+5+1])) {
						// No NULL terminator.
						p += 4+5;
						handled = true;
					} else {
						// NULL terminator.
						if (p[4] == '~' && p + 4+6+2 < p_end && ISUPPER(p[4+5+2])) {
							p += 4+5+1;
							handled = true;
						}
					}
				}

				// Search for `00 00`.
				if (!handled) {
					for (p += 4; p < p_end-1; p += 2) {
						if (p[0] == 0 && p[1] == 0) {
							// Found it!
							p += 2;
							handled = true;
							break;
						}
					}
				}

				assert(handled);
				if (!handled) {
					p = p_end;
					break;
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
				// NOTE: Digits might not be NULL-terminated,
				// so instead of using readAsciiNumberFromBuffer(),
				// parse the two digits manually.
				assert(p <= p_end - 4);
				if (p > p_end - 4) {
					// Out of bounds.
					p = p_end;
					break;
				}

				assert(ISDIGIT(p[2]));
				assert(ISDIGIT(p[3]));
				if (!ISDIGIT(p[2]) || !ISDIGIT(p[3])) {
					// Not digits.
					p = p_end;
					break;
				}

				tags.subtunes = ((p[2] - '0') * 10) + (p[3] - '0');
				p += 4;
				break;

			case '!V':
				// VBlank frequency.
				p += 2;
				tags.vblank_freq = readAsciiNumberFromBuffer(&p, p_end, &err);
				assert(!err);
				if (err) {
					// An error occurred.
					p = p_end;
				}
				break;

			case 'TA':
			case 'TB':
			case 'TC':
			case 'TD': {
				// Timer frequency.

				// Check for invalid digits after 'Tx'.
				// If present, this is probably the end of the header,
				// and the file is missing an HDNS tag.
				// See: Beast/Boring.sndh
				if (!ISDIGIT(p[2])) {
					// End of header.
					p = p_end;
					break;
				}

				const uint8_t idx = p[1] - 'A';
				p += 2;
				tags.timer_freq[idx] = readAsciiNumberFromBuffer(&p, p_end, &err);
				assert(!err);
				if (err) {
					// An error occurred.
					p = p_end;
				}
				break;
			}

			case '!#':
				// Default subtune.
				// NOTE: First subtune is 1, not 0.
				// TODO: Check that it doesn't exceed the subtune count?
				p += 2;
				tags.def_subtune = readAsciiNumberFromBuffer(&p, p_end, &err);
				assert(!err);
				if (err) {
					// An error occurred.
					p = p_end;
				}
				break;

			default:
				// Unsupported tag...
				// If this is a NULL byte or a space, find the next
				// non-NULL/non-space byte and continue.
				// Otherwise, it's an invalid tag, so stop processing.
				if (*p == 0 || *p == ' ') {
					while (p < p_end && (*p == 0 || *p == ' ')) {
						p++;
					}
				} else {
					// Invalid tag.
					p = p_end;
				}
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
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
SNDH::SNDH(const IRpFilePtr &file)
	: super(new SNDHPrivate(file))
{
	RP_D(SNDH);
	d->mimeType = "audio/x-sndh";	// unofficial, not on fd.o
	d->fileType = FileType::AudioFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the SNDH header.
	// NOTE: Reading up to 512 bytes to detect certain
	// ICE-packed files:
	// - Connolly_Sean/Viking_Child.sndh: Has 'HDNS' at 0x1F4.
	uint8_t buf[512];
	d->file->rewind();
	size_t size = d->file->read(buf, sizeof(buf));
	// NOTE: Allowing less than 512 bytes, since some
	// ICE-compressed SNDH files are really small.
	// - Lowe_Al/Kings_Quest_II.sndh: 453 bytes.
	if (size < 12) {
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, static_cast<uint32_t>(size), buf},
		nullptr,	// ext (not needed for SNDH)
		0		// szFile (not needed for SNDH)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
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

#ifdef ENABLE_UNICE68
	// Is it packed with ICE?
	if (pData32[0] == cpu_to_be32('ICE!') || pData32[0] == cpu_to_be32('Ice!')) {
		// Packed. Check for other SNDH data.
		// TODO: Test on test suite.
		// Reference: https://bugs.launchpad.net/ubuntu/+source/file/+bug/946696
		size_t sz = std::min<size_t>(512, info->header.size);
		if (sz > 12) {
			sz -= 12;
			// Check for fragments of known SNDH tags.
			// FIXME: The following ICE-compressed files are not being detected:
			// - Kauling_Andy/Infinity_One.sndh
			struct sndh_fragment_t {
				uint8_t len;
				char data[4];
			};
			static constexpr array<sndh_fragment_t, 5> fragments = {{
				{3, {'N','D','H',0}},
				{4, {'T','I','T','L'}},
				{4, {'C','O','N','V'}},
				{4, {'R','I','P','P'}},
				{4, {'H','D','N','S'}},
			}};

			for (const auto &fragment : fragments) {
				void *p = memmem(&info->header.pData[12], sz, fragment.data, fragment.len);
				if (p) {
					// Found a matching fragment.
					// TODO: Use a constant to indicate ICE-compressed?
					return 1;
				}
			}
		}
	}
#endif /* ENABLE_UNICE68 */

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

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"Atari ST SNDH Audio", "SNDH", "SNDH", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int SNDH::loadFieldData(void)
{
	RP_D(SNDH);
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

	// Get the tags.
	SNDHPrivate::TagData tags = d->parseTags();
	if (!tags.tags_read) {
		// No tags.
		return 0;
	}

	// SNDH header.
	d->fields.reserve(13);	// Maximum of 4 fields.

	// NOTE: Some strings have trailing spaces.

	// Song title.
	if (!tags.title.empty()) {
		d->fields.addField_string(C_("RomData|Audio", "Song Title"),
			tags.title, RomFields::STRF_TRIM_END);
	}

	// Composer.
	if (!tags.composer.empty()) {
		d->fields.addField_string(C_("RomData|Audio", "Composer"),
			tags.composer, RomFields::STRF_TRIM_END);
	}

	// Ripper.
	if (!tags.ripper.empty()) {
		d->fields.addField_string(C_("SNDH", "Ripper"),
			tags.ripper, RomFields::STRF_TRIM_END);
	}

	// Converter.
	if (!tags.converter.empty()) {
		d->fields.addField_string(C_("SNDH", "Converter"),
			tags.converter, RomFields::STRF_TRIM_END);
	}

	// Year of release.
	if (tags.year != 0) {
		d->fields.addField_string_numeric(C_("SNDH", "Year of Release"), tags.year);
	}

	// Number of subtunes.
	// TODO: Omit this if it's 0 or 1?
	d->fields.addField_string_numeric(C_("SNDH", "# of Subtunes"),
		(tags.subtunes > 0) ? tags.subtunes : 1);

	// NOTE: Tag listing on http://sndh.atari.org/fileformat.php lists
	// VBL *after* timers, but "Calling method and speed" lists
	// VBL *before* timers. We'll list it before timers.

	// VBlank frequency.
	const char *const s_hz = C_("RomData", "{:Ld} Hz");
	if (tags.vblank_freq != 0) {
		d->fields.addField_string(C_("SNDH", "VBlank Freq"),
			fmt::format(s_hz, tags.vblank_freq));
	}

	// Timer frequencies.
	// TODO: Use RFT_LISTDATA?
	// tr: Frequency of Timer A, Timer B, etc. ("Timer {:c}" is a single entity)
	const char *const s_timer_freq = C_("SNDH", "Timer {:c} Freq");
	for (int i = 0; i < ARRAY_SIZE_I(tags.timer_freq); i++) {
		if (tags.timer_freq[i] == 0)
			continue;

		d->fields.addField_string(
			fmt::format(s_timer_freq, 'A'+i).c_str(),
			fmt::format(s_hz, tags.timer_freq[i]));
	}

	// Default subtune.
	// NOTE: First subtune is 1, not 0.
	if (tags.subtunes > 1 && tags.def_subtune > 0) {
		d->fields.addField_string_numeric(C_("SMDH", "Default Subtune"), tags.def_subtune);
	}

	// Subtune list.
	// NOTE: We don't want to display the list if no subtune names are present
	// and we have a single subtune length, since that means we have only a
	// single song with a single duration.
	if (!tags.subtune_names.empty() || tags.subtune_lengths.size() > 1) {
		// NOTE: While most SMDH files have both '!#SN' and 'TIME',
		// some files might have only one or the other.
		// Example: Count_Zero/Decade_Demo_Quartet.sndh ('!#SN' only)
		const bool has_SN = !tags.subtune_names.empty();
		const bool has_TIME = !tags.subtune_lengths.empty();
		unsigned int col_count = 2 + (has_SN && has_TIME);

		// Some SNDH files have all zeroes for duration.
		// Example: Taylor_Nathan/180.sndh
		// If this is the case, and there are no names, don't bother showing the list.
		// TODO: Hide the third column if there are names but all zero durations?
		uint64_t duration_total = 0;

		const size_t count = std::max(tags.subtune_names.size(), tags.subtune_lengths.size());
		auto *const vv_subtune_list = new RomFields::ListData_t(count);
		unsigned int idx = 0;
		for (vector<string> &data_row : *vv_subtune_list) {
			data_row.reserve(col_count);	// 2 or 3 fields per row.

			data_row.emplace_back(fmt::to_string(idx+1));	// NOTE: First subtune is 1, not 0.
			if (has_SN) {
				if (idx < tags.subtune_names.size()) {
					data_row.emplace_back(tags.subtune_names.at(idx));
				} else {
					data_row.emplace_back();
				}
			}

			if (has_TIME) {
				if (idx < tags.subtune_lengths.size()) {
					// Format as m:ss.
					// TODO: Separate function?
					const unsigned int duration = tags.subtune_lengths.at(idx);
					duration_total += duration;
					const unsigned int min = duration / 60;
					const unsigned int sec = duration % 60;
					data_row.emplace_back(fmt::format(FSTR("{:d}:{:0>2d}"), min, sec));
				} else {
					data_row.emplace_back();
				}
			}

			// Next row.
			idx++;
		}

		if (!has_SN && has_TIME && duration_total == 0) {
			// No durations. Don't bother showing the list.
			delete vv_subtune_list;
		} else {
			array<const char*, 3> subtune_list_hdr = {{
				NOP_C_("SNDH|SubtuneList", "#"),
				nullptr,
				nullptr
			}};
			if (has_SN && has_TIME) {
				subtune_list_hdr[1] = NOP_C_("SNDH|SubtuneList", "Name");
				subtune_list_hdr[2] = NOP_C_("RomData|Audio", "Duration");
			} else if (has_SN) {
				subtune_list_hdr[1] = NOP_C_("SNDH|SubtuneList", "Name");
			} else if (has_TIME) {
				subtune_list_hdr[1] = NOP_C_("RomData|Audio", "Duration");
			} else {
				assert(!"Invalid combination of has_SN and has_TIME.");
				col_count = 1;
			}

			vector<string> *const v_subtune_list_hdr = RomFields::strArrayToVector_i18n(
				"SNDH|SubtuneList", subtune_list_hdr.data(), col_count);

			RomFields::AFLD_PARAMS params;
			params.headers = v_subtune_list_hdr;
			params.data.single = vv_subtune_list;
			d->fields.addField_listData(C_("SNDH", "Subtune List"), &params);
		}
	} else if (tags.subtune_names.empty() && tags.subtune_lengths.size() == 1) {
		// No subtune names, but we have one subtune length.
		// This means it's the length of the entire song.

		// Format as m:ss.
		// TODO: Separate function?
		const uint32_t duration = tags.subtune_lengths.at(0);
		const uint32_t min = duration / 60;
		const uint32_t sec = duration % 60;

		d->fields.addField_string(C_("RomData|Audio", "Duration"),
			fmt::format(FSTR("{:d}:{:0>2d}"), min, sec));
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int SNDH::loadMetaData(void)
{
	RP_D(SNDH);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// Get the tags.
	const SNDHPrivate::TagData tags = d->parseTags();
	if (!tags.tags_read) {
		// No tags.
		return 0;
	}

	d->metaData.reserve(4);	// Maximum of 4 metadata properties.

	// Song title.
	if (!tags.title.empty()) {
		d->metaData.addMetaData_string(Property::Title,
			tags.title, RomFields::STRF_TRIM_END);
	}

	// Composer.
	if (!tags.composer.empty()) {
		d->metaData.addMetaData_string(Property::Composer,
			tags.composer, RomFields::STRF_TRIM_END);
	}

	// Year of release.
	if (tags.year != 0) {
		d->metaData.addMetaData_uint(Property::ReleaseYear, tags.year);
	}

	// Duration.
	// This is the total duration of *all* subtunes.
	const unsigned int duration = std::accumulate(
		tags.subtune_lengths.cbegin(),
		tags.subtune_lengths.cend(), 0U);
	if (duration != 0) {
		// NOTE: Length is in milliseconds, so we need to
		// multiply duration by 1000.
		d->metaData.addMetaData_integer(Property::Duration, duration * 1000);
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData.count());
}

} // namespace LibRomData
