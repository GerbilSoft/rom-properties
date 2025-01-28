/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * FileFormatFactory.cpp: FileFormat factory class.                        *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"

#include "FileFormatFactory.hpp"
#include "fileformat/FileFormat.hpp"
#include "fileformat/FileFormat_p.hpp"	// for TextureInfo

// librpbase, librpfile
#include "librpfile/FileSystem.hpp"
using namespace LibRpBase;
using namespace LibRpFile;

// librpthreads
#include "librpthreads/pthread_once.h"

// C++ STL classes
using std::array;
using std::string;
using std::unordered_set;
using std::vector;

// FileFormat subclasses.
#include "fileformat/ASTC.hpp"
#include "fileformat/DidjTex.hpp"
#include "fileformat/DirectDrawSurface.hpp"
#include "fileformat/GodotSTEX.hpp"
#include "fileformat/KhronosKTX.hpp"
#include "fileformat/KhronosKTX2.hpp"
#include "fileformat/PowerVR3.hpp"
#include "fileformat/SegaPVR.hpp"
#include "fileformat/TGA.hpp"
#include "fileformat/ValveVTF.hpp"
#include "fileformat/ValveVTF3.hpp"
#include "fileformat/XboxXPR.hpp"

// TGA structs.
#include "fileformat/tga_structs.h"

namespace LibRpTexture { namespace FileFormatFactory {

namespace Private {

/** FileFormat subclass check arrays **/

// For FileFormat, we assume that the magic number appears
// at the beginning of the file for all formats.
// FIXME: TGA format doesn't follow this...
//typedef int (*pfnIsTextureSupported_t)(const FileFormat::DetectInfo *info);	// TODO
typedef const TextureInfo* (*pfnTextureInfo_t)(void);
typedef FileFormatPtr (*pfnNewFileFormat_t)(const IRpFilePtr &file);

struct FileFormatFns {
	//pfnIsTextureSupported_t isTextureSupported;	// TODO
	pfnNewFileFormat_t newFileFormat;
	pfnTextureInfo_t textureInfo;

