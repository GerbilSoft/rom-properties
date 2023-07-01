/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * FileFormatFactory.cpp: FileFormat factory class.                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
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

// C++ STL classes.
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

namespace LibRpTexture {

class FileFormatFactoryPrivate
{
	public:
		// Static class
		FileFormatFactoryPrivate() = delete;
		~FileFormatFactoryPrivate() = delete;
	private:
		RP_DISABLE_COPY(FileFormatFactoryPrivate)

	public:
		// For FileFormat, we assume that the magic number appears
		// at the beginning of the file for all formats.
		// FIXME: TGA format doesn't follow this...
		//typedef int (*pfnIsTextureSupported_t)(const FileFormat::DetectInfo *info);	// TODO
		typedef const TextureInfo* (*pfnTextureInfo_t)(void);
		typedef FileFormat* (*pfnNewFileFormat_t)(IRpFile *file);

		struct FileFormatFns {
			//pfnIsTextureSupported_t isTextureSupported;	// TODO
			pfnNewFileFormat_t newFileFormat;
			pfnTextureInfo_t textureInfo;

			uint32_t magic;
		};

		/**
		 * Templated function to construct a new FileFormat subclass.
		 * @param klass Class name.
		 */
		template<typename klass>
		static FileFormat *FileFormat_ctor(LibRpFile::IRpFile *file)
		{
			return new klass(file);
		}

#define GetFileFormatFns(format, magic) \
	{/*format::isRomSupported_static,*/ /* TODO */ \
	 FileFormatFactoryPrivate::FileFormat_ctor<format>, \
	 format::textureInfo, \
	 (magic)}

		// FileFormat subclasses that use a header at 0 and
		// definitely have a 32-bit magic number at address 0.
		static const FileFormatFns FileFormatFns_magic[];

		// FileFormat subclasses that have special checks.
		// This array is for file extensions and MIME types only.
		static const FileFormatFns FileFormatFns_mime[];

		// Vectors for file extensions.
		// We want to collect them once per session instead of
		// repeatedly collecting them, since the caller might
		// not cache them.
		static vector<const char*> vec_exts;
		// pthread_once() control variables
		static pthread_once_t once_exts;

