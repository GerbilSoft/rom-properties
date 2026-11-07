/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NSF.hpp: NSF audio reader.                                              *
 *                                                                         *
 * Copyright (c) 2018-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "NSF.hpp"
#include "RomData_p.hpp"

#include "nsf_structs.h"
#include "nsfe_structs.h"

// Other rom-properties libraries
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

class NSFPrivate final : public RomDataPrivate
{
public:
	explicit NSFPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
public:
	RP_DISABLE_COPY(NSFPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 2+1> exts;
	static const array<const char*, 2+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// Audio format
	enum class AudioFormat {
		Unknown = -1,

		NSF	= 0,	// NSF or NSF2
		NSFe	= 1,	// NSFe

		Max
	};
	AudioFormat audioFormat;

public:
	// NSF header
	// NOTE: **NOT** byteswapped in memory.
	union {
		NSF_Header nsf;		// NSF/NSF2 header
		NSFe_Header nsfe;	// NSFe header (just a magic number)
	} header;

	// NSFe metadata
	// NOTE: All strings are encoded in UTF-8 in the original file.
	struct {
		// NOTE: The following chunks may NOT be present in an NSF2 file:
		// 'INFO', 'DATA', 'BANK', 'NSF2'
		// These chunks will be indicated with [E].

		// Bitfield indicating which chunks are present.
		union {
			struct {
				uint16_t INFO : 1;	// [E]
				uint16_t BANK : 1;	// [E]
				uint16_t RATE : 1;
				uint16_t NSF2 : 1;	// [E]
				uint16_t VRC7 : 1;
				uint16_t regn : 1;

				// Text chunks
				uint16_t auth : 1;
				uint16_t text : 1;

				// Track list chunks
				uint16_t plst : 1;
				uint16_t psfx : 1;
				uint16_t time : 1;
				uint16_t fade : 1;
				uint16_t tlbl : 1;
				uint16_t taut : 1;

				// Per-device chunks
				uint16_t mixe : 1;
			};
			uint16_t u16;
		} has;

		NSFe_INFO_t INFO;	// [E]
		NSFe_BANK_t BANK;	// [E]
		NSFe_RATE_t RATE;
		NSFe_NSF2_t NSF2;	// [E]
		NSFe_VRC7_t VRC7;
		NSFe_regn_t regn;

		// 'auth' chunk
		struct auth_t {
			explicit auth_t()
				: game_title(nullptr)
				, artist(nullptr)
				, copyright(nullptr)
				, ripper(nullptr)
			{}

			// Table containing the four NULL-terminated strings.
			rp::uvector<char> strtbl;

			// Pointers into strtbl.
			union {
				struct {
					const char *game_title;
					const char *artist;
					const char *copyright;
					const char *ripper;
				};
				const char *str[4];
			};
		};
		auth_t auth;

		// 'text' chunk
		string text;

		// Track struct
		// Contains information from the following chunks:
		// 'plst', 'psfx', 'time', 'fade', 'tlbl', 'taut'
		struct track_t {
			explicit track_t()
				: order(0)
				, is_sfx(false)
				, duration(0)
				, fade(0)
			{}

			explicit track_t(uint8_t index)
				: order(index)
				, is_sfx(false)
				, duration(0)
				, fade(0)
			{}

			uint8_t order;		// 'plst': playback order (if 'plst' is not set, this is set to the track index)
			bool is_sfx;		// 'psfx': true if this is a "sound effect" instead of music
			uint32_t duration;	// 'time': track duration, in milliseconds
			uint32_t fade;		// 'fade': fadeout duration, in milliseconds (default is 0)

			string name;		// 'tlbl': Track name (label)
			string author;		// 'taut': Track author
		};
		vector<track_t> tracks;
	} nsfe;

public:
	/**
	 * Get the NSFe metadata start address.
	 * If NSF1, this is invalid and returns 0.
	 * If NSF2, converts nsf2_prg_size from 24-bit to uint32_t.
	 * If NSFe, returns the beginning of the file, right after the "NSFE" magic number.
	 * @return NSFe metadata start address, or 0 on error.
	 */
	uint32_t getNSFeMetadataStartAddress(void) const;

	/**
	 * Read the NSFe metadata.
	 * @return 0 on success; non-zero on error.
	 */
	int readNSFeMetadata(void);
};

ROMDATA_IMPL(NSF)

/** NSFPrivate **/

/* RomDataInfo */
const array<const char*, 2+1> NSFPrivate::exts = {{
	".nsf",		// Common file extension
	".nsfe",	// Sometimes used for NSFe

	nullptr
}};
const array<const char*, 2+1> NSFPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"audio/x-nsf",
	"audio/x-nsfe",

