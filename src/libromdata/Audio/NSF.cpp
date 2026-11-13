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

// NOTE: Limiting the maximum chunk size to 16,384 bytes.
#define CHUNK_SIZE_MAX 16384U

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

	// String table offset type
	// Negative indciates empty string.
	// (We can't use 0 because the string table is loaded directly
	// from the NSFe metadata, and prepending a 0 wastes time.)
	typedef int32_t strtbl_offset_t;

	// NSFe metadata
	// NOTE: All strings are encoded in UTF-8 in the original file.
	struct nsfe_t {
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
				: game_title(-1)
				, artist(-1)
				, copyright(-1)
				, ripper(-1)
			{}

			// Table containing the four NULL-terminated strings.
			rp::uvector<char> strtbl;

			// Offsets into strtbl.
			union {
				struct {
					strtbl_offset_t game_title;
					strtbl_offset_t artist;
					strtbl_offset_t copyright;
					strtbl_offset_t ripper;
				};
				strtbl_offset_t str[4];
			};
		};
		auth_t auth;

		// 'text' chunk
		string text;

		// String tables for the tracks struct.
		rp::uvector<char> track_name_strtbl;	// 'tlbl'
		rp::uvector<char> track_author_strtbl;	// 'taut'

		// Track struct
		// Contains information from the following chunks:
		// 'plst', 'psfx', 'time', 'fade', 'tlbl', 'taut'
		struct track_t {
			explicit track_t()
				: order(-1)
				, is_sfx(false)
				, duration(-1)
				, fade(-1)
				, name(-1)
				, author(-1)
			{}

			int16_t order;		// 'plst': playback order (-1 if not set)
			bool is_sfx;		// 'psfx': true if this is a "sound effect" instead of music
			int32_t duration;	// 'time': track duration, in milliseconds (negative for not specified)
			int32_t fade;		// 'fade': fadeout duration, in milliseconds (negative for not specified)

			strtbl_offset_t name;	// 'tlbl': Track name (label) [offset into track_name_strtbl]
			strtbl_offset_t author;	// 'taut': Track author [offset into track_author_strtbl]
		};
		vector<track_t> tracks;
	};
	nsfe_t nsfe;

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
	 * Decode a chunk of NULL-terminated strings into a vector of string offsets.
	 * strtbl_offset_t is int32_t; -1 indicates an invalid string.
	 * (Can't use 0 because 0 is the start of the chunk, and prepending a NUL
	 * would be a waste of time.)
	 *
	 * @param data Chunk data (must be NULL-terminated!)
	 * @param size Size of chunk data
	 * @return Vector of string offsets, or empty vector on error.
	 */
	static vector<strtbl_offset_t> decodeChunkOfStrings(const char *data, size_t size);

	/**
	 * Decode a chunk of NULL-terminated strings into a vector of string offsets.
	 * strtbl_offset_t is int32_t; -1 indicates an invalid string.
	 * (Can't use 0 because 0 is the start of the chunk, and prepending a NUL
	 * would be a waste of time.)
	 *
	 * @param vec Chunk data (must be NULL-terminated!)
	 * @return Vector of string offsets, or empty vector on error.
	 */
	static inline vector<strtbl_offset_t> decodeChunkOfStrings(const rp::uvector<char> &vec)
	{
		return decodeChunkOfStrings(vec.data(), vec.size());
	}

	/**
	 * Read the NSFe metadata.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int readNSFeMetadata(void);

	/**
	 * Format a duration as "m:ss.ddd".
	 * @param duration Duration
	 * @return Formatted duration ("<?>" if negative)
	 */
	static string formatDuration(int32_t duration);

	/**
	 * Get a C string pointer from a string table and offset.
	 * @param data String table data
	 * @param size String table size
	 * @param offset Offset
	 * @return C string pointer, or nullptr if invalid.
	 */
	static inline const char *getCStringPointerFromTable(const char *data, size_t size, strtbl_offset_t offset)
	{
		assert(offset <= static_cast<strtbl_offset_t>(CHUNK_SIZE_MAX));
		assert(offset < static_cast<strtbl_offset_t>(size));
		if (offset < 0 || static_cast<size_t>(offset) >= size) {
			return nullptr;
		}
		return &data[offset];
	}

	/**
	 * Get a C string pointer from a string table and offset.
	 * @param vec String table data
	 * @param offset Offset
	 * @return C string pointer, or nullptr if invalid.
	 */
	static inline const char *getCStringPointerFromTable(const rp::uvector<char> &vec, strtbl_offset_t offset)
	{
		return getCStringPointerFromTable(vec.data(), vec.size(), offset);
	}
};

