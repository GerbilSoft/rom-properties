/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * FileFormatFactory.cpp: FileFormat factory class.                        *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"

#include "FileFormatFactory.hpp"
#include "fileformat/FileFormat.hpp"

// librpbase
using namespace LibRpBase;

// C++ STL classes.
using std::string;
using std::unordered_set;
using std::vector;

// FileFormat subclasses.
#include "fileformat/DidjTex.hpp"
#include "fileformat/DirectDrawSurface.hpp"
#include "fileformat/KhronosKTX.hpp"
#include "fileformat/KhronosKTX2.hpp"
#include "fileformat/PowerVR3.hpp"
#include "fileformat/SegaPVR.hpp"
#include "fileformat/ValveVTF.hpp"
#include "fileformat/ValveVTF3.hpp"
#include "fileformat/XboxXPR.hpp"

namespace LibRpTexture {

class FileFormatFactoryPrivate
{
	private:
		FileFormatFactoryPrivate();
		~FileFormatFactoryPrivate();

	private:
		RP_DISABLE_COPY(FileFormatFactoryPrivate)

	public:
		// For FileFormat, we assume that the magic number appears
		// at the beginning of the file for all formats.
		// FIXME: TGA format doesn't follow this...
		//typedef int (*pfnIsTextureSupported_t)(const FileFormat::DetectInfo *info);	// TODO
		typedef const char *const * (*pfnSupportedFileExtensions_t)(void);
		typedef const char *const * (*pfnSupportedMimeTypes_t)(void);
		typedef FileFormat* (*pfnNewFileFormat_t)(IRpFile *file);

		struct FileFormatFns {
			//pfnIsTextureSupported_t isTextureSupported;	// TODO
			pfnNewFileFormat_t newFileFormat;
			pfnSupportedFileExtensions_t supportedFileExtensions;
			pfnSupportedMimeTypes_t supportedMimeTypes;

			uint32_t magic;
		};

		/**
		 * Templated function to construct a new FileFormat subclass.
		 * @param klass Class name.
		 */
		template<typename klass>
		static FileFormat *FileFormat_ctor(LibRpBase::IRpFile *file)
		{
			return new klass(file);
		}

#define GetFileFormatFns(format, magic) \
	{/*format::isRomSupported_static,*/ /* TODO */ \
	 FileFormatFactoryPrivate::FileFormat_ctor<format>, \
	 format::supportedFileExtensions_static, \
	 format::supportedMimeTypes_static, \
	 (magic)}

		// FileFormat subclasses that use a header at 0 and
		// definitely have a 32-bit magic number at address 0.
		static const FileFormatFns FileFormatFns_magic[];
};

/** FileFormatFactoryPrivate **/

// FileFormat subclasses that use a header at 0 and
// definitely have a 32-bit magic number at address 0.
// TODO: Add support for multiple magic numbers per class.
const FileFormatFactoryPrivate::FileFormatFns FileFormatFactoryPrivate::FileFormatFns_magic[] = {
	GetFileFormatFns(DirectDrawSurface, 'DDS '),
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

	{nullptr, nullptr, nullptr, 0}
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
	// TODO: Special case for TGA?
	uint32_t magic[2];
	file->rewind();
	size_t size = file->read(&magic, sizeof(magic));
	if (size != sizeof(magic)) {
		// Read error.
		return nullptr;
	}

	// Special check for Khronos KTX, which has the same
	// 32-bit magic number for two completely different versions.
	if (magic[0] == cpu_to_be32('\xABKTX')) {
		FileFormat *fileFormat = nullptr;
		if (magic[1] == cpu_to_be32(' 11\xBB')) {
			// KTX 1.1
			fileFormat = new KhronosKTX(file);
		} else if (magic[1] == cpu_to_be32(' 20\xBB')) {
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

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Magic number needs to be in host-endian.
	magic[0] = be32_to_cpu(magic[0]);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// Check FileFormat subclasses that take a header at 0
	// and definitely have a 32-bit magic number at address 0.
	const FileFormatFactoryPrivate::FileFormatFns *fns =
		&FileFormatFactoryPrivate::FileFormatFns_magic[0];
	for (; fns->supportedFileExtensions != nullptr; fns++) {
		// Check the magic number.
		if (magic[0] == fns->magic) {
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
 * Get all supported file extensions.
 * Used for Win32 COM registration.
 *
 * @return All supported file extensions, including the leading dot.
 */
vector<const char*> FileFormatFactory::supportedFileExtensions(void)
{
	// In order to handle multiple FileFormat subclasses
	// that support the same extensions, we're using
	// an unordered_set<string>.
	//
	// The actual data is stored in the vector<const char*>.
	vector<const char*> vec_exts;
	unordered_set<string> set_exts;

	static const size_t reserve_size = ARRAY_SIZE(FileFormatFactoryPrivate::FileFormatFns_magic);
	vec_exts.reserve(reserve_size);
#ifdef HAVE_UNORDERED_SET_RESERVE
	set_exts.reserve(reserve_size);
#endif /* HAVE_UNORDERED_SET_RESERVE */

	const FileFormatFactoryPrivate::FileFormatFns *fns =
		&FileFormatFactoryPrivate::FileFormatFns_magic[0];
	for (; fns->supportedFileExtensions != nullptr; fns++) {
		const char *const *sys_exts = fns->supportedFileExtensions();
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

	return vec_exts;
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
	for (; fns->supportedFileExtensions != nullptr; fns++) {
		const char *const *sys_mimeTypes = fns->supportedMimeTypes();
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