	nullptr
}};
const RomDataInfo NSFPrivate::romDataInfo = {
	"NSF", exts.data(), mimeTypes.data()
};

NSFPrivate::NSFPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the NSF header struct.
	memset(&header, 0, sizeof(header));

	// Initialize the NSFe chunk struct.
	nsfe.has.u16 = 0;
}

/**
 * Get the NSFe metadata start address.
 * If NSF1, this is invalid and returns 0.
 * If NSF2, converts nsf2_prg_size from 24-bit to uint32_t.
 * If NSFe, returns the beginning of the file, right after the "NSFE" magic number.
 * @return NSFe metadata start address, or 0 on error.
 */
uint32_t NSFPrivate::getNSFeMetadataStartAddress(void) const
{
	uint32_t ret = 0;

	switch (audioFormat) {
		default:
			assert(!"Invalid NSF audio format.");
			break;

		case NSFPrivate::AudioFormat::NSF: {
			const NSF_Header *const nsfHeader = &header.nsf;
			assert(nsfHeader->version == 2);
			if (nsfHeader->version != 2) {
				break;
			}

			ret = (nsfHeader->nsf2_prg_size[2] << 16) |
			      (nsfHeader->nsf2_prg_size[1] <<  8) |
			       nsfHeader->nsf2_prg_size[0];
			if (ret != 0) {
				// Non-zero value indicates the metadata is present.
				// The value actually contains the program data length,
				// so add the NSF header size to get the start of the
				// metadata section.
				ret += static_cast<unsigned int>(sizeof(NSF_Header));
			}
			break;
		}

		case NSFPrivate::AudioFormat::NSFe:
			ret = static_cast<unsigned int>(sizeof(NSFe_Header));
			break;
	}

	return ret;
}

/**
 * Read the NSFe metadata.
 * @return 0 on success; non-zero on error.
 */