ROMDATA_IMPL(NSF)
ROMDATA_IMPL_systemName(NSF, "Nintendo Sound Format", "NSF", "NSF")

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
 * Decode a chunk of NULL-terminated strings into a vector of string offsets.
 * strtbl_offset_t is int32_t; -1 indicates an invalid string.
 * (Can't use 0 because 0 is the start of the chunk, and prepending a NUL
 * would be a waste of time.)
 *
 * @param data Chunk data (must be NULL-terminated!)
 * @param size Size of chunk data
 * @return Vector of string offsets, or empty vector on error.
 */
vector<NSFPrivate::strtbl_offset_t> NSFPrivate::decodeChunkOfStrings(const char *data, size_t size)
{
	vector<strtbl_offset_t> v_ret;

	assert(data != nullptr);
	assert(size > 0);
	assert(size <= CHUNK_SIZE_MAX);
	if (!data || size == 0 || size > CHUNK_SIZE_MAX) {
		// Invalid parameters.
		return v_ret;
	}

	// Make sure the chunk is NULL-terminated.
	assert(data[size-1] == '\0');
	if (data[size-1] != '\0') {
		// Not NULL-terminated.
		return v_ret;
	}

	const char *p = data;
	const char *const p_end = data + size;
	for (; p < p_end; ) {
		size_t remain = p_end - p;
		const char *const p_nul = static_cast<const char*>(memchr(p, '\0', remain));
		assert(p_nul != nullptr);
		if (!p_nul) {
			// Shouldn't happen...
			break;
		}

		if ((p_nul - p) > 1) {
			// Push the starting offset of the string.
			v_ret.push_back(static_cast<strtbl_offset_t>(p - data));
		} else {
			// Empty string.
			v_ret.push_back(-1);
		}

		// Next string.
		p = p_nul + 1;
	}

	return v_ret;
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
	rp::uvector<char> buf;

	/**
	 * Read a data chunk into the temporary buffer.
	 * @param chunk_size	[in] Chunk size
	 * @param rdbuf		[out,opt] Alternate read buffer to use
	 * @return 0 on success; positive if the chunk should be skipped; negative POSIX error code on error.
	 */
	auto read_chunk = [&buf, this](unsigned int chunk_size, rp::uvector<char> *rdbuf = nullptr) -> int {
		// Read the chunk data into the temporary buffer.
		assert(chunk_size > 0);
		assert(chunk_size <= CHUNK_SIZE_MAX);
		if (chunk_size == 0) {
			// Empty chunk. Skip it.
			return 1;
		} else if (chunk_size > CHUNK_SIZE_MAX) {
			// Chunk is too big. Skip it.
			return 2;
		}

		if (!rdbuf) {
			rdbuf = &buf;
		}
		rdbuf->resize(chunk_size);
		size_t size = file->read(rdbuf->data(), rdbuf->size());
		if (size != rdbuf->size()) {
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
		// TODO: Don't allow duplicate chunks?
		bool done = false;
		switch (be32_to_cpu(chunkHeader.fourCC)) {
			default:
				// Skip unknown chunks.
				break;

			case 0:	// assuming all zeroes means an error
			case NSFe_Chunk_NEND:
				// End of file.
				done = true;
				break;

			/**
			 * Standard chunks that read in a fixed block of data.
			 * @param fourCC Chunk fourCC (no quotes)
			 * @param no_NSF2 If true, this chunk is not allowed in NSF2 files.
			 */
#define NSFe_STANDARD_CHUNK(fourCC, no_NSF2) \
			case NSFe_Chunk_##fourCC: \
				/* Standard chunk */ \
				if (no_NSF2) { \
					assert(!isNSF2); \
					if (isNSF2) { \
						/* Chunk is not allowed in NSF2 files. */ \
						break; \
					} \
				} \
\
				assert(chunk_size >= sizeof(nsfe.fourCC)); \
				if (chunk_size < sizeof(nsfe.fourCC)) { \
					/* Chunk size is incorrect. */ \
					break; \
				} \
\
				size = file->read(&nsfe.fourCC, sizeof(nsfe.fourCC)); \
				if (size != sizeof(nsfe.fourCC)) { \
					/* Read error. */ \
					int err = -file->lastError(); \
					if (err == 0) { \
						err = -EIO; \
					} \
					return err; \
				} \
				nsfe.has.fourCC = 1; \
				break
			
			NSFe_STANDARD_CHUNK(INFO, true);
			NSFe_STANDARD_CHUNK(BANK, true);
			NSFe_STANDARD_CHUNK(RATE, false);	// allowed in NSF2 for Dendy mode
			NSFe_STANDARD_CHUNK(NSF2, true);

			// 'VRC7' is allowed in NSF2 for patch data.
			// NOTE: We're not reading the patch data here.
			NSFe_STANDARD_CHUNK(VRC7, false);

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
				const uint8_t *p = reinterpret_cast<const uint8_t*>(buf.data());
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
				const uint8_t *p = reinterpret_cast<const uint8_t*>(buf.data());
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

			/**
			 * Chunks that contain a list of durations for the track list.
			 * @param fourCC Chunk fourCC (no quotes)
			 * @param field Field name within track_t
			 */
#define NSFe_DURATION_LIST_CHUNK(fourCC, field) \
			case NSFe_Chunk_##fourCC: { \
				/* Duration chunk */ \
				/* Chunk contains a list of 32-bit times (in milliseconds), */ \
				/* one for each track. (Note that it may be shorter than expected.) */ \
				assert(chunk_size >= sizeof(uint32_t)); \
				if (chunk_size < sizeof(uint32_t)) { \
					/* Chunk is empty. */ \
					break; \
				} \
\
				const int track_count = initNSFeTracks(); \
				if (track_count <= 0) { \
					/* No tracks... */ \
					break; \
				} \
\
				/* Read the chunk data into the temporary buffer. */ \
				int err = read_chunk(chunk_size); \
				if (err > 0) { \
					/* Skip the chunk. */ \
					break; \
				} else if (err < 0) { \
					/* Error reading the chunk... */ \
					return err; \
				} \
\
				/* Update the track list. */ \
				const uint32_t *p = reinterpret_cast<const uint32_t*>(buf.data()); \
				const uint32_t *const p_end = p + (buf.size() / sizeof(uint32_t)); \
				for (int i = 0; i < track_count && p < p_end; i++, p++) { \
					nsfe.tracks[i].field = le32_to_cpu(*p); \
				} \
				nsfe.has.fourCC = 1; \
				nsfe.has.any_tlist = 1; \
				break; \
			}

			NSFe_DURATION_LIST_CHUNK(time, duration)
			NSFe_DURATION_LIST_CHUNK(fade, fade)

			/**
			 * Chunks that contain a list of strings for the track list.
			 * @param fourCC Chunk fourCC (no quotes)
			 * @param field Field name within track_t
			 */
#define NSFe_STRING_LIST_CHUNK(fourCC, field) \
			case NSFe_Chunk_##fourCC: { \
				/* 'tlbl' chunk */ \
				assert(chunk_size > 0); \
				if (chunk_size == 0) { \
					/* Chunk is empty. */ \
					break; \
				} \
\
				int track_count = initNSFeTracks(); \
				if (track_count <= 0) { \
					/* No tracks... */ \
					break; \
				} \
\
				/* Read the chunk data into track_##field##_strtbl. */ \
				int err = read_chunk(chunk_size, &nsfe.track_##field##_strtbl); \
				if (err > 0) { \
					/* Skip the chunk. */ \
					break; \
				} else if (err < 0) { \
					/* Error reading the chunk... */ \
					nsfe.track_##field##_strtbl.clear(); \
					return err; \
				} \
\
				/* Make sure the chunk is NULL-terminated. */ \
				if (nsfe.track_##field##_strtbl.back() != '\0') { \
					nsfe.track_##field##_strtbl.push_back('\0'); \
				} \
\
				/* Should contain track_count strings. */ \
				/* We'll allow up to track_count, ignoring any additional strings. */ \
				/* TODO: Add a maximum string count parameter to decodeChunkOfStrings()? */ \
				vector<strtbl_offset_t> v_str = decodeChunkOfStrings(nsfe.track_##field##_strtbl); \
				const int v_str_size = static_cast<int>(v_str.size()); \
				for (int i = 0; i < track_count && i < v_str_size; i++) { \
					nsfe.tracks[i].field = v_str[i]; \
				} \
				nsfe.has.fourCC = 1; \
				nsfe.has.any_tlist = 1; \
				break; \
			}

			NSFe_STRING_LIST_CHUNK(tlbl, name)
			NSFe_STRING_LIST_CHUNK(taut, author)

			case NSFe_Chunk_auth: {
				// 'auth' chunk
				// Read the chunk data into nsfe.auth.strtbl.
				assert(chunk_size > 0);
				assert(chunk_size <= CHUNK_SIZE_MAX);
				if (chunk_size == 0 || chunk_size > CHUNK_SIZE_MAX) {
					// Empty chunk, or too big...
					break;
				}

				// Read the chunk data into nsfe.auth.strtbl.
				int err = read_chunk(chunk_size, &nsfe.auth.strtbl);
				if (err > 0) {
					// Skip the chunk.
					break; \
				} else if (err < 0) {
					// Error reading the chunk...
					nsfe.auth.strtbl.clear();
					return err;
				}

				// Make sure the chunk is NULL-terminated.
				if (nsfe.auth.strtbl.back() != '\0') {
					nsfe.auth.strtbl.push_back('\0');
				}

				// Should contain 4 strings.
				// We'll allow up to 4, ignoring any additional strings.
				// TODO: Add a maximum string count parameter to decodeChunkOfStrings()?
				vector<strtbl_offset_t> v_str = decodeChunkOfStrings(nsfe.auth.strtbl);
				const size_t v_str_size = static_cast<int>(v_str.size()); \
				// TODO: memcpy() or std::copy()?
				for (size_t i = 0; i < ARRAY_SIZE(nsfe.auth.str) && i < v_str_size; i++) {
					nsfe.auth.str[i] = v_str[i];
				}
				nsfe.has.auth = 1;
				break;
			}

			case NSFe_Chunk_text: {
				// 'text' chunk
				// Read the chunk data into nsfe.text.
				assert(chunk_size > 0);
				assert(chunk_size <= CHUNK_SIZE_MAX);
				if (chunk_size == 0 || chunk_size > CHUNK_SIZE_MAX) {
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
		}

		if (done) {
			break;
		}

		// Add the chunk header and chunk size to the seek position.
		pos += sizeof(chunkHeader) + chunk_size;
	}

	return 0;
}

/**
 * Format a duration as "m:ss.ddd".
 * @param duration Duration
 * @return Formatted duration ("<?>" if negative)
 */
string NSFPrivate::formatDuration(int32_t duration)
{
	string s_ret;

	if (duration < 0) {
		// Duration not set.
		// NOTE: Assigning to `s_ret` for named-return-value optimization.
		s_ret.assign("<?>", 3);
		return s_ret;
	}

	const uint32_t u_duration = static_cast<uint32_t>(duration);
	const unsigned int min = (u_duration / 1000) / 60;
	const unsigned int sec = (u_duration / 1000) % 60;
	const unsigned int ms =  (u_duration % 1000);
	fmt::format_to(std::back_inserter(s_ret), FSTR("{:d}:{:0>2d}.{:0>3d}"), min, sec, ms);
	return s_ret;
};

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

	static const array<const char*, 4> nsf2_feature_flags_names = {{
		"IRQ support", "Non-returning INIT",
		"No PLAY subroutine", "NSFe has mandatory chunk",
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
			const char *const game_title = d->getCStringPointerFromTable(nsfe->auth.strtbl, nsfe->auth.game_title);
			if (game_title) {
				d->fields.addField_string(C_("RomData|Audio", "Title"), game_title);
			}

			// Artist
			const char *const artist = d->getCStringPointerFromTable(nsfe->auth.strtbl, nsfe->auth.artist);
			if (artist) {
				d->fields.addField_string(C_("RomData|Audio", "Composer"), artist);
			}

			// Copyright
			const char *const copyright = d->getCStringPointerFromTable(nsfe->auth.strtbl, nsfe->auth.copyright);
			if (copyright) {
				d->fields.addField_string(C_("RomData|Audio", "Copyright"), copyright);
			}

			// Ripper
			const char *const ripper = d->getCStringPointerFromTable(nsfe->auth.strtbl, nsfe->auth.ripper);
			if (ripper) {
				d->fields.addField_string(C_("RomData|Audio", "Ripper"), ripper);
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

		if (nsfe->has.NSF2) {
			// NSF2 feature flags
			// Bits 0-3 are reserved. Adjust the flags to match the names.
			const uint32_t adjflags = (nsfe->NSF2.nsf2_feature_flags) >> 4;
			vector<string> *const v_nsf2_feature_flags_names = RomFields::strArrayToVector(nsf2_feature_flags_names);
			d->fields.addField_bitfield(C_("NSF", "NSF2 Feature Flags"),
				v_nsf2_feature_flags_names, 2, adjflags);
		}

		if (nsfe->has.text && likely(!nsfe->text.empty())) {
			// 'text' chunk is available.
			d->fields.addField_string(C_("RomData", "Description"), nsfe->text);
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
					const char *const name = d->getCStringPointerFromTable(nsfe->track_name_strtbl, src_iter->name);
					if (name) {
						data_row.push_back(name);
					} else {
						data_row.push_back(string());
					}
				}

				if (nsfe->has.taut) {
					const char *const author = d->getCStringPointerFromTable(nsfe->track_author_strtbl, src_iter->author);
					if (author) {
						data_row.push_back(author);
					} else {
						data_row.push_back(string());
					}
				}

				if (nsfe->has.time) {
					data_row.push_back(d->formatDuration(src_iter->duration));
				}
				if (nsfe->has.fade) {
					data_row.push_back(d->formatDuration(src_iter->fade));
				}
			}

			vector<const char*> track_list_headers;
			track_list_headers.reserve(7);

			uint16_t align_data = TXA_R;
			uint16_t sizing = COLSZ_R;
			uint16_t sorting = COLSORT_NUM;
			track_list_headers.push_back(C_("NSF|TrackList", "#"));

			unsigned int shift = 2;	// for alignment/sorting
			if (nsfe->has.plst) {
				track_list_headers.push_back(C_("NSF|TrackList", "Order"));
				align_data |= (TXA_R << shift);
				sizing |= (COLSZ_R << shift);
				sorting |= (COLSORT_NUM << shift);
				shift += 2;
			}
			if (nsfe->has.psfx) {
				track_list_headers.push_back(C_("NSF|TrackList", "Is SFX"));
				align_data |= (TXA_R << shift);
				sizing |= (COLSZ_R << shift);
				sorting |= (COLSORT_STD << shift);
				shift += 2;
			}
			if (nsfe->has.tlbl) {
				track_list_headers.push_back(C_("NSF|TrackList", "Name"));
				align_data |= (TXA_D << shift);
				sizing |= (COLSZ_S << shift);
				sorting |= (COLSORT_NC << shift);
				shift += 2;
			}
			if (nsfe->has.taut) {
				track_list_headers.push_back(C_("NSF|TrackList", "Author"));
				align_data |= (TXA_D << shift);
				sizing |= (COLSZ_S << shift);
				sorting |= (COLSORT_NC << shift);
				shift += 2;
			}
			if (nsfe->has.time) {
				track_list_headers.push_back(C_("NSF|TrackList", "Duration"));
				align_data |= (TXA_R << shift);
				sizing |= (COLSZ_R << shift);
				sorting |= (COLSORT_NUM << shift);
				shift += 2;
			}
			if (nsfe->has.fade) {
				track_list_headers.push_back(C_("NSF|TrackList", "Fade"));
				align_data |= (TXA_R << shift);
				sizing |= (COLSZ_R << shift);
				sorting |= (COLSORT_NUM << shift);
				shift += 2;
			}
			vector<string> *const v_track_list_headers = RomFields::strArrayToVector(track_list_headers);

			RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_SEPARATE_ROW, 8);
			params.headers = v_track_list_headers;
			params.data.single = vv_track_list;
			params.col_attrs.align_headers = AFLD_ALIGN7(TXA_D, TXA_D, TXA_D, TXA_D, TXA_D, TXA_D, TXA_D);
			params.col_attrs.align_data = align_data;
			params.col_attrs.sizing = sizing;
			params.col_attrs.sorting = sorting;
			params.col_attrs.sort_col = 0; // #
			params.col_attrs.sort_dir = RomFields::COLSORTORDER_ASCENDING;
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

	if (d->nsfe.has.auth) {
		// 'auth' chunk is available.
		// NOTE: May overwrite NSF2 metadata if it's already present.
		const auto *nsfe = &d->nsfe;

		// Title
		const char *const game_title = d->getCStringPointerFromTable(nsfe->auth.strtbl, nsfe->auth.game_title);
		if (game_title) {
			d->metaData.addMetaData_string(Property::Title, game_title);
		}

		// Artist
		const char *const artist = d->getCStringPointerFromTable(nsfe->auth.strtbl, nsfe->auth.artist);
		if (artist) {
			d->metaData.addMetaData_string(Property::Composer, artist);
		}

		// Copyright
		const char *const copyright = d->getCStringPointerFromTable(nsfe->auth.strtbl, nsfe->auth.copyright);
		if (copyright) {
			d->metaData.addMetaData_string(Property::Copyright, copyright);
		}
	}

	// Finished reading the metadata.
	return d->metaData.count();
}

} // namespace LibRomData
