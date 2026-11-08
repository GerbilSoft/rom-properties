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
				uint16_t any_tlist : 1;	// set if any of the chunks are present
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
				: order(-1)
				, is_sfx(false)
				, duration(-1)
				, fade(-1)
			{}

			int16_t order;		// 'plst': playback order (-1 if not set)
			bool is_sfx;		// 'psfx': true if this is a "sound effect" instead of music
			int32_t duration;	// 'time': track duration, in milliseconds (negative for not specified)
			int32_t fade;		// 'fade': fadeout duration, in milliseconds (negative for not specified)

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
	 * Resize nsfe.tracks to match the track count.
	 * @return Number of tracks (or 0 if no track count) on success; negative POSIX error code on error.
	 */
	int initNSFeTracks(void);

	/**
	 * Read the NSFe metadata.
	 * @return 0 on success; negative POSIX error code on error.
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
 * Resize nsfe.tracks to match the track count.
 * @return Number of tracks (or 0 if no track count) on success; negative POSIX error code on error.
 */
int NSFPrivate::initNSFeTracks(void)
{
	if (!nsfe.tracks.empty()) {
		// nsfe.tracks was already initialized.
		return static_cast<int>(nsfe.tracks.size());
	}

	uint8_t track_count = 0;

	switch (audioFormat) {
		default:
			assert(!"Invalid NSF audio format.");
			return -EINVAL;

		case NSFPrivate::AudioFormat::NSF:
			track_count = header.nsf.track_count;
			break;

		case NSFPrivate::AudioFormat::NSFe:
			// 'INFO' chunk must have been loaded before calling this function.
			assert(nsfe.has.INFO);
			if (!nsfe.has.INFO) {
				return -EINVAL;
			}
			track_count = nsfe.INFO.song_count;
	}

	nsfe.tracks.resize(track_count);
	return static_cast<int>(track_count);
}

/**
 * Read the NSFe metadata.
 * @return 0 on success; negative POSIX error code on error.
 */