int NSFPrivate::readNSFeMetadata(void)
{
	uint32_t addr = getNSFeMetadataStartAddress();
	if (addr == 0) {
		// No NSFe metadata...
		return -ENOENT;
	}

	// Seek to the beginning of the NSFe metadata.
	int ret = file->seek(addr);
	if (ret != 0) {
		// Seek error.
		int err = -file->lastError();
		if (err == 0) {
			err = -EIO;
		}
		return err;
	}

	// If NSF2: 'INFO', 'DATA', 'BANK', and 'NSF2' chunks are invalid.
	const bool isNSF2 = (audioFormat != NSFPrivate::AudioFormat::NSFe);

	// Start reading chunk headers.
	while (true) {
		NSFe_Chunk_Header chunkHeader;
		size_t size = file->read(&chunkHeader, sizeof(chunkHeader));
		if (size != sizeof(chunkHeader)) {
			// Read error.
			if (file->eof()) {
				// End of file. (No 'NEND' chunk, which seems to happen with NSF2.)
				break;
			}

			int err = -file->lastError();
			if (err == 0) {
				err = -EIO;
			}
			return err;
		}

		const uint32_t chunk_size = le32_to_cpu(chunkHeader.size);

		// Check the chunk ID.
		bool done = false;
		bool skip = false;	// Skip the chunk. (assuming file->read() was not called)
		switch (be32_to_cpu(chunkHeader.fourCC)) {
			case 0:	// assuming all zeroes means an error
			case NSFe_Chunk_NEND:
				// End of file.
				done = true;
				break;

			case NSFe_Chunk_auth: {
				// 'auth' chunk. Read the chunk data into nsfe.auth.strtbl.
				// NOTE: Limiting to 16,384 bytes.
				assert(chunk_size > 0);
				assert(chunk_size <= 16384);
				if (chunk_size == 0 || chunk_size > 16384) {
					// Empty chunk, or too big...
					skip = true;
					break;
				}

				rp::uvector<char> &strtbl = nsfe.auth.strtbl;
				strtbl.resize(chunk_size);
				size = file->read(const_cast<char*>(strtbl.data()), chunk_size);
				if (size != chunk_size) {
					// Read error.
					strtbl.clear();
					int err = -file->lastError();
					if (err == 0) {
						err = -EIO;
					}
					return err;
				}

				// Make sure the strings are NULL-terminated.
				if (strtbl.back() != '\0') {
					strtbl.push_back('\0');
				}

				// Should contain 4 strings.
				// We'll allow up to 4, ignoring any additional strings.
				const char *p = nsfe.auth.strtbl.data();
				const char *const p_end = nsfe.auth.strtbl.data() + nsfe.auth.strtbl.size();
				for (size_t i = 0; i < ARRAY_SIZE(nsfe.auth.str) && p < p_end; i++) {
					size_t remain = p_end - p;
					const char *const p_nul = static_cast<const char*>(memchr(p, '\0', remain));
					assert(p_nul != nullptr);
					if (!p_nul) {
						// Shouldn't happen...
						break;
					}

					if ((p_nul - p) <= 1) {
						// Empty string.
						nsfe.auth.str[i] = nullptr;
					} else {
						// Set the string pointer.
						nsfe.auth.str[i] = p;
					}

					// Next string.
					p = p_nul + 1;
				}
				nsfe.has.auth = 1;
				break;
			}

			case NSFe_Chunk_text: {
				// 'text' chunk. Read the chunk data into nsfe.text.
				// NOTE: Limiting to 16,384 bytes.
				assert(chunk_size > 0);
				assert(chunk_size <= 16384);
				if (chunk_size == 0 || chunk_size > 16384) {
					// Too big...
					skip = true;
					break;
				}

				nsfe.text.resize(chunk_size);
				size = file->read(const_cast<char*>(nsfe.text.data()), chunk_size);
				if (size != chunk_size) {
					// Read error.
					nsfe.text.clear();
					int err = -file->lastError();
					if (err == 0) {
						err = -EIO;
					}
					return err;
				}
				nsfe.has.text = 1;
				break;
			}

			default:
				skip = true;
				break;
		}

		if (skip) {
			// Skipping this chunk...
			ret = file->seek(chunk_size, IRpFile::SeekWhence::Cur);
			if (ret != 0) {
				int err = -file->lastError();
				if (err == 0) {
					err = -EIO;
				}
				return err;
			}
		}

		if (done) {
			break;
		}
	}

	return 0;
}

/** NSF **/