	std::array<uint32_t, 2> magic;
};

/**
 * Templated function to construct a new FileFormat subclass.
 * @param klass Class name.
 */
template<typename klass>
static FileFormatPtr FileFormat_ctor(const IRpFilePtr &file)
{
	return std::make_shared<klass>(file);
}

#define GetFileFormatFns(format, magic) \
	{/*format::isRomSupported_static,*/ /* TODO */ \
	 FileFormat_ctor<format>, \
	 format::textureInfo, \
	 magic}
#define P99_PROTECT(...) __VA_ARGS__	/* Reference: https://stackoverflow.com/a/5504336 */

#ifdef FILEFORMATFACTORY_USE_FILE_EXTENSIONS
vector<const char*> vec_exts;
pthread_once_t once_exts = PTHREAD_ONCE_INIT;
#endif /* FILEFORMATFACTORY_USE_FILE_EXTENSIONS */
#ifdef FILEFORMATFACTORY_USE_MIME_TYPES
vector<const char*> vec_mimeTypes;
pthread_once_t once_mimeTypes = PTHREAD_ONCE_INIT;
#endif /* FILEFORMATFACTORY_USE_MIME_TYPES */

// FileFormat subclasses that use a header at 0 and
// definitely have a 32-bit magic number at address 0.
// TODO: Add support for multiple magic numbers per class.
const array<FileFormatFns, 15> FileFormatFns_magic = {{
	GetFileFormatFns(ASTC,			P99_PROTECT({{0x13ABA15C, 0}})),	// Needs to be in multi-char constant format
	GetFileFormatFns(DirectDrawSurface,	P99_PROTECT({{'DDS ', 0}})),
	GetFileFormatFns(GodotSTEX,		P99_PROTECT({{'GDST', 'GST2'}})),
	GetFileFormatFns(PowerVR3,		P99_PROTECT({{'PVR\x03', '\x03RVP'}})),
	GetFileFormatFns(SegaPVR,		P99_PROTECT({{'PVRT', 'GVRT'}})),
	GetFileFormatFns(SegaPVR,		P99_PROTECT({{'PVRX', 'GBIX'}})),
	GetFileFormatFns(SegaPVR,		P99_PROTECT({{'GCIX', 0}})),
	GetFileFormatFns(ValveVTF,		P99_PROTECT({{'VTF\0', 0}})),
	GetFileFormatFns(ValveVTF3,		P99_PROTECT({{'VTF3', 0}})),
	GetFileFormatFns(XboxXPR,		P99_PROTECT({{'XPR0', 0}})),

	// Less common formats
	GetFileFormatFns(DidjTex,		P99_PROTECT({{(uint32_t)0x03000000U, 0}})),
}};

// FileFormat subclasses that have special checks.
// This array is for file extensions and MIME types only.
const array<FileFormatFns, 3> FileFormatFns_mime = {{
	GetFileFormatFns(KhronosKTX,		P99_PROTECT({{0, 0}})),
	GetFileFormatFns(KhronosKTX2,		P99_PROTECT({{0, 0}})),
	GetFileFormatFns(TGA,			P99_PROTECT({{0, 0}})),
}};

} // namespace Private

/** FileFormatFactory **/

/**
 * Create a FileFormat subclass for the specified texture file.
 *
 * NOTE: FileFormat::isValid() is checked before returning a
 * created FileFormat instance, so returned objects can be
 * assumed to be valid as long as they aren't nullptr.
 *
 * If imgbf is non-zero, at least one of the specified image
 * types must be supported by the FileFormat subclass in order to
 * be returned.
 *
 * @param file Texture file
 * @return FileFormat subclass, or nullptr if the texture file isn't supported.
 */
FileFormatPtr create(const IRpFilePtr &file)
{
	assert(file != nullptr);
	if (!file || file->isDevice()) {
		// Either no file was specified, or this is
		// a device. No one would realistically use
		// a whole device to store one texture...
		return {};
	}

	// Read the file's magic number.
	union {
		uint8_t u8[64];
		uint32_t u32[64/4];
	} magic;
	file->rewind();
	size_t size = file->read(&magic, sizeof(magic));
	if (size != sizeof(magic)) {
		// Read error.
		return {};
	}

	// Special check for Khronos KTX, which has the same
	// 32-bit magic number for two completely different versions.
	if (magic.u32[0] == cpu_to_be32('\xABKTX')) {
		FileFormatPtr fileFormat;
		if (magic.u32[1] == cpu_to_be32(' 11\xBB')) {
			// KTX 1.1
			fileFormat = std::make_shared<KhronosKTX>(file);
		} else if (magic.u32[1] == cpu_to_be32(' 20\xBB')) {
			// KTX 2.0
			fileFormat = std::make_shared<KhronosKTX2>(file);
		}

		if (fileFormat && fileFormat->isValid()) {
			// FileFormat subclass obtained.
			return FileFormatPtr(fileFormat);
		}
	}

	// Use some heuristics to check for TGA files.
	// Based on heuristics from `file`.
	// TGA 2.0 has an identifying footer as well.
	// NOTE: We're also checking the file extension due to
	// conflicts with "WWF Raw" on SNES.
	const char *const filename = file->filename();
	const char *const ext = FileSystem::file_ext(filename);
	bool ext_ok = false;
	if (!ext || ext[0] == '\0') {
		// No extension. Check for TGA anyway.
		ext_ok = true;
	} else if (!strcasecmp(ext, ".tga")) {
		// TGA extension.
		ext_ok = true;
	} else if (!strcasecmp(ext, ".gz")) {
		// Check if it's ".tga.gz".
		const size_t filename_len = strlen(filename);
		if (filename_len >= 7) {
			if (!strncasecmp(&filename[filename_len-7], ".tga", 4)) {
				// It's ".tga.gz".
				ext_ok = true;
			}
		}
	}

	// test of Color Map Type 0~no 1~color map
	// and Image Type 1 2 3 9 10 11 32 33
	// and Color Map Entry Size 0 15 16 24 32
	if (ext_ok &&
	    ((magic.u32[0] & be32_to_cpu(0x00FEC400)) == 0) &&
	    ((magic.u32[1] & be32_to_cpu(0x000000C0)) == 0))
	{
		const TGA_Header *const tgaHeader = reinterpret_cast<const TGA_Header*>(&magic);

		// skip some MPEG sequence *.vob and some CRI ADX audio with improbable interleave bits
		if ((tgaHeader->img.attr_dir & 0xC0) != 0xC0 &&
		// skip more garbage like *.iso by looking for positive image type
		     tgaHeader->image_type > 0 &&
		// skip some compiled terminfo like xterm+tmux by looking for image type less equal 33
		     tgaHeader->image_type < 34 &&
		// skip some MPEG sequence *.vob HV001T01.EVO winnicki.mpg with unacceptable alpha channel depth 11
		    (tgaHeader->img.attr_dir & 0x0F) != 11)
		{
			// skip arches.3200 , Finder.Root , Slp.1 by looking for low pixel depth 1 8 15 16 24 32
			switch (tgaHeader->img.bpp) {
				case 1:  case 8:
				case 15: case 16:
				case 24: case 32: {
					// Valid color depth.
					// This might be TGA.
					FileFormatPtr fileFormat = std::make_shared<TGA>(file);
					if (fileFormat->isValid()) {
						// FileFormat subclass obtained.
						return fileFormat;
					}

					// Not actually supported.
					break;
				}

				default:
					break;
			}
		}
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Magic number needs to be in host-endian.
	magic.u32[0] = be32_to_cpu(magic.u32[0]);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// Check FileFormat subclasses that take a header at 0
	// and definitely have a 32-bit magic number at address 0.
	for (const auto &fns : Private::FileFormatFns_magic) {
		// Check the magic number(s).
		static_assert(Private::FileFormatFns_magic[0].magic.size() == 2, "need to update for more than 2 magic numbers");
		if (magic.u32[0] == fns.magic[0] ||
		    (fns.magic[1] != 0 && magic.u32[0] == fns.magic[1]))
		{
			// Found a matching magic number.
			// TODO: Implement fns->isTextureSupported.
			/*if (fns->isTextureSupported(&info) >= 0)*/ {
				FileFormatPtr fileFormat = fns.newFileFormat(file);
				if (fileFormat->isValid()) {
					// FileFormat subclass obtained.
					return FileFormatPtr(fileFormat);
				}
			}
		}
	}

	// Special case: Check for PowerVR v2.
	if (magic.u32[0x2C/4] == 0x21525650U || magic.u32[0x2C/4] == 0x50565221U) {
		// Found a matching magic number.
		FileFormatPtr fileFormat = std::make_shared<PowerVR3>(file);
		if (fileFormat->isValid()) {
			// FileFormat subclass obtained.
			return FileFormatPtr(fileFormat);
		}
	}

	// Not supported.
	return {};
}

#ifdef FILEFORMATFACTORY_USE_FILE_EXTENSIONS
namespace Private {

/**
 * Initialize the vector of supported file extensions.
 * Used for Win32 COM registration.
 *
 * Internal function; must be called using pthread_once().
 *
 * NOTE: The return value is a struct that includes a flag
 * indicating if the file type handler supports thumbnails.
 */
static void init_supportedFileExtensions(void)
{
	// In order to handle multiple FileFormat subclasses
	// that support the same extensions, we're using
	// an unordered_set<string>.
	//
	// The actual data is stored in the vector<const char*>.
	unordered_set<string> set_exts;

	static constexpr size_t reserve_size =
		(FileFormatFns_magic.size() +
		 FileFormatFns_mime.size()) * 2;
	vec_exts.reserve(reserve_size);
#ifdef HAVE_UNORDERED_SET_RESERVE
	set_exts.reserve(reserve_size);
#endif /* HAVE_UNORDERED_SET_RESERVE */

	for (const auto &fns : FileFormatFns_magic) {
		const char *const *sys_exts = fns.textureInfo()->exts;
		if (!sys_exts)
			continue;

		for (; *sys_exts != nullptr; sys_exts++) {
			auto iter = set_exts.find(*sys_exts);
			if (iter == set_exts.end()) {
				set_exts.emplace(*sys_exts);
				vec_exts.push_back(*sys_exts);
			}
		}
	}

	// Also handle FileFormat subclasses that have custom magic checks.
	for (const auto &fns : FileFormatFns_mime) {
		const char *const *sys_exts = fns.textureInfo()->exts;
		if (!sys_exts)
			continue;

		for (; *sys_exts != nullptr; sys_exts++) {
			auto iter = set_exts.find(*sys_exts);
			if (iter == set_exts.end()) {
				set_exts.emplace(*sys_exts);
				vec_exts.push_back(*sys_exts);
			}
		}
	}
}

} // namespace Private

/**
 * Get all supported file extensions.
 * Used for Win32 COM registration.
 *
 * @return All supported file extensions, including the leading dot.
 */
const vector<const char*> &supportedFileExtensions(void)
{
	pthread_once(&Private::once_exts, Private::init_supportedFileExtensions);
	return Private::vec_exts;
}
#endif /* FILEFORMATFACTORY_USE_FILE_EXTENSIONS */

#ifdef FILEFORMATFACTORY_USE_MIME_TYPES
namespace Private {

/**
 * Initialize the vector of supported MIME types.
 * Used for KFileMetaData.
 *
 * Internal function; must be called using pthread_once().
 */
static void init_supportedMimeTypes(void)
{
	// TODO: Add generic types, e.g. application/octet-stream?

	// In order to handle multiple FileFormat subclasses
	// that support the same MIME types, we're using
	// an unordered_set<string>. The actual data
	// is stored in the vector<const char*>.
	vector<const char*> vec_mimeTypes;
	unordered_set<string> set_mimeTypes;

	static constexpr size_t reserve_size =
		(Private::FileFormatFns_magic.size() +
		 Private::FileFormatFns_mime.size()) * 2;
	vec_mimeTypes.reserve(reserve_size);
#ifdef HAVE_UNORDERED_SET_RESERVE
	set_mimeTypes.reserve(reserve_size);
#endif /* HAVE_UNORDERED_SET_RESERVE */

	for (const auto &fns : FileFormatFns_magic) {
		const char *const *sys_mimeTypes = fns.textureInfo()->mimeTypes;
		if (!sys_mimeTypes)
			continue;

		for (; *sys_mimeTypes != nullptr; sys_mimeTypes++) {
			auto iter = set_mimeTypes.find(*sys_mimeTypes);
			if (iter == set_mimeTypes.end()) {
				set_mimeTypes.emplace(*sys_mimeTypes);
				vec_mimeTypes.push_back(*sys_mimeTypes);
			}
		}
	}

	// Also handle FileFormat subclasses that have custom magic checks.
	for (const auto &fns : FileFormatFns_mime) {
		const char *const *sys_mimeTypes = fns.textureInfo()->mimeTypes;
		if (!sys_mimeTypes)
			continue;

		for (; *sys_mimeTypes != nullptr; sys_mimeTypes++) {
			auto iter = set_mimeTypes.find(*sys_mimeTypes);
			if (iter == set_mimeTypes.end()) {
				set_mimeTypes.emplace(*sys_mimeTypes);
				vec_mimeTypes.push_back(*sys_mimeTypes);
			}
		}
	}
}

} // namespace Private

/**
 * Get all supported MIME types.
 * Used for KFileMetaData.
 *
 * @return All supported MIME types.
 */
vector<const char*> supportedMimeTypes(void)
{
	pthread_once(&Private::once_mimeTypes, Private::init_supportedMimeTypes);
	return Private::vec_mimeTypes;
}
#endif /* FILEFORMATFACTORY_USE_MIME_TYPES */

} }