int NSFPrivate::readNSFeMetadata(void)
{
	off64_t pos = getNSFeMetadataStartAddress();
	if (pos == 0) {
		// No NSFe metadata...
		return -ENOENT;
	}

	// NOTE: We need to maintain our own seek pointer because
	// some chunks may be larger than what we're expecting.
	// Hence, we'll use file->seekAndRead(pos) for every chunk read.

	// If NSF2: 'INFO', 'DATA', 'BANK', and 'NSF2' chunks are invalid.
	const bool isNSF2 = (audioFormat != NSFPrivate::AudioFormat::NSFe);

	// Temporary read buffer
	rp::uvector<uint8_t> buf;

	/**
	 * Read a data chunk into the temporary buffer.
	 * @param chunk_size Chunk size
	 * @return 0 on success; positive if the chunk should be skipped; negative POSIX error code on error.
	 */
	auto read_chunk = [&buf, this](unsigned int chunk_size) -> int
	{
		// Read the chunk data into the temporary buffer.
		// NOTE: Limiting to 16,384 bytes.
		static constexpr unsigned int CHUNK_SIZE_MAX = 16384;
		assert(chunk_size > 0);
		assert(chunk_size <= CHUNK_SIZE_MAX);
		if (chunk_size == 0) {
			// Empty chunk. Skip it.
			return 1;
		} else if (chunk_size > CHUNK_SIZE_MAX) {
			// Chunk is too big. Skip it.
			return 2;
		}

		buf.resize(chunk_size);
		size_t size = file->read(buf.data(), buf.size());
		if (size != buf.size()) {
			// Read error.
			int err = -file->lastError();
			if (err == 0) {
				err = -EIO;
			}
			return err;
		}

		return 0;
	};

	// Start reading chunk headers.
	while (true) {
		NSFe_Chunk_Header chunkHeader;
		size_t size = file->seekAndRead(pos, &chunkHeader, sizeof(chunkHeader));
		if (size != sizeof(chunkHeader)) {
			// Seek and/or read error.
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
		switch (be32_to_cpu(chunkHeader.fourCC)) {
			case 0:	// assuming all zeroes means an error
			case NSFe_Chunk_NEND:
				// End of file.
				done = true;
				break;

			case NSFe_Chunk_INFO: {
				// 'info' chunk
				// NOTE: Chunk may be larger than 10 bytes. Ignore the extra data.
				assert(!isNSF2);
				assert(chunk_size >= sizeof(NSFe_INFO_t));
				if (isNSF2 || chunk_size < sizeof(NSFe_INFO_t)) {
					// isNSF2: This chunk is not allowed.
					// Other: Chunk size is incorrect.
					break;
				}

				size = file->read(&nsfe.INFO, sizeof(nsfe.INFO));
				if (size != sizeof(nsfe.INFO)) {
					// Read error.
					int err = -file->lastError();
					if (err == 0) {
						err = -EIO;
					}
					return err;
				}
				nsfe.has.INFO = 1;
				break;
			}

			case NSFe_Chunk_plst: {
				// 'plst' chunk
				int track_count = initNSFeTracks();
				if (track_count <= 0) {
					// No tracks...
					break;
				}

				// Chunk contains one byte per track.
				// NOTE: May not contain every track number, and may have
				// duplicated track numbers.
				assert(chunk_size > 0);
				if (chunk_size == 0) {
					// Chunk is empty.
					break;
				}

				// Read the chunk data into the temporary buffer.
				int err = read_chunk(chunk_size);
				if (err > 0) {
					// Skip the chunk.
					break;
				} else if (err < 0) {
					// Error reading the chunk...
					return err;
				}

				// Update the track list.
				const uint8_t *p = buf.data();
				const uint8_t *const p_end = p + buf.size();
				int16_t order = 0;
				for (; p < p_end; p++) {
					unsigned int i = static_cast<unsigned int>(*p);
					if (i < nsfe.tracks.size()) {
						// TODO: Handle duplicates?
						if (nsfe.tracks[i].order < 0) {
							nsfe.tracks[i].order = order++;
						}
					}
				}
				nsfe.has.plst = 1;
				nsfe.has.any_tlist = 1;
				break;
			}

			case NSFe_Chunk_psfx: {
				// 'psfx' chunk
				int track_count = initNSFeTracks();
				if (track_count <= 0) {
					// No tracks...
					break;
				}

				// Chunk contains a list of which tracks are "sound effects".
				assert(chunk_size > 0);
				if (chunk_size == 0) {
					// Chunk is empty.
					break;
				}

				// Read the chunk data into the temporary buffer.
				int err = read_chunk(chunk_size);
				if (err > 0) {
					// Skip the chunk.
					break;
				} else if (err < 0) {
					// Error reading the chunk...
					return err;
				}

				// Update the track list.
				const uint8_t *p = buf.data();
				const uint8_t *const p_end = p + buf.size();
				for (; p < p_end; p++) {
					unsigned int i = static_cast<unsigned int>(*p);
					if (i < nsfe.tracks.size()) {
						nsfe.tracks[i].is_sfx = true;
					}
				}
				nsfe.has.psfx = 1;
				nsfe.has.any_tlist = 1;
				break;
			}

			case NSFe_Chunk_time: {
				// 'time' chunk
				int track_count = initNSFeTracks();
				if (track_count <= 0) {
					// No tracks...
					break;
				}

				// Chunk contains a list of 32-bit times (in milliseconds),
				// one for each track. (Note that it may be shorter than expected.)
				assert(chunk_size > 0);
				if (chunk_size == 0) {
					// Chunk is empty.
					break;
				}

				// Read the chunk data into the temporary buffer.
				int err = read_chunk(chunk_size);
				if (err > 0) {
					// Skip the chunk.
					break;
				} else if (err < 0) {
					// Error reading the chunk...
					return err;
				}

				// Update the track list.
				const uint32_t *p = reinterpret_cast<const uint32_t*>(buf.data());
				const uint32_t *const p_end = p + (buf.size() / sizeof(uint32_t));
				for (int i = 0; i < track_count && p < p_end; i++, p++) {
					nsfe.tracks[i].duration = le32_to_cpu(*p);
				}
				nsfe.has.time = 1;
				nsfe.has.any_tlist = 1;
				break;
			}

			case NSFe_Chunk_tlbl: {
				// 'tlbl' chunk
				int track_count = initNSFeTracks();
				if (track_count <= 0) {
					// No tracks...
					break;
				}

				// Read the chunk data into the temporary buffer.
				int err = read_chunk(chunk_size);
				if (err > 0) {
					// Skip the chunk.
					break;
				} else if (err < 0) {
					// Error reading the chunk...
					return err;
				}

				// Make sure the strings are NULL-terminated.
				if (buf.back() != '\0') {
					buf.push_back('\0');
				}

				// Should contain track_count strings.
				// We'll allow up to track_count, ignoring any additional strings.
				const char *p = reinterpret_cast<const char*>(buf.data());
				const char *const p_end = p + buf.size();
				for (int i = 0; i < track_count && p < p_end; i++) {
					auto &track = nsfe.tracks[i];

					size_t remain = p_end - p;
					const char *const p_nul = static_cast<const char*>(memchr(p, '\0', remain));
					assert(p_nul != nullptr);
					if (!p_nul) {
						// Shouldn't happen...
						break;
					}

					if ((p_nul - p) <= 1) {
						// Empty string.
						track.name.clear();
					} else {
						// Copy the string data.
						track.name.assign(p, (p_nul - p));
					}

					// Next string.
					p = p_nul + 1;
				}
				nsfe.has.tlbl = 1;
				nsfe.has.any_tlist = 1;
				break;
			}

			case NSFe_Chunk_auth: {
				// 'auth' chunk
				// Read the chunk data into nsfe.auth.strtbl.
				// NOTE: Limiting to 16,384 bytes.
				assert(chunk_size > 0);
				assert(chunk_size <= 16384);
				if (chunk_size == 0 || chunk_size > 16384) {
					// Empty chunk, or too big...
					break;
				}

				rp::uvector<char> &strtbl = nsfe.auth.strtbl;
				strtbl.resize(chunk_size);
				size = file->read(strtbl.data(), strtbl.size());
				if (size != strtbl.size()) {
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
				// 'text' chunk
				// Read the chunk data into nsfe.text.
				// NOTE: Limiting to 16,384 bytes.
				assert(chunk_size > 0);
				assert(chunk_size <= 16384);
				if (chunk_size == 0 || chunk_size > 16384) {
					// Too big...
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
				break;
		}

		if (done) {
			break;
		}

		// Add the chunk header and chunk size to the seek position.
		pos += sizeof(chunkHeader) + chunk_size;
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
	// - NSFe:
	//   - 'auth': 4
	//   - 'INFO': 7
	//   - 'text': 1
	d->fields.reserve(1+10+4+7+1);

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

	// TV system
	static const array<const char*, 2> tv_system_bitfield_names = {{
		"NTSC", "PAL",
	}};

	// Expansion audio
	static const array<const char*, 7> expansion_bitfield_names = {{
		"Konami VRC6", "Konami VRC7", "2C33 (FDS)",
		"MMC5", "Namco N163", "Sunsoft 5B",
		"VT02+"
	}};

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

		// TV system
		// TODO: NTSC/PAL framerates?
		// NOTE: NSF uses an enum, not a bitfield.
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

		// NOTE: Using the same ordering as NSF2, even though it
		// doesn't necessarily match NSFe ordering.
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

		if (nsfe->has.INFO) {
			// Number of tracks
			d->fields.addField_string_numeric(C_("RomData|Audio", "Track Count"),
				nsfe->INFO.song_count);

			// Default track number
			d->fields.addField_string_numeric(C_("RomData|Audio", "Default Track #"),
				nsfe->INFO.starting_song);

			// Load address
			d->fields.addField_string_numeric(C_("RomData|Audio", "Load Address"),
				le16_to_cpu(nsfe->INFO.load_address),
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

			// Init address
			d->fields.addField_string_numeric(C_("RomData|Audio", "Init Address"),
				le16_to_cpu(nsfe->INFO.init_address),
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

			// Play address
			d->fields.addField_string_numeric(C_("RomData|Audio", "Play Address"),
				le16_to_cpu(nsfe->INFO.play_address),
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

			// TV System
			// TODO: NTSC/PAL framerates?
			// NOTE: NSFe uses an unusual format:
			// - Bit 0: clear for NTSC, set for PAL
			// - Bit 1: if set, supports both NTSC and PAL
			uint32_t bfval;
			if (nsfe->INFO.extra_sound_chips & NSFe_INFO_DUAL_NTSC_PAL) {
				bfval = 3;	// NTSC and PAL
			} else if (nsfe->INFO.extra_sound_chips & NSFe_INFO_TV_TYPE_MASK) {
				bfval = 2;	// PAL
			} else {
				bfval = 1;	// NTSC
			}
			vector<string> *const v_tv_system_bitfield_names = RomFields::strArrayToVector(tv_system_bitfield_names);
			d->fields.addField_bitfield(C_("NSF", "TV System"),
				v_tv_system_bitfield_names, 0, bfval);

			// Expansion audio
			vector<string> *const v_expansion_bitfield_names = RomFields::strArrayToVector(expansion_bitfield_names);
			d->fields.addField_bitfield(C_("NSF", "Expansion"),
				v_expansion_bitfield_names, 3, nsfe->INFO.extra_sound_chips);
		}

		if (nsfe->has.text && likely(!nsfe->text.empty())) {
			// 'text' chunk is available.
			d->fields.addField_string(C_("NSF", "Description"), nsfe->text);
		}

		if (nsfe->has.any_tlist && !nsfe->tracks.empty()) {
			// Track list
			// NOTE: Not showing columns if we don't have the data for them.
			const char *const s_yes = C_("RomData", "Yes");
			const char *const s_no = C_("RomData", "No");

			unsigned int track_num = 0;
			auto *const vv_track_list = new RomFields::ListData_t(nsfe->tracks.size());
			auto src_iter = nsfe->tracks.cbegin();
			auto dest_iter = vv_track_list->begin();
			const auto track_list_end = vv_track_list->end();
			for ( ; dest_iter != track_list_end; ++src_iter, ++dest_iter, track_num++) {
				vector<string> &data_row = *dest_iter;
				data_row.reserve(7);	// 7 fields per row.

				data_row.push_back(fmt::format_int(track_num).str());
				if (nsfe->has.plst) {
					if (src_iter->order >= 0) {
						data_row.push_back(fmt::format_int(src_iter->order).str());
					} else {
						data_row.push_back(string());
					}
				}
				if (nsfe->has.psfx) {
					data_row.push_back(src_iter->is_sfx ? s_yes : s_no);
				}
				if (nsfe->has.tlbl) {
					data_row.push_back(src_iter->name);
				}
				if (nsfe->has.taut) {
					data_row.push_back(src_iter->author);
				}

				// Format duration as m:ss.ddd.
				// TODO: Separate function?
				if (nsfe->has.time) {
					const int32_t duration = src_iter->duration;
					if (duration >= 0) {
						const uint32_t u_duration = static_cast<unsigned int>(duration);
						const uint32_t min = (u_duration / 1000) / 60;
						const uint32_t sec = (u_duration / 1000) % 60;
						const uint32_t ms =  (u_duration % 1000);
						data_row.push_back(fmt::format(FSTR("{:d}:{:0>2d}.{:0>3d}"), min, sec, ms));
					} else {
						data_row.push_back("<?>");
					}
				}

				// Format fade as m:ss.ddd.
				// TODO: Separate function?
				if (nsfe->has.fade) {
					const int32_t duration = src_iter->fade;
					if (duration >= 0) {
						const uint32_t u_duration = static_cast<unsigned int>(duration);
						const uint32_t min = (u_duration / 1000) / 60;
						const uint32_t sec = (u_duration / 1000) % 60;
						const uint32_t ms =  (u_duration % 1000);
						data_row.push_back(fmt::format(FSTR("{:d}:{:0>2d}.{:0>3d}"), min, sec, ms));
					} else {
						data_row.push_back("<?>");
					}
				}
			}

			vector<const char*> track_list_headers;
			track_list_headers.reserve(7);
			track_list_headers.push_back(C_("NSF|TrackList", "#"));
			if (nsfe->has.plst) {
				track_list_headers.push_back(C_("NSF|TrackList", "Order"));
			}
			if (nsfe->has.psfx) {
				track_list_headers.push_back(C_("NSF|TrackList", "Is SFX"));
			}
			if (nsfe->has.tlbl) {
				track_list_headers.push_back(C_("NSF|TrackList", "Name"));
			}
			if (nsfe->has.taut) {
				track_list_headers.push_back(C_("NSF|TrackList", "Author"));
			}
			if (nsfe->has.time) {
				track_list_headers.push_back(C_("NSF|TrackList", "Duration"));
			}
			if (nsfe->has.fade) {
				track_list_headers.push_back(C_("NSF|TrackList", "Fade"));
			}
			vector<string> *const v_track_list_headers = RomFields::strArrayToVector(track_list_headers);

			RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_SEPARATE_ROW, 8);
			params.headers = v_track_list_headers;
			params.data.single = vv_track_list;
			d->fields.addField_listData(C_("NSF", "Track List"), &params);
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