/**
 * Read an NSF audio file.
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
NSF::NSF(const IRpFilePtr &file)
	: super(new NSFPrivate(file))
{
	RP_D(NSF);
	d->fileType = FileType::AudioFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the NSF header.
	d->file->rewind();
	size_t size = d->file->read(&d->header, sizeof(d->header));
	if (size != sizeof(d->header)) {
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(d->header), reinterpret_cast<const uint8_t*>(&d->header)},
		nullptr,	// ext (not needed for NSF)
		0		// szFile (not needed for NSF)
	};
	d->audioFormat = static_cast<NSFPrivate::AudioFormat>(isRomSupported_static(&info));
	d->isValid = (static_cast<int>(d->audioFormat) >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	switch (d->audioFormat) {
		default:
			assert(!"Invalid NSF audio format.");
			d->isValid = false;
			d->file.reset();
			break;

		case NSFPrivate::AudioFormat::NSF: {
			d->mimeType = "audio/x-nsf";	// unofficial

			const NSF_Header *const nsfHeader = &d->header.nsf;
			d->isPAL = (nsfHeader->tv_system == NSF_TV_PAL);

			// If this is NSF2, and metadata is available,
			// read the metadata.
			if (nsfHeader->version == 2) {
				d->readNSFeMetadata();
			}
			break;
		}

		case NSFPrivate::AudioFormat::NSFe:
			d->mimeType = "audio/x-nsfe";	// unofficial

			// Read the NSFe metadata.
			d->readNSFeMetadata();
			if (d->nsfe.has.INFO) {
				d->isPAL = (d->nsfe.INFO.pal_ntsc_bits == NSFe_INFO_IS_PAL);
			}
			break;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int NSF::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(NSF_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(NSFPrivate::AudioFormat::Unknown);
	}

	const NSF_Header *const nsfHeader =
		reinterpret_cast<const NSF_Header*>(info->header.pData);

	// Check for the NSF magic number.
	if (!memcmp(nsfHeader->magic, NSF_MAGIC, sizeof(nsfHeader->magic))) {
		// Found the NSF magic number.
		// Version number must be either 1 or 2.
		if (nsfHeader->version == 1 || nsfHeader->version == 2) {
			// Valid version number.
			return static_cast<int>(NSFPrivate::AudioFormat::NSF);
		}
	}

	// Check for the NSFe magic number.
	const NSFe_Header *const nsfeHeader =
		reinterpret_cast<const NSFe_Header*>(info->header.pData);
	if (nsfeHeader->magic == cpu_to_be32(NSFe_MAGIC)) {
		// It's an NSFe file.
		return static_cast<int>(NSFPrivate::AudioFormat::NSFe);
	}

	// Not supported.
	return static_cast<int>(NSFPrivate::AudioFormat::Unknown);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *NSF::systemName(unsigned int type) const
{
	RP_D(const NSF);
	if (!d->isValid || !isSystemNameTypeValid(type)) {
		return nullptr;
	}

	// NSF has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"NSF::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"Nintendo Sound Format", "NSF", "NSF", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int NSF::loadFieldData(void)
{
	RP_D(NSF);
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

	// Field count:
	// - NSF Version: 1
	// - NSF1/NSF2: 10
	// - NSFe: 5
	d->fields.reserve(1+10+5);

	// NSF version
	string s_nsfVersion;
	switch (d->audioFormat) {
		default:
			assert(!"Invalid NSF audio format.");
			break;
		case NSFPrivate::AudioFormat::NSF:
			fmt::format_to(std::back_inserter(s_nsfVersion), FSTR("NSF{:d}"), d->header.nsf.version);
			break;
		case NSFPrivate::AudioFormat::NSFe:
			s_nsfVersion = "NSFe";
			break;
	}
	d->fields.addField_string(C_("NSF", "NSF Version"),
		(likely(!s_nsfVersion.empty())) ? s_nsfVersion.c_str() : C_("RomData", "Unknown"));

	bool hasNSFtab = false;
	if (d->audioFormat == NSFPrivate::AudioFormat::NSF) {
		// NSF header
		const NSF_Header *const nsfHeader = &d->header.nsf;
		d->fields.setTabName(0, "NSF");
		hasNSFtab = true;

		// NOTE: The NSF specification says ASCII, but I'm assuming
		// the text is cp1252 and/or Shift-JIS.

		// Title
		if (nsfHeader->title[0] != 0) {
			d->fields.addField_string(C_("RomData|Audio", "Title"),
				cp1252_sjis_to_utf8(nsfHeader->title, sizeof(nsfHeader->title)));
		}

		// Composer
		if (nsfHeader->composer[0] != 0) {
			d->fields.addField_string(C_("RomData|Audio", "Composer"),
				cp1252_sjis_to_utf8(nsfHeader->composer, sizeof(nsfHeader->composer)));
		}

		// Copyright
		if (nsfHeader->copyright[0] != 0) {
			d->fields.addField_string(C_("RomData", "Copyright"),
				cp1252_sjis_to_utf8(nsfHeader->copyright, sizeof(nsfHeader->copyright)));
		}

		// Number of tracks
		d->fields.addField_string_numeric(C_("RomData|Audio", "Track Count"),
			nsfHeader->track_count);

		// Default track number
		d->fields.addField_string_numeric(C_("RomData|Audio", "Default Track #"),
			nsfHeader->default_track);

		// Load address
		d->fields.addField_string_numeric(C_("RomData|Audio", "Load Address"),
			le16_to_cpu(nsfHeader->load_address),
			RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

		// Init address
		d->fields.addField_string_numeric(C_("RomData|Audio", "Init Address"),
			le16_to_cpu(nsfHeader->init_address),
			RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

		// Play address
		d->fields.addField_string_numeric(C_("RomData|Audio", "Play Address"),
			le16_to_cpu(nsfHeader->play_address),
			RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

		// TV System
		// TODO: NTSC/PAL framerates?
		// NOTE: NSF uses an enum, not a bitfield.
		static const array<const char*, 2> tv_system_bitfield_names = {{
			"NTSC", "PAL",
		}};
		uint32_t bfval = nsfHeader->tv_system;
		if (bfval < NSF_TV_MAX) {
			bfval++;
		} else {
			bfval = 0;
		}
		vector<string> *const v_tv_system_bitfield_names = RomFields::strArrayToVector(tv_system_bitfield_names);
		d->fields.addField_bitfield(C_("NSF", "TV System"),
			v_tv_system_bitfield_names, 0, bfval);

		// Expansion audio
		static const array<const char*, 6> expansion_bitfield_names = {{
			"Konami VRC6", "Konami VRC7", "2C33 (FDS)",
			"MMC5", "Namco N163", "Sunsoft 5B",
		}};
		vector<string> *const v_expansion_bitfield_names = RomFields::strArrayToVector(expansion_bitfield_names);
		d->fields.addField_bitfield(C_("NSF", "Expansion"),
			v_expansion_bitfield_names, 3, nsfHeader->expansion_audio);

		if (nsfHeader->version >= 2) {
			// NSF2 feature flags
			static const array<const char*, 4> nsf2_feature_flags_names = {{
				"IRQ support", "Non-returning INIT",
				"No PLAY subroutine", "NSFe has mandatory chunk",
			}};

			// Bits 0-3 are reserved. Adjust the flags to match the names.
			const uint32_t adjflags = (nsfHeader->nsf2_feature_flags) >> 4;
			vector<string> *const v_nsf2_feature_flags_names = RomFields::strArrayToVector(nsf2_feature_flags_names);
			d->fields.addField_bitfield(C_("NSF", "NSF2 Feature Flags"),
				v_nsf2_feature_flags_names, 2, adjflags);
		}
	}

	if (d->nsfe.has.u16 != 0) {
		// NSFe metadata was loaded.
		if (hasNSFtab) {
			d->fields.addTab("NSFe");
		} else {
			d->fields.setTabName(0, "NSFe");
		}

		const auto *nsfe = &d->nsfe;

		if (nsfe->has.auth) {
			// 'auth' chunk is available.

			// Title
			if (nsfe->auth.game_title) {
				d->fields.addField_string(C_("RomData|Audio", "Title"), nsfe->auth.game_title);
			}

			// Artist
			if (nsfe->auth.artist) {
				d->fields.addField_string(C_("RomData|Audio", "Composer"), nsfe->auth.artist);
			}

			// Copyright
			if (nsfe->auth.copyright) {
				d->fields.addField_string(C_("RomData", "Copyright"), nsfe->auth.copyright);
			}

			// Ripper
			if (nsfe->auth.ripper) {
				d->fields.addField_string(C_("RomData|Audio", "Ripper"), nsfe->auth.ripper);
			}
		}

		if (nsfe->has.text && likely(!nsfe->text.empty())) {
			// 'text' chunk is available.
			d->fields.addField_string(C_("NSF", "Description"), nsfe->text);
		}
	}

	// Finished reading the field data.
	return d->fields.count();
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int NSF::loadMetaData(void)
{
	RP_D(NSF);
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

	if (d->audioFormat == NSFPrivate::AudioFormat::NSF) {
		// NSF header
		// TODO: Override NSF2 fields with NSFe metadata, if present?
		const NSF_Header *const nsfHeader = &d->header.nsf;
		d->metaData.reserve(3);	// Maximum of 3 metadata properties.

		// Title
		if (nsfHeader->title[0] != 0) {
			d->metaData.addMetaData_string(Property::Title,
				cp1252_sjis_to_utf8(nsfHeader->title, sizeof(nsfHeader->title)));
		}

		// Composer
		if (nsfHeader->composer[0] != 0) {
			d->metaData.addMetaData_string(Property::Composer,
				cp1252_sjis_to_utf8(nsfHeader->composer, sizeof(nsfHeader->composer)));
		}

		// Copyright
		if (nsfHeader->copyright[0] != 0) {
			d->metaData.addMetaData_string(Property::Copyright,
				cp1252_sjis_to_utf8(nsfHeader->copyright, sizeof(nsfHeader->copyright)));
		}
	}

	// Finished reading the metadata.
	return d->metaData.count();
}

} // namespace LibRomData