		/**
		 * Initialize the vector of supported file extensions.
		 * Used for Win32 COM registration.
		 *
		 * Internal function; must be called using pthread_once().
		 *
		 * NOTE: The return value is a struct that includes a flag
		 * indicating if the file type handler supports thumbnails.
		 */
		static void init_supportedFileExtensions(void);
};

/** FileFormatFactoryPrivate **/

vector<const char*> FileFormatFactoryPrivate::vec_exts;
// pthread_once() control variables
pthread_once_t FileFormatFactoryPrivate::once_exts = PTHREAD_ONCE_INIT;

// FileFormat subclasses that use a header at 0 and
// definitely have a 32-bit magic number at address 0.
// TODO: Add support for multiple magic numbers per class.
const FileFormatFactoryPrivate::FileFormatFns FileFormatFactoryPrivate::FileFormatFns_magic[] = {
	GetFileFormatFns(ASTC, 0x13ABA15C),	// Needs to be in multi-char constant format.
	GetFileFormatFns(DirectDrawSurface, 'DDS '),
	GetFileFormatFns(GodotSTEX, 'GDST'),
	GetFileFormatFns(GodotSTEX, 'GST2'),
	GetFileFormatFns(PowerVR3, 'PVR\x03'),
	GetFileFormatFns(PowerVR3, '\x03RVP'),
	GetFileFormatFns(SegaPVR, 'PVRT'),
	GetFileFormatFns(SegaPVR, 'GVRT'),
	GetFileFormatFns(SegaPVR, 'PVRX'),
	GetFileFormatFns(SegaPVR, 'GBIX'),
	GetFileFormatFns(SegaPVR, 'GCIX'),
	GetFileFormatFns(ValveVTF, 'VTF\0'),
	GetFileFormatFns(ValveVTF3, 'VTF3'),
	GetFileFormatFns(XboxXPR, 'XPR0'),

	// Less common formats.
	GetFileFormatFns(DidjTex, (uint32_t)0x03000000),

	{nullptr, nullptr, 0}
};

// FileFormat subclasses that have special checks.
// This array is for file extensions and MIME types only.
const FileFormatFactoryPrivate::FileFormatFns FileFormatFactoryPrivate::FileFormatFns_mime[] = {
	GetFileFormatFns(KhronosKTX, 0),
	GetFileFormatFns(KhronosKTX2, 0),
	GetFileFormatFns(TGA, 0),

	{nullptr, nullptr, 0}
};

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
 * @param file Texture file.
 * @return FileFormat subclass, or nullptr if the texture file isn't supported.
 */
FileFormat *FileFormatFactory::create(IRpFile *file)
{
	assert(file != nullptr);
	if (!file || file->isDevice()) {
		// Either no file was specified, or this is
		// a device. No one would realistically use
		// a whole device to store one texture...
		return nullptr;
	}

	// Read the file's magic number.
	union {
		uint8_t u8[32];
		uint32_t u32[32/4];
	} magic;
	file->rewind();
	size_t size = file->read(&magic, sizeof(magic));
	if (size != sizeof(magic)) {
		// Read error.
		return nullptr;
	}

	// Special check for Khronos KTX, which has the same
	// 32-bit magic number for two completely different versions.
	if (magic.u32[0] == cpu_to_be32('\xABKTX')) {
		FileFormat *fileFormat = nullptr;
		if (magic.u32[1] == cpu_to_be32(' 11\xBB')) {
			// KTX 1.1
			fileFormat = new KhronosKTX(file);
		} else if (magic.u32[1] == cpu_to_be32(' 20\xBB')) {
			// KTX 2.0
			fileFormat = new KhronosKTX2(file);
		}

		if (fileFormat) {
			if (fileFormat->isValid()) {
				// FileFormat subclass obtained.
				return fileFormat;
			}

			// Not actually supported.
			fileFormat->unref();
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
					FileFormat *const fileFormat = new TGA(file);
					if (fileFormat->isValid()) {
						// FileFormat subclass obtained.
						return fileFormat;
					}
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
	const FileFormatFactoryPrivate::FileFormatFns *fns =
		&FileFormatFactoryPrivate::FileFormatFns_magic[0];
	for (; fns->textureInfo != nullptr; fns++) {
		// Check the magic number.
		if (magic.u32[0] == fns->magic) {
			// Found a matching magic number.
			// TODO: Implement fns->isTextureSupported.
			/*if (fns->isTextureSupported(&info) >= 0)*/ {
				FileFormat *const fileFormat = fns->newFileFormat(file);
				if (fileFormat->isValid()) {
					// FileFormat subclass obtained.
					return fileFormat;
				}

				// Not actually supported.
				fileFormat->unref();
			}
		}
	}

	// Not supported.
	return nullptr;
}

/**
 * Initialize the vector of supported file extensions.
 * Used for Win32 COM registration.
 *
 * Internal function; must be called using pthread_once().
 *
 * NOTE: The return value is a struct that includes a flag
 * indicating if the file type handler supports thumbnails.
 */
void FileFormatFactoryPrivate::init_supportedFileExtensions(void)
{
	// In order to handle multiple FileFormat subclasses
	// that support the same extensions, we're using
	// an unordered_set<string>.
	//
	// The actual data is stored in the vector<const char*>.
	unordered_set<string> set_exts;

	static const size_t reserve_size = ARRAY_SIZE(FileFormatFactoryPrivate::FileFormatFns_magic);
	vec_exts.reserve(reserve_size);
#ifdef HAVE_UNORDERED_SET_RESERVE
	set_exts.reserve(reserve_size);
#endif /* HAVE_UNORDERED_SET_RESERVE */

	const FileFormatFactoryPrivate::FileFormatFns *fns =
		&FileFormatFactoryPrivate::FileFormatFns_magic[0];
	for (; fns->textureInfo != nullptr; fns++) {
		const char *const *sys_exts = fns->textureInfo()->exts;
		if (!sys_exts)
			continue;

		for (; *sys_exts != nullptr; sys_exts++) {
			auto iter = set_exts.find(*sys_exts);
			if (iter == set_exts.end()) {
				set_exts.insert(*sys_exts);
				vec_exts.emplace_back(*sys_exts);
			}
		}
	}

	// Also handle FileFormat subclasses that have custom magic checks.
	fns = &FileFormatFactoryPrivate::FileFormatFns_mime[0];
	for (; fns->textureInfo != nullptr; fns++) {
		const char *const *sys_exts = fns->textureInfo()->exts;
		if (!sys_exts)
			continue;

		for (; *sys_exts != nullptr; sys_exts++) {
			auto iter = set_exts.find(*sys_exts);
			if (iter == set_exts.end()) {
				set_exts.insert(*sys_exts);
				vec_exts.emplace_back(*sys_exts);
			}
		}
	}
}

/**
 * Get all supported file extensions.
 * Used for Win32 COM registration.
 *
 * @return All supported file extensions, including the leading dot.
 */
const vector<const char*> &FileFormatFactory::supportedFileExtensions(void)
{
	pthread_once(&FileFormatFactoryPrivate::once_exts, FileFormatFactoryPrivate::init_supportedFileExtensions);
	return FileFormatFactoryPrivate::vec_exts;
}

/**
 * Get all supported MIME types.
 * Used for KFileMetaData.
 *
 * @return All supported MIME types.
 */
vector<const char*> FileFormatFactory::supportedMimeTypes(void)
{
	// TODO: Add generic types, e.g. application/octet-stream?

	// In order to handle multiple FileFormat subclasses
	// that support the same MIME types, we're using
	// an unordered_set<string>. The actual data
	// is stored in the vector<const char*>.
	vector<const char*> vec_mimeTypes;
	unordered_set<string> set_mimeTypes;

	static const size_t reserve_size = ARRAY_SIZE(FileFormatFactoryPrivate::FileFormatFns_magic) * 2;
	vec_mimeTypes.reserve(reserve_size);
#ifdef HAVE_UNORDERED_SET_RESERVE
	set_mimeTypes.reserve(reserve_size);
#endif /* HAVE_UNORDERED_SET_RESERVE */

	const FileFormatFactoryPrivate::FileFormatFns *fns =
		&FileFormatFactoryPrivate::FileFormatFns_magic[0];
	for (; fns->textureInfo != nullptr; fns++) {
		const char *const *sys_mimeTypes = fns->textureInfo()->mimeTypes;
		if (!sys_mimeTypes)
			continue;

		for (; *sys_mimeTypes != nullptr; sys_mimeTypes++) {
			auto iter = set_mimeTypes.find(*sys_mimeTypes);
			if (iter == set_mimeTypes.end()) {
				set_mimeTypes.insert(*sys_mimeTypes);
				vec_mimeTypes.emplace_back(*sys_mimeTypes);
			}
		}
	}

	// Also handle FileFormat subclasses that have custom magic checks.
	fns = &FileFormatFactoryPrivate::FileFormatFns_mime[0];
	for (; fns->textureInfo != nullptr; fns++) {
		const char *const *sys_mimeTypes = fns->textureInfo()->mimeTypes;
		if (!sys_mimeTypes)
			continue;

		for (; *sys_mimeTypes != nullptr; sys_mimeTypes++) {
			auto iter = set_mimeTypes.find(*sys_mimeTypes);
			if (iter == set_mimeTypes.end()) {
				set_mimeTypes.insert(*sys_mimeTypes);
				vec_mimeTypes.emplace_back(*sys_mimeTypes);
			}
		}
	}

	return vec_mimeTypes;
}

}
